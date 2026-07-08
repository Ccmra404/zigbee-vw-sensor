#include "wireless_board.h"
#include "app_config.h"
#include "app_delay.h"
#include "app_uart.h"
#include "gpio.h"
#include "usart.h"

#define WIRELESS_ZIGBEE_CFG_TIMEOUT_MS      500U
#define WIRELESS_ZIGBEE_BAUD_SWITCH_GAP_MS  10U

#define WIRELESS_STATUS_IDLE                 0U
#define WIRELESS_STATUS_TRY_FACTORY_BAUD     1U
#define WIRELESS_STATUS_TRY_RUNTIME_BAUD     2U
#define WIRELESS_STATUS_ACK_OK               3U
#define WIRELESS_STATUS_FAULT                4U
#define WIRELESS_STATUS_DIAG_RUNNING         5U
#define WIRELESS_STATUS_DIAG_DONE            6U
#define WIRELESS_STATUS_TRANSPARENT_TEST     7U
#define WIRELESS_STATUS_PASSIVE_RX_TEST      8U
#define WIRELESS_STATUS_PA10_PIN_TEST        9U

static u8 DRF1609CfgMsg[42] = {0xFC,0x27,0x07,0x02,0x65,0x01,0x14,
								0x01,0x66,0x77,0xAA,0xBB,0x05,0x01,
								0x01,0x01,0x05,0xA6,0x01,0x02,0x65,
								0x01,0x14,0x01,0x66,0x77,0xCC,0xDD,
								0x06,0x01,0x01,0x01,0x05,0xA6,0x00,
								0x00,0x02,0x11,0x12,0x13,0x14,0xA0};

volatile u32 ZigbeeDebugLastBaud = 0;
volatile int ZigbeeDebugLastLen = 0;
volatile u8 ZigbeeDebugLastRx[WIRELESS_ZIGBEE_DEBUG_RX_SIZE] = {0};
volatile int ZigbeeDebugFactoryLen = 0;
volatile int ZigbeeDebugFactoryFrameCount = 0;
volatile u8 ZigbeeDebugFactoryRx[WIRELESS_ZIGBEE_DEBUG_RX_SIZE] = {0};
volatile u8 ZigbeeDebugCaptureFactoryEnabled = 0;
volatile u32 ZigbeeDebugAttemptCount = 0;
volatile u32 ZigbeeDebugLastStatus = WIRELESS_STATUS_IDLE;
volatile u32 ZigbeeDiagMagic = 0;
volatile u32 ZigbeeDiagCycle = 0;
volatile u32 ZigbeeDiagSlotCount = 0;
volatile u32 ZigbeeDiagHitMaskLow = 0;
volatile u32 ZigbeeDiagHitMaskHigh = 0;
volatile u32 ZigbeeDiagLastSlot = 0;
volatile u32 ZigbeeDiagBaud[WIRELESS_ZIGBEE_DIAG_SLOT_COUNT] = {0};
volatile u8 ZigbeeDiagFormat[WIRELESS_ZIGBEE_DIAG_SLOT_COUNT] = {0};
volatile u8 ZigbeeDiagCmd[WIRELESS_ZIGBEE_DIAG_SLOT_COUNT] = {0};
volatile u8 ZigbeeDiagLen[WIRELESS_ZIGBEE_DIAG_SLOT_COUNT] = {0};
volatile u8 ZigbeeDiagAck[WIRELESS_ZIGBEE_DIAG_SLOT_COUNT] = {0};
volatile u32 ZigbeeDiagError[WIRELESS_ZIGBEE_DIAG_SLOT_COUNT] = {0};
volatile u8 ZigbeeDiagRx[WIRELESS_ZIGBEE_DIAG_SLOT_COUNT][WIRELESS_ZIGBEE_DIAG_RX_SIZE] = {{0}};
volatile u32 ZigbeeTransparentMagic = 0;
volatile u32 ZigbeeTransparentBaud = 0;
volatile u32 ZigbeeTransparentTxCount = 0;
volatile u32 ZigbeeTransparentRxCount = 0;
volatile u32 ZigbeeTransparentLastLen = 0;
volatile u8 ZigbeeTransparentLastRx[WIRELESS_ZIGBEE_TRANSPARENT_RX_SIZE] = {0};
volatile u32 ZigbeePinMagic = 0;
volatile u32 ZigbeePinSampleCount = 0;
volatile u32 ZigbeePinHighCount = 0;
volatile u32 ZigbeePinLowCount = 0;
volatile u32 ZigbeePinEdgeCount = 0;
volatile u32 ZigbeePinFirstBits = 0;
volatile u32 ZigbeePinLastLevel = 0;

