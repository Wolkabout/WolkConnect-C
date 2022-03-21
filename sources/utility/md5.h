/*********************************************************************
 * Filename:   md5.h
 * Author:     Brad Conte (brad AT bradconte.com)
 * Copyright:
 * Disclaimer: This code is presented "as is" without any guarantees.
 * Details:    Defines the API for the corresponding MD5 implementation.
 *********************************************************************/

#ifndef WOLKCONNECTOR_C_MD5_H
#define WOLKCONNECTOR_C_MD5_H

/*************************** HEADER FILES ***************************/
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/****************************** MACROS ******************************/
#define MD5_BLOCK_SIZE 32   // MD5 outputs a 32 byte digest
typedef unsigned char BYTE; // 8-bit byte
typedef unsigned int WORD;  // 32-bit word, change to "long" for 16-bit machines

/**************************** DATA TYPES ****************************/

typedef struct {
    BYTE data[64];
    WORD datalen;
    unsigned long long bitlen;
    WORD state[4];
} MD5_CTX;

/*********************** FUNCTION DECLARATIONS **********************/
void md5_init(MD5_CTX* ctx);
void md5_update(MD5_CTX* ctx, const BYTE data[], size_t len);
void md5_final(MD5_CTX* ctx, BYTE hash[]);

#endif // WOLKCONNECTOR_C_MD5_H
