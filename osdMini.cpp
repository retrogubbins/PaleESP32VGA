
/*
#define MY_SCREEN_WIDTH 31
#define MY_SCREEN_HEIGHT 31


#define CHAR_WIDE   8
#define CHAR_HIGH   8
#define MAX_CHAR  256

#define CHAR_SIZE ((CHAR_HIGH * CHAR_WIDE) / 8)

#define CHAR_FILL 0xDB

// Global Variables
unsigned char charmap[MAX_CHAR][CHAR_SIZE];


// Function Prototypes
void init_fonts(void);
int  load_font(char *fname);
void showchar(int curr_char, int xpos, int ypos);

//extern d horiz_vidbytes;


void init_fonts(void)
{
    memset(charmap, 0, (MAX_CHAR * CHAR_SIZE));
}

unsigned char byte_reverse(unsigned char value)
{
    value = (value & 0x0f) << 4 | (value & 0xf0) >> 4;
    value = (value & 0x33) << 2 | (value & 0xcc) >> 2;
    value = (value & 0x55) << 1 | (value & 0xaa) >> 1;
    return value;
}


int load_font(char *fname)
{
    FILE *fp;
    int i,j;

    fp = fopen(fname, "rb");

    for (i = 0; i < MAX_CHAR; i++) {
        fread(&charmap[i][0], CHAR_SIZE, 1, fp);
        for(j=0;j<CHAR_SIZE;j++)
            charmap[i][j] = byte_reverse(charmap[i][j]);
    }

    fclose(fp);

    return 1;
}


int mycursor_x = 0;
int mycursor_y = 0;
unsigned char mycolour_foreground = 7;
unsigned char mycolour_background = 0;

void showchar(int curr_char, int xpos, int ypos)
{
    unsigned char *bytes;
    
    int i, j, temp,memx,bank,retbyte,scanline;
    unsigned char paper,ink;
    
    unsigned char *vmem;

    vmem = vidmem +(ypos * CHAR_HIGH * horiz_vidbytes) + xpos;
    for(scanline = 0;scanline<CHAR_HIGH;scanline++)
    {
        bytes=vmem+(scanline*horiz_vidbytes);   //80 bytes
        if(horiz_vidbytes == HORIZ_BYTES_640x480)bytes+=LCD_LEFT_BORDER;
        //get pointer to character
        retbyte =  charmap[curr_char][scanline];
        for(bank = 0;bank < 3;bank++)
        {
            if(((unsigned char)1 << bank) & mycolour_background)
            {
                paper = 0xff;
            }
            else
                paper = 0x0;
            if(((unsigned char)1 << bank) & mycolour_foreground)
            {
                ink = retbyte;
            }
            else
                ink = 0x0;
            plane(bank);
            *((unsigned char *)(bytes)) = ink | paper;
        }    
    }

}

void my_paint(unsigned char x1,unsigned char x2,unsigned char y1,unsigned char y2)
{
    unsigned char *bytes;
    unsigned char write_byte;
    int i, j, temp,memx,bank,retbyte,scanline,vert_h;
//    unsigned char *vidmem=(unsigned char *)0xa0000;
    unsigned char *vmem;
    unsigned char pix_height,bytes_width,bytes_high,n;
    char lbl[200];
  
    vmem = vidmem +(y1 * CHAR_HIGH * horiz_vidbytes);
    bytes_high = y2 - y1;
    pix_height = bytes_high * CHAR_HIGH;
    bytes_width = x2 - x1;

   // sprintf(lbl,"pix_height = %d, bytes_width = %d, bytes_high is %d,CHH is %d",pix_height,bytes_width,bytes_high,CHAR_HIGH);
   // my_move(0,5);
   // my_print(lbl);
    for(bank = 0;bank < 3;bank++)
    {
        plane(bank);
        if(((unsigned char)1 << bank) & mycolour_foreground)
            write_byte = 0xff;
        else
            write_byte = 0x0;
        for(scanline = 0;scanline<pix_height;scanline++)
        {
            bytes=vmem+(scanline*horiz_vidbytes)+x1; 
            if(horiz_vidbytes == HORIZ_BYTES_640x480)bytes+=LCD_LEFT_BORDER;
            for(n = 0;n < bytes_width;n++)
            {
                *((unsigned char *)(bytes+n)) = write_byte;
            }
        }    
    }


}

void my_move(unsigned char x,unsigned char y)
{
    mycursor_x = x;
    mycursor_y = y;
}

void my_print(char text[])
{
    char c;
    int c_pos = 0;

    c = text[c_pos++];
    while(c!='\0')
    {
        switch(c)
        {
            case '\b'://backspace
                if(mycursor_x > 0)mycursor_x--;
                break;
            case '\t'://tab
                if(mycursor_x < (MY_SCREEN_WIDTH-7))mycursor_x +=5;
                break;
            case '\n'://newline
                mycursor_x = 0;
                mycursor_y ++;
                break;
            case '\f'://form feed -  here used as set foreground colour
                c = text[c_pos++];
                mycolour_foreground = c - '0';
                break;
            case '\a'://beep -  here used as set background colour
                c = text[c_pos++];
                mycolour_background = c - '0';
                break;

            default:
                showchar(c,mycursor_x,mycursor_y);
                mycursor_x ++;
                break;
        }
        
        if(mycursor_x > MY_SCREEN_WIDTH)
        {
            mycursor_x = 0;
            mycursor_y ++;
        }
        if(mycursor_y > MY_SCREEN_HEIGHT)
            mycursor_y = 0;

        c = text[c_pos++];
    }
}

void my_input(char *prompt,char *text)
{
    char c;
    char lbl[200];
    char charout[4];
    int text_ptr = 0;
    //Get input string from the user

    char cursor[5] = {'<',' ','\b','\b','\0'};

    sprintf(lbl,"%s",prompt);
    my_print(lbl);
    my_print(cursor);

    //unhook the keyboard from the emu
    close_keys();

    c = getch();
    while(c != 13 && c != 27)
    {
        if(c == 8)//backspace
        {
            if(text_ptr > 0)
            {
                text_ptr--;
                my_print("\b");
                my_print(cursor);
            }
        }
        else
        {
            text[text_ptr++] = c;
            sprintf(charout,"%c",c);        
            my_print(charout);
            my_print(cursor);
            //my_print(" -");
            //sprintf(lbl," %d ",(unsigned int)c);        
            //my_print(lbl);
    
            if(text_ptr>50) break;
        }
        c = getch();
    }
    text[text_ptr] = '\0';
    hook_keys();
}

void show_help()
{
    char name[200];
    char lbl[200];

//      my_show_speed();

//vidmem = VIDEO_PAGE1;
//Another way of doing it
//vidmem = vga_mem;

    my_print("\f2");
    my_paint(0,32,6,27);
    my_move(0,7);
        my_print("\f6P\f4a\f2l\f3e\f5DOS\n\f6Version:");
        sprintf(lbl,"%.2f",PALE_REV);
        my_print(lbl);
        my_print("\n\n\f1Peter Todd 2008\n\n");
    my_print("\f7\a2");
    my_print("F2  - Show Status\n");
    my_print("F3  - Pause Emu\n");
    my_print("F4  - Toggle Speed\n");
    my_print("F5  - Change Machine\n");
    my_print("F6  - Show DPMI Status\n");
    my_print("F7  - Load LSF Snapshot\n");
    my_print("F8  - Load TAP File\n");
    my_print("F9  - Reset Z80\n");
    my_print("F10 - Load LDF Disk File\n");
    my_print("F11 - Hard Reset Machine\n");
    my_print("F12 - Quit Pale\n\n");
    my_print("\f7\a0\n");
    my_input("Press Return ",name);
}


void my_show_directory(char *pathext,unsigned int highlight,unsigned char *name)
{
    char cmdresult[4000];
    my_print("\f0");
    my_paint(0,31,0,20);
    my_print("\f2\a0");
    my_move(0,0);
    get_directory(pathext,cmdresult,highlight,name);
    my_print(cmdresult);
    my_print("\f7\a0");
}



void my_filechooser(char *pathext,unsigned char *name)
{
        unsigned int highlight = 0;
        char list_dir[256];

        while(1)
        {       
                my_show_directory(pathext,highlight,name);
                while(1)
                {
                        //no need to poll anything, interrupts work magic in the background :)
                        if (key[SDLK_F12])
                        {
                                finish_emu=1;
                        return;
                        }
                        if (key[SDLK_ESCAPE])
                                return;
                        if (key[SDLK_UP])
                        {
                                waitkeyup(SDLK_UP);
                                if(highlight>0)highlight--;
                        break;
                        }
                        if (key[SDLK_DOWN])
                        {
                                waitkeyup(SDLK_DOWN);
                                highlight++;
                                break;          
                        }
                        if (key[SDLK_RETURN])
                                return;
                }       
        }
}



void my_show_speed()
{
        char lbl[20];
        int s = emuspeeds[emu_speed];
    my_print("\f0");
    my_paint(0,10,0,1);
    my_print("\f4\a0");
    my_move(0,0);
    sprintf(lbl,"%d\%",s);
    my_print(lbl);
    my_print("\f7\a0");
}

*/


