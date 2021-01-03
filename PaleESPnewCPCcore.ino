
// ____________________________________________________
// PaleESP32TTGO
// Pete Todd 2017 - 2020 
// Using Z80 core from CPC-em Tom Walker
// ____________________________________________________
#include <Arduino.h>

#include <ESP32Lib.h>
#include <Ressources/Font6x8.h>
//#include "fabgl.h"
//#include "fabutils.h"

#include <bt.h>
#include <esp_wifi.h>
#include "PS2Kbd.h"
#include <SPI.h>

#include "Z80.h"
#include "lynxConfig.h"
#include "hardware.h"
#include "Z80IO.h"
#include "keys.h"

#ifdef enable_disks
  #include "PALEDISK.H"
#endif
  
// ________________________________________________
// SWITCHES
//
//#define WIFI_ENABLED
#define SD_ENABLED
//#define MENU_ENABLED

//#define DEBUG

//#define USE_FRAMESYNC

bool run_debug = false;

#ifdef SD_ENABLED
  #include <FS.h>
  #include <SD.h>
  static fs::FS* fileSystem;
#endif


const int redPin = 22;    //was 27
const int greenPin = 19;   //was 21
const int bluePin = 5;    //was 14
const int hsyncPin = 23;  
const int vsyncPin = 15; // was 22


// ____________________________________________________________________
// INSTANTS
//
// WiFi
//
//VGA Device
VGA3Bit vga;



// ________________________________________________________________________
// EXTERNS + GLOBALS
//
unsigned Z80_GetPC (void);         /* Get program counter                   */
void Z80_Reset (void);             /* Reset registers to the initial values */
unsigned int  Z80_Execute ();           /* Execute IPeriod T-States              */

void pump_key(char k);

byte Z80_RDMEM(uint16_t A);
void Z80_WRMEM(uint16_t A,byte V);
extern byte bank_latch;
extern byte lynxrom[24576];

// ________________________________________________________________________
// GLOBALS
//
byte *bank1;
byte *bank2;
byte *bank3;
byte *diskbuf;
uint16_t *tftmem;
byte z80ports_in[16];

int interruptfps = 0;
int interruppted = 0;
bool show_redblue = true;
bool use_interrupts = false;
static unsigned long elapsed_t;
int current_diskno = 1;

hw_timer_t * timer = NULL;
SemaphoreHandle_t xMutex;
unsigned char gb_run_emulacion = 1; //Ejecuta la emulacion
//unsigned char gb_current_ms_poll_sound = gb_ms_sound;
unsigned char gb_current_ms_poll_keyboard = gb_ms_keyboard;
unsigned char gb_current_frame_crt_skip= gb_frame_crt_skip; //No salta frames
unsigned char gb_current_delay_emulate_ms= gb_delay_emulate_ms;
unsigned char gb_sdl_blit=0;
unsigned char gb_screen_xOffset=0;
static unsigned long gb_time_ini_espera;
static unsigned long gb_currentTime;
static unsigned long gb_keyboardTime;


// SETUP *************************************
// SETUP *************************************
// SETUP *************************************

void setup()
{
  // Turn off peripherals to gain memory (?do they release properly)
 esp_bt_controller_deinit();
 esp_bt_controller_mem_release(ESP_BT_MODE_BTDM);
 esp_wifi_set_mode(WIFI_MODE_NULL);

 Serial.begin(115200);

 Serial.printf("PALE Emulator Pete Todd 2017\n");


#ifdef SD_ENABLED
    test_sd();
#endif

  pinMode(SPEAKER_PIN,OUTPUT);

  //we need double buffering for smooth animations
  vga.setFrameBufferCount(1);
  //initializing i2s vga (with only one framebuffer)
//  vga.init(vga.MODE320x240.custom(256, 248), redPin, greenPin, bluePin, hsyncPin, vsyncPin);
  vga.init(vga.MODE400x300.custom(256, 248), redPin, greenPin, bluePin, hsyncPin, vsyncPin);
  
  kb_begin();


//#ifdef MENU_ENABLED
//  vga.setFont(Font6x8);
//  //displaying the text
// vga.setCursor(5, 5);
// //print the text, println also exists
//  vga.print("dot(x,y,c)");
//  vga.println("Hello World!");
//#endif

// ALLOCATE MEMORY
//
  bank1 = (byte *)malloc(65536);
  if(bank1 == NULL)Serial.println("Failed to allocate Bank 1 memory");
  bank2 = (byte *)malloc(16384);
  if(bank2 == NULL)Serial.println("Failed to allocate Bank 2 memory");
  bank3 = (byte *)malloc(16384);
  if(bank3 == NULL)Serial.println("Failed to allocate Bank 3 memory");

#ifdef enable_disks
    init_disks(); 
    open_working_disk(1); // loads jd1.ldf disk from sd card  
#endif


// START Z80
// START Z80
// START Z80
  Serial.println("RESETTING Z80");
 // patchrom();
 //initz80();
 resetz80();

  // make sure keyboard ports are FF
  for(int t = 0;t < 16;t++)
  {
    z80ports_in[t] = 0xff;
  }

  Serial.print("Setup: MAIN Executing on core ");
  Serial.println(xPortGetCoreID());
  Serial.print("Free Heap: ");
  Serial.println(system_get_free_heap_size());
  Serial.print("vga free memory: ");
  Serial.println((int)heap_caps_get_free_size(MALLOC_CAP_DEFAULT));
 
  xMutex = xSemaphoreCreateMutex();

  xTaskCreatePinnedToCore(
                    videoTask,   /* Function to implement the task */
                    "videoTask", /* Name of the task */
                    2048,      /* Stack size in words */
                    NULL,       /* Task input parameter */
                    0,          /* Priority of the task */
                    NULL,       /* Task handle. */
                    0);  /* Core where the task should run */
                          
 
}


