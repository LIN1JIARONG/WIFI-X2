 /*
 * iot_fota.c
 *
 *  Created on: 2021年12月8日
 *      Author: Administrator
 */

#define  IOT_FOTA_GLOBAL
#include "iot_fota.h"
#include "iot_universal.h"
#include "iot_params.h"
#include "esp_log.h"

#include "iot_ota.h"
#include "iot_crc32.h"
#include "iot_bsdiff_api.h"

#include "iot_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "iot_https_ota.h"
#include "iot_inverter_fota.h"

#include "iot_spi_flash.h"
#include "ESP_spi_flash.h"

#include "iot_gatts_table.h"

static const char *FOTA = "iot_fota";



extern uint32_t IOT_GetTick(void);


TaskHandle_t HTTP_Handle = NULL;

#if 1


uint8_t IOT_MasterFotaStatus_SetAndGet_ui(uint8_t ucMode_tem , uint8_t ucSetType_tem)
{
	uint8_t type_t = 0;
	if(ucMode_tem == 1)
	{
		g_FotaParams.cMasterFotaflag = ucSetType_tem;
		type_t =  ucSetType_tem;
	}
	else if(ucMode_tem == 0xFF)
	{
		type_t =  g_FotaParams.cMasterFotaflag;
	}

	return type_t;
}


/******************************************************************************
 * FunctionName : IOT_DeviceFotaTimeOut_Handle
 * Description  : 系统升级超时判断
 * Parameters   : unsigned char ucMode_tem： 1-设置 / 0xFF-查询
 * Returns      : 0/1
*******************************************************************************/
unsigned char IOT_DeviceFotaTimeOut_Handle_uc(unsigned char ucMode_tem)
{
	#define FOTA_TIMEOUT ((5) * (60) * (1000))  // 5分钟 : 1000 = 1s

	if(ucMode_tem == 1)
	{
		g_FotaParams.uiFotaTime = IOT_GetTick();
	}
	else if(ucMode_tem == 0xFF)
	{
		if(EFOTA_NULL != IOT_MasterFotaStatus_SetAndGet_ui(0xFF, EFOTA_NULL)) // 判断系统是否在升级 add isen 20210528
		{
			if((IOT_GetTick() - g_FotaParams.uiFotaTime) >= FOTA_TIMEOUT)
			{
				IOT_Printf("\r\n\r\n\r\n\r\n\r\n\r\n**************************************************\r\n");
				IOT_Printf("DeviceFotaTimeOut_Judge OUTTIME = %d\r\n",FOTA_TIMEOUT);
				IOT_Printf("DeviceFotaTimeOut_Judge OUTTIME cout = %d\r\n",((IOT_GetTick() - g_FotaParams.uiFotaTime)/1000));
				IOT_Printf("DeviceFotaTimeOut_Judge IOT_ESP_System_Get_Time() = %d\r\n",IOT_GetTick());
				IOT_Printf("DeviceFotaTimeOut_Judge uiFotaTime = %d\r\n",g_FotaParams.uiFotaTime);
				IOT_Printf("**************************************************\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n");

				g_FotaParams.uiFotaTime = IOT_GetTick();
				IOT_SystemParameterSet(10,"0", 1);  //保存数据
				IOT_SystemParameterSet(36,"0", 1);  //保存数据

				IOT_UpdateProgress_record(HTTP_PROGRESS, HTTP_DL_ERROR);
				g_FotaParams.cMasterFotaflag = EFOTA_FINISH;  // 升级结束
				return 0;
			}
		}
	}

	return 1;
}
#endif

