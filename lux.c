#include <math.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include "zwave.h"

#define ARGBOILER(ARG) \
  ARG(ARG_INT0,now,"n","now","level to dim to immediately (0--99)",-1) \
  ARG(ARG_DBL0,now_fade,"x","now-fade","upper bound on 'immediately'",0.2) \
  ARG(ARG_DBL0,ramp_time,"r","time","number of seconds to smoothly ramp over",60.0) \
  ARG(ARG_INT0,ramp_from,"f","from","level to ramp from (0--99)",0) \
  ARG(ARG_INT0,ramp_to,"t","to","level to ramp to (0--99)",99) \
  ARG(ARG_DBL0,ramp_ec,"c","ec","exponent correction parameter",1.0) \
  ARG(ARG_INT0,blink,"b","blinks","number of blinks (0 for indefinite)",-1) \
  ARG(ARG_DBL0,period,"p","blink-period","blink period (seconds)",1.0) \
  ARG(ARG_LIT0,end_off,"o","blink-off","blink sequence ends off",0) \
  ARG(ARG_LIT0,blink_fade,"y","blink-fade","use the -x value to fade blinks",0) \
  ARG(ARG_DBL0,duty,"d","blink-duty","blink duty cycle",0.5) \
  ARG(ARG_INT0,unit,"u","unit","Z-Wave Unit number to control",2) \
  ARG(ARG_LIT0,verbose,"v","verbose","blabber",0) \
//ARG(ARG_DBL0,ramp_steps,"s","steps","max steps per second",5) \

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
  unsigned char param7;
  if(step_size<1.0) param7=1;
  else if(step_size>99.0) param7=99;
  else param7=(unsigned char)(0.5+step_size);
  double step_time = param7*x/99.0;
  unsigned char param8 = (unsigned char)( 100*step_time +0.5 );
  zwave_param(zwave,unit,7,param7);
  zwave_param(zwave,unit,8,param8);
}

void set_instant(int zwave, unsigned char unit) {
  zwave_param(zwave,unit,7,99);
  zwave_param(zwave,unit,8,1);
}

inline double myfmod(double x, double y) {
  double fm = fmod(x,y);
  if(fm<0) fm+=y;
  return fm;
}

int main(int argc, char **argv) {
  args_t args;
  parse_args(argc,argv,&args);
  int zwave=zwave_open("/dev/ttyUSB0");
  if(args.now>=0 && args.now<=99) {
    set_xfade(zwave,args.unit,args.now_fade);
    zwave_dim(zwave,args.unit,args.now);
  } else if(args.blink>=0) {
    if(args.blink_fade) {
      set_xfade(zwave,args.unit,args.now_fade);
    } else {
      set_instant(zwave,args.unit);
    }
    zwave_dim(zwave,args.unit,0);
    int i=0;
    struct timespec hw;
    mark(&hw);
    do {
      usleep(myfmod((args.period*(1-args.duty)-elapsed(hw)),args.period)*1e6);
      mark(&hw);
      zwave_dim(zwave,args.unit,99);
      if(i+1!=args.blink || args.end_off) {
        usleep(myfmod((args.period*(args.duty)-elapsed(hw)),args.period)*1e6);
        mark(&hw);
        zwave_dim(zwave,args.unit,0);
      }
      ++i;
    } while(i!=args.blink);
  } else {
    set_instant(zwave,args.unit);
    zwave_dim(zwave,args.unit,args.ramp_from);

    double levels_per_s = abs(args.ramp_to - args.ramp_from) / args.ramp_time;
    unsigned char param8 = 1;
    zwave_param(zwave,args.unit,7,1);

    struct timespec start_time,cmd_slot;
    mark(&start_time);
    unsigned char current_uc=args.ramp_from;
    double current_level=(double)current_uc;
    unsigned char prev_level=254;
    //double slot_length = 1.0/args.ramp_steps;
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
        unsigned char new_param8 = (unsigned char)( (100/levels_per_s) +0.5);
        new_param8=new_param8?new_param8:1;
        if(new_param8!=param8) {
          param8=new_param8;
          zwave_param(zwave,args.unit,8,param8);
        }
        current_uc=(unsigned char)(current_level+0.5);
        if(current_uc==prev_level) usleep(1000);
      } while(current_uc==prev_level);
      prev_level=current_uc;
      mark(&cmd_slot);

      if(args.verbose) {
        printf("Step %d: t=%lfs, x=%lf/1.0, current level=%lf/99.0 (%hhd/99)\n",i,t,x,current_level,current_uc);
      }

      zwave_dim(zwave,args.unit,current_uc);
      double cmd_t = elapsed(cmd_slot);

      if(args.verbose) {
        printf("  Command took %lfs\n",cmd_t);
      }

      //if(slot_remaining>0) usleep(slot_remaining*1e6);

      i++;
    }
  }
  zwave_close(zwave);
  return 0;
}