// VIDEO core 0 *************************************
// VIDEO core 0 *************************************
// VIDEO core 0 *************************************

void videoTask( void * parameter )
{
   while(1)
    {
#ifdef USE_FRAMESYNC
         xSemaphoreTake( xMutex, portMAX_DELAY );
#endif     
  //    int16_t startlinebyteaddr = 32*4;  // skip 1st 4 lines so that 240 pixels will get whole lynx text screen
      for (int lin = 0;lin < 248;lin++)
      {
        int16_t coladdr = 0;
        for(int col = 0;col < 32;col++)  
        {
          uint16_t bytaddr =  lin * 32 + col;  //startlinebyteaddr +
          byte redbyte = bank2[bytaddr + 0x2000];
          byte bluebyte = bank2[bytaddr];
          byte greenbyte = bank3[bytaddr + 0x2000];
          for(int bb = 0;bb < 8;bb++)
          {
            char dored = 0;
            char dogreen = 0;
            char doblue = 0;
            
            byte bitpos = (0x80 >> bb);
            if(show_redblue || ((video_latch & 0x04) == 0))     // emulate level 9 video show banks from video latch
            {
              if((redbyte & bitpos) != 0)
                dored = 255;
              if((bluebyte & bitpos) != 0)
                doblue = 255;
            }
            if((greenbyte & bitpos) != 0)
                dogreen = 255;
            
            vga.dotFast(col * 8 + bb, lin,vga.RGB(dored,dogreen,doblue)) ;          
          }
        }
      } 
    //  vTaskDelay(0) ; 
interruptfps++;
#ifdef USE_FRAMESYNC
         xSemaphoreGive( xMutex ); 
        vTaskDelay(1) ;
#endif    
     }
}



/*
 *      emu_cycles_scanline=(long)(emuspeeds[x]*256)/100; //256 = 64us Scanline Period
      emu_cycles_lineblank=(long)(emuspeeds[x]*88)/100; //88 = 22us LineBlank Period
      emu_cycles_vblank=(long)(emuspeeds[x]*160)/100;  //160= 40us Vblank(IRQ LOW) Period
      emu_cycles=(long)(emuspeeds[x]*13600)/100;  //13600 = 3.4ms Vblank period

 */
// LOOP core 1 *************************************
// LOOP core 1 *************************************
// LOOP core 1 *************************************
unsigned long gb_tickerTime;

  int noof_cyc = 0;
  int scanline = 0;

void loop() 
{ 
#ifdef USE_FRAMESYNC
     xSemaphoreTake( xMutex, portMAX_DELAY );
#endif 
    gb_currentTime = millis();
    if ((gb_currentTime-gb_keyboardTime) >= gb_current_ms_poll_keyboard)
    { 
        gb_keyboardTime = gb_currentTime;
        do_keyboard();
    }

//    if ((gb_currentTime-gb_tickerTime) >= 1000)
//    { 
//        gb_tickerTime = gb_currentTime;
//        Serial.println(noof_cyc);
//    }
     
 //   gb_currentTime = millis();

     noof_cyc = execz80(base_speed_cycles * speed_mult);
 //  noof_cyc = execz80(344);
  // Line_Blank=0;

   scanline++;
   if(scanline > 251)
   {
      scanline = 0;
//      intreq=1;
//      noof_cyc = execz80(160);   
//      intreq=0;
      noof_cyc = execz80(13600  * speed_mult); 
     vTaskDelay(1) ;  //important to avoid task watchdog timeouts - change this to slow down emu
#ifdef USE_FRAMESYNC
        xSemaphoreGive( xMutex );
#endif
   }
 //    elapsed_t = millis() -  gb_currentTime;
    
//     noof_cyc = execz80(base_speed_cycles * speed_mult);

 //     vTaskDelay(1) ;
//  } 
//vTaskDelay(1) ;
}