/******************************************************************************
 * FunctionName : IOT_Fota_Init()
 * Description  : 升级初始化
 * Parameters   : none
 * Returns      : none
 * Notice       : none
*******************************************************************************/
void IOT_Fota_Init(void)
{
	unsigned char ucFotaParamsBuf[5] = {0};

	IOT_AddParam_Read(ucFotaParamsBuf, 0);
	g_FotaParams.uiFileSize = IOT_htoi(ucFotaParamsBuf, 4);

	memset(ucFotaParamsBuf , 0x00 , 5);
	IOT_AddParam_Read(ucFotaParamsBuf, 1);
	g_FotaParams.uiCrc32Val = IOT_htoi(ucFotaParamsBuf, 4);

	memset(ucFotaParamsBuf , 0x00 , 5);
	IOT_AddParam_Read(ucFotaParamsBuf, 4);
	g_FotaParams.uiPointSize = IOT_htoi(ucFotaParamsBuf, 4);

	//memset(ucFotaParamsBuf , 0x00 , 5);
	//IOT_AddParam_Read(ucFotaParamsBuf, 6);			//读取保存文件的flash类型
	//g_FotaParams.uiFlashType= IOT_htoi(ucFotaParamsBuf, 1);

	ESP_LOGI(FOTA, "IOT_Fota_Init() size = %x crc32 = %x piontsize = %x \r\n", g_FotaParams.uiFileSize,g_FotaParams.uiCrc32Val,g_FotaParams.uiPointSize);
	ESP_LOGI(FOTA, "IOT_Fota_Init() uiFlashType = %d\r\n" , g_FotaParams.uiFlashType);

	IOT_DeviceFotaTimeOut_Handle_uc(1);		//设置数据超时时间
	g_FotaParams.uiFotaAckErrTimes = 0;
	IOT_MasterFotaStatus_SetAndGet_ui( 1 , EFOTA_INIT );	//更新升级标志
}

#if 0
/******************************************************************************
 * FunctionName : IOT_FotaFile_Check_c()
 * Description  : 升级文件校验
 * Parameters   : char cType_tem：0-升级文件头20字节 / 1-主机升级参数初始化 / 2-固定文件标志位
                  unsigned char ucSrc_tem ：升级文件头20字节
 * Returns      : 0-成功 / 1-失败
 * Notice       : 1、升级数据包包头: 地址偏移 - 数据包长度 - 数据包crc32校验值 - 预留 - 预留 (各4个字节)
				  举例：00000000 - 00004994 - 9eca3495 - 00000000 - 00000000

*******************************************************************************/

