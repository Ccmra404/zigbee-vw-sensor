#include "app_uart.h"

__IO char MsgFlag1;
__IO char MsgFlag2;
__IO char MsgFlag3;

MB_PARAM mb_usart1_t;
MB_PARAM mb_usart2_t;
MB_PARAM mb_usart3_t;

/**
  * @brief  Initialize software receive states for all application UARTs.
  * @retval None
  */
void AppUart_InitContexts(void)
{
	mbslave_init(&mb_usart1_t);
	mbslave_init(&mb_usart2_t);
	mbslave_init(&mb_usart3_t);
}

/**
  * @brief  Clamp an interrupt-reported UART frame length to the local buffer size.
  * @param[in] length Interrupt-reported byte count.
  * @retval Safe byte count for MB_BUF_SIZE buffers.
  */
int AppUart_LimitFrameLength(int length)
{
	if(length <= 0)
	{
		return 0;
	}
	if(length > MB_BUF_SIZE)
	{
		return MB_BUF_SIZE;
	}
	return length;
}

/**
  * @brief  Copy a completed UART frame into an application buffer.
  * @param[out] dst Destination buffer.
  * @param[in] src Source UART receive buffer.
  * @param[in] length Received byte count.
  * @retval Copied byte count after bounds checking.
  */
int AppUart_CopyRxData(u8 *dst, const u8 *src, int length)
{
	length = AppUart_LimitFrameLength(length);
	if(0 == length)
	{
		return 0;
	}
	memcpy(dst, src, length);
	return length;
}
