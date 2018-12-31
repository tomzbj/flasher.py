#include "misc.h"

void CRC32_Init(void)
{
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_CRC, ENABLE);
    CRC_DeInit();
    CRC_ResetDR();
//    CRC_PolynomialSizeSelect(CRC_PolSize_32);
//    CRC_SetInitRegister(0xffffffff);
    CRC_SetPolynomial(0x04c11db7);
    CRC_ReverseInputDataSelect(CRC_ReverseInputData_8bits);
    CRC_ReverseOutputDataCmd(ENABLE);
}

void CRC32_Calc(const unsigned char* msg, int size)
{
    for(register int i = 0; i < size; i++)
        CRC_CalcCRC8bits(msg[i]);
}

unsigned long CRC32(unsigned long crc, unsigned char* msg, int size)
{
    const unsigned long polynormial = 0xedb88320;
    for(int i = 0; i < size; i++) {
        crc ^= msg[i];
        for(int j = 0; j < 8; j++) {
            bool lsb = crc % 2;
            crc >>= 1;
            if(lsb)
                crc ^= polynormial;
        }
    }
    return crc;
}
