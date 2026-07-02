#ifndef __HY65_ASCII_CMD_H
#define __HY65_ASCII_CMD_H

#include "main.h"
#include "ch8_measure.h"

typedef enum
{
	HY65_ASCII_NOT_MATCH = 0,
	HY65_ASCII_HANDLED,
	HY65_ASCII_CONTINUE
} HY65_ASCII_RESULT;

/**
  * @brief  Process one HY65 ASCII 65TX command frame.
  * @param[in] rxBuffer Received UART buffer.
  * @param[in,out] txBuffer Response buffer.
  * @param[in] msgLen Received frame length.
  * @param[in,out] testValue CH8 measurement buffer.
  * @param[in,out] errorCode Current measurement/protocol error code.
  * @param[in,out] measureFlag Deferred measurement request flag.
  * @retval HY65_ASCII_NOT_MATCH if the frame is not 65TX; otherwise handling status.
  */
HY65_ASCII_RESULT HY65_AsciiProcess(u8 *rxBuffer, u8 *txBuffer, int msgLen, int *testValue, u8 *errorCode, u8 *measureFlag);

#endif