char IOT_FotaFile_Check_c(char cType_tem, unsigned char *pucSrc_tem)
{
	unsigned int uiFileSumSize = 0; // 升级文件总长度
	unsigned int uiFileCRC = 0;     // 升级文件保存“页”地址，断点下载需要用到

	if(cType_tem == 0)
	{
		uiFileSumSize = IOT_htoi(&pucSrc_tem[4],4); // 得到升级文件总长度
		uiFileCRC = IOT_htoi(&pucSrc_tem[8],4);     // 得到升级文件CRC32

		if((g_FotaParams.uiFileSize != uiFileSumSize) || (g_FotaParams.uiCrc32Val != uiFileCRC))
		{
			g_FotaParams.uiFileSize = uiFileSumSize;
			g_FotaParams.uiCrc32Val = uiFileCRC;
			g_FotaParams.uiPointSize = 0;

			IOT_AddParam_Write(&pucSrc_tem[4], 0, 4);
			IOT_AddParam_Write(&pucSrc_tem[8], 1, 4);
			memset(pucSrc_tem,0,4);
			IOT_AddParam_Write(&pucSrc_tem[0], 4, 4);
		}

		g_FotaParams.uiFileAddr = g_FotaParams.uiFotaFileStartAddr + g_FotaParams.uiPointSize; // 得到完整存储soc-flash升级文件起始地址
	}
	else if(cType_tem == 1)
	{
		IOT_Fota_Init();
		g_FotaParams.uiPointSize = 0;
		g_FotaParams.uiFileAddr = 0;
	}
	else if(cType_tem == 2)
	{

	}

	ESP_LOGI(FOTA, " IOT_FotaFile_Check_c() size = %x crc32 = %x piontsize = %x \r\n", g_FotaParams.uiFileSize,g_FotaParams.uiCrc32Val,g_FotaParams.uiPointSize);

	return 0;
}
#else
/******************************************************************************
 * FunctionName : IOT_FotaFile_Check_c()
 * Description  : 升级文件头校验
 * Parameters   : char cType_tem：0-升级文件头20字节 / 1-主机升级参数初始化 / 2-固定文件标志位
                  unsigned char ucSrc_tem ：升级文件头20字节
 * Returns      : 0-成功 / 1-失败
 * Notice       : 1、升级数据包包头: 地址偏移 - 数据包长度 - 数据包crc32校验值 - 预留 - 预留 (各4个字节)
				  举例：00000000 - 00004994 - 9eca3495 - 00000000 - 00000000

*******************************************************************************/
char IOT_FotaFile_HEAD_Check_c(unsigned char *pucSrc_tem)
{
	unsigned int uiFileSumSize = 0; // 升级文件总长度
	unsigned int uiFileCRC = 0;     // 升级文件CRC
	unsigned char ucPstrbuf[4] = {0};
	uint32_t uiOTASaveFileAddr = 0;
	uint32_t uiOTASaveFileCrc = 0;

	int8_t err_t = -1;

	uiFileSumSize = IOT_htoi(&pucSrc_tem[4],4); // 得到升级文件总长度
	uiFileCRC = IOT_htoi(&pucSrc_tem[8],4);     // 得到升级文件CRC32

	//IOT_Fota_Init();		//获取已保存的下载文件信息

	uiOTASaveFileAddr = IOT_OTA_Get_Save_File_addr_uc();	//获取保存下载文件的起始位置
	g_FotaParams.uiFotaFileStartAddr = uiOTASaveFileAddr;

	ucPstrbuf[0] = ( uiOTASaveFileAddr >> 24 ) & 0xff;
	ucPstrbuf[1] = ( uiOTASaveFileAddr >> 16 ) & 0xff;
	ucPstrbuf[2] = ( uiOTASaveFileAddr >> 8) & 0xff;
	ucPstrbuf[3] = uiOTASaveFileAddr & 0xff;

	IOT_AddParam_Write(&ucPstrbuf[0], 7, 4);		//保存文件起始地址
	ESP_LOGI(FOTA, " uiOTASaveFileAddr = %x  \r\n", uiOTASaveFileAddr);

	g_FotaParams.uiFlashType = IOT_OTA_Get_Save_Flash_Type_uc();

	ucPstrbuf[0] = ( g_FotaParams.uiFlashType >> 24 ) & 0xff;
	ucPstrbuf[1] = ( g_FotaParams.uiFlashType >> 16 ) & 0xff;
	ucPstrbuf[2] = ( g_FotaParams.uiFlashType >> 8) & 0xff;
	ucPstrbuf[3] = g_FotaParams.uiFlashType & 0xff;
	IOT_AddParam_Write(&ucPstrbuf[0], 6, 4);		//保存Flash 类型
	ESP_LOGI(FOTA, " g_FotaParams.uiFlashType = %d  \r\n", g_FotaParams.uiFlashType);


	if((g_FotaParams.uiFileSize != uiFileSumSize) || (g_FotaParams.uiCrc32Val != uiFileCRC))   //与上一次下载的文件不同
	{
		g_FotaParams.uiFileSize = uiFileSumSize;
		g_FotaParams.uiCrc32Val = uiFileCRC;
		g_FotaParams.uiPointSize = 0;					//断点清零

		IOT_AddParam_Write(&pucSrc_tem[4], 0, 4);		//保存新文件大小
		IOT_AddParam_Write(&pucSrc_tem[8], 1, 4);		//保存新文件CRC32校验码

		memset(ucPstrbuf,0,4);
		IOT_AddParam_Write(&ucPstrbuf[0], 4, 4);		//断点清零

		ESP_LOGI(FOTA, " IOT_FotaFile_HEAD_Check_c() uiFileSumSize = %x uiFileCRC = %x piontsize = %x \r\n", \
								uiFileSumSize , uiFileCRC , g_FotaParams.uiPointSize);

		err_t = 1;
	}else if((g_FotaParams.uiPointSize  > 0)  && (g_FotaParams.uiPointSize  < uiFileSumSize))
	{
		//从断点处开始下载
		err_t = 2;
	}
	else if(g_FotaParams.uiPointSize  == uiFileSumSize)
	{
		err_t = 2;	//从断点开始下载
		if((g_FotaParams.uiFlashType > NULL_FLASH) &&  (g_FotaParams.uiFlashType < FLASH_NUM))
		{
			uiOTASaveFileCrc = FLASH_GetCRC32 (uiOTASaveFileAddr , uiFileSumSize , g_FotaParams.uiFlashType);
			ESP_LOGI(FOTA, " uiOTASaveFileCrc = 0x%x , uiFileCRC = \r\n",uiOTASaveFileCrc,uiFileCRC);
			if(uiOTASaveFileCrc == uiFileCRC)
			{
				//已保存有相同完整的文件,无需重新下载
				err_t = 0;
			}
		}
	}else	//断点异常,重新清0
	{
		g_FotaParams.uiPointSize = 0;
	}

	//文件头信息解析
	g_FotaParams.SOTAFileheader_t.uiOldFilelen	 = IOT_htoi(&pucSrc_tem[0],4); 	 // 0 ~ 3 字节 正在运行的固件的长度
	g_FotaParams.SOTAFileheader_t.uiPatchFilelen = uiFileSumSize; 				 // 4 ~ 7 字节 正在下载的固件(补丁文件)的长度
	g_FotaParams.SOTAFileheader_t.uiPatchFileCRC32 = uiFileCRC; 				 // 8 ~ 11 字节 正在下载的固件(补丁文件)的CRC32校验码
	g_FotaParams.SOTAFileheader_t.uiNewFileLen 	  = IOT_htoi(&pucSrc_tem[12],4);   // 12 ~ 15 字节 差分升级解压后的文件的长度
	g_FotaParams.SOTAFileheader_t.uiNewFileCRC32  = IOT_htoi(&pucSrc_tem[16],4);   // 16 ~ 19 字节 差分升级解压后的文件的CRC32校验码


	g_FotaParams.uiFileAddr = g_FotaParams.uiFotaFileStartAddr + g_FotaParams.uiPointSize; // 得到完整存储soc-flash升级文件起始地址

	ESP_LOGI(FOTA, " IOT_FotaFile_HEAD_Check_c() size = %x RecCrc32 = %x piontsize = %x , NewCrc32 = %x , NewFileLen = %x\r\n", \
												g_FotaParams.uiFileSize,g_FotaParams.uiCrc32Val,g_FotaParams.uiPointSize , uiFileCRC,uiFileSumSize);

	return err_t;
}

