//武汉华岩电子有限责任公司产品通用头格式校验
//修订时间:	2002-7-10
//

#include "HYDZHEAD.h"
#include "HY65CLIENT.h"

unsigned long CheckSum_Fuc(unsigned char*pbuf,unsigned short size)
{
	unsigned long checksum=0L;
   	unsigned short div,mod,t_div,t_mod;
   	unsigned char*p=(unsigned char*)&checksum;
	div=size>>2;						//div=size/sizeof(long)
        mod=size&(unsigned short)0x03;				//mod=size%sizeof(long)

        for(t_div=0;t_div<div;t_div++)
        	for(t_mod=0;t_mod<4;t_mod++)			//for(t_mod=0;t_mod<sizeof(long);t_mod++)
			p[t_mod]^=pbuf[(t_div<<2)+t_mod];	//p[t_mod]^=pbuf[(t_div*sizeof(long))+t_mod];
        for(t_mod=0;t_mod<mod;t_mod++)
        	p[t_mod]^=pbuf[(t_div<<2)+t_mod];
        return checksum;
}

unsigned long CheckSum(LPHYDZHEAD phydzhead,void*pdata)
{
	unsigned long checksum;
	checksum=CheckSum_Fuc((unsigned char*)&phydzhead->checksum+sizeof(phydzhead->checksum),(unsigned short)(HYDZHEAD_SIZE-((unsigned char*)&phydzhead->checksum-(unsigned char*)phydzhead+sizeof(phydzhead->checksum))));
	checksum^=CheckSum_Fuc((unsigned char*)pdata,(unsigned short)(phydzhead->size-HYDZHEAD_SIZE));
	return checksum;
}

unsigned long HY65_CheckSum(LPHY65_CLIENT phy65_client)
{
	return CheckSum((LPHYDZHEAD)phy65_client,(unsigned char*)phy65_client+HYDZHEAD_SIZE);
}