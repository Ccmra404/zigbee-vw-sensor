#include "ch8_measure.h"
#include "app_uart.h"
#include "usart.h"

#define CH8_SELECT_GAP_MS 10U
#define CH8_SELECT_SETTLE_MS 6000U
#define VM101_TIMEOUT_MS 6000U
#define VM101_DIAG_READ_INTERVAL_MS 6000U
#define VM101_DIAG_START_DELAY_MS 1000U

#define VM101_DIAG_STATUS_OK 0U
#define VM101_DIAG_STATUS_TIMEOUT 1U
#define VM101_DIAG_STATUS_SHORT_FRAME 2U
#define VM101_DIAG_STATUS_WRONG_HEADER 3U
#define VM101_DIAG_STATUS_BAD_BYTE_COUNT 4U
#define VM101_DIAG_STATUS_BAD_CRC 5U

#define CH8_SCAN_STATE_IDLE 0U
#define CH8_SCAN_STATE_OFF_GAP 1U
#define CH8_SCAN_STATE_SETTLE 2U
#define CH8_SCAN_STATE_SEND 3U
#define CH8_SCAN_STATE_WAIT 4U
#define CH8_SCAN_STATE_NEXT 5U

volatile u32 Vm101DiagMagic = 0;
volatile u32 Vm101DiagCycle = 0;
volatile u32 Vm101DiagDone = 0;
volatile u32 Vm101DiagLastChannel = 0;
volatile u32 Vm101DiagSelectedMask[CH8_CHANNEL_COUNT] = {0};
volatile u32 Vm101DiagStatus[CH8_CHANNEL_COUNT] = {0};
volatile u32 Vm101DiagRxLen[CH8_CHANNEL_COUNT] = {0};
volatile u32 Vm101DiagCrc[CH8_CHANNEL_COUNT] = {0};
volatile u32 Vm101DiagFreq[CH8_CHANNEL_COUNT] = {0};
volatile u32 Vm101DiagTemp[CH8_CHANNEL_COUNT] = {0};
volatile u8 Vm101DiagRx[CH8_CHANNEL_COUNT][VM101_DIAG_RX_SIZE] = {{0}};
volatile u8 Vm101DiagCmd[8] = {0x01,0x03,0x00,0x00,0x00,0x02,0xC4,0x0B};
volatile u32 CH8ScanState = CH8_SCAN_STATE_IDLE;
volatile u32 CH8ScanCurrentChannel = 0;
volatile u32 CH8ScanCycleCount = 0;
volatile u32 CH8ScanReadyMask = 0;
volatile u32 CH8ScanStatus[CH8_CHANNEL_COUNT] = {0};
volatile u32 CH8ScanLastTick = 0;

static const u8 VM101ReadCmd[8] = {0x01,0x03,0x00,0x00,0x00,0x02,0xC4,0x0B};

static const uint16_t CH8CtrlPins[CH8_CHANNEL_COUNT] = {
	GPIO_PIN_0, GPIO_PIN_1, GPIO_PIN_2, GPIO_PIN_3,
	GPIO_PIN_4, GPIO_PIN_5, GPIO_PIN_6, GPIO_PIN_7
};

/**
  * @brief  浣跨敤 HAL 鑺傛媿寤舵椂鎸囧畾姣鏁般€?  * @param[in] nms 寤舵椂鏃堕棿锛屽崟浣嶄负姣銆?  * @retval 鏃?  */
static void CH8_DelayMs(u32 nms)
{
	u32 start = HAL_GetTick();
	while(HAL_GetTick() - start < nms)
	{
	}
}

/**
  * @brief  鍏抽棴 CH8 鐨?8 璺€氶亾閫夋嫨杈撳嚭銆?  * @retval 鏃?  */
void CH8_AllChannelsOff(void)
{
	HAL_GPIO_WritePin(GPIOB,
		GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|
		GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7,
		GPIO_PIN_RESET);
}

/**
  * @brief  娓呴櫎鍏ㄩ儴閫氶亾鍚庨€夋嫨涓€璺?CH8 娴嬮噺閫氶亾銆?  * @param[in] channel 浠?0 寮€濮嬬殑閫氶亾搴忓彿锛屾湁鏁堣寖鍥翠负 0 鍒?7銆?  * @retval 鏃?  */
