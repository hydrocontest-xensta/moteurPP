#if !defined(MOTEURPP_H)
#define MOTEURPP_H

#include "cmsis_os.h"
#include "usart.h"

enum mpp
{
  MAX_TELEGRAM_SIZE_bytes = 64,
};

enum mpp_return
{
  MPP_ERROR_HAL = -1,
  MPP_ERROR_MSG = -2,
  MPP_ERROR_CRC = -3,
  MPP_OK = 0
};

struct mpp_payload
{
  uint8_t node_index;
  uint8_t command_code;
  uint8_t data_size;
  uint8_t data[MAX_TELEGRAM_SIZE_bytes - 6];
  uint8_t crc;
};

struct status_word
{
  uint8_t ready_switch_on:1;
  uint8_t switched_on:1;
  uint8_t operation_enabled:1;
  uint8_t fault:1;
  uint8_t quick_stop:1;
  uint8_t switch_on_disabled:1;
};

struct mpp_config
{
  UART_HandleTypeDef huart;
  
}

// *Low-level API
uint8_t
mpp_calc_CRC(uint8_t u8Byte, uint8_t u8CRC);
enum mpp_return
mpp_send_telegram(uint8_t* telegram, uint8_t telegram_size);
enum mpp_return
mpp_send_payload(struct mpp_payload payload);
enum mpp_return
mpp_sync_order(struct mpp_payload order_in, struct mpp_payload* response_out);
enum mpp_return
mpp_receive_telegram(uint8_t* telegram_dest, uint8_t* telegram_size_out);
enum mpp_return
mpp_parse_telegram(uint8_t const* message,
                   int8_t message_len,
                   struct mpp_payload* payload_out);
enum mpp_return
mpp_build_telegram(struct mpp_payload in,
                   uint8_t* message_out,
                   uint8_t* message_size_out);
#endif // MOTEURPP_H