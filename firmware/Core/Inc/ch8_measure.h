#ifndef __CH8_MEASURE_H
#define __CH8_MEASURE_H

#include "main.h"

#define CH8_CHANNEL_COUNT 8

#define CH8_FREQ_BASE 0
#define CH8_TEMP_BASE CH8_CHANNEL_COUNT
#define CH8_STRAIN_BASE (CH8_CHANNEL_COUNT * 2)
#define CH8_FMOD_BASE (CH8_CHANNEL_COUNT * 3)
#define CH8_RESULT_SCRATCH (CH8_CHANNEL_COUNT * 4)
#define CH8_TEST_VALUE_SIZE (CH8_CHANNEL_COUNT * 4 + 1)

#define CH8_RESPONSE_MASK ((u8)((1U << CH8_CHANNEL_COUNT) - 1U))

/**
  * @brief  Turn off all eight CH8 channel selection outputs.
  * @retval None
  */
void CH8_AllChannelsOff(void);

/**
  * @brief  Measure all CH8 channels and calculate mode-specific results.
  * @param[in,out] testValue Measurement buffer using CH8_*_BASE offsets.
  * @param[in] referenceData Per-channel reference values for strain mode.
  * @param[in] mode 0: strain, 1: frequency modulus, 2: corrected frequency.
  * @param[in] kValue Strain calculation coefficient.
  * @param[in] correctScale Frequency correction scale.
  * @retval 0 on success, non-zero if VM101 communication fails.
  */
int CH8_MeasureAllChannels(int *testValue, int *referenceData, u8 mode, float kValue, float correctScale);

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
int CH8_MeasureChannel(int *testValue, int *referenceData, u8 channelIndex, u8 mode, float kValue, float correctScale);

/**
  * @brief  Select the measurement result field that matches the work mode.
  * @param[in] testValue CH8 measurement buffer.
  * @param[in] channelIndex Zero-based channel index.
  * @param[in] mode 0: strain, 1: frequency modulus, 2/other: raw frequency.
  * @retval Selected result value.
  */
int CH8_ResultForMode(const int *testValue, u8 channelIndex, u8 mode);

/**
  * @brief  Check whether an ID maps to one of this node's CH8 channels.
  * @param[in] baseId First channel ID assigned to this node.
  * @param[in] id Requested channel ID.
  * @retval 1 if the ID is local, otherwise 0.
  */
u8 CH8_IsLocalChannelId(u32 baseId, u32 id);

/**
  * @brief  Convert a local channel ID to a zero-based channel index.
  * @param[in] baseId First channel ID assigned to this node.
  * @param[in] id Local channel ID to convert.
  * @retval Zero-based channel index.
  */
u8 CH8_ChannelIndexFromId(u32 baseId, u32 id);

/**
  * @brief  Find the first channel that has not replied in a response mask.
  * @param[in] mask Bit mask of replied channels, bit0 maps to CH1.
  * @retval First pending zero-based channel index, or 0 if all bits are set.
  */
u8 CH8_NextPendingChannelIndex(u8 mask);

#endif
