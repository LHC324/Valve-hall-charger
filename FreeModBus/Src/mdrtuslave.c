#include <stdlib.h>
#include "mdrtuslave.h"
#include "mdcrc16.h"
#include "usart.h"
#include "shell_port.h"
#include "tool.h"
#include "dwin.h"
#if defined(USING_FREERTOS)
#include "cmsis_os.h"
#endif

#if (USER_MODBUS_LIB)

/*modebus对象选用的目标串口*/
#define MDSLAVE1_UART huart1
#define MDSLAVE2_UART huart2

#if defined(USING_FREERTOS)
extern void *pvPortMalloc(size_t xWantedSize);
extern void vPortFree(void *pv);
#endif

/*定义Modbus句柄*/
ModbusRTUSlaveHandler mdSlave1;
ModbusRTUSlaveHandler mdSlave2;

/**
 * @brief	modbus钩子函数
 * @details
 * @param   @handler 句柄
 * @param	@addr 当前寄存器地址
 * @retval	None
 */
static mdVOID mdRTUHook(ModbusRTUSlaveHandler handler, mdU16 addr)
{
    //     pDwinHandle pd = Dwin_Object;
    //     Save_HandleTypeDef *ps = &Save_Flash;
    //     extern uint32_t FLASH_Write(uint32_t Address, const uint16_t *pBuf, uint32_t Size);
    //     bool save_flag = false;

    //     /*处于后台参数区*/
    //     if ((addr >= PARAM_MD_ADDR) && (addr < MDUSER_NAME_ADDR))
    //     {
    //         uint8_t site = (addr - PARAM_MD_ADDR) / 2U;
    //         float data = 0, temp_data = 0;

    //         mdSTATUS ret = mdRTU_ReadHoldRegisters(handler, addr, 2U, (mdU16 *)&data);
    //         if (ret == mdFALSE)
    //         {
    // #if defined(USING_DEBUG)
    //             shellPrint(Shell_Object, "Holding register addr[0x%x], Read: %.3f failed!\r\n", addr, data);
    // #endif
    //         }
    //         else
    //         {
    //             float *pdata = (float *)pd->Slave.pHandle;
    //             /*保留迪文屏幕值*/
    //             temp_data = data;
    //             Endian_Swap((uint8_t *)&data, 0U, sizeof(float));
    //             if ((data >= pd->Slave.pMap[site].lower) && (data <= pd->Slave.pMap[site].upper))
    //             {
    //                 /*确认数据回传到屏幕*/
    //                 pd->Dw_Write(pd, pd->Slave.pMap[site].addr, (uint8_t *)&temp_data, sizeof(float));
    //                 if (site < pd->Slave.Events_Size)
    //                 {
    //                     pdata[site] = data;
    //                     /*存储标志*/
    //                     save_flag = true;
    //                 }
    //             }
    //         }
    // #if defined(USING_DEBUG)
    //         shellPrint(Shell_Object, "Modbus[0x%x] received a data: %.3f.\r\n", addr, data);
    // #endif
    //     }
    //     /*用户名和密码处理*/
    //     else if (addr >= MDUSER_NAME_ADDR)
    //     {
    //         uint16_t data = 0;
    //         mdSTATUS ret = mdRTU_ReadHoldRegisters(handler, addr, 2U, (mdU16 *)&data);
    //         if (ret == mdFALSE)
    //         {
    // #if defined(USING_DEBUG)
    //             shellPrint(Shell_Object, "Holding register addr[0x%x], Read: 0x%x failed!\r\n", addr, data);
    // #endif
    //         }
    //         else
    //         {
    // #define NUMBER_USER_MAX 9999U
    //             uint8_t site = addr - PARAM_MD_ADDR;
    //             uint16_t *puser = (uint16_t *)pd->Slave.pHandle;
    //             // Endian_Swap((uint8_t *)&data, 0U, sizeof(uint16_t));
    //             if (data <= NUMBER_USER_MAX)
    //             {
    //                 puser[site] = data;
    //                 /*存储标志*/
    //                 save_flag = true;
    //             }
    //         }
    // #if defined(USING_DEBUG)
    //         shellPrint(Shell_Object, "Modbus[0x%x] received a data: %d.\r\n", addr, data);
    // #endif
    //     }
    //     if (save_flag)
    //     {
    //         save_flag = false;
    // #if defined(USING_FREERTOS)
    //         taskENTER_CRITICAL();
    // #endif
    //         /*计算crc校验码*/
    //         ps->Param.crc16 = Get_Crc16((uint8_t *)&ps->Param, sizeof(Save_Param) - sizeof(ps->Param.crc16), 0xFFFF);
    //         /*参数保存到Flash*/
    //         FLASH_Write(PARAM_SAVE_ADDRESS, (uint16_t *)&Save_Flash.Param, sizeof(Save_Param));
    // #if defined(USING_FREERTOS)
    //         taskEXIT_CRITICAL();
    // #endif
    // #if defined(USING_DEBUG)
    //         shellPrint(Shell_Object, "Save parameters...\r\n");
    // #endif
    //     }
}

