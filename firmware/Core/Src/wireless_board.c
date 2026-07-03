#include "wireless_board.h"
#include "app_config.h"
#include "app_delay.h"
#include "app_uart.h"
#include "gpio.h"
#include "usart.h"

static u8 E330ModuleCfg[6] = {0xC0,0,0,0x18,0x2E,0x14};
static u8 DRF1609CfgMsg[42] = {0xFC,0x27,0x07,0x02,0x65,0x01,0x14,
								0x01,0x66,0x77,0xAA,0xBB,0x05,0x01,
								0x01,0x01,0x05,0xA6,0x01,0x02,0x65,
								0x01,0x14,0x01,0x66,0x77,0xCC,0xDD,
								0x06,0x01,0x01,0x01,0x05,0xA6,0x00,
								0x00,0x02,0x11,0x12,0x13,0x14,0xA0};

/**
  * @brief  判断最新 Zigbee 响应是否为配置确认帧。
  * @param[in] rxBuffer Zigbee 接收缓冲区。
  * @param[in] length 接收到的字节数。
  * @retval 1 表示响应有效，0 表示响应无效。
  */
static u8 Wireless_IsZigbeeConfigAck(const u8 *rxBuffer, int length)
{
	if(length >= 5
		&& rxBuffer[0] == 0xFA
		&& rxBuffer[1] == 0x01
		&& rxBuffer[2] == 0x0A
		&& rxBuffer[3] == 0x07
		&& rxBuffer[4] == 0x0C)
	{
		return 1;
	}
	return 0;
}

/**
  * @brief  无线模块严重错误后持续闪烁状态灯。
  * @param[in] intervalMs LED 亮灭间隔，单位为毫秒。
  * @retval 无
  */
static void Wireless_FaultBlinkForever(u32 intervalMs)
{
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
	while(1)
	{
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);
		delay_ms(intervalMs);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET);
		delay_ms(intervalMs);
	}
}

/**
  * @brief  进入 Stop 模式前配置 GPIO、UART 和时钟。
  * @retval 无
  */
void Wireless_PrepareStopMode(void)
{
	GPIO_InitTypeDef GPIO_Initure;

	HAL_UART_DeInit(&huart1);
	HAL_UART_DeInit(&huart2);
	HAL_UART_DeInit(&hlpuart1);
	HAL_PWREx_EnableUltraLowPower();
	HAL_PWREx_EnableFastWakeUp();
	__HAL_RCC_WAKEUPSTOP_CLK_CONFIG(RCC_STOP_WAKEUPCLOCK_MSI);

	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();

	GPIO_Initure.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_7
					| GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13
					| GPIO_PIN_14 | GPIO_PIN_15;
	GPIO_Initure.Mode = GPIO_MODE_ANALOG;
	GPIO_Initure.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOA, &GPIO_Initure);
	GPIO_Initure.Pin = GPIO_PIN_All;
	GPIO_Initure.Mode = GPIO_MODE_ANALOG;
	GPIO_Initure.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOB, &GPIO_Initure);
	HAL_GPIO_Init(GPIOC, &GPIO_Initure);

	__HAL_RCC_GPIOA_CLK_DISABLE();
	__HAL_RCC_GPIOB_CLK_DISABLE();
	__HAL_RCC_GPIOC_CLK_DISABLE();
}

/**
  * @brief  Stop 模式退出后恢复 GPIO 和唤醒中断配置。
  * @retval 无
  */
void Wireless_RestoreAfterStopMode(void)
{
	GPIO_InitTypeDef GPIO_Initure = {0};

	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);
	GPIO_Initure.Pin = GPIO_PIN_5 | GPIO_PIN_6;
	GPIO_Initure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_Initure.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOA, &GPIO_Initure);
	HAL_Delay(10);
	GPIO_Initure.Pin = GPIO_PIN_15;
	GPIO_Initure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_Initure.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOB, &GPIO_Initure);
	GPIO_Initure.Pin = GPIO_PIN_11;
	GPIO_Initure.Mode = GPIO_MODE_IT_RISING_FALLING;
	GPIO_Initure.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(GPIOB, &GPIO_Initure);

	HAL_NVIC_SetPriority(EXTI4_15_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);
}

