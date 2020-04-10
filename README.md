# flasher.py
Simple spiflash manager through serial port.

# 使用方法
## 上位机:

运行环境: python3, 需要pyserial库.

开头的几个常量如波特率之类, 需要自己设置一下. 注意读写块大小不能大于下位机里的相应设置.

其中LIMIT_256K用于调试, 设为True则无论flash大小一律按256k/64个块来处理.  正常情况设置为False.

如果运行时提示找不到串口, 可以修改一下第76行:     for i in range(16), 改为for i in range(2, 16), 跳过com1.

## 下位机

见demo/firmware/thirdparty/flasher下的flasher.h和flasher.c.

在工程中使用时需要提供flash读/写/擦除/读取信息和串口输出的函数指针, 用fl_init函数初始化.

    static fl_config_t icfg;
    icfg.read_f = SPIFLASH_FastRead;
    icfg.write_f = SPIFLASH_Write;
    icfg.erase_f = SPIFLASH_Erase;
    icfg.readinfo_f = SPIFLASH_ReadJedecID;
    icfg.uwrite_f = uputs;
    fl_init(&icfg);
之后在串口数据帧处理程序里调用fl_parse即可.   

## 性能

编写一个批处理文件, 内容如下:

    time < nul
    py flasher.py -w hzk11.bin
    time < nul

运行结果如下:

    D:\work\__my_github\flasher.py>time  <nul
    当前时间: 21:45:58.18
    输入新时间:
    D:\work\__my_github\flasher.py>py flasher.py -w hzk11.bin
    Connected, Port=COM11, baudrate=1000000

    Erasing 100%...
    Writing 100%...
    Verifying... OK!


    D:\work\__my_github\flasher.py>time <nul
    当前时间: 21:46:06.34
    输入新时间:

hzk11.bin的大小是167200字节, 写入用时8.16s, 平均写入速度20k/s.

再试读取hzk11.bin, 用时3.68s, 平均读取速度44k/s.

实际上写入时间里擦除占了很大一部分. 

demo程序里的擦除函数一律按4k块进行, 写167200字节需要执行41次4k擦除, 按W25Q32的典型值45ms计算, 需要约1.8s.

如果加一个判断, 有连续、对齐的32k/64k块时优先按32k/64k擦除, 则167200字节需要进行两次64k擦除、一次32k擦除和一次4k擦除, 总共只需要465ms, 可以节约一秒多.

## 支持的SPIFLASH型号

查阅了市面常见的SPIFLASH手册, 如下型号的读/写/4K擦除命令基本相同, 都是03h/0Bh、02h、20h, 读取JEDEC ID命令都是9Fh, 因此程序不加修改可以直接互换使用.

    W25Q16 SST26VF032 GD25Q16 EN25F40 AT25SF641 FM25F01 
    P25Q32 MX25L2006E KH25L4006 S25FL128S AT25SF081 

比较奇葩的是SST25VF016, 虽然有快速擦除的优势, 但写命令和其他型号不兼容. 另一个是M25P16, 不支持4K擦除, 只能64K或全片擦除.

## Bugs

对大文件求CRC32时可能会超时报错, 具体情况看配置. 在我的某块板上一开始是对512k的文件算CRC32就会超时, 因为DMA通道不够用, 没有给SPIFLASH启用DMA. 给CRC32启用DMA之后有改善. 
如果是在8位机上运行, 则需要对大文件的CRC32做分块处理, 有空再做.


## 常见问题解决

1. 通讯错误
    1. 检查串口接收缓冲区是否足够大, 至少应有512字节, 再加上包头包尾等, 保险起见应设置为600字节左右.
    2. 检查SPI分频系数. 在STM32F103的SPI1上应设置为4分频, 2分频时会出错.    
