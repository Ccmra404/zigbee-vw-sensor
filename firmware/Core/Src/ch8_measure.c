#include "ch8_measure.h"
#include "app_uart.h"
#include "usart.h"

#define CH8_SELECT_GAP_MS 10U
#define CH8_SELECT_SETTLE_MS 80U
#define VM101_TIMEOUT_MS 6000U

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