#if WIRELESS_ZIGBEE_DIAG_MODE
typedef struct
{
	u8 id;
	u8 responseId;
	const u8 *data;
	u8 length;
} WIRELESS_ZIGBEE_DIAG_CMD;

typedef struct
{
	u32 parity;
	u8 id;
} WIRELESS_ZIGBEE_DIAG_FORMAT;

static const u32 ZigbeeDiagBaudList[] =
{
	1200U, 2400U, 4800U, 9600U,
	19200U, 38400U, 57600U, 115200U
};
static const WIRELESS_ZIGBEE_DIAG_FORMAT ZigbeeDiagFormatList[] =
{
	{UART_PARITY_NONE, 0U},
	{UART_PARITY_EVEN, 1U},
	{UART_PARITY_ODD,  2U}
};
static const u8 ZigbeeDiagIns01[] = {0xFC,0x06,0x04,0x44,0x54,0x4B,0x52,0x46,0x81};
static const u8 ZigbeeDiagIns05[] = {0xFC,0x06,0x0E,0x44,0x54,0x4B,0x52,0x46,0x8B};
static const WIRELESS_ZIGBEE_DIAG_CMD ZigbeeDiagCmdList[] =
{
	{1U, 0x04U, ZigbeeDiagIns01, (u8)sizeof(ZigbeeDiagIns01)},
	{5U, 0x0EU, ZigbeeDiagIns05, (u8)sizeof(ZigbeeDiagIns05)}
};
#endif

/**
  * @brief  清空出厂波特率调试缓存。
  * @retval 无
  */
static void Wireless_ClearZigbeeFactoryDebugFrame(void)
{
	int i = 0;

	ZigbeeDebugFactoryLen = 0;
	ZigbeeDebugFactoryFrameCount = 0;
	for(i = 0;i < (int)WIRELESS_ZIGBEE_DEBUG_RX_SIZE;i++)
	{
		ZigbeeDebugFactoryRx[i] = 0;
	}
}

/**
  * @brief  保存最近一次 Zigbee 配置响应，便于调试时查看真实返回帧。
  * @param[in] baud 当前尝试通信的波特率。
  * @param[in] rxBuffer 串口接收缓冲区。
  * @param[in] length 本次接收到的字节数。
  * @retval 无
  */
static void Wireless_SaveZigbeeDebugFrame(u32 baud, const u8 *rxBuffer, int length)
{
	int i = 0;
	int copyLength = length;
	int maxLength = (int)WIRELESS_ZIGBEE_DEBUG_RX_SIZE;

	if(copyLength < 0)
	{
		copyLength = 0;
	}
	if(copyLength > maxLength)
	{
		copyLength = maxLength;
	}

	ZigbeeDebugLastBaud = baud;
	ZigbeeDebugLastLen = copyLength;
	for(i = 0;i < copyLength;i++)
	{
		ZigbeeDebugLastRx[i] = rxBuffer[i];
	}
	for(;i < maxLength;i++)
	{
		ZigbeeDebugLastRx[i] = 0;
	}

	if(baud == WIRELESS_ZIGBEE_FACTORY_BAUDRATE && 0 == ZigbeeDebugCaptureFactoryEnabled)
	{
		ZigbeeDebugFactoryLen = 0;
		ZigbeeDebugFactoryFrameCount = 1;
		for(i = 0;i < copyLength;i++)
		{
			ZigbeeDebugFactoryRx[i] = rxBuffer[i];
		}
		for(;i < maxLength;i++)
		{
			ZigbeeDebugFactoryRx[i] = 0;
		}
	}
}

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

static void Wireless_ClearZigbeeRxContext(void)
{
	memset(mb_usart1_t.rx_buf, 0, MB_BUF_SIZE);
	mb_usart1_t.rx_end_flg = 0;
	MsgFlag1 = 0;
}

