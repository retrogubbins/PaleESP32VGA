#pragma once

#include <Arduino.h>
extern byte video_latch;
extern bool freeze_z80;
byte Z80_In (uint16_t Port);

void Z80_Out (uint16_t Port,byte Value);

byte Z80_RDMEM(uint16_t A);

void Z80_WRMEM(uint16_t A,byte V);