/**
  * @brief  配置无线模块模式引脚和状态输出。
  * @retval 无
  */
void Wireless_M0M1ModeConfig(void)
{
	GPIO_InitTypeDef GPIO_Initure = {0};

	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);
	GPIO_Initure.Pin = GPIO_PIN_5 | GPIO_PIN_6;
	GPIO_Initure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_Initure.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOA, &GPIO_Initure);

	GPIO_Initure.Pin = GPIO_PIN_13;
	GPIO_Initure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_Initure.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOB, &GPIO_Initure);
	GPIO_Initure.Pin = GPIO_PIN_15;
	GPIO_Initure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_Initure.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOB, &GPIO_Initure);
}

/**
  * @brief  启动检查阶段打开 Zigbee、传感器电源和 VM101 后级电源。
  * @retval 无
  */
void Wireless_PowerOnStartupModules(void)
{
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
	delay_ms(200);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
	delay_ms(200);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
	delay_ms(1000);
}

/**
  * @brief  运行阶段打开 Zigbee、传感器电源和 VM101 后级电源。
  * @retval 无
  */
void Wireless_PowerOnRuntimeModules(void)
{
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
	delay_ms(200);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
	delay_ms(200);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
	delay_ms(200);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);
	delay_ms(200);
}

/**
  * @brief  关闭 Zigbee、传感器电源和 VM101 后级电源。
  * @retval 无
  */
void Wireless_PowerOffModules(void)
{
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
}

/**
  * @brief  初始化或校验 Zigbee 模块；失败时停留在故障闪灯状态。
  * @retval 无
  */
void Wireless_InitZigbeeOrFault(void)
{
	u8 ackOk = 0;
	int i = 0;
	u32 tmrcnt = 0;

	if(0 == ZigbeeCfgFlag)
	{
		MX_USART2_UART_Init(WIRELESS_ZIGBEE_FACTORY_BAUDRATE);
		HAL_UART_Receive_IT(&huart2, mb_usart2_t.rx_buf, MB_BUF_SIZE);
		Usart_Printf_Len(&huart2, DRF1609CfgMsg, 42);
		tmrcnt = uwTick;
		while(1)
		{
			if(mb_usart2_t.rx_end_flg == 1)
			{
				ackOk = Wireless_IsZigbeeConfigAck(mb_usart2_t.rx_buf, MsgFlag2);
				mb_usart2_t.rx_end_flg = 0;
				MsgFlag2 = 0;
				if(ackOk)
				{
					ZigbeeCfgFlag = 1;
					__disable_irq();
					SaveData(APP_CONFIG_ADDR_ZIGBEE_CFG, 1);
					__enable_irq();
					break;
				}
			}

			if(uwTick - tmrcnt >= 500)
			{
				tmrcnt = uwTick;
				MX_USART2_UART_Init(WIRELESS_ZIGBEE_RUNTIME_BAUDRATE);
				HAL_UART_Receive_IT(&huart2, mb_usart2_t.rx_buf, MB_BUF_SIZE);
				Usart_Printf_Len(&huart2, DRF1609CfgMsg, 42);
				MsgFlag2 = 0;
				while(1)
				{
					if(mb_usart2_t.rx_end_flg == 1)
					{
						ackOk = Wireless_IsZigbeeConfigAck(mb_usart2_t.rx_buf, MsgFlag2);
						mb_usart2_t.rx_end_flg = 0;
						MsgFlag2 = 0;
						if(ackOk)
						{
							ZigbeeCfgFlag = 1;
							__disable_irq();
							SaveData(APP_CONFIG_ADDR_ZIGBEE_CFG, 1);
							__enable_irq();
							break;
						}
					}
					if(uwTick - tmrcnt >= 500)
					{
						break;
					}
				}

				if(0 == ZigbeeCfgFlag)
				{
					Wireless_FaultBlinkForever(200);
				}
				else
				{
					break;
				}
			}
		}
	}
	else
	{
		tmrcnt = uwTick;
		MX_USART2_UART_Init(WIRELESS_ZIGBEE_RUNTIME_BAUDRATE);
		HAL_UART_Receive_IT(&huart2, mb_usart2_t.rx_buf, MB_BUF_SIZE);
		delay_ms(10);
		Usart_Printf_Len(&huart2, DRF1609CfgMsg, 42);
		MsgFlag2 = 0;
		while(1)
		{
			if(mb_usart2_t.rx_end_flg == 1)
			{
				ackOk = Wireless_IsZigbeeConfigAck(mb_usart2_t.rx_buf, MsgFlag2);
				mb_usart2_t.rx_end_flg = 0;
				MsgFlag2 = 0;
				if(ackOk)
				{
					for(i = 0;i < 3;i++)
					{
						HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);
						delay_ms(100);
						HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET);
						delay_ms(100);
					}
					break;
				}
			}

			if(uwTick - tmrcnt >= 500)
			{
				ZigbeeCfgFlag = 0;
				__disable_irq();
				SaveData(APP_CONFIG_ADDR_ZIGBEE_CFG, ZigbeeCfgFlag);
				__enable_irq();
				Wireless_FaultBlinkForever(500);
			}
		}
	}
}