/**
 * @brief	接口：Modbus协议栈发送底层接口
 * @details
 * @param	@c 待发送字符
 * @param   @handler 句柄
 * @retval	None
 */
static mdVOID popchar(ModbusRTUSlaveHandler handler, mdU8 *data, mdU32 length)
{
    // UART_HandleTypeDef *pHuart = (handler->slaveId == SLAVE1_ID) ? &MDSLAVE1_UART : &MDSLAVE2_UART;
    UART_HandleTypeDef *pHuart = &MDSLAVE1_UART;
/*启用DMA发送一包数据*/
#if (USING_DMA_TRANSPORT)

    HAL_UART_Transmit_DMA(pHuart, data, length);
    while (__HAL_UART_GET_FLAG(pHuart, UART_FLAG_TC) == RESET)
    {
    }
#else
    HAL_UART_Transmit(pHuart, data[i], length, 0xFFFF);
#endif
}

/*
    ModbusInit
        @void
    接口：初始化Modbus协议栈
*/
mdAPI mdVOID MX_ModbusInit(mdVOID)
{
    struct ModbusRTUSlaveRegisterInfo info;
    info.slaveId = SLAVE1_ID;
    info.usartBaudRate = SLAVE1_BUAD_RATE;
    info.mdRTUPopChar = popchar;
    /*Creation failed*/
    if (mdCreateModbusRTUSlave(&Slave1_Object, info) != mdTRUE)
    {
#if defined(USING_DEBUG)
        // shellPrint(Shell_Object,"AD[%d] = 0x%d\r\n", 0, Get_AdcValue(ADC_CHANNEL_1));
#endif
    }
    info.slaveId = SLAVE2_ID;
    info.usartBaudRate = SLAVE2_BUAD_RATE;
    if (mdCreateModbusRTUSlave(&Slave2_Object, info) != mdTRUE)
    {
#if defined(USING_DEBUG)
        // shellPrint(Shell_Object,"AD[%d] = 0x%d\r\n", 0, Get_AdcValue(ADC_CHANNEL_1));
#endif
    }
}

#if (!USING_DMA_TRANSPORT)
/*
    portRtuPushChar
        @handler 句柄
        @c 待接收字符
        @return
    接口：接收一个字符
*/
static mdVOID portRtuPushChar(ModbusRTUSlaveHandler handler, mdU8 c)
{
    ReceiveBufferHandle recbuf = handler->receiveBuffer;
    recbuf->buf[recbuf->count++] = c;
}

/*
    portRtuPushString
        @handler 句柄
        @*data   数据
        @length  数据长度
    接口：接收一个字符串
*/
static mdVOID portRtuPushString(ModbusRTUSlaveHandler handler, mdU8 *data, mdU32 length)
{
    ReceiveBufferHandle recbuf = handler->receiveBuffer;
    memcpy(&recbuf->buf[0], data, length);
    recbuf->count = length;
}
#endif

