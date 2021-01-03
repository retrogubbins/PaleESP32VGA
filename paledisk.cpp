#include <Arduino.h>


#include "PALEDISK.H"
//  #include <FS.h>
  #include <SD.h>

char disk_options=0x0;
bool disk_rom_enabled=true;

char disk_trackreg;
char disk_comreg;       
char disk_sectreg;      
char disk_statusreg;    
char disk_datareg;      
int disk_head=0;
int disk_drive=0;
char disk_motor=0;
char disk_precomp=0;
char disk_step125=0;
int last_command;

////THESE ARE FOR LIBDSK
char disk_sect_to_write=0;
char disk_head_to_write=0;
int disk_sect_buf_ptr=0;
int disk_track_buf_ptr=0;
char disk_track_to_write=0;
char disk_drive_to_write=0;

char last_drive_sel=0;  //holds last user selected disk drive

unsigned char *disk_sect_buf;
unsigned char *disk_track_buf;

byte *disk0;

void init_disks()
{
   disk0 = (byte *)ps_malloc(LYNX_MAX_DISK_SIZE);
   if(disk0 == NULL)Serial.println("Failed to allocate Disk Buffer memory");
   disk_sect_buf = (byte *)ps_malloc(BYTES_PER_SECT);
   if(disk_sect_buf == NULL)Serial.println("Failed to allocate Sector memory");
   disk_track_buf = (byte *)ps_malloc(BYTES_PER_TRACK);
   if(disk_track_buf == NULL)Serial.println("Failed to allocate Track memory");

   reset_disk_vars();         
}

void kill_disk()
{
    //    int ret;
     //   char lbl1[50];
    //    ret=dsk_close(&ddrive[driveno]);
}


void open_working_disk(int x)
{
  File  lhandle;
  char lbl[30];
  sprintf(lbl,"/jd%d.ldf",x);
//  lhandle = SD.open("/jd"+ce+".ldf", FILE_READ);
  lhandle = SD.open(lbl, FILE_READ);
  if(lhandle!=NULL)
  {
    if (lhandle.available())
    {
       lhandle.read(disk0,LYNX_MAX_DISK_SIZE);
    }
    lhandle.close();
  }  
  else
  {
    Serial.println("Error Opening Disk File");
  }
}


void save_working_disk()
{
  File  lhandle;
  lhandle = SD.open("/ldump.ldf", FILE_WRITE);
  if(lhandle!=NULL)
  {
    {
        lhandle.write(disk0,LYNX_MAX_DISK_SIZE);
    }
    lhandle.close();
  }  
  else
  {
    Serial.println("Error Opening DiskWrite File");
  }
}

void clear_track_buf()
{
        int y;
        for(y=0;y<BYTES_PER_TRACK;y++)
                disk_track_buf[y]=(char)0xe5;
}
void clear_sect_buf()
{
        int y;
        for(y=0;y<BYTES_PER_SECT;y++)
                disk_sect_buf[y]=(char)0xe5;
}

void do_disk_updates()
{
        if(last_command==0)return;
        
        if((last_command & 0xe0)==0xa0)  //we had a write sector last time - so write the sector buffer out
        {
               for(int f = 0; f < BYTES_PER_SECT; f++)
                    disk0[disk_head_to_write*BYTES_PER_SIDE + disk_track_to_write*BYTES_PER_TRACK+(disk_sect_to_write-1)*BYTES_PER_SECT+f] = disk_sect_buf[f];
               last_command=0;disk_comreg=0;
               disk_statusreg=0;
        }
        if((last_command & 0xf8)==0xf0)  //we had a write track last time - so write the track buffer out - unused by lynxdos except format?
        {
                //For CPM disk should format tracks 2 onwards as interleaved 1,6,2,7,3,8,4,9,5,10
                last_command=0;
                disk_comreg=0;
                disk_statusreg=0;
        }
}



void do_disk_command()
{
        int ret;
        unsigned char command,status;
        static char step_direction=+1;
 
        //Look at the command register to decide what to do
        last_command=disk_comreg;

        if ((disk_comreg & 0xf0)==0)//restore
        {
                disk_trackreg=0;
                disk_statusreg=disk_statusreg | 0x04;   //set Track 0 positioned (type 1)
                disk_statusreg=disk_statusreg | 0x02;   //set Index Mark Detected (type 1)
                disk_statusreg=disk_statusreg | 0x20;   //set Head Loaded
        }
        else if((disk_comreg & 0xf0)==0x10)//Seek
        {
                disk_trackreg=disk_datareg;                    
                if (disk_trackreg>=TRACKS_PER_DISK) //TRACKS PER DISK
                {
                        disk_trackreg=TRACKS_PER_DISK-1;
                        disk_statusreg=disk_statusreg & 0x10;   //set couldnt find track        - used to be =0x10 bug?
                        return;
                }
                if (disk_trackreg==0)disk_statusreg=disk_statusreg | 0x04;//set TRACK0 flag
                disk_statusreg=disk_statusreg | 0x02;   //set Index Mark Detected (type 1)
                disk_statusreg=disk_statusreg | 0x20;   //set Head Loaded
        }
        else if((disk_comreg & 0xe0)==0x20)//Step
        {
                disk_trackreg+=step_direction;
                if (disk_trackreg>=TRACKS_PER_DISK)
                {
                        disk_trackreg=TRACKS_PER_DISK-1;
                        disk_statusreg=disk_statusreg = 0x10;   //set couldnt find track
                        return;
                }
                if (disk_trackreg==0)disk_statusreg=disk_statusreg | 0x04;//set TRACK0 flag
                disk_statusreg=disk_statusreg | 0x02;   //set Index Mark Detected (type 1)
                disk_statusreg=disk_statusreg | 0x20;   //set Head Loaded
        }
        else if((disk_comreg & 0xe0)==0x40)//Step-In
        {
                disk_trackreg++;
                step_direction=1;
                if(disk_trackreg>=TRACKS_PER_DISK)
                {
                        disk_trackreg=TRACKS_PER_DISK-1;
                        disk_statusreg=disk_statusreg = 0x10;   //set couldnt find track
                        return;
                }
                if (disk_trackreg==0)disk_statusreg=disk_statusreg | 0x04;//set TRACK0 flag
                disk_statusreg=disk_statusreg | 0x02;   //set Index Mark Detected (type 1)
                disk_statusreg=disk_statusreg | 0x20;   //set Head Loaded
        }
        else if((disk_comreg & 0xe0)==0x60)//Step_Out
        {
              
                disk_trackreg--;    //had a =0 for the trackreg here once
                step_direction=-1;
                if (disk_trackreg==0)disk_statusreg=disk_statusreg | 0x04;//set TRACK0 flag
                disk_statusreg=disk_statusreg | 0x02;   //set Index Mark Detected (type 1)
                disk_statusreg=disk_statusreg | 0x20;   //set Head Loaded
        }
        else if((disk_comreg & 0xe0)==0x80)//                           *********    Read Sector INIT  *******
        {
                clear_sect_buf();
                disk_sect_buf_ptr=0;
//Serial.println("Read sector init");
//Serial.println("head");
//Serial.println(disk_head);
//Serial.println("track");
//Serial.println((int)disk_trackreg);
//
//Serial.println("sector");
//Serial.println((int)disk_sectreg);

                for(int f = 0; f < BYTES_PER_SECT; f++)
                    disk_sect_buf[f] = disk0[disk_head*BYTES_PER_SIDE + disk_trackreg*BYTES_PER_TRACK+(disk_sectreg-1)*BYTES_PER_SECT+f];
              
                disk_statusreg=0;
                disk_statusreg=disk_statusreg | 0x01;//set busy flag
                disk_statusreg=disk_statusreg | 0x02;//set ready for data
                last_command=0;
                return;
        }
        else if((disk_comreg & 0xe0)==0xa0)//Write Sector
        {
                disk_sect_to_write=disk_sectreg;
                disk_track_to_write=disk_trackreg;
                disk_head_to_write=0;
                disk_drive_to_write=0;
                disk_sect_buf_ptr=0;
                clear_sect_buf();
//Serial.println("Write sector init");
//Serial.println("head");
//Serial.println(disk_head);
//Serial.println("track");
//Serial.println((int)disk_trackreg);
//
//Serial.println("sector");
//Serial.println((int)disk_sectreg);                
        
                disk_statusreg=0;
                disk_statusreg=disk_statusreg | 0x01;//set busy flag
                disk_statusreg=disk_statusreg | 0x02;//set ready for data
                return;
        }
        else if((disk_comreg & 0xf8)==0xc0)//Read Address
        {
                        //Track address, Side number, Sector address,
                        //      Sector Length, CRC1, CRC2
                        disk_sect_buf_ptr=0;
                        clear_sect_buf();
                        disk_sect_buf[disk_sect_buf_ptr++]=disk_trackreg;
                        disk_sect_buf[disk_sect_buf_ptr++]=disk_head;
                        disk_sect_buf[disk_sect_buf_ptr++]=disk_sectreg++;              //Increment along to next sector
                        disk_sect_buf[disk_sect_buf_ptr++]=(char)255;
                        disk_sect_buf[disk_sect_buf_ptr++]=0;
                        disk_sect_buf[disk_sect_buf_ptr++]=0;
                        disk_sect_buf_ptr=0;
                        
                        disk_statusreg=0;
                        disk_statusreg=disk_statusreg | 0x01;//set busy flag
                        disk_statusreg=disk_statusreg | 0x02;//set ready for data
                        return;
        }
        else if((disk_comreg & 0xf8)==0xe0)//Read Track
        {
                //gui_error("DISKREAD TRACK - not implemented yet");
        }
        else if((disk_comreg & 0xf8)==0xf0)//Write Track
        {
                disk_track_to_write=disk_trackreg;
                disk_sect_to_write=disk_sectreg;
                disk_head_to_write=disk_head;
                disk_drive_to_write=disk_drive;
                disk_track_buf_ptr=0;
                clear_track_buf();
            
                disk_statusreg=0;
                disk_statusreg=disk_statusreg | 0x01;//set busy flag
                disk_statusreg=disk_statusreg | 0x02;//set ready for data
                return;
        }
        else if((disk_comreg & 0xf0)==0xd0)//Force Interrupt
        {
                disk_statusreg=0x00;    //set Track 0 positioned (type 1)
                last_command=0;
                disk_statusreg=disk_statusreg | 0x20;   //set HEAD LOADed (type 1)
                disk_statusreg=disk_statusreg | 0x02;   //set Index Mark Detected (type 1)
        }
}




