/* spiflash.h
 * r0.2
 * Zhang Hao, stavrosatic@gmail.com
 * 2018.7.11
 */

#include "spiflash_port.h"

#ifndef _SPIFLASH_H
#define _SPIFLASH_H

typedef unsigned char (*spiflash_writebyte_func_t)(unsigned char);
typedef void (*spiflash_cs_func_t)(int);
typedef void (*spiflash_fastread_func_t)(unsigned char* buf, int nbytes);
typedef struct {
    spiflash_writebyte_func_t writebyte_f;
    spiflash_cs_func_t cs_f;
    spiflash_fastread_func_t fastread_f;
} spiflash_config_t;

unsigned long SPIFLASH_ReadJedecID(void);
void SPIFLASH_Init(spiflash_config_t* cfg);
unsigned char SPIFLASH_ReadStatus(void);
int SPIFLASH_Read(unsigned long addr, int nbytes, void* buf);
int SPIFLASH_FastRead(unsigned long addr, int num, void* buf);
int SPIFLASH_Write(unsigned long addr, int nbytes, const void* buf);
int SPIFLASH_Erase(unsigned long addr, int nbytes);
void SPIFLASH_SectorErase(unsigned long addr);
void SPIFLASH_ChipErase(void);

void SPIFLASH_EnterLowPowerMode(void);
void SPIFLASH_LeaveLowPowerMode(void);

#endif
