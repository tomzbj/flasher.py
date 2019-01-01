# flasher.py
Simple spiflash manager through serial port.

# 使用方法
## 上位机:

运行环境: python3, 需要pyserial库.

开头的几个常量如波特率之类, 需要自己设置一下. 

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
