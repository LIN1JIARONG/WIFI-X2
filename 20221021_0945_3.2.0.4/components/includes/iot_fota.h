/*
 * iot_fota.h
 *
 *  Created on: 2021��12��20��
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

#define FOTA_SINGLE_PACKAGE_LEN   128   // �������������ȣ�128�ֽ�


#ifndef HTTP_PROGRESS
#define INVERTRE_PROGRESS	(0x01)			//���������
#define HTTP_PROGRESS		(0xa2)			//�����ļ��������ʹ���
#define HTTP_DL_ERROR		(0x85)			//���ع̼�ʧ��
#define HTTP_CHECK_ERROR    (0x86)			//���ع̼��Ƿ�,У��ʧ��
#define HTTP_UPDATE_TIMEOUT (0x88)			//�����������ʱ

#endif


enum {
	EFOTA_TYPR_FULL = 0,
	EFOTA_TYPR_DATALOGER, // �ɼ���
	EFOTA_TYPR_DEVICE,    // �豸 - inv/storage/�ƶ����
	EFOTA_TYPR_ALL
};

enum {
	EFOTA_NULL = 0,
	EFOTA_INIT,		//������ʼ����־  20220507
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
	SOTAFileheaderINFO SOTAFileheader_t;	//�̼�ǰ20�ֽ���Ϣ
	unsigned int uiFileAddr;
	unsigned int uiFlashType;				//�����ļ���flash����
	unsigned int uiCrc32Val;
	unsigned int uiFileSize;
	unsigned int uiPointSize;
	unsigned int uiSumPackages;     // �����ļ�����/�·��ܰ��� ÿ����128�ֽ� FOTA_SINGLE_PACKAGE_LEN
	unsigned int uiCurrentPackages; // �����ļ�����/�·��ܰ��� ÿ����128�ֽ� FOTA_SINGLE_PACKAGE_LEN
	unsigned int uiSendDataLen;
	unsigned char ucSendDataBuf[256];
	unsigned int uiFotaTime;
	unsigned int uiFotaAckErrTimes;
	unsigned int uiFotaFileStartAddr;
	unsigned int uiFotaDeviceAddr;
	unsigned int uiFotaTargetAddr;
	unsigned int uiFotaProgress;  //��������
	unsigned int ProgressType;   //���ȵ�����
	unsigned int uiWayOfUpdate;
	bool ucIsbsdiffFlag;		 //�Ƿ���в�������־
	char cMasterFotaflag;        // ����������־λ  IOT_RESET-������/IOT_SET-��������/IOT_WAIT-��ʼ����/IOT_FINISH-�������/IOT_OTHER-�ȴ���һ�����ݷ���Ӧ��
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


