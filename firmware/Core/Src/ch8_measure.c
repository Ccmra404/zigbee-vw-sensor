#include "ch8_measure.h"
#include "app_controller.h"
#include "usart.h"

#define CH8_SELECT_GAP_MS 10U
#define CH8_SELECT_SETTLE_MS 80U
#define VM101_TIMEOUT_MS 6000U

static const uint16_t CH8CtrlPins[CH8_CHANNEL_COUNT] = {
	GPIO_PIN_0, GPIO_PIN_1, GPIO_PIN_2, GPIO_PIN_3,
	GPIO_PIN_4, GPIO_PIN_5, GPIO_PIN_6, GPIO_PIN_7
};

/**
  * @brief  Delay for the given number of milliseconds using HAL tick.
  * @param[in] nms Delay time in milliseconds.
  * @retval None
  */
static void CH8_DelayMs(u32 nms)
{
	u32 start = HAL_GetTick();
	while(HAL_GetTick() - start < nms)
	{
	}
}

/**
  * @brief  Turn off all eight CH8 channel selection outputs.
  * @retval None
  */
void CH8_AllChannelsOff(void)
{
	HAL_GPIO_WritePin(GPIOB,
		GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|
		GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7,
		GPIO_PIN_RESET);
}

/**
  * @brief  Select one CH8 measurement channel after clearing all channels.
  * @param[in] channel Zero-based channel index, valid range is 0 to 7.
  * @retval None
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
  * @brief  Check whether an ID maps to one of this node's CH8 channels.
  * @param[in] baseId First channel ID assigned to this node.
  * @param[in] id Requested channel ID.
  * @retval 1 if the ID is local, otherwise 0.
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
  * @brief  Convert a local channel ID to a zero-based channel index.
  * @param[in] baseId First channel ID assigned to this node.
  * @param[in] id Local channel ID to convert.
  * @retval Zero-based channel index.
  */
u8 CH8_ChannelIndexFromId(u32 baseId, u32 id)
{
	return (u8)(id - baseId);
}

/**
  * @brief  Find the first channel that has not replied in a response mask.
  * @param[in] mask Bit mask of replied channels, bit0 maps to CH1.
  * @retval First pending zero-based channel index, or 0 if all bits are set.
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
  * @brief  Read one Modbus measurement frame from the VM101 module.
  * @param[out] freq Raw frequency value returned by VM101.
  * @param[out] temp Raw temperature value returned by VM101.
  * @retval 0 on success, non-zero on timeout or invalid response.
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
  * @brief  Calculate mode-specific result fields for one CH8 channel.
  * @param[in,out] testValue Measurement buffer using CH8_*_BASE offsets.
  * @param[in] referenceData Per-channel reference values for strain mode.
  * @param[in] channelIndex Zero-based channel index.
  * @param[in] mode 0: strain, 1: frequency modulus, 2: corrected frequency.
  * @param[in] kValue Strain calculation coefficient.
  * @param[in] correctScale Frequency correction scale.
  * @retval None
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
  * @brief  Select the measurement result field that matches the work mode.
  * @param[in] testValue CH8 measurement buffer.
  * @param[in] channelIndex Zero-based channel index.
  * @param[in] mode 0: strain, 1: frequency modulus, 2/other: raw frequency.
  * @retval Selected result value.
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
  * @brief  Measure all CH8 channels and calculate mode-specific results.
  * @param[in,out] testValue Measurement buffer using CH8_*_BASE offsets.
  * @param[in] referenceData Per-channel reference values for strain mode.
  * @param[in] mode 0: strain, 1: frequency modulus, 2: corrected frequency.
  * @param[in] kValue Strain calculation coefficient.
  * @param[in] correctScale Frequency correction scale.
  * @retval 0 on success, non-zero if VM101 communication fails.
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
  * @brief  Measure one CH8 channel and refresh its mode-specific result.
  * @param[in,out] testValue Measurement buffer using CH8_*_BASE offsets.
  * @param[in] referenceData Per-channel reference values for strain mode.
  * @param[in] channelIndex Zero-based channel index.
  * @param[in] mode 0: strain, 1: frequency modulus, 2: corrected frequency.
  * @param[in] kValue Strain calculation coefficient.
  * @param[in] correctScale Frequency correction scale.
  * @retval 0 on success, non-zero if the channel is invalid or VM101 communication fails.
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
