   .set noreorder

#include "pspstub.s"

   STUB_START   "sceNand_driver",0x00010000,0x00010005
	STUB_FUNC	0x0BEE8F36,sceNandSetMagic
	STUB_END
