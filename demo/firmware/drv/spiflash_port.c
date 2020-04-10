#include <stdlib.h>
#include "flasher.h"
#include "misc.h"
#include "spiflash.h"
#include "usart_f0.h"
#include "crc32.h"

void SPI1_SetCS(int cs)
{
    if(cs)
        GPIOA->BSRR = GPIO_Pin_4;
    else
        GPIOA->BRR = GPIO_Pin_4;
//    _delay_us(3);
}

unsigned char SPI1_WriteByte(unsigned char data)
{
    ( {  while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);});
    SPI_SendData8(SPI1, data);
    ( {  while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);});
    return SPI_ReceiveData8(SPI1);
}

void uputs(const void* data, int size)
{
    USART_WriteData(USART2, data, size);
}

#define BUFFER_SIZE 256
static struct {
    unsigned char txbuf[BUFFER_SIZE];
    unsigned char rxbuf[BUFFER_SIZE];
} g;

void SPIDMA_Config(void)
{
    DMA_InitTypeDef DMA_InitStructure;

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    DMA_DeInit(DMA1_Channel2);  // SPI1 rx
    DMA_DeInit(DMA1_Channel3);  // SPI1 tx

    // 默认; 外设基址0, 内存基址0, 方向为外设源, 缓存大小0, 缓存自增disable
    // 内存自增disable, 外设数据大小byte, 内存数据大小byte, 模式normal
    // 优先级low, 内存到内存disable
    DMA_StructInit(&DMA_InitStructure);

    DMA_InitStructure.DMA_PeripheralBaseAddr = (unsigned long)&(SPI1->DR);
    DMA_InitStructure.DMA_MemoryBaseAddr = (unsigned long)(g.rxbuf);
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
    DMA_InitStructure.DMA_BufferSize = BUFFER_SIZE;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel2, &DMA_InitStructure);
    DMA_Cmd(DMA1_Channel2, ENABLE);
    DMA_InitStructure.DMA_MemoryBaseAddr = (unsigned long)(g.txbuf);
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
    DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
    DMA_Init(DMA1_Channel3, &DMA_InitStructure);
    DMA_Cmd(DMA1_Channel3, ENABLE);
}

void SPI1_Read_DMA(unsigned char* buf, int num)
{
    unsigned char dummy = 0xff;
    DMA_InitTypeDef DMA_InitStructure;

    DMA_DeInit(DMA1_Channel2);  // SPI1 rx
    DMA_DeInit(DMA1_Channel3);  // SPI1 tx

    DMA_StructInit(&DMA_InitStructure);
    DMA_InitStructure.DMA_PeripheralBaseAddr = (unsigned long)&(SPI1->DR);
    DMA_InitStructure.DMA_MemoryBaseAddr = (unsigned long)(buf);
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
    DMA_InitStructure.DMA_BufferSize = num;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel2, &DMA_InitStructure);
    DMA_Cmd(DMA1_Channel2, ENABLE);
    DMA_InitStructure.DMA_MemoryBaseAddr = (unsigned long)(&dummy);
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
    DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
    DMA_Init(DMA1_Channel3, &DMA_InitStructure);
    DMA_Cmd(DMA1_Channel3, ENABLE);
    SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx | SPI_I2S_DMAReq_Rx, ENABLE);
    while(DMA_GetFlagStatus(DMA1_FLAG_TC2) == RESET);
}

void SPIFLASH_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    SPI_InitTypeDef SPI_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
    // PA5~7 for SPIFLASH
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // PA4 as CS for SPIFLASH
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    SPI1_SetCS(1);

    GPIO_PinAFConfig(GPIOA, GPIO_PinSource5, GPIO_AF_0);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource6, GPIO_AF_0);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource7, GPIO_AF_0);

    SPI_StructInit(&SPI_InitStructure);
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;
    SPI_Init(SPI1, &SPI_InitStructure);
    SPI_RxFIFOThresholdConfig(SPI1, SPI_RxFIFOThreshold_QF);
    SPI_Cmd(SPI1, ENABLE);

    static spiflash_config_t scfg;
    scfg.cs_f = SPI1_SetCS;
    scfg.writebyte_f = SPI1_WriteByte;
    scfg.fastread_f = SPI1_Read_DMA;
    SPIFLASH_Init(&scfg);
    SPIFLASH_LeaveLowPowerMode();

    CRC32_Init();
    static fl_cfg_t icfg;
    icfg.read_f = SPIFLASH_FastRead;
    icfg.write_f = SPIFLASH_Write;
    icfg.erase_f = SPIFLASH_Erase;
    icfg.readinfo_f = SPIFLASH_ReadJedecID;
    icfg.uwrite_f = uputs;
//    icfg.crc32_f = NULL;
    icfg.crc32_f = CRC32_Calc;
    fl_init(&icfg);
}
