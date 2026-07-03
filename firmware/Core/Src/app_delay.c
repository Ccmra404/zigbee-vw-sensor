#include "app_delay.h"

/**
  * @brief  基于 HAL 毫秒节拍的阻塞延时。
  * @param[in] nms 延时时间，单位为毫秒。
  * @retval 无
  */
void delay_ms(u32 nms)
{
	u32 tmrcnt = uwTick;

	while(1)
	{
		if(uwTick - tmrcnt >= nms)
		{
			break;
		}
	}
}

/**
  * @brief  基于 SysTick 的微秒级阻塞延时。
  * @param[in] us 延时时间，单位为微秒。
  * @retval 无
  */
void delayMicroseconds(uint32_t us)
{
	__IO uint32_t currentTicks = SysTick->VAL;
	const uint32_t tickPerMs = SysTick->LOAD + 1;
	const uint32_t nbTicks = ((us - ((us > 0) ? 1 : 0)) * tickPerMs) / 1000;
	uint32_t elapsedTicks = 0;
	__IO uint32_t oldTicks = currentTicks;

	do
	{
		currentTicks = SysTick->VAL;
		elapsedTicks += (oldTicks < currentTicks) ? tickPerMs + oldTicks - currentTicks : oldTicks - currentTicks;
		oldTicks = currentTicks;
	} while(nbTicks > elapsedTicks);
}
