/*
	NandTool - dump/restore nand images in fractions
*/

#include <pspctrl.h>
#include <psppower.h>
//#include <pspsyscon.h>
//#include <pspidstorage.h>
//#include <psputilsforkernel.h>

//#include <pspdebug.h>

#include "main.h"

// include prototypes for specific operations
#include "write.h"
#include "dump.h"
#include "other.h"
#include "idstore.h"
#include "lflash.h"
#include "usb.h"

#include "splash.h"
#include "warn.h"

#ifndef RELVER
	#define RELVER "0.xx"
#endif

//PSP_MODULE_INFO("NandTool", 0x200, 1, 0);
PSP_MODULE_INFO("NandTool", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);

PSP_HEAP_SIZE_KB(20*1024);

#define MAX_FILES 256 // the number of filenames in the fnames array

// some constants
#define ROW_OFFSET 3 	// the number of lines down that the menu begins on
#define ROW_FILE_OFFSET 4
#define ROW_MAX 29 		// the max number of lines down for showing files - gives ROW_MAX-ROW_FILE_OFFSET+1 lines (29 gives 26 files shown)
enum {
	ELF_LOAD, // 0
	READ_MENU, // 1
	WRITE_MENU,// 2
	LFLASH_MENU, // 3
	WRITE_OTHER,// 4
	SHOW_INFO, // 5
	QUIT, // 6
	// these are other menus
	MAIN_MENU, // 
	WRITE_SUB, // 
	WRITE_KEYS,// 
};

// some globals
static u16 row;
static u16 rowprev;
static int menu;
static int maxrow;
static int filesel;
static u8 flashLocked;// keep track of whether the flash is currently locked from changes
static int ttlfiles;
static int currfile;
static u8 shutDown;
static u8 otherFormat;
static u8 fade;

u32 lsResult;

hard_info hard;
file_names fnames[MAX_FILES];

// printf with a color component
void cprintf(u32 color, const char* text, ...)
{
	va_list list;
	char msg[256];	

	va_start(list, text);
	vsprintf(msg, text, list);
	va_end(list);

	myScreenSetTextColor(color);
	printf(msg);
	myScreenSetTextColor(C_WHITE);
	return;
}

void errorExit(const char *fmt, ...)
{
	va_list list;
	char msg[256];	

	va_start(list, fmt);
	vsprintf(msg, fmt, list);
	va_end(list);

	myScreenClear();
	cprintf(C_RED,"Error: ");
	printf(msg);
	printf("\n\nexiting shortly...");
	swap();
	sceKernelDelayThread(5*1000*1000);
	sceKernelExitGame();
}

void showMsgBox(void)
{
	showPercentBar(26, 3, FBOX_COLOR, C_RED, C_GREEN, C_BLACK);
	drawTextBox(3, 5, 63, 26, FBOX_COLOR);
	return;
}

void showMsg(const char* title, const char* text, ...)
{
	va_list list;
	char msg[256];	

	va_start(list, text);
	vsprintf(msg, text, list);
	va_end(list);

	removeBound();
	setc(3,3);
	myScreenSetTextColor(C_LT_BLUE);
	printf(title);
	myScreenSetTextColor(C_WHITE);
	printf(msg);
	restoreBound();
	return;
}

void showInfo(int safety, const char* text)
{
	setc(58,29);
	if(safety != 0)
	{
		if(safety == NOT_SAFE)
		{
			cprintf(C_RED,"dangerous");
		}
		else if(safety == SAFE)
		{
			cprintf(C_GREEN,"     safe");
		}
	}
	else
		printf("         ");
	if(row != 0)
	{
		setc(0,31);
		printf("                                                                                                              ");
	}
	setc(0,31);
	printf(text);
	if(row == 0)
	{
		setc(0, 29);
		cprintf(C_BLUE,"Option Info:");
		drawLine(0,228,479,C_BLUE);
		drawLine(0,242,479,C_BLUE);
	}
	setc(0, row + ROW_OFFSET +1);
	return;
}

// checks to see whether the files we have defined are usuable
// 0= does not exist, 1=exists
int fileExists(const char* fpath)
{
	SceUID fp;
	int ret = 0;
	fp = sceIoOpen(fpath, PSP_O_RDONLY, 0777);
	if (fp > 0)
		ret = 1;
	sceIoClose(fp);
	return ret;
}
/*
void ClearCaches()
{
	sceKernelDcacheWritebackAll();
	sceKernelIcacheInvalidateAll();
}
*/
// Disable system-wide flash write access
void LockFlash(u8 write) 
{
	if (!flashLocked)
		ntNandLock(write);
	flashLocked = 1;
	scePowerLock(0);
	return;
}

// Enable system-wide flash write access
void UnlockFlash(void) 
{
	if (flashLocked)
		ntNandUnlock();
	flashLocked = 0;
	scePowerUnlock(0);
	return;
}

void appHeader(int msgbox)
{
	myScreenClear();
	cprintf(C_BLUE," NandTool %s\n\n\n", RELVER);
//	while (confirm() != 1){};
	drawLine(0, 10, 117, C_BLUE);
	drawLine(0, 12, 117, C_BLUE);
	if(msgbox == 1)
		showMsgBox();
	else if(msgbox == 2)
		drawTextBox(3, 5, 63, 26, FBOX_COLOR);
	return;
}

// returns 0 if success, -1 if load fails, -2 if start fails
int LoadStartModule(char *path)
{
    u32 loadResult;
    u32 startResult;
    int status;
	SceKernelLMOption option;
	SceUID mpid = 1;
	memset(&option, 0, sizeof(option));
	option.size = sizeof(option);
	option.mpidtext = mpid;
	option.mpiddata = mpid;
	option.position = 0;
	option.access = 1;

    loadResult = sceKernelLoadModule(path, 0, &option);
    if (loadResult & 0x80000000)
	{
		lsResult = loadResult;
		return -1;
    }
	else
	startResult = sceKernelStartModule(loadResult, 0, NULL, &status, NULL);

    if (loadResult != startResult)
	{
		lsResult = startResult;
		return -2;
    }
	return 0;
}

