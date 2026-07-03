#ifndef __WIRELESS_BOARD_H
#define __WIRELESS_BOARD_H

#include "main.h"

#define WIRELESS_ZIGBEE_FACTORY_BAUDRATE 38400U
#define WIRELESS_ZIGBEE_RUNTIME_BAUDRATE 19200U
#define WIRELESS_VM101_BAUDRATE          9600U

/**
  * @brief  杩愯闃舵鎵撳紑 Zigbee銆佷紶鎰熷櫒鐢垫簮鍜?VM101 鍚庣骇鐢垫簮銆?  * @retval 鏃?  */

/**
  * @brief  鍒濆鍖栨垨鏍￠獙 Zigbee 妯″潡锛涘け璐ユ椂鍋滅暀鍦ㄦ晠闅滈棯鐏姸鎬併€?  * @retval 鏃?  */
void Wireless_InitZigbeeOrFault(void);

/**
  * @brief  鍒濆鍖栨甯歌繍琛岄樁娈典娇鐢ㄧ殑 GPIO銆佺數婧愬拰 UART銆?  * @retval 鏃?  */
void Wireless_StartRuntimeUarts(void);

#endif
