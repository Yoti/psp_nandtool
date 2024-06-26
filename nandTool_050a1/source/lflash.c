#include <malloc.h>
#include "main.h"
#include "pbr.h"

extern hard_info hard;

// idstore/ipl take up first 0x40/64 blocks (1MiB)
// 32: has 0x800 blocks, 0x7C0 which can be used for lflash - uses actual 0x780
// 64: has 0x1000 blocks, 0xFCO which can be used for lflash - uses actual 0xF00
// spare EC EC EC 00 00 ff lbh lbl  00 00 00 00 EC EC ff ff
void rawLflashFmt (void)
{
	int i, userbufsz, spbufsz, ecc, ret;
	int ttl=0; // compiler warning
	int ins = -1;
	int count = 0; // every 480 blocks we need to skip 16-baddies
	int bbcount = 0;
	u8* userbuf;
	u8* spbuf;
	int curb = 0x40; // set to start of lflash area
	u8 spare[12] = {0x00,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff};
	u16 logical = 0;
	u32 ppn;
	
	if(hard.ttlblock == 2048) // 32M NAND PSP
		ttl = 0x780; // 1920; 2048-64-1920= 64 blank
	else if (hard.ttlblock == 4096) // 64M NAND PSP
		ttl = 0xF00; // 3840; 4096-64-3840= 192 blank
	else // just in case someone wants to be a wise guy and use a bigger NAND than 64M
		errorExit("This function does not support this NAND!");
		
	// allocate some memory
	userbufsz = hard.ppb*hard.pagesz; // enough for 1 block
	spbufsz = hard.ppb*SPARE_NOUECC_SIZE;
	userbuf = (u8*)malloc(userbufsz);
	spbuf = (u8*)malloc(spbufsz);
	if((userbuf == NULL) || (spbuf == NULL))
		errorExit("unable to allocate memory!");

	//loop to handle blocks, ttl is the number of logical blocks we need, logical is the one we are at currently
	LockFlash(1);
//	ntNandDoMarkAsBadBlock(10240); // for debugging
	cprintf(C_LT_BLUE, "Writing generic partition records...\n");
	while (logical < ttl)
	{
		showMsg("block: ", "%04i of %04i", logical+1, ttl);
		updatePercent(logical+1, ttl);
		swap();
		// insert lba into spare
		spare[2] = (u8)((0xFF00&logical)>>8);
		spare[3] = (u8)(0xFF&logical);

		// clear userdata
		memset(userbuf, 0x00, userbufsz);
//		memset(spbuf, 0xFF, spbufsz);

		// get the spare ecc for this and insert it
		ecc = (ntNandCalcEcc(spare)|0xF000); // get the spare ecc and set the nibble
		spare[8] = (u8)(0xFF&ecc);
		spare[9] = (u8)((0xFF00&ecc)>>8);

		// copy spare across the entire spare buffer
		for(i=0;i<hard.ppb;i++)
		{
			memcpy((u8*)(spbuf+(i*SPARE_NOUECC_SIZE)), (u8*)spare, SPARE_NOUECC_SIZE);
		}
		ins = -1;
		// see if it is one of the specific pages, if so insert partition info
		if(hard.ttlblock == 2048) // fat
		{
			if(logical == 0x0000)
				ins=0;
			else if (logical == 0x0002)
				ins=1;
			else if (logical == 0x0602)
				ins=2;
			else if (logical == 0x0702)
				ins=3;
			else if (logical == 0x0742)
				ins=4;
			else if (logical == 0x077E)
				ins=5;

			if(ins >= 0)
			{
				memcpy((u8*)(userbuf+446),(u8*)(phat_data+(ins*32)), 32);
				memcpy((u8*)(userbuf+510),(u8*)(ident),2);
			}
		}
		else if (hard.ttlblock == 4096) // slim
		{
			if (logical == 0x0000)
				ins=0;
			else if (logical == 0x0002)
				ins=1;
			else if (logical == 0x0A42)
				ins=2;
			else if (logical == 0x0B82)
				ins=3;
			else if (logical == 0x0C82)
				ins=4;
			else if (logical == 0x0ECA)
				ins=5;
			else if (logical == 0x0EFE)
				ins=6;

			if(ins >= 0)
			{
				memcpy((u8*)(userbuf+446),(u8*)(slim_data+(ins*32)), 32);
				memcpy((u8*)(userbuf+510),(u8*)ident,2);
			}
		}

		// write the block with auto userECC, if it fails move to the next block and try again
		ret = 1; // set this so it does the while loop at least once
		while(ret != 0)
		{
			ppn = curb*hard.ppb;
			// don't try to write to bad blocks
			if(ntNandIsBadBlock(ppn))
			{
				cprintf(C_RED, "bad block skipped: ");
				printf("page %d block %d\n", (curb*hard.ppb), curb);
				bbcount++;
				ret = 1;
				swap();
			}
			else if ((count >= 480) && (bbcount <= 16)) // we need some free blocks every 480 or so else lflash format corrupts stuff
			{
				ntNandEraseBlockWithRetry(ppn);
				bbcount++;
			}
			else
			{
				ret = ntNandWriteBlockWithVerify(ppn, userbuf, spbuf);
				if (ret != 0)
				{
					cprintf(C_RED, "block write: ");
					printf("error at block %d\n", curb);
					swap();
				}
				else
					count++;
			}
			if((bbcount >= 16) || (count >= 480))
			{
				bbcount = 0;
				count = 0;
			}
			curb++;
			if(curb == hard.ttlblock) errorExit("Error: ran out of NAND for lflash!");
		}
		logical++;
	}

	// erase whatever is left of the nand
	cprintf(C_LT_BLUE, "erasing remainder of NAND...\n");
	swap();
	ttl = hard.ttlblock - curb;
	logical = 1;
	while(curb < hard.ttlblock)
	{
		showMsg("block: ", "%04i of %04i", logical, ttl);
		swap();
		if(!ntNandIsBadBlock((curb*hard.ppb)))
			ntNandEraseBlockWithRetry(curb*32);
		else
		{
			cprintf(C_RED, "bad block skipped: ");
			printf("page %d block %d\n", (curb*hard.ppb), curb);
			swap();
			ret = 1;
		}
		curb++; logical++;
	}
	UnlockFlash();

	free(userbuf);
	free(spbuf);
	return;
}

