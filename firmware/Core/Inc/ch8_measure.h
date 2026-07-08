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

#define VM101_DIAG_RX_SIZE 16U

#define CH8_SCAN_STATUS_OK 0U
#define CH8_SCAN_STATUS_NOT_READY 1U
#define CH8_SCAN_STATUS_TIMEOUT 2U
#define CH8_SCAN_STATUS_SHORT_FRAME 3U
#define CH8_SCAN_STATUS_WRONG_HEADER 4U
#define CH8_SCAN_STATUS_BAD_BYTE_COUNT 5U
#define CH8_SCAN_STATUS_BAD_CRC 6U

extern volatile u32 Vm101DiagMagic;
extern volatile u32 Vm101DiagCycle;
extern volatile u32 Vm101DiagDone;
extern volatile u32 Vm101DiagLastChannel;
extern volatile u32 Vm101DiagSelectedMask[CH8_CHANNEL_COUNT];
extern volatile u32 Vm101DiagStatus[CH8_CHANNEL_COUNT];
extern volatile u32 Vm101DiagRxLen[CH8_CHANNEL_COUNT];
extern volatile u32 Vm101DiagCrc[CH8_CHANNEL_COUNT];
extern volatile u32 Vm101DiagFreq[CH8_CHANNEL_COUNT];
extern volatile u32 Vm101DiagTemp[CH8_CHANNEL_COUNT];
extern volatile u8 Vm101DiagRx[CH8_CHANNEL_COUNT][VM101_DIAG_RX_SIZE];
extern volatile u8 Vm101DiagCmd[8];
extern volatile u32 CH8ScanState;
extern volatile u32 CH8ScanCurrentChannel;
extern volatile u32 CH8ScanCycleCount;
extern volatile u32 CH8ScanReadyMask;
extern volatile u32 CH8ScanStatus[CH8_CHANNEL_COUNT];
extern volatile u32 CH8ScanLastTick;

void VM101_RunDiagForever(void);
void CH8_ScanInit(void);
void CH8_ScanRestart(void);
void CH8_ScanProcess(int *testValue, int *referenceData, u8 mode, float kValue, float correctScale);
u8 CH8_GetChannelError(u8 channelIndex);
u8 CH8_IsScanBusy(void);

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
