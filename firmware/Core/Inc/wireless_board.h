#ifndef __WIRELESS_BOARD_H
#define __WIRELESS_BOARD_H

#include "main.h"

#define WIRELESS_ZIGBEE_FACTORY_BAUDRATE 38400U
#define WIRELESS_ZIGBEE_RUNTIME_BAUDRATE 19200U
#define WIRELESS_VM101_BAUDRATE          9600U
#define WIRELESS_STOP_WAKE_SETTLE_US     800U

/**
  * @brief  Configure GPIO, UART and clock settings before entering Stop mode.
  * @retval None
  */
void Wireless_PrepareStopMode(void);

/**
  * @brief  Restore GPIO and wake-up interrupt configuration after Stop mode.
  * @retval None
  */
void Wireless_RestoreAfterStopMode(void);

/**
  * @brief  Configure the wireless module mode pins and status outputs.
  * @retval None
  */
void Wireless_M0M1ModeConfig(void);

/**
  * @brief  Power on Zigbee, sensor power and VM101 back-end rails for startup checks.
  * @retval None
  */
void Wireless_PowerOnStartupModules(void);

/**
  * @brief  Power on Zigbee, sensor power and VM101 back-end rails for runtime.
  * @retval None
  */
void Wireless_PowerOnRuntimeModules(void);

/**
  * @brief  Power off Zigbee, sensor power and VM101 back-end rails.
  * @retval None
  */
void Wireless_PowerOffModules(void);

/**
  * @brief  Initialize or verify the Zigbee module; stays in fault blink on failure.
  * @retval None
  */
void Wireless_InitZigbeeOrFault(void);

/**
  * @brief  Check whether the E330 remote module can respond to parameter reads.
  * @retval 1 if the module responds with expected parameters, otherwise 0.
  */
u8 Wireless_CheckRemoteModule(void);

/**
  * @brief  Initialize GPIO and UARTs used by the runtime measurement state.
  * @param[in] remoteModuleOk 1 if the remote module is available, otherwise 0.
  * @retval None
  */
void Wireless_StartRuntimeUarts(u8 remoteModuleOk);

#endif
