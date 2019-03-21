#include <string.h>

#include "cmsis_os.h"

#include "main.h"
#include "usart.h"

#include "moteurpp.h"

#define MAX_MESSAGE_LEN 32
#define MOTOR_NODE_NUMBER 1
#define CRC_POLYNOMIAL 0x5D
#define MOTOR_HUART huart2

/**
 * @brief CRC calculation
 *
 * Given by Faulhaber documentation
 *
 * @param[in] u8Byte current byte
 * @param[in] u8CRC value of CRC for preceeding bytes
 *
 */
CalcCRCByte(uint8_t u8Byte, uint8_t u8CRC)
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

int8_t
sendMessage(uint8_t* payload, int8_t payload_size)
{
  char msg[MAX_MESSAGE_LEN];
  HAL_StatusTypeDef status;
  // message should be specified without 's' and 't',
  // first byte should be 'command'
  // we need to allocate payload_size+5 bytes
  // - 's'
  // - user data length
  // - node Number
  // - message (payload_size bytes)
  // - CRC
  // - 't'
  if (payload_size + 5 > MAX_MESSAGE_LEN) {
    return -1;
  }
  // payload + length byte + node number + CRC
  int8_t user_data_length = payload_size + 3;
  uint8_t message_length = user_data_length + 2;

  msg[0] = 's';
  msg[1] = user_data_length;
  msg[2] = MOTOR_NODE_NUMBER;
  for (size_t i = 0; i < payload_size; i++) {
    msg[3 + i] = payload[i];
  }
  // computing CRC
  uint8_t curr_CRC = 0xFF;
  for (size_t i = 1; i < payload_size + 3; i++) {
    curr_CRC = CalcCRCByte(msg[i], curr_CRC);
  }

  msg[3 + payload_size] = curr_CRC;
  msg[4 + payload_size] = 't';

  status = HAL_UART_Transmit(&huart2, msg, message_length, 0xFFFF);
  if (status != HAL_OK) {
    return -2;
  } else {
    return message_length;
  }
}

int8_t synchronous_request(uint8_t *payload, uint8_t payload_size, uint8_t *response, uint8_t response_buffer_size, uint8_t * actual_response_size_out){
    int8_t send_status = sendMessage(payload, payload_size);
    if (send_status < 0){
        return -1;
    }
    int8_t receive_status = receiveMessage(response, response_buffer_size);
    if (receive_status < 0)
    {
        return -2;
    }
    *actual_buffer_size_out = receive_status;
}

int8_t sendReadSDO(uint8_t ind_LB, uint8_t ind_HB, uint8_t sub){
    char payload[4];
    payload[0] = 0x01; //read command
    payload[1] = ind_LB;
    payload[2] = ind_HB;
    payload[3] = sub;
    sendMessage(payload, 4);
}

int8_t
receiveMessage(char* msg_dest, int8_t buffer_size)
{
  int8_t max_len = buffer_size - 1;
  uint8_t* buffer = (uint8_t*)msg_dest;
  uint8_t rx_char = 0;
  int8_t buffer_ix = 0;
  HAL_StatusTypeDef status;
  // wait for valid start bit
  while (rx_char != 's') {
    HAL_UART_Receive(&huart2, &rx_char, 1, 0xFF);
  }
  // store intermediate bits
  rx_char = 0;
  while (rx_char != 't' && buffer_ix < max_len) {
    status = HAL_UART_Receive(&huart2, &rx_char, 1, 0xFF);
    if (status == HAL_OK) {
      buffer[buffer_ix] = rx_char;
      ++buffer_ix;
    }
  }
  if (buffer_ix == max_len) {
    //   we reached end of buffer
    // the message is invalid, unfinished
    // let's just add a 0 to avoid endless string.
    buffer[max_len] = 0;
    return -1;
  }
  // otherwise let's replace 't' by a \0
  // --buffer_ix;
  buffer[buffer_ix - 1] = 0;

  //   return length of message (without terminating 0)
  return buffer_ix - 1;
}