void formatPartitions(void)
{
	char *argv[2];
	int x;
	u32* value = 0; // for temporal patch of set magic function to allow non scrambled formatting for fat PSP/cfw
	u32 store[2] = {0,0}; // place to store original instructions to restore function when done
	printf("Format partitions...\n");
	cprintf(C_LT_BLUE,"starting modules...");
	swap();
	if (pspSdkLoadStartModule("ms0:/elf/prx/nand_updater.prx", PSP_MEMORY_PARTITION_KERNEL) < 0)
		errorExit("\nCould not load nand_updater.prx.\n");
	if (pspSdkLoadStartModule("ms0:/elf/prx/lfatfs_updater.prx", PSP_MEMORY_PARTITION_KERNEL) < 0)
		errorExit("\nCould not load lfatfs_updater.prx.\n");
	if (pspSdkLoadStartModule("ms0:/elf/prx/lflash_fatfmt_updater.prx", PSP_MEMORY_PARTITION_KERNEL) < 0)
		errorExit("\nCould not load lflash_fatfmt_updater.prx.\n");

	cprintf(C_GREEN,"done.\n\n");
	updatePercent(1, 10);
	swap();
	
	if(!hard.isSlim) // if we are on a fat PSP
	{
		cprintf(C_BLUE,"FAT PSP detected, disabling lflash scramble\n");
		value = (u32*)FindProc("sceNAND_Updater_Driver", "sceNand_updater_driver", 0x0BEE8F36);
		store[0] = value[0];
		store[1] = value[1];
		value[0] = 0x03e00008; // branch return
		value[1] = 0x00000000; // nop
	}
	else
		cprintf(C_BLUE,"Slim PSP detected, allowing lflash scramble\n");
	swap();

	cprintf(C_LT_BLUE,"Logically formatting partitions..\n");
	swap();
	if (sceIoUnassign("flash0:") < 0)
		errorExit("Cannot unassign flash0!");
	if (sceIoUnassign("flash1:") < 0)
		errorExit("Cannot unassign flash1!");
	updatePercent(2, 10);
	swap();

	argv[0] = "fatfmt";
	argv[1] = "lflash0:0,0";
	printf("formatting flash0...");
	swap();
	if ((x = ntLflashFatfmtStartFatfmt(2, argv)) < 0)
		cprintf(C_RED,"\nError formating flash0 :%08X.\n", x);
	else
		cprintf(C_GREEN,"done!\n");
	updatePercent(3, 10);
	swap();

	argv[1] = "lflash0:0,1";
	printf("formatting flash1...");
	swap();
	if ((x = ntLflashFatfmtStartFatfmt(2, argv)) < 0)
		cprintf(C_RED,"\nError formating flash1 :%08X.\n", x);
	else
		cprintf(C_GREEN,"done!\n");
	updatePercent(4, 10);
	swap();

	argv[1] = "lflash0:0,2";
	printf("formatting flash2...");
	swap();
	if ((x = ntLflashFatfmtStartFatfmt(2, argv)) < 0)
		cprintf(C_RED,"\nError formating flash2 :%08X.\n", x);
	else
		cprintf(C_GREEN,"done!\n");
	updatePercent(5, 10);
	swap();

	argv[1] = "lflash0:0,3";
	printf("formatting flash3...");
	swap();
	if ((x = ntLflashFatfmtStartFatfmt(2, argv)) < 0)
		cprintf(C_RED,"\nError formating flash3 :%08X.\n", x);
	else
		cprintf(C_GREEN,"done!\n");
	updatePercent(6, 10);
	swap();

	cprintf(C_LT_BLUE,"\nre-assigning devices...\n");
	swap();

	if (sceIoAssign("flash0:", "lflash0:0,0", "flashfat0:", IOASSIGN_RDWR, NULL, 0) < 0)
		cprintf(C_RED,"\nError re-assigning flash0.\n");
	else
		cprintf(C_GREEN,"flash0 assigned OK.\n");
	updatePercent(7, 10);
	swap();

	if (sceIoAssign("flash1:", "lflash0:0,1", "flashfat1:", IOASSIGN_RDWR, NULL, 0) < 0)
		cprintf(C_RED,"\nError re-assigning flash1.\n");
	else
		cprintf(C_GREEN,"flash1 assigned OK.\n");
	updatePercent(8, 10);
	swap();

	if (sceIoAssign("flash2:", "lflash0:0,2", "flashfat2:", IOASSIGN_RDWR, NULL, 0) < 0)
		cprintf(C_RED,"\nError re-assigning flash2.\n");
	else
		cprintf(C_GREEN,"flash2 assigned OK.\n");
	updatePercent(9, 10);
	swap();

	if (sceIoAssign("flash3:", "lflash0:0,3", "flashfat3:", IOASSIGN_RDWR, NULL, 0) < 0)
		cprintf(C_RED,"\nError re-assigning flash3.\n");
	else
		cprintf(C_GREEN,"flash3 assigned OK.\n");
	updatePercent(10, 10);
	swap();
	
	if(!hard.isSlim)
	{
		value[0] = store[0];
		value[1] = store[1];
		cprintf(C_BLUE,"lflash scramble re-enabled.\n");
	}
	cprintf(C_GREEN,"\ncompleted! Shutting down now...");
	swap();
	return;

}

