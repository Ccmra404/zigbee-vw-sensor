#include "mbslave.h"

/**
  * @brief  Calculate standard Modbus RTU CRC16.
  * @param[in] buf Data buffer to check.
  * @param[in] len Number of bytes to include.
  * @retval Calculated CRC16 value. A valid full RTU frame returns 0.
  */
uint16_t crc16_calc(uint8_t *buf, uint16_t len)
{
	uint16_t crc = 0xFFFF;
	uint16_t i = 0;

	while(len--)
	{
		crc ^= *buf++;
		for(i = 0;i < 8;i++)
		{
			if(crc & 0x0001)
			{
				crc = (crc >> 1) ^ 0xA001;
			}
			else
			{
				crc >>= 1;
			}
		}
	}

	return crc;
}

/**
  * @brief  Clear the Modbus receive buffer state.
  * @param[out] port Receive state object to initialize.
  * @retval None
  */
void mbslave_init(MB_PARAM *port)
{
	memset(port->rx_buf, 0x00, MB_BUF_SIZE);
	port->rx_end_flg = 0;
}
