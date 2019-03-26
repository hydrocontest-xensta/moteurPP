/**
 * @file moteurpp.c
 * @author g.roussel
 * @brief Lib pour controler le moteur PP
 * @version 0.1
 * @date 2019-03-26
 *
 * @copyright Copyright G. Roussel, 2019. Under GNU GPLv3 licence
 *
 * CANOPEN Telegrame are in the form (one line per byte)
 * 'S' SOF (Start of Frame)
 * user data length (from this byte to the one before 'E'). It's `telegram size`
 * - 2 node numder Command code Data
 * ...
 * Data
 * CRC
 * 'E' EOF (End of Frame)
 *
 * message[user_data_length] gives the CRC code
 *
 * max telegram size is 64 byte
 *
 */
#include <string.h>

#include "cmsis_os.h"

#include "main.h"
#include "usart.h"

#include "moteurpp.h"

#define MPP_MOTOR_NODE_NUMBER 1
#define MPP_MOTOR_HUART huart2
#define MPP_UART_TIMEOUT 0xFF
#define MPP_MAX_CHAR_POLLS 128

enum telegram_commands
{
  BOOT_UP = 0x00,
  SDO_READ = 0x01,
  STD_WRITE = 0x02,
  CONTROL_WORD = 0x04,
  STATUS_WORD = 0x05
};

/**
 * @brief Protocol constants
 */
enum telegram_internals
{
  /**
   * @brief Datasheet
   */

  CRC_POLYNOMIAL = 0x5D,
  CRC_INIT_VALUE = 0xFF,
  SOF = 'S',
  EOF = 'E'
};

/**
 * @brief CRC calculation
 *
 * Given by Faulhaber documentation
 *
 * @param[in] u8Byte current byte
 * @param[in] u8CRC value of CRC for preceeding bytes
 *
 */
uint8_t
mpp_calc_CRC(uint8_t u8Byte, uint8_t u8CRC)
{
  uint8_t i;
  u8CRC = u8CRC ^ u8Byte;
  for (i = 0; i < 8; i++) {
    if (u8CRC & 0x01) {
      u8CRC = (u8CRC >> 1) ^ CRC_POLYNOMIAL;
    } else {
      u8CRC >>= 1;
    }
  }
  return u8CRC;
}

enum mpp_return
mpp_send_telegram(uint8_t* telegram, uint8_t telegram_size)
{
  HAL_StatusTypeDef status;
  status =
    HAL_UART_Transmit(&huart2, telegram, telegram_size, MPP_UART_TIMEOUT);
  if (status == HAL_OK) {
    return MPP_OK;
  } else {
    return MPP_ERROR_HAL;
  }
}

enum mpp_return
mpp_send_payload(struct mpp_payload payload)
{
  uint8_t telegram[MAX_TELEGRAM_SIZE_bytes];
  uint8_t telegram_size = 0;
  enum mpp_return status_code;
  status_code = mpp_build_telegram(payload, telegram, &telegram_size);
  if (status_code != MPP_OK) {
    return status_code;
  }
  status_code = mpp_send_telegram(telegram, telegram_size);
  return status_code;
}

enum mpp_return
mpp_sync_order(struct mpp_payload order_in, struct mpp_payload* response_out)
{
  // we may need to clear uart buffer in the future
  uint8_t telegram[MAX_TELEGRAM_SIZE_bytes];
  uint8_t telegram_size = 0;

  //* SEND PHASE
  enum mpp_return status_code;
  status_code = mpp_build_telegram(order_in, telegram, &telegram_size);
  if (status_code != MPP_OK) {
    return status_code;
  }
  status_code = mpp_send_telegram(telegram, telegram_size);
  if (status_code != MPP_OK) {
    return status_code;
  }

  //* RECEIVE PHASE
  status_code = mpp_receive_telegram(telegram, &telegram_size);
  if (status_code != MPP_OK) {
    return status_code;
  }
  status_code = mpp_parse_telegram(telegram, telegram_size, response_out);
  if (status_code != MPP_OK) {
    return status_code;
  }
}