// reads the idstore raw NAND data into a buffer
void startLba(int* lbabuf)
{
	u32 i, j, page;
	u8 sp[16];
	u16 lbav;
	for (i = 0; i < (hard.ttlblock-64); i++)
	{
		page = (i+64)*hard.ppb;
		if (ntNandIsBadBlock(page) == 0)
		{
			for (j = 0; j < 4; j++)
			{
				ntNandReadExtraOnly(page, (u8*)sp, 1); // we only need the first spare of each block to find the index with
			}
			lbav = (sp[6]<<8) + sp[7];
			if(lbav != 0xFFFF)
			{
//			printf("lbabuf: %i\n", lbabuf[lbav]);
				if(lbabuf[lbav] == -1)// catch dupe lbas
					lbabuf[lbav] = (i+64);
				else
				{
					lbabuf[lbav] = -2;
					cprintf(C_RED, "dupe lba at block: %d  LBA val: 0x%04x : %d\n", i+64, lbav, lbav);
					swap();
				}
			}
		}
		else
		{
			cprintf(C_RED, "bad block: %d\n", i+64);
			swap();
		}
	}
}

void readPageB(u8* dest, int blocknum) // reads the first page of a block
{
	u32 page;
	int i;
	page = blocknum*hard.ppb;
//printf("reading first page %d of block %i\n", page, blocknum+64);
	for(i=0; i<4; i++)
	{
		ntNandReadAccess(page, dest, NULL, 1, (USER_ECC_IN_SPARE|NO_AUTO_USER_ECC|NO_AUTO_SPARE_ECC)); // read one page user data
	}
	return;
}