/*
    mdRTUSendString
        @handler 句柄
        @*data   数据缓冲区
        @length  数据长度
        @return
    接口：发送一帧数据
*/
static mdVOID mdRTUSendString(ModbusRTUSlaveHandler handler, mdU8 *data, mdU32 length)
{
    if (handler)
    {
        handler->mdRTUPopChar(handler, data, length);
    }
}

/*
    mdRTUError
        @handler 句柄
        @error   错误码
        @return
    接口：Modbus协议栈出错处理
*/
static mdVOID mdRTUError(ModbusRTUSlaveHandler handler, mdU8 error)
{
}

#if defined(USING_MASTER)

// typedef struct
// {
//     mdU8 Conter;
//     mdU8 *pBuf;
// } mdRTUMaster;
// mdRTUMaster g_Master;

/**
 * @brief  带CRC的发送从站数据
 * @param  _pBuf 数据缓冲区指针
 * @param  _ucLen 数据长度
 * @retval None
 */
// mdVOID mdRTU_SendWithCRC(mdU8 *_pBuf, mdU8 _ucLen)
// {
// #define MOD_TX_BUF_SIZE 64U

//     uint16_t crc;
//     uint8_t buf[MOD_TX_BUF_SIZE];

//     memcpy(buf, _pBuf, _ucLen);
//     crc = mdCrc16(_pBuf, _ucLen);
//     buf[_ucLen++] = crc;
//     buf[_ucLen++] = crc >> 8;

//     HAL_UART_Transmit(&MDSLAVE1_UART, buf, _ucLen, 0xffff);
// }

/**
 * @brief  有人云自定义46指令
 * @param  slaveaddr 从站地址
 * @param  regaddr 寄存器开始地址
 * @param  reglength 寄存器长度
 * @param  dat 数据
 * @retval None
 */
mdVOID mdRTUHandleCode46(ModbusRTUSlaveHandler handler, mdU16 regaddr, mdU16 reglen, mdU8 datalen, mdU8 *dat)
{
    mdU8 frame_head[] = {handler->slaveId, 0x46, regaddr >> 8U, regaddr, reglen >> 8U, reglen,
                         datalen};
    mdU16 counter = sizeof(frame_head);
    mdU16 crc16 = 0U;
#if defined(USING_FREERTOS)
    mdU8 *pBuf = (mdU8 *)CUSTOM_MALLOC(counter + datalen + sizeof(crc16));
    if (!pBuf)
        goto __exit;
#else
    mdU8 pBuf[MODBUS_PDU_SIZE_MAX];
    memset(pBuf, 0x00, MODBUS_PDU_SIZE_MAX);
#endif
    memcpy(pBuf, frame_head, counter);
    /*发送数据拷贝到发送缓冲区*/
    memcpy(&pBuf[counter], dat, datalen);
    counter += datalen;
    crc16 = mdCrc16(pBuf, counter);
    memcpy(&pBuf[counter], &crc16, sizeof(crc16));
    counter += sizeof(crc16);

    handler->mdRTUPopChar(handler, pBuf, counter);
    // HAL_UART_Transmit(&MDSLAVE1_UART, pBuf, counter, 0xFFFF);

__exit:
#if defined(USING_FREERTOS)
    CUSTOM_FREE(pBuf);
#endif
}

#endif