enum mpp_return
mpp_receive_telegram(uint8_t* telegram_dest, uint8_t* telegram_size_out)
{
  uint8_t rx_char = 0;
  uint8_t telegram_ix = 0;
  HAL_StatusTypeDef status;
  // wait for valid start bit
  while (rx_char != SOF) {
    HAL_UART_Receive(&MPP_MOTOR_HUART, &rx_char, 1, MPP_UART_TIMEOUT);
  }
  telegram_dest[telegram_ix] = SOF;

  // store intermediate bits
  rx_char = 0;
  while (rx_char != EOF && telegram_ix < MAX_TELEGRAM_SIZE_bytes) {
    status = HAL_UART_Receive(&MPP_MOTOR_HUART, &rx_char, 1, MPP_UART_TIMEOUT);
    if (status == HAL_OK) {
      telegram_dest[telegram_ix] = rx_char;
      ++telegram_ix;
    } else if (status = HAL_ERROR) {
      return MPP_ERROR_HAL;
    }
    if (telegram_ix == MAX_TELEGRAM_SIZE_bytes) {
      //   we reached end of buffer
      // the message is invalid, unfinished
      // let's just add a 0 to avoid endless string.
      telegram_dest[MAX_TELEGRAM_SIZE_bytes - 1] = 0;
      return MPP_ERROR_MSG;
    }
  }
  // for sake of security, adding a `\0` char
  telegram_dest[telegram_ix] = 0;

  // at this point, last rx_char is EOF and it was stored
  // string ends after it
  // exiting gracefully
  *telegram_size_out = telegram_ix;
  return MPP_OK;
}

// enum mpp_return
// receiveMessage(char* msg_dest, int8_t buffer_size)
// {
//   int8_t max_len = buffer_size - 1;
//   uint8_t* buffer = (uint8_t*)msg_dest;
//   uint8_t rx_char = 0;
//   uint8_t receive_attempts = 0;
//   int8_t buffer_ix = 0;
//   HAL_StatusTypeDef status;
//   // wait for valid start bit
//   while (rx_char != 's') {
//     HAL_UART_Receive(&huart2, &rx_char, 1, 0xFF);
//   }
//   // store intermediate bits
//   rx_char = 0;
//   while (rx_char != EOF) {
//     if (buffer_ix == max_len || receive_attempts > MPP_MAX_CHAR_POLLS) {
//       //   we reached end of buffer
//       // the message is invalid, unfinished
//       // let's just add a 0 to avoid endless string.
//       buffer[max_len] = 0;
//       return -1;
//     }

//     ++receive_attempts;
//     status = HAL_UART_Receive(&huart2, &rx_char, 1, 0xFF);
//     if (status == HAL_OK) {
//       buffer[buffer_ix] = rx_char;
//       ++buffer_ix;
//     }
//   }

//   // otherwise let's replace 't' by a \0
//   // --buffer_ix;
//   buffer[buffer_ix] = 0;

//   //   return length of message (without terminating 0)
//   return buffer_ix;
// }

enum mpp_return
mpp_parse_telegram(uint8_t const* message,
                   int8_t message_len,
                   struct mpp_payload* payload_out)
{
  int8_t user_data_length = message[1];
  // check that we were given a buffer long enough to contain the message in
  // full
  if (message_len < user_data_length + 2) {
    return MPP_ERROR_MSG;
  }
  // check that start and end of frame chars are at their places
  if (message[0] != SOF || message[user_data_length + 1] != EOF) {
    return MPP_ERROR_MSG;
  }
  payload_out->data_size = user_data_length - 2;
  payload_out->node_index = message[2];
  payload_out->command_code = message[3];
  memcpy(payload_out->data, message[4], payload_out->data_size);
  payload_out->crc = message[user_data_length];
  // check CRC
  uint8_t crc = CRC_INIT_VALUE;
  for (uint8_t i = 1; i < user_data_length; i++) {
    crc = mpp_calc_CRC(message[i], crc);
  }
  if (payload_out->crc != crc) {
    return MPP_ERROR_CRC;
  }
  return MPP_OK;
}

enum mpp_return
mpp_build_telegram(struct mpp_payload in,
                   uint8_t* message_out,
                   uint8_t* message_size_out)
{
  // we're not expecting the crc
  uint8_t user_data_length = in.data_size + 2;

  message_out[0] = SOF;
  message_out[1] = MPP_MOTOR_NODE_NUMBER;
  message_out[2] = user_data_length;
  message_out[3] = in.command_code;
  memcpy(message_out[4], in.data, in.data_size);

  // check CRC
  uint8_t crc = CRC_INIT_VALUE;
  for (uint8_t i = 1; i < user_data_length; i++) {
    crc = mpp_calc_CRC(message_out[i], crc);
  }

  message_out[user_data_length] = crc;
  message_out[user_data_length + 1] = EOF;

  *message_size_out = user_data_length + 2;
  return MPP_OK;
}