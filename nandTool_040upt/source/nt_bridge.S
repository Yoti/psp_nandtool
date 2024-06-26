	.set noreorder

#include "pspstub.s"

	STUB_START	"nt_bridge",0x40090000,0x001E0005
	STUB_FUNC	0x7F69BA57,ntNandSetMagic
	STUB_FUNC	0x11E75935,ntIdsGetMagic
	STUB_FUNC	0x1C03D098,ntLflashFatfmtStartFatfmt
	STUB_FUNC	0x81D16A3A,ntNandLock
	STUB_FUNC	0xD36B79A6,ntNandUnlock
	STUB_FUNC	0x55ACDF5E,ntNandWritePages
	STUB_FUNC	0x9BC80D4F,ntNandReadAccess
	STUB_FUNC	0x469DBEA8,ntNandReadExtraOnly
	STUB_FUNC	0x0A10B8B7,ntNandCalcEcc
	STUB_FUNC	0x3D96114D,ntNandGetPageSize
	STUB_FUNC	0x8A81BE9A,ntNandGetPagesPerBlock
	STUB_FUNC	0x25B76B30,ntNandGetTotalBlocks
	STUB_FUNC	0x1DDCC20C,ntNandWriteBlockWithVerify
	STUB_FUNC	0x5CFFAAFA,ntNandEraseBlockWithRetry
	STUB_FUNC	0xCF64066F,ntNandIsBadBlock
	STUB_FUNC	0x710C02D4,ntKernelFindModuleByName
	STUB_FUNC	0x9947E189,FindProc
	STUB_FUNC	0xCF8A5B97,ntSysregGetTachyonVersion
	STUB_FUNC	0x5DA4433B,ntSysregGetFuseId
	STUB_FUNC	0x62509FDA,ntSysregGetFuseConfig
	STUB_FUNC	0x898C837C,ntSysconGetBaryonVersion
	STUB_FUNC	0x2C51CFF7,ntSysconPowerStandby
	STUB_FUNC	0xCAFE835F,ntUsbStart
	STUB_FUNC	0x82B883D6,ntUsbStop
	STUB_FUNC	0x5DCC3398,ntUsbActivate
	STUB_FUNC	0xD1999C4A,ntUsbDeactivate
	STUB_FUNC	0x507060C4,ntUsbGetState
	STUB_FUNC	0x41458E04,ntUsbstorBootSetCapacity
	STUB_FUNC	0xBF6015C1,ntResolveUSB
	STUB_FUNC	0x99A26FBD,ntGetDKVer
	STUB_END
