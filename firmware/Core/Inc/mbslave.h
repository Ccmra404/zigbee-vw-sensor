/*
 * Copyright (c) 2006-2100, Tommy Guo
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-04-27     Tommy        first version
 */
 
#ifndef __MBSLAVE_H__
#define __MBSLAVE_H__

#include "stm32l0xx_hal.h"
#include "string.h"

/* 接收和发送缓冲区大小 */
#define MB_BUF_SIZE        120

typedef struct
{
    /* 接收缓冲区 */
    uint8_t rx_buf[MB_BUF_SIZE];
    /* 接收完成标志 */
    uint8_t rx_end_flg;
}MB_PARAM;

/**
  * @brief  清空 Modbus 接收缓冲区状态。
  * @param[out] port 待初始化的接收状态对象。
  * @retval 无
  */
void mbslave_init(MB_PARAM *port);

/**
  * @brief  计算标准 Modbus RTU CRC16。
  * @param[in] buf 待校验数据缓冲区。
  * @param[in] len 参与校验的字节数。
  * @retval 计算得到的 CRC16 值；完整有效 RTU 帧返回 0。
  */
uint16_t crc16_calc(uint8_t *buf, uint16_t len);

#endif
/*****END OF FILE*****/
