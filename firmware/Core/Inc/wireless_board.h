#ifndef __WIRELESS_BOARD_H
#define __WIRELESS_BOARD_H

#include "main.h"

#define WIRELESS_ZIGBEE_FACTORY_BAUDRATE 38400U
#define WIRELESS_ZIGBEE_RUNTIME_BAUDRATE 19200U
#define WIRELESS_VM101_BAUDRATE          9600U
#define WIRELESS_ZIGBEE_DEBUG_RX_SIZE    32U
#define WIRELESS_ZIGBEE_DIAG_MODE        0U
#define WIRELESS_ZIGBEE_TRANSPARENT_TEST 1U
#define WIRELESS_ZIGBEE_DIAG_SLOT_COUNT  40U
#define WIRELESS_ZIGBEE_DIAG_RX_SIZE     24U
#define WIRELESS_ZIGBEE_TRANSPARENT_RX_SIZE 64U

extern volatile u32 ZigbeeDebugLastBaud;
extern volatile int ZigbeeDebugLastLen;
extern volatile u8 ZigbeeDebugLastRx[WIRELESS_ZIGBEE_DEBUG_RX_SIZE];
extern volatile int ZigbeeDebugFactoryLen;
extern volatile int ZigbeeDebugFactoryFrameCount;
extern volatile u8 ZigbeeDebugFactoryRx[WIRELESS_ZIGBEE_DEBUG_RX_SIZE];
extern volatile u8 ZigbeeDebugCaptureFactoryEnabled;
extern volatile u32 ZigbeeDebugAttemptCount;
extern volatile u32 ZigbeeDebugLastStatus;
extern volatile u32 ZigbeeDiagMagic;
extern volatile u32 ZigbeeDiagCycle;
extern volatile u32 ZigbeeDiagSlotCount;
extern volatile u32 ZigbeeDiagHitMaskLow;
extern volatile u32 ZigbeeDiagHitMaskHigh;
extern volatile u32 ZigbeeDiagLastSlot;
extern volatile u32 ZigbeeDiagBaud[WIRELESS_ZIGBEE_DIAG_SLOT_COUNT];
extern volatile u8 ZigbeeDiagCmd[WIRELESS_ZIGBEE_DIAG_SLOT_COUNT];
extern volatile u8 ZigbeeDiagLen[WIRELESS_ZIGBEE_DIAG_SLOT_COUNT];
extern volatile u8 ZigbeeDiagAck[WIRELESS_ZIGBEE_DIAG_SLOT_COUNT];
extern volatile u8 ZigbeeDiagRx[WIRELESS_ZIGBEE_DIAG_SLOT_COUNT][WIRELESS_ZIGBEE_DIAG_RX_SIZE];
extern volatile u32 ZigbeeTransparentMagic;
extern volatile u32 ZigbeeTransparentBaud;
extern volatile u32 ZigbeeTransparentTxCount;
extern volatile u32 ZigbeeTransparentRxCount;
extern volatile u32 ZigbeeTransparentLastLen;
extern volatile u8 ZigbeeTransparentLastRx[WIRELESS_ZIGBEE_TRANSPARENT_RX_SIZE];

/**
  * @brief  鍒濆鍖栨垨鏍￠獙 Zigbee 妯″潡锛涘け璐ユ椂鍋滅暀鍦ㄦ晠闅滈棯鐏姸鎬併€?  * @retval 鏃?  */
void Wireless_InitZigbeeOrFault(void);

/**
  * @brief  鍒濆鍖栨甯歌繍琛岄樁娈典娇鐢ㄧ殑 GPIO 鍜?UART銆?  * @retval 鏃?  */
void Wireless_StartRuntimeUarts(void);

#endif