static u8 Wireless_SendZigbeeConfigAtBaud(u32 baud)
{
	u32 startTick = 0;
	u8 ackOk = 0;

	ZigbeeDebugAttemptCount++;
	if(baud == WIRELESS_ZIGBEE_FACTORY_BAUDRATE)
	{
		ZigbeeDebugLastStatus = WIRELESS_STATUS_TRY_FACTORY_BAUD;
	}
	else
	{
		ZigbeeDebugLastStatus = WIRELESS_STATUS_TRY_RUNTIME_BAUD;
	}

	MX_USART1_UART_Init(baud);
	Wireless_ClearZigbeeRxContext();
	HAL_UART_Receive_IT(&huart1, mb_usart1_t.rx_buf, MB_BUF_SIZE);
	delay_ms(WIRELESS_ZIGBEE_BAUD_SWITCH_GAP_MS);
	Usart_Printf_Len(&huart1, DRF1609CfgMsg, (uint16_t)sizeof(DRF1609CfgMsg));

	startTick = uwTick;
	while(uwTick - startTick < WIRELESS_ZIGBEE_CFG_TIMEOUT_MS)
	{
		if(mb_usart1_t.rx_end_flg == 1)
		{
			Wireless_SaveZigbeeDebugFrame(baud, mb_usart1_t.rx_buf, MsgFlag1);
			ackOk = Wireless_IsZigbeeConfigAck(mb_usart1_t.rx_buf, MsgFlag1);
			Wireless_ClearZigbeeRxContext();
			if(ackOk)
			{
				ZigbeeDebugLastStatus = WIRELESS_STATUS_ACK_OK;
				return 1;
			}
		}
	}

	return 0;
}

#if WIRELESS_ZIGBEE_DIAG_MODE
static u8 Wireless_IsDiagKnownAck(const u8 *rxBuffer, int length, u8 responseId)
{
	if(length >= 4
		&& rxBuffer[0] == 0xFA
		&& rxBuffer[2] == 0x0A
		&& rxBuffer[3] == responseId)
	{
		return 1;
	}
	return 0;
}

static void Wireless_ClearDiagSlots(void)
{
	int slot = 0;
	int index = 0;

	ZigbeeDiagMagic = 0x44524631UL;
	ZigbeeDiagCycle = 0;
	ZigbeeDiagSlotCount = 0;
	ZigbeeDiagHitMaskLow = 0;
	ZigbeeDiagHitMaskHigh = 0;
	ZigbeeDiagLastSlot = 0;
	for(slot = 0;slot < (int)WIRELESS_ZIGBEE_DIAG_SLOT_COUNT;slot++)
	{
		ZigbeeDiagBaud[slot] = 0;
		ZigbeeDiagFormat[slot] = 0;
		ZigbeeDiagCmd[slot] = 0;
		ZigbeeDiagLen[slot] = 0;
		ZigbeeDiagAck[slot] = 0;
		ZigbeeDiagError[slot] = 0;
		for(index = 0;index < (int)WIRELESS_ZIGBEE_DIAG_RX_SIZE;index++)
		{
			ZigbeeDiagRx[slot][index] = 0;
		}
	}
}

static void Wireless_SaveDiagSlot(int slot, u32 baud, u8 formatId,
	u8 cmdId, u8 responseId, const u8 *rxBuffer, int length, u32 errorCode)
{
	int i = 0;
	int copyLength = length;

	if(slot < 0 || slot >= (int)WIRELESS_ZIGBEE_DIAG_SLOT_COUNT)
	{
		return;
	}
	if(copyLength < 0)
	{
		copyLength = 0;
	}
	if(copyLength > (int)WIRELESS_ZIGBEE_DIAG_RX_SIZE)
	{
		copyLength = (int)WIRELESS_ZIGBEE_DIAG_RX_SIZE;
	}

	ZigbeeDiagBaud[slot] = baud;
	ZigbeeDiagFormat[slot] = formatId;
	ZigbeeDiagCmd[slot] = cmdId;
	ZigbeeDiagLen[slot] = (u8)copyLength;
	ZigbeeDiagAck[slot] = Wireless_IsDiagKnownAck(rxBuffer, length, responseId);
	ZigbeeDiagError[slot] = errorCode;
	for(i = 0;i < copyLength;i++)
	{
		ZigbeeDiagRx[slot][i] = rxBuffer[i];
	}
	for(;i < (int)WIRELESS_ZIGBEE_DIAG_RX_SIZE;i++)
	{
		ZigbeeDiagRx[slot][i] = 0;
	}
	if(copyLength > 0)
	{
		if(slot < 32)
		{
			ZigbeeDiagHitMaskLow |= (1UL << slot);
		}
		else
		{
			ZigbeeDiagHitMaskHigh |= (1UL << (slot - 32));
		}
	}
	ZigbeeDiagLastSlot = (u32)slot;
}

