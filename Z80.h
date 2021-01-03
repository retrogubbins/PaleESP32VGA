#ifndef _Z80_H
 #define _Z80_H
 
#include <Arduino.h>
 
 #define N_FLAG 0x80
 #define Z_FLAG 0x40
 #define H_FLAG 0x10
 #define P_FLAG 0x04
 #define V_FLAG 0x04
 #define S_FLAG 0x02
 #define C_FLAG 0x01
 
 void initz80();
 void resetz80();
 void loadroms();
 int execz80(int);
 
extern byte speed_mult;
extern unsigned int base_speed_cycles;
extern int intreq;
extern bool stop_z80;

#endif
 
