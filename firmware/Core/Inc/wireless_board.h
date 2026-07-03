#ifndef __WIRELESS_BOARD_H
#define __WIRELESS_BOARD_H

#include "main.h"

#define WIRELESS_ZIGBEE_FACTORY_BAUDRATE 38400U
#define WIRELESS_ZIGBEE_RUNTIME_BAUDRATE 19200U
#define WIRELESS_VM101_BAUDRATE          9600U
#define WIRELESS_STOP_WAKE_SETTLE_US     800U

/**
  * @brief  进入 Stop 模式前配置 GPIO、UART 和时钟。
  * @retval 无
  */
void Wireless_PrepareStopMode(void);

/**
  * @brief  Stop 模式退出后恢复 GPIO 和唤醒中断配置。
  * @retval 无
  */
void Wireless_RestoreAfterStopMode(void);

/**
  * @brief  配置无线模块模式引脚和状态输出。
  * @retval 无
  */
void Wireless_M0M1ModeConfig(void);

/**
  * @brief  启动检查阶段打开 Zigbee、传感器电源和 VM101 后级电源。
  * @retval 无
  */
void Wireless_PowerOnStartupModules(void);

/**
  * @brief  运行阶段打开 Zigbee、传感器电源和 VM101 后级电源。
  * @retval 无
  */
void Wireless_PowerOnRuntimeModules(void);

/**
  * @brief  关闭 Zigbee、传感器电源和 VM101 后级电源。
  * @retval 无
  */
void Wireless_PowerOffModules(void);

/**
  * @brief  初始化或校验 Zigbee 模块；失败时停留在故障闪灯状态。
  * @retval 无
  */
void Wireless_InitZigbeeOrFault(void);

/**
  * @brief  检查 E330 遥控模块是否能正常响应参数读取。
  * @retval 1 表示模块返回预期参数，0 表示未返回预期参数。
  */
u8 Wireless_CheckRemoteModule(void);

/**
  * @brief  初始化运行测量阶段使用的 GPIO 和 UART。
  * @param[in] remoteModuleOk 1 表示遥控模块可用，0 表示不可用。
  * @retval 无
  */
void Wireless_StartRuntimeUarts(u8 remoteModuleOk);

#endif
