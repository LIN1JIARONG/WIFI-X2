#include "iot_local_update.h"
#include "iot_universal.h"
#include "iot_ota.h"
#include "esp_err.h"
#include "iot_fota.h"
#include "iot_system.h"

#include "iot_protocol.h"
#include "esp_log.h"
#include "iot_timer.h"
#include "iot_inverter_fota.h"
#include "iot_Local_Protocol.h"

#if 0
升级初始化完成标志需要优化，防止两种升级冲突
#endif

#define LOG_LOCAL_UPDATE "LOG_LOCAL_UPDATE"


extern uint8_t encrypt(uint8_t *pdata, uint16_t length);
extern uint8_t IOT_SystemParameterSet(uint16_t ParamValue ,char * data,uint8_t len);
/******************************************************************************
 * FunctionName : IOT_WiFiConnect_Handle_uc
 * Description  : WiFi连接状态判断
 * Parameters   : u8 ucMode_tem: 模式
 *               u16 usiVal_tem: 值
 * Returns      : 0-成功   / 1-失败
 * Notice       : none
*******************************************************************************/
uint8_t IOT_LocalFota_Handle_uc(uint8_t ucMode_tem, uint8_t *pucSrc_tem, uint16_t usiVal_tem)
{
	int err_t = 1;
	char cFotaType = EFOTA_TYPR_FULL;
	int OTA_TYPE =	EUPDADTE_NULL;

	if(ucMode_tem == 0) // 接收到第0包数据
	{

		cFotaType = IOT_FotaType_Manage_c(0xff, 0);

		if(cFotaType == EFOTA_TYPR_DATALOGER)
		{
			OTA_TYPE = EUPDADTE_COLLACTOR;
		}else
		{
			OTA_TYPE = EUPDADTE_INERTER;
		}

		if(ESP_OK == IOT_OTA_Init_uc(OTA_TYPE,g_FotaParams.ucIsbsdiffFlag))	//初始化升级
		{
			err_t = 0;
		}

	}
	else if(ucMode_tem == 1) // 接收到固件升级包数据
	{
		if(ESP_OK == IOT_OTA_Write_uc(pucSrc_tem , usiVal_tem))
		{
			err_t = 0;
		}
	}

	return err_t;
}


/******************************************************************************
 * FunctionName : LocalUpdate_ZeroPackage_judge_b
 * Description  : 判断本地升级第0包数据是否获取
 * Parameters   : u8 ucMode_tem: 模式	1 设置  0 复位  0xff 查询
 *               bool bFlags_tem: 值
 * Returns      : true-已获取   / false-未获取
 * Notice       : none
*******************************************************************************/
bool LocalUpdate_ZeroPackage_judge_b(uint8_t ucMode_tem , bool bFlags_tem)
{
	static bool bGetZeroPackageFlag = false;		//是否已经获取到第0包数据编号

	if(1 == ucMode_tem)	//设置flag
	{
		bGetZeroPackageFlag = bFlags_tem;
	}
	else if(0 == ucMode_tem)
	{
		bGetZeroPackageFlag = false;
	}
	else if(0xff == ucMode_tem)
	{
		return bGetZeroPackageFlag;
	}

	return false;
}


