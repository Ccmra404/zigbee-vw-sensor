#include "app_config.h"

u32 CurrentChannelNumber = APP_CONFIG_DEFAULT_ID;
u32 groupID = APP_CONFIG_DEFAULT_GROUP_ID;
u32 Correct = 0;
u32 UART2_BaudRate = APP_CONFIG_DEFAULT_BAUDRATE;
u8 NodeNumber[3] = {'0','0','1'};
int RefferenceData[CH8_CHANNEL_COUNT];
float fCorrect = 1.0f;
u32 ZigbeeCfgFlag = 0;
u16 CenterFre = APP_CONFIG_DEFAULT_CENTER_FREQ;
u8 workMode = APP_CONFIG_DEFAULT_WORK_MODE;
APP_FLOAT_WORD kindex;

/**
  * @brief  Read one 32-bit word from the persistent configuration area.
  * @param[in] addr Flash data EEPROM address to read.
  * @retval Stored 32-bit value.
  */
uint32_t AppConfig_ReadWord(uint32_t addr)
{
	return *(uint32_t *)addr;
}

/**
  * @brief  Write one 32-bit data word to the configured flash data area.
  * @param[in] addr Flash address to program.
  * @param[in] data 32-bit value to write.
  * @retval None
  */
void SaveData(uint32_t addr, uint32_t data)
{
	int i = 0;

	HAL_FLASH_Unlock();
	for(i = 0;i < 2;i++)
	{
		if(HAL_FLASH_Program(FLASH_TYPEPROGRAMDATA_WORD, addr, data) != HAL_OK)
		{
			HAL_FLASH_Program(FLASH_TYPEPROGRAMDATA_WORD, addr, data);
		}
	}
	HAL_FLASH_Lock();
}

/**
  * @brief  Copy the current group ID into the three ASCII node number bytes.
  * @retval None
  */
static void AppConfig_UpdateNodeNumber(void)
{
	NodeNumber[0] = groupID / 100 + 0x30;
	NodeNumber[1] = groupID % 100 / 10 + 0x30;
	NodeNumber[2] = groupID % 10 + 0x30;
}

/**
  * @brief  Load persistent application parameters, or initialize defaults.
  * @retval None
  */
void AppConfig_Load(void)
{
	int i = 0;
	u32 data32 = AppConfig_ReadWord(APP_CONFIG_ADDR_VALID_FLAG);

	if(APP_CONFIG_FLASH_FLAG == (uint8_t)data32)
	{
		CurrentChannelNumber = AppConfig_ReadWord(APP_CONFIG_ADDR_DEVICE_ID);
		groupID = AppConfig_ReadWord(APP_CONFIG_ADDR_GROUP_ID);
		Correct = AppConfig_ReadWord(APP_CONFIG_ADDR_CORRECT);
		ZigbeeCfgFlag = AppConfig_ReadWord(APP_CONFIG_ADDR_ZIGBEE_CFG);
		workMode = AppConfig_ReadWord(APP_CONFIG_ADDR_WORK_MODE);
		CenterFre = AppConfig_ReadWord(APP_CONFIG_ADDR_CENTER_FREQ);
		kindex.kvalue = AppConfig_ReadWord(APP_CONFIG_ADDR_K_VALUE);
		if(0 == AppConfig_ReadWord(APP_CONFIG_ADDR_CENTER_FREQ))
		{
			CenterFre = APP_CONFIG_DEFAULT_CENTER_FREQ;
		}
		if(0 == AppConfig_ReadWord(APP_CONFIG_ADDR_K_VALUE))
		{
			kindex.fkvalue = APP_CONFIG_DEFAULT_K_VALUE;
		}
		for(i = 0;i < CH8_CHANNEL_COUNT;i++)
		{
			RefferenceData[i] = AppConfig_ReadWord(APP_CONFIG_ADDR_REF_DATA + APP_CONFIG_REF_DATA_STRIDE_BYTES * i);
		}
		AppConfig_UpdateNodeNumber();
	}
	else
	{
		__disable_irq();
		SaveData(APP_CONFIG_ADDR_VALID_FLAG, APP_CONFIG_FLASH_FLAG);
		SaveData(APP_CONFIG_ADDR_DEVICE_ID, APP_CONFIG_DEFAULT_ID);
		SaveData(APP_CONFIG_ADDR_GROUP_ID, APP_CONFIG_DEFAULT_GROUP_ID);
		SaveData(APP_CONFIG_ADDR_CORRECT, 0);
		SaveData(APP_CONFIG_ADDR_WORK_MODE, APP_CONFIG_DEFAULT_WORK_MODE);
		SaveData(APP_CONFIG_ADDR_ZIGBEE_CFG, 0);
		SaveData(APP_CONFIG_ADDR_CENTER_FREQ, APP_CONFIG_DEFAULT_CENTER_FREQ);
		kindex.fkvalue = APP_CONFIG_DEFAULT_K_VALUE;
		SaveData(APP_CONFIG_ADDR_K_VALUE, kindex.kvalue);
		for(i = 0;i < CH8_CHANNEL_COUNT;i++)
		{
			SaveData(APP_CONFIG_ADDR_REF_DATA + i * APP_CONFIG_REF_DATA_STRIDE_BYTES, 0);
		}
		__enable_irq();

		CurrentChannelNumber = APP_CONFIG_DEFAULT_ID;
		groupID = APP_CONFIG_DEFAULT_GROUP_ID;
		CenterFre = APP_CONFIG_DEFAULT_CENTER_FREQ;
		for(i = 0;i < CH8_CHANNEL_COUNT;i++)
		{
			RefferenceData[i] = CenterFre * 10;
		}
		Correct = 0;
		ZigbeeCfgFlag = 0;
		NodeNumber[0] = '0';
		NodeNumber[1] = '0';
		NodeNumber[2] = '1';
		workMode = APP_CONFIG_DEFAULT_WORK_MODE;
	}

	fCorrect = Correct / APP_CONFIG_CORRECT_DIVISOR + 1;
}
