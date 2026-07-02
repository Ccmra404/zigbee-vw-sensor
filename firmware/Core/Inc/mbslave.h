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
  * @brief  Clear the Modbus receive buffer state.
  * @param[out] port Receive state object to initialize.
  * @retval None
  */
void mbslave_init(MB_PARAM *port);

/**
  * @brief  Calculate standard Modbus RTU CRC16.
  * @param[in] buf Data buffer to check.
  * @param[in] len Number of bytes to include.
  * @retval Calculated CRC16 value. A valid full RTU frame returns 0.
  */
uint16_t crc16_calc(uint8_t *buf, uint16_t len);

#endif
/*****END OF FILE*****/