// I hate writing controller code, this is from Chilly Willy's app as it works well for this
void wait_release(unsigned int buttons)
{
	SceCtrlData pad;

	sceCtrlReadBufferPositive(&pad, 1);
	while (pad.Buttons & buttons)
	{
		sceKernelDelayThread(10000);
		sceCtrlReadBufferPositive(&pad, 1);
	}
}

unsigned int wait_press(unsigned int buttons)
{
	SceCtrlData pad;

	sceCtrlReadBufferPositive(&pad, 1);
	while (1)
	{
		if (pad.Buttons & buttons)
			return pad.Buttons & buttons;
		sceKernelDelayThread(10000);
		sceCtrlReadBufferPositive(&pad, 1);
	}
	return 0;   /* never reaches here, again, just to suppress warning */
}

int confirm(void)
{
	SceCtrlData pad;

	while (1)
	{
		sceKernelDelayThread(10000);
		sceCtrlReadBufferPositive(&pad, 1);
		if(pad.Buttons & PSP_CTRL_CROSS)
		{
			wait_release(PSP_CTRL_CROSS);
			return 1;
		}
		if(pad.Buttons & PSP_CTRL_CIRCLE)
		{
			wait_release(PSP_CTRL_CIRCLE);
			return 0;
		}
	}
	return 0;   /* never reaches here, this suppresses a warning */
}
// end excerpt from Chilly Willy's app

u32 getKeys(void)
{
	SceCtrlData pad;
	sceCtrlReadBufferPositive(&pad, 1);
	return pad.Buttons;
}

int areYouSure(void)
{
	SceCtrlData pad;
	int ret = 0;
	setc(0,10);
	printf("Are you sure you wish to proceed?\n");
	cprintf(C_PURPLE,"press X to cancel or triangle to confirm"); 
	swap();
	while (1)
	{
		sceKernelDelayThread(10000);
		sceCtrlReadBufferPositive(&pad, 1);
		if(pad.Buttons & PSP_CTRL_CROSS)
		{
			wait_release(PSP_CTRL_CROSS);
			ret = 0;
			break;
		}
		if(pad.Buttons & PSP_CTRL_TRIANGLE)
		{
			wait_release(PSP_CTRL_TRIANGLE);
			ret = 1;
			break;
		}
		if((pad.Buttons & PSP_CTRL_SELECT)&&(pad.Buttons & PSP_CTRL_LTRIGGER))
		{
			wait_release(PSP_CTRL_SELECT);
			ret = 2;
			break;
		}
	}
	setc(0,10);
	printf("                                       \n"); 
	printf("                                       \n");
	setc(0,4);
	swap();
	return ret;
}

void exitApp(u32 time)
{
	if(time>0)
		sceKernelDelayThread(time);//2*1000*1000
	if(!shutDown)
		sceKernelExitGame();
	else
		ntSysconPowerStandby();
}

void quitSel(void)
{
	fadeTo(C_BLACK, 31);
	
	appHeader(0);
	drawTextBox(4, 4, 20, 3, FBOX_COLOR);

	if(shutDown)
		printf("Shutting down...\n\n   ");
	else
		printf("Quitting...\n\n   ");
	fadeIn();
	myScreenSetBackColor(C_LT_BLUE);
	myScreenPutSChar(RO_LEFT);
	cprintf(C_BLACK,"Now");
	myScreenPutSChar(RO_RIGHT);
	myScreenSetBackColor(C_BLACK);
	swap();
	exitApp(0);
}

void getPspVer(void)
{
	char mobos[11][15]= {"UNKNOWN", "TOOL-TEST", "TA-079_V1_Fat", "TA-079_V2_Fat", "TA-079_V3_Fat", "TA-081_Fat", "TA-082_Fat", "TA-086_Fat", "TA-085_V1_Slim", "TA-085_V2_Slim", "TA-088_Slim"};
	hard.tachyon = ntSysregGetTachyonVersion();
	ntSysconGetBaryonVersion(&hard.baryon);
	hard.fuseid = ntSysregGetFuseId();
	hard.fusecnf = ntSysregGetFuseConfig();
	hard.moboType = 0;
	hard.isSlim = 0;
	if(hard.tachyon == 0x00100000) hard.moboType = 1; // TOOL-TEST
	else if (hard.tachyon == 0x00140000) hard.moboType = 2; // TA-079_V1_Fat
	else if (hard.tachyon == 0x00200000)
	{
		if(hard.baryon == 0x00030600) hard.moboType = 3; // TA-079_V2_Fat
		else if(hard.baryon == 0x00040600) hard.moboType = 4; // TA-079_V3_Fat
	}
	else if (hard.tachyon == 0x00300000) hard.moboType = 5; // TA-081_Fat
	else if (hard.tachyon == 0x00400000)
	{
		if(hard.baryon == 0x00114000) hard.moboType = 6; // TA-082_Fat
		else if(hard.baryon == 0x00121000) hard.moboType = 7; // TA-086_Fat
	}
	else if (hard.tachyon == 0x00500000)
	{
		if(hard.baryon == 0x0022B200) hard.moboType = 8; // TA-085_V1_Slim
		else if(hard.baryon == 0x00234000) hard.moboType = 9; // TA-085_V2_Slim
		else if(hard.baryon == 0x00243000) hard.moboType = 10; // TA-088_Slim
		hard.isSlim = 1;
	}
	strcpy (hard.mobo, mobos[hard.moboType]);
	return;
}

