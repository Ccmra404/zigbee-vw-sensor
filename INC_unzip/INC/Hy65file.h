//武汉华岩电子有限责任公司HY65系列位移传感器文件头格式
//修订时间:	2002-7-10
//
//本文件内所有结构都必须使用字(word)边界对齐,如果使用VC外的编译器请自行设置字(word)边界对齐方式 

#include "HYDZHEAD.h"
#include "HYDZFILE.h"

#ifndef __HY65FILE__
#define __HY65FILE__ 1
#ifdef _MSC_VER
#if _MSC_VER > 1000
#pragma pack(push)
#pragma pack(2)				
#endif
#endif


#define HY65_RECORD_VERSION		0x0100
#define HY65_RECORD_REMARK_SIZE	1024
#define HY65_RECORD_RESERVED	1024
//HY65系列产品纪录文件纪录数据类型
typedef struct HY65_FILE_RECORD_DATA_STRUCT
{	
	unsigned short	error_code;			//错误码
	unsigned long	time;				//时间
	unsigned long	tickcount;			//时间tickcount
	long			result;				//读数
	short			temperature;		//温度
}HY65_FILE_RECORD_DATA,*LPHY65_FILE_RECORD_DATA;
#define HY65_FILE_RECORD_DATA_SIZE	(sizeof(HY65_FILE_RECORD_DATA))

//HY65系列产品纪录文件头类型
typedef struct HY65_FILE_RECORD_STRUCT
{
	HYDZ_FILE_HEAD	hydz_file_head;					//通用文件头
	unsigned short	record_version;					//版本
	HYDZ_INFO		info;							//使用的传感器信息
	char record_remark[HY65_RECORD_REMARK_SIZE];	//记录备注
	char reserved[HY65_RECORD_RESERVED];			//保留
	//....以下为HY65系列产品纪录文件纪录数据类型HY65_FILE_RECORD_DATA
}HY65_FILE_RECORD,*LPHY65_FILE_RECORD;
#define HY65_FILE_RECORD_SIZE	(sizeof(HY65_FILE_RECORD))
#define HY65_FILE_RECORD_EXT	(".65R")			//HY65记录文件扩展文件名


#define HY65_PROJECT_VERSION		0x0100
#define HY65_PROJECT_REMARK_SIZE	1024
#define HY65_PROJECT_RESERVED		1024
//HY65系列产品PROJECT文件数据类型
typedef struct HY65_FILE_PROJECT_DATA_STRUCT
{
	unsigned long serial_number;						//记录文件使用的传感器号
	unsigned long datetime;								//记录文件生成时间
}HY65_FILE_PROJECT_DATA,*LPHY65_FILE_PROJECT_DATA;
#define HY65_FILE_PROJECT_DATA_SIZE	(sizeof(HY65_FILE_PROJECT_DATA))

//HY65系列产品PROJECT文件头类型
typedef struct HY65_FILE_PROJECT_STRUCT
{
	HYDZ_FILE_HEAD	hydz_file_head;						//通用文件头
	unsigned short	project_version;					//版本
	char project_remark[HY65_PROJECT_REMARK_SIZE];		//工程备注
	char reserved[HY65_PROJECT_RESERVED];				//保留
	//....以下为HY65系列产品PROJECT文件数据类型HY65_FILE_PROJECT_DATA
}HY65_FILE_PROJECT,*LPHY65_FILE_PROJECT;
#define HY65_FILE_PROJECT_SIZE	(sizeof(HY65_FILE_PROJECT))
#define HY65_FILE_PROJECT_EXT	(".65P")				//HY65工程文件扩展文件名

#ifdef _MSC_VER
#if _MSC_VER > 1000
#pragma pack(pop)				//恢复old边界对齐
#endif
#endif
#endif
