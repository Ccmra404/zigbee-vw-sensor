#include "wireless_board.h"
#include "app_controller.h"
#include "app_config.h"
#include "app_delay.h"
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
  * @brief  Check whether the latest Zigbee response matches the config ACK.
  * @retval 1 if the response is valid, otherwise 0.
  */
static u8 Wireless_IsZigbeeConfigAck(void)
{
	if(RxBuffer2[0] == 0xFA
		&& RxBuffer2[1] == 0x01
		&& RxBuffer2[2] == 0x0A
		&& RxBuffer2[3] == 0x07
		&& RxBuffer2[4] == 0x0C)
	{
		return 1;
	}
	return 0;
}

/**
  * @brief  Blink the status LED forever after a fatal wireless-module error.
  * @param[in] intervalMs LED on/off interval in milliseconds.
  * @retval None
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
  * @brief  Configure GPIO, UART and clock settings before entering Stop mode.
  * @retval None
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
  * @brief  Restore GPIO and wake-up interrupt configuration after Stop mode.
  * @retval None
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
  * @brief  Configure the wireless module mode pins and status outputs.
  * @retval None
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
  * @brief  Power on Zigbee, sensor power and VM101 back-end rails for startup checks.
  * @retval None
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
  * @brief  Power on Zigbee, sensor power and VM101 back-end rails for runtime.
  * @retval None
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
  * @brief  Power off Zigbee, sensor power and VM101 back-end rails.
  * @retval None
  */
void Wireless_PowerOffModules(void)
{
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
}

/**
  * @brief  Initialize or verify the Zigbee module; stays in fault blink on failure.
  * @retval None
  */
void Wireless_InitZigbeeOrFault(void)
{
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
				for(i = 0;i < MsgFlag2;i++)
				{
					RxBuffer2[i] = mb_usart2_t.rx_buf[i];
				}
				mb_usart2_t.rx_end_flg = 0;
				if(Wireless_IsZigbeeConfigAck())
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
						for(i = 0;i < MsgFlag2;i++)
						{
							RxBuffer2[i] = mb_usart2_t.rx_buf[i];
						}
						mb_usart2_t.rx_end_flg = 0;
						if(Wireless_IsZigbeeConfigAck())
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
				for(i = 0;i < MsgFlag2;i++)
				{
					RxBuffer2[i] = mb_usart2_t.rx_buf[i];
				}
				mb_usart2_t.rx_end_flg = 0;
				if(Wireless_IsZigbeeConfigAck())
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
  * @brief  Check whether the E330 remote module can respond to parameter reads.
  * @retval 1 if the module responds with expected parameters, otherwise 0.
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
  * @brief  Initialize GPIO and UARTs used by the runtime measurement state.
  * @param[in] remoteModuleOk 1 if the remote module is available, otherwise 0.
  * @retval None
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