typedef struct {
	u8 status; //  0x80 = bootable, 0x00 = non-bootable, other = malformed
	u8 sChs[3]; // Cylinder-head-sector address of the first sector in the partition
	u8 type; // 
	u8 eChs[3]; // Cylinder-head-sector address of the last sector in the partition
	u32 lba; // Logical block address of the first sector in the partition
	u32 len;// Length of the partition, in sectors
} __attribute__ ((packed)) PartEnt_T; // total 16 bytes

typedef struct {
	u8 code[440];
	u8 opt_sig[4];
	u8 unk1[2]; // 446 unused bytes
	PartEnt_T ent[4]; //4*16 byte entries
	u8 sig[2];
} __attribute__ ((packed)) BootRec_T; // total 512 bytes

static int verifySig(BootRec_T* record)
{
//	printf("sig 0: %02x sig 1: %02x\n", 
	if(record->sig[0]== 0x55 && record->sig[1] == 0xAA)
		return 1;
	else
		return 0;
}
/*
static void showStatus(u8 status)
{
	printf("  status         : 0x%02x ",status);
	switch(status)
	{
		case 0x00:
			printf("(non-bootable)\n");
			break;
		case 0x80:
			printf("(bootable)\n");
			break;
		default:
			printf("(error - unknown)\n");
			break;
	}
	swap();
	return;
}
*/
static void showPartType(u8 type)
{
//	printf("  type           : 0x%02x ",type);
	switch(type)
	{
		case 0x01:
			printf("DOS FAT12  ");
			break;
		case 0x05:
			printf("DOS 3.3 Ext");
			break;
		case 0x0e:
			printf("DOS FAT16  ");
			break;
		case 0x0f:
			printf("W95 Ext    ");
			break;
		default:
			printf("unknown    ");
			break;
	}
	swap();
	return;
}

int showRecord(BootRec_T* record)
{
	int i, ret = 0;
	if(verifySig(record))
	{
		for(i = 0; i<4; i++)
		{
			if(record->ent[i].type != 0)
			{
				cprintf(C_GREY,"Entry %i: ", i);
//				showStatus(record->ent[i].status);
				showPartType(record->ent[i].type);
//				printf("  lba first block: %d\n",(record->ent[i].lba)/32); swap();
//				printf("  length sectors : %d\n",record->ent[i].len); swap();
				printf(" %d KiB (%d MiB)\n",((record->ent[i].len)*512)/1024, ((record->ent[i].len)*512)/(1024*1024)); swap();
				if((record->ent[i].type == 0x05)||(record->ent[i].type == 0x0f))
					ret = record->ent[i].lba;
			}
/*			else
			{
				printf("Entry %i *empty*\n", i);
				swap();
			}
*/
//			printf("  first cyl: 0x%02x head: 0x%02x sector: 0x%02x\n",record->ent[i].sChs[0]&0xFF,record->ent[i].sChs[1]&0xFF,record->ent[i].sChs[2]&0xFF);
//			printf("  last  cyl: 0x%02x head: 0x%02x sector: 0x%02x\n",record->ent[i].eChs[0]&0xFF,record->ent[i].eChs[1]&0xFF,record->ent[i].eChs[2]&0xFF);
		}
		return ret;
	}
	else
	{
		printf("Signature not found\n");
		swap();
		return 0;
	}
	return 0;
}

