#include "HYDZHEAD.h"

#ifndef __ADMIN__
#define __ADMIN__	1
#endif

#ifndef __HY65CLIENT__
#define __HY65CLIENT__	1
#ifdef _MSC_VER
#if _MSC_VER > 1000
#pragma pack(push)
#pragma pack(2)				//2 bytes边界对齐the same as 430 does
#endif
#endif

typedef struct HY65_COMMCONFIG_STRUCT
{
        unsigned long   	baud;          				//波特率        
        unsigned char		bits;					//位设置
        unsigned char   	comm_mode;                
}HY65_COMMCONFIG,*LPHY65_COMMCONFIG;
#define HY65_COMMCONFIG_SIZE	(sizeof(HY65_COMMCONFIG))
//HY65_COMMCONFIG.comm_mode
#define HY65_COMM_MODE_ALL	0					//重设baud&bits;
#define HY65_COMM_MODE_BAUDONLY	1					//只设定baud,忽略bits
#define HY65_COMM_MODE_BITSONLY	2					//只设定bits,忽略baud
//HY65_COMMCONFIG.baud
#define HY65_BAUD_115200	115200L					//default
#define HY65_BAUD_57600		57600L
#define HY65_BAUD_38400		38400L
#define HY65_BAUD_28800		28800L
#define HY65_BAUD_19200		19200L
#define HY65_BAUD_14400		14400L
#define HY65_BAUD_9600		9600L
#define HY65_BAUD_4800		4800L
#define HY65_BAUD_2400		2400L
#define HY65_BAUD_1200		1200L
#define HY65_BAUD_MIN		HY65_BAUD_1200
#define HY65_BAUD_MAX		HY65_BAUD_115200			//最大极限921600
//HY65_COMMCONFIG.bits
#define HY65_BITS_DATA_7		0				//数据位=7
#define HY65_BITS_DATA_8		0x10				//数据位=8,default
#define HY65_BITS_STOP_1		0				//停止位=1,default
#define HY65_BITS_STOP_2		0x20				//停止位=2
#define HY65_BITS_PARITY_ODD		0				//奇校验
#define HY65_BITS_PARITY_EVEN		0x40				//偶校验
#define HY65_BITS_PARITY_DISABLE	0				//不使用校验,default
#define HY65_BITS_PARITY_ENABLE		0x80				//使用校验

typedef struct HY65_SETZERO_STRUCT
{
        unsigned short 		setzero_mode;				//设置零点模式
        long    		setzero_data;				//设置零点数据
}HY65_SETZERO,*LPHY65_SETZERO;
#define HY65_SETZERO_SIZE	(sizeof(HY65_SETZERO))
//HY65_SETZERO.setzero_mode
#define SETZERO_CUR            	0x00					//设置零点=当前读数,并存储新的零点为当前零点,ignore setzero_data,
#define SETZERO_ADD            	0x01					//设置零点=当前零点+setzero_data,并存储新的零点为当前零点,need setzero_data
#define SETZERO_SUB            	0x02					//设置零点=当前零点-setzero_data,并存储新的零点为当前零点,need setzero_data
#define SETZERO_SET            	0x03					//设置零点使得当前读数=setzero_data(zero=setzero_data+result),并存储新的零点为当前零点,need setzero_data
#define SETZERO_LAST		0x04					//设置零点=当前零点的上一个零点(即最近一次使用SETZERO_SET,SETZERO_SUB,SETZERO_ADD或SETZERO_CUR,SETZERO_DEFAULT存储的零点之前的一个零点),ignore setzero_data
#define SETZERO_DEFAULT		0x05					//设置零点=出厂零点,ignore setzero_data,并存储新的零点为当前零点
                
typedef struct HY65_STATUS_STRUCT
{
	HY65_COMMCONFIG		commconfig;				//通讯设置
	unsigned short		flag;					//内部标志
	short			temperature;				//温度,如果=TEMPERATURE_INVALID,此温度无效
	long			result;					//传感器读数
	
}HY65_STATUS,*LPHY65_STATUS;
#define HY65_STATUS_SIZE	(sizeof(HY65_STATUS))
#define TEMPERATURE_INVALID	0x7fff					//温度无效标志

