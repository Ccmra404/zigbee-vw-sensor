//武汉华岩电子有限责任公司产品通用头格式定义文件
//修订时间:	2003-3-2
//
//本文件内所有结构都必须使用字(word)边界对齐,如果使用VC外的编译器请自行设置字(word)边界对齐方式 

#ifndef __HYDZHEAD__
#define __HYDZHEAD__	1
#ifdef _MSC_VER
#if _MSC_VER > 1000
#pragma pack(push)
#pragma pack(2)							
#endif
#endif

#define HARDWARE_VERSION			0x01				//传感器硬件版本号

//TYPE
/*strcut TYPE
{
	short sub_sub_type:4;		//mask 0x000f			//HYDZ产品软件控制类型
	short sub_type:4;			//mask 0x00f0			//HYDZ产品硬件子系列类型
	short main_type:8;			//mask 0xff00			//HYDZ产品硬件主系列类型
};*/
//TYPE_HY65(type是软件命令结构的类型包含产品类型,hardware是产品规格)
//HY65系列位移传感器软件控制
#define TYPE_BASE					0x0000
#define TYPE_HY65					(TYPE_BASE+0x0100)		
#define TYPE_HY65_CLIENT			(TYPE_HY65+0x0000)			//HYDZ HY65系列传感器硬件类型定义
#define TYPE_HY65_CLIENT_COMMAND	(TYPE_HY65_CLIENT+0x0000)	//HYDZ HY65系列传感器软件命令"控制"类型定义
#define TYPE_HY65_CLIENT_BD			(TYPE_HY65_CLIENT+0x0001)	//HYDZ HY65系列传感器软件命令"标定"类型定义
#define TYPE_HY65_CLIENT_DOWNLOAD	(TYPE_HY65_CLIENT+0x0002)	//HYDZ HY65系列传感器软件命令"下载"类型定义
#define TYPE_HY65_CLIENT_UPDATE		(TYPE_HY65_CLIENT+0x0003)	//HYDZ HY65系列传感器软件命令"更新"类型定义
#define TYPE_HY65_PROXY				(TYPE_HY65+0x0010)			//HYDZ HY65系列传感器控制节点硬件类型定义
#define TYPE_HY65_PROXY_COMMAND		(TYPE_HY65_PROXY+0x0000)	//HYDZ HY65系列传感器控制节点软件命令"控制"类型定义
#define TYPE_HY65_PROXY_BD			(TYPE_HY65_PROXY+0x0001)	//HYDZ HY65系列传感器控制节点软件命令"标定"类型定义

//Next Sample
//#define TYPE_HY31					(TYPE_BASE+0x0200)
//#define TYPE_HY31_CLIENT			(TYPE_HY31+0x0000)
//#define TYPE_HY31_CLIENT_COMMAND	(TYPE_HY31_CLIENT+0x0000)
//#define TYPE_HY31_CLIENT_DOWNLOAD	(TYPE_HY31_CLIENT+0x0001)
//#define TYPE_HY31_SERVER			(TYPE_HY31+0x0010)

//#define TYPE_????					(TYPE_BASE+0x??00)
//#define TYPE_????_????			(TYPE_????+0x00?0)
//#define TYPE_????_????_????		(TYPE_????+0x000?)
#define TYPE_MASK				0xfff0			//info.type&TYPE_MASK=产品硬件类型

//hardware(产品硬件规格)
//HY65系列位移传感器硬件规格
#define HARDWARE_UNKNOWN		0x00
#define HARDWARE_HY65_1			0x01
#define HARDWARE_HY65_5			0x02
#define HARDWARE_HY65_10		0x03
#define HARDWARE_HY65_30		0x04
#define HARDWARE_HY65_50		0x05
#define HARDWARE_HY65_100		0x06
#define HARDWARE_HY65_200		0x07
#define HARDWARE_HY65_500		0x08
#define HARDWARE_HY65_1000		0x09
#define HARDWARE_HY65_2000		0x0a

#define HARDWARE_HY65_YB		0x0f

#ifndef	HYDZ_TYPE
typedef	unsigned short	HYDZ_TYPE;				//HYDZ产品类型定义
#endif
#ifndef	HYDZ_SERIAL_NUMBER
typedef	unsigned long	HYDZ_SERIAL_NUMBER;		//HYDZ产品序列号定义
#endif
#ifndef	HYDZ_ERROR_CODE
typedef	unsigned short	HYDZ_ERROR_CODE;		//HYDZ产品错误码定义
#endif