void do_keyboard()
{
    bitWrite(z80ports_in[0], 0, keymap[0x16]);
 //   bitWrite(z80ports_in[0], 1, keymap[0x1a]);
 //   bitWrite(z80ports_in[0], 2, keymap[0x22]);
    bitWrite(z80ports_in[0], 3, keymap[0x58]);
    bitWrite(z80ports_in[0], 4, keymap[0x75]);
    bitWrite(z80ports_in[0], 5, keymap[0x72]);
    bitWrite(z80ports_in[0], 6, keymap[0x76]);
    bitWrite(z80ports_in[0], 7, keymap[0x12]);

    bitWrite(z80ports_in[1], 0, keymap[0x26]);
    bitWrite(z80ports_in[1], 1, keymap[0x25]);
    bitWrite(z80ports_in[1], 2, keymap[0x24]);
    bitWrite(z80ports_in[1], 3, keymap[0x22]);
    bitWrite(z80ports_in[1], 4, keymap[0x23]);
    bitWrite(z80ports_in[1], 5, keymap[0x21]);

    bitWrite(z80ports_in[2], 0, keymap[0x1e]);
    bitWrite(z80ports_in[2], 1, keymap[0x15]);
    bitWrite(z80ports_in[2], 2, keymap[0x1d]);
    bitWrite(z80ports_in[2], 3, keymap[0x1a]);
    bitWrite(z80ports_in[2], 4, keymap[0x1b]);
    bitWrite(z80ports_in[2], 5, keymap[0x1c]);
    bitWrite(z80ports_in[2], 6, keymap[0x14]); // control

    bitWrite(z80ports_in[3], 0, keymap[0x2e]);
    bitWrite(z80ports_in[3], 1, keymap[0x2d]);
    bitWrite(z80ports_in[3], 2, keymap[0x2c]);
    bitWrite(z80ports_in[3], 3, keymap[0x2a]);
    bitWrite(z80ports_in[3], 4, keymap[0x34]);
    bitWrite(z80ports_in[3], 5, keymap[0x2b]); //F
    
    bitWrite(z80ports_in[4], 0, keymap[0x36]);
    bitWrite(z80ports_in[4], 1, keymap[0x35]);
    bitWrite(z80ports_in[4], 2, keymap[0x33]);
    bitWrite(z80ports_in[4], 3, keymap[0x29]);
    bitWrite(z80ports_in[4], 4, keymap[0x31]);
    bitWrite(z80ports_in[4], 5, keymap[0x32]); //B
    
    bitWrite(z80ports_in[5], 0, keymap[0x3d]);
    bitWrite(z80ports_in[5], 1, keymap[0x3e]);
    bitWrite(z80ports_in[5], 2, keymap[0x3c]);
    bitWrite(z80ports_in[5], 3, keymap[0x3a]);
 //   bitWrite(z80ports_in[5], 4, keymap[0x35]);
    bitWrite(z80ports_in[5], 5, keymap[0x3b]); //J
    
    bitWrite(z80ports_in[6], 0, keymap[0x46]);
    bitWrite(z80ports_in[6], 1, keymap[0x43]);
    bitWrite(z80ports_in[6], 2, keymap[0x44]);
    bitWrite(z80ports_in[6], 3, keymap[0x41]);
 //   bitWrite(z80ports_in[6], 4, keymap[0x3b]);
    bitWrite(z80ports_in[6], 5, keymap[0x42]); // K
    
    bitWrite(z80ports_in[7], 0, keymap[0x45]);
    bitWrite(z80ports_in[7], 1, keymap[0x4d]);
    bitWrite(z80ports_in[7], 2, keymap[0x4b]);
    bitWrite(z80ports_in[7], 3, keymap[0x49]);
 //   bitWrite(z80ports_in[7], 4, keymap[0x32]);
    bitWrite(z80ports_in[7], 5, keymap[0x4c]); // semi colon

    bitWrite(z80ports_in[8], 0, keymap[0x4e]);
    bitWrite(z80ports_in[8], 1, keymap[0x52]);
    bitWrite(z80ports_in[8], 2, keymap[0x54]);
    bitWrite(z80ports_in[8], 3, keymap[0x4a]);
 //   bitWrite(z80ports_in[8], 4, keymap[0x32]);
    bitWrite(z80ports_in[8], 5, keymap[0x5d]); //colon 


    bitWrite(z80ports_in[9], 0, keymap[0x66]);
    bitWrite(z80ports_in[9], 1, keymap[0x5b]);
    bitWrite(z80ports_in[9], 2, keymap[0x6b]);
    bitWrite(z80ports_in[9], 3, keymap[0x5a]);
 //   bitWrite(z80ports_in[9], 4, keymap[0x32]);
    bitWrite(z80ports_in[9], 5, keymap[0x74]); // right arrow
    
//    if(keymap[KEY_F1] == 0)   // F1  key    Help
//    {
//      keymap[KEY_F1] = 1;
//       //load_lynx_tap();
//    }
    if(keymap[KEY_F2] == 0)   // F2  key  Toggle Turbo speed
    {
       keymap[KEY_F2] = 1;
       if(speed_mult > 1)
            speed_mult = 1;
       else
            speed_mult = 16;
    } 
    
    if(keymap[KEY_F3] == 0)   // F3  key    Get System Info
    {
      keymap[KEY_F3] = 1;
      Serial.print("Free Heap: ");
      Serial.println(system_get_free_heap_size());
      Serial.print("vga free memory: ");
      Serial.println((int)heap_caps_get_free_size(MALLOC_CAP_DEFAULT));
      Serial.print("Total heap: ");
      Serial.println(ESP.getHeapSize());
        Serial.print("Free heap: ");
        Serial.println( ESP.getFreeHeap());
        Serial.print("Total PSRAM: ");
        Serial.println(ESP.getPsramSize());
        Serial.print("Free PSRAM: ");
      Serial.println(ESP.getFreePsram());
    }

 

    if(keymap[KEY_F4] == 0)   //   Toggle display red/blue for Level9 Adventures
    {
       keymap[KEY_F4] = 1;
       show_redblue = !show_redblue;
    } 
     if(keymap[KEY_F5] == 0)   //  
    {
       keymap[KEY_F5] = 1;
       Serial.println(elapsed_t);
    } 
     if(keymap[KEY_F6] == 0)   //  Increment Disk number and reload
    {
       keymap[KEY_F6] = 1;
       current_diskno++;
       if(current_diskno > 20)
          current_diskno = 0;
       open_working_disk(current_diskno);
    } 
     if(keymap[KEY_F7] == 0)  
    {
       keymap[KEY_F7] = 1;
       use_interrupts = !use_interrupts;
    } 
        
      
//    if(keymap[KEY_F8] == 0)   //   Save Current Working Disk buffer as ldump.ldf on SD card
//    {
//     keymap[KEY_F8] = 1;
//       save_working_disk();
//    }   
    if(keymap[KEY_F10] == 0)   //  CLOSE SDCard 
    {
      keymap[KEY_F10] = 1;
      SPI.end();
      // .close();
    }  
    if(keymap[KEY_F11] == 0)   //  REBOOT ESP32
    {
     keymap[KEY_F11] = 1;
       esp_restart();
    }    
    if(keymap[KEY_F12] == 0)   //  REBOOT Z80
    {
     keymap[KEY_F12] = 1;
       resetz80();
    }    

    
}