static void Wireless_RunZigbeeDiagForever(void)
{
	int baudIndex = 0;
	int formatIndex = 0;
	int cmdIndex = 0;
	int slot = 0;
	u32 startTick = 0;
	int sawFrame = 0;

	Wireless_ClearDiagSlots();
	ZigbeeDebugLastStatus = WIRELESS_STATUS_DIAG_RUNNING;

	ZigbeeDiagCycle++;
	for(baudIndex = 0;baudIndex < (int)(sizeof(ZigbeeDiagBaudList) / sizeof(ZigbeeDiagBaudList[0]));baudIndex++)
	{
		for(formatIndex = 0;formatIndex < (int)(sizeof(ZigbeeDiagFormatList) / sizeof(ZigbeeDiagFormatList[0]));formatIndex++)
		{
			MX_USART1_UART_InitEx(ZigbeeDiagBaudList[baudIndex], ZigbeeDiagFormatList[formatIndex].parity);
			delay_ms(WIRELESS_ZIGBEE_BAUD_SWITCH_GAP_MS);
			for(cmdIndex = 0;cmdIndex < (int)(sizeof(ZigbeeDiagCmdList) / sizeof(ZigbeeDiagCmdList[0]));cmdIndex++)
			{
				if(slot >= (int)WIRELESS_ZIGBEE_DIAG_SLOT_COUNT)
				{
					break;
				}
				Wireless_ClearZigbeeRxContext();
				huart1.ErrorCode = HAL_UART_ERROR_NONE;
				HAL_UART_Receive_IT(&huart1, mb_usart1_t.rx_buf, MB_BUF_SIZE);
				delay_ms(WIRELESS_ZIGBEE_BAUD_SWITCH_GAP_MS);
				Usart_Printf_Len(&huart1, (u8 *)ZigbeeDiagCmdList[cmdIndex].data, ZigbeeDiagCmdList[cmdIndex].length);

				sawFrame = 0;
				startTick = uwTick;
				while(uwTick - startTick < WIRELESS_ZIGBEE_CFG_TIMEOUT_MS)
				{
					if(mb_usart1_t.rx_end_flg == 1)
					{
						Wireless_SaveDiagSlot(slot, ZigbeeDiagBaudList[baudIndex],
							ZigbeeDiagFormatList[formatIndex].id,
							ZigbeeDiagCmdList[cmdIndex].id,
							ZigbeeDiagCmdList[cmdIndex].responseId,
							mb_usart1_t.rx_buf, MsgFlag1, huart1.ErrorCode);
						Wireless_SaveZigbeeDebugFrame(ZigbeeDiagBaudList[baudIndex], mb_usart1_t.rx_buf, MsgFlag1);
						Wireless_ClearZigbeeRxContext();
						sawFrame = 1;
						break;
					}
				}
				if(0 == sawFrame)
				{
					Wireless_SaveDiagSlot(slot, ZigbeeDiagBaudList[baudIndex],
						ZigbeeDiagFormatList[formatIndex].id,
						ZigbeeDiagCmdList[cmdIndex].id,
						ZigbeeDiagCmdList[cmdIndex].responseId,
						mb_usart1_t.rx_buf, 0, huart1.ErrorCode);
				}
				slot++;
				ZigbeeDiagSlotCount = (u32)slot;
				delay_ms(100);
			}
		}
	}
	ZigbeeDebugLastStatus = WIRELESS_STATUS_DIAG_DONE;
	while(1)
	{
		__NOP();
	}
}
#endif

static void Wireless_FaultBlinkForever(u32 intervalMs)
{
	(void)intervalMs;
	ZigbeeDebugLastStatus = WIRELESS_STATUS_FAULT;
	while(1)
	{
		__NOP();
	}
}

static void Wireless_SaveTransparentRx(const u8 *rxBuffer, int length)
{
	int i = 0;
	int copyLength = length;

	if(copyLength < 0)
	{
		copyLength = 0;
	}
	if(copyLength > (int)WIRELESS_ZIGBEE_TRANSPARENT_RX_SIZE)
	{
		copyLength = (int)WIRELESS_ZIGBEE_TRANSPARENT_RX_SIZE;
	}
	ZigbeeTransparentLastLen = (u32)copyLength;
	for(i = 0;i < copyLength;i++)
	{
		ZigbeeTransparentLastRx[i] = rxBuffer[i];
	}
	for(;i < (int)WIRELESS_ZIGBEE_TRANSPARENT_RX_SIZE;i++)
	{
		ZigbeeTransparentLastRx[i] = 0;
	}
	if(copyLength > 0)
	{
		ZigbeeTransparentRxCount++;
		Wireless_SaveZigbeeDebugFrame(WIRELESS_ZIGBEE_FACTORY_BAUDRATE, rxBuffer, copyLength);
	}
}

