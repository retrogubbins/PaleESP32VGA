// Pale ESP32 IO routines

#include <Arduino.h>

#include "lynxConfig.h"

#include "Z80.h"
#include "LynxRom.h"
#include "hardware.h"

#ifdef enable_disks
    #include "PALEDISK.H"
    #include "LynxDOSRom1-0.h"
#endif

extern byte z80ports_in[16];
extern byte *bank1;
extern byte *bank2;
extern byte *bank3;

byte bank_latch = 0x00;
byte video_latch = 0x00;
byte speaker_enable = 0x0;
bool freeze_z80 = false;
byte Line_Blank = 0;

byte Z80_In (uint16_t Port)
{
  if((Port & 0x00ff)  == 0x0080)
  {
    int16_t kbdportvalmasked = (Port & 0x0f00);
    int16_t kbdarrno = kbdportvalmasked >> 8;
    return( z80ports_in[kbdarrno] );
  }

#ifdef enable_disks
    if((Port & 0xf0)==0x50)
      return(disk_inp(Port));
#endif
  
  return 0xff;
}


// Z80  OUTPUT
// Z80  OUTPUT
// Z80  OUTPUT
// Z80  OUTPUT
// Z80  OUTPUT

void Z80_Out (uint16_t Port,byte Value)
{
  Port = Port & 0xFF;
  switch (Port) 
  {
    case 0x7F:
    {
      bank_latch = Value;

//FIXME  bank latch has bit 2? is read 23 enable
//    if ((value & 0x40)==0x40)     //READ 2 & 3 Enable 
//        update_vid_maps96k();
      
      
      break;
    }
    case 0x80:
    {
      speaker_enable = Value & 0x01;
      digitalWrite(SPEAKER_PIN,speaker_enable);
      video_latch = Value;
      if ((Value & 0x40) && Line_Blank==0)  //FIXME synch with video draw
      {   //Line Blanking monostable - freezes z80 till next scanline end
          Line_Blank=1;
          stop_z80 = true;
      }  
      
      
      break;
    }
    case 0x84:
    {
      if(speaker_enable == 1)
      {
        if(Value > 32)
            digitalWrite(SPEAKER_PIN,HIGH);
        else
            digitalWrite(SPEAKER_PIN,LOW);
      }
    }
  }
#ifdef enable_disks
  if((Port & 0xf0) == 0x50)
      disk_outp(Port, Value);
#endif
  
}


byte Z80_RDMEM(uint16_t A)
{
  if((bank_latch & 0x10) == 0x00)
  {
    if (A < 0x6000)
    {
//        Serial.print("Returning ");
//        Serial.println(A);
// if(A == 0xc95)Serial.print("hitc95 ");//return(0xc3);  //set up an infint loop to wait here whilst VB is loading the RAM
// if(A == 0xc96)return(0x95);
// if(A == 0xc97)return(0x0c);
// if(A == 0x3f62)return(0xc3);  //set up an infint loop to wait here whilst VB is loading the RAM
// if(A == 0x3f63)return(0x62);
// if(A == 0x3f64){Serial.print("tapload ");return(0x3f);}
        return lynxrom[A];
    }
#ifdef enable_disks
    if(A >= 0xe000 && disk_rom_enabled)
    {
            return dosrom[A-0xE000];   // DISK ROM
    }       
#endif
  }

  if((bank_latch & 0x20) == 0x00)  // BANK 1  user ram
  {
    return bank1[A];
  }

  if((bank_latch & 0x40) == 0x40)   // BANK 2 video 
  {
    if((video_latch & 0x04) == 0x00)
    {
        if(A<0x4000 || (A >= 0x8000 && A < 0xc000))
          return bank2[A % 0x2000];        // mirror
        else //if((A>=0x4000 && A<0x8000) || A >=0xc000)
          return bank2[0x2000+(A % 0x2000)];       // BLUE RED
    }
    if((video_latch & 0x08) == 0x00)
    {
        if(A<0x4000 || (A >= 0x8000 && A < 0xc000))
          return bank3[A % 0x2000];        // mirror
        else //if((A>=0x4000 && A<0x8000) || A >=0xc000)
          return bank3[0x2000+(A % 0x2000)];       // GREEN  ALTGREEN
    }
  }
 
  return 0xff;        
}

void Z80_WRMEM(uint16_t A,byte V)
{
    if((bank_latch & 0x01)==0)
    {      
        bank1[A]=V;
    }
    if((bank_latch & 0x02)==0x02)
    {
      if ((video_latch & 0x04)==0) 
      {
        if(A<0x4000 || (A >= 0x8000 && A < 0xc000))
          bank2[A % 0x2000]=V;        // mirror
        else //if((A>=0x4000 && A<0x8000) || A >=0xc000)
          bank2[0x2000+(A % 0x2000)]=V;       // BLUE RED
 
      }
    }
    if((bank_latch & 0x04)==0x04)
    {
      if ((video_latch & 0x08)==0) 
      {
       if(A<0x4000 || (A >= 0x8000 && A < 0xc000))
          bank3[A % 0x2000]=V;        // mirror
        else //if((A>=0x4000 && A<0x8000) || A >=0xc000)
         bank3[0x2000+(A % 0x2000)]=V;       //AGREEN  GREEN
      }
    }
}




