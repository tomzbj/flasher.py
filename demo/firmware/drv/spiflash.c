#include "spiflash.h" 

//#include "usart.h"

typedef enum {
    CMD_READ = 0x03, CMD_FAST_READ = 0x0b, CMD_ERASE4K = 0x20,
    CMD_ERASE32K = 0x52, CMD_ERASE64K = 0xd8, CMD_ERASECHIP = 0x60,
    CMD_BYTE_PROGRAM = 0x02, CMD_AAI_WORD_PROGRAM = 0xad,
    CMD_READ_STATUS = 0x05, CMD_EN_WRITE_STATUS = 0x50,
    CMD_WRITE_STATUS = 0x01,
    CMD_WRITE_ENABLE = 0x06, CMD_WRITE_DISABLE = 0x04, CMD_READ_ID = 0x90,
    CMD_JEDEC_ID = 0x9f, CMD_POWERDOWN = 0xb9, CMD_RELEASE_POWERDOWN = 0xab
} spiflash_cmd_t;

#define ReadByte() WriteByte(0xff)
#define WriteEnable() WriteCmd(CMD_WRITE_ENABLE)
#define WriteDisable() WriteCmd(CMD_WRITE_DISABLE)
//#define WaitBusy() ({unsigned char _st; while((_st = SPIFLASH_ReadStatus()) & 0x1) {_delay_ms_loop(1); uprintf(USART1, "%02x\n", _st);}})
#define WaitBusy() ({while(SPIFLASH_ReadStatus() & 0x1); })

spiflash_writebyte_func_t WriteByte;
spiflash_cs_func_t SetCS;
spiflash_fastread_func_t FastRead;

static void WriteCmd(spiflash_cmd_t cmd);

/// @brief 传入spiflash字节读写及操作CS的函数指针.
/// @param spiflash_writebyte_func 字节读写函数
/// @param spiflash_cs_func 操作CS函数
/// @retval None
/// @todo 未来加入dma块读写的函数指针
#include "misc.h"
void SPIFLASH_Init(spiflash_config_t* cfg)
{
    WriteByte = cfg->writebyte_f;
    SetCS = cfg->cs_f;
    FastRead = cfg->fastread_f;

//    unsigned char ret;
//    _delay_ms_loop(100);

#if 1
    SetCS(0);
    WriteByte(CMD_EN_WRITE_STATUS);
    SetCS(1);
    ( {  for(volatile int n = 1000UL; n; n--);});
    SetCS(0);
    WriteByte(CMD_WRITE_STATUS);
    WriteByte(0x00);
    SetCS(1);
    WaitBusy();
#endif
}

/// @brief 写命令
/// @param spiflash操作命令
/// @retval 命令返回值
static void WriteCmd(spiflash_cmd_t cmd)
{
    SetCS(0);
    WriteByte(cmd);
    SetCS(1);
}

/// @brief 读取spiflash状态
/// @param None
/// @retval spiflash状态寄存器内容
unsigned char SPIFLASH_ReadStatus(void)
{
    unsigned char ret;
    SetCS(0);
    WriteByte(CMD_READ_STATUS);
    ret = ReadByte();
    SetCS(1);
    return ret;
}

/// @brief 读取spiflash信息
/// @retval 指向spiflash信息的静态结构体指针
unsigned long SPIFLASH_ReadJedecID(void)
{
    unsigned long ret;

    SetCS(0);
    WriteByte(CMD_JEDEC_ID);
    ret = ReadByte();
    ret <<= 8;
    ret |= ReadByte();
    ret <<= 8;
    ret |= ReadByte();
    SetCS(1);

    return ret;
}

/// @brief 从SPIFLASH读取数据.
/// @param addr 数据地址.
/// @param buf 数据缓冲区指针.
/// @param num 要读取的字节数.
/// @retval None
int SPIFLASH_FastRead(unsigned long addr, int num, void* buf)
{
    WaitBusy();
    SetCS(0);
    WriteByte(CMD_FAST_READ);
    WriteByte(addr >> 16);
    WriteByte(addr >> 8);
    WriteByte(addr);
    WriteByte(0xff);

    FastRead(buf, num);
    SetCS(1);
    WaitBusy();
    return 0;
}

int SPIFLASH_Read(unsigned long addr, int num, void* buf)
{
    WaitBusy();
    SetCS(0);
    WriteByte(CMD_FAST_READ);
    WriteByte(addr >> 16);
    WriteByte(addr >> 8);
    WriteByte(addr);
    WriteByte(0xff);

    for(int i = 0; i < num; i++) {
        *(unsigned char*)buf = ReadByte();
        buf++;
    }
    SetCS(1);
    WaitBusy();
    return 0;
}

/// @brief 向SPIFLASH写入数据(需要自行确保已擦除).
/// @param addr 数据地址.
/// @param buf 数据缓冲区指针.
/// @param num 要写入的字节数.
/// @retval None
int SPIFLASH_Write(unsigned long addr, int num, const void* buf)
{
    WaitBusy();
    while(num > 0) {
        WriteEnable();
        SetCS(0);
        WriteByte(0x02);
        WriteByte(addr >> 16);
        WriteByte(addr >> 8);
        WriteByte(addr);
        do {
            WriteByte(*(unsigned char*)buf);
            addr++;
            buf++;
            num--;
        } while((addr & 0xff) && (num > 0));
        SetCS(1);
        WaitBusy();
//        _delay_ms_loop(3);
        WriteDisable();
    }
    return 0;
}

/// @brief 擦除SPIFLASH的一个扇区(4K).
/// @param addr 擦除地址.
/// @retval None
void SPIFLASH_SectorErase(unsigned long addr)
{
    WaitBusy();
    WriteEnable();
    SetCS(0);
    WriteByte(CMD_ERASE4K);
    WriteByte(addr >> 16);
    WriteByte(addr >> 8);
    WriteByte(addr);
    SetCS(1);
    WriteDisable();
    WaitBusy();
}

/// @brief 擦除SPIFLASH的一个块(64K).
/// @param addr 擦除地址.
/// @retval None
int SPIFLASH_Erase(unsigned long addr, int nbytes)
{
    addr &= 0xfffff000;
    do {
        SPIFLASH_SectorErase(addr);
        addr += 4096UL;
        nbytes -= 4096UL;
    } while(nbytes > 0);
    return 0;
}

/// @brief 擦除SPIFLASH的全部内容.
/// @param None
/// @retval None
void SPIFLASH_ChipErase(void)
{
    WaitBusy();
    WriteEnable();
    SetCS(0);
    WriteByte(CMD_ERASECHIP);
    SetCS(1);
    WriteDisable();
    WaitBusy();
}

void SPIFLASH_EnterLowPowerMode(void)
{
    WaitBusy();
    SetCS(0);
    WriteByte(CMD_POWERDOWN);
    SetCS(1);
}

void SPIFLASH_LeaveLowPowerMode(void)
{
    SetCS(0);
    WriteByte(CMD_RELEASE_POWERDOWN);
    SetCS(1);
    _delay_us(10);
}