/**
  * @brief  检查 E330 遥控模块是否能正常响应参数读取。
  * @retval 1 表示模块返回预期参数，0 表示未返回预期参数。
  */
u8 Wireless_CheckRemoteModule(void)
{
	u8 readParamCmd[3] = {0xC1,0xC1,0xC1};
	int i = 0;
	int k = 0;
	u32 tmrcnt = 0;

	Wireless_M0M1ModeConfig();
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
	delay_ms(10);

	MX_LPUART1_UART_Init();
	HAL_UART_Receive_IT(&hlpuart1, mb_usart3_t.rx_buf, MB_BUF_SIZE);
	mb_usart3_t.rx_end_flg = 0;

	Usart_Printf_Len(&hlpuart1, readParamCmd, 3);

	tmrcnt = uwTick;
	i = 0;
	while(1)
	{
		if(mb_usart3_t.rx_end_flg == 1)
		{
			mb_usart3_t.rx_end_flg = 0;
			for(k = 1;k < 6;k++)
			{
				if(mb_usart3_t.rx_buf[k] != E330ModuleCfg[k])
				{
					break;
				}
			}
			if(k == 6)
			{
				return 1;
			}
			Usart_Printf_Len(&hlpuart1, E330ModuleCfg, 6);
		}

		if(uwTick - tmrcnt >= 500)
		{
			i++;
			if(i >= 10)
			{
				return 0;
			}
			tmrcnt = uwTick;
			Usart_Printf_Len(&hlpuart1, readParamCmd, 3);
		}
	}
}

/**
  * @brief  初始化运行测量阶段使用的 GPIO 和 UART。
  * @param[in] remoteModuleOk 1 表示遥控模块可用，0 表示不可用。
  * @retval 无
  */
void Wireless_StartRuntimeUarts(u8 remoteModuleOk)
{
	MX_GPIO_Init();
	delay_ms(10);
	Wireless_PowerOnRuntimeModules();

	if(remoteModuleOk == 0)
	{
		Wireless_M0M1ModeConfig();
		MX_LPUART1_UART_Init();
		HAL_UART_Receive_IT(&hlpuart1, mb_usart3_t.rx_buf, MB_BUF_SIZE);
		mb_usart3_t.rx_end_flg = 0;
	}

	MX_USART1_UART_Init(WIRELESS_VM101_BAUDRATE);
	HAL_UART_Receive_IT(&huart1, mb_usart1_t.rx_buf, MB_BUF_SIZE);
	mb_usart1_t.rx_end_flg = 0;

	MX_USART2_UART_Init(WIRELESS_ZIGBEE_RUNTIME_BAUDRATE);
	HAL_UART_Receive_IT(&huart2, mb_usart2_t.rx_buf, MB_BUF_SIZE);
	mb_usart2_t.rx_end_flg = 0;
}

