#ifndef _IOT_HTTP_UPDATE_H_
#define _IOT_HTTP_UPDATE_H_


#include <stdbool.h>

#define   __IO volatile
typedef unsigned int  u32;
typedef unsigned char u8;

#if 0
#define HTTP_CFG_STEP					1			//配置1
#define HTTP_GET_STEP					2			//激活
#define HTTP_READ_STEP					3			//查询激活状态
#define HTTP_QUIT_STEP					4			//查询激活状态

#pragma pack(4)
typedef struct
{
	__IO u32 		ReadStartAddr;			//要读取的起始地址
	__IO u32 		ReadStartAddrLast;		//上一次要读取的起始地址
	__IO u32 		ReadStartZeroFlag;		//新的从零读取

	__IO u32 		ReadSizeThis;			//本次读取的长度
	__IO u32		ReadSizeAll;			//需要读取数据总大小

	__IO u32		GetStartAddr;			//GET起始地址
	__IO u32		GetEndAddr;				//GET结束地址
	__IO u32		GetSizeThis;			//本次Get到的数据长度

	__IO u32		Filelen;				//文件长度
	__IO u32		CRC32;				//文件长度

	__IO u32 		ModeSwitchTime;			//模式切换超时时间,超过这个时间,调节允许下,自动切换为HTTP模式   //u8改成u32类型 20211119  chen
	__IO u8			RstWlModFlag;			//是否在重启无线模块
	__IO u8			RstWlModFinishFlag;		//是否重启完成

	__IO u8			ChangeMode;				//是否需要切换为HTTP模式
	__IO u8 		StepFlag;				//HTTP步骤

	__IO u8 		ReadFlag;				//要读取状态
	__IO u8			Progress;				//HTTP下载进度
	__IO u8			ProUploadFlag;			//上传标记位,标记是否已上传进度
	__IO u8			ProUploadFinishFlag;	//上传标记位,标记是否已上传进度

	__IO u8			GetLastFlag;			//最后一次请求标志
	 u8 		ucA[3]; 				 //用于结构体对齐 20211119  chen


}HTTP_DOWNLOAD_STA;
#pragma pack()

#endif


#define HTTP_READ_MAX_SIZE 1024

#define HTTP_SUCCESSED					 	 0		//HTTP成功
#define HTTP_ERR_INIT_FAILED 				-1		//初始化失败
#define HTTP_ERR_GET_HEADER_FAILED  		-2		//获取文件头失败
#define HTTP_ERR_GET_FAILED  				-3		//HTTP GET请求失败
#define HTTP_ERR_READ_FAILED  				-4		//HTTP READ请求失败
#define HTTP_ERR_TIMEOUT 					-5   	//下载超时
#define HTTP_ERR_DL_FILE_CRC32_FAILED		-6		//下载文件CRC校验错误
#define HTTP_ERR_BSDIFF_CRC32_FAILED		-7		//差分文件CRC校验错误
#define HTTP_ERR_DEVICE_TYPE_FAILED			-8		//差分文件CRC校验错误



extern int IOT_HTTP_OTA_i(const char *cURL,char cOTAType_tem , bool is_Bsdiff_tem);

#endif


