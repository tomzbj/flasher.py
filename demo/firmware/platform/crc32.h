#ifndef _CRC32_H
#define _CRC32_H

void CRC32_Init(void);
void CRC32_Calc(const unsigned char* msg, int size);
unsigned long CRC32(unsigned long crc, unsigned char* msg, int size);

#endif