static void CH8_SelectChannel(u8 channel)
{
	CH8_AllChannelsOff();
	CH8_DelayMs(CH8_SELECT_GAP_MS);
	if(channel < CH8_CHANNEL_COUNT)
	{
		HAL_GPIO_WritePin(GPIOB, CH8CtrlPins[channel], GPIO_PIN_SET);
		CH8_DelayMs(CH8_SELECT_SETTLE_MS);
	}
}

static void VM101_DiagClearChannel(u8 channel)
{
	u32 i = 0;

	if(channel >= CH8_CHANNEL_COUNT)
	{
		return;
	}

	Vm101DiagStatus[channel] = VM101_DIAG_STATUS_TIMEOUT;
	Vm101DiagRxLen[channel] = 0;
	Vm101DiagCrc[channel] = 0xFFFFFFFFU;
	Vm101DiagFreq[channel] = 0;
	Vm101DiagTemp[channel] = 0;
	for(i = 0;i < VM101_DIAG_RX_SIZE;i++)
	{
		Vm101DiagRx[channel][i] = 0;
	}
}

static void VM101_DiagArmRx(void)
{
	u32 i = 0;

	MsgFlag2 = 0;
	mb_usart2_t.rx_end_flg = 0;
	for(i = 0;i < MB_BUF_SIZE;i++)
	{
		mb_usart2_t.rx_buf[i] = 0;
	}
	if(huart2.RxState == HAL_UART_STATE_READY)
	{
		HAL_UART_Receive_IT(&huart2, mb_usart2_t.rx_buf, MB_BUF_SIZE);
	}
}

static void VM101_DiagCopyRx(u8 channel, u32 length)
{
	u32 i = 0;
	u32 copyLength = length;

	if(channel >= CH8_CHANNEL_COUNT)
	{
		return;
	}
	if(copyLength > VM101_DIAG_RX_SIZE)
	{
		copyLength = VM101_DIAG_RX_SIZE;
	}
	for(i = 0;i < copyLength;i++)
	{
		Vm101DiagRx[channel][i] = mb_usart2_t.rx_buf[i];
	}
}

static void VM101_DiagReadChannel(u8 channel)
{
	u32 start = 0;
	u32 length = 0;
	u32 crc = 0;

	VM101_DiagClearChannel(channel);
	CH8_SelectChannel(channel);
	Vm101DiagLastChannel = channel;
	Vm101DiagSelectedMask[channel] = GPIOB->ODR & 0x00FFU;
	CH8_DelayMs(VM101_DIAG_READ_INTERVAL_MS);
	VM101_DiagArmRx();
	Usart_Printf_Len(&huart2, (uint8_t *)Vm101DiagCmd, 8);
	start = HAL_GetTick();

	while(1)
	{
		if(mb_usart2_t.rx_end_flg == 1)
		{
			length = (u32)(uint8_t)MsgFlag2;
			mb_usart2_t.rx_end_flg = 0;
			Vm101DiagRxLen[channel] = length;
			VM101_DiagCopyRx(channel, length);

			if(length < 9U)
			{
				Vm101DiagStatus[channel] = VM101_DIAG_STATUS_SHORT_FRAME;
				return;
			}
			if(mb_usart2_t.rx_buf[0] != 0x01 || mb_usart2_t.rx_buf[1] != 0x03)
			{
				Vm101DiagStatus[channel] = VM101_DIAG_STATUS_WRONG_HEADER;
				return;
			}
			if(mb_usart2_t.rx_buf[2] < 0x04)
			{
				Vm101DiagStatus[channel] = VM101_DIAG_STATUS_BAD_BYTE_COUNT;
				return;
			}

			crc = crc16_calc(mb_usart2_t.rx_buf, 9);
			Vm101DiagCrc[channel] = crc;
			if(crc != 0)
			{
				Vm101DiagStatus[channel] = VM101_DIAG_STATUS_BAD_CRC;
				return;
			}

			Vm101DiagFreq[channel] = ((u32)mb_usart2_t.rx_buf[3] << 8) | mb_usart2_t.rx_buf[4];
			Vm101DiagTemp[channel] = (((u32)mb_usart2_t.rx_buf[5] << 8) | mb_usart2_t.rx_buf[6]) - 500U;
			Vm101DiagStatus[channel] = VM101_DIAG_STATUS_OK;
			return;
		}
		if(HAL_GetTick() - start >= VM101_TIMEOUT_MS)
		{
			Vm101DiagStatus[channel] = VM101_DIAG_STATUS_TIMEOUT;
			return;
		}
	}
}

