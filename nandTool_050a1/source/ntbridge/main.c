#include <pspsdk.h>
#include <pspkernel.h>
#include <string.h>
//#include <pspsysmem.h>
#include <pspsyscon.h>
#include <pspusb.h>
#include <pspusbstor.h>
//#include <psppower.h>
#include "nand_driver.h"
//#include "cuType.h"  // SceNandEccMode_t enum

// some name defines not in the sdk

#define sceSysregGetTachyonVersion sceSysreg_driver_E2A5D1EE
#define sceSysconGetBaryonVersion sceSyscon_driver_7EC5A957
#define sceSysregGetFuseId sceSysreg_driver_4F46EEDE
#define sceSysregGetFuseConfig sceSysreg_driver_8F4F4E96

//PSP_MODULE_INFO("nt_bridge", 0x1006, 1, 0);
PSP_MODULE_INFO("nt_bridge",PSP_MODULE_KERNEL, 1, 1);
PSP_MAIN_THREAD_ATTR(0);

// some prototypes not in the SDK

int sceSysregGetTachyonVersion(void);
int sceSysconGetBaryonVersion(u32* val);
u64 sceSysregGetFuseId(void);
u32 sceSysregGetFuseConfig(void);

int sceNandSetMagic(u32 magic);
int sceLflashFatfmtStartFatfmt(int argc, char *argv[]);

int (*UsbStart)(const char* driverName, int size, void *args);// 0xAE5DE6AF 
int (*UsbStop)(const char* driverName, int size, void *args);// 0xC2464FA0
int (*UsbActivate)(u32 pid);// 0x586DB82C
int (*UsbDeactivate)(u32 pid);// 0xC572A9C8
int (*UsbGetState)(void);// 0xC21645A4
int (*UsbstorBootSetCapacity)(u32 size);// 0xE58818A8	


int ntNandSetMagic(u32 magic)
{
	u32 k1;
	k1 = pspSdkSetK1(0);
//	int ret = 0;
	int ret = sceNandSetMagic(magic);
	pspSdkSetK1(k1);
	return ret;
}

u32 ntIdsGetMagic(void)
{
	u32 k1;
	u32 magic;
	k1 = pspSdkSetK1(0);
	u32 buf[4] = {0x00000000,0x00000000,0x00000000,0x00000000};
	u32 sha[5] = {0x00000000,0x00000000,0x00000000,0x00000000,0x00000000};
 
	buf[0] = *(vu32*)(0xBC100090);
	buf[1] = *(vu32*)(0xBC100094);
	buf[2] = *(vu32*)(0xBC100090)<<1;
	buf[3] = 0xD41D8CD9;
 
	sceKernelUtilsSha1Digest((u8*)buf, sizeof(buf), (u8*)sha);
	magic = (sha[0] ^ sha[3]) + sha[2];

	pspSdkSetK1(k1);
	return magic;
}

int ntLflashFatfmtStartFatfmt(int argc, char *argv[])
{
	u32 k1;
	k1 = pspSdkSetK1(0);
//	int ret = 0; 
	int ret = sceLflashFatfmtStartFatfmt(argc, argv);
	pspSdkSetK1(k1);
	return ret;
}

int ntNandLock(int writeFlag)
{
	u32 k1;
	k1 = pspSdkSetK1(0);
//	int ret = 0; 
	int ret = sceNandLock(writeFlag);
	pspSdkSetK1(k1);
	return ret;
}

void ntNandUnlock(void)
{
	u32 k1;
	k1 = pspSdkSetK1(0);
	sceNandUnlock();
	pspSdkSetK1(k1);
	return;
}

int ntNandWritePages(u32 ppn, u8 *user, u8 *spare, u32 len)
{
	u32 k1;
	k1 = pspSdkSetK1(0);
//	int ret = 0; 
	int ret = sceNandWritePages(ppn, user, spare, len);
	pspSdkSetK1(k1);
	return ret;
}

