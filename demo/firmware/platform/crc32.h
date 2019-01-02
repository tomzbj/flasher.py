#ifndef _CRC32_H
#define _CRC32_H

void CRC32_Init(void);
unsigned long CRC32_Calc(unsigned long initial, const void* msg, int size);

#endif
