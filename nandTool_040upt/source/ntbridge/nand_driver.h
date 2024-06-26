/*
 * PSP Software Development Kit - http://www.pspdev.org
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in PSPSDK root for details.
 *
 * pspnand_driver.h - Definitions and interfaces to the NAND (flash) driver.
 *
 * Copyright (c) 2005 Marcus R. Brown <mrbrown@0xd6.org>
  */

#ifndef PSPNAND_DRIVER_H
#define PSPNAND_DRIVER_H

#include <pspkerneltypes.h>
#include "cuType.h"  // SceNandEccMode_t enum

#define sceNandReadPagesRawAll sceNand_driver_C478C1DE //sceNand_c478c1de
#define sceNandWritePagesRawAll sceNand_driver_BADD5D46 //sceNand_badd5d46
#define sceNandDoMarkAsBadBlock sceNand_driver_C29DA136
//#define sceNandEraseBlock sceNand_EB0A0022

/* USER_ECC_IN_SPARE means that the spare page buffer contains four bytes at the beginning, reserved for user page ECC.
 * NO_AUTO_USER_ECC disables automatic ECC calculations for user pages.
 * NO_AUTO_SPARE_ECC disables automatic ECC calculations for spare pages.
typedef enum {
    USER_ECC_IN_SPARE = 0x01,
    NO_AUTO_USER_ECC = 0x10,
    NO_AUTO_SPARE_ECC = 0x20
} SceNandEccMode_t;
*/
/* Erases the block, retrying up to four times in case of failure
 * ppn = physical page number*/
int sceNandEraseBlockWithRetry(u32 ppn);

/* block / 32pages =* page=512 bytes, spare = 12 bytes,
 * Erases the block, writes (with automatic ECC) and verifies it, retrying up to four times in case of failure
 * ppn = physical page number
 * user = buffer for user block
 * spare = buffer for spare block (12 bytes per page, no user ECC provided)*/
int sceNandWriteBlockWithVerify(u32 ppn, u8 *user, u8 *spare);

/* Reads the block (with automatic ECC), retrying up to four times in case of failure
 * ppn = physical page number
 * user = buffer for user block
 * spare = buffer for spare block (12 bytes, no user ECC stored)*/
int sceNandReadBlockWithRetry(u32 ppn, u8 *user, u8 *spare);

/* Reads user and spare pages from nand with automatic ECC
 * ppn = physical page number
 * user = buffer for user pages or NULL
 * spare = buffer for spare pages (12 bytes, no user ECC stored)
 * len = number of pages to read, max 32*/
int sceNandReadPages(u32 ppn, u8 *user, u8 *spare, u32 len);

/* Writes user and spare pages to nand with automatic ECC
 * ppn = physical page number
 * user = buffer for user pages or NULL
 * spare = buffer for spare pages or NULL (12 bytes, no user ECC provided)
 * len = number of pages to write, max 32*/
int sceNandWritePages(u32 ppn, u8 *user, u8 *spare, u32 len);

/* Reads user and spare pages from nand without automatic ECC
 * ppn = physical page number
 * user = buffer for user pages
 * spare = buffer for spare pages (16 bytes, user ECC stored)
 * len = number of pages to read, max 32*/
int sceNandReadPagesRawAll(u32 ppn, u8 *user, u8 *spare, u32 len);

/* Writes user and spare pages to nand without automatic ECC
 * ppn = physical page number
 * user = buffer for user pages
 * spare = buffer for spare pages (16 bytes, user ECC stored)
 * len = number of pages to write, max 32*/
int sceNandWritePagesRawAll(u32 ppn, u8 *user, u8 *spare, u32 len);

/* Erases the block, retrying up to four times in case of failure
 * ppn = physical page number*/
int sceNandEraseBlockWithRetry(u32 ppn);

/* Returns the total number of blocks of the nand */
int sceNandGetTotalBlocks(void);

/* Returns the number of pages per block of the nand */
int sceNandGetPagesPerBlock(void);

/* Returns the page size of the nand */
int sceNandGetPageSize(void);

/* gives exclusive access to NAND, writeFlag true = write access*/
int sceNandLock(int writeFlag);

/* releases exclusive access to NAND*/
void sceNandUnlock(void);

/* Enables/disables write protection for the nand
 * protect = TRUE, write protect nand
 * protect = FALSE, enable writing to nand*/
//int sceNandProtect(bool_t protect);
int sceNandSetWriteProtect(int protectFlag);

int sceNandReset(int flag);

int sceNandInit(void);

/* Issues an ID command to the nand
 * id = buffer to hold id codes
 * len = size of id buffer*/
int sceNandReadId(u8 *id, s32 len);

/* Reads user and spare pages from nand with automatic ECC
 * ppn = physical page number
 * user = buffer for user pages or NULL
 * spare = buffer for spare pages (12 bytes, no user ECC stored)
 * len = number of pages to read, max 32*/
int sceNandReadPages(u32 ppn, u8 *user, u8 *spare, u32 len);

/* Reads spare pages without auto ECC from nand through serial interface
 * ppn = physical page number
 * spare = buffer for spare pages (16 bytes per page, user ECC stored)
 * len = number of spare pages to read, max 32.*/
int sceNandReadExtraOnly(u32 ppn, u8 *spare, u32 len);

/* Read physical pages from nand
 * ppn = physical page number
 * user = buffer for user pages or NULL
 * spare = buffer for spare pages or NULL
 * len = number of pages to read, max. 32
 * mode = how ECC is to be handled and the size of the spare page buffer*/
int sceNandReadAccess(u32 ppn, u8 *user, u8 *spare, u32 len, SceNandEccMode_t mode);

/* Write physical pages to nand
 * ppn = physical page number
 * user = buffer for user pages or NULL
 * spare = buffer for spare pages or NULL
 * len = number of pages to write, max 32
 * mode = how ECC is to be handled and the size of the spare page buffer*/
int sceNandWriteAccess(u32 ppn, u8 *user, u8 *spare, u32 len, SceNandEccMode_t mode);

/* Checks if the block is bad (check the block_status byte in the spare area)
 * ppn = physical page number*/
int sceNandIsBadBlock(u32 ppn);

/* marks the block as bad (block_status byte in the spare area)
 * ppn = physical page number*/
int sceNandDoMarkAsBadBlock(u32 ppn);

/* Calculates and returns a 12-bit ECC value suitable for the spare area
 * buf = eight bytes of data on which to calculate*/
int sceNandCalcEcc(u8 *buf);

#endif /* PSPNAND_DRIVER_H */