/*
    mdRTUHandleCode1
        @handler 句柄
        @return
    接口：解析01功能码
*/
static mdVOID mdRTUHandleCode1(ModbusRTUSlaveHandler handler)
{
    //    mdU32 reclen = handler->receiveBuffer->count;
    mdU8 *recbuf = handler->receiveBuffer->buf;
    RegisterPoolHandle regPool = handler->registerPool;
    mdU16 startAddress = ToU16(recbuf[2], recbuf[3]);
    mdU16 length = ToU16(recbuf[4], recbuf[5]);
    mdBit *data;
    mdU8 *data2;
    mdU8 length2 = 0;
    mdU16 crc;

    mdmalloc(data, mdBit, length);
    regPool->mdReadCoils(regPool, startAddress, length, data);
    length2 = length % 8 > 0 ? length / 8 + 1 : length / 8;
    mdmalloc(data2, mdU8, 5 + length2);
    data2[0] = recbuf[0];
    data2[1] = recbuf[1];
    data2[2] = length2;
    for (mdU32 i = 0; i < length2; i++)
    {
        for (mdU32 j = 0; j < 8 && (i * 8 + j) < length; j++)
        {
            data2[3 + i] |= ((data[i * 8 + j] & 0x01) << j);
        }
    }
    crc = mdCrc16(data2, 3 + length2);
    /*注意CRC顺序*/
    data2[3 + length2] = LOW(crc);
    data2[4 + length2] = HIGH(crc);
    handler->mdRTUSendString(handler, data2, 5 + length2);
    mdfree(data);
    mdfree(data2);
}

static mdVOID mdRTUHandleCode2(ModbusRTUSlaveHandler handler)
{
    mdU8 *recbuf = handler->receiveBuffer->buf;
    RegisterPoolHandle regPool = handler->registerPool;
    mdU16 startAddress = ToU16(recbuf[2], recbuf[3]);
    mdBit *data;
    mdU16 length = ToU16(recbuf[4], recbuf[5]);
    mdU8 *data2;
    mdU8 length2 = 0;
    mdU16 crc;

    mdmalloc(data, mdBit, length);
    regPool->mdReadInputCoils(regPool, startAddress, length, data);
    length2 = length % 8 > 0 ? length / 8 + 1 : length / 8;
    mdmalloc(data2, mdU8, 5 + length2);
    data2[0] = recbuf[0];
    data2[1] = recbuf[1];
    data2[2] = length2;
    for (mdU32 i = 0; i < length2; i++)
    {
        for (mdU32 j = 0; j < 8 && (i * 8 + j) < length; j++)
        {
            data2[3 + i] |= ((data[i * 8 + j] & 0x01) << j);
        }
    }
    crc = mdCrc16(data2, 3 + length2);
    data2[3 + length2] = LOW(crc);
    data2[4 + length2] = HIGH(crc);
    handler->mdRTUSendString(handler, data2, 5 + length2);
    mdfree(data);
    mdfree(data2);
}

static mdVOID mdRTUHandleCode3(ModbusRTUSlaveHandler handler)
{
    mdU8 *recbuf = handler->receiveBuffer->buf;
    RegisterPoolHandle regPool = handler->registerPool;
    mdU16 startAddress = ToU16(recbuf[2], recbuf[3]);
    mdU16 *data;
    mdU16 length = ToU16(recbuf[4], recbuf[5]);
    mdU8 *data2;
    mdU16 crc;

    mdmalloc(data, mdU16, length);
    regPool->mdReadHoldRegisters(regPool, startAddress, length, data);
    /*此处为了解决由于ARM小端存储造成的半字顺序混乱问题*/
    if (length > sizeof(mdU8))
    { /*只针对2个及以上寄存器而言*/
        mdU16Swap(data, length);
    }

    mdmalloc(data2, mdU8, 5 + length * 2);
    data2[0] = recbuf[0];
    data2[1] = recbuf[1];
    data2[2] = (mdU8)(length * 2);
    for (mdU32 i = 0; i < length; i++)
    {
        data2[3 + 2 * i] = HIGH(data[i]);
        data2[3 + 2 * i + 1] = LOW(data[i]);
    }
    crc = mdCrc16(data2, 3 + length * 2);
    data2[3 + length * 2] = LOW(crc);
    data2[4 + length * 2] = HIGH(crc);
    handler->mdRTUSendString(handler, data2, 5 + length * 2);
    mdfree(data);
    mdfree(data2);
}

