#include "zdyh_cmd.h"
#include "app_config.h"
#include "app_delay.h"
#include "hy65_protocol.h"
#include "usart.h"

/**
  * @brief  Send a prepared 40-byte ZDYH response frame.
  * @param[in,out] txBuffer ZDYH response frame buffer.
  * @retval None
  */
static void ZDYH_SendFrame(u8 *txBuffer)
{
	HY65_AddCheckSum(txBuffer);
	delay_ms(20);
	Usart_Printf_Len(&huart2, txBuffer, 40);
}

/**
  * @brief  Store a 32-bit signed value into four little-endian response bytes.
  * @param[in,out] buffer Destination byte buffer.
  * @param[in] value Value to store.
  * @retval None
  */
static void ZDYH_WriteInt32(u8 *buffer, int value)
{
	buffer[0] = (unsigned char)(value);
	buffer[1] = (unsigned char)(value >> 8);
	buffer[2] = (unsigned char)(value >> 16);
	buffer[3] = (unsigned char)(value >> 24);
}

/**
  * @brief  Process one 40-byte ZDYH protocol command frame.
  * @param[in] rxBuffer Received UART buffer.
  * @param[in,out] txBuffer Response buffer.
  * @param[in,out] testValue CH8 measurement buffer.
  * @param[in,out] bcState Broadcast response state.
  * @param[in,out] bcResp Broadcast channel response mask.
  * @param[in,out] data32Bak Previous broadcast response ID.
  * @param[in,out] lastCmd Last accepted command code.
  * @param[in,out] lastMsg Last accepted command payload.
  * @param[in,out] errorCode Current measurement/protocol error code.
  * @param[in,out] measureFlag Deferred measurement request flag.
  * @retval ZDYH_NOT_MATCH if the frame is not ZDYH; otherwise handling status.
  */
