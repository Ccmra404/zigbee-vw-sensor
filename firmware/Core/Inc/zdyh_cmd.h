#ifndef __ZDYH_CMD_H
#define __ZDYH_CMD_H

#include "main.h"
#include "ch8_measure.h"

typedef enum
{
	ZDYH_NOT_MATCH = 0,
	ZDYH_HANDLED,
	ZDYH_CONTINUE
} ZDYH_RESULT;

/**
  * @brief  Process one 40-byte ZDYH protocol command frame.
  * @param[in] rxBuffer Received UART buffer.
  * @param[in,out] txBuffer Response buffer.
  * @param[in,out] testValue CH8 measurement buffer.
  * @param[in,out] bcState Broadcast response state.
  * @param[in,out] bcResp Broadcast channel response mask.
  * @param[in,out] data32Bak Previous broadcast response ID.
  * @param[in,out] lastCmd Last accepted command code.
  * @param[in,out] lastMsg Last accepted command payload.
  * @param[in,out] errorCode Current measurement/protocol error code.
  * @param[in,out] measureFlag Deferred measurement request flag.
  * @retval ZDYH_NOT_MATCH if the frame is not ZDYH; otherwise handling status.
  */
ZDYH_RESULT ZDYH_Process(u8 *rxBuffer, u8 *txBuffer, int *testValue,
	u8 *bcState, u8 *bcResp, u32 *data32Bak,
	uint8_t *lastCmd, uint8_t *lastMsg,
	u8 *errorCode, u8 *measureFlag);

#endif