void k_delay(int del);
 
void pump_key(char k)
{
  int f;
//  int PUMP_DELAY=2000;
 

//  if key[SDLK_CAPSLOCK]) z80ports_in[0x0080] &= 0xF7;
  if (k == 0x83){ z80ports_in[0] &= 0xEF; k_delay(4); return;}
  if (k == 0x81){ z80ports_in[0] &= 0xDF; k_delay(4); return;}
  if  (k == 0x80){z80ports_in[9] &= 0xFB; k_delay(4); return;}
  if  (k == 0x82){z80ports_in[9] &= 0xDF; k_delay(4); return;}
//  if key[SDLK_ESCAPE]) z80ports_in[0x0080] &= 0xBF;
//  if ((key[SDLK_RSHIFT]) || (key[SDLK_LSHIFT])) z80ports_in[0x0080] &= 0x7F;


    k=tolower (k);
     
  // Real Keyboard Table        
  if (k=='1') z80ports_in[0] &= 0xFE;
  if (k=='!')
  {
    z80ports_in[0] &= 0x7F;//SHIFT
    z80ports_in[0] &= 0xFE;
  }

  if (k=='3') z80ports_in[1] &= 0xFE; // 01
  if(k=='#')
  {
    z80ports_in[0] &= 0x7F;//SHIFT
    z80ports_in[1] &= 0xFE;// 3
  }
  if (k=='4') z80ports_in[1] &= 0xFD;
  if(k=='$')
  {
    z80ports_in[0] &= 0x7F;//SHIFT
    z80ports_in[1] &= 0xFD;// 4
  }
// 02
  if (k=='e') z80ports_in[1] &= 0xFB;
// 04
  if (k=='x') z80ports_in[1] &= 0xF7;
// 08
  if (k=='d') z80ports_in[1] &= 0xEF;
// 10
    if (k=='c') z80ports_in[1] &= 0xDF; // 20
  if (k=='2') z80ports_in[2] &= 0xFE;
  if(k=='\"')
  {
    z80ports_in[0] &= 0x7F;//SHIFT
    z80ports_in[2] &= 0xFE;// 2
  }

  if (k=='q') z80ports_in[2] &= 0xFD;
  if (k=='w') z80ports_in[2] &= 0xFB;
  if (k=='z') z80ports_in[2] &= 0xF7;
  if (k=='s') z80ports_in[2] &= 0xEF;
  if (k=='a') z80ports_in[2] &= 0xDF;
//  if ((key[SDLK_RCTRL]) || (key[SDLK_LCTRL])) z80ports_in[0x0280] &= 0xBF;
  
  if (k=='5') z80ports_in[3] &= 0xFE;
  if(k=='%')
  {
    z80ports_in[0] &= 0x7F;//SHIFT
    z80ports_in[3] &= 0xFE;// 5
  }
  if (k=='r') z80ports_in[3] &= 0xFD;
  if (k=='t') z80ports_in[3] &= 0xFB;
  if (k=='v') z80ports_in[3] &= 0xF7;
  if (k=='g') z80ports_in[3] &= 0xEF;
  if (k=='f') z80ports_in[3] &= 0xDF;


  if (k=='6') z80ports_in[4] &= 0xFE;
  if(k=='&')
  {
    z80ports_in[0] &= 0x7F;//SHIFT
    z80ports_in[4] &= 0xFE;// 6
  }
  if (k=='y') z80ports_in[4] &= 0xFD;
  if (k=='h') z80ports_in[4] &= 0xFB;
  if (k==' ') z80ports_in[4] &= 0xF7;
  if (k=='n') z80ports_in[4] &= 0xEF;
  if (k=='b') z80ports_in[4] &= 0xDF;

  if (k=='7') z80ports_in[5] &= 0xFE;
  if(k=='\'')
  {
    z80ports_in[0] &= 0x7F;//SHIFT
    z80ports_in[5] &= 0xFE;// 7
  }
  if (k=='8') z80ports_in[5] &= 0xFD;
  if(k=='(')
  {
    z80ports_in[0] &= 0x7F;//SHIFT
    z80ports_in[5] &= 0xFD;// 8
  }
  if (k=='u') z80ports_in[5] &= 0xFB;
  if (k=='m') z80ports_in[5] &= 0xF7;
  if (k=='j') z80ports_in[5] &= 0xDF;

  if (k=='9') z80ports_in[6] &= 0xFE;
  if(k==')')
  {
    z80ports_in[0] &= 0x7F;//SHIFT
    z80ports_in[6] &= 0xFE;// 9
  }
  if (k=='i') z80ports_in[6] &= 0xFD;
  if (k=='o') z80ports_in[6] &= 0xFB;
  if (k==',') z80ports_in[6] &= 0xF7;
  if(k=='<')
  {
    z80ports_in[0] &= 0x7F;//SHIFT
    z80ports_in[6] &= 0xF7;// ,
  }

  if (k=='k') z80ports_in[6] &= 0xDF;

  if (k=='0') z80ports_in[7] &= 0xFE;

  if (k=='p') z80ports_in[7] &= 0xFD;
  if (k=='l') z80ports_in[7] &= 0xFB;

// unsure about these
  if (k=='.') z80ports_in[7] &= 0xF7;
  if(k=='>')
  {
    z80ports_in[0] &= 0x7F;//SHIFT
    z80ports_in[7] &= 0xF7;// .
  }
  if (k==';') z80ports_in[7] &= 0xDF;
  if (k=='+')
  {
    z80ports_in[0] &= 0x7F;//SHIFT ;
    z80ports_in[7] &= 0xDF;
  }
  


  if (k=='-') z80ports_in[8] &= 0xFE;
  // = key is shifted -
  if (k=='=') 
  {
    z80ports_in[0] &= 0x7F;//SHIFT -
    z80ports_in[8] &= 0xFE;
  }

//  if (k=='(') z80ports_in[0x0880] &= 0xFB;
  if (k=='/') z80ports_in[8] &= 0xF7;
  if (k=='?')
  {
    z80ports_in[0] &= 0x7F;//SHIFT /
    z80ports_in[8] &= 0xF7;
  }

  if (k==':') z80ports_in[8] &= 0xDF;
  if (k=='*')
  {
    z80ports_in[0] &= 0x7F;//SHIFT :
    z80ports_in[8] &= 0xDF;
  }

  if (k=='[') z80ports_in[8] &= 0xFB;

// Does this work? gives a n amperrsand
//  if (k=='@') z80ports_in[0x0580] &= 0xEF;


  if (k=='@') z80ports_in[8] &= 0xFD;
  if (k=='\\')
  {
    z80ports_in[0] &= 0x7F;//SHIFT @
    z80ports_in[8] &= 0xFD;
  }


  // Newline Character 13
//  if (k==')') z80ports_in[0x0980] &= 0xFD;  
  if (k==']') z80ports_in[9] &= 0xFD;
  if (k=='\x0d') z80ports_in[9] &= 0xF7;

  if (k=='`' || k == 0x1b) z80ports_in[0] &= 0xBF;  //escape key
  if(k == 0x08) z80ports_in[9] &= 0x02;

 k_delay(4);
}

