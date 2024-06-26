#include "main.h"
#include <malloc.h>

#define DUMP_BLOCKS 64

extern hard_info hard;

void readBlock(u32 page, u8 *buffer)
{
	u32 i, j;
	for (i = 0; i < hard.ppb; i++)
	{
		for (j = 0; j < 4; j++)
		{
			ntNandReadAccess(page, buffer, NULL, 1, (USER_ECC_IN_SPARE|NO_AUTO_USER_ECC|NO_AUTO_SPARE_ECC)); // read one page user data
			ntNandReadExtraOnly(page, buffer+hard.pagesz, 1);
		}
		page++;
		buffer += (hard.pagesz+SPARE_SIZE);
	}
}

void dumpNand(const char* dest)
{
	int bytes;
	u32 blocksz;
	u8* block;
	u32 i, j;
//printf("here\nppb: %d pgsz: %d ssz: %d db: %d\n",hard.ppb, hard.pagesz, SPARE_SIZE, DUMP_BLOCKS); swap();
	blocksz = (hard.ppb*(hard.pagesz+SPARE_SIZE))*DUMP_BLOCKS;
	block = (u8*)memalign(64, blocksz); // array to store 32 blocks
    if (block == NULL)
	{
		cprintf(C_RED, "\n\nInternal Memory Error\n");
		printf("Tried to alloc %d bytes\n", blocksz);
		cprintf(C_RED, "Press X to shutdown\n");
		swap();
		while(confirm() != 1){};
		errorExit("Unable to allocate memory!");
	}
//printf("alloc'd %ld bytes\n", blocksz); swap();
	
	SceUID fd = sceIoOpen(dest, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
	if (fd < 0)
	{
		cprintf(C_RED, "ERROR: ");
		printf("Unable to open file for writing!!");
		swap();
		return;
	}
	
	LockFlash(0);
	for (i = 0; i < (hard.ttlblock*hard.ppb);)
	{
		u8 *p;
		memset(block, 0xff, blocksz);
		showMsg("block: ","%04li of %i", (i/hard.ppb), hard.ttlblock);
		updatePercent((i/hard.ppb), hard.ttlblock);
		swap();

		p = block;

		for (j = 0; j < DUMP_BLOCKS; j++)
		{
			if (ntNandIsBadBlock(i) == 0)
			{
				readBlock(i, p);
			}
			else
			{
				cprintf(C_RED, "bad block: ");
				printf("page %d block %d\n", i, i/hard.ppb);
				swap();
			}

			i += hard.ppb;
			p += ((hard.pagesz+SPARE_SIZE)*hard.ppb);
		}

		bytes = sceIoWrite(fd, block, blocksz);
		if(bytes != blocksz)
		{
			sceIoClose(fd);
			errorExit("Write error! Your card may be full!");
		}
	}
	UnlockFlash();
	showMsg("block: ", "%04d of %d", hard.ttlblock , hard.ttlblock);
	updatePercent(1, 1);
	swap();

	sceIoClose(fd);
	free(block);
	return;
}