#define HY65_SETSIGNAL_MAX      8                                      //信号最多8 个
typedef struct HY65_SIGNALDATA_STRUCT					//信号数据结构
{
	unsigned long	mode;						//信号设置方式
	long	data;							//信号设置数值(开关方式的开关限定值或状态逻辑方式的范围1)
	long	data2;							//信号设置数值(状态逻辑方式的范围2),开关方式忽略
}HY65_SIGNALDATA,*LPHY65_SIGNALDATA;
#define HY65_SIGNALDATA_SIZE	(sizeof(HY65_SIGNALDATA))
//触发允许
#define HY65_SIGNALDATA_MODE_SIGNAL1ENABLE	0x00000001			//信号1允许触发
#define HY65_SIGNALDATA_MODE_SIGNAL1DISABLE	0x00000000			//信号1禁止触发
#define HY65_SIGNALDATA_MODE_SIGNAL2ENABLE	0x00000002			//信号2允许触发
#define HY65_SIGNALDATA_MODE_SIGNAL2DISABLE	0x00000000			//信号2禁止触发
#define HY65_SIGNALDATA_MODE_SIGNAL3ENABLE	0x00000004			//信号3允许触发
#define HY65_SIGNALDATA_MODE_SIGNAL3DISABLE	0x00000000			//信号3禁止触发
//触发电平状态
#define HY65_SIGNALDATA_MODE_SIGNAL1HIGH	0x00000008			//信号1满足条件后电平为高
#define HY65_SIGNALDATA_MODE_SIGNAL1LOW		0x00000000			//信号1满足条件后电平为低
#define HY65_SIGNALDATA_MODE_SIGNAL2HIGH	0x00000010			//信号2满足条件后电平为高
#define HY65_SIGNALDATA_MODE_SIGNAL2LOW		0x00000000			//信号2满足条件后电平为低
#define HY65_SIGNALDATA_MODE_SIGNAL3HIGH	0x00000020			//信号3满足条件后电平为高
#define HY65_SIGNALDATA_MODE_SIGNAL3LOW		0x00000000			//信号3满足条件后电平为低
//开关逻辑方式触发条件，状态逻辑方式忽略
#define HY65_SIGNALDATA_MODE_SIGNAL1TOGGLEH	0x00000040			//信号1在大于等于data设置值时触发
#define HY65_SIGNALDATA_MODE_SIGNAL1TOGGLEL	0x00000000			//信号1在小于等于data设置值时触发
#define HY65_SIGNALDATA_MODE_SIGNAL2TOGGLEH	0x00000080			//信号2在大于等于data设置值时触发
#define HY65_SIGNALDATA_MODE_SIGNAL2TOGGLEL	0x00000000			//信号2在小于等于data设置值时触发
#define HY65_SIGNALDATA_MODE_SIGNAL3TOGGLEH	0x00000100			//信号3在大于等于data设置值时触发
#define HY65_SIGNALDATA_MODE_SIGNAL3TOGGLEL	0x00000000			//信号3在小于等于data设置值时触发
//触发方式
#define HY65_SIGNALDATA_MODE_A			0x80000000			//开关逻辑方式
#define HY65_SIGNALDATA_MODE_B			0x00000000			//状态逻辑方式

typedef struct HY65_SETSIGNAL_STRUCT					//信号设置结构
{
	unsigned char signal_command;					//信号设置命令
	unsigned char signal_number;					//信号设置编号，0-(HY65_SETSIGNAL_MAX-1)共HY65_SETSIGNAL_MAX个
	HY65_SIGNALDATA	signal_data;	
}HY65_SETSIGNAL,*LPHY65_SETSIGNAL;
#define HY65_SETSIGNAL_SIZE	(sizeof(HY65_SETSIGNAL))

#define HY65_SETSIGNAL_SIGNAL_COMMAND_SET	0				//设置信号命令
#define HY65_SETSIGNAL_SIGNAL_COMMAND_GET	1			//取回信号命令
#define HY65_SETSIGNAL_SIGNAL_COMMAND_CLEAR	2			//清除信号命令
#define HY65_SETSIGNAL_SIGNAL_COMMAND_CLEARALL	3			//清除所有信号命令


typedef union HY65_CLIENTCONFIG_UNION
{
	unsigned char		callback_time;			//设定返回延时(单位:1/2 ms)
	unsigned char 		group_number;			//将要设定的工作组号码(1-255)
	unsigned char		enable;					//if TRUE enable,FALSE disable
        HY65_COMMCONFIG		commconfig;			//传感器通讯设置
        HY65_SETZERO     	setzero;       			//传感器零点设置
        HY65_STATUS		status;				//传感器当前状态
        HY65_SETSIGNAL		setsignal;			//传感器信号设置
        
        
        unsigned char		reserved[14];
        
}HY65_CLIENTCONFIG,*LPHY65_CLIENTCONFIG;
#define HY65_CLIENTCONFIG_SIZE	(sizeof(HY65_CLIENTCONFIG))
   
typedef struct HY65_CLIENT_STRUCT
{
        HYDZHEAD		hydzhead;				//hydzhead.type=TYPE_HY65_CLINT_COMMAND
        unsigned char 		group_number;				//工作组号码(1-255),若为0所有在线传感器都响应,如果命令代码设置了S_NEED_SERIAL这将被忽略
        unsigned char 		command;                		//命令代码
        unsigned long		timeflag;				//时间标记,传感器内部忽略此项并且不会对它进行修改,用于判断命令是否被传感器正确收到;Example:在命令A发出时timeflag=GetTickCount(),发出后取得C_HY65_LASTERROR时若timeflag=发出A命令时的timeflag则A命令证实被传感器收到
        HY65_CLIENTCONFIG 	clientconfig;    			//设置
}HY65_CLIENT,*LPHY65_CLIENT;
#define HY65_CLIENT_SIZE	(sizeof(HY65_CLIENT))
//HY65_CLIENT.command,除C_HY65_LASTERROR,C_HY65_RESULT外所有命令都会对error_code有影响,C_HY65_SERIAL必须与其他命令联合使用不能单独使用
#define C_HY65_SERIAL		0x80					//需要使用serial_number,忽略group,不使用此命令则使用group忽略serial_number,此命令不能单独使用,需要和其他命令OR
#define C_HY65_LASTERROR	0x00					//返回传感器状态,此命令主要用于取得传感器上次正确收到的命令的error_code
									//发回的HY65_CLIENT中除了error_code,checksum外,所有内容都是上一个被传感器正确收到的HY65_CLIENT
									
