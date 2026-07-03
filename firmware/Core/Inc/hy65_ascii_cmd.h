#ifndef __HY65_ASCII_CMD_H
#define __HY65_ASCII_CMD_H

#include "main.h"
#include "ch8_measure.h"

typedef enum
{
	HY65_ASCII_NOT_MATCH = 0,
	HY65_ASCII_HANDLED,
	HY65_ASCII_CONTINUE
} HY65_ASCII_RESULT;

/**
  * @brief  处理一帧 HY65 ASCII 65TX 命令。
  * @param[in] rxBuffer 接收到的串口缓冲区。
  * @param[in,out] txBuffer 响应缓冲区。
  * @param[in] msgLen 接收到的帧长度。
  * @param[in,out] testValue CH8 测量缓冲区。
  * @param[in,out] errorCode 当前测量或协议错误码。
  * @param[in,out] measureFlag 延迟测量请求标志。
  * @retval HY65_ASCII_NOT_MATCH 表示不是 65TX 帧，其他值表示处理状态。
  */
HY65_ASCII_RESULT HY65_AsciiProcess(u8 *rxBuffer, u8 *txBuffer, int msgLen, int *testValue, u8 *errorCode, u8 *measureFlag);

#endif