#if WIRELESS_ZIGBEE_PASSIVE_RX_TEST
static void Wireless_RunPassiveRxTestForever(void)
{
	u32 startTick = 0;

	ZigbeeTransparentMagic = 0x52585041UL;
	ZigbeeTransparentBaud = WIRELESS_ZIGBEE_RUNTIME_BAUDRATE;
	ZigbeeTransparentTxCount = 0;
	ZigbeeTransparentRxCount = 0;
	ZigbeeTransparentLastLen = 0;
	ZigbeeDebugLastStatus = WIRELESS_STATUS_PASSIVE_RX_TEST;

	MX_USART1_UART_Init(WIRELESS_ZIGBEE_RUNTIME_BAUDRATE);
	Wireless_ClearZigbeeRxContext();
	HAL_UART_Receive_IT(&huart1, mb_usart1_t.rx_buf, MB_BUF_SIZE);

	startTick = uwTick;
	while(uwTick - startTick < 5000U)
	{
		if(mb_usart1_t.rx_end_flg == 1)
		{
			Wireless_SaveTransparentRx(mb_usart1_t.rx_buf, MsgFlag1);
			Wireless_SaveZigbeeDebugFrame(WIRELESS_ZIGBEE_RUNTIME_BAUDRATE, mb_usart1_t.rx_buf, MsgFlag1);
			Wireless_ClearZigbeeRxContext();
			HAL_UART_Receive_IT(&huart1, mb_usart1_t.rx_buf, MB_BUF_SIZE);
		}
	}
	ZigbeeDebugLastStatus = WIRELESS_STATUS_DIAG_DONE;
	while(1)
	{
		__NOP();
	}
}
#endif

#if WIRELESS_ZIGBEE_PA10_PIN_TEST
static void Wireless_RunPa10PinTestForever(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	u32 startTick = 0;
	u32 level = 0;
	u32 lastLevel = 0;
	u32 sampleIndex = 0;

	__HAL_RCC_GPIOA_CLK_ENABLE();
	GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	ZigbeePinMagic = 0x50413130UL;
	ZigbeePinSampleCount = 0;
	ZigbeePinHighCount = 0;
	ZigbeePinLowCount = 0;
	ZigbeePinEdgeCount = 0;
	ZigbeePinFirstBits = 0;
	ZigbeePinLastLevel = 0;
	ZigbeeDebugLastStatus = WIRELESS_STATUS_PA10_PIN_TEST;

	lastLevel = ((GPIOA->IDR & GPIO_PIN_10) != 0U) ? 1U : 0U;
	startTick = uwTick;
	while(uwTick - startTick < 5000U)
	{
		level = ((GPIOA->IDR & GPIO_PIN_10) != 0U) ? 1U : 0U;
		if(sampleIndex < 32U && level != 0U)
		{
			ZigbeePinFirstBits |= (1UL << sampleIndex);
		}
		if(level != 0U)
		{
			ZigbeePinHighCount++;
		}
		else
		{
			ZigbeePinLowCount++;
		}
		if(level != lastLevel)
		{
			ZigbeePinEdgeCount++;
			lastLevel = level;
		}
		sampleIndex++;
	}
	ZigbeePinSampleCount = sampleIndex;
	ZigbeePinLastLevel = lastLevel;
	ZigbeeDebugLastStatus = WIRELESS_STATUS_DIAG_DONE;
	while(1)
	{
		__NOP();
	}
}
#endif

#if WIRELESS_ZIGBEE_TRANSPARENT_TEST