void VM101_RunDiagForever(void)
{
	u8 channel = 0;

	Vm101DiagMagic = 0x56313031U;
	Vm101DiagCycle++;
	Vm101DiagDone = 0;
	CH8_AllChannelsOff();
	CH8_DelayMs(VM101_DIAG_START_DELAY_MS);

	for(channel = 0;channel < CH8_CHANNEL_COUNT;channel++)
	{
		VM101_DiagReadChannel(channel);
		CH8_AllChannelsOff();
	}

	CH8_AllChannelsOff();
	Vm101DiagDone = 1;

	while(1)
	{
		__NOP();
	}
}

/**
  * @brief  鍒ゆ柇鎸囧畾 ID 鏄惁灞炰簬鏈妭鐐圭殑 CH8 閫氶亾銆?  * @param[in] baseId 鏈妭鐐瑰垎閰嶅埌鐨勯涓€氶亾 ID銆?  * @param[in] id 璇锋眰鐨勯€氶亾 ID銆?  * @retval 1 琛ㄧず鏈妭鐐归€氶亾锛? 琛ㄧず涓嶆槸鏈妭鐐归€氶亾銆?  */
u8 CH8_IsLocalChannelId(u32 baseId, u32 id)
{
	if(id < baseId)
	{
		return 0;
	}
	if(id >= baseId + CH8_CHANNEL_COUNT)
	{
		return 0;
	}
	return 1;
}

/**
  * @brief  灏嗘湰鑺傜偣閫氶亾 ID 杞崲涓轰粠 0 寮€濮嬬殑閫氶亾搴忓彿銆?  * @param[in] baseId 鏈妭鐐瑰垎閰嶅埌鐨勯涓€氶亾 ID銆?  * @param[in] id 寰呰浆鎹㈢殑鏈妭鐐归€氶亾 ID銆?  * @retval 浠?0 寮€濮嬬殑閫氶亾搴忓彿銆?  */
u8 CH8_ChannelIndexFromId(u32 baseId, u32 id)
{
	return (u8)(id - baseId);
}

/**
  * @brief  鍦ㄥ搷搴旀帺鐮佷腑鏌ユ壘绗竴涓湭鍥炲閫氶亾銆?  * @param[in] mask 宸插洖澶嶉€氶亾浣嶆帺鐮侊紝bit0 瀵瑰簲 CH1銆?  * @retval 绗竴涓湭鍥炲閫氶亾鐨勪粠 0 寮€濮嬪簭鍙凤紱鍏ㄩ儴鍥炲鏃惰繑鍥?0銆?  */
u8 CH8_NextPendingChannelIndex(u8 mask)
{
	u8 i = 0;
	for(i = 0;i < CH8_CHANNEL_COUNT;i++)
	{
		if(0 == (mask & (1U << i)))
		{
			return i;
		}
	}
	return 0;
}

/**
  * @brief  浠?VM101 妯″潡璇诲彇涓€甯?Modbus 娴嬮噺鏁版嵁銆?  * @param[out] freq VM101 杩斿洖鐨勫師濮嬮鐜囧€笺€?  * @param[out] temp VM101 杩斿洖鐨勫師濮嬫俯搴﹀€笺€?  * @retval 0 琛ㄧず鎴愬姛锛岄潪 0 琛ㄧず瓒呮椂鎴栧搷搴旀棤鏁堛€?  */
static int VM101_ReadOnce(int *freq, int *temp)
{
	u32 start = 0;
	u8 cmd[8] = {0x01,0x03,0x00,0x00,0x00,0x02,0xC4,0x0B};

	MsgFlag2 = 0;
	mb_usart2_t.rx_end_flg = 0;
	Usart_Printf_Len(&huart2,cmd,8);
	start = HAL_GetTick();

	while(1)
	{
		if(HAL_GetTick() - start >= VM101_TIMEOUT_MS)
		{
			return 1;
		}
		if(mb_usart2_t.rx_end_flg == 1)
		{
			mb_usart2_t.rx_end_flg = 0;
			if(MsgFlag2 >= 9
				&& mb_usart2_t.rx_buf[0] == 0x01
				&& mb_usart2_t.rx_buf[1] == 0x03
				&& mb_usart2_t.rx_buf[2] >= 0x04
				&& crc16_calc(mb_usart2_t.rx_buf,9) == 0)
			{
				*freq = ((int)mb_usart2_t.rx_buf[3] << 8) | mb_usart2_t.rx_buf[4];
				*temp = (((int)mb_usart2_t.rx_buf[5] << 8) | mb_usart2_t.rx_buf[6]) - 500;
				return 0;
			}
		}
	}
}