void showHardInfo(void)
{
	appHeader(0);
	drawTextBox(4, 3, 45, 26, FBOX_COLOR);

	cprintf(C_GREEN,"Hardware Information\n--------------------\n");
	// mobo info
	cprintf(C_LT_BLUE,"Mobo Version : ");
	printf("%s\n", hard.mobo);
	cprintf(C_LT_BLUE,"Tachyon      : ");
	printf("0x%08x\n", hard.tachyon);
	cprintf(C_LT_BLUE,"Baryon       : ");
	printf("0x%08x\n", hard.baryon);
	// NAND info
	cprintf(C_GREEN,"\nNAND Information\n----------------\n");
	cprintf(C_LT_BLUE,"Size         : ");
	printf("%i MiB\n", hard.nszMiB);
	cprintf(C_LT_BLUE,"Total blocks : ");
	printf("%i\n", hard.ttlblock);
	cprintf(C_LT_BLUE,"Pages/block  : ");
	printf("%i ppb\n", hard.ppb);
	cprintf(C_LT_BLUE,"Page size    : ");
	printf("%i bytes\n", hard.pagesz);
	cprintf(C_LT_BLUE,"Spare size   : ");
	printf("%i bytes\n", SPARE_SIZE);
	cprintf(C_LT_BLUE,"File Size    : ");
	printf("%i bytes\n", hard.filesize);
	// other info
	cprintf(C_GREEN,"\nOther Information\n-----------------\n");
	cprintf(C_LT_BLUE,"IDS scramble : ");
	printf("0x%08x\n", hard.idsmag);
	cprintf(C_LT_BLUE,"FuseId       : ");
	printf("0x%012llx\n", hard.fuseid);
	cprintf(C_LT_BLUE,"FuseConfig   : ");
	printf("0x%08x\n", hard.fusecnf);
	cprintf(C_LT_BLUE,"Devkit Ver   : ");
	printf("0x%08x\n", hard.dkver);
	cprintf(C_LT_BLUE,"ELF Path     : ");
	printf("%s\n", hard.pathBase[ELF_PATH]);
	cprintf(C_LT_BLUE,"Dump Path    : ");
	printf("%s\n", hard.pathBase[DUMP_PATH]);
	cprintf(C_LT_BLUE,"Filename Base: ");
	printf("%s\n", hard.fnameBase);
	cprintf(C_LT_BLUE,"USB Status   : ");
	if(hard.usbEnable == 1)
		cprintf(C_GREEN,"USB available\n\n");
	else if(hard.usbEnable == 2)
		cprintf(C_GREEN,"USB and HOST available");
	else
		cprintf(C_RED,"USB modules not installed");
	removeBound();
	setc(0, 30);
	cprintf(C_PURPLE,"    Press X or O to return");
//	fadeIn();
	swap();
	confirm();
	return;
}

void showSplash(void)
{
	loadBmpFromBuf((u8*) splash);
	fadeIn();
	sceKernelDelayThread(1*1000*1000);
	fadeTo(C_BLACK, 31);
}

void showWarn(void)
{
/*	appHeader(0);
	cprintf(C_BLUE,"Mobo Version: ");
	printf("%s\n", hard.mobo);
	cprintf(C_RED,"\n**WARNING**\n\n");
	printf("Writing to NAND is plausibly dangerous to your PSP.\n\n");
	printf("  If you choose to accept, ALL risks associated to using this\n");
	printf("program are yours alone and you agree to not hold the author of\n");
	printf("this program or any other person(s) responsible for your actions.\n");
	printf("You may also skip this notice by creating a file called 'iaccept'\n");
	printf("in the folder /elf/ on the root of your MS.\n\n\n");
	cprintf(C_GREEN,"Be sure to backup your data before modifying it.\n\n");
	cprintf(C_PURPLE,"Press O to accept or X to quit.\n\n\n");
	printf("Thanks to everyone at ps2dev, lan.st and in Prometheus\n");
	printf("Without your work we'd still be dreaming... without your help\n");
	printf("this program would never have been completed\n\n");
	printf(" cory1492\n");
*/
	loadBmpFromBuf((u8*) warn);
	fadeIn();

	if (confirm() == 1)
	{
		fadeTo(C_BLACK, 31);
	
		appHeader(0);
		drawTextBox(4, 4, 20, 3, FBOX_COLOR);

		printf("Quitting...\n\n   ");
		fadeIn();
		myScreenSetBackColor(C_LT_BLUE);
		myScreenPutSChar(RO_LEFT);
		cprintf(C_BLACK,"Now");
		myScreenPutSChar(RO_RIGHT);
		myScreenSetBackColor(C_BLACK);
		swap();
		exitApp(0);
	}
	fadeTo(C_BLACK, 31);
	return;
}

void filterName(char* sname, int isdir) // remove extension and underscore
{
	int i, len;
	if(!isdir)
	{
		char* pch = strrchr(sname,'.');
		if(pch != NULL)
			pch[0] = '\0';
	}
	len = strlen(sname);
	for(i=0; i<len; i++) // remove underscore
		if(sname[i] == '_') sname[i] = ' ';
}

