// exports from ntbridge
#ifndef NTBRIDGE_H
#define NTBRIDGE_H

#include "cuType.h"

int ntNandSetMagic(u32 magic);
u32 ntIdsGetMagic(void);
int ntLflashFatfmtStartFatfmt(int argc, char *argv[]);
int ntNandLock(int writeFlag);
void ntNandUnlock(void);
int ntNandWritePages(u32 ppn, u8 *user, u8 *spare, u32 len);
int ntNandReadAccess(u32 ppn, u8 *user, u8 *spare, u32 len, SceNandEccMode_t mode);
int ntNandReadExtraOnly(u32 ppn, u8 *spare, u32 len);
int ntNandCalcEcc(u8 *buf);
int ntNandGetPageSize(void);
int ntNandGetPagesPerBlock(void);
int ntNandGetTotalBlocks(void);
int ntNandWriteBlockWithVerify(u32 ppn, u8 *user, u8 *spare);
int ntNandEraseBlockWithRetry(u32 ppn);
int ntNandIsBadBlock(u32 ppn);
int ntKernelFindModuleByName(const char *modname);
u32 FindProc(const char* szMod, const char* szLib, u32 nid);
int ntSysregGetTachyonVersion(void);
u64 ntSysregGetFuseId(void);
u32 ntSysregGetFuseConfig(void);
int ntSysconGetBaryonVersion(u32* val);
void ntSysconPowerStandby(void);
int ntUsbStart(const char* driverName, int size, void *args);
int ntUsbStop(const char* driverName, int size, void *args);
int ntUsbActivate(u32 pid);
int ntUsbDeactivate(u32 pid);
int ntUsbGetState(void);
int ntUsbstorBootSetCapacity(u32 size);	
int ntResolveUSB(void);
int ntGetDKVer(void);

#endif /* NTBRIDGE_H */
