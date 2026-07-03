#include "app_uart.h"

__IO char MsgFlag1;
__IO char MsgFlag2;
__IO char MsgFlag3;

MB_PARAM mb_usart1_t;
MB_PARAM mb_usart2_t;
MB_PARAM mb_usart3_t;

/**
  * @brief  初始化所有应用串口的软件接收状态。
  * @retval 无
  */
void AppUart_InitContexts(void)
{
	mbslave_init(&mb_usart1_t);
	mbslave_init(&mb_usart2_t);
	mbslave_init(&mb_usart3_t);
}

/**
  * @brief  将中断上报的串口帧长度限制在本地缓冲区范围内。
  * @param[in] length 中断上报的字节数。
  * @retval 适用于 MB_BUF_SIZE 缓冲区的安全字节数。
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
  * @brief  将已接收完成的串口帧复制到应用缓冲区。
  * @param[out] dst 目标缓冲区。
  * @param[in] src 源串口接收缓冲区。
  * @param[in] length 接收到的字节数。
  * @retval 边界检查后的实际复制字节数。
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
