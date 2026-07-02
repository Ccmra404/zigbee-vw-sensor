#ifndef __APP_CONFIG_H
#define __APP_CONFIG_H

#include "main.h"
#include "ch8_measure.h"

#define APP_CONFIG_FLASH_FLAG             0xA5U
#define APP_CONFIG_DEFAULT_BAUDRATE       19200U
#define APP_CONFIG_DEFAULT_ID             0x253FF601U
#define APP_CONFIG_DEFAULT_GROUP_ID       1U
#define APP_CONFIG_DEFAULT_WORK_MODE      2U
#define APP_CONFIG_DEFAULT_CENTER_FREQ    800U
#define APP_CONFIG_DEFAULT_K_VALUE        3.7f
#define APP_CONFIG_CORRECT_DIVISOR        1000000U
#define APP_CONFIG_REF_DATA_STRIDE_BYTES  4U

#define APP_CONFIG_ADDR_BASE              0x08080000U
#define APP_CONFIG_ADDR_VALID_FLAG        (APP_CONFIG_ADDR_BASE + 0x00U)
#define APP_CONFIG_ADDR_DEVICE_ID         (APP_CONFIG_ADDR_BASE + 0x04U)
#define APP_CONFIG_ADDR_GROUP_ID          (APP_CONFIG_ADDR_BASE + 0x08U)
#define APP_CONFIG_ADDR_CORRECT           (APP_CONFIG_ADDR_BASE + 0x0CU)
#define APP_CONFIG_ADDR_WORK_MODE         (APP_CONFIG_ADDR_BASE + 0x14U)
#define APP_CONFIG_ADDR_CENTER_FREQ       (APP_CONFIG_ADDR_BASE + 0x18U)
#define APP_CONFIG_ADDR_ZIGBEE_CFG        (APP_CONFIG_ADDR_BASE + 0x1CU)
#define APP_CONFIG_ADDR_K_VALUE           (APP_CONFIG_ADDR_BASE + 0x20U)
#define APP_CONFIG_ADDR_REF_DATA          (APP_CONFIG_ADDR_BASE + 0x24U)

typedef union
{
	float fkvalue;
	u32 kvalue;
} APP_FLOAT_WORD;

extern u32 CurrentChannelNumber;
extern u32 groupID;
extern u32 Correct;
extern u32 UART2_BaudRate;
extern u8 NodeNumber[3];
extern int RefferenceData[CH8_CHANNEL_COUNT];
extern float fCorrect;
extern u32 ZigbeeCfgFlag;
extern u16 CenterFre;
extern u8 workMode;
extern APP_FLOAT_WORD kindex;

/**
  * @brief  Load persistent application parameters, or initialize defaults.
  * @retval None
  */
void AppConfig_Load(void);

/**
  * @brief  Read one 32-bit word from the persistent configuration area.
  * @param[in] addr Flash data EEPROM address to read.
  * @retval Stored 32-bit value.
  */
uint32_t AppConfig_ReadWord(uint32_t addr);

/**
  * @brief  Write one 32-bit data word to the configured flash data area.
  * @param[in] addr Flash address to program.
  * @param[in] data 32-bit value to write.
  * @retval None
  */
void SaveData(uint32_t addr, uint32_t data);

#endif