#endif


/******************************************************************************
 * FunctionName : IOT_FotaFile_Integrality_Check_c()
 * Description  : 升级文件完整性校验
 * Parameters   : uint8_t CheckMode_tem
 * 					1  下载文件校验
 * 					2  差分升级解压后的文件校验
 * Returns      : ture-校验成功 / flase-校验失败
 * Notice       :在升级流程中才能调用

*******************************************************************************/
bool IOT_FotaFile_Integrality_Check_b(uint8_t CheckMode_tem)
{
	uint32_t uiFileCRC = 0;
	uint32_t uiOTASaveFileAddr = 0;
	uint32_t uiOTASaveFileSize = 0;
	uint32_t uiOTASaveFileCrc = 0;

	FlASHTYPE SOtaFlashType = NULL_FLASH;
	bool err_t = false;


	//g_FotaParams.
	if(1 == CheckMode_tem)			//下载的文件校验
	{
		uiOTASaveFileAddr = IOT_OTA_Get_Save_File_addr_uc();			//获取保存下载文件的起始位置
		uiOTASaveFileSize = IOT_OTA_Get_Written_Size_uc();   			//获取下载文件的长度
		SOtaFlashType = IOT_OTA_Get_Save_Flash_Type_uc();			//获取保存文件所在的flash
		ESP_LOGI(FOTA, " SOtaFlashType = %d\r\n",SOtaFlashType);
		uiFileCRC = g_FotaParams.uiCrc32Val; //文件头中的下载文件的CRC32校验码
		IOT_Printf("uiOTASaveFileAddr =0x%x",uiOTASaveFileAddr);

	}else if(2 == CheckMode_tem)	//差分升级解压后的文件校验
	{
		uiOTASaveFileAddr = IOT_OTA_Get_Update_Partition_addr_uc();
		uiOTASaveFileSize = g_FotaParams.SOTAFileheader_t.uiNewFileLen;
		uiFileCRC =  g_FotaParams.SOTAFileheader_t.uiNewFileCRC32;
		SOtaFlashType = System_FLASH;			//获取保存文件所在的flash , 差分后保存的文件默认在系统Flash中
	}

	uiOTASaveFileCrc =  FLASH_GetCRC32 (uiOTASaveFileAddr , uiOTASaveFileSize ,SOtaFlashType);
	if(uiFileCRC == uiOTASaveFileCrc)
	{
		err_t = true;
	}

	ESP_LOGI(FOTA, " IOT_FotaFile_Integrality_Check_b  Mode = %d",CheckMode_tem);
	ESP_LOGI(FOTA, " IOT_FotaFile_Integrality_Check_b() size = %x uiOTASaveFileCrc = %x uiCrc32Val = %x \r\n", \
			uiOTASaveFileSize , uiOTASaveFileCrc , uiFileCRC);

	return err_t;
}