static mdVOID mdRTUHandleCode4(ModbusRTUSlaveHandler handler)
{
    mdU8 *recbuf = handler->receiveBuffer->buf;
    RegisterPoolHandle regPool = handler->registerPool;
    mdU16 startAddress = ToU16(recbuf[2], recbuf[3]);
    mdU16 *data;
    mdU16 length = ToU16(recbuf[4], recbuf[5]);
    mdU8 *data2;
    mdU16 crc;

    mdmalloc(data, mdU16, length);
    regPool->mdReadInputRegisters(regPool, startAddress, length, data);
    /*此处为了解决由于ARM小端存储造成的半字顺序混乱问题*/
    if (length > sizeof(mdU8))
    { /*只针对2个及以上寄存器而言*/
        mdU16Swap(data, length);
    }
    mdmalloc(data2, mdU8, 5 + length * 2);
    data2[0] = recbuf[0];
    data2[1] = recbuf[1];
    data2[2] = (mdU8)(length * 2);
    for (mdU32 i = 0; i < length; i++)
    {
        data2[3 + 2 * i] = HIGH(data[i]);
        data2[3 + 2 * i + 1] = LOW(data[i]);
    }
    crc = mdCrc16(data2, 3 + length * 2);
    data2[3 + length * 2] = LOW(crc);
    data2[4 + length * 2] = HIGH(crc);
    handler->mdRTUSendString(handler, data2, 5 + length * 2);
    mdfree(data);
    mdfree(data2);
}

static mdVOID mdRTUHandleCode5(ModbusRTUSlaveHandler handler)
{
    mdU32 reclen = handler->receiveBuffer->count;
    mdU8 *recbuf = handler->receiveBuffer->buf;
    RegisterPoolHandle regPool = handler->registerPool;
    mdU16 startAddress = ToU16(recbuf[2], recbuf[3]);
    mdBit data = ToU16(recbuf[4], recbuf[5]) > 0 ? mdHigh : mdLow;
    regPool->mdWriteCoil(regPool, startAddress, data);
    handler->mdRTUSendString(handler, recbuf, reclen);
}

static mdVOID mdRTUHandleCode6(ModbusRTUSlaveHandler handler)
{
    mdU32 reclen = handler->receiveBuffer->count;
    mdU8 *recbuf = handler->receiveBuffer->buf;
    RegisterPoolHandle regPool = handler->registerPool;
    mdU16 startAddress = ToU16(recbuf[2], recbuf[3]);
    mdU16 data = ToU16(recbuf[4], recbuf[5]);
    regPool->mdWriteHoldRegister(regPool, startAddress, data);
    handler->mdRTUSendString(handler, recbuf, reclen);
}

static mdVOID mdRTUHandleCode15(ModbusRTUSlaveHandler handler)
{
    mdU8 *recbuf = handler->receiveBuffer->buf;
    RegisterPoolHandle regPool = handler->registerPool;
    mdU16 startAddress = ToU16(recbuf[2], recbuf[3]);
    mdU16 length = ToU16(recbuf[4], recbuf[5]);
    mdU8 *data;
    mdU16 crc;
    for (mdU32 i = 0; i < length; i++)
    {
        regPool->mdWriteCoil(regPool, startAddress + i, ((recbuf[7 + i / 8] >> (i % 8)) & 0x01));
    }
    mdmalloc(data, mdU8, 8);
    memcpy(data, recbuf, 6);
    crc = mdCrc16(data, 6);
    data[6] = LOW(crc);
    data[7] = HIGH(crc);
    handler->mdRTUSendString(handler, data, 8);
    mdfree(data);
}

static mdVOID mdRTUHandleCode16(ModbusRTUSlaveHandler handler)
{
    mdU8 *recbuf = handler->receiveBuffer->buf;
    RegisterPoolHandle regPool = handler->registerPool;
    mdU16 startAddress = ToU16(recbuf[2], recbuf[3]);
    mdU16 length = ToU16(recbuf[4], recbuf[5]);
    mdU8 *data;
    mdU16 crc;
    for (mdU32 i = 0; i < length; i++)
    {
        regPool->mdWriteHoldRegister(regPool, startAddress + i,
                                     ToU16(recbuf[7 + (length - 2 * i)], recbuf[7 + (length - 2 * i) + 1]));
    }
    mdmalloc(data, mdU8, 8);
    memcpy(data, recbuf, 6);
    crc = mdCrc16(data, 6);
    data[6] = LOW(crc);
    data[7] = HIGH(crc);
    handler->mdRTUSendString(handler, data, 8);
    mdfree(data);
}

