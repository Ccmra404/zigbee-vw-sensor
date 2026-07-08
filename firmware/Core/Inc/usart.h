/**
  ******************************************************************************
  * @file    usart.h
  * @brief   This file contains all the function prototypes for
  *          the usart.c file
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USART_H__
#define __USART_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

/* USER CODE BEGIN Private defines */
/* USER CODE END Private defines */

/**
  * @brief  初始化 485/VM101 通信串口 USART1。
  * @param[in] baud USART1 波特率。
  * @retval 无
  */
void MX_USART1_UART_Init(u32 baud);

void MX_USART1_UART_InitEx(u32 baud, u32 parity);

/**
  * @brief  初始化 Zigbee 通信串口 USART2。
  * @param[in] baud USART2 波特率。
  * @retval 无
  */
void MX_USART2_UART_Init(u32 baud);

/* USER CODE BEGIN Prototypes */
/**
  * @brief  串口阻塞发送辅助函数。
  * @param[in] uartHandle 待发送的串口句柄。
  * @param[in] buffer 待发送数据缓冲区。
  * @param[in] len 待发送字节数。
  * @retval 无
  */
void Usart_Printf_Len(UART_HandleTypeDef* uartHandle, uint8_t *buffer,uint16_t len);

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __USART_H__ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
