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

unsigned long reverse32(unsigned long data)
{
    data = ((data >> 1) & 0x55555555) | ((data & 0x55555555) << 1);
    data = ((data >> 2) & 0x33333333) | ((data & 0x33333333) << 2);
    data = ((data >> 4) & 0x0f0f0f0f) | ((data & 0x0f0f0f0f) << 4);
    data = ((data >> 8) & 0x00ff00ff) | ((data & 0x00ff00ff) << 8);
    data = (data >> 16) | (data << 16);
    return data;
}

unsigned long CRC32_Calc(unsigned long initial, const void* msg, int size)
{

    CRC_SetInitRegister(reverse32(initial));
    for(register int i = 0; i < size; i++)
        CRC_CalcCRC8bits(*(unsigned char*)(msg + i));
    return CRC_GetCRC();
}

