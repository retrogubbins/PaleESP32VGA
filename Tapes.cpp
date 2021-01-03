/*

void load_speccy()
{
  File  lhandle,lhandle2;
  unsigned int  buf_p=0,quot=0,size_read,load_address;
  unsigned char csum,flag_byte,header_byte1;
  unsigned int cdd,f,ret,blocksize,data_leng,param1;
  unsigned int tap_leng,exec_address, buf_pale;
  char csum_ok[10];
int tape_type = 0;
//  pump_string("mload\"\"\x0d");
int file_id; 
byte *lbuffer;
Z80_Regs i;

  Serial.print("Free Heap: ");
  Serial.println(system_get_free_heap_size());
 
  // open a file for input      
 // lhandle = SD.open("/AirRaid.tap", FILE_READ);
 // lhandle = SD.open("/FloydsBank .tap", FILE_READ);
//  lhandle = SD.open("/OhMummy.tap", FILE_READ);
 lhandle = SD.open("/manic.sna", FILE_READ);
//  lhandle = SD.open("/jetpac.sna", FILE_READ);
//  lhandle = SD.open("/atic.sna", FILE_READ);
//  lhandle = SD.open("/sabre.sna", FILE_READ);
//  lhandle = SD.open("/underw.sna", FILE_READ);
// lhandle = SD.open("/alien8.sna", FILE_READ);
//  lhandle = SD.open("/emerald.sna", FILE_READ);
// lhandle = SD.open("/magic.sna", FILE_READ);
// lhandle = SD.open("/cookie.sna", FILE_READ);
// lhandle = SD.open("/jetman.sna", FILE_READ);
// lhandle = SD.open("/daaw.sna", FILE_READ);

  size_read=0;
  if(lhandle!=NULL)
  {
    Serial.println("Loading:");
    //Read in the registers
    i.I=lhandle.read();
    i.HL2.B.l=lhandle.read();
    i.HL2.B.h=lhandle.read();
    i.DE2.B.l=lhandle.read();
    i.DE2.B.h=lhandle.read();
    i.BC2.B.l=lhandle.read();
    i.BC2.B.h=lhandle.read();
    i.AF2.B.l=lhandle.read();
    i.AF2.B.h=lhandle.read();

    i.HL.B.l=lhandle.read();
    i.HL.B.h=lhandle.read();
    i.DE.B.l=lhandle.read();
    i.DE.B.h=lhandle.read();
    i.BC.B.l=lhandle.read();
    i.BC.B.h=lhandle.read();
    i.IY.B.l=lhandle.read();
    i.IY.B.h=lhandle.read();
    i.IX.B.l=lhandle.read();
    i.IX.B.h=lhandle.read();

    byte inter =lhandle.read();
  Serial.println("inter address: ");
    Serial.println((unsigned int)inter, HEX);
    if(inter & 0x04 == 0)
        i.IFF2 = 0;
    else
        i.IFF2 = 1;

    i.R =lhandle.read();
 Serial.println("R : ");
    Serial.println((unsigned int)i.R, HEX);
 
    i.AF.B.l=lhandle.read();
    i.AF.B.h=lhandle.read();
 Serial.println("AF : ");
    Serial.println((unsigned int)i.AF.B.l, HEX);
    Serial.println((unsigned int)i.AF.B.h, HEX);
        
    i.SP.B.l=lhandle.read();
    i.SP.B.h=lhandle.read();
 // Serial.println("SP address");
 //   Serial.println((unsigned int)i.SP.W);
  Serial.println("SP address: ");
    Serial.println((unsigned int)i.SP.B.l, HEX);
    Serial.println((unsigned int)i.SP.B.h, HEX);

    i.IM = lhandle.read();
 Serial.println("IM : ");
    Serial.println((unsigned int)i.IM, HEX);
 
    byte bordercol =lhandle.read();
 Serial.println("Border : ");
    Serial.println((unsigned int)bordercol, HEX);
 

    i.IFF1 = i.IFF2;

   int32_t thestack =  i.SP.B.h * 256 + i.SP.B.l;
 Serial.println("thestack is : ");
    Serial.println(thestack, HEX);
    unsigned int  buf_p = 0;
    while (lhandle.available())
    {
      bank1[buf_p] = lhandle.read();
      buf_p++;
    }
    lhandle.close();
    Serial.println("noof bytes");
    Serial.println(buf_p);


//    lhandle = SD.open("/atic.dmp", FILE_WRITE);
//    buf_p = 0;
//    while (buf_p < 49152)
//    {
//      lhandle.write(bank1[buf_p]);
//      buf_p++;
//    }
//    lhandle.close();



    Serial.println("STACK:");
    for(int16_t yy = 0;yy < 32;yy++)
    {
          Serial.print(thestack - 0x4000 - 8 + yy, HEX);
          Serial.print(" - ");
          Serial.println(bank1[thestack - 0x4000 - 8 + yy], HEX);
//         Serial.print("      stack_4000 : ");
//           Serial.print(thestack  - 8 + yy, HEX);
//          Serial.print(" - ");
//          Serial.println(bank1[thestack  - 8 + yy], HEX);
    }


    //   unsigned int retaddr = bank1[(i.SP.D >>16) - 0x4000];
    int32_t retaddr = bank1[thestack - 0x4000] + bank1[thestack+1 - 0x4000] * 256 ;
    Serial.println("retn address");
    Serial.println(retaddr, HEX);
// i.SP.D = thestack;
  
     Serial.println("sp before");
     Serial.println(i.SP.D, HEX);
 
    i.SP.D++;
    i.SP.D++;
     Serial.println("sp after");
     Serial.println( i.SP.D, HEX);
 //  i.PC.D = 0x7c19; //retaddr;  //0x7c19;  // 0x8400;  //manic miner is 0x8400   // 0x6000; atic atac doesnt run properly
    i.PC.D = retaddr;
 //   i.PC.D = 0x5f3e; 
     Serial.println("i.PC.D is");
     Serial.println( i.PC.D, HEX);

    Z80_SetRegs (&i);
  }
  else
  {
    Serial.println("Couldn't Open SNA file ");
    return;
  }
}


*/
