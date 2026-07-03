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
  * @brief  从持久化配置区读取一个 32 位数据。
  * @param[in] addr 待读取的 Flash Data EEPROM 地址。
  * @retval 读取到的 32 位数据。
  */
uint32_t AppConfig_ReadWord(uint32_t addr)
{
	return *(uint32_t *)addr;
}

/**
  * @brief  向配置区写入一个 32 位数据。
  * @param[in] addr 待写入的 Flash 地址。
  * @param[in] data 待写入的 32 位数据。
  * @retval 无
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
  * @brief  将当前组号转换并写入 3 字节 ASCII 节点号。
  * @retval 无
  */
static void AppConfig_UpdateNodeNumber(void)
{
	NodeNumber[0] = groupID / 100 + 0x30;
	NodeNumber[1] = groupID % 100 / 10 + 0x30;
	NodeNumber[2] = groupID % 10 + 0x30;
}

/**
  * @brief  读取应用持久化参数；无有效参数时初始化默认值。
  * @retval 无
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
