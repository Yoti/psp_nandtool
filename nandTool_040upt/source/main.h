#ifndef MAIN_H
#define MAIN_H

#include <pspsdk.h>
#include <pspkernel.h>
//#include <pspdebug.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <graphics.h>
#include "ntbridge.h"

//#define DEBUG

#define printf myScreenPrintf
#define setc myScreenSetXY
#define swap myScreenSwapBuffer

#define MODE_IPL 0
#define MODE_LFLASH 1
#define MODE_XIDSTORE 2
#define MODE_IDSTORE 3
#define MODE_FULL 4

#define SPARE_SIZE 	16
#define SPARE_NOUECC_SIZE 12

#define C_RED 		0x000000ff
#define C_GREEN 	0x0000ff00
#define C_BLUE 		0x00ff0000
#define C_LT_BLUE	0x00FF9C11
#define C_PURPLE 	0x00ff00ff
#define C_YORANGE	0x00119CFF
#define C_WHITE 	0x00fffff0 // tiny bit of blue is whiter than white
#define C_GREY		0x00aaaaaa
#define C_BLACK 	0x00000000
//#define FBOX_COLOR	0x00fffff0
#define FBOX_COLOR	0x0068b908
#define C_SEL_BG	0x00aaaaaa
#define C_SEL_FG	0x00000000
#define C_NSEL_FG	0x00aaaaaa

#define SAFE 		1
#define NOT_SAFE 	2

#define DUMP_PATH 0
#define ELF_PATH 1 // pathBase[DUMP_PATH] pathBase[DUMP_PATH]
#define BASE_DUMP_PATH "ms0:/nandTool_dumps"
#define BASE_ELF_PATH "ms0:/elf"
#define BASE_DUMP_PATH_HOST "host0:/nandTool_dumps"

// some place to stow all the globals
typedef struct{
int ppb; // pages per block
int ttlblock; // total blocks in NAND
int pagesz; // page size x
int filesize; // file size of NAND dump x
int moboType; // number representation of the mobo type x
u8 isSlim;
int nszMiB; // NAND user size in MiB x
u32 idsmag; // seed for ids decrypt/encrypt x
u32 tachyon; // tachyon version number x
u32 baryon; // baryon version number x
u64 fuseid; // fuse info - not sure this is necessary
u32 fusecnf;
u8 usbEnable; // mask for USB functions
u8 usbStat;
u32 dkver; // devkit version, mainly to be used to know which USB prxs to load
char mobo[20]; // name for mobo type x
char fnameBase[40]; // base of all filenames for the current unit
char pathBase[2][40]; // base path
} hard_info;

typedef struct{
char fname[256];
char sname[256]; // display name, sans underscores and extension
} file_names;

// external prototypes
void showMenu (int refresh); // only used in main.c
void appHeader(int msgbox); // shows the application's header, init percent bar and msg box if msgbox == 1
int LoadStartModule(char *path);
void wait_release(unsigned int buttons);
unsigned int wait_press(unsigned int buttons);
int confirm(void); // returns 1 when pressing x, 0 when pressing O
u32 getKeys(void); // returns the current pad.Buttons
void cprintf(u32 color, const char* text, ...);
void showMsg(const char* title, const char* text, ...);
void showMsgBox(void);
void errorExit(const char *fmt, ...); // displays an error message and exits the application
void LockFlash(u8 write); // set write to true for write access
void UnlockFlash();
void ClearCaches(void);
void verifyCreateDumpDir(void);

#endif //  MAIN_H 
