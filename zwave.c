#include "zwave.h"

int zwave_open(const char* fname) {
  char stty_cmd[4096];
  snprintf(stty_cmd,sizeof(stty_cmd),"stty -F %s 115200 cs8 -cstopb -parenb -crtscts -echo -icanon -isig -iexten -icrnl -imaxbel -brkint ignbrk line 0 min 0 time 0",fname);
  system(stty_cmd);
  return open(fname,O_RDWR|O_NOCTTY|O_NONBLOCK);
}

int zwave_send(int dev, const unsigned char* command, unsigned char length) {
  unsigned char bytes[length+3];
  bytes[0]=0x1;
  bytes[1]=length+1;
  int i;
  unsigned char checksum = 0xff ^ bytes[1];
  for(i=0;i<length;i++) {
    bytes[2+i]=command[i];
    checksum^=command[i];
  }
  bytes[length+2]=checksum;

  int rc;
  rc = write(dev,bytes,length+3);
  return rc;
}

int zwave_send_retry(int dev, const unsigned char* command, unsigned char length) {
  int tries = 8;
  do { 
    if(tries<8) usleep(80000);
    zwave_send(dev,command,length);
  } while(zwave_listen(dev) && tries--);
  return 0;
}

int zwave_listen(int dev) {
  char response[4096];
  int rc,n=0;
  fd_set fds_r, fds_w, fds_e;
  FD_ZERO(&fds_r);
  FD_ZERO(&fds_w);
  FD_ZERO(&fds_e);
  FD_SET(dev,&fds_r);
  FD_SET(dev,&fds_e);
  struct timeval wait = {.tv_sec=0,.tv_usec=120000};
  do { 
    select(1,&fds_r,&fds_w,&fds_e,&wait);
    rc = read(dev,response+n,1);
    if(rc>0) n+=rc;
  } while (rc>0);
  int i;
  for(i=0;i<n;i++) {
    //printf("%hhX ",response[i]);
  }
  //printf("\n");
  for(i=0;i<n;i++) {
    if(response[0]==0x1 || response[1]==0x1) {
      char ack[] = {0x6};
      write(dev,ack,sizeof(ack));
    }
  }
  if(response[0]!=0x18) return 0;
  else return 1;
}

int zwave_param(int dev, unsigned char unit, unsigned char param, unsigned char value) {
  unsigned char command[] =
    {0x0, ZWAVE_SEND_DATA, unit, 5,
          ZWAVE_CONFIG_CLASS, ZWAVE_CONFIG_SET, param, ZWAVE_CONFIG_ONE_BYTE_VALUE, value,
          ZWAVE_ROUTE_ACK};
  return zwave_send_retry(dev,command,sizeof(command));
}

int zwave_dim(int dev, unsigned char unit, unsigned char level) {
  unsigned char command[] =
    {0x0, ZWAVE_SEND_DATA, unit, 3,
          ZWAVE_BASIC_CLASS, ZWAVE_BASIC_SET, level,
          ZWAVE_ROUTE_ACK};
  return zwave_send_retry(dev,command,sizeof(command));
}