/******************************************************************************
 * FunctionName : IOT_LocalTCPAck0x26_Package
 * Description  : 本地升级0x26功能码响应数据打包
 * Parameters   : u8 ucMode_tem: 模式	1 打包数据    0xff 获取响应数据包
 *               uint8_t *TargetBUF 响应包存放的buff
 *                uint8_t AckData_tem[5] 应答数据
 * Returns      : 数据包的长度
 * Notice       : none
*******************************************************************************/
uint32_t IOT_LocalTCPAck0x26_Package(uint8_t ucMode_tem , uint8_t *TargetBUF, uint8_t AckData_tem[5])
{
	#ifndef _VERSION_		//协议版本
	#define	_VERSION_ 		0x0005
	#endif

	static uint8_t 	ACK_DATA[30] = {0};
	static uint16_t ACK_DATA_Len = 0;
	uint16_t len = 0;
	uint32_t uiCRC16 = 0;

	if(1 == ucMode_tem)	//打包数据
	{
		ACK_DATA[len++] = 0x00;	//通信编号
		ACK_DATA[len++] = 0x01;
		ACK_DATA[len++] = (_VERSION_ >> 8) & 0xff;	//协议版本
		ACK_DATA[len++] = _VERSION_ & 0xff;
		ACK_DATA[len++]	= 0x00;
		ACK_DATA[len++] = (2 + 10 + 5);
		ACK_DATA[len++] = 0x01;  //设备地址
		ACK_DATA[len++] = 0x26;  //功能码

		memcpy(&ACK_DATA[len], &SysTAB_Parameter[Parameter_len[REG_SN]], 10);		 //采集器SN号
		len += 10;
		memcpy(&ACK_DATA[18], AckData_tem, 5);
		len += 5;

		ACK_DATA_Len = len;
	}
	else if(0xff == ucMode_tem)
	{
		if(TargetBUF == NULL)
		{
			return 0;
		}


		if((LOCAL_PROTOCOL & 0x000000ff) == (ProtocolVS06  & 0x000000ff))	//本地通讯6.0协议
		{
			ACK_DATA_Len = IOT_Repack_ProtocolData_V5ToV6_us(ACK_DATA, ACK_DATA_Len , TargetBUF);
		}else
		{
			memcpy(TargetBUF, ACK_DATA, ACK_DATA_Len);
			encrypt(&TargetBUF[8], (ACK_DATA_Len - 8));		//异或加密
			IOT_Printf("\r\n***********************encrypt**************************  \r\n",ACK_DATA_Len);

			uiCRC16 = Modbus_Caluation_CRC16(TargetBUF, ACK_DATA_Len);
			TargetBUF[ACK_DATA_Len] = HIGH(uiCRC16);
			TargetBUF[ACK_DATA_Len+1] = LOW(uiCRC16);
			ACK_DATA_Len = ACK_DATA_Len + 2;

		}

		for(int i = 0 ; i < ACK_DATA_Len ; i++)
		{
			IOT_Printf("%02x",TargetBUF[i]);
		}
	}

	return ACK_DATA_Len;
}

