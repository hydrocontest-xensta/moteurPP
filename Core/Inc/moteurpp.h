#if !defined(MOTEURPP_H)
#define MOTEURPP_H

#include "cmsis_os.h"

int8_t
receiveMessage(char* msg_dest, int8_t buffer_size);
#endif // MOTEURPP_H