void reset_disk_vars()
{
        disk_options=0;
        disk_comreg=0;
        disk_trackreg=0;
        disk_sectreg=0;
        clear_track_buf();
        clear_sect_buf();
        disk_statusreg=0;
        disk_head=0;
        disk_precomp=0;
        disk_step125=0;
        disk_motor=0;
}


void disk_outp(unsigned int Port,unsigned char Value)
{
        static unsigned int temp_bufptr=0;
        //  DISK   DISK   DISK   DISK   DISK   DISK   DISK   DISK   DISK   DISK   DISK 
        //  DISK   DISK   DISK   DISK   DISK   DISK   DISK   DISK   DISK   DISK   DISK 
        //  DISK   DISK   DISK   DISK   DISK   DISK   DISK   DISK   DISK   DISK   DISK 
        //  DISK   DISK   DISK   DISK   DISK   DISK   DISK   DISK   DISK   DISK   DISK 
        if((Port & 0xff)==0x54) //DISK Write reg 1793 Command/Status Register
        {
                disk_comreg=Value;
                do_disk_command();
        }



        if((Port & 0xff)==0x55) //DISK Write reg 1793 Track Register
        {
                do_disk_updates();      //if a command is pending then do it
                disk_trackreg=Value;
                disk_track_buf_ptr=0;
                if(Value>=TRACKS_PER_DISK)
                {
                        disk_trackreg=TRACKS_PER_DISK-1;
                }
        
        }
        if((Port & 0xff)==0x56) //DISK Write reg 1793 Sector Register
        {
                do_disk_updates();      //if a command is pending then do it
                disk_sectreg=Value;
                disk_sect_buf_ptr=0;
        }

        
        if((Port & 0xff)==0x57) //DISK Write reg 1793 Data Register
        {
                if((disk_comreg & 0xf8)==0xf0)  //if were in a write track command
                {
                        disk_statusreg=0x00;//disk_statusreg | 0x02;//set DRQ data request active
                        disk_statusreg=disk_statusreg | 0x02;//set readuy for data
                        disk_statusreg=disk_statusreg | 0x01;//set busy flag
                        //THROW AWAY first 90 bytes of DAM header
                        //just pass the data for each sector through to libdsk
Serial.println("ERROR - in write track not implemented");
                        //rubbish code
                        //I dont do anything with the trackbuf created here anyway at the moment
                        if(temp_bufptr++>90)
//                                      disk_track_buf[disk_track_buf_ptr++]=Value;
//                                      disk_track_buf[disk_track_buf_ptr]=Value;
                        if(temp_bufptr>603)
                                temp_bufptr=0;

                        disk_track_buf_ptr++;
                        if(disk_track_buf_ptr>=BYTES_PER_TRACK)
                        {
                                do_disk_updates();
                                disk_statusreg=0x00;
                        }
                }
                else if((disk_comreg & 0xe0)==0xa0)     //write sector command
                {
                        disk_statusreg=0x00;//disk_statusreg | 0x02;//set DRQ data request active
                        disk_statusreg=disk_statusreg | 0x02;//set readuy for data
                        disk_statusreg=disk_statusreg | 0x01;//set busy flag

                        if(disk_sect_buf_ptr<BYTES_PER_SECT)
                                disk_sect_buf[disk_sect_buf_ptr++]=Value;
                        if(disk_sect_buf_ptr>=BYTES_PER_SECT)
                        {
                                do_disk_updates();
                                disk_statusreg=0x00;//disk_statusreg | 0x02;//set DRQ data request active
                        }
                }
                disk_datareg=Value;
                return;
        }
        if((Port & 0xff)==0x58) //DISK Write Options
        {
                do_disk_updates();      //if a command is pending then do it

                disk_options=Value;

  
                if((disk_options&0x10)==0x10)
                {
                   // Serial.println("DISK ROM DISABLED");
                    disk_rom_enabled=false;
                }
                else
                {
                   // Serial.println("DISK ROM ENABLED");
                    disk_rom_enabled=true;
                }
                if((disk_options&0x08)==0x08)
                         disk_motor=0;
                else
                         disk_motor=1;
                if((disk_options&0x40)==0x40)
                         disk_precomp=0;
                else
                         disk_precomp=1;
                if((disk_options&0x80)==0x80)
                         disk_step125=1;
                else
                         disk_step125=0;        //defaults back to 250ns

                disk_drive=disk_options & 0x03;

                if((disk_options & 0x04)==0x04)
                {
                        disk_head=1;
                }
                else
                {
                        disk_head=0;
                }
        }

}


