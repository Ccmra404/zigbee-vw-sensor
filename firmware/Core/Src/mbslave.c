#include "mbslave.h"

/**
  * @brief  计算标准 Modbus RTU CRC16。
  * @param[in] buf 待校验数据缓冲区。
  * @param[in] len 参与校验的字节数。
  * @retval 计算得到的 CRC16 值；完整有效 RTU 帧返回 0。
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
  * @brief  清空 Modbus 接收缓冲区状态。
  * @param[out] port 待初始化的接收状态对象。
  * @retval 无
  */
void mbslave_init(MB_PARAM *port)
{
	memset(port->rx_buf, 0x00, MB_BUF_SIZE);
	port->rx_end_flg = 0;
}
