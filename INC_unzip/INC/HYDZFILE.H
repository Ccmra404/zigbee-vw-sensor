//武汉华岩电子有限责任公司产品通用文件头格式
//修订时间:	2002-7-10
//
//本文件内所有结构都必须使用字(word)边界对齐,如果使用VC外的编译器请自行设置字(word)边界对齐方式 


#include "HYDZHEAD.h"

#ifndef __HYDZFILE__
#define __HYDZFILE__
#ifdef _MSC_VER
#if _MSC_VER > 1000
#pragma pack(push)
#pragma pack(2)				
#endif
#endif

//*****************************************************************************************

//文件版本
#define HYDZ_FILE_VERSION	0x0100

//HYDZ通用文件头
typedef struct HYDZ_FILE_HEAD_STRUCT
{
	unsigned long 		hydzhead_flag;			//hydz标志头=HYDZHEAD_FLAG
	unsigned long 		file_flag;				//文件类型标志	example:'65BD'
	unsigned short		hydz_version;			//文件头版本=HYDZ_FILE_VERSION	
	unsigned long 		hydz_file_size_low;		//文件长度=文件总长度(低DWORD)
	unsigned long 		hydz_file_size_high;	//文件长度=文件总长度(高DWORD)
	unsigned short		hydz_file_head_size;	//文件头长度=HYDZ_FILE_HEAD_SIZE(通用文件头)+特定文件头长度
	unsigned long 		checksum;				//文件校验和=HYDZHEAD_CheckSum(....)
	
}HYDZ_FILE_HEAD,*LPHYDZ_FILE_HEAD;
#define HYDZ_FILE_HEAD_SIZE	sizeof(HYDZ_FILE_HEAD)


//文件类型标志
#define FILE_FLAG_HY65_BD		((unsigned long)'65BD')		//HY65系列产品BD文件(HY65_FILE_BD_EXT)
#define FILE_FLAG_HY65_RECORD	((unsigned long)'65RC')		//HY65系列产品纪录文件
#define FILE_FLAG_HY65_PROJECT  ((unsigned long)'65PJ')		//HY65系列产品工程文件
//.....


//文件检查和计算函数
#ifdef __cplusplus
extern "C" {
#endif
unsigned long HYDZFILE_CheckSum(LPHYDZ_FILE_HEAD lphydz_file_head,void* pdata);
#ifdef __cplusplus
}
#endif

//*****************************************************************************************
#ifdef _MSC_VER
#if _MSC_VER > 1000
#pragma pack(pop)				//恢复old边界对齐
#endif
#endif
#endif