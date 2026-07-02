#include "app_delay.h"

/**
  * @brief  Busy-wait delay based on the HAL millisecond tick.
  * @param[in] nms Delay time in milliseconds.
  * @retval None
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
  * @brief  Busy-wait delay with microsecond resolution using SysTick.
  * @param[in] us Delay time in microseconds.
  * @retval None
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
