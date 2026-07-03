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
  * @brief  使用 HAL 节拍延时指定毫秒数。
  * @param[in] nms 延时时间，单位为毫秒。
  * @retval 无
  */
static void CH8_DelayMs(u32 nms)
{
	u32 start = HAL_GetTick();
	while(HAL_GetTick() - start < nms)
	{
	}
}

/**
  * @brief  关闭 CH8 的 8 路通道选择输出。
  * @retval 无
  */
void CH8_AllChannelsOff(void)
{
	HAL_GPIO_WritePin(GPIOB,
		GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|
		GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7,
		GPIO_PIN_RESET);
}

/**
  * @brief  清除全部通道后选择一路 CH8 测量通道。
  * @param[in] channel 从 0 开始的通道序号，有效范围为 0 到 7。
  * @retval 无
  */
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
  * @brief  判断指定 ID 是否属于本节点的 CH8 通道。
  * @param[in] baseId 本节点分配到的首个通道 ID。
  * @param[in] id 请求的通道 ID。
  * @retval 1 表示本节点通道，0 表示不是本节点通道。
  */
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
  * @brief  将本节点通道 ID 转换为从 0 开始的通道序号。
  * @param[in] baseId 本节点分配到的首个通道 ID。
  * @param[in] id 待转换的本节点通道 ID。
  * @retval 从 0 开始的通道序号。
  */
u8 CH8_ChannelIndexFromId(u32 baseId, u32 id)
{
	return (u8)(id - baseId);
}

/**
  * @brief  在响应掩码中查找第一个未回复通道。
  * @param[in] mask 已回复通道位掩码，bit0 对应 CH1。
  * @retval 第一个未回复通道的从 0 开始序号；全部回复时返回 0。
  */
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
  * @brief  从 VM101 模块读取一帧 Modbus 测量数据。
  * @param[out] freq VM101 返回的原始频率值。
  * @param[out] temp VM101 返回的原始温度值。
  * @retval 0 表示成功，非 0 表示超时或响应无效。
  */
static int VM101_ReadOnce(int *freq, int *temp)
{
	u32 start = 0;
	u8 cmd[8] = {0x01,0x03,0x00,0x00,0x00,0x02,0xC4,0x0B};

	MsgFlag1 = 0;
	mb_usart1_t.rx_end_flg = 0;
	Usart_Printf_Len(&huart1,cmd,8);
	start = HAL_GetTick();

	while(1)
	{
		if(HAL_GetTick() - start >= VM101_TIMEOUT_MS)
		{
			return 1;
		}
		if(mb_usart1_t.rx_end_flg == 1)
		{
			mb_usart1_t.rx_end_flg = 0;
			if(MsgFlag1 >= 9
				&& mb_usart1_t.rx_buf[0] == 0x01
				&& mb_usart1_t.rx_buf[1] == 0x03
				&& mb_usart1_t.rx_buf[2] >= 0x04
				&& crc16_calc(mb_usart1_t.rx_buf,9) == 0)
			{
				*freq = ((int)mb_usart1_t.rx_buf[3] << 8) | mb_usart1_t.rx_buf[4];
				*temp = ((int)mb_usart1_t.rx_buf[5] << 8) | mb_usart1_t.rx_buf[6];
				return 0;
			}
		}
	}
}

/**
  * @brief  计算单个 CH8 通道在当前模式下的结果字段。
  * @param[in,out] testValue 使用 CH8_*_BASE 偏移的测量缓冲区。
  * @param[in] referenceData 应变模式下每个通道的参考值。
  * @param[in] channelIndex 从 0 开始的通道序号。
  * @param[in] mode 0：应变；1：频率模数；2：频率修正。
  * @param[in] kValue 应变计算系数。
  * @param[in] correctScale 频率修正系数。
  * @retval 无
  */
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
  * @brief  按工作模式选择对应的测量结果字段。
  * @param[in] testValue CH8 测量缓冲区。
  * @param[in] channelIndex 从 0 开始的通道序号。
  * @param[in] mode 0：应变；1：频率模数；2 或其他：原始频率。
  * @retval 选中的结果值。
  */
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
  * @brief  测量 CH8 全部通道并计算当前模式下的结果。
  * @param[in,out] testValue 使用 CH8_*_BASE 偏移的测量缓冲区。
  * @param[in] referenceData 应变模式下每个通道的参考值。
  * @param[in] mode 0：应变；1：频率模数；2：频率修正。
  * @param[in] kValue 应变计算系数。
  * @param[in] correctScale 频率修正系数。
  * @retval 0 表示成功，非 0 表示 VM101 通信失败。
  */
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
  * @brief  测量单个 CH8 通道并刷新当前模式下的结果。
  * @param[in,out] testValue 使用 CH8_*_BASE 偏移的测量缓冲区。
  * @param[in] referenceData 应变模式下每个通道的参考值。
  * @param[in] channelIndex 从 0 开始的通道序号。
  * @param[in] mode 0：应变；1：频率模数；2：频率修正。
  * @param[in] kValue 应变计算系数。
  * @param[in] correctScale 频率修正系数。
  * @retval 0 表示成功，非 0 表示通道无效或 VM101 通信失败。
  */
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