#define C_HY65_RESET		0x01					//传感器复位
#define C_HY65_SAVE		0x02					//存储传感器的当前设置(包括传感器当前的串口通讯设置(COMMCONFIG)及工作组号码(group))
#define C_HY65_COMM  		0x03					//传感器串口通讯设置(使用HY65_CLIENT.clientconfig.comm)
#define C_HY65_SETZERO          0x04					//设置零点(使用HY65_CLIENT.clientconfig.setzero)
#define C_HY65_GROUP		0x05					//设置工作组号码(使用HY65_CLIENT.clientconfig.group_number)
#define C_HY65_STATUS		0x06					//取回传感器当前状态(使用HY65_CLIENT.clientconfig.status)
#define C_HY65_CALLBACKTIME	0x07					//设置回调时间(单位:1/2ms),影响C_HY65_LASTERROR和C_HY65_STATUE的返回时间;default=0(if =0,根据baud不同自动修正);Example:callbacktime=5,传感器正确收到C_HY65_LASTERROR或C_HY65_STATUS命令后5*1/2=2.5ms后才返回
#define C_HY65_ENABLE		0x08					//传感器允许或禁止(这个命令其实就是C_HY65_ADMIN_CC的简化版,不过只能ENABLE所有非ADMIN命令或DISABLE所有非ADMIN命令),当DISABLE时传感器将忽略除C_HY65_ENABLE外的所有非ADMIN命令
#define C_HY65_SIGNAL		0x09					//传感器触发信号设置

#define C_HY65_COMMAND_LAST		0x09				//最后一个命令index
#define C_HY65_COMMAND_NUMBER	(C_HY65_COMMAND_LAST+1)			//命令数目

/*ADMIN command*/
#define C_HY65_ADMIN		0x40
#define C_HY65_ADMIN_DOWNLOAD	(C_HY65_ADMIN+0x00)			//sensor download
#define C_HY65_ADMIN_UPDATE	(C_HY65_ADMIN+0x01)			//sensor update
#define C_HY65_ADMIN_BD         (C_HY65_ADMIN+0x02)			//sensor bd
#define C_HY65_ADMIN_CLEAR	(C_HY65_ADMIN+0x03)			//sensor clear
#define C_HY65_ADMIN_RC		(C_HY65_ADMIN+0x04)			//result enable control
#define C_HY65_ADMIN_CC		(C_HY65_ADMIN+0x05)			//command enable control
#define C_HY65_ADMIN_AUTOBD     (C_HY65_ADMIN+0x06)			//自动标定设置

//一般错误(0x0100-0x017f)
#define HY65_RESULT_INVALIED_ERR	(TYPE_HY65+0x0000)		//读数无效(传感器工作异常)
#define HY65_RESULT_OUTRANGE_ERR	(TYPE_HY65+0x0001)		//读数量程范围外
#define HY65_SETZERO_FLASH_ERR		(TYPE_HY65+0x0002)		//置零时flash错误
#define HY65_SETZERO_MODE_ERR		(TYPE_HY65+0x0003)		//setzero_mode must=HY65_SETZERO_MODE_????
#define HY65_SETZERO_NOLAST_ERR		(TYPE_HY65+0x0004)		//当setzero_mode==HY65_SETZERO_MODE_LAST时却没有上一个存储的zero
#define HY65_COMM_BAUD_ERR		(TYPE_HY65+0x0005)		//0<baud<115200*800/184
#define HY65_COMM_BITS_ERR		(TYPE_HY65+0x0006)		//bits只有高4位有效,低4位必须为0
#define HY65_COMM_MODE_ERR		(TYPE_HY65+0x0007)		//comm_mode must=HY65_COMM_MODE_ALL,HY65_COMM_MODE_BAUDONLY,HY65_COMM_MODE_BITSONLY
#define HY65_SETSIGNAL_COMMAND_ERR	(TYPE_HY65+0x0008)		//传感器信号设置命令错误
#define HY65_SETSIGNAL_NUMBER_ERR	(TYPE_HY65+0x0009)		//传感器信号设置编号错误


#ifdef _MSC_VER
#if _MSC_VER > 1000
#pragma pack(pop)				//恢复old边界对齐
#endif
#endif
#endif