unsigned char disk_inp(unsigned int Port)
{
        byte tmp;
        // DISK INPUT    DISK INPUT    DISK INPUT    DISK INPUT    DISK INPUT    DISK INPUT   
        // DISK INPUT    DISK INPUT    DISK INPUT    DISK INPUT    DISK INPUT    DISK INPUT   
        // DISK INPUT    DISK INPUT    DISK INPUT    DISK INPUT    DISK INPUT    DISK INPUT   
        // DISK INPUT    DISK INPUT    DISK INPUT    DISK INPUT    DISK INPUT    DISK INPUT   
        // DISK INPUT    DISK INPUT    DISK INPUT    DISK INPUT    DISK INPUT    DISK INPUT   
        // DISK INPUT    DISK INPUT    DISK INPUT    DISK INPUT    DISK INPUT    DISK INPUT   
        if((Port & 0xff)==0x50)
        {
               return(disk_statusreg);//notice that dual purpose here
        }
        if((Port & 0xff)==0x51)
                return(disk_trackreg);
        if((Port & 0xff)==0x52)
                return(disk_sectreg);
        if((Port & 0xff)==0x53)
        {
                if((disk_comreg & 0xe0)==0x80)  //if were in a read sector command
                {
                        tmp=disk_sect_buf[disk_sect_buf_ptr++];
                        if(disk_sect_buf_ptr>=BYTES_PER_SECT)
                        {
                                disk_statusreg=0x00;
                                disk_sect_buf_ptr=0;
                        }
                        return(tmp);
                }
                else if((disk_comreg & 0xf8)==0xc0)//Read Address
                {
                        tmp=disk_sect_buf[disk_sect_buf_ptr++];
                        if(disk_sect_buf_ptr>=8)
                        {
                                disk_statusreg=0x00;
                                disk_sect_buf_ptr=0;
                        }                       
                        return(tmp);
                }
                return(disk_datareg);
        }
}
