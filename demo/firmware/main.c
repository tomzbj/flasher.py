#include "platform.h"
#include "spiflash.h"

static void Init(void)
{
    SystemInit();
    SystemCoreClockUpdate();
    SysTick_Config(48000UL);
    NVIC_SetPriority(SysTick_IRQn, 1);
    USART_Config();
    SPIFLASH_Config();
}

int main(void)
{
    Init();
    while(1) {
        USART_Poll();
    }
    return 0;
}
