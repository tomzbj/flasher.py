/// @file usart.c
/// @brief 串口通讯程序.
/// @author Zhang Hao, stavrosatic@gmail.com
/// @version R0.1
/// @date 2017-10-13
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "usart_f0.h"

#include "flasher.h"
#include "misc.h"
#include "stm32f0xx.h"

#define MAX_LEN 1152
unsigned char usart2_rx_buf[MAX_LEN];
//unsigned char usart3_rx_buf[MAX_LEN];
unsigned char* g_usartx_rx_buf;

enum {
    FRAME_TYPE_NONE, FRAME_TYPE_CLI, FRAME_TYPE_MODBUS_ASCII,
    FRAME_TYPE_MODBUS_RTU, FRAME_TYPE_FLASHER
};

struct {
    char msg[MAX_LEN];
    USART_TypeDef* USARTx;
    int size;
    unsigned char type;
} g_frame;

/// @brief 向指定USART端口输出的printf.
/// @param USARTn 指定的USART端口.
/// @param fmt 输出格式.
/// @param ... 要输出的数据.
/// @retval None
void uprintf(USART_TypeDef* USARTn, const char* __restrict fmt, ...)
{
    char buf[256];
    va_list arp;
    va_start(arp, fmt);
    vsprintf(buf, fmt, arp);
    USART_WriteData(USARTn, (unsigned char*)buf, strlen(buf));
    va_end(arp);
}

/// @brief 配置USART参数. 
/// @param None
/// @retval None
void USART_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    DMA_InitTypeDef DMA_InitStructure;
    USART_InitTypeDef USART_InitStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA | RCC_AHBPeriph_GPIOB, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    // PA2 as USART2_TX, PA3 as USART2_RX, PB10 as USART3_TX, PB11 as USART3_RX
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_1);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_1);

    // nvic init
    NVIC_InitStructure.NVIC_IRQChannelPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;   //USARTn_IRQChannel;
    NVIC_Init(&NVIC_InitStructure);

    // DMA init
    DMA_DeInit(DMA1_Channel5);  // DMA1_Channel5用于USART2_RX
    DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)(&USART2->RDR);
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)usart2_rx_buf;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;  // 单向，外设源
    DMA_InitStructure.DMA_BufferSize = MAX_LEN;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;  // 禁止外设递增
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable; // 允许内存递增  
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel5, &DMA_InitStructure);
    DMA_Cmd(DMA1_Channel5, ENABLE);

    // usart init
    USART_DeInit(USART2);
    // 默认: 波特率9600, 字宽8位, 停止位1, 极性no, 模式tx/rx, 流控none
    USART_StructInit(&USART_InitStructure);
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl =
        USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_InitStructure.USART_BaudRate = 1000000UL;
    USART_Init(USART2, &USART_InitStructure);
    USART_ITConfig(USART2, USART_IT_IDLE, ENABLE);
    USART_ITConfig(USART2, USART_IT_ORE, ENABLE);
    USART_DMACmd(USART2, USART_DMAReq_Rx, ENABLE);  // 采用 DMA 方式接收
    USART_Cmd(USART2, ENABLE);
}

void USART_ChangeBaudRate(USART_TypeDef* USARTx, unsigned long baudrate)
{
    USART_InitTypeDef USART_InitStructure;

    USART_StructInit(&USART_InitStructure);
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl =
        USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_InitStructure.USART_BaudRate = baudrate;
    USART_Cmd(USARTx, DISABLE);
    USART_Init(USARTx, &USART_InitStructure);
    USART_Cmd(USARTx, ENABLE);
}

/// @brief 向USART写数据.
/// @param USARTx 要写数据的USART端口.
/// @param data 要写入的数据缓冲区指针.
/// @param num 要写入的字节数.
/// @retval None
void USART_WriteData(USART_TypeDef* USARTx, const void* data, int num)
{
    while(num--) {
        ( {  while(USART_GetFlagStatus(USARTx, USART_FLAG_TXE) == RESET);});
        USART_SendData(USARTx, *((unsigned char *)data));
        data++;
    }
    ( {  while(USART_GetFlagStatus(USARTx, USART_FLAG_TC) == RESET);});
}

void USART_Poll(void)
{
    switch(g_frame.type) {
        case FRAME_TYPE_NONE:
            return;
        case FRAME_TYPE_FLASHER:
            fl_parse(g_frame.msg, g_frame.size);
            break;
        default:
            break;
    }
    g_frame.type = FRAME_TYPE_NONE;
}

/// @brief 向USART写一个字节数据. 
/// @param USARTx 要写数据的USART端口.
/// @param byte 要写的字节.
/// @retval None
void USART_WriteByte(USART_TypeDef* USARTx, unsigned char byte)
{
    ( {  while(USART_GetFlagStatus(USARTx, USART_FLAG_TXE) == RESET);});
    USART_SendData(USARTx, byte);
    ( {  while(USART_GetFlagStatus(USARTx, USART_FLAG_TC) == RESET);});
}

/// @brief USART空闲中断处理程序, 用于接收数据. 
/// @param USARTx 要处理的USART端口.
/// @retval None
void USART_RX_IDLE_IRQHandler(USART_TypeDef* USARTx)
{
    typeof(DMA1_Channel5) dma_channel = NULL;
    unsigned short size = size;
    const char* usartx_rx_buf = NULL;
    if(USARTx == USART2) {
        dma_channel = DMA1_Channel5;
        usartx_rx_buf = (const char*)usart2_rx_buf;
    }
    DMA_Cmd(dma_channel, DISABLE);
    size = USARTx->ISR;
    size = USARTx->RDR;
    size = MAX_LEN - DMA_GetCurrDataCounter(dma_channel);
    DMA_SetCurrDataCounter(dma_channel, MAX_LEN);

    if(g_frame.type == FRAME_TYPE_NONE) {   // 判断上一帧是否已处理完, 未处理完则直接忽略
        memcpy((char*)g_frame.msg, usartx_rx_buf, MAX_LEN);
        g_frame.size = size;
        g_frame.USARTx = USARTx;

        if(0) {
        }
        else if(usartx_rx_buf[0] == '?' || usartx_rx_buf[0] == '#') {
            g_frame.type = FRAME_TYPE_CLI;
        }
        else {
            g_frame.type = FRAME_TYPE_FLASHER;
        }
    }
    bzero((char*)usartx_rx_buf, sizeof(usart2_rx_buf));
    DMA_Cmd(dma_channel, ENABLE);
}