typedef	struct	HYDZ_INFO_STRUCT				//HYDZ产品信息结构,包含华岩产品硬件信息
{
	HYDZ_TYPE			type;					//HYDZ产品类型或HYDZ产品软件控制类型
	HYDZ_SERIAL_NUMBER 	serial_number;			//HYDZ产品序列号
	unsigned char		hardware;				//HYDZ硬件类型
	unsigned char 		version;				//HYDZ硬件版本
}HYDZ_INFO,*LPHYDZ_INFO;
#define HYDZ_INFO_SIZE	(sizeof(HYDZ_INFO))

typedef struct HYDZHEAD_STRUCT					//HYDZ产品通用软件控制命令头结构
{
	unsigned long		hydzhead_flag;			//HYDZ结构头标志,必须为HYDZHEAD_FLAG
	unsigned short 		size;					//数据块大小,=HYDZHEAD结构长度(sizeof(HYDZHEAD_STRUCT))+sizeof(具体结构内容)的总长度	
	unsigned long 		checksum;				//数据块校验和,=从&HYDZHEAD.info起到数据块结束以CheckSum_Fuc(&HYDZHEAD.info,size-sizeof(size)-sizeof(checksum)-sizeof(hydzhead_flag))运算得出的数值
	HYDZ_INFO			info;					//HYDZ产品信息结构
	HYDZ_ERROR_CODE		error_code;				//错误码
	//以下为具体数据内容......
}HYDZHEAD,*LPHYDZHEAD;

//所有向HYDZ产品发送或由HYDZ产品返回的数据块都将是以下结构
/*
	struct ???
	{
		HYDZHEAD	hydzhead;
		.....						//.....可以是任何类型,任何长度<=65535-HYDZHEAD_SIZE的数据
	}
*/

//#define HYDZHEAD_FLAG		((unsigned long)'HYDZ')
#define HYDZHEAD_FLAG		0x4859445aL				//HYDZ产品头标志
#define HYDZHEAD_SIZE		(sizeof(HYDZHEAD))		//HYDZ产品通用软件控制命令头结构大小
#define SIZE_MIN			HYDZHEAD_SIZE


//HYDZ通用错误error_code(0x0000--0x00ff)
#define HYDZ_ERR		TYPE_BASE
#define OK_ERR			(HYDZ_ERR+0x0000)			//命令正确完成
#define TYPE_ERR		(HYDZ_ERR+0x0001)			//type类型未定义错误或size与type不匹配,或系统不能处理当前软件控制类型
#define BUSY_ERR        (HYDZ_ERR+0x0002)   		//系统忙,有一个必须要处理的命令正在执行中
#define SERIAL_ERR     	(HYDZ_ERR+0x0003)   	   	//serial_number错误,表示此命令需要使用serial_number但设置的serial_number错误,请正确设置serial_number,
#define CHECKSUM_ERR	(HYDZ_ERR+0x0004)			//数据传输校验错误
#define SIZE_ERR		(HYDZ_ERR+0x0005)			//size<SIZE_MIN错误或当前软件控制类型要求接受的bytes数与size不符
#define COMMAND_ERR		(HYDZ_ERR+0x0006)			//命令码错误
#define NORETURN_ERR	(HYDZ_ERR+0x0007)			//不可返回错误,HYDZ产品返回的数据块中error_code绝不可能=NORETURN_ERR,在向HYDZ产品发送数据块时建议使数据块中error_code=NORETURN_ERR,以分辨返回的数据是否是由HYDZ产品返回

#ifdef __cplusplus
extern "C" {
#endif
//必须在HYDZ产品通用软件控制命令头结构phydzhead和pdata指向的数据块填充完毕,准备发送前调用此函数
unsigned long CheckSum(LPHYDZHEAD phydzhead,void* pdata);
//unsigned long CheckSum_Fuc(unsigned char*pbuf,unsigned short size);
#ifdef __cplusplus
}
#endif


#ifdef _MSC_VER
#if _MSC_VER > 1000
#pragma pack(pop)				//恢复old边界对齐
#endif
#endif
#endif