#ifndef __HY65_PROTOCOL_H
#define __HY65_PROTOCOL_H

#include "main.h"

/**
  * @brief  Calculate the HY65 40-byte protocol XOR checksum.
  * @param[in] buffer Data buffer to check.
  * @param[in] size Number of bytes to include.
  * @retval Calculated 32-bit checksum value.
  */
unsigned long HY65_CheckSum40B(unsigned char *buffer, unsigned short size);

/**
  * @brief  Initialize a 40-byte ZDYH transmit frame template.
  * @param[out] txBuffer Destination frame buffer, at least 40 bytes.
  * @retval None
  */
void HY65_InitZdyhTxBuffer(u8 *txBuffer);

/**
  * @brief  Initialize a 65RX response frame template.
  * @param[out] txBuffer Destination frame buffer, at least 18 bytes.
  * @param[in] nodeNumber Three ASCII digits of the node number.
  * @retval None
  */
void HY65_Init65RxBuffer(u8 *txBuffer, const u8 *nodeNumber);

/**
  * @brief  Fill ID and baud-rate fields in a ZDYH frame.
  * @param[in,out] txBuffer ZDYH frame buffer.
  * @param[in] data32 Device/channel ID value.
  * @param[in] groupId Current group ID.
  * @param[in] currentChannelNumber Base channel ID of this node.
  * @param[in] uartBaudRate UART baud rate to write into the frame.
  * @retval None
  */
void HY65_AddIdBaudrateInfo(u8 *txBuffer, u32 data32, u32 groupId, u32 currentChannelNumber, u32 uartBaudRate);

/**
  * @brief  Add the HY65 checksum fields to a ZDYH frame.
  * @param[in,out] txBuffer ZDYH frame buffer.
  * @retval None
  */
void HY65_AddCheckSum(u8 *txBuffer);

/**
  * @brief  Send a signed integer measurement in a 65RX response frame.
  * @param[in,out] txBuffer 65RX response frame buffer.
  * @param[in] measurement Measurement value to format as ASCII.
  * @retval None
  */
void HY65_SendRespInt(u8 *txBuffer, int measurement);

/**
  * @brief  Send a floating-point measurement in a 65RX response frame.
  * @param[in,out] txBuffer 65RX response frame buffer.
  * @param[in] measurement Measurement value to format as ASCII.
  * @retval None
  */
void HY65_SendRespFloat(u8 *txBuffer, float measurement);

#endif