int checkPart(int* lbabuf)
{
	int currlba = 0, currpart = 0, extv, baselba;
	u8 page[512]; // cache for page user data
	BootRec_T* record = (BootRec_T*)page;
	readPageB(page,lbabuf[currlba]); // read the MBR
	cprintf(C_LT_BLUE,"\n**Master Boot Record**\n");
	swap();
	extv = showRecord(record);
	baselba = extv/32;
	currlba = baselba;
	while(extv != 0)
	{
		cprintf(C_LT_BLUE,"\n--*Partition %d*--\n", currpart);
//		printf("Looking for extended partition chain at LBA %d\n", currlba);
//		swap();
		readPageB(page,lbabuf[currlba]); // read first partition entry
		extv = showRecord(record);
		currlba = baselba+(extv/32);
		currpart++;
	}
	return currpart;
}

// returns negative on error, else number of keys dumped - less than 120 keys there are definitely problems
int lfsCheck(void)
{
	int* lbabuf; // enough bytes to store the block numbers of each LBA
	int ttl = 0, err = 0, lbf = 0, i, parts, endmsg = 0;
	if(!hard.isSlim) // 32M NAND PSP
		ttl = 0x780;
	else // 64M NAND PSP
		ttl = 0xF00;

	lbabuf = (int*)memalign(64,(hard.ttlblock-64));
	//memset((int*)lbabuf, -1, (hard.ttlblock-64));
	for(i = 0; i < (hard.ttlblock-64); i++)
	{
		lbabuf[i] = -1;
	}
	printf("Caching LBA up to %i blocks...\n", (hard.ttlblock-64));
	swap();

	LockFlash(0);
	startLba(lbabuf);
	UnlockFlash();

	printf("done caching!\n");
	printf("checking for all LBAs from 0 to %i\n", ttl);
	swap();
	for(i=0; i < ttl; i++)
	{
		if(lbabuf[i] == -1)
		{
			printf("missing block at LBA %i\n", i);
			printf("  lba: %d val: 0x%08x : %d\n", i, lbabuf[i], lbabuf[i]);
			err++;
			swap();
		}
		else if (lbabuf[i] == -2)
		{
			printf("duplicated block at LBA %i\n", i);
			printf("  lba: %d val: 0x%08x : %d\n", i, lbabuf[i], lbabuf[i]);
			err++;
			swap();
		}
		else lbf++;
	}
	if(err != 0)
	{
		cprintf(C_RED,"\n %i incorrect LBAs found\n", err);
		cprintf(C_BLUE, "Press X to continue\n");
		while(!confirm()){};
		swap();
	}
	else
	{
		printf("\n-- %i of %i LBAs found --\n", lbf, ttl);
		swap();
	}

	printf("Attempting to verify partition records\n");
	swap();
	LockFlash(0);
	parts = checkPart(lbabuf);
	UnlockFlash();
	
	if(hard.ttlblock == 4096) // slim needs currpart to be 5
	{
		if((parts != 5) || (err != 0))
			endmsg = 1;
	}
	else if(hard.ttlblock == 2048) // fat needs currpart to be 4
	{
		if((parts != 4) || (err != 0))
			endmsg = 1;
	}
	if(endmsg)
		cprintf(C_RED,"\n\nRepartition NAND, something seems wrong.\n\n");
	else
		cprintf(C_GREEN,"\n\nPartition records seem OK\n\n");

	free(lbabuf);
	return err;
}
