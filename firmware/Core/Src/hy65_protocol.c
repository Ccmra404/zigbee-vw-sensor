#include "hy65_protocol.h"
#include "usart.h"

/**
  * @brief  璁＄畻 HY65 40 瀛楄妭鍗忚浣跨敤鐨勫紓鎴栨牎楠屽€笺€?  * @param[in] buffer 寰呮牎楠屾暟鎹紦鍐插尯銆?  * @param[in] size 鍙備笌鏍￠獙鐨勫瓧鑺傛暟銆?  * @retval 璁＄畻寰楀埌鐨?32 浣嶆牎楠屽€笺€?  */
unsigned long HY65_CheckSum40B(unsigned char *buffer, unsigned short size)
{
	unsigned long checksum = 0L;
	unsigned short div = 0;
	unsigned short mod = 0;
	unsigned short t_div = 0;
	unsigned short t_mod = 0;
	unsigned char *p = (unsigned char*)&checksum;

	div = size >> 2;
	mod = size & (unsigned short)0x03;

	for(t_div = 0;t_div < div;t_div++)
	{
		for(t_mod = 0;t_mod < 4;t_mod++)
		{
			p[t_mod] ^= buffer[(t_div << 2) + t_mod];
		}
	}
	for(t_mod = 0;t_mod < mod;t_mod++)
	{
		p[t_mod] ^= buffer[(t_div << 2) + t_mod];
	}

	return checksum;
}

/**
  * @brief  鍒濆鍖?40 瀛楄妭 ZDYH 鍙戦€佸抚妯℃澘銆?  * @param[out] txBuffer 鐩爣甯х紦鍐插尯锛岃嚦灏?40 瀛楄妭銆?  * @retval 鏃?  */
void HY65_InitZdyhTxBuffer(u8 *txBuffer)
{
	int i = 0;
	for(i = 0;i < 40;i++)
	{
		txBuffer[i] = 0;
	}

	txBuffer[0] = 'Z';
	txBuffer[1] = 'D';
	txBuffer[2] = 'Y';
	txBuffer[3] = 'H';
	txBuffer[4] = 40;
	txBuffer[5] = 0;
	txBuffer[10] = 0;
	txBuffer[11] = 1;
	txBuffer[16] = 0x0F;
	txBuffer[17] = 1;
	txBuffer[18] = 0;
	txBuffer[30] = 0x10;
	txBuffer[31] = 1;
	txBuffer[32] = 0x0A;
	txBuffer[33] = 0x3F;
}

/**
  * @brief  鍒濆鍖?65RX 鍝嶅簲甯фā鏉裤€?  * @param[out] txBuffer 鐩爣甯х紦鍐插尯锛岃嚦灏?18 瀛楄妭銆?  * @param[in] nodeNumber 鑺傜偣鍙风殑 3 涓?ASCII 鏁板瓧銆?  * @retval 鏃?  */
void HY65_Init65RxBuffer(u8 *txBuffer, const u8 *nodeNumber)
{
	txBuffer[0] = '6';
	txBuffer[1] = '5';
	txBuffer[2] = 'R';
	txBuffer[3] = 'X';
	txBuffer[4] = nodeNumber[0];
	txBuffer[5] = nodeNumber[1];
	txBuffer[6] = nodeNumber[2];
	txBuffer[7] = '0';
	txBuffer[8] = 'O';
	txBuffer[9] = '+';
	txBuffer[0x0A] = '0';
	txBuffer[0x0B] = '0';
	txBuffer[0x0C] = '0';
	txBuffer[0x0D] = '0';
	txBuffer[0x0E] = '0';
	txBuffer[0x0F] = '0';
	txBuffer[0x10] = 0x0F;
}

/**
  * @brief  濉厖 ZDYH 甯т腑鐨?ID 鍜屾尝鐗圭巼瀛楁銆?  * @param[in,out] txBuffer ZDYH 甯х紦鍐插尯銆?  * @param[in] data32 璁惧鎴栭€氶亾 ID銆?  * @param[in] groupId 褰撳墠缁勫彿銆?  * @param[in] currentChannelNumber 鏈妭鐐硅捣濮嬮€氶亾 ID銆?  * @param[in] uartBaudRate 鍐欏叆甯т腑鐨勪覆鍙ｆ尝鐗圭巼銆?  * @retval 鏃?  */
