#ifndef OTHER_H
#define OTHER_H

int checkBlock (void); // checks for bad blocks and reports them to debug printf
void rawLflashFmt(void); // initializes lflash area of NAND to defaults for 32/64M PSPs
void formatPartitions(void);

#endif //  OTHER_H 
