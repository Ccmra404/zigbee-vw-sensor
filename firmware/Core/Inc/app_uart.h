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
  * @brief  Initialize software receive states for all application UARTs.
  * @retval None
  */
void AppUart_InitContexts(void);

/**
  * @brief  Clamp an interrupt-reported UART frame length to the local buffer size.
  * @param[in] length Interrupt-reported byte count.
  * @retval Safe byte count for MB_BUF_SIZE buffers.
  */
int AppUart_LimitFrameLength(int length);

/**
  * @brief  Copy a completed UART frame into an application buffer.
  * @param[out] dst Destination buffer.
  * @param[in] src Source UART receive buffer.
  * @param[in] length Received byte count.
  * @retval Copied byte count after bounds checking.
  */
int AppUart_CopyRxData(u8 *dst, const u8 *src, int length);

#endif
