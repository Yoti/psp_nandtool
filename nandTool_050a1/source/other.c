#include "main.h"

extern hard_info hard;

int checkBlock (void)
{
	int i, k=0;
	int j = hard.ttlblock/10;
	int badf = 0;
	LockFlash(0);
	for(i=0; i<hard.ttlblock; i++)
	{
		showMsg("block: ", "%04d of %d", i+1, hard.ttlblock);
		if(k == j)
		{
			updatePercent(i+1, hard.ttlblock);
			swap();
			k=0;
		}
		if(ntNandIsBadBlock((i*hard.ppb)))
		{
			cprintf(C_RED, "bad block: ");
			printf("page %d block %d\n", (i*hard.ppb), i);
			badf++;
		}
		k++;
	}
	updatePercent(i+1, hard.ttlblock);
	swap();
	UnlockFlash();
	return badf;
}