static void Wireless_RunTransparentTestForever(void)
{
	static u8 testFrame[24] = {'S','T','M','3','2','-','D','R','F','1','6','0','9','H','-', '0','0','0','0','\r','\n'};
	u32 lastTxTick = 0;
	u32 count = 0;

	ZigbeeTransparentMagic = 0x5452414EUL;
	ZigbeeTransparentBaud = WIRELESS_ZIGBEE_FACTORY_BAUDRATE;
	ZigbeeTransparentTxCount = 0;
	ZigbeeTransparentRxCount = 0;
	ZigbeeTransparentLastLen = 0;
	ZigbeeDebugLastStatus = WIRELESS_STATUS_TRANSPARENT_TEST;

	MX_USART1_UART_Init(WIRELESS_ZIGBEE_FACTORY_BAUDRATE);
	Wireless_ClearZigbeeRxContext();
	HAL_UART_Receive_IT(&huart1, mb_usart1_t.rx_buf, MB_BUF_SIZE);

	while(1)
	{
		if(mb_usart1_t.rx_end_flg == 1)
		{
			Wireless_SaveTransparentRx(mb_usart1_t.rx_buf, MsgFlag1);
			Wireless_ClearZigbeeRxContext();
			HAL_UART_Receive_IT(&huart1, mb_usart1_t.rx_buf, MB_BUF_SIZE);
		}
		if(uwTick - lastTxTick >= 1000U)
		{
			lastTxTick = uwTick;
			count = ZigbeeTransparentTxCount;
			testFrame[15] = (u8)('0' + ((count / 1000U) % 10U));
			testFrame[16] = (u8)('0' + ((count / 100U) % 10U));
			testFrame[17] = (u8)('0' + ((count / 10U) % 10U));
			testFrame[18] = (u8)('0' + (count % 10U));
			Usart_Printf_Len(&huart1, testFrame, 21U);
			ZigbeeTransparentTxCount++;
		}
	}
}
#endif

void Wireless_InitZigbeeOrFault(void)
{
	u8 ackOk = 0;

	if(0 == ZigbeeCfgFlag)
	{
		Wireless_ClearZigbeeFactoryDebugFrame();
		ZigbeeDebugCaptureFactoryEnabled = 1;
		ackOk = Wireless_SendZigbeeConfigAtBaud(WIRELESS_ZIGBEE_FACTORY_BAUDRATE);
		ZigbeeDebugCaptureFactoryEnabled = 0;
		if(0 == ackOk)
		{
			ackOk = Wireless_SendZigbeeConfigAtBaud(WIRELESS_ZIGBEE_RUNTIME_BAUDRATE);
		}
	}
	else
	{
		ZigbeeDebugCaptureFactoryEnabled = 0;
		ackOk = Wireless_SendZigbeeConfigAtBaud(WIRELESS_ZIGBEE_RUNTIME_BAUDRATE);
		if(0 == ackOk)
		{
			ackOk = Wireless_SendZigbeeConfigAtBaud(WIRELESS_ZIGBEE_FACTORY_BAUDRATE);
		}
	}

	if(ackOk)
	{
		ZigbeeCfgFlag = 1;
		__disable_irq();
		SaveData(APP_CONFIG_ADDR_ZIGBEE_CFG, 1);
		__enable_irq();
	}
	else
	{
		ZigbeeCfgFlag = 0;
		__disable_irq();
		SaveData(APP_CONFIG_ADDR_ZIGBEE_CFG, ZigbeeCfgFlag);
		__enable_irq();
		Wireless_FaultBlinkForever(500);
	}
}

void Wireless_StartRuntimeUarts(void)
{
	MX_GPIO_Init();
	delay_ms(2000);
#if WIRELESS_ZIGBEE_PA10_PIN_TEST
	Wireless_RunPa10PinTestForever();
#elif WIRELESS_ZIGBEE_PASSIVE_RX_TEST
	Wireless_RunPassiveRxTestForever();
#elif WIRELESS_ZIGBEE_TRANSPARENT_TEST
	Wireless_RunTransparentTestForever();
#elif WIRELESS_ZIGBEE_DIAG_MODE
	Wireless_RunZigbeeDiagForever();
#else
	Wireless_InitZigbeeOrFault();

	MX_USART1_UART_Init(WIRELESS_ZIGBEE_RUNTIME_BAUDRATE);
	HAL_UART_Receive_IT(&huart1, mb_usart1_t.rx_buf, MB_BUF_SIZE);
	mb_usart1_t.rx_end_flg = 0;

	MX_USART2_UART_Init(WIRELESS_VM101_BAUDRATE);
	HAL_UART_Receive_IT(&huart2, mb_usart2_t.rx_buf, MB_BUF_SIZE);
	mb_usart2_t.rx_end_flg = 0;
#endif
}