ZDYH_RESULT ZDYH_Process(u8 *rxBuffer, u8 *txBuffer, int *testValue,
	u8 *bcState, u8 *bcResp, u32 *data32Bak,
	uint8_t *lastCmd, uint8_t *lastMsg,
	u8 *errorCode, u8 *measureFlag)
{
	unsigned long checksum = 0;
	u32 data32 = 0;
	u32 tmp = 0;
	u8 channelIndex = 0;
	int i = 0;

	if(rxBuffer[0] != 'Z'
		|| rxBuffer[1] != 'D'
		|| rxBuffer[2] != 'Y'
		|| rxBuffer[3] != 'H')
	{
		return ZDYH_NOT_MATCH;
	}

	checksum = HY65_CheckSum40B(rxBuffer + 10, 10);
	checksum ^= HY65_CheckSum40B(rxBuffer + 20, 20);
	if(checksum != (rxBuffer[6]
		| (u32)rxBuffer[7] << 8
		| (u32)rxBuffer[8] << 16
		| (u32)rxBuffer[9] << 24))
	{
		return ZDYH_HANDLED;
	}

	data32 = rxBuffer[12] | (u32)rxBuffer[13] << 8 | (u32)rxBuffer[14] << 16 | (u32)rxBuffer[15] << 24;
	if(data32 != 0
		&& 0 == CH8_IsLocalChannelId(CurrentChannelNumber, data32)
		&& rxBuffer[21] != 0x06)
	{
		return ZDYH_HANDLED;
	}

	if(0 == data32)
	{
		*bcState = 1;
		*bcResp = 0;
	}

	if(rxBuffer[21] == 0x06 && 1 == *bcState)
	{
		if(*bcResp == CH8_RESPONSE_MASK)
		{
			return ZDYH_CONTINUE;
		}
		if(0 == data32)
		{
			data32 = CurrentChannelNumber;
		}
		else if(CH8_IsLocalChannelId(CurrentChannelNumber, data32))
		{
			channelIndex = CH8_ChannelIndexFromId(CurrentChannelNumber, data32);
			*bcResp = *bcResp | (1U << channelIndex);
			if(*bcResp == CH8_RESPONSE_MASK)
			{
				return ZDYH_CONTINUE;
			}
			data32 = CurrentChannelNumber + CH8_NextPendingChannelIndex(*bcResp);
		}
		else
		{
			if(*data32Bak == data32)
			{
				*data32Bak = data32;
				data32 = CurrentChannelNumber + CH8_NextPendingChannelIndex(*bcResp);
				delay_ms((u8)CurrentChannelNumber);
			}
			else
			{
				*data32Bak = data32;
				delay_ms(20);
				return ZDYH_CONTINUE;
			}
		}
	}
	else if(rxBuffer[21] == 0x86)
	{
		*bcState = 0;
	}

	if(0 == data32 && ((rxBuffer[21] & 0x7F) != 0x06))
	{
		data32 = CurrentChannelNumber;
	}

	HY65_InitZdyhTxBuffer(txBuffer);
	txBuffer[22] = rxBuffer[22];
	txBuffer[23] = rxBuffer[23];
	txBuffer[24] = rxBuffer[24];
	txBuffer[25] = rxBuffer[25];
	HY65_AddIdBaudrateInfo(txBuffer, data32, groupID, CurrentChannelNumber, UART2_BaudRate);

	if(((rxBuffer[21] & 0x7F) != 0x06) && ((rxBuffer[21] & 0x7F) != 0x00))
	{
		*lastCmd = rxBuffer[21];
		for(i = 0;i < 20;i++)
		{
			lastMsg[i] = rxBuffer[i + 20];
		}
	}

	switch(rxBuffer[21] & 0x7F)
	{
		case 0x06:
			txBuffer[21] = 0x86;
			txBuffer[32] = 0x0A;
			txBuffer[33] = 0x3F;
			txBuffer[34] = 0xFF;
			txBuffer[35] = 0x7F;
			txBuffer[36] = 0;
			txBuffer[37] = 0;
			txBuffer[38] = 0;
			txBuffer[39] = 0;

			if(CH8_IsLocalChannelId(CurrentChannelNumber, data32))
			{
				channelIndex = CH8_ChannelIndexFromId(CurrentChannelNumber, data32);
				*errorCode = CH8_MeasureChannel(testValue, RefferenceData, channelIndex, workMode, kindex.fkvalue, fCorrect);
				*measureFlag = 0;
			}
			else
			{
				*errorCode = 1;
			}

			if(0 == *errorCode)
			{
				testValue[CH8_RESULT_SCRATCH] = CH8_ResultForMode(testValue, channelIndex, workMode);
				ZDYH_WriteInt32(&txBuffer[36], testValue[CH8_RESULT_SCRATCH]);
				txBuffer[34] = (unsigned char)testValue[CH8_TEMP_BASE + channelIndex];
				txBuffer[35] = (unsigned char)(testValue[CH8_TEMP_BASE + channelIndex] >> 8);
			}
			txBuffer[19] = *errorCode;
			ZDYH_SendFrame(txBuffer);
			break;
		case 0x00:
			if(0x80 == *lastCmd)
			{
				for(i = 22;i < 40;i++)
				{
					txBuffer[i] = 0;
				}
				txBuffer[21] = 0x80;
				txBuffer[19] = 0;
				txBuffer[32] = 0x00;
				txBuffer[33] = 0x1F;
			}
			else
			{
				for(i = 0;i < 20;i++)
				{
					txBuffer[i + 20] = lastMsg[i];
				}
				txBuffer[19] = 0;
				txBuffer[32] = 0x00;
				txBuffer[33] = 0x1F;
			}
			ZDYH_SendFrame(txBuffer);
			break;
		case 0x01:
			break;
		case 0x02:
			__disable_irq();
			SaveData(APP_CONFIG_ADDR_GROUP_ID, groupID);
			__enable_irq();
			break;
		case 0x03:
			break;
		case 0x04:
			tmp = rxBuffer[26] | (u32)rxBuffer[27] << 8;
			channelIndex = CH8_ChannelIndexFromId(CurrentChannelNumber, data32);
			if(0 == tmp && 0 == workMode)
			{
				*errorCode = CH8_MeasureChannel(testValue, RefferenceData, channelIndex, workMode, kindex.fkvalue, fCorrect);
				*measureFlag = 0;
				if(0 != *errorCode)
				{
					break;
				}
			}
			switch(tmp)
			{
				case 0:
					if(0 == workMode)
					{
						RefferenceData[channelIndex] = testValue[CH8_FREQ_BASE + channelIndex];
					}
					break;
				case 1:
					testValue[CH8_RESULT_SCRATCH] = rxBuffer[28] | (u32)rxBuffer[29] << 8 | (u32)rxBuffer[30] << 16 | (u32)rxBuffer[31] << 24;
					if(0 == workMode)
					{
						RefferenceData[channelIndex] += testValue[CH8_RESULT_SCRATCH];
					}
					break;
				case 2:
					testValue[CH8_RESULT_SCRATCH] = rxBuffer[28] | (u32)rxBuffer[29] << 8 | (u32)rxBuffer[30] << 16 | (u32)rxBuffer[31] << 24;
					if(0 == workMode)
					{
						RefferenceData[channelIndex] -= testValue[CH8_RESULT_SCRATCH];
					}
					break;
				case 3:
					if(0 == workMode)
					{
						RefferenceData[channelIndex] = rxBuffer[28] | (u32)rxBuffer[29] << 8 | (u32)rxBuffer[30] << 16 | (u32)rxBuffer[31] << 24;
					}
					break;
				case 4:
					break;
				case 5:
					if(0 == workMode)
					{
						RefferenceData[channelIndex] = CenterFre * 10;
					}
					break;
				default:
					break;
			}
			__disable_irq();
			SaveData(APP_CONFIG_ADDR_REF_DATA + APP_CONFIG_REF_DATA_STRIDE_BYTES * channelIndex, RefferenceData[channelIndex]);
			__enable_irq();
			break;
		case 0x05:
			groupID = rxBuffer[26];
			break;
		case 0x07:
			break;
		case 0x08:
			break;
		case 0x09:
			break;
		case 0x0A:
			CurrentChannelNumber = rxBuffer[26] | (u32)rxBuffer[27] << 8 | (u32)rxBuffer[28] << 16 | (u32)rxBuffer[29] << 24;
			__disable_irq();
			SaveData(APP_CONFIG_ADDR_DEVICE_ID, CurrentChannelNumber);
			__enable_irq();
			break;
		default:
			break;
	}

	return ZDYH_HANDLED;
}