void k_delay(int del)
{
  int PUMP_DELAY=4;
  //Make sure the keypress is recognised
  //in future check in the keyboard buffer on the ynx that they key gets there
  
  for(int f=0;f<del;f++)
       execz80(1000);
   

  // Clear all keypresses
  z80ports_in[0] = 0xFF;
  z80ports_in[1] = 0xFF;
  z80ports_in[2] = 0xFF;
  z80ports_in[3] = 0xFF;
  z80ports_in[4] = 0xFF;
  z80ports_in[5] = 0xFF;
  z80ports_in[6] = 0xFF;
  z80ports_in[7] = 0xFF;
  z80ports_in[8] = 0xFF;
  z80ports_in[9] = 0xFF;

  //Make sure the keypress is recognised
  //in future check in the keyboard buffer on the ynx that they key gets there
  for(int f=0;f<del;f++)
     execz80(1000);
   
}

void patchrom()
{

  //These are for the TAPE load/save routines
  lynxrom[0xd67]=0xed;  //change Tape Byte output, just return 0 in A ?
  lynxrom[0xd68]=0x00;
  lynxrom[0xd69]=0xc9;  

  lynxrom[0xb65]=0xc9;  //disabled completely - Read Tape Sync
  
  lynxrom[0xcd4]=0xed;  //change Read Bit, just return 1 in A
  lynxrom[0xcd5]=0x01;
  lynxrom[0xcd6]=0xc9;
//  lynxrom[0xcd4]=0xc3;  //change Read Bit, just return 1 in A
//  lynxrom[0xcd5]=0xd4;
//  lynxrom[0xcd6]=0x0c;
  lynxrom[0xc95]=0xc3;  //setup an infint loop to wait here whilst VB is loading the RAM
  lynxrom[0xc96]=0x95;
  lynxrom[0xc97]=0x0c;

  lynxrom[0x3f62]=0xc3; //and again for MLOAD
  lynxrom[0x3f63]=0x62;
  lynxrom[0x3f64]=0x3f;

  
  //Patch Save routine to output OUT 93,x trapped here as SAVE
  //jump back in at 0cfb

  int sav_ad=0xbcb;
  lynxrom[sav_ad+0]=0x20;
  lynxrom[sav_ad+1]=0xf4;
  lynxrom[sav_ad+2]=0x01;//ld BC,0093
  lynxrom[sav_ad+3]=0x93;
  lynxrom[sav_ad+4]=0x00;
  lynxrom[sav_ad+5]=0xed;//out a (c)
  lynxrom[sav_ad+6]=0x79;
  lynxrom[sav_ad+7]=0x00;//never gets to these :)
  lynxrom[sav_ad+8]=0x00;

//  lynxrom[0x8cf]=0xc9;      //dev clear srcreen ld a,b etc bugs araound 08da in 48/96k rom 

}
