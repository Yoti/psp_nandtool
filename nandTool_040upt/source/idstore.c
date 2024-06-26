#include "main.h"
#include <malloc.h>
#include <pspidstorage.h>

int confirm(void); // returns 1 when pressing x, 0 when pressing O

extern hard_info hard;
u8* idsbuf; //262144 bytes
u8* idsspare;
u16* idsdex;

void readIds(u32 block, u8*buffer, u8* spare)
{
	u32 i, j;
	u32 page = (block+48)*hard.ppb;
	for (i = 0; i < hard.ppb; i++)
	{
		for (j = 0; j < 4; j++)
		{
			ntNandReadAccess(page, buffer, NULL, 1, (USER_ECC_IN_SPARE|NO_AUTO_USER_ECC|NO_AUTO_SPARE_ECC)); // read one page user data
			if(i==0)
				ntNandReadExtraOnly(page, spare, 1); // we only need the first spare of each block
		}
		page++;
		buffer += (512);
	}
}

void startIds(void)
{
	int s;
	u8 sp[16];
	idsbuf = (u8*)memalign(64, 262144);
	if(idsbuf == NULL) errorExit("Error allocating buffers!");

	if(hard.moboType > 7)
	{
		ntNandSetMagic(hard.idsmag);
	}
	LockFlash(0);
	// find index via spare page and read entire ids into the buffer
	for(s=0; s<16; s++)
	{
		readIds(s, idsbuf+(16384*s), (u8*)sp);
		if((sp[6] == 0x73)&&(sp[7] == 0x01)&&(sp[8] == 0x01)&&(sp[9] == 0x01))
		{
			idsdex = (u16*)(idsbuf+(16384*s));
		}
	}
	UnlockFlash();
	if(hard.moboType > 7)
		ntNandSetMagic(0);
	return;
}

void endIds(void)
{
	free(idsbuf); idsbuf = NULL;
	idsdex = NULL;
	return;
}

int ReadKey(u16 key)
{
	int i;
	// find key in index
	for(i=0; i<512; i++)
	{
		if(idsdex[i] == key)
			return i;
	}

	return -1;
}

void dumpKeys(const char* fname)
{
	char filepath[256], folder[256];
	int f, s, currkey, bytes, d, i;
	idsdex = NULL;
	
	for(i=0; i<1000; i++) // get a unique folder name using the base name
	{
		if(i != 0)
			sprintf(folder,"%s_%03d", fname, i); 
		else
			strncpy(folder, fname, 256);
		d = sceIoDopen(folder);
		if (d < 0) // directory does not exist
		{
			sceIoMkdir(folder, 0777);
			i = 1000;
		}
		else
		{
			sceIoDclose(d);
			printf("%s\n", folder);
			cprintf(C_LT_BLUE, "  exists, incrementing name\n\n");
		}
	}
	
	cprintf(C_LT_BLUE,"Dumping to folder:\n");
	printf("%s\n", folder);
	startIds();
	swap();
/*
	sprintf(filepath,"%s/idsdec.bin", folder);
	f = sceIoOpen(filepath, PSP_O_WRONLY | PSP_O_CREAT, 0777); // debug, dumps decrypted idstore
	if (f > 0)
	{
		printf("Saving decrypted data to file /idsdec.bin...");
		sceIoWrite(f, idsbuf, 262144);
		sceIoClose(f);
		printf(" done.\n");
	}
*/
	if(idsdex != NULL)
	{
		printf("index read OK\n");
		swap();

		sprintf(filepath,"%s/index.bin", folder);
		f = sceIoOpen(filepath, PSP_O_WRONLY | PSP_O_CREAT, 0777);
		if (f > 0)
		{
			printf("Saving index to /index.bin...");
			swap();
			bytes = sceIoWrite(f, (u8*)idsdex, 1024);
			sceIoClose(f);
			if(bytes != 1024)
				errorExit("\nWrite error! Your card may be full!\n");
			printf(" done.\n");
			swap();
		}
		for (currkey=0; currkey<0x0200; currkey++) // debug
		{
			s = ReadKey(currkey);
			updatePercent(currkey, 0x200);
			swap();
			if (s >= 0)
			{
				sprintf(filepath, "%s/0x%04X.bin", folder, currkey);
				f = sceIoOpen(filepath, PSP_O_WRONLY | PSP_O_CREAT, 0777);
				if (f > 0)
				{
					printf("\nSaving to file /0x%04X.bin...", currkey);
					swap();
					bytes = sceIoWrite(f, idsbuf+(s*512), 512);
					sceIoClose(f);
					if(bytes != 512)
						errorExit("\nWrite error! Your memory card may be full!\n");
					printf("done.");
					swap();
				}
			}
		}
		updatePercent(0x200, 0x200);
		cprintf(C_LT_BLUE, "\n\nCompleted, keys dumped to:\n");
		printf("%s", folder);
		swap();
	}
	else
	{	
		printf("\n\n\n\nERROR: idstore index not found\n");
		swap();
	}
	endIds();
	return;
}