// traverse dirent to find valid files for this specific PSP
// stat can be either FIO_SO_IFREG, FIO_SO_IFDIR, sized=0 not care, sized=1 size conforms to NAND, sized=2 looking for .elf extension
void updateFileStat(u32 stat, u8 sized, int pt)
{
	SceIoDirent tmp;
	int fd, ret;//, i;
	ttlfiles = 0;
	currfile = 0;
	memset(&tmp, 0, sizeof(tmp)); // might be important?
	fd = sceIoDopen(hard.pathBase[pt]);
	
    if (fd<0)
	{
		ttlfiles = -1;
		return;
//		errorExit("opening directory %s", hard.pathBase[pt]);
	}
	else
	{
		while(1)
		{
			ret = sceIoDread(fd, &tmp);
//			printf(" "); // not sure why I put this...
			if (ret<=0)
				break;
			if(tmp.d_stat.st_attr & stat) // check dirent for proper stat
			{
				if(tmp.d_name[0] != '.') // filter out . and .. entries
				{
					if(sized == 1) // if we are size matching the NAND
					{
						if(tmp.d_stat.st_size == hard.filesize)
						{
							strcpy(fnames[ttlfiles].fname, tmp.d_name);
							strcpy(fnames[ttlfiles].sname, tmp.d_name);
							filterName(fnames[ttlfiles].sname, 0);
							ttlfiles++;
						}
					}
					else if(sized == 2)
					{
						char* pch = strrchr(tmp.d_name,'.');
						if(pch != NULL)
						{
							if((strcmp(pch, ".elf")==0)||(strcmp(pch, ".ELF")==0))
							{
								strcpy(fnames[ttlfiles].fname,tmp.d_name);
								strcpy(fnames[ttlfiles].sname, tmp.d_name);
								filterName(fnames[ttlfiles].sname, 0);
								ttlfiles++;
							}
						}
					}
					else // if we aren't size matching,  like directories
					{
						strcpy(fnames[ttlfiles].fname,tmp.d_name);
						strcpy(fnames[ttlfiles].sname, tmp.d_name);
						filterName(fnames[ttlfiles].sname, 1);
						ttlfiles++;
					}
				}
			}
		}
		sceIoDclose(fd);
	}
	return;
}

void displayFiles(void)//writeMenu (void)
{
	u32 len;
	char fnameSh[68]; // filename show string
	setc(0, ROW_FILE_OFFSET-2);
	cprintf(C_LT_BLUE," path: ");
	if((menu == WRITE_MENU)||(menu == WRITE_KEYS))
		printf("%s", hard.pathBase[DUMP_PATH]);
	else
		printf("%s", hard.pathBase[ELF_PATH]);
	if(ttlfiles <= 0)
	{
		setc(5, 4);
		if(ttlfiles == 0)
			printf("no valid files files found...");
		else if(ttlfiles == -1)
			cprintf(C_RED,"ERROR: could not open directory");
//		setc(5, 6);
//		cprintf(C_PURPLE,"press O to return");
		maxrow = 0;
//		return;
	}
	else
	{
		maxrow = (ttlfiles-1);
		if(maxrow > (ROW_MAX-ROW_FILE_OFFSET))
			maxrow = ROW_MAX-ROW_FILE_OFFSET;
		int i;
		for(i=0; i<(maxrow+1); i++)
		{
			setc(4, (i+ROW_FILE_OFFSET));
			memset(&fnameSh, 0, sizeof(fnameSh));
			len = strlen(fnames[i+currfile].sname);
			strncat(fnameSh, fnames[i+currfile].sname, (len < 60 ? len :60));
			if (i == row)
			{
				myScreenSetBackColor(C_SEL_BG);
				myScreenPutSChar(RO_LEFT);
				cprintf(C_SEL_FG,"%s", fnameSh);
				myScreenPutSChar(RO_RIGHT);
				myScreenSetBackColor(C_BLACK);
			}
			else
				cprintf(C_NSEL_FG," %s ", fnameSh);
		}
	}
	setc(0,31);
	cprintf(C_LT_BLUE,"    %i matching file(s) found...\n", ttlfiles);
	cprintf(C_PURPLE,"    choose an entry and press X to select or press O to return");
	return;
}

// ****other menu start
void checkbb(void)
{
	int bad;
	appHeader(1);
	cprintf(C_GREEN,"Bad Block Check\n");
	swap();
	bad = checkBlock();
	cprintf(C_GREEN,"Done, %d bad block(s) found\n", bad);
	cprintf(C_PURPLE,"Press X to continue.");
	swap();
	while (confirm() != 1){};
	return;
}

void lcheck(void)
{
	int err;
	appHeader(2);
	cprintf(C_GREEN,"Check partition integrity\n");
	swap();
	err = lfsCheck();
	if(err == 0)
		cprintf(C_GREEN,"Done, %d inconsistencies found\n", err);
	else
		cprintf(C_RED,"Done, %d inconsistencies found\n", err);
	cprintf(C_PURPLE,"Press X to continue.");
	swap();
	while (confirm() != 1){};
	return;
}

void lformat(void)
{
	appHeader(2);
	cprintf(C_GREEN,"Logically repartition lflash area\n");
	if (areYouSure() == 1)
	{
		appHeader(1);
		swap();
		rawLflashFmt();
	}
	else
		return;
	cprintf(C_PURPLE,"\nDone, Press X to continue.");
	swap();
	while (confirm() != 1){};
	return;
}

void dkeys (void)
{
	char fname[256];
	appHeader(2);
	cprintf(C_GREEN,"Dump idstore to key files\n");
	sprintf(fname,"%s/Keys_%s", hard.pathBase[DUMP_PATH], hard.fnameBase);
	if (areYouSure() == 1)
	{
		appHeader(1);
		swap();
		dumpKeys(fname);
	}
	else
		return;
	cprintf(C_PURPLE,"\nDone, Press X to continue.");
	swap();
	while (confirm() != 1){};
	return;
}

void wkeys(void)
{
	char fnamed[256];
	int sure;
	appHeader(2);
	cprintf(C_GREEN,"Write idstore from key files\n");
	sprintf(fnamed,"%s/%s", hard.pathBase[DUMP_PATH], fnames[filesel].fname);
//	printf("Chose: %s\n", fnamed);
	sure = areYouSure();
	if (sure == 1)
	{
		//ClearCaches();
		appHeader(1);
		swap();
		writeKeys(0, fnamed);
	}
	else if (sure == 2)
	{
		//ClearCaches();
		appHeader(1);
		swap();
		writeKeys(1, fnamed);
	}
	else
		return;
	cprintf(C_PURPLE,"\nDone, Press X to continue.");
	swap();
	while (confirm() != 1){};
	return;
}

