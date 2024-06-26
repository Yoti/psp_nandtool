#include "main.h"
#include <malloc.h>

extern hard_info hard;

u8* user;
u8* spare;
u8* buf;

void errMsg(const char* msg, int block)
{
	cprintf(C_RED,"%s ", msg);
	printf("page %04d block %04d\n", block*hard.ppb, block);
	swap();
	return;
}

void showType(const char * msg)
{
	cprintf(C_PURPLE,"Writing %s...\n", msg);
	swap();
	return;
}

void writeBlock(int blocknum, int eraseonly)
{
	if(eraseonly)
	{
		if(ntNandEraseBlockWithRetry((blocknum*hard.ppb)) != 0)
			errMsg("Erase Fail:", blocknum);
		return;
	}
	if(ntNandWriteBlockWithVerify((blocknum*hard.ppb), user, spare) != 0)
		errMsg("Write Fail:", blocknum);
	return;
}

void writePages(int blocknum, int pages)
{
//	errMsg("Write Partial block:", blocknum); // debug
//	errMsg("Write Partial pages:", pages); // debug
	if(ntNandEraseBlockWithRetry((blocknum*hard.ppb)) != 0)
	{
		errMsg("Erase Fail:", blocknum);
		return;
	}
	if(ntNandWritePages((blocknum*hard.ppb), user, spare, pages) != 0)
		errMsg("PWrite Fail:", blocknum);
	return;
}

// returns number of highest page containing data, returns 0 if none contain data
int checkForData(int size)
{
	int i, j;
	int ret = 0;
	int cnt = 1;
	for(i=0; i<size;i += (hard.pagesz+SPARE_SIZE))
	{
		for(j=0;j<(hard.pagesz+SPARE_SIZE);j++)
		{
			if(buf[i+j] != 0xFF)
			{
				ret = cnt;
				break;
			}
		}
		cnt++;
	}
	return ret;
}

void writeBlockSet(int fp, int startblock, int endblock, int bbignore)
{
	int cnt, i, wpages;// d, s;
	int readsz = hard.ppb*(hard.pagesz+SPARE_SIZE);

	// seek to the required position in the file
	sceIoLseek(fp, (startblock*readsz), SEEK_SET);

	for(i=startblock; i<endblock; i++)
	{
		// update onscreen block info
//		setc(5,5);
//		printf("block %04d of %04d (cur: %04d)", (i-startblock+1),(endblock-startblock), i);
		showMsg("block: ", "%04i of %04i", (i-startblock+1), (endblock-startblock));
		updatePercent((i-startblock+1), (endblock-startblock));
		swap();

		// read in the block
        if (sceIoRead(fp, buf, readsz) != readsz)
        {
            UnlockFlash();
			sceIoClose(fp);
            errorExit("Error reading data from MS!");
        }

		// don't write bad blocks
		if(ntNandIsBadBlock((i*hard.ppb)) && (!bbignore))
		{
			errMsg("bad block:", i);
		}
		else
		{
			// check if there is actually data in the block
			wpages = checkForData(readsz);
			if(wpages != 0)
			{
				// parse data into spare/user array
			    for (cnt = 0; cnt < hard.ppb; cnt++)
			    {
					// separate spare and user data for WriteBlock
					memcpy((user+(512*cnt)), (buf+(cnt*528)), 512);
					memcpy((spare+(12*cnt)), (buf+516+(cnt*528)), 12);
					if(spare[(12*cnt)+1] != 0xFF)
					{
						errMsg("fixed bad block from data:", i);
						spare[(12*cnt)+1] = 0xFF;
					}
/*			        d = hard.pagesz*cnt;
					s = cnt*(hard.pagesz+SPARE_SIZE);
			        memcpy((u8*)(user+d), (u8*)(buf+s), hard.pagesz);
					d = SPARE_NOUECC_SIZE*cnt;
					s = hard.pagesz+(SPARE_SIZE-SPARE_NOUECC_SIZE)+(cnt*(hard.pagesz+SPARE_SIZE));
			        memcpy((u8*)(user+d), (u8*)(buf+s), SPARE_NOUECC_SIZE);*/
			    }
				// write the block to NAND
				if(wpages == hard.ppb)
					writeBlock(i, 0);
				else
					writePages(i, wpages);
			}
			else
				writeBlock(i, 1); // erase the block if there is no data in the dump
		}
	}
	return;
}

void writeToNand(int mode, int bbignore, const char* filename)
{
	SceUID fp;
	int spsz, ussz, blsz;
	char fnameSh[68]; // there are only 68 chars per line on the PSP screen

//	setc(0, 4);
	memset(&fnameSh, 0, sizeof(fnameSh));
	strncpy(fnameSh, filename, 61); // just in case file names are longer than the screen
	cprintf(C_LT_BLUE,"file: ");
	printf("%s\n\n", fnameSh);
	swap();

	// open the file for reading
	fp = sceIoOpen(filename, PSP_O_RDONLY, 0777);
	if (fp <= 0)
		errorExit("Error opening file for reading!");

	// allocate memory for blocks
	spsz = SPARE_NOUECC_SIZE*hard.ppb;
	ussz = hard.pagesz*hard.ppb;
	blsz = hard.ppb*(SPARE_SIZE+hard.pagesz);

	spare = (u8*)memalign(64,spsz);
	if(spare == NULL) errorExit("Error allocating buffers!");
	user = (u8*)memalign(64,ussz);
	if(user == NULL) errorExit("Error allocating buffers!");
	buf = (u8*)memalign(64,blsz);
	if(buf == NULL) errorExit("Error allocating buffers!");

	LockFlash(1);	

	// classify which set it is that is being written
	if((mode == MODE_IPL)|| (mode == MODE_XIDSTORE))// IPL is from block 4 to block 47
	{
			showType("IPL");
			writeBlockSet(fp, 4, 48, bbignore);
	}
	if((mode == MODE_LFLASH)|| (mode == MODE_XIDSTORE)) // Lflash is from block 64 to end of NAND
	{
			showType("LFlash");
			writeBlockSet(fp, 64, hard.ttlblock, bbignore);
	}
	if(mode == MODE_IDSTORE) // idstore is from block 48 to block 63
	{
			showType("IDStore");
			writeBlockSet(fp, 48, 64, bbignore);
	}
	if(mode == MODE_FULL) // from block 0 to end of NAND
	{
			showType("ALL");
			writeBlockSet(fp, 0, hard.ttlblock, bbignore);
	}

	UnlockFlash();

	free(user); user = NULL;
	free(spare); spare = NULL;
	free(buf); buf = NULL;
	sceIoClose(fp);
	return;
}
