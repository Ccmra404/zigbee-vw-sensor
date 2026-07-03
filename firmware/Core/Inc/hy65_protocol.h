#ifndef __HY65_PROTOCOL_H
#define __HY65_PROTOCOL_H

#include "main.h"

/**
  * @brief  计算 HY65 40 字节协议使用的异或校验值。
  * @param[in] buffer 待校验数据缓冲区。
  * @param[in] size 参与校验的字节数。
  * @retval 计算得到的 32 位校验值。
  */
unsigned long HY65_CheckSum40B(unsigned char *buffer, unsigned short size);

/**
  * @brief  初始化 40 字节 ZDYH 发送帧模板。
  * @param[out] txBuffer 目标帧缓冲区，至少 40 字节。
  * @retval 无
  */
void HY65_InitZdyhTxBuffer(u8 *txBuffer);

/**
  * @brief  初始化 65RX 响应帧模板。
  * @param[out] txBuffer 目标帧缓冲区，至少 18 字节。
  * @param[in] nodeNumber 节点号的 3 个 ASCII 数字。
  * @retval 无
  */
void HY65_Init65RxBuffer(u8 *txBuffer, const u8 *nodeNumber);

/**
  * @brief  填充 ZDYH 帧中的 ID 和波特率字段。
  * @param[in,out] txBuffer ZDYH 帧缓冲区。
  * @param[in] data32 设备或通道 ID。
  * @param[in] groupId 当前组号。
  * @param[in] currentChannelNumber 本节点起始通道 ID。
  * @param[in] uartBaudRate 写入帧中的串口波特率。
  * @retval 无
  */
void HY65_AddIdBaudrateInfo(u8 *txBuffer, u32 data32, u32 groupId, u32 currentChannelNumber, u32 uartBaudRate);

/**
  * @brief  为 ZDYH 帧写入 HY65 校验字段。
  * @param[in,out] txBuffer ZDYH 帧缓冲区。
  * @retval 无
  */
void HY65_AddCheckSum(u8 *txBuffer);

/**
  * @brief  使用 65RX 响应帧发送有符号整数测量值。
  * @param[in,out] txBuffer 65RX 响应帧缓冲区。
  * @param[in] measurement 待格式化为 ASCII 的测量值。
  * @retval 无
  */
void HY65_SendRespInt(u8 *txBuffer, int measurement);

/**
  * @brief  使用 65RX 响应帧发送浮点测量值。
  * @param[in,out] txBuffer 65RX 响应帧缓冲区。
  * @param[in] measurement 待格式化为 ASCII 的测量值。
  * @retval 无
  */
void HY65_SendRespFloat(u8 *txBuffer, float measurement);

#endif
