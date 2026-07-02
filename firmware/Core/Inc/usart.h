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

extern UART_HandleTypeDef hlpuart1;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

/* USER CODE BEGIN Private defines */
/* USER CODE END Private defines */

/**
  * @brief  Initialize LPUART1 for the 433 MHz wireless module.
  * @retval None
  */
void MX_LPUART1_UART_Init(void);

/**
  * @brief  Initialize USART1 for the VM101 measurement module.
  * @param[in] baud USART1 baud rate.
  * @retval None
  */
void MX_USART1_UART_Init(u32 baud);

/**
  * @brief  Initialize USART2 for the Zigbee module.
  * @param[in] baud USART2 baud rate.
  * @retval None
  */
void MX_USART2_UART_Init(u32 baud);

/* USER CODE BEGIN Prototypes */
/**
  * @brief  Blocking transmit helper for a UART handle.
  * @param[in] uartHandle UART handle to transmit through.
  * @param[in] buffer Data buffer to transmit.
  * @param[in] len Number of bytes to transmit.
  * @retval None
  */
void Usart_Printf_Len(UART_HandleTypeDef* uartHandle, uint8_t *buffer,uint16_t len);

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __USART_H__ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
