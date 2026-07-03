#include "wireless_board.h"
#include "app_config.h"
#include "app_delay.h"
#include "app_uart.h"
#include "gpio.h"
#include "usart.h"

static u8 DRF1609CfgMsg[42] = {0xFC,0x27,0x07,0x02,0x65,0x01,0x14,
								0x01,0x66,0x77,0xAA,0xBB,0x05,0x01,
								0x01,0x01,0x05,0xA6,0x01,0x02,0x65,
								0x01,0x14,0x01,0x66,0x77,0xCC,0xDD,
								0x06,0x01,0x01,0x01,0x05,0xA6,0x00,
								0x00,0x02,0x11,0x12,0x13,0x14,0xA0};

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

static void Wireless_FaultBlinkForever(u32 intervalMs)
{
	(void)intervalMs;
	while(1)
	{
		__NOP();
	}
}

void Wireless_InitZigbeeOrFault(void)
{
	u8 ackOk = 0;
	int i = 0;
	u32 tmrcnt = 0;

	if(0 == ZigbeeCfgFlag)
	{
		MX_USART1_UART_Init(WIRELESS_ZIGBEE_FACTORY_BAUDRATE);
		HAL_UART_Receive_IT(&huart1, mb_usart1_t.rx_buf, MB_BUF_SIZE);
		Usart_Printf_Len(&huart1, DRF1609CfgMsg, 42);
		tmrcnt = uwTick;
		while(1)
		{
			if(mb_usart1_t.rx_end_flg == 1)
			{
				ackOk = Wireless_IsZigbeeConfigAck(mb_usart1_t.rx_buf, MsgFlag1);
				mb_usart1_t.rx_end_flg = 0;
				MsgFlag1 = 0;
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
				MX_USART1_UART_Init(WIRELESS_ZIGBEE_RUNTIME_BAUDRATE);
				HAL_UART_Receive_IT(&huart1, mb_usart1_t.rx_buf, MB_BUF_SIZE);
				Usart_Printf_Len(&huart1, DRF1609CfgMsg, 42);
				MsgFlag1 = 0;
				while(1)
				{
					if(mb_usart1_t.rx_end_flg == 1)
					{
						ackOk = Wireless_IsZigbeeConfigAck(mb_usart1_t.rx_buf, MsgFlag1);
						mb_usart1_t.rx_end_flg = 0;
						MsgFlag1 = 0;
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
		MX_USART1_UART_Init(WIRELESS_ZIGBEE_RUNTIME_BAUDRATE);
		HAL_UART_Receive_IT(&huart1, mb_usart1_t.rx_buf, MB_BUF_SIZE);
		delay_ms(10);
		Usart_Printf_Len(&huart1, DRF1609CfgMsg, 42);
		MsgFlag1 = 0;
		while(1)
		{
			if(mb_usart1_t.rx_end_flg == 1)
			{
				ackOk = Wireless_IsZigbeeConfigAck(mb_usart1_t.rx_buf, MsgFlag1);
				mb_usart1_t.rx_end_flg = 0;
				MsgFlag1 = 0;
				if(ackOk)
				{
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

void Wireless_StartRuntimeUarts(void)
{
	MX_GPIO_Init();
	delay_ms(10);
	Wireless_InitZigbeeOrFault();

	MX_USART1_UART_Init(WIRELESS_ZIGBEE_RUNTIME_BAUDRATE);
	HAL_UART_Receive_IT(&huart1, mb_usart1_t.rx_buf, MB_BUF_SIZE);
	mb_usart1_t.rx_end_flg = 0;

	MX_USART2_UART_Init(WIRELESS_VM101_BAUDRATE);
	HAL_UART_Receive_IT(&huart2, mb_usart2_t.rx_buf, MB_BUF_SIZE);
	mb_usart2_t.rx_end_flg = 0;
}