/**
  * @brief  璁＄畻鍗曚釜 CH8 閫氶亾鍦ㄥ綋鍓嶆ā寮忎笅鐨勭粨鏋滃瓧娈点€?  * @param[in,out] testValue 浣跨敤 CH8_*_BASE 鍋忕Щ鐨勬祴閲忕紦鍐插尯銆?  * @param[in] referenceData 搴斿彉妯″紡涓嬫瘡涓€氶亾鐨勫弬鑰冨€笺€?  * @param[in] channelIndex 浠?0 寮€濮嬬殑閫氶亾搴忓彿銆?  * @param[in] mode 0锛氬簲鍙橈紱1锛氶鐜囨ā鏁帮紱2锛氶鐜囦慨姝ｃ€?  * @param[in] kValue 搴斿彉璁＄畻绯绘暟銆?  * @param[in] correctScale 棰戠巼淇绯绘暟銆?  * @retval 鏃?  */
static void CH8_UpdateChannelResult(int *testValue, const int *referenceData, u8 channelIndex, u8 mode, float kValue, float correctScale)
{
	float current = 0;
	float reference = 0;

	if(channelIndex >= CH8_CHANNEL_COUNT)
	{
		return;
	}

	if(0 == mode)
	{
		current = testValue[CH8_FREQ_BASE + channelIndex];
		current = current / 10;
		current = current * current / 1000;

		reference = referenceData[channelIndex];
		reference = reference / 10;
		reference = reference * reference / 1000;

		current = (current - reference) * 10 * kValue * correctScale;
		testValue[CH8_STRAIN_BASE + channelIndex] = (int)(current + 0.5);
	}
	else if(1 == mode)
	{
		current = testValue[CH8_FREQ_BASE + channelIndex] / 10;
		current = current * (current / 100) * correctScale;
		testValue[CH8_FMOD_BASE + channelIndex] = (int)(current + 0.5);
	}
	else if(2 == mode)
	{
		current = testValue[CH8_FREQ_BASE + channelIndex] * correctScale;
		testValue[CH8_FREQ_BASE + channelIndex] = (int)(current + 0.5);
	}
}

static void CH8_ScanArmRx(void)
{
	u32 i = 0;

	MsgFlag2 = 0;
	mb_usart2_t.rx_end_flg = 0;
	for(i = 0;i < MB_BUF_SIZE;i++)
	{
		mb_usart2_t.rx_buf[i] = 0;
	}
	if(huart2.RxState == HAL_UART_STATE_READY)
	{
		HAL_UART_Receive_IT(&huart2, mb_usart2_t.rx_buf, MB_BUF_SIZE);
	}
}

static u32 CH8_ScanDecodeFrame(int *testValue, const int *referenceData, u8 mode, float kValue, float correctScale)
{
	u8 channel = (u8)CH8ScanCurrentChannel;
	u32 length = (u32)(uint8_t)MsgFlag2;
	u32 crc = 0;

	if(length < 9U)
	{
		return CH8_SCAN_STATUS_SHORT_FRAME;
	}
	if(mb_usart2_t.rx_buf[0] != 0x01 || mb_usart2_t.rx_buf[1] != 0x03)
	{
		return CH8_SCAN_STATUS_WRONG_HEADER;
	}
	if(mb_usart2_t.rx_buf[2] < 0x04)
	{
		return CH8_SCAN_STATUS_BAD_BYTE_COUNT;
	}

	crc = crc16_calc(mb_usart2_t.rx_buf, 9);
	if(crc != 0)
	{
		return CH8_SCAN_STATUS_BAD_CRC;
	}

	testValue[CH8_FREQ_BASE + channel] = ((int)mb_usart2_t.rx_buf[3] << 8) | mb_usart2_t.rx_buf[4];
	testValue[CH8_TEMP_BASE + channel] = (((int)mb_usart2_t.rx_buf[5] << 8) | mb_usart2_t.rx_buf[6]) - 500;
	CH8_UpdateChannelResult(testValue, referenceData, channel, mode, kValue, correctScale);
	CH8ScanReadyMask |= (1U << channel);

	return CH8_SCAN_STATUS_OK;
}