/******************************************************************************
 * FunctionName : IOT_LocalTCPCode0x26_Handle() add LI 20210115
 * Description  : 接收本地 升级数据包
 * Parameters   : unsigned char *pucSrc_tem : 升级数据包
 * Returns      : none
 * Notice       : 0x19:
 * 						请求：0001 0005 0014 01 26 30303030303030303030 [0006 00FF 0001 0001 2*crc] - 下发总包 255/第一个包 /数据 0001
 * 						应答：0001 0005 0013 01 26 30303030303030303030 [00FF 0001 00 2*crc] - 下发总包 255/第一个包 /状态 00
*******************************************************************************/
void IOT_LocalTCPCode0x26_Handle(unsigned char *pucSrc_tem)
{
	#define SINGLE_PACKAGE_SIZE  512

	unsigned char ucAckBuf[5] = {0};
	unsigned char ucAcklen = 0;
	unsigned char ucDataOffset = 6;         // 数据偏移地址，6-除去数据长度2b / 总包数2b / 当前包数2b
	unsigned char i = 0;
	unsigned int  uiPackageLen = 0;         // 升级包长度
	unsigned int  uiSumPackageNums = 0;       // 总升级包数
	unsigned int  uiPointPackageNums = 0;     // 断点的包数
	unsigned int  uiCurrentPackageNums = 0; // 当前升级包编号
	uint8_t  ucReDownloadFlag = 0;

	#if 0
	字段解析

	命令数据格式
	pucSrc_tem[00 ~ 01]	数据区长度
	pucSrc_tem[02 ~ 03]	文件数据分包总数量
	pucSrc_tem[04 ~ 05]	当前数据包编号
	pucSrc_tem[06 ~ xx]	数据区内容

	响应的数据格式

	ucAckBuf[00 ~ 01]	文件数据分包总数量
	ucAckBuf[02 ~ 03]	当前数据包编号
	ucAckBuf[04]		接收状态码
	#endif


	ucAckBuf[0] = pucSrc_tem[2];
	ucAckBuf[1] = pucSrc_tem[3]; // 总包数据
	ucAckBuf[2] = pucSrc_tem[4];
	ucAckBuf[3] = pucSrc_tem[5]; // 当前包数
	ucAckBuf[4] = 0x00;          // 应答状态 -- 成功
	ucAcklen = 7;                // 应答长度

	uiPackageLen = IOT_htoi(&pucSrc_tem[0],2);
	uiSumPackageNums = IOT_htoi(&pucSrc_tem[2],2);
	uiCurrentPackageNums = IOT_htoi(&pucSrc_tem[4],2);

	IOT_Printf(" \r\nIOT_LocalTCPCode0x26_Handle \r\n");
	IOT_Printf(" uiPackageLen = %d\r\n",uiPackageLen);
	IOT_Printf(" uiSumPackageNums = %d\r\n",uiSumPackageNums);
	IOT_Printf(" uiCurrentPackageNums = %d\r\n",uiCurrentPackageNums);
//	IOT_ESP_Char_printf("IOT_LocalTCPCode0x26_Handle()_1", &pucSrc_tem[ucDataOffset], uiPackageLen-4);

	if((uiPackageLen > (SINGLE_PACKAGE_SIZE + 4)) || \
	   (uiSumPackageNums > ((2048 * 1024) / SINGLE_PACKAGE_SIZE)))     // 单包升级数据包长度大于 1024字节 / 总升级数据包大小> 2M
	{
		ucAckBuf[4] = 0x02;          // 应答状态 -- 单包超长度失败失败
		goto ACK_FAIL_0X26;
	}

	if(uiCurrentPackageNums == 0x00) // 第一包数据  开始读取断点数据包编号  开始做升级准备
	{

		if(IOT_LocalFota_Handle_uc(0, &pucSrc_tem[ucDataOffset], (uiPackageLen-4))) // 写入第一包数据, 升级初始化
		{
			ucAckBuf[4] = 0x01;      // 应答状态 -- 错误 1
			goto ACK_FAIL_0X26;
		}

		IOT_MasterFotaStatus_SetAndGet_ui(1 , EFOTA_LOCAL_UPDATE);  //设置状态为本地升级
		//校验文件头,检查断点位置
		if(0 == IOT_FotaFile_HEAD_Check_c(&pucSrc_tem[ucDataOffset]))
		{
			//Flash已保存有相同的文件,无需重复下载
			ucReDownloadFlag = 1;	//add chen 20220712  已保存有相同的文件 不需要重复校验
			ESP_LOGI(LOG_LOCAL_UPDATE,"The Flash has saved the same file!!");
			uiPointPackageNums = uiSumPackageNums;
			ucAckBuf[2] = (uiPointPackageNums >> 8 ) & 0xff; //高位
			ucAckBuf[3] = uiPointPackageNums  & 0xff ; 		// 低位


		}else
		{
			#if 0	//测试代码
			g_FotaParams.uiPointSize = 0 ;
			IOT_FotaParams_Write_c(4, g_FotaParams.uiPointSize);
			#endif

			//应答的断点位置
			uiPointPackageNums = g_FotaParams.uiPointSize / SINGLE_PACKAGE_SIZE;
			g_FotaParams.uiPointSize = (uiPointPackageNums * SINGLE_PACKAGE_SIZE); //断点起始地址 需要为 SINGLE_PACKAGE_SIZE 的倍数

			ucAckBuf[2] = (uiPointPackageNums >> 8 ) & 0xff; //高位
			ucAckBuf[3] = uiPointPackageNums  & 0xff ; // 低位

			ESP_LOGI(LOG_LOCAL_UPDATE,"uiPointSize = %d, PointPackageNums = %d",g_FotaParams.uiPointSize,uiPointPackageNums);
		}

		IOT_OTA_Set_Written_Offset_uc(g_FotaParams.uiPointSize); //设置升级保存数据的位置偏移
		LocalUpdate_ZeroPackage_judge_b(1 , true);		//标记已经获取编号0的数据

	}
	else // 开始保存升级文件包
	{
		if(LocalUpdate_ZeroPackage_judge_b( 0xff , 0) != true)	//未获取到第0包数据,重新获取
		{
			ucAckBuf[2] = 0x00;	//设置应答编号为0
			ucAckBuf[3] = 0x00;
			ucAckBuf[4] = 0x03;      // 应答状态 -- 03 其他错误
			goto ACK_FAIL_0X26;
		}
		if(IOT_LocalFota_Handle_uc(1, &pucSrc_tem[ucDataOffset], (uiPackageLen-4))) // 写入升级包数据
		{
			ucAckBuf[4] = 0x01;      // 应答状态 -- 单包超长度失败失败
			goto ACK_FAIL_0X26;
		}
		g_FotaParams.uiPointSize += (uiPackageLen - 4);		//断点位置更新
		if((uiCurrentPackageNums != 0) && (uiCurrentPackageNums % 40 == 0))		//每40包保存一次断点
		{
			IOT_FotaParams_Write_c(4, g_FotaParams.uiPointSize); // 记录HTTP断点下载保存数据长度：编号=4

			uint8_t ucDLProgress = (uiCurrentPackageNums * 100) / uiSumPackageNums;

			char sProgress[3] = {0};

			if(ucDLProgress >= 100)
			{
				ucDLProgress = 99;   //最后1% 需要校验完成在更新
			}

			sProgress[0] ='0' + ((ucDLProgress / 100) % 10);
			sProgress[1] ='0' + ((ucDLProgress / 10) % 10) ;
			sProgress[2] ='0' + (ucDLProgress % 10) ;

			IOT_SystemParameterSet(101 ,(char *)sProgress , 3);  //保存进度
		}
	}
	if((uiSumPackageNums == uiCurrentPackageNums) || (1 == ucReDownloadFlag)) // 升级文件包下发完成，开始触发升级
	{
		if((1 == ucReDownloadFlag) || (true == IOT_FotaFile_Integrality_Check_b(1)))	//校验通过
		{
			char cOTAType = IOT_FotaType_Manage_c(0xff, 0);//获取升级的类型
			ucAckBuf[4] = 0x00;          // 应答状态 -- 成功
			LocalUpdate_ZeroPackage_judge_b(1 , false);	//清空获取文件头标志
			if(cOTAType == EFOTA_TYPR_DATALOGER)
			{
				if(true == g_FotaParams.ucIsbsdiffFlag)	//判断是否进行差分升级
				{
					iot_Bsdiff_Update_Judge_uc(1);
				}
				else
				{
					IOT_MasterFotaStatus_SetAndGet_ui(1 , EFOTA_FINISH);

					if(true == IOT_FotaFile_DeviceType_Check_b())	//校验升级文件的设备类型
					{
						IOT_SystemParameterSet(101 ,(char *)"100" , 3);  //保存进度
						IOT_OTA_Set_Boot();		//设置启动分区
					}else
					{
						IOT_SystemParameterSet(101 ,(char *)"134" , 3);  //失败
					}
					IOT_OTA_Finish_uc();	//OTA结束
					IOT_SYSRESTTime_uc(ESP_WRITE, 30 );  //等待30s 后重启
					System_state.SYS_RestartFlag = 1;
				}
			}else
			{
				INVFOAT.InvUpdateFlag = 1;		//触发升级逆变器
				//IOT_MasterFotaStatus_SetAndGet_ui(1 , EFOTA_UPDATE_DEVICE);
				IOT_MasterFotaStatus_SetAndGet_ui(1 ,EFOTA_LOCAL_UPDATE_DEVICE);
			}

			if(0 == ucReDownloadFlag)
			{
				IOT_FotaParams_Write_c(4, g_FotaParams.uiPointSize); //保存断点
			}

		}else	//下载文件校验不通过
		{
			g_FotaParams.uiPointSize = 0;  				//断点清0	,下次下载从0开始
			IOT_FotaParams_Write_c(4, g_FotaParams.uiPointSize); //保存断点
			LocalUpdate_ZeroPackage_judge_b(0 , 0);	 		//获取文件头标志清0

			IOT_SystemParameterSet(101 ,(char *)"134" , 3);  //保存进度,固件非法

			IOT_MasterFotaStatus_SetAndGet_ui(1 , EFOTA_FINISH);
			ucAckBuf[4] = 0x02;      // 应答状态 -- 整体校验错误
			goto ACK_FAIL_0X26;
		}
	}

ACK_FAIL_0X26:

IOT_LocalTCPAck0x26_Package(1 ,NULL ,ucAckBuf);	//打包0x26功能码响应包

	//IOT_LocalFotaStatus_check_uc(); // 升级状态监测

}