void fparts(void)
{
	appHeader(2);
	cprintf(C_GREEN,"Format Partitions\n");
	cprintf(C_RED,"The program will exit or shutdown when done!\n");
	if(areYouSure() == 1)
	{
		appHeader(1);
		swap();
		formatPartitions();
	}
	else
		return;
	exitApp(2*1000*1000);
}

char writeOth[3][20] = {"Bad Block check", "Dump idstore keys", "Write idstore keys"};
char writeOthInfo [3][100] = {
"Checks NAND for bad blocks",
"Dump idstore keys to key files\n Creates a unique directory in the dump path",
"Erases then writes idstore from a key file dump\n Brings up a folder picker to choose keys to write",
};
u8 writeOthRisk [3] = {SAFE,SAFE,NOT_SAFE};
void writeOther(void) // raw write sub
{
	int i;
	maxrow = 2;
	for(i = 0; i<(maxrow+1); i++)
	{
		setc(4, ROW_OFFSET+i);
		if (i == row)
		{
			myScreenSetBackColor(C_SEL_BG);
			myScreenPutSChar(RO_LEFT);
			cprintf(C_SEL_FG,"%s", writeOth[i]);
			myScreenPutSChar(RO_RIGHT);
			myScreenSetBackColor(C_BLACK);
			showInfo(writeOthRisk[i], writeOthInfo[i]);
		}
		else
			printf(" %s ", writeOth[i]);
	}
	setc(0, maxrow+ROW_OFFSET+2);
	cprintf(C_PURPLE,"    press X to select or O to return\n");
	return;
}
// ****other menu end

char writeLflash[3][20] = {"Integrity check", "Repartition flash", "Format flash"};
char writeLflashInfo [3][100] = {
"Scans the logical flash area for valid partition records\n reports whether repartition is required",
"Reinstate partition records into lflash area of NAND\n Capable of working around bad blocks",
"Formats flash0/1/2/3\n Partitions must exist before they can be formatted"
};
u8 writeLflashRisk [3] = {SAFE,NOT_SAFE,NOT_SAFE};
void writeLflashMenu(void)
{
	int i;
	if(otherFormat)
		maxrow = 2;
	else
		maxrow = 1;
	for(i = 0; i<(maxrow+1); i++)
	{
		setc(4, ROW_OFFSET+i);
		if (i == row)
		{
			myScreenSetBackColor(C_SEL_BG);
			myScreenPutSChar(RO_LEFT);
			cprintf(C_SEL_FG,"%s", writeLflash[i]);
			myScreenPutSChar(RO_RIGHT);
			myScreenSetBackColor(C_BLACK);
			showInfo(writeLflashRisk[i], writeLflashInfo[i]);
		}
		else
			printf(" %s ", writeLflash[i]);
	}
	setc(0, maxrow+ROW_OFFSET+2);
	cprintf(C_PURPLE,"    press X to select or O to return\n");
	return;
}
// ****lflashmenu end

void makeDump (void)
{
	char file[256];
	int i = 0;
	// build the base file name
	sprintf(file,"%s/%s.bin", hard.pathBase[DUMP_PATH], hard.fnameBase);
	while(fileExists(file))
	{
		appHeader(2);
		printf("%s \n", file);
		cprintf(C_RED,"    already exists!\n", file);
		cprintf(C_PURPLE,"press X to increment or O to overwrite\n");
		swap();
		if(confirm() == 1)
		{
			sprintf(file,"%s/%s_%03d.bin", hard.pathBase[DUMP_PATH], hard.fnameBase, i);
			i++;
		}
		else break;
	}
	appHeader(2);
	cprintf(C_GREEN,"Dump to:\n");
	printf("%s\n\n", file);
	cprintf(C_PURPLE,"Press X to continue or O to cancel");
	fadeIn();
//	swap();
	if(confirm() == 1)
	{
		appHeader(1);
		cprintf(C_GREEN,"dumping to: \n");
		printf("%s\n", file);
		fadeIn();
		dumpNand(file);
		cprintf(C_PURPLE,"\nDone, Press X to continue.");
		swap();
		while (confirm() != 1){};
	}
	return;
}

void launchElf(void)
{
	char fnamed[256];
	struct SceKernelLoadExecParam param;
	sprintf(fnamed,"%s/%s", hard.pathBase[ELF_PATH], fnames[filesel].fname);
	fadeTo(C_BLACK, 31);

	appHeader(0);
	drawTextBox(4, 4, 60, 5, FBOX_COLOR);

	cprintf(C_GREEN," launching file:\n");
	printf(" %s\n\n ", fnamed);
	fadeIn();

	// change to the proper directory so that apps that load relative prx's will work
	sceIoChdir(hard.pathBase[ELF_PATH]);

	// setup the param struct
	memset(&param, 0, sizeof(param));
	param.size = sizeof(param);
	param.args = strlen(fnamed)+1;
	param.argp = fnamed;
	param.key = "game";

	// start the elf
	myScreenSetBackColor(C_LT_BLUE);
	myScreenPutSChar(RO_LEFT);
	cprintf(C_BLACK,"Now");
	myScreenPutSChar(RO_RIGHT);
	myScreenSetBackColor(C_BLACK);
	fadeIn();
	sceKernelLoadExec(fnamed, &param);

	// should never get past LoadExec
	cprintf(C_PURPLE,"\nDone, Press X to continue.");
	swap();
	while (confirm() != 1){};
	return;
}

