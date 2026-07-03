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
  * @brief  关闭 CH8 的 8 路通道选择输出。
  * @retval 无
  */
void CH8_AllChannelsOff(void);

/**
  * @brief  测量 CH8 全部通道并计算当前模式下的结果。
  * @param[in,out] testValue 使用 CH8_*_BASE 偏移的测量缓冲区。
  * @param[in] referenceData 应变模式下每个通道的参考值。
  * @param[in] mode 0：应变；1：频率模数；2：频率修正。
  * @param[in] kValue 应变计算系数。
  * @param[in] correctScale 频率修正系数。
  * @retval 0 表示成功，非 0 表示 VM101 通信失败。
  */
int CH8_MeasureAllChannels(int *testValue, int *referenceData, u8 mode, float kValue, float correctScale);

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
int CH8_MeasureChannel(int *testValue, int *referenceData, u8 channelIndex, u8 mode, float kValue, float correctScale);

/**
  * @brief  按工作模式选择对应的测量结果字段。
  * @param[in] testValue CH8 测量缓冲区。
  * @param[in] channelIndex 从 0 开始的通道序号。
  * @param[in] mode 0：应变；1：频率模数；2 或其他：原始频率。
  * @retval 选中的结果值。
  */
int CH8_ResultForMode(const int *testValue, u8 channelIndex, u8 mode);

/**
  * @brief  判断指定 ID 是否属于本节点的 CH8 通道。
  * @param[in] baseId 本节点分配到的首个通道 ID。
  * @param[in] id 请求的通道 ID。
  * @retval 1 表示本节点通道，0 表示不是本节点通道。
  */
u8 CH8_IsLocalChannelId(u32 baseId, u32 id);

/**
  * @brief  将本节点通道 ID 转换为从 0 开始的通道序号。
  * @param[in] baseId 本节点分配到的首个通道 ID。
  * @param[in] id 待转换的本节点通道 ID。
  * @retval 从 0 开始的通道序号。
  */
u8 CH8_ChannelIndexFromId(u32 baseId, u32 id);

/**
  * @brief  在响应掩码中查找第一个未回复通道。
  * @param[in] mask 已回复通道位掩码，bit0 对应 CH1。
  * @retval 第一个未回复通道的从 0 开始序号；全部回复时返回 0。
  */
u8 CH8_NextPendingChannelIndex(u8 mask);

#endif