/******************************************************************************
 * FunctionName : IOT_FotaParams_Write_b
 * Description  : 空中升级数据写入
 * Parameters   : unsigned char ucParamNums_tem  : 参数编号
 *                unsigned int uiParamVal_tem  : 保存参数十进制值
 * Returns      : 0
 * Notice       : 写入值已经由“十进制值”转化为“hex”写入保存
*******************************************************************************/
char IOT_FotaParams_Write_c(unsigned char ucParamNums_tem, unsigned int uiParamVal_tem)
{
	uint8_t ucFotaParamsBuf[9] = {0};
	IOT_itoh(ucFotaParamsBuf,uiParamVal_tem,4);
	IOT_AddParam_Write(ucFotaParamsBuf, ucParamNums_tem, 4); // 保存数据   x-保存编号/4-长度
	ESP_LOGI(FOTA, " IOT_FotaParams_Write_c() Num = %d Val = %d ", ucParamNums_tem, uiParamVal_tem);

	return 0;
}

/******************************************************************************
 * FunctionName : IOT_FotaType_Manage_c()
 * Description  : 升级类型管理（采集器升级/设备-inv/storage/电池升级）
 * Parameters   : char cType_tem：0-清空节 / 1-设置 / 0xFF-查询
                  char cVal_tem ：升级类型值
 * Returns      : 0-升级类型
 * Notice       : 1、触发升级的时，调用该api设置升级类型 -- 必须
*******************************************************************************/
char IOT_FotaType_Manage_c(char cType_tem, char cVal_tem)
{
	static char s_cFotaType = EFOTA_TYPR_DATALOGER; // 设备升级类型 采集器 / 设备-inv/storage/电池
	if(cType_tem == 0)			// 清零
	{
		s_cFotaType = 0;
	}
	else if(cType_tem == 1)		// 设置类型
	{
		s_cFotaType = cVal_tem;
	}
	else  if(cType_tem == 0xFF)   //查询
	{
		return s_cFotaType;
	}
	return s_cFotaType;
}

/******************************************************************************
 * FunctionName : iot_Bsdiff_Update_Judge()
 * Description  : 判断是否进行差分升级
 * Parameters   : char cType_tem：0-清空节 / 1-设置 / 0xFF-查询
                  char cVal_tem ：升级类型值
 * Returns      : 0-升级类型
 * Notice       : 1、触发升级的时，调用该api设置升级类型 -- 必须
*******************************************************************************/
uint8_t iot_Bsdiff_Update_Judge_uc(uint8_t ucMode_tem)
{
	static uint8_t sucUpdateFlag = 0;
	uint8_t ERR_t = 0;

	if(1 == ucMode_tem)
	{
		sucUpdateFlag = 1;
	}
	else if(0 == ucMode_tem)
	{
		sucUpdateFlag = 0;
	}

	if(1 == sucUpdateFlag )
	{
		sucUpdateFlag = 0;
		BsPatchFile_t OTAUpdateFileInfo = {0};
		SOTAFileheaderINFO *pSBsdiffUpdateFileInfo =   &g_FotaParams.SOTAFileheader_t;
		OTAUpdateFileInfo.uiOldFileAddr   = IOT_OTA_Get_Running_app_addr_uc();
		//OTAUpdateFileInfo.uiOldFileSize   = pSBsdiffUpdateFileInfo->uiOldFilelen;
		OTAUpdateFileInfo.uiOldFileSize	 = 1498256;
		OTAUpdateFileInfo.uiPatchFileAddr = IOT_OTA_Get_Save_File_addr_uc();
		OTAUpdateFileInfo.uiPatchFileSize =	pSBsdiffUpdateFileInfo->uiPatchFilelen;
		OTAUpdateFileInfo.uiNewFileAddr   = IOT_OTA_Get_Update_Partition_addr_uc();
		//OTAUpdateFileInfo.uiNewFileSize   = pSBsdiffUpdateFileInfo->uiNewFileLen;

		OTAUpdateFileInfo.uiNewFileSize = 1498256;
		g_FotaParams.SOTAFileheader_t.uiNewFileLen = 1498256;
		g_FotaParams.SOTAFileheader_t.uiNewFileCRC32 = 0x56a06974;
		IOT_BsPatch_Run(OTAUpdateFileInfo);	//差分升级解压文件

		if(true == IOT_FotaFile_Integrality_Check_b(2))		//校验差分升级解压后的文件
		{
			if(true == IOT_FotaFile_DeviceType_Check_b())	//校验升级文件的设备类型
			{
				IOT_OTA_Set_Boot();
				ERR_t = 0;
			}else
			{
				ERR_t = 1;
			}
			IOT_OTA_Finish_uc();	//OTA结束
			//System_state.SYS_RestartTime = IOT_GetTick();
			System_state.SYS_RestartFlag = 1;
		}
		else
		{
			return 1;  //失败
		}

	}

	return ERR_t;
}


