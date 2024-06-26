#ifndef LFLASH_H
#define LFLASH_H

void rawLflashFmt(void); // initializes lflash area of NAND to defaults for 32/64M PSPs
void formatPartitions(void);
int lfsCheck(void);

#endif //  LFLASH_H 