int ntNandReadAccess(u32 ppn, u8 *user, u8 *spare, u32 len, SceNandEccMode_t mode)
{
	u32 k1;
	k1 = pspSdkSetK1(0);
//	int ret = 0; 
	int ret = sceNandReadAccess(ppn, user, spare, len, mode);
	pspSdkSetK1(k1);
	return ret;
}

int ntNandReadExtraOnly(u32 ppn, u8 *spare, u32 len)
{
	u32 k1;
	k1 = pspSdkSetK1(0);
//	int ret = 0; 
	int ret = sceNandReadExtraOnly(ppn, spare, len);
	pspSdkSetK1(k1);
	return ret;
}

int ntNandCalcEcc(u8 *buf)
{
	u32 k1;
	k1 = pspSdkSetK1(0);
//	int ret = 0; 
	int ret = sceNandCalcEcc(buf);
	pspSdkSetK1(k1);
	return ret;
}

int ntNandGetPageSize(void)
{
	u32 k1;
	k1 = pspSdkSetK1(0);
//	int ret = 0; 
	int ret = sceNandGetPageSize();
	pspSdkSetK1(k1);
	return ret;
}

int ntNandGetPagesPerBlock(void)
{
	u32 k1;
	k1 = pspSdkSetK1(0);
//	int ret = 0; 
	int ret = sceNandGetPagesPerBlock();
	pspSdkSetK1(k1);
	return ret;
}

int ntNandGetTotalBlocks(void)
{
	u32 k1;
	k1 = pspSdkSetK1(0);
//	int ret = 0; 
	int ret = sceNandGetTotalBlocks();
	pspSdkSetK1(k1);
	return ret;
}

int ntNandWriteBlockWithVerify(u32 ppn, u8 *user, u8 *spare)
{
	u32 k1;
	k1 = pspSdkSetK1(0);
//	int ret = 0; 
	int ret = sceNandWriteBlockWithVerify(ppn, user, spare);
	pspSdkSetK1(k1);
	return ret;
}

int ntNandEraseBlockWithRetry(u32 ppn)
{
	u32 k1;
	k1 = pspSdkSetK1(0);
//	int ret = 0; 
	int ret = sceNandEraseBlockWithRetry(ppn);
	pspSdkSetK1(k1);
	return ret;
}

int ntNandIsBadBlock(u32 ppn)
{
	u32 k1;
	k1 = pspSdkSetK1(0);
//	int ret = 0; 
	int ret = sceNandIsBadBlock(ppn);
	pspSdkSetK1(k1);
	return ret;
}

// sees if the module is loaded, returns 1 if found
int ntKernelFindModuleByName(const char *modname)
{
	u32 k1;
	int ret = 0;
	k1 = pspSdkSetK1(0);
	SceModule *mod = sceKernelFindModuleByName(modname);
	if (mod != NULL)
		ret = 1;
	pspSdkSetK1(k1);
	return ret;
}

u32 FindProc(const char* szMod, const char* szLib, u32 nid)
{
	int count, total, i = 0;
	struct SceLibraryEntryTable *entry;
	SceModule *pMod;
	void *entTab;
	unsigned int *vars;
	u32 ret = 0;
	u32 k1;
	k1 = pspSdkSetK1(0);

	pMod = sceKernelFindModuleByName(szMod);
	if (pMod == NULL)
		ret = 0; // module not found
	else
	{
		entTab = pMod->ent_top;
		while(i < (pMod->ent_size))
	    {
			entry = (struct SceLibraryEntryTable *) (entTab + i);

	        if(entry->libname && (strcmp(entry->libname, szLib) == 0))
			{
				total = entry->stubcount + entry->vstubcount;
				vars = entry->entrytable;
				if(entry->stubcount > 0)
				{
					for(count = 0; count < entry->stubcount; count++)
					{
						if (vars[count] == nid)
						{
							ret = vars[count+total];
							goto RET_FUNC;
						}
					}
				}
			}
			i += (entry->len * 4);
		}
	}
RET_FUNC:
	pspSdkSetK1(k1);
	return ret; // function not found
}

