#ifndef __APP_UART_H
#define __APP_UART_H

#include "main.h"
#include "mbslave.h"

extern __IO char MsgFlag1;
extern __IO char MsgFlag2;
extern __IO char MsgFlag3;

extern MB_PARAM mb_usart1_t;
extern MB_PARAM mb_usart2_t;
extern MB_PARAM mb_usart3_t;

/**
  * @brief  初始化所有应用串口的软件接收状态。
  * @retval 无
  */
void AppUart_InitContexts(void);

/**
  * @brief  将中断上报的串口帧长度限制在本地缓冲区范围内。
  * @param[in] length 中断上报的字节数。
  * @retval 适用于 MB_BUF_SIZE 缓冲区的安全字节数。
  */
int AppUart_LimitFrameLength(int length);

/**
  * @brief  将已接收完成的串口帧复制到应用缓冲区。
  * @param[out] dst 目标缓冲区。
  * @param[in] src 源串口接收缓冲区。
  * @param[in] length 接收到的字节数。
  * @retval 边界检查后的实际复制字节数。
  */
int AppUart_CopyRxData(u8 *dst, const u8 *src, int length);

#endif
