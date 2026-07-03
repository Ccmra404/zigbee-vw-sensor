#ifndef __APP_DELAY_H
#define __APP_DELAY_H

#include "main.h"

/**
  * @brief  基于 HAL 毫秒节拍的阻塞延时。
  * @param[in] nms 延时时间，单位为毫秒。
  * @retval 无
  */
void delay_ms(u32 nms);

/**
  * @brief  基于 SysTick 的微秒级阻塞延时。
  * @param[in] us 延时时间，单位为微秒。
  * @retval 无
  */
void delayMicroseconds(uint32_t us);

#endif
