#include <math.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include "zwave.h"

#define ARGBOILER(ARG) \
  ARG(ARG_INT0,now,"n","now","level to dim to immediately (0--99)",-1) \
  ARG(ARG_DBL0,now_fade,"x","now-fade","upper bound on 'immediately'",0.2) \
  ARG(ARG_DBL0,ramp_time,"r","ramp","number of seconds to smoothly ramp over",300.0) \
  ARG(ARG_INT0,ramp_from,"f","from","level to ramp from (0--99)",0) \
  ARG(ARG_INT0,ramp_to,"t","to","level to ramp to (0--99)",99) \
  ARG(ARG_DBL0,ramp_ec,"c","ec","exponent correction parameter",1.0) \
  ARG(ARG_INT0,blink,"b","blink","number of blinks (0 for indefinite)",-1) \
  ARG(ARG_DBL0,period,"p","blink-period","blink period (seconds)",1.0) \
  ARG(ARG_LIT0,end_off,"o","blink-off","blink sequence ends off",0) \
  ARG(ARG_LIT0,blink_fade,"y","blink-fade","use the -x value to fade blinks",0) \
  ARG(ARG_DBL0,duty,"d","blink-duty","blink duty cycle",0.5) \
  ARG(ARG_FIL0,dev_fname,"u","usb","specify USB device","/dev/ttyUSB0") \
  ARG(ARG_INT0,unit,"z","unit","Z-Wave Unit number to control",2) \
  ARG(ARG_LIT0,on,"1","on","Turn on switch",0) \
  ARG(ARG_LIT0,off,"0","off","Turn off switch",0) \
  ARG(ARG_LIT0,verbose,"v","verbose","blabber",0) \

#include "argboiler.h"

void mark(struct timespec *stp) {
  clock_gettime(CLOCK_REALTIME,stp);
}

double elapsed(struct timespec s_t) {
  struct timespec e_t;
  clock_gettime(CLOCK_REALTIME,&e_t);
  return (e_t.tv_sec-s_t.tv_sec)+((e_t.tv_nsec-s_t.tv_nsec)*1e-9);
}

void set_xfade(int zwave, unsigned char unit, double x) {
  double step_size = 0.99/x;
  unsigned char jump_size;
  if(step_size<1.0) jump_size=1;
  else if(step_size>99.0) jump_size=99;
  else jump_size=(unsigned char)(0.5+step_size);
  double step_time = jump_size*x/99.0;
  unsigned char jump_length = (unsigned char)( 100*step_time +0.5 );
  zwave_param(zwave,unit,GE_45602_FADE_JUMP_SIZE,jump_size);
  zwave_param(zwave,unit,GE_45602_FADE_JUMP_LENGTH,jump_length);
}

void set_instant(int zwave, unsigned char unit) {
  zwave_param(zwave,unit,GE_45602_FADE_JUMP_SIZE,99);
  zwave_param(zwave,unit,GE_45602_FADE_JUMP_LENGTH,1);
}

inline double myfmod(double x, double y) {
  double fm = fmod(x,y);
  if(fm<0) fm+=y;
  return fm;
}

int main(int argc, char **argv) {
  args_t args;
  parse_args(argc,argv,&args);
  int zwave=zwave_open(args.dev_fname);
  if(args.on) {
    zwave_on(zwave,args.unit);
  } else if (args.off) {
    zwave_off(zwave,args.unit);
  } else if(args.now>=0 && args.now<=99) {
    set_xfade(zwave,args.unit,args.now_fade);
    zwave_dim(zwave,args.unit,args.now);
  } else if(args.blink>=0) {
    if(args.blink_fade) {
      double x = args.now_fade*(99/(double)(args.ramp_to-args.ramp_from));
      set_xfade(zwave,args.unit,x);
    } else {
      set_instant(zwave,args.unit);
    }
    zwave_dim(zwave,args.unit,args.ramp_from);
    int i=0;
    struct timespec hw;
    mark(&hw);
    do {
      usleep(myfmod((args.period*(1-args.duty)-elapsed(hw)),args.period)*1e6);
      mark(&hw);
      zwave_dim(zwave,args.unit,args.ramp_to);
      if(i+1!=args.blink || args.end_off) {
        usleep(myfmod((args.period*(args.duty)-elapsed(hw)),args.period)*1e6);
        mark(&hw);
        zwave_dim(zwave,args.unit,args.ramp_from);
      }
      ++i;
    } while(i!=args.blink);
  } else {
    set_instant(zwave,args.unit);
    zwave_dim(zwave,args.unit,args.ramp_from);

    double levels_per_s = abs(args.ramp_to - args.ramp_from) / args.ramp_time;
    unsigned char jump_length = 1;
    zwave_param(zwave,args.unit,GE_45602_FADE_JUMP_SIZE,1);

    struct timespec start_time,cmd_slot;
    mark(&start_time);
    unsigned char current_uc=args.ramp_from;
    double current_level=(double)current_uc;
    unsigned char prev_level=254;
    int i=0;
    while(current_uc != args.ramp_to) {
      double t,x;
      do {
        t = elapsed(start_time);
        x = t/args.ramp_time;
        if(x>1.0) x=1.0;
        double cc=args.ramp_ec;
        current_level=-cc+exp(x*log((double)args.ramp_to+cc)+(1-x)*log((double)args.ramp_from+cc));
        levels_per_s = (current_level+cc)*(log((double)args.ramp_to+cc)-log((double)args.ramp_from+cc));
        unsigned char new_jump_length = (unsigned char)( (100/levels_per_s) +0.5);
        new_jump_length=new_jump_length?new_jump_length:1;
        if(new_jump_length!=jump_length) {
          jump_length=new_jump_length;
          zwave_param(zwave,args.unit,GE_45602_FADE_JUMP_LENGTH,jump_length);
        }
        current_uc=(unsigned char)(current_level+0.5);
        if(current_uc==prev_level) usleep(1000);
      } while(current_uc==prev_level);
      prev_level=current_uc;
      mark(&cmd_slot);

      if(args.verbose) {
        printf("Step %d: t=%lfs, x=%lf/1.0, current level=%lf/99.0 (%hhd/99)\n",i++,t,x,current_level,current_uc);
      }

      zwave_dim(zwave,args.unit,current_uc);
      double cmd_t = elapsed(cmd_slot);

      if(args.verbose) {
        printf("  Command took %lfs\n",cmd_t);
      }
    }
  }
  zwave_close(zwave);
  return 0;
}
