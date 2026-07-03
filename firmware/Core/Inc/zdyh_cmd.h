#ifndef __ZDYH_CMD_H
#define __ZDYH_CMD_H

#include "main.h"
#include "ch8_measure.h"

typedef enum
{
	ZDYH_NOT_MATCH = 0,
	ZDYH_HANDLED,
	ZDYH_CONTINUE
} ZDYH_RESULT;

/**
  * @brief  处理一帧 40 字节 ZDYH 协议命令。
  * @param[in] rxBuffer 接收到的串口缓冲区。
  * @param[in,out] txBuffer 响应缓冲区。
  * @param[in,out] testValue CH8 测量缓冲区。
  * @param[in,out] bcState 广播响应状态。
  * @param[in,out] bcResp 广播通道响应掩码。
  * @param[in,out] data32Bak 上一次广播响应 ID。
  * @param[in,out] lastCmd 上一次接收的有效命令码。
  * @param[in,out] lastMsg 上一次接收的有效命令负载。
  * @param[in,out] errorCode 当前测量或协议错误码。
  * @param[in,out] measureFlag 延迟测量请求标志。
  * @retval ZDYH_NOT_MATCH 表示不是 ZDYH 帧，其他值表示处理状态。
  */
ZDYH_RESULT ZDYH_Process(u8 *rxBuffer, u8 *txBuffer, int *testValue,
	u8 *bcState, u8 *bcResp, u32 *data32Bak,
	uint8_t *lastCmd, uint8_t *lastMsg,
	u8 *errorCode, u8 *measureFlag);

#endif
