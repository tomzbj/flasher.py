#include "misc.h"

void CRC32_Init(void)
{
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_CRC, ENABLE);
    CRC_DeInit();
    CRC_ResetDR();
    CRC_SetPolynomial(0x04c11db7);
    CRC_ReverseInputDataSelect(CRC_ReverseInputData_8bits);
    CRC_ReverseOutputDataCmd(ENABLE);
}

unsigned long CRC32_Calc(unsigned long initial, const void* msg, int size)
{
    CRC_SetInitRegister(initial);
    for(register int i = 0; i < size; i++)
        CRC_CalcCRC8bits(*(unsigned char*)(msg + i));
    return CRC_GetCRC();
}