// ****write menu start
void writeDump (int mode)
{
	char fnamed[256];
	int sure;
	appHeader(2);
	if(mode == MODE_IPL)
			cprintf(C_GREEN,"IPL write mode\n");
	else if(mode == MODE_LFLASH)
			cprintf(C_GREEN,"LFLASH write mode\n");
	else if(mode == MODE_XIDSTORE)
			cprintf(C_GREEN,"Exclude IDSTORE write mode\n");
	else if(mode == MODE_IDSTORE)
			cprintf(C_GREEN,"IDSTORE only write mode\n");
	else if(mode == MODE_FULL)
			cprintf(C_GREEN,"FULL write mode\n");

	sprintf(fnamed,"%s/%s", hard.pathBase[DUMP_PATH], fnames[filesel].fname);
	sure = areYouSure();
	if (sure == 1)
	{
		appHeader(1);
		swap();
		writeToNand(mode, 0, fnamed);
	}
	else if (sure == 2)
	{
		appHeader(1);
		swap();
		writeToNand(mode, 1, fnamed);
	}
	else
		return;
	cprintf(C_PURPLE,"\nDone, Press X to continue.");
	swap();
	while (confirm() != 1){};
	return;
}

char wsub[5][29] = {"Write IPL only", "Write Lflash only", "Write everything but idstore", "Write idstore only", "Write full image"};
char wsubInfo[5][60] = {
"writes the IPL section\n blocks 0-47",
"writes the lflash section\n blocks 64-end",
"writes IPL and lflash sections\n blocks 0-47 and 64-end",
"writes idstore section\n blocks 48-63",
"writes entire NAND\n blocks 0-end"
};
void writeSub(void) // raw write sub
{
	int i;
	maxrow = 4;
	for(i = 0; i<5; i++)
	{
		setc(4, ROW_OFFSET+i);
		if (i == row)
		{
			myScreenSetBackColor(C_SEL_BG);
			myScreenPutSChar(RO_LEFT);
			cprintf(C_SEL_FG,"%s", wsub[i]);
			myScreenPutSChar(RO_RIGHT);
			myScreenSetBackColor(C_BLACK);
			showInfo(NOT_SAFE, wsubInfo[i]);
		}
		else
			printf(" %s ", wsub[i]);
	}
	setc(0, maxrow+ROW_OFFSET+2);
	cprintf(C_PURPLE,"    press X to select or O to return\n\n");
	cprintf(C_LT_BLUE,"  %s\n", fnames[filesel].fname);
	printf("  selected\n");
	return;
}
// ****write menu end

char mmenu[8][20] = {"Load ELF", "Dump from NAND", "Write to NAND RAW", "Partition Tools", "Other Tools", "System Info", "Quit", "Shutdown"};
char mmenuInfo[8][94] = {
"Load a different program from ms0:/elf\n shows a listing of ELF files (not 4.xx compatible)",
"Dump NAND to dump directory\n will make an image of the NAND's data",
"Write NAND from file\n will attempt to directly image blocks onto the NAND",
"Tools to check and maintain partitions\n in the logical drive area",
"Other operations\n options that don't fit anywhere else",
"Will show information regarding hardware\n and program environment",
"Exit the program",
"Power off the PSP"
};
u8 mmenuRisk [8] = {0,SAFE,NOT_SAFE,NOT_SAFE,NOT_SAFE,0,0,0};
void mainMenu (void)
{
	int i;
	currfile = 0;
	maxrow = 6;
	for(i = 0; i<(maxrow+1); i++)
	{
		setc(4, ROW_OFFSET+i);
		if (i == row)
		{
			if((i==maxrow)&&shutDown) // to give the shutdown option
				i = maxrow+1;
			myScreenSetBackColor(C_SEL_BG);
			myScreenPutSChar(RO_LEFT);
			cprintf(C_SEL_FG,"%s", mmenu[i]);
			myScreenPutSChar(RO_RIGHT);
			myScreenSetBackColor(C_BLACK);
			showInfo(mmenuRisk[i],mmenuInfo[i]);
		}
		else
		{
			if((i==maxrow)&&shutDown) // to give the shutdown option
				i = maxrow+1;
			printf(" %s ", mmenu[i]);
		}
	}
	setc(0, maxrow+ROW_OFFSET+2);
	cprintf(C_PURPLE,"    press X to select\n");
	if(hard.usbEnable != NO_USB_CAP) // don't show this if the USB prx is not loaded through pspbtcnf and running/ready
	{
//		cprintf(C_PURPLE,"press SELECT to mount MS to USB %04x:%04x\n", hard.usbEnable, hard.usbStat);
		cprintf(C_PURPLE,"    press SELECT to mount MS to USB\n");
		if (hard.usbEnable == HOST_USB_CAP)
		{
			cprintf(C_PURPLE,"    press START to toggle usbhost (now %s)\n", (hard.usbStat & USB_EN_HOST?"on":"off"));
		}
	}
	return;
}

