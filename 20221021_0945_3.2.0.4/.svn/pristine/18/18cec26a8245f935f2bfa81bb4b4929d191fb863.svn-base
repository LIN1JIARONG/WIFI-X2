/*
 * iot_fota.h
 *
 *  Created on: 2021年12月20日
 *      Author: Administrator
 */

#ifndef COMPONENTS_INCLUDES_IOT_FOTA_H_
#define COMPONENTS_INCLUDES_IOT_FOTA_H_

#include <stdbool.h>
#include <stdint.h>

#ifndef IOT_FOTA_GLOBAL
#define IOT_FOTA_EXTERN extern
#else
#define IOT_FOTA_EXTERN
#endif

#define FOTA_SINGLE_PACKAGE_LEN   128   // 单次升级包长度：128字节


#ifndef HTTP_PROGRESS
#define INVERTRE_PROGRESS	(0x01)			//逆变器升级
#define HTTP_PROGRESS		(0xa2)			//下载文件进度类型代码
#define HTTP_DL_ERROR		(0x85)			//下载固件失败
#define HTTP_CHECK_ERROR    (0x86)			//下载固件非法,校验失败
#define HTTP_UPDATE_TIMEOUT (0x88)			//升级逆变器超时

#endif


enum {
	EFOTA_TYPR_FULL = 0,
	EFOTA_TYPR_DATALOGER, // 采集器
	EFOTA_TYPR_DEVICE,    // 设备 - inv/storage/移动电池
	EFOTA_TYPR_ALL
};

enum {
	EFOTA_NULL = 0,
	EFOTA_INIT,		//新增初始化标志  20220507
	EFOTA_INITING,
	EFOTA_HEAD,
	EFOTA_HEADING,
	EFOTA_FILE,
	EFOTA_FILEING,
	EFOTA_FINISHING,
	EFOTA_UPDATE_DEVICE,
	EFOTA_LOCAL_UPDATE,
	EFOTA_LOCAL_UPDATE_DEVICE,
	EFOTA_FINISH,
	EFOTA_OTHER,
	EFOTA_ALL
};

enum {
	ELOCAL_UPDATE = 1,
	EHTTP_UPDATE,
};


typedef struct{
	uint32_t uiOldFilelen;
	uint32_t uiPatchFilelen;
	uint32_t uiPatchFileCRC32;
	uint32_t uiNewFileLen;
	uint32_t uiNewFileCRC32;
}SOTAFileheaderINFO;


typedef struct _SFotaParams{
	SOTAFileheaderINFO SOTAFileheader_t;	//固件前20字节信息
	unsigned int uiFileAddr;
	unsigned int uiFlashType;				//保存文件的flash类型
	unsigned int uiCrc32Val;
	unsigned int uiFileSize;
	unsigned int uiPointSize;
	unsigned int uiSumPackages;     // 升级文件下载/下发总包数 每包：128字节 FOTA_SINGLE_PACKAGE_LEN
	unsigned int uiCurrentPackages; // 升级文件下载/下发总包数 每包：128字节 FOTA_SINGLE_PACKAGE_LEN
	unsigned int uiSendDataLen;
	unsigned char ucSendDataBuf[256];
	unsigned int uiFotaTime;
	unsigned int uiFotaAckErrTimes;
	unsigned int uiFotaFileStartAddr;
	unsigned int uiFotaDeviceAddr;
	unsigned int uiFotaTargetAddr;
	unsigned int uiFotaProgress;  //升级进度
	unsigned int ProgressType;   //进度的类型
	unsigned int uiWayOfUpdate;
	bool ucIsbsdiffFlag;		 //是否进行差升级标志
	char cMasterFotaflag;        // 主机升级标志位  IOT_RESET-无升级/IOT_SET-触发升级/IOT_WAIT-开始升级/IOT_FINISH-完成升级/IOT_OTHER-等待第一包数据返回应答
}FotaParams;
IOT_FOTA_EXTERN FotaParams g_FotaParams;

IOT_FOTA_EXTERN unsigned char IOT_DeviceFotaTimeOut_Handle_uc(unsigned char ucMode_tem);
IOT_FOTA_EXTERN void IOT_Fota_Init(void);
IOT_FOTA_EXTERN char IOT_FotaFile_HEAD_Check_c(unsigned char *pucSrc_tem);
IOT_FOTA_EXTERN char IOT_FotaType_Manage_c(char cType_tem, char cVal_tem);
IOT_FOTA_EXTERN char IOT_FotaParams_Write_c(unsigned char ucParamNums_tem, unsigned int uiParamVal_tem);
IOT_FOTA_EXTERN uint8_t iot_Bsdiff_Update_Judge_uc(uint8_t ucMode_tem);
IOT_FOTA_EXTERN bool IOT_FotaFile_Integrality_Check_b(uint8_t CheckMode_tem);
IOT_FOTA_EXTERN void IOT_Set_OTA_breakpoint_ui(uint32_t BreakPoint_tem);

IOT_FOTA_EXTERN void IOT_Update_Process_Judge(void);
IOT_FOTA_EXTERN uint8_t IOT_MasterFotaStatus_SetAndGet_ui(uint8_t ucMode_tem , uint8_t ucSetType_tem);
IOT_FOTA_EXTERN void IOT_UpdateProgress_record(uint16_t usProgressType_tem , uint32_t uiUpdateProgress_tem);
IOT_FOTA_EXTERN uint8_t IOT_UpdateProgress_Upload_Handle(uint8_t ucMode_tem);

IOT_FOTA_EXTERN uint32_t IOT_Get_OTA_FlashType_ui(void);
IOT_FOTA_EXTERN uint32_t IOT_Get_OTA_FileStartAddr_ui(void);
IOT_FOTA_EXTERN uint32_t IOT_Get_OTA_FileSize_ui(void);
IOT_FOTA_EXTERN uint32_t IOT_Get_OTA_FileCRC32Val_ui(void);
IOT_FOTA_EXTERN uint32_t IOT_Get_OTA_breakpoint_ui(void);
IOT_FOTA_EXTERN bool IOT_FotaFile_DeviceType_Check_b(void);
#endif