/******************************************************************************
 * FunctionName : IOT_Get_OTA_breakpoint_ui()
 * Description  : 获取下载文件的断点
 * Parameters   : void
 * Returns      : 断点地址为相对于保存文件起始地址的偏移
 * Notice       : none
*******************************************************************************/
uint32_t IOT_Get_OTA_breakpoint_ui(void)
{
#if 1
	unsigned char ucFotaParamsBuf[5] = {0};

	IOT_AddParam_Read(ucFotaParamsBuf, 4);			//4
	g_FotaParams.uiPointSize = IOT_htoi(ucFotaParamsBuf, 4);

#endif

	return g_FotaParams.uiPointSize;

}


/******************************************************************************
 * FunctionName : IOT_Get_OTA_FileSize_ui()
 * Description  : 获取下载文件的大小
 * Parameters   : void
 * Returns      : 文件大小从文件20字节中获取
 * Notice       : none
*******************************************************************************/
uint32_t IOT_Get_OTA_FileSize_ui(void)
{
	#if 1
		unsigned char ucFotaParamsBuf[5] = {0};

		IOT_AddParam_Read(ucFotaParamsBuf, 0);
		g_FotaParams.uiFileSize = IOT_htoi(ucFotaParamsBuf, 4);

	#endif

	return g_FotaParams.uiFileSize;

}

/******************************************************************************
 * FunctionName : IOT_Get_OTA_FileSize_ui()
 * Description  : 获取下载文件的CRC32
 * Parameters   : void
 * Returns      : 文件大小从文件20字节中获取
 * Notice       : none
*******************************************************************************/
uint32_t IOT_Get_OTA_FileCRC32Val_ui(void)
{
	#if 1
		unsigned char ucFotaParamsBuf[5] = {0};

		IOT_AddParam_Read(ucFotaParamsBuf, 1);
		g_FotaParams.uiCrc32Val = IOT_htoi(ucFotaParamsBuf, 4);

	#endif

	return g_FotaParams.uiCrc32Val;

}
/******************************************************************************
 * FunctionName : IOT_Get_OTA_FlashType_ui()
 * Description  : 获取下载文件的保存的Flash 类型
 * Parameters   : void
 * Returns      : void
 * Notice       : none
*******************************************************************************/
uint32_t IOT_Get_OTA_FlashType_ui(void)
{
	#if 1
		unsigned char ucFotaParamsBuf[5] = {0};

		IOT_AddParam_Read(ucFotaParamsBuf, 6);			//读取保存文件的flash类型
		g_FotaParams.uiFlashType= IOT_htoi(ucFotaParamsBuf, 4);

	#endif

	return g_FotaParams.uiFlashType;

}

/******************************************************************************
 * FunctionName : IOT_Get_OTA_FileStartAddr_ui()
 * Description  : 获取下载文件的起始地址
 * Parameters   : void
 * Returns      : 文件起始地址
 * Notice       : none
*******************************************************************************/
uint32_t IOT_Get_OTA_FileStartAddr_ui(void)
{
	#if 1
		unsigned char ucFotaParamsBuf[5] = {0};

		IOT_AddParam_Read(ucFotaParamsBuf, 7);			//读取保存文件的起始地址
		g_FotaParams.uiFotaFileStartAddr= IOT_htoi(ucFotaParamsBuf, 4);

	#endif

	return g_FotaParams.uiFotaFileStartAddr;

}



/******************************************************************************
 * FunctionName : IOT_FotaFile_DeviceType_Check_b()
 * Description  : 校验升级文件的设备类型
 * Parameters   :
 * Returns      : true-校验成功 / false-校验失败
 * Notice       :在升级流程中才能调用

*******************************************************************************/
bool IOT_FotaFile_DeviceType_Check_b(void)
{
	uint8_t  ucUserData[USER_DATA_LEN]  = {0};
	bool err_t = false;


	uint32_t FileSaveAddr_tem = IOT_Get_OTA_FileStartAddr_ui();
	uint32_t FlashType_tem = 	IOT_Get_OTA_FlashType_ui();

	if(External_FLASH == FlashType_tem)
	{
		IOT_Mutex_SPI_FLASH_BufferRead(ucUserData , FileSaveAddr_tem + USER_DATA_ADDR , USER_DATA_LEN);
	}else if(System_FLASH == FlashType_tem)
	{
		spi_flash_read(FileSaveAddr_tem + USER_DATA_ADDR , ucUserData , USER_DATA_LEN);
	}

	ESP_LOGI(FOTA, " IOT_FotaFile_DeviceType_Check_b Fota  DeviceType = %s",ucUserData);
	if(strcmp((const char *)ucUserData , (const char *)DATALOG_TYPE) == 0)
	{
		ESP_LOGI(FOTA, " IOT_FotaFile_DeviceType_Check_b Compared  DeviceType Pass");
		err_t = true;
	}


	return err_t;
}