// handle keypresses
void doMenu(void)
{
	while(1)
	{
		unsigned int b;
		b = wait_press(PSP_CTRL_UP|PSP_CTRL_DOWN|PSP_CTRL_CROSS|PSP_CTRL_CIRCLE|PSP_CTRL_SELECT|PSP_CTRL_START);
		wait_release(PSP_CTRL_UP|PSP_CTRL_DOWN|PSP_CTRL_CROSS|PSP_CTRL_CIRCLE|PSP_CTRL_SELECT|PSP_CTRL_START);
		if (b & PSP_CTRL_UP)
		{
			if((row == 0) && (currfile > 0))
				currfile --;
			else
			{
				rowprev = row;
				if(row != 0)
					row--;
				else
				{
					row = maxrow;
					if(ttlfiles>(ROW_MAX-ROW_FILE_OFFSET))
						currfile = ttlfiles-(ROW_MAX-ROW_FILE_OFFSET)-1;
				}
			}
			showMenu(1);
		}
		else if (b & PSP_CTRL_DOWN)
		{
			if(row == (ROW_MAX-ROW_FILE_OFFSET)) // not going to get this high unless we are dealing with files/dirs
			{
				if(currfile != (ttlfiles-(ROW_MAX-ROW_FILE_OFFSET)-1))
					currfile++;
				else
				{
					row = 0;
					currfile = 0;
				}
			}
			else
			{
				rowprev = row;
				if(row != maxrow) row++;
				else row = 0;
			}
			showMenu(1);
		}
		else if (b & PSP_CTRL_CROSS) // figure out which menu option was selected and use it
		{
//			fade = 1;
			if(menu == MAIN_MENU)
			{
				if(row == ELF_LOAD)
				{
					updateFileStat(FIO_SO_IFREG, 2, ELF_PATH);
					menu = ELF_LOAD;
				}
				if(row == READ_MENU)	// dump NAND
				{
					makeDump();
					fade = 1;
					return;
				}
				else if (row == WRITE_MENU) // write NAND
				{
					updateFileStat(FIO_SO_IFREG, 1, DUMP_PATH); // looking for files that match in size
					menu = WRITE_MENU;
				}
				else if (row == LFLASH_MENU)
					menu = LFLASH_MENU;
				else if (row == WRITE_OTHER) // other operations
					menu = WRITE_OTHER;
				else if (row == SHOW_INFO)
				{
					showHardInfo();
					fade = 1;
					return;
				}
				else if (row == QUIT)
				{
					quitSel();
				}
				return;
			}
			else if(menu == WRITE_MENU)
			{
				filesel = row+currfile;
				if(ttlfiles > 0)
					menu = WRITE_SUB;
				else
					menu = MAIN_MENU;
				return;
			}
//			else if(menu == READ_MENU) // menu doesn't exist
//			{
//				filesel = row;
//				makeDump();
//				menu = MAIN_MENU;
//				return;
//			}
			else if(menu == WRITE_SUB)
			{
				writeDump(row);
//				menu = MAIN_MENU;
//				currfile = 0;
				return;
			}
			else if(menu == LFLASH_MENU)
			{
				if(row == 0) // checks partition records
					lcheck();
				if(row == 1) // logical format lflash
					lformat();
				else if(row == 2) //format partitions
					fparts();
				return;
			}
			else if(menu == WRITE_OTHER)
			{
				if (row == 0) // bad block check
					checkbb();
				else if(row == 1) //dump keys
					dkeys();
				else if(row == 2) //write keys
				{
					updateFileStat(FIO_SO_IFDIR, 0, DUMP_PATH); // looking for directories
					menu = WRITE_KEYS;
				}
//				menu = MAIN_MENU;
				return;
			}
			else if(menu == WRITE_KEYS)
			{
				filesel = row+currfile;
				wkeys();
				return;
			}
			else if(menu == ELF_LOAD)
			{
				if(ttlfiles != 0)
				{
					filesel = row+currfile;
					launchElf();
				}
				return;
			}
		}
		else if (b & PSP_CTRL_CIRCLE)
		{
			if (menu != MAIN_MENU) // in sub menus, pressing O will return to main menu
			{
				if (menu == WRITE_SUB) // go only 1 level up from the sub menu
					menu = WRITE_MENU;
				else
					menu = MAIN_MENU;
				currfile = 0;
				fade = 1;
				return;
			}
			else // in main menu, pressing O will select exit option, pressing it again will exit app
			{
				if (row == maxrow)
				{
					quitSel();
				}
				else
				{
//					setc(1, row + ROW_OFFSET);
//					printf("   ");
					row = maxrow;
					setc(0, ROW_OFFSET);
					showMenu(1);
//					setc(1, row + ROW_OFFSET);
//					printf("-->");
//					return;
				}
			}
		}
		else if ((b & PSP_CTRL_SELECT) && (menu == MAIN_MENU) && (hard.usbEnable!=NO_USB_CAP))
		{
			toggleUsb(MS_USB_MOUNT);
			return;
		}
		else if ((b & PSP_CTRL_START) && (menu == MAIN_MENU)&& (hard.usbEnable==HOST_USB_CAP))
		{
			toggleUsb(HOST_USB_MOUNT);
			return;
		}
	}
	return;
}

void showMenu(int refresh)
{
	if (refresh==0)
	{
		appHeader(0);
		rowprev = 0; row = 0;
		if((menu == WRITE_MENU)||(menu == WRITE_KEYS)||menu == ELF_LOAD)
			drawBox(23, 28, 467, 243, FBOX_COLOR);
		else if(menu == MAIN_MENU) // 133 wide 64 tall -- ym 170
			drawBox(23, 19, 240, 84,FBOX_COLOR);
		else if(menu == LFLASH_MENU)
			drawBox(23, 19, 240, 52,FBOX_COLOR);
		else if(menu == WRITE_OTHER)
			drawBox(23, 19, 240, 52,FBOX_COLOR);
		else if(menu == WRITE_SUB)
			drawBox(23, 19, 240, 68,FBOX_COLOR);

	}
	// show the proper menu
	setc(0, ROW_OFFSET);
	switch(menu)
	{
		case MAIN_MENU:
			mainMenu();
			break;
		case WRITE_OTHER:
			writeOther();
			break;
		case LFLASH_MENU:
			writeLflashMenu();
			break;
		case WRITE_SUB:
			writeSub();
			break;
		case WRITE_MENU:
		case WRITE_KEYS:
		case ELF_LOAD:
			displayFiles();
			break;
		default:
			break;
	}
/*
	if (menu == MAIN_MENU)
		mainMenu();
	else if(menu == WRITE_OTHER)
		writeOther();
	else if(menu == WRITE_SUB)
		writeSub();
	else if((menu == WRITE_MENU)||(menu == WRITE_KEYS)||menu == ELF_LOAD)
		displayFiles();
*/
	if(fade)
	{
		fadeIn();
		fade = 0;
	}
	else
		swap();
	return;
}

