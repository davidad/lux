#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#define ZWAVE_SEND_DATA 0x13
#define ZWAVE_BASIC_CLASS 0x20
#define ZWAVE_BASIC_SET 0x1
#define ZWAVE_CONFIG_CLASS 0x70
#define ZWAVE_CONFIG_SET 0x4
#define ZWAVE_CONFIG_ONE_BYTE_VALUE 0x1
#define ZWAVE_ROUTE_ACK 0x5

#define GE_45602_FADE_JUMP_SIZE 0x7   // Parameter value given in steps, 0-99
#define GE_45602_FADE_JUMP_LENGTH 0x8 // Parameter value given in 10ms increments, 0-255


int zwave_open(const char*);
int zwave_send(int, const unsigned char*, unsigned char);
int zwave_send_retry(int, const unsigned char*, unsigned char);
int zwave_listen(int);
#define zwave_close close

int zwave_param(int, unsigned char, unsigned char, unsigned char);
int zwave_dim(int, unsigned char, unsigned char);