/*
    mdRtuBaseTimerTick
        @handler 句柄
        @time   时长跨度，单位 us
        @return
    接口：帧间隙监控
*/
static mdVOID portRtuTimerTick(ModbusRTUSlaveHandler handler, mdU32 ustime)
{
    // static mdU32 lastCount;
    // static mdU64 timeSum;
    // static mdFMStatus error;
    // if (handler->receiveBuffer->count > 0)
    // {
    //     if (handler->receiveBuffer->count != lastCount)
    //     {
    //         if (timeSum > handler->invalidTime)
    //         {
    //             error++;
    //         }
    //         lastCount = handler->receiveBuffer->count;
    //         timeSum = 0;
    //     }
    //     if (timeSum > handler->stopTime)
    //     {
    //         if (error == 0 || IGNORE_LOSS_FRAME != 0)
    //         { /*发生主机对从机寄存器写数据操作*/
    //             handler->updateFlag = true;
    //             handler->mdRTUCenterProcessor(handler);
    //         }
    //         else
    //         {
    //             handler->mdRTUError(handler, ERROR1);
    //         }
    //         mdClearReceiveBuffer(handler->receiveBuffer);
    //         TIMER_CLEAN();
    //     }
    //     timeSum += ustime;
    // }
    // else
    // {
    //     TIMER_CLEAN();
    // }
    /*发生主机对从机寄存器写数据操作*/
    handler->updateFlag = true;
    handler->mdRTUCenterProcessor(handler);
    mdClearReceiveBuffer(handler->receiveBuffer);
}

/*
    mdModbusRTUCenterProcessor
        @handler 句柄
        @receFrame 待处理的帧（已校验通过）
    处理一帧，并且通过接口发送处理结果
*/
static mdVOID mdRTUCenterProcessor(ModbusRTUSlaveHandler handler)
{
    mdU32 reclen = handler->receiveBuffer->count;
    mdU8 *recbuf = handler->receiveBuffer->buf;
    if (reclen < 3)
    {
        handler->mdRTUError(handler, ERROR2);
        return;
    }
    if (mdCrc16(recbuf, reclen - 2) != mdGetCrc16() && CRC_CHECK != 0)
    {
        handler->mdRTUError(handler, ERROR3);
        return;
    }
    if (mdGetSlaveId() != handler->slaveId)
    {
        handler->mdRTUError(handler, ERROR4);
        return;
    }
    switch (mdGetCode())
    {
    case MODBUS_CODE_1:
        handler->mdRTUHandleCode1(handler);
        break;
    case MODBUS_CODE_2:
        handler->mdRTUHandleCode2(handler);
        break;
    case MODBUS_CODE_3:
        handler->mdRTUHandleCode3(handler);
        break;
    case MODBUS_CODE_4:
        handler->mdRTUHandleCode4(handler);
        break;
    case MODBUS_CODE_5:
        handler->mdRTUHandleCode5(handler);
        break;
    case MODBUS_CODE_6:
        handler->mdRTUHandleCode6(handler);
        /*更改用户名和密码*/
        handler->mdRTUHook(handler, ToU16(recbuf[2], recbuf[3]));
        break;
    case MODBUS_CODE_15:
        handler->mdRTUHandleCode15(handler);
        break;
    case MODBUS_CODE_16:
        handler->mdRTUHandleCode16(handler);
        /*更新屏幕后台参数*/
        handler->mdRTUHook(handler, ToU16(recbuf[2], recbuf[3]));
        break;
    default:
        handler->mdRTUError(handler, ERROR5);
        break;
    }
}