/******************************************************************************
 * FunctionName : IOT_Set_OTA_breakpoint_ui()
 * Description  : 设置并保存下载文件的断点
 * Parameters   : void
 * Returns      : 断点地址为相对于保存文件起始地址的偏移
 * Notice       : none
*******************************************************************************/
void IOT_Set_OTA_breakpoint_ui(uint32_t BreakPoint_tem)
{
	g_FotaParams.uiPointSize = BreakPoint_tem;
	IOT_FotaParams_Write_c(4, g_FotaParams.uiPointSize); // 记录HTTP断点下载保存数据长度：编号=4
}



/******************************************************************************
 * FunctionName : IOT_UpdateProgress_record
 * Description  : 升级进度记录
 * Parameters   :	uint16_t usProgressType_tem			上传进度类型
 * 					INVERTRE_PROGRESS	(0x01)			//逆变器升级类型
   	   	   	   	   	HTTP_PROGRESS		(0xa2)			//下载文件进度类型

 * 				 uint16_t uiUpdateProgress 进度
 * Returns      : void
 * Notice       : none
*******************************************************************************/

void IOT_UpdateProgress_record(uint16_t usProgressType_tem , uint32_t uiUpdateProgress_tem)
{
	char sProgress[4] = {0};
	sProgress[0] ='0' + ((uiUpdateProgress_tem / 100) % 10);
	sProgress[1] ='0' + ((uiUpdateProgress_tem / 10) % 10) ;
	sProgress[2] ='0' + (uiUpdateProgress_tem % 10) ;

	g_FotaParams.uiFotaProgress = uiUpdateProgress_tem;
	g_FotaParams.ProgressType =  usProgressType_tem;
	IOT_SystemParameterSet(101 ,(char *)sProgress , 3);  //保存进度

	if(g_FotaParams.uiFotaProgress == 0)
	{
		IOT_UpdateProgress_Upload_Handle(0);	//初始化
	}
}


/******************************************************************************
 * FunctionName : IOT_UpdateProgress_record
 * Description  : 上传进度处理
 * Parameters   : ucMode_tem  0  清除上一次上传进度的标志
 * 							  1  查询最后一次进度是否上传完成
 * 							  0xff  查询是否需要上传进度
 * Returns      : 1  需要上传进度  0  无需上传进度
 * Notice       : none
*******************************************************************************/
uint8_t IOT_UpdateProgress_Upload_Handle(uint8_t ucMode_tem)
{
	#define PROGRESS_UPLOAD_THRESHOLD   5   //阈值 每 5% 上传一次进度

	static uint16_t usrecProgress = 0;
	static uint16_t usUploadFirstTimesFlag = 0;		//第一次上传标志
	static uint16_t usUploadLastTimesFlag = 0;		//最后一次上传标志

	if(0 == ucMode_tem)
	{
		usrecProgress = 0;
		usUploadFirstTimesFlag = 0;
		usUploadLastTimesFlag = 0;

	}
	else if(1 == ucMode_tem)
	{
		return usUploadLastTimesFlag;
	}
	else if(EFOTA_NULL != IOT_MasterFotaStatus_SetAndGet_ui(0xff , EFOTA_NULL)) //若处于升级状态,则判断是否需要上传进度
	{
		if((g_FotaParams.uiFotaProgress - usrecProgress) > PROGRESS_UPLOAD_THRESHOLD)
		{
			usrecProgress = g_FotaParams.uiFotaProgress;
			return 1;
		}
		else if((0 == g_FotaParams.uiFotaProgress) && (0 == usUploadFirstTimesFlag))  //进度为0%时,允许上传第一次
		{
			usUploadFirstTimesFlag = 1;
			return 1;
		}
		else if((g_FotaParams.uiFotaProgress >= 100) && (0 == usUploadLastTimesFlag)) //进度大于或等于 100 时 ,上传最后一次进度
		{
			usUploadLastTimesFlag = 1;
			return 1;
		}
	}

	return 0;
}