void HY65_AddIdBaudrateInfo(u8 *txBuffer, u32 data32, u32 groupId, u32 currentChannelNumber, u32 uartBaudRate)
{
	txBuffer[12] = (u8)data32;
	txBuffer[13] = (u8)(data32 >> 8);
	txBuffer[14] = (u8)(data32 >> 16);
	txBuffer[15] = (u8)(data32 >> 24);
	txBuffer[20] = (u8)(groupId + data32 - currentChannelNumber);
	txBuffer[21] = 0x86;
	txBuffer[26] = (u8)uartBaudRate;
	txBuffer[27] = (u8)(uartBaudRate >> 8);
	txBuffer[28] = (u8)(uartBaudRate >> 16);
	txBuffer[29] = (u8)(uartBaudRate >> 24);
}

/**
  * @brief  涓?ZDYH 甯у啓鍏?HY65 鏍￠獙瀛楁銆?  * @param[in,out] txBuffer ZDYH 甯х紦鍐插尯銆?  * @retval 鏃?  */
void HY65_AddCheckSum(u8 *txBuffer)
{
	unsigned long checksum = 0;
	checksum = HY65_CheckSum40B(txBuffer + 10,10);
	checksum ^= HY65_CheckSum40B(txBuffer + 20,20);
	txBuffer[6] = (u8)checksum;
	txBuffer[7] = (u8)(checksum >> 8);
	txBuffer[8] = (u8)(checksum >> 16);
	txBuffer[9] = (u8)(checksum >> 24);
}

/**
  * @brief  浣跨敤 65RX 鍝嶅簲甯у彂閫佹湁绗﹀彿鏁存暟娴嬮噺鍊笺€?  * @param[in,out] txBuffer 65RX 鍝嶅簲甯х紦鍐插尯銆?  * @param[in] measurement 寰呮牸寮忓寲涓?ASCII 鐨勬祴閲忓€笺€?  * @retval 鏃?  */
void HY65_SendRespInt(u8 *txBuffer, int measurement)
{
	char tmp[32];
	u8 checksum = 0;
	int i = 0;

	if(measurement >= 0)
	{
		sprintf(tmp, "%06d", measurement);
		txBuffer[9] = '+';
		txBuffer[0x0A] = tmp[0];
		txBuffer[0x0B] = tmp[1];
		txBuffer[0x0C] = tmp[2];
		txBuffer[0x0D] = tmp[3];
		txBuffer[0x0E] = tmp[4];
		txBuffer[0x0F] = tmp[5];
	}
	else
	{
		sprintf(tmp, "%07d", measurement);
		txBuffer[0x09] = tmp[0];
		txBuffer[0x0A] = tmp[1];
		txBuffer[0x0B] = tmp[2];
		txBuffer[0x0C] = tmp[3];
		txBuffer[0x0D] = tmp[4];
		txBuffer[0x0E] = tmp[5];
		txBuffer[0x0F] = tmp[6];
	}

	checksum = 21;
	for(i = 0;i < 13;i++)
	{
		checksum = checksum + txBuffer[i + 4];
	}
	txBuffer[0x11] = checksum;
	Usart_Printf_Len(&huart1,txBuffer,18);
}

/**
  * @brief  浣跨敤 65RX 鍝嶅簲甯у彂閫佹诞鐐规祴閲忓€笺€?  * @param[in,out] txBuffer 65RX 鍝嶅簲甯х紦鍐插尯銆?  * @param[in] measurement 寰呮牸寮忓寲涓?ASCII 鐨勬祴閲忓€笺€?  * @retval 鏃?  */
void HY65_SendRespFloat(u8 *txBuffer, float measurement)
{
	char tmp[32];
	u8 checksum = 0;
	int i = 0;

	sprintf(tmp, "%+06f", measurement);
	txBuffer[0x09] = tmp[0];
	txBuffer[0x0A] = tmp[1];
	txBuffer[0x0B] = tmp[2];
	txBuffer[0x0C] = tmp[3];
	txBuffer[0x0D] = tmp[4];
	txBuffer[0x0E] = tmp[5];
	txBuffer[0x0F] = tmp[6];

	checksum = 21;
	for(i = 0;i < 13;i++)
	{
		checksum = checksum + txBuffer[i + 4];
	}
	txBuffer[0x11] = checksum;
	Usart_Printf_Len(&huart1,txBuffer,18);
}