/* ================================================================== */
/*                        API                                         */
/* ================================================================== */
/*
    mdCreateModbusRTUSlave
        @handler 句柄
        @mdRtuPopChar 字符发送函数
    创建一个modbus从机
*/
mdSTATUS mdCreateModbusRTUSlave(ModbusRTUSlaveHandler *handler, struct ModbusRTUSlaveRegisterInfo info)
{
#if defined(USING_FREERTOS)
    (*handler) = (ModbusRTUSlaveHandler)CUSTOM_MALLOC(sizeof(struct ModbusRTUSlave));
#else
    (*handler) = (ModbusRTUSlaveHandler)malloc(sizeof(struct ModbusRTUSlave));
#endif
#if defined(USING_DEBUG)
    shellPrint(Shell_Object, "Slave[%d]_handler = 0x%p\r\n", info.slaveId, *handler);
#endif
    if ((*handler) != NULL)
    {
        (*handler)->mdRTUHook = mdRTUHook;
        (*handler)->mdRTUPopChar = info.mdRTUPopChar;
        (*handler)->mdRTUCenterProcessor = mdRTUCenterProcessor;
        (*handler)->mdRTUError = mdRTUError;
        (*handler)->slaveId = info.slaveId;
        (*handler)->invalidTime = (int)(1.5 * DATA_BITS * 1000 * 1000 / info.usartBaudRate);
        (*handler)->stopTime = (int)(3.5 * DATA_BITS * 1000 * 1000 / info.usartBaudRate);
        (*handler)->updateFlag = false;
#if (!USING_DMA_TRANSPORT)
        (*handler)->portRTUPushChar = portRtuPushChar;
        (*handler)->portRTUPushString = portRtuPushString;
#else
        (*handler)->portRTUPushChar = NULL;
        (*handler)->portRTUPushString = NULL;
#endif
        (*handler)->portRTUTimerTick = portRtuTimerTick;
        (*handler)->mdRTUSendString = mdRTUSendString;
        (*handler)->mdRTUHandleCode1 = mdRTUHandleCode1;
        (*handler)->mdRTUHandleCode2 = mdRTUHandleCode2;
        (*handler)->mdRTUHandleCode3 = mdRTUHandleCode3;
        (*handler)->mdRTUHandleCode4 = mdRTUHandleCode4;
        (*handler)->mdRTUHandleCode5 = mdRTUHandleCode5;
        (*handler)->mdRTUHandleCode6 = mdRTUHandleCode6;
        (*handler)->mdRTUHandleCode15 = mdRTUHandleCode15;
        (*handler)->mdRTUHandleCode16 = mdRTUHandleCode16;
        (*handler)->mdRTUHandleCode46 = mdRTUHandleCode46;

        if (mdCreateRegisterPool(&((*handler)->registerPool)) &&
            mdCreateReceiveBuffer(&((*handler)->receiveBuffer)))
        {
            return mdTRUE;
        }
        else
        {
#if defined(USING_DEBUG)
            shellPrint(Shell_Object, "Cpool = %d, Crec = %d\r\n", mdCreateRegisterPool(&((*handler)->registerPool)), mdCreateReceiveBuffer(&((*handler)->receiveBuffer)));
#endif
#if defined(USING_FREERTOS)
            CUSTOM_FREE((*handler));
            while (1)
                ;
#else
            free((*handler));
#endif
        }
    }
    return mdFALSE;
}

/*
    mdDestoryModbusRTUSlave
        @handler 句柄
    销毁一个modbus从机
*/
mdVOID mdDestoryModbusRTUSlave(ModbusRTUSlaveHandler *handler)
{
    mdDestoryRegisterPool(&((*handler)->registerPool));
    mdDestoryReceiveBuffer(&((*handler)->receiveBuffer));
    mdfree(*handler);
    (*handler) = NULL;
}

/*
    mdU16Swap
        @*data   交换的缓冲区
        @*length 长度
    交换一个mdU16缓冲区中相邻两个元素
*/
mdVOID mdU16Swap(mdU16 *data, mdU32 length)
{
    mdU16 temp = 0;

    for (mdU16 *p = &data[0]; p < &data[0] + length; p += 2)
    {
        temp = *p;
        *p = *(p + 1);
        *(p + 1) = temp;
    }
}

#endif
