#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int zwave_open(char*);
int zwave_send(int, unsigned char*, unsigned char);
int zwave_send_retry(int, unsigned char*, unsigned char);
int zwave_listen(int);
#define zwave_close close

int zwave_param(int, unsigned char, unsigned char, unsigned char);
int zwave_dim(int, unsigned char, unsigned char);
