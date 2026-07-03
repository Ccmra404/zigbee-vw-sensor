#include "hy65_ascii_cmd.h"
#include "app_config.h"
#include "hy65_protocol.h"

/**
  * @brief  清除 Flash 和 RAM 中所有通道的参考值。
  * @retval 无
  */
static void HY65_AsciiClearReferenceData(void)
{
	int i = 0;

	for(i = 0;i < CH8_CHANNEL_COUNT;i++)
	{
		RefferenceData[i] = 0;
		SaveData(APP_CONFIG_ADDR_REF_DATA + APP_CONFIG_REF_DATA_STRIDE_BYTES * i, 0);
	}
}

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
HY65_ASCII_RESULT HY65_AsciiProcess(u8 *rxBuffer, u8 *txBuffer, int msgLen, int *testValue, u8 *errorCode, u8 *measureFlag)
{
	char nptr[7] = {0,0,0,0,0,0,0};
	unsigned long checksum = 0;
	u32 value = 0;
	int i = 0;
	u8 channelIndex = 0;

	if(rxBuffer[0] != '6'
		|| rxBuffer[1] != '5'
		|| rxBuffer[2] != 'T'
		|| rxBuffer[3] != 'X')
	{
		return HY65_ASCII_NOT_MATCH;
	}

	if(rxBuffer[7] == 'N' && rxBuffer[8] == '0')
	{
		HY65_Init65RxBuffer(txBuffer, NodeNumber);
		checksum = 23;
		for(i = 4;i < 9;i++)
		{
			checksum = checksum + rxBuffer[i];
		}
		if(rxBuffer[9] != (u8)checksum)
		{
			return HY65_ASCII_HANDLED;
		}
		value = (rxBuffer[4] - 0x30) * 100 + (rxBuffer[5] - 0x30) * 10 + rxBuffer[6] - 0x30;
		groupID = value;
		__disable_irq();
		SaveData(APP_CONFIG_ADDR_GROUP_ID, groupID);
		__enable_irq();
		NodeNumber[0] = rxBuffer[4];
		NodeNumber[1] = rxBuffer[5];
		NodeNumber[2] = rxBuffer[6];
		txBuffer[4] = NodeNumber[0];
		txBuffer[5] = NodeNumber[1];
		txBuffer[6] = NodeNumber[2];
		HY65_SendRespInt(txBuffer, 0);
		return HY65_ASCII_CONTINUE;
	}

	value = (rxBuffer[4] - 0x30) * 100 + (rxBuffer[5] - 0x30) * 10 + rxBuffer[6] - 0x30;
	if(value < groupID || value >= groupID + CH8_CHANNEL_COUNT)
	{
		return HY65_ASCII_HANDLED;
	}

	channelIndex = (u8)(value - groupID);
	NodeNumber[0] = rxBuffer[4];
	NodeNumber[1] = rxBuffer[5];
	NodeNumber[2] = rxBuffer[6];
	HY65_Init65RxBuffer(txBuffer, NodeNumber);
	checksum = 23;
	for(i = 4;i < msgLen - 1;i++)
	{
		checksum = checksum + rxBuffer[i];
	}
	if(rxBuffer[msgLen - 1] != (u8)checksum)
	{
		return HY65_ASCII_HANDLED;
	}

	switch(rxBuffer[7])
	{
		case 'S':
			txBuffer[7] = 'S';
			if('0' == rxBuffer[8])
			{
				*errorCode = CH8_MeasureChannel(testValue, RefferenceData, channelIndex, workMode, kindex.fkvalue, fCorrect);
				*measureFlag = 0;
				if(0 == *errorCode)
				{
					HY65_SendRespInt(txBuffer, CH8_ResultForMode(testValue, channelIndex, workMode));
				}
				else
				{
					txBuffer[8] = 'X';
					HY65_SendRespInt(txBuffer, *errorCode);
					*errorCode = 0;
				}
			}
			break;
		case 'T':
			txBuffer[7] = 'T';
			if('0' == rxBuffer[8])
			{
				*errorCode = CH8_MeasureChannel(testValue, RefferenceData, channelIndex, workMode, kindex.fkvalue, fCorrect);
				*measureFlag = 0;
				if(0 == *errorCode)
				{
					HY65_SendRespInt(txBuffer, testValue[CH8_TEMP_BASE + channelIndex]);
				}
				else
				{
					txBuffer[8] = 'X';
					HY65_SendRespInt(txBuffer, *errorCode);
					*errorCode = 0;
				}
			}
			break;
		case 'Z':
			if('0' == rxBuffer[8])
			{
				if(0 == workMode)
				{
					*errorCode = CH8_MeasureChannel(testValue, RefferenceData, channelIndex, workMode, kindex.fkvalue, fCorrect);
					*measureFlag = 0;
					if(0 != *errorCode)
					{
						break;
					}
					RefferenceData[channelIndex] = testValue[CH8_FREQ_BASE + channelIndex];
					__disable_irq();
					SaveData(APP_CONFIG_ADDR_REF_DATA + APP_CONFIG_REF_DATA_STRIDE_BYTES * channelIndex, RefferenceData[channelIndex]);
					__enable_irq();
				}
			}
			else if('1' == rxBuffer[8])
			{
				if(0 == workMode)
				{
					RefferenceData[channelIndex] = 0;
					__disable_irq();
					SaveData(APP_CONFIG_ADDR_REF_DATA + APP_CONFIG_REF_DATA_STRIDE_BYTES * channelIndex, 0);
					__enable_irq();
				}
			}
			break;
		case 'B':
			break;
		case 'C':
			if('r' == rxBuffer[8])
			{
				HY65_SendRespInt(txBuffer, AppConfig_ReadWord(APP_CONFIG_ADDR_CORRECT));
			}
			else if('w' == rxBuffer[8])
			{
				for(i = 0;i < 6;i++)
				{
					nptr[i] = rxBuffer[i + 9];
				}
				Correct = atoi(nptr);
				__disable_irq();
				SaveData(APP_CONFIG_ADDR_CORRECT, Correct);
				HY65_AsciiClearReferenceData();
				__enable_irq();
				txBuffer[7] = 'C';
				HY65_SendRespInt(txBuffer, AppConfig_ReadWord(APP_CONFIG_ADDR_CORRECT));
			}
			break;
		case 'F':
			if('r' == rxBuffer[8])
			{
				HY65_SendRespFloat(txBuffer, AppConfig_ReadWord(APP_CONFIG_ADDR_CENTER_FREQ));
			}
			else if('w' == rxBuffer[8])
			{
				for(i = 0;i < 6;i++)
				{
					nptr[i] = rxBuffer[i + 9];
				}
				CenterFre = atoi(nptr);
				__disable_irq();
				SaveData(APP_CONFIG_ADDR_CENTER_FREQ, CenterFre);
				HY65_AsciiClearReferenceData();
				__enable_irq();
				txBuffer[7] = 'F';
				HY65_SendRespFloat(txBuffer, AppConfig_ReadWord(APP_CONFIG_ADDR_CENTER_FREQ));
			}
			break;
		case 'K':
			if('r' == rxBuffer[8])
			{
				HY65_SendRespFloat(txBuffer, kindex.fkvalue);
			}
			else if('w' == rxBuffer[8])
			{
				for(i = 0;i < 7;i++)
				{
					nptr[i] = rxBuffer[i + 9];
				}
				sscanf(nptr, "%f", &kindex.fkvalue);
				__disable_irq();
				SaveData(APP_CONFIG_ADDR_K_VALUE, kindex.kvalue);
				HY65_AsciiClearReferenceData();
				__enable_irq();
				txBuffer[7] = 'K';
				HY65_SendRespFloat(txBuffer, kindex.fkvalue);
			}
			break;
		case 'M':
			if('r' == rxBuffer[8])
			{
				HY65_SendRespInt(txBuffer, AppConfig_ReadWord(APP_CONFIG_ADDR_WORK_MODE));
			}
			else if('w' == rxBuffer[8])
			{
				if('0' == rxBuffer[9])
				{
					workMode = 0;
				}
				else if('1' == rxBuffer[9])
				{
					workMode = 1;
				}
				else if('2' == rxBuffer[9])
				{
					workMode = 2;
				}
				__disable_irq();
				SaveData(APP_CONFIG_ADDR_WORK_MODE, workMode);
				__enable_irq();
				txBuffer[7] = 'M';
				HY65_SendRespInt(txBuffer, AppConfig_ReadWord(APP_CONFIG_ADDR_WORK_MODE));
			}
			break;
		default:
			break;
	}

	return HY65_ASCII_HANDLED;
}

