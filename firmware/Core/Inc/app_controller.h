#ifndef __APP_CONTROLLER_H
#define __APP_CONTROLLER_H

#include "main.h"
#include "mbslave.h"

extern u8 RxBuffer2[MB_BUF_SIZE];

extern __IO char MsgFlag1;
extern __IO char MsgFlag2;
extern __IO char MsgFlag3;

extern MB_PARAM mb_usart1_t;
extern MB_PARAM mb_usart2_t;
extern MB_PARAM mb_usart3_t;

/**
  * @brief  Run the CH8 application state machine.
  * @retval None
  */
void App_Run(void);

#endif
