#include <stdio.h>
#include "stm32f0xx_it.h"
#include "platform.h"

static unsigned int g_usart_idle_count = 0;

void SysTick_Handler(void)
{
    static u16 count = 0;
    _delay_nms_mm();            // executes every 1ms
    count++;
    g_usart_idle_count++;
}

void USART1_IRQHandler(void)
{
    if (USART_GetITStatus(USART1, USART_IT_IDLE) != RESET) {
        USART_RX_IDLE_IRQHandler(USART1);
        USART_ClearITPendingBit(USART1, USART_IT_IDLE);
    }
}

void USART2_IRQHandler(void)
{
    if (USART_GetITStatus(USART2, USART_IT_IDLE) != RESET) {
        USART_RX_IDLE_IRQHandler(USART2);
        USART_ClearITPendingBit(USART2, USART_IT_IDLE);
    }
    if (USART_GetITStatus(USART2, USART_IT_ORE) != RESET) {
        USART_ClearITPendingBit(USART2, USART_IT_ORE);
    }
    if (USART_GetITStatus(USART2, USART_IT_FE) != RESET) {
        USART_ClearITPendingBit(USART2, USART_IT_FE);
    }
}

void PPP_IRQHandler(void) { while (1) { } } 
void NMI_Handler(void) { while (1) { } } 
void HardFault_Handler(void) { while (1) { } } 
void SVC_Handler(void) { while (1) { } } 
void PendSV_Handler(void) { while (1) { } }