void eMsg(const char* msg, int block)
{
	cprintf(C_RED,"\n%s ", msg);
	printf("page %04d block %04d", (block+48)*hard.ppb, block);
	swap();
}

void setIndexBlock(int block, u16 val)
{
	int i, bk;
	for(i=0; i<hard.ppb; i++)
	{
		bk = i+(block*hard.ppb);
		idsdex[bk] = val;
	}
	return;
}

void moveIndexBlock(int block)
{
	int i;
	int src = block*hard.ppb;
	int dest = (block+1)*hard.ppb;
	for(i=0; i<32; i++)
	{
		idsdex[i+dest] = idsdex[i+src];
	}
	return;
}

void writeKeys(int bbignore, const char* path)
{
	char filepath[256];
	u8 spare[12] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xF0, 0xFF, 0xFF}; // normal idstore block spare page
	u8 indSpare[12] = {0xFF, 0xFF, 0x73, 0x01, 0x01, 0x01, 0xFF, 0xFF, 0x86, 0xF1, 0xFF, 0xFF}; // index spare page
	u16 currkey;
	int f, bytes, i, j = 0, block = 0;
	u32 ppn;

	idsbuf = (u8*)memalign(64, 16384); // big enough buffer for 1 block
	if(idsbuf == NULL)
		errorExit("Error allocating buffers!");
	idsspare = (u8*)memalign(64, 384); // big enough buffer for 1 block of spares
	if(idsspare == NULL)
		errorExit("Error allocating buffers!");
	idsdex = (u16*)memalign(64, 1024); // big enough buffer for 1 block
	if(idsbuf == NULL)
		errorExit("Error allocating buffers!");
	memset(idsbuf, 0x00, 16384);
	memset(idsspare, 0xFF, 384);
	memset((u8*)idsdex, 0xFF, 1024);

	// end idstore just in case