/*
//Directory functions


#include <stdio.h>
#include <direct.h>

typedef struct {
    unsigned short twosecs : 5; // seconds / 2 
    unsigned short minutes : 6;
    unsigned short hours : 5;
}ftime_t;

typedef struct {
    unsigned short day : 5;
    unsigned short month : 4;
    unsigned short year : 7;
}fdate_t;

void get_directory(char *pathext,char *listing,unsigned int highlight,unsigned char *name)
{
    DIR *dirp;
    struct dirent *direntp;
    ftime_t *f_time;
    fdate_t *f_date;
    char lbl[200],lbl2[200];
    char list_dir[256];
    int noof_entries_horiz,noof_entries_vert = 0;
        int noof_entries = 0;
 
    sprintf(listing,"Directory:\n");

    sprintf(list_dir,"%s\\*.%s",pathext,pathext);
    dirp = opendir( list_dir );
    if( dirp != NULL )
    {
        noof_entries_horiz = 0;
        for(;;)
        {
            direntp = readdir( dirp );
            if(direntp == NULL)
                break;
                        f_time = (ftime_t *)&direntp->d_time;
            f_date = (fdate_t *)&direntp->d_date;
                        if(noof_entries == highlight)
                        {
                            sprintf(lbl,"\a1\f7");
                            sprintf(lbl2,"%s",direntp->d_name);
                            strcat(lbl,lbl2);
                            sprintf(lbl2,"\a0\f2   ");
                            strcat(lbl,lbl2);
                            strcpy(name,direntp->d_name);
                        }
                        else
                            sprintf(lbl,"%s   ",direntp->d_name);
            
                        noof_entries_horiz++;
            if(noof_entries_horiz > 1)
            {
                strcat(listing,"\n");    
                noof_entries_horiz = 0;
                //noof_entries_vert++;
                //if(noof_entries_vert > 15)break;
            }
            noof_entries ++;
            strcat(listing,lbl);    
        }
        closedir(dirp);
     }
    sprintf(lbl,"\n\n\a1\f4Filename: ");
    strcat(lbl,name);
    strcat(listing,lbl);    
     strcat(listing,"\0");
    sprintf(list_dir,"%s\\%s",pathext,name);
    strcpy(name,list_dir);

}

*/