#ifdef SD_ENABLED
void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("Failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.name(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}
//// Flash and SDCard configuration
//#define FORMAT_ON_FAIL     true
//#define SPIFFS_MOUNT_PATH  "/flash"
//#define SDCARD_MOUNT_PATH  "/SD"
//// base path (can be SPIFFS_MOUNT_PATH or SDCARD_MOUNT_PATH depending from what was successfully mounted first)
//char const * basepath = nullptr;
//
//void test_sd()
//{
//
//    if (FileBrowser::mountSDCard(false, SDCARD_MOUNT_PATH))
//    basepath = SDCARD_MOUNT_PATH;
//  else if (FileBrowser::mountSPIFFS(false, SPIFFS_MOUNT_PATH))
//    basepath = SPIFFS_MOUNT_PATH;
//
//}
void test_sd()
{
    SPI.begin(14, 2, 12);
    if (!SD.begin(13)) 
    {
        Serial.println("Mount SD failed");
    }

    fileSystem = &SD;
    vTaskDelay(2);
   
    uint8_t cardType = SD.cardType();

    if(cardType == CARD_NONE){
        Serial.println("No SD card attached");
        return;
    }

    Serial.print("SD Card Type: ");
    if(cardType == CARD_MMC){
        Serial.println("MMC");
    } else if(cardType == CARD_SD){
        Serial.println("SDSC");
    } else if(cardType == CARD_SDHC){
        Serial.println("SDHC");
    } else {
        Serial.println("UNKNOWN");
    }

    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("SD Card Size: %lluMB\n", cardSize);
 
 /*
  // re-open the file for reading:
  myFile = SD.open("/AirRaid.tap", FILE_READ);
  if (myFile) {
    Serial.println("COMMAND.TXT:");

    // read from the file until there's nothing else in it:
    while (myFile.available()) {
      Serial.write(myFile.read());
    }
    // close the file:
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening COMMAND.TXT");
  }
*/

//listDir(SD, "/", 0);

}
#endif
