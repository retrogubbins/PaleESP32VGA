#ifndef __PALEDISK_H__
#define __PALEDISK_H__

extern int disk_mode;
void set_d_mode(int ff);

#define HEADS_PER_DISK 1              //change this to 2 FOR 400k DOUBLE SIDE DISK
#define TRACKS_PER_DISK 40
//#define HEADS_PER_DISK 2
//#define TRACKS_PER_DISK 80

#define SECT_PER_TRACK 10
#define BYTES_PER_SECT 512
//#define BYTES_PER_TRACK BYTES_PER_SECT*SECT_PER_TRACK
//#define BYTES_PER_TRACK 6040   
#define BYTES_PER_TRACK BYTES_PER_SECT*SECT_PER_TRACK

//There are 604 bytes in each (physical) sector - 512 data rest ID numbers
//A track is 6040 bytes long

#define BYTES_PER_SIDE SECT_PER_TRACK*BYTES_PER_SECT*TRACKS_PER_DISK


//#define LYNX_DISK_SIZE HEADS_PER_DISK*BYTES_PER_SECT*SECT_PER_TRACK*TRACKS_PER_DISK
//#define LYNX_MAX_DISK_SIZE 800*1024
#define LYNX_MAX_DISK_SIZE HEADS_PER_DISK*BYTES_PER_SIDE //800k is 2 heads of 80 tracks

#define BADDRIVE 0
#define DRIVE200K 40
#define DRIVE800K 80
#define NOOF_DRIVES 1

extern char disk_options;
extern bool disk_rom_enabled;

void kill_disk(int);
void init_disks();
void open_working_disk(int);
void save_working_disk();
void reset_disk_vars();

void disk_outp(unsigned int Port,unsigned char Value);
unsigned char disk_inp(unsigned int Port);
//extern  int  load_ldisk(int driveno,char fnam[]);
//extern  int  save_ldisk(int driveno,char fnam[]);

#endif
