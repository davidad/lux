#include "zwave.h"

#define ARGBOILER(ARG) \
  ARG(ARG_FIL0,dev_fname,"u","usb","specify USB device","/dev/ttyUSB0") \
  ARG(ARG_INT0,unit,"z","unit","Z-Wave Unit number to control",3) \
  ARG(ARG_LIT0,on,"1","on","Turn on kettle",0) \
  ARG(ARG_LIT0,off,"0","off","Turn off kettle",0) \
  ARG(ARG_LIT0,verbose,"v","verbose","blabber",0) \

#include "argboiler.h"

int main(int argc, char **argv) {
  args_t args;
  parse_args(argc,argv,&args);
  int zwave=zwave_open(args.dev_fname);
  if(args.on || !args.off) {
    zwave_on(zwave,args.unit);
  } else {
    zwave_off(zwave,args.unit);
  }
  zwave_close(zwave);
  return 0;
}