/*
// Example custom exception handler
void MyExceptionHandler(PspDebugRegBlock *regs)
{
	// Do normal initial dump, setup screen etc 
	pspDebugScreenInit();

	// I always felt BSODs were more interesting that white on black 
	pspDebugScreenSetBackColor(0x00FF0000);
	pspDebugScreenSetTextColor(0xFFFFFFFF);
	pspDebugScreenClear();

	pspDebugDumpException(regs);
}
*/
void verifyCreateDumpDir(void)
{
	int i;
	// create dump directory if it doesn't already exist
//	printf("checking dir: %s\n",hard.pathBase[DUMP_PATH]);
	i = sceIoDopen(hard.pathBase[DUMP_PATH]);
//	printf("i: %08x\n",i);
	if(i>=0)
	{
//		printf("exists\n");
		sceIoDclose(i);
	}
	else
	{
		i = sceIoMkdir(hard.pathBase[DUMP_PATH], 0777);
//		printf("mkdir i: %i %08x\n",i, i);
	}
//	swap();
//	while (confirm() != 1){;}
}

int main(int argc, char *argv[])
{
	int i;
//	pspDebugInstallErrorHandler(MyExceptionHandler);
	myScreenInit();
	myScreenSetBackColor(C_BLACK);
	myScreenSetTextColor(C_WHITE);
	myScreenClear();
	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_DIGITAL);
	row = 0;
	menu = MAIN_MENU;
	flashLocked = 0;
	shutDown = 0;
	otherFormat = 0;
	fade = 1;
	
	// load the kernel helper prx
	if(!fileExists("ntbridge.prx") && !fileExists("ms0:/elf/prx/ntbridge.prx"))
		errorExit("Could not find file ntbridge.prx!\n");
	else
	{
		printf("ntbridge.prx found\n");
		swap();
	}
	if(fileExists("ntbridge.prx"))
		i = LoadStartModule("ntbridge.prx");
	else
		i = LoadStartModule("ms0:/elf/prx/ntbridge.prx");

	if(i < 0)
	{
		if(i == -1)
			errorExit("Could not load file ntbridge.prx!\n code: %08#x", lsResult);
		if(i == -2)
			errorExit("Could not start file ntbridge.prx!\n code: %08#x", lsResult);
	}
	else
	{
		printf("ntbridge.prx started ok\n\n");
		swap();
	}
	sceKernelDelayThread(100000);
//	printf("argc: %i\n", argc);
//	printf("argv[0]: %s\n", argv[0]);
//	while (confirm() != 1){};
	// fill in the hard_info struct
	hard.ppb = ntNandGetPagesPerBlock();
	hard.ttlblock = ntNandGetTotalBlocks();
	hard.pagesz = ntNandGetPageSize();
	hard.filesize = (hard.ttlblock*hard.ppb*(hard.pagesz+SPARE_SIZE));
	hard.idsmag = ntIdsGetMagic();
	hard.nszMiB = (hard.ttlblock*hard.ppb*hard.pagesz)/1048576;
	hard.dkver = ntGetDKVer();
	getPspVer();

	// build the base name of the file and set the main dirs
	strcpy(hard.pathBase[DUMP_PATH], BASE_DUMP_PATH);
	strcpy(hard.pathBase[ELF_PATH], BASE_ELF_PATH);
	sprintf(hard.fnameBase,"%s_%08x_%iM", hard.mobo, hard.idsmag, hard.nszMiB);

	// determine if the modules are around to format flash partitions under des.cem
	if(fileExists("ms0:/elf/prx/nand_updater.prx") && fileExists("ms0:/elf/prx/lfatfs_updater.prx") && fileExists("ms0:/elf/prx/lflash_fatfmt_updater.prx"))
		otherFormat = 1;

	swap();
	setupUsb();
	
	showSplash();

	// show the openeing warning screen with the option to quit immediate
	if(argc == 0) // we were run as a resurrection.elf replacement
	{
		shutDown = 1;
		if(!fileExists("ms0:/kd/iaccept") && !fileExists("ms0:/elf/iaccept"))
			showWarn();
	}
	else if(!fileExists("iaccept")&& !fileExists("ms0:/kd/iaccept") && !fileExists("ms0:/elf/iaccept"))
		showWarn();
	
//	showWarn();

	// create dump directory if it doesn't already exist
//	i = sceIoDopen(hard.pathBase[DUMP_PATH]);
//	if(i>=0)
//		sceIoDclose(i);
//	else
//		sceIoMkdir(hard.pathBase[DUMP_PATH], 0777);
	verifyCreateDumpDir();

/*
while (confirm() != 1){};

myScreenClear();
showPercentBar(26, 2, FBOX_COLOR, C_RED, C_GREEN, C_BLACK);
drawTextBox(3, 4, 63, 26, FBOX_COLOR);
for(i=0; i<110; i++)
{
	printf("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
	updatePercent(i,100);
	swap();
}
fadeIn();
while (confirm() != 1){;}
fadeTo(C_BLACK, 31);
while (confirm() != 1){;}
fadeTo(C_WHITE, 31);
while (confirm() != 1){;}
*/
	swap();

	// go into a loop that will keep us in a menu
	while(1)
	{
		showMenu(0);
		doMenu();
	}

	exitApp(2*1000*1000);	
	return 0;   // never reaches here, again, just to suppress warning 
}
