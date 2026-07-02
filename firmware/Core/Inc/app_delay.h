#ifndef __APP_DELAY_H
#define __APP_DELAY_H

#include "main.h"

/**
  * @brief  Busy-wait delay based on the HAL millisecond tick.
  * @param[in] nms Delay time in milliseconds.
  * @retval None
  */
void delay_ms(u32 nms);

/**
  * @brief  Busy-wait delay with microsecond resolution using SysTick.
  * @param[in] us Delay time in microseconds.
  * @retval None
  */
void delayMicroseconds(uint32_t us);

#endif
