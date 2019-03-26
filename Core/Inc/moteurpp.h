#if !defined(MOTEURPP_H)
#define MOTEURPP_H

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