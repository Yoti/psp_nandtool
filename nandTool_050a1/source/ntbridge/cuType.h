// exports from ntbridge
#ifndef CUTYPE_H
#define CUTYPE_H

/* USER_ECC_IN_SPARE means that the spare page buffer contains four bytes at the beginning, reserved for user page ECC.
 * NO_AUTO_USER_ECC disables automatic ECC calculations for user pages.
 * NO_AUTO_SPARE_ECC disables automatic ECC calculations for spare pages.*/
typedef enum {
    USER_ECC_IN_SPARE = 0x01,
    NO_AUTO_USER_ECC = 0x10,
    NO_AUTO_SPARE_ECC = 0x20
} SceNandEccMode_t;

#endif /* CUTYPE_H */