void CH8_ScanInit(void)
{
	u8 i = 0;

	CH8_AllChannelsOff();
	CH8ScanState = CH8_SCAN_STATE_IDLE;
	CH8ScanCurrentChannel = 0;
	CH8ScanCycleCount = 0;
	CH8ScanReadyMask = 0;
	CH8ScanLastTick = HAL_GetTick();
	for(i = 0;i < CH8_CHANNEL_COUNT;i++)
	{
		CH8ScanStatus[i] = CH8_SCAN_STATUS_NOT_READY;
	}
}

void CH8_ScanRestart(void)
{
	CH8_AllChannelsOff();
	CH8ScanState = CH8_SCAN_STATE_IDLE;
	CH8ScanCurrentChannel = 0;
	CH8ScanLastTick = HAL_GetTick();
}

void CH8_ScanProcess(int *testValue, int *referenceData, u8 mode, float kValue, float correctScale)
{
	u32 now = HAL_GetTick();
	u8 channel = (u8)CH8ScanCurrentChannel;

	if(channel >= CH8_CHANNEL_COUNT)
	{
		CH8ScanCurrentChannel = 0;
		channel = 0;
	}

	switch(CH8ScanState)
	{
		case CH8_SCAN_STATE_IDLE:
			CH8_AllChannelsOff();
			CH8ScanLastTick = now;
			CH8ScanState = CH8_SCAN_STATE_OFF_GAP;
			break;

		case CH8_SCAN_STATE_OFF_GAP:
			if(now - CH8ScanLastTick >= CH8_SELECT_GAP_MS)
			{
				HAL_GPIO_WritePin(GPIOB, CH8CtrlPins[channel], GPIO_PIN_SET);
				CH8ScanLastTick = now;
				CH8ScanState = CH8_SCAN_STATE_SETTLE;
			}
			break;

		case CH8_SCAN_STATE_SETTLE:
			if(now - CH8ScanLastTick >= CH8_SELECT_SETTLE_MS)
			{
				CH8ScanState = CH8_SCAN_STATE_SEND;
			}
			break;

		case CH8_SCAN_STATE_SEND:
			CH8_ScanArmRx();
			Usart_Printf_Len(&huart2, (uint8_t *)VM101ReadCmd, 8);
			CH8ScanLastTick = now;
			CH8ScanState = CH8_SCAN_STATE_WAIT;
			break;

		case CH8_SCAN_STATE_WAIT:
			if(mb_usart2_t.rx_end_flg == 1)
			{
				mb_usart2_t.rx_end_flg = 0;
				CH8ScanStatus[channel] = CH8_ScanDecodeFrame(testValue, referenceData, mode, kValue, correctScale);
				CH8_AllChannelsOff();
				CH8ScanState = CH8_SCAN_STATE_NEXT;
			}
			else if(now - CH8ScanLastTick >= VM101_TIMEOUT_MS)
			{
				CH8ScanStatus[channel] = CH8_SCAN_STATUS_TIMEOUT;
				CH8_AllChannelsOff();
				CH8ScanState = CH8_SCAN_STATE_NEXT;
			}
			break;

		case CH8_SCAN_STATE_NEXT:
			CH8ScanCurrentChannel++;
			if(CH8ScanCurrentChannel >= CH8_CHANNEL_COUNT)
			{
				CH8ScanCurrentChannel = 0;
				CH8ScanCycleCount++;
			}
			CH8ScanState = CH8_SCAN_STATE_IDLE;
			break;

		default:
			CH8_ScanRestart();
			break;
	}
}

u8 CH8_GetChannelError(u8 channelIndex)
{
	if(channelIndex >= CH8_CHANNEL_COUNT)
	{
		return 1;
	}
	if(0 == (CH8ScanReadyMask & (1U << channelIndex)))
	{
		return 1;
	}
	if(CH8ScanStatus[channelIndex] != CH8_SCAN_STATUS_OK)
	{
		return 1;
	}
	return 0;
}

u8 CH8_IsScanBusy(void)
{
	return CH8ScanState != CH8_SCAN_STATE_IDLE;
}

