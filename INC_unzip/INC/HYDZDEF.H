//武汉华岩电子有限责任公司
//修订时间:	2002-7-10
//


#ifndef __HYDZDEF__
#define __HYDZDEF__	1
#ifdef _MSC_VER
#if _MSC_VER > 1000
#pragma pack(push)
#pragma pack(2)				//2 bytes边界对齐the same as 430 does
#endif
#endif


#ifndef NULL
#define NULL		0
#endif

#ifndef FALSE
#define FALSE		0
#endif

#ifndef	TRUE
#define TRUE		1
#endif

#ifndef ON
#define ON		1
#endif

#ifndef OFF
#define OFF		0
#endif

#ifndef BYTE
typedef unsigned char   BYTE;
#endif
#ifndef WORD
typedef unsigned short  WORD;
#endif
#ifndef	DWORD		
typedef unsigned long	DWORD;
#endif
#ifndef	BOOL
#ifndef _WIN32
typedef short           BOOL;
#else
typedef int	        BOOL;
#endif
#endif
#ifndef FLOAT
typedef float           FLOAT;
#endif

//x不能是指针,可以是char,short,int,long等基本类型(BYTE,WORD,DWORD)
#define LPBYTE1ST(x)	((unsigned char*)&x)
#define LPBYTE2ND(x)	((unsigned char*)((unsigned char*)&x+1))
#define LPBYTE3TH(x)	((unsigned char*)((unsigned char*)&x+2))
#define LPBYTE4TH(x)	((unsigned char*)((unsigned char*)&x+3))
#define LPWORD1ST(x)	((unsigned short*)&x)
#define LPWORD2ND(x)	((unsigned short*)((unsigned short*)&x+1))
#define BYTE1ST(x)	(*LPBYTE1ST(x))
#define BYTE2ND(x)	(*LPBYTE2ND(x))
#define BYTE3TH(x)	(*LPBYTE3TH(x))
#define BYTE4TH(x)	(*LPBYTE4TH(x))
#define WORD1ST(x)	(*LPWORD1ST(x))
#define WORD2ND(x)	(*LPWORD2ND(x))

typedef struct WORD_BYTE_STRUCT
{
    unsigned char 	l;
    unsigned char 	h;
}WORD_BYTE,*LPWORD_BYTE;
  
typedef struct DWORD_WORD_STRUCT
{
    unsigned short 	l;
    unsigned short 	h;
}DWORD_WORD,*LPDWORD_WORD;
  
typedef union LWB_UNION
{
    WORD_BYTE  		b;
    DWORD_WORD  	w;
    unsigned long 	l;
}LWB,*LPLWB;
    
typedef union LW_UNION
{
      DWORD_WORD 	w;
      unsigned long 	l;
}LW,*LPLW;

typedef union WB_UNION
{
    WORD_BYTE  		b;
    unsigned short  	w;
}WB,*LPWB;

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#ifndef MAKEWORD
#define MAKEWORD(a, b)      ((WORD)(((BYTE)(a)) | ((WORD)((BYTE)(b))) << 8))
#endif

#ifndef MAKELONG
#define MAKELONG(a, b)      ((LONG)(((WORD)(a)) | ((DWORD)((WORD)(b))) << 16))
#endif

#ifndef LOWORD
#define LOWORD(l)           ((WORD)(l))
#endif

#ifndef HIWORD
#define HIWORD(l)           ((WORD)(((DWORD)(l) >> 16) & 0xFFFF))
#endif

#ifndef LOBYTE
#define LOBYTE(w)           ((BYTE)(w))
#endif

#ifndef HIBYTE
#define HIBYTE(w)           ((BYTE)(((WORD)(w) >> 8) & 0xFF))
#endif

#ifdef _MSC_VER
#if _MSC_VER > 1000
#pragma pack(pop)				//恢复old边界对齐
#endif
#endif
#endif