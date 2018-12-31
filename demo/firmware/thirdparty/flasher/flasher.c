#include "misc.h" //debug
#include <string.h>
#include "flasher.h"
#include "crc32.h"

static fl_config_t g;

#define MAX_PACK_SIZE 1024

void fl_init(fl_config_t* cfg)
{
    memcpy(&g, cfg, sizeof(fl_config_t));
}

static void ReadImage(unsigned long pos, int size)
{
    unsigned char buf[MAX_PACK_SIZE];
    if(size > MAX_PACK_SIZE)
        size = MAX_PACK_SIZE;

    g.read_f(pos, size, buf);
    g.uwrite_f(buf, size);
}

static void ReadCRC32(unsigned long pos, int total_size)
{
    unsigned char buf[MAX_PACK_SIZE];
    int size = MAX_PACK_SIZE;
    unsigned long crc;

    CRC32_Init();
    crc = 0xffffffff;
    while(total_size > 0) {
        if(total_size < size)
            size = total_size;
        g.read_f(pos, size, buf);
        CRC32_Calc(buf, size);
//        crc = CRC32(crc, buf, size);
        pos += size;
        total_size -= size;
    }
//    crc = ~crc;
    crc = ~CRC_GetCRC();
    g.uwrite_f(&crc, 4);
}

// Make sure the flash area was erased!
static void WriteImage(unsigned long pos, unsigned short size,
    const void* content)
{
    g.write_f(pos, size, (void*)content);
    g.uwrite_f(0, 1);
}

static void EraseImage(unsigned long pos, unsigned short size)
{
    pos &= 0xfffff000;
    g.erase_f(pos, size);
    g.uwrite_f(0, 1);
}

static void CheckBlockEmpty(unsigned long pos)
{
    unsigned char buf[4096];

    pos &= 0xfffff000;  // position to 4k block border
    int size = 4096;
    unsigned char ret;

    g.read_f(pos, size, buf);
    for(int i = 0; i < size; i += 4) {
        if(*(unsigned long*)(buf + i) != 0xffffffff)
            goto end;
    }
    ret = 1;
    g.uwrite_f(&ret, 1);
    return;

    end:
    ret = 0;
    g.uwrite_f(&ret, 1);
}

//void fl_Parse(USART_TypeDef* USARTx, const unsigned char* msg, int size)
void fl_parse(const void* pmsg, int msg_size)
{
    unsigned char* msg = (unsigned char*)pmsg;
    unsigned char checksum = 0;
    const unsigned char* content;
    unsigned long pos;
    unsigned long size;

    if(msg_size < 2)
        return;
    for(int i = 0; i < msg_size; i++) {
        checksum += msg[i];
    }

    if(checksum != 0)
        return;

    switch(msg[0]) {            // func code
        case 0x90:              // get flash info
            pos = g.readinfo_f();
            g.uwrite_f(&pos, 4);
            break;
        case 0x91:              // check whether given block is empty
            pos = *(unsigned long*)(&msg[4]);
            CheckBlockEmpty(pos);
            break;
        case 0x92:
            size = *(unsigned long*)(&msg[4]);
            pos = *(unsigned long*)(&msg[8]);
            ReadCRC32(pos, size);
            break;
        case 0x9c:              // read full image
            size = *(unsigned short*)(&msg[2]);
            pos = *(unsigned long*)(&msg[4]);
            ReadImage(pos, size);
            break;
        case 0x9d:              // erase flash block
            size = *(unsigned short*)(&msg[2]);
            pos = *(unsigned long*)(&msg[4]);
            EraseImage(pos, size);
            break;
        case 0x9e:              // write image
            size = *(unsigned short*)(&msg[2]);
            pos = *(unsigned long*)(&msg[4]);
            content = &msg[8];
            WriteImage(pos, size, content);
            break;
    }
}