#include "iot_mqtt.h"
#include "iot_system.h"
/******************************************************************************
 * FunctionName : IOT_Update_Process_Judge()
 * Description  : 升级处理
 * Parameters   : void
 * Notice       : void
*******************************************************************************/
void IOT_Update_Process_Judge(void)
{
	static uint8_t sCreateFotaTaskFlag =0;

	uint8_t ucFotaStatus = IOT_MasterFotaStatus_SetAndGet_ui(0xff , EFOTA_NULL);


	if(ESP_OK1 != IOT_UpgradeDelayTime_uc(ESP_READ , 0))
	{
		return;
	}

	switch(ucFotaStatus)
	{
		case EFOTA_INIT:
		{
			if((0 == sCreateFotaTaskFlag) && (EHTTP_UPDATE == g_FotaParams.uiWayOfUpdate))
			{
				if(System_state.ble_onoff==1 ) 		//关闭
				{
					IOT_GattsEvenOFFBLE();   		//通知关闭BLE
					IOT_UpgradeDelayTime_uc(ESP_WRITE , 15);   //关闭蓝牙 15s 后触发升级
				}else
				{
					if(1 == System_state.Mbedtls_state)
					{
						xTaskCreate(IOT_FotaHttpFile_Get_uc,"FotaHttptack", 1024*8, NULL, configMAX_PRIORITIES - 11, &HTTP_Handle);
						sCreateFotaTaskFlag = 1;
					}
				}
			}

		}
		break;
		case EFOTA_LOCAL_UPDATE:
		break;

		case EFOTA_LOCAL_UPDATE_DEVICE:
		{
			IOT_Historydate_us(ESP_WRITE , 60);      //升级过程中延后上传历史数据
			IOT_TCPDelaytimeSet_uc(ESP_WRITE ,60);   //升级过程中延后上传04数据

			if(0 == INVFOAT.InvUpdateFlag)
			{
				IOT_MasterFotaStatus_SetAndGet_ui(1 , EFOTA_FINISH);
			}
		}
		break;
		case EFOTA_FINISHING:
		{
			if(HTTP_Handle != NULL)		//Delete Update task
			{
				vTaskDelete(HTTP_Handle);
				HTTP_Handle = NULL;
			}

			if(1 == IOT_UpdateProgress_Upload_Handle(1))	//判断最后一次进度是否上传完成
			{
				if(SysTAB_Parameter[Parameter_len[REG_INUPGRADE]] != '0')	//判断是否需要升级逆变器
				{
					INVFOAT.InvUpdateFlag = 1;
					IOT_UpdateProgress_record(INVERTRE_PROGRESS,0); //上传第一次逆变器的升级进度
					IOT_MasterFotaStatus_SetAndGet_ui(1 , EFOTA_UPDATE_DEVICE);		//触发升级逆变器
				}
				else
				{
					IOT_MasterFotaStatus_SetAndGet_ui(1 , EFOTA_FINISH);	//上传完成,OTA Finish
				}
			}
		}
		break;
		case EFOTA_UPDATE_DEVICE:
		{
			IOT_Historydate_us(ESP_WRITE , 60);      //升级过程中延后上传历史数据
			IOT_TCPDelaytimeSet_uc(ESP_WRITE ,60);   //升级过程中延后上传04数据

			if(1 == IOT_UpdateProgress_Upload_Handle(1))	//判断最后一次下载进度是否上传完成
			{
				IOT_MasterFotaStatus_SetAndGet_ui(1 , EFOTA_FINISH);	//逆变器最后一次进度上传完成,OTA Finish
			}
		}
		break;
		case EFOTA_FINISH:
		{
			IOT_MasterFotaStatus_SetAndGet_ui(1 , EFOTA_NULL) ;
			sCreateFotaTaskFlag = 0;

			IOT_SYSRESTTime_uc(ESP_WRITE ,30 );		//30s后重启
			System_state.SYS_RestartFlag = 1 ;
		}
		break;
		default:
		break;
	}



	if((EFOTA_NULL != ucFotaStatus) && (EFOTA_FINISH != ucFotaStatus) \
			&& (EFOTA_UPDATE_DEVICE != ucFotaStatus) && ((EFOTA_LOCAL_UPDATE != ucFotaStatus))) //触发升级升级事务
	{
		IOT_Historydate_us(ESP_WRITE , 60);      //升级过程中延后上传历史数据
		IOT_TCPDelaytimeSet_uc(ESP_WRITE ,60);   //升级过程中延后上传04数据

		IOT_DeviceFotaTimeOut_Handle_uc(0xFF);	//监控升级是否超时
	}

}