int ntSysregGetTachyonVersion(void)
{
	u32 k1;
	k1 = pspSdkSetK1(0);
//	int ret = 0; 
	int ret = sceSysregGetTachyonVersion();
	pspSdkSetK1(k1);
	return ret;
}

u64 ntSysregGetFuseId(void)
{
	u32 k1;
	k1 = pspSdkSetK1(0);
//	u64 ret = 0; 
	u64 ret = sceSysregGetFuseId();
	pspSdkSetK1(k1);
	return ret;
}

u32 ntSysregGetFuseConfig(void)
{
	u32 k1;
	k1 = pspSdkSetK1(0);
//	u32 ret = 0; 
	u32 ret = sceSysregGetFuseConfig();
	pspSdkSetK1(k1);
	return ret;
}

int ntSysconGetBaryonVersion(u32* val)
{
	u32 k1;
	k1 = pspSdkSetK1(0);
//	int ret = 0; 
	int ret = sceSysconGetBaryonVersion(val);
	pspSdkSetK1(k1);
	return ret;
}

void ntSysconPowerStandby(void)
{
	u32 k1;
	k1 = pspSdkSetK1(0);
	sceSysconPowerStandby();
	pspSdkSetK1(k1);
	return;
}

int ntUsbStart(const char* driverName, int size, void *args)// 0xAE5DE6AF 
{
	u32 k1;
	k1 = pspSdkSetK1(0);
	int ret = UsbStart(driverName, size, args);
	pspSdkSetK1(k1);
	return ret;
}

int ntUsbStop(const char* driverName, int size, void *args)// 0xC2464FA0
{
	u32 k1;
	k1 = pspSdkSetK1(0);
	int ret = UsbStop(driverName, size, args);
	pspSdkSetK1(k1);
	return ret;
}

int ntUsbActivate(u32 pid)// 0x586DB82C
{
	u32 k1;
	k1 = pspSdkSetK1(0);
	int ret = UsbActivate(pid);
	pspSdkSetK1(k1);
	return ret;
}

int ntUsbDeactivate(u32 pid)// 0xC572A9C8
{
	u32 k1;
	k1 = pspSdkSetK1(0);
	int ret = UsbDeactivate(pid);
	pspSdkSetK1(k1);
	return ret;
}

int ntUsbGetState(void)// 0xC21645A4
{
	u32 k1;
	k1 = pspSdkSetK1(0);
	int ret = UsbGetState();
	pspSdkSetK1(k1);
	return ret;
}

int ntUsbstorBootSetCapacity(u32 size)// 0xE58818A8	
{
	u32 k1;
	k1 = pspSdkSetK1(0);
	int ret = UsbstorBootSetCapacity(size);
	pspSdkSetK1(k1);
	return ret;
}

void ntResolveUSB(void)
{
	u32 k1;
	k1 = pspSdkSetK1(0);
	UsbStart = (void *)FindProc("sceUSB_Driver", "sceUsb", 0xAE5DE6AF);
	UsbStop = (void *)FindProc("sceUSB_Driver", "sceUsb", 0xC2464FA0);
	UsbActivate = (void *)FindProc("sceUSB_Driver", "sceUsb", 0x586DB82C);
	UsbDeactivate = (void *)FindProc("sceUSB_Driver", "sceUsb", 0xC572A9C8);
	UsbGetState = (void *)FindProc("sceUSB_Driver", "sceUsb", 0xC21645A4);
	UsbstorBootSetCapacity = (void *)FindProc("sceUSB_Stor_Boot_Driver", "sceUsbstorBoot", 0xE58818A8);
	pspSdkSetK1(k1);
	return;
}

int ntGetDKVer(void) // requires an array of 3 u32s
{
	u32 k1;
	k1 = pspSdkSetK1(0);
	u32 ret = sceKernelDevkitVersion();
	pspSdkSetK1(k1);
	return ret;
}	

int module_start(SceSize args, void *argp)
{
	return 0;
}

int module_stop()
{
	return 0;
}