//	cprintf(C_LT_BLUE,"Flushing idstore...");
//	sceIdStorageEnd();
//	sceKernelDelayThread(1*1000*1000);
//	cprintf(C_GREEN,"done!\n"); 
	cprintf(C_LT_BLUE,"Writing idstore from:\n");
	printf("%s\n",path);
	swap();
	LockFlash(1);

	// clear whatever data is in idstore area already
	cprintf(C_LT_BLUE,"Formatting idstore...\n");
	swap();
	for(i=0; i<16; i++) // find out if there are any existing bad blocks and clear/erase the ones that aren't bad
	{
		//if not already a bad block, erase it
		ppn = (i+48)*hard.ppb;
		if(ntNandIsBadBlock(ppn) && (!bbignore))
		{
			eMsg("Bad Block:", i);
			setIndexBlock(i, 0xFFF0);
		}
		else 
		{
			if(ntNandEraseBlockWithRetry(ppn) != 0)
			{	
				eMsg("Erase Fail:", i);
				setIndexBlock(i, 0xFFF0);
			}
		}
	}
	if(hard.moboType > 7)
	{
		ntNandSetMagic(hard.idsmag);
	}
	cprintf(C_GREEN,"done!\n");
	swap();
	
	// read in keys, write out each block as they are full and add them to the index for the correct block
	for (currkey=0; currkey<0x0200; currkey++) // 0x200 should be enough to hit all known keys
	{
		sprintf(filepath, "%s/0x%04X.bin", path, currkey);
		f = sceIoOpen(filepath, PSP_O_RDONLY, 0777);
		if (f > 0)
		{
			cprintf(C_LT_BLUE,"\nreading key ");
			printf("0x%04x", currkey);
			swap();
			bytes = sceIoRead(f, (idsbuf+(j*512)), 512);
			sceIoClose(f);
			if(bytes != 512)
				errorExit("\nRead error!\n");
			idsdex[(block*32)+j] = currkey;
			j++;
		}
		if(j == 32)
		{
			memset(idsspare, 0xFF, 384);
			for(i=0; i<32; i++)
				memcpy((idsspare+(i*12)), (u8*)spare, 12);
			cprintf(C_LT_BLUE,"\nWriting block %d", block);
			while(ntNandWriteBlockWithVerify(((block+48)*hard.ppb), idsbuf, idsspare) != 0)
			{
				eMsg("Write fail, retrying", block);
				moveIndexBlock(block);
				setIndexBlock(block, 0xFFF0);
				block++;
				if(block == 16)
					errorExit("\nran out of blocks!");
				else
					cprintf(C_LT_BLUE,"\nRetry write on block %d", block);
			}
			block++;
			if(block == 16)
				errorExit("\nran out of blocks!");
			j = 0;
			memset(idsbuf, 0x00, 16384);
		}
	}
	// catch the last few keys
	if(j != 0)
	{
		memset(idsspare, 0xFF, 384);
		for(i=0; i<32; i++)
			memcpy((idsspare+(i*12)), (u8*)spare, 12);
//		while(ntNandWriteBlockWithVerify(((block+48)*hard.ppb), idsbuf, idsspare) != 0)
		while(ntNandWritePages(((block+48)*hard.ppb), idsbuf, idsspare, j) != 0)
		{
			eMsg("Write fail, retrying", block);
			moveIndexBlock(block);
			setIndexBlock(block, 0xFFF0);
			block++;
			if(block == 16)
				errorExit("\nran out of blocks!");
		}
		block++;
		if(block == 16)
			errorExit("\nran out of blocks!");
	}

	// write out index, make sure to mark index in the index
	cprintf(C_LT_BLUE,"\nwriting index to block %d", block);
//	printf("0x%04x", currkey);
	swap();
	memset(idsspare, 0xFF, 384);
	setIndexBlock(block, 0xFFF5);
	for(i=0; i<2; i++)
		memcpy((idsspare+(i*12)), (u8*)indSpare, 12);
	while(ntNandWritePages(((block+48)*hard.ppb), (u8*)idsdex, idsspare, 2) != 0)
	{
		eMsg("Write fail, retrying", block);
		moveIndexBlock(block);
		setIndexBlock(block, 0xFFF0);
		block++;
		if(block == 16)
			errorExit("\nran out of blocks!");
		else
			cprintf(C_LT_BLUE,"\nRetry write index to block %d", block);
	}
	
	UnlockFlash();
	ntNandSetMagic(0);
//	cprintf(C_GREEN,"done!");
//	swap();

/*
		sprintf(filepath,"ms0:/index.bin");
		f = sceIoOpen(filepath, PSP_O_WRONLY | PSP_O_CREAT, 0777); // debug, dumps decrypted idstore
		if (f > 0)
		{
			printf("Saving index to file ms0:/index.bin...");
			swap();
			sceIoWrite(f, (u8*)idsdex, 1024);
			sceIoClose(f);
			printf(" done.\n");
			swap();
		}
*/
	free(idsbuf); idsbuf = NULL;
	free(idsspare); idsspare = NULL;
	free(idsdex); idsdex = NULL;
	return;
}	
