#ifndef _SPIFLASH_CONF_H
#define _SPIFLASH_CONF_H

void SPI1_SetCS(int cs);
unsigned char SPI1_WriteByte(unsigned char data);
void SPI1_Read_DMA(unsigned char* buf, int num);
void SPIFLASH_Config(void);

#endif