/**
  * @brief  鎸夊伐浣滄ā寮忛€夋嫨瀵瑰簲鐨勬祴閲忕粨鏋滃瓧娈点€?  * @param[in] testValue CH8 娴嬮噺缂撳啿鍖恒€?  * @param[in] channelIndex 浠?0 寮€濮嬬殑閫氶亾搴忓彿銆?  * @param[in] mode 0锛氬簲鍙橈紱1锛氶鐜囨ā鏁帮紱2 鎴栧叾浠栵細鍘熷棰戠巼銆?  * @retval 閫変腑鐨勭粨鏋滃€笺€?  */
int CH8_ResultForMode(const int *testValue, u8 channelIndex, u8 mode)
{
	if(0 == mode)
	{
		return testValue[CH8_STRAIN_BASE + channelIndex];
	}
	if(1 == mode)
	{
		return testValue[CH8_FMOD_BASE + channelIndex];
	}
	return testValue[CH8_FREQ_BASE + channelIndex];
}

/**
  * @brief  娴嬮噺 CH8 鍏ㄩ儴閫氶亾骞惰绠楀綋鍓嶆ā寮忎笅鐨勭粨鏋溿€?  * @param[in,out] testValue 浣跨敤 CH8_*_BASE 鍋忕Щ鐨勬祴閲忕紦鍐插尯銆?  * @param[in] referenceData 搴斿彉妯″紡涓嬫瘡涓€氶亾鐨勫弬鑰冨€笺€?  * @param[in] mode 0锛氬簲鍙橈紱1锛氶鐜囨ā鏁帮紱2锛氶鐜囦慨姝ｃ€?  * @param[in] kValue 搴斿彉璁＄畻绯绘暟銆?  * @param[in] correctScale 棰戠巼淇绯绘暟銆?  * @retval 0 琛ㄧず鎴愬姛锛岄潪 0 琛ㄧず VM101 閫氫俊澶辫触銆?  */
int CH8_MeasureAllChannels(int *testValue, int *referenceData, u8 mode, float kValue, float correctScale)
{
	int i = 0;
	int ret = 0;

	for(i = 0;i < CH8_CHANNEL_COUNT;i++)
	{
		CH8_SelectChannel((u8)i);
		ret = VM101_ReadOnce(&testValue[CH8_FREQ_BASE + i], &testValue[CH8_TEMP_BASE + i]);
		if(ret)
		{
			CH8_AllChannelsOff();
			return ret;
		}
	}
	CH8_AllChannelsOff();

	for(i = 0;i < CH8_CHANNEL_COUNT;i++)
	{
		CH8_UpdateChannelResult(testValue, referenceData, (u8)i, mode, kValue, correctScale);
	}

	return 0;
}

/**
  * @brief  娴嬮噺鍗曚釜 CH8 閫氶亾骞跺埛鏂板綋鍓嶆ā寮忎笅鐨勭粨鏋溿€?  * @param[in,out] testValue 浣跨敤 CH8_*_BASE 鍋忕Щ鐨勬祴閲忕紦鍐插尯銆?  * @param[in] referenceData 搴斿彉妯″紡涓嬫瘡涓€氶亾鐨勫弬鑰冨€笺€?  * @param[in] channelIndex 浠?0 寮€濮嬬殑閫氶亾搴忓彿銆?  * @param[in] mode 0锛氬簲鍙橈紱1锛氶鐜囨ā鏁帮紱2锛氶鐜囦慨姝ｃ€?  * @param[in] kValue 搴斿彉璁＄畻绯绘暟銆?  * @param[in] correctScale 棰戠巼淇绯绘暟銆?  * @retval 0 琛ㄧず鎴愬姛锛岄潪 0 琛ㄧず閫氶亾鏃犳晥鎴?VM101 閫氫俊澶辫触銆?  */
int CH8_MeasureChannel(int *testValue, int *referenceData, u8 channelIndex, u8 mode, float kValue, float correctScale)
{
	int ret = 0;

	if(channelIndex >= CH8_CHANNEL_COUNT)
	{
		return 1;
	}

	CH8_SelectChannel(channelIndex);
	ret = VM101_ReadOnce(&testValue[CH8_FREQ_BASE + channelIndex], &testValue[CH8_TEMP_BASE + channelIndex]);
	CH8_AllChannelsOff();
	if(ret)
	{
		return ret;
	}

	CH8_UpdateChannelResult(testValue, referenceData, channelIndex, mode, kValue, correctScale);
	return 0;
}
