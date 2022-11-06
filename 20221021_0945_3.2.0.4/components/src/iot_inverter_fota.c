/*
 * iot_inverter_fota.c
 *
 *  Created on: 2022年3月21日
 *      Author: Administrator
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "iot_fota.h"
#include "iot_net.h"
#include "iot_uart1.h"
#include "iot_spi_flash.h"
#include "iot_system.h"
#include "iot_crc32.h"
#include "iot_protocol.h"
#include "iot_inverter.h"
#include "iot_universal.h"

#include "iot_inverter_fota.h"
#include "iot_fota.h"
#include "iot_crc32.h"

/* 升级命令码，每当添加升级命令码寄存器后，这几个数据必须要进行相应修改 */
unsigned char GucCode_06_register[Code_06_register_num] = {0x02,0x16,0x1F};
unsigned char GucCode_17_register[Code_17_register_num] = {0x02,0x03,0x04,0x05,0x06};
unsigned char GucCode_03_register[Code_03_register_num] = {0x1F};


unsigned char IOTESPUsart0RcvBuf[UsartDataMaxLen];
unsigned char IOTESPUsart0RcvLen=0;
GPFotaParam GPFotaParams[GPFotaCodeNum];

static uint16_t _GusiFotaFileType = file_bin; 		// 默认升级文件类型-bin文件
static uint8_t _GucGprotocolVersion_H = 0;    		// 数服协议版本值-高位
static uint8_t _GucGprotocolVersion_L = 0;    		// 数服协议版本值-低位
static uint8_t _GucInverterAddr = 0;          		// 逆变器地址
static uint8_t _GucInverterID[10];            		// 逆变器序列号
static uint8_t _GucCollectorID[10];           		// 采集器器序列号
static uint32_t _Gui_IAP_CACHE_ADDR;			 	// IAP数据缓存区,4Mbyte后
static uint32_t _Gui_APPLENGTH_ADDR;		     	// boot执行选择开关存放地址
//static uint32_t _Gui_PVUPDATA_DATAADDR = 4096;		// 升级文件实际偏移  2017.07.05 请参考：iot_fota。h升级地址定义宏
static uint32_t _GuiInverterFotaTime = 0;     		// 升级时间
static uint32_t _GuiFotaFileSize = 0;		 		// 升级文件大小
static uint32_t _GuiFotaFileCRC = 0;          		// 升级文件CRC值
static uint8_t _GuInverterFotaStatus = code_wait; 	// 升级状态

static uint8_t _GuInverterFotaERRCode = 0;    		// 逆变器升级错误码

typedef struct {
	uint32_t uiFileCRC32 ;
	uint32_t uiFileLen;
	uint32_t  FileAddr;
	uint32_t uiFlashType;

}PVFOTAPARAM;


PVFOTAPARAM SPvFotaParam = {0};



INVFota_date INVFOAT;
GPFotaCode_register GPFotaCode_registers;

extern void IOT_SPI_External_FLASH_BufferRead(uint8_t *buff , uint32_t addr , uint32_t read_len);
extern void IOT_SPI_System_FLASH_BufferRead(uint8_t *buff , uint32_t addr , uint32_t read_len);

static void (*pIOT_SPI_FLASH_BufferRead)(uint8_t *buff , uint32_t addr , uint32_t read_len) = NULL;



/******************************************************************************
 * FunctionName : IOT_Task_Delay_ms
 * Description  : 任务延时函数
 * Parameters   : uint32_t DelayTime_ms 延时的时间 单位 毫秒
 * Returns      :进入延时状态时，任务会挂起，延时结束后，会抢占低优先级的任务（需要rtos开启抢占）
*******************************************************************************/
void IOT_Task_Delay_ms(uint32_t DelayTime_ms)
{
	if(DelayTime_ms != 0)
	vTaskDelay(DelayTime_ms  / portTICK_PERIOD_MS);
}



/******************************************************************************
 * FunctionName : IOT_InverterFotaParameter_Config
 * Description  :升级参数初始化
 * Parameters   : void
 * Returns      :
*******************************************************************************/
void IOT_InverterFotaParameter_Config(void)
{

	SPvFotaParam.uiFileCRC32 = IOT_Get_OTA_FileCRC32Val_ui();
	SPvFotaParam.uiFileLen = IOT_Get_OTA_FileSize_ui();
	SPvFotaParam.FileAddr = IOT_Get_OTA_FileStartAddr_ui();
	SPvFotaParam.uiFlashType =	IOT_Get_OTA_FlashType_ui();

	_Gui_IAP_CACHE_ADDR = SPvFotaParam.FileAddr;

	if(External_FLASH == SPvFotaParam.uiFlashType)	//根据文件所在flash的位置,选择不同的flash数据读取函数
	{
		pIOT_SPI_FLASH_BufferRead = IOT_SPI_External_FLASH_BufferRead;
	}else
	{
		pIOT_SPI_FLASH_BufferRead = IOT_SPI_System_FLASH_BufferRead;
	}

	ESP_LOGI("Inverter fota","SPvFotaParam.uiFileCRC32 = %x",SPvFotaParam.uiFileCRC32);
	ESP_LOGI("Inverter fota","SPvFotaParam.uiFileLen = %d",SPvFotaParam.uiFileLen);
	ESP_LOGI("Inverter fota","SPvFotaParam.FileAddr = %x",SPvFotaParam.FileAddr);
	ESP_LOGI("Inverter fota","SPvFotaParam.uiFlashType = %d",SPvFotaParam.uiFlashType);


}




/******************************************************************************
 * FunctionName : InverterFotaTimeOut_Judge
 * Description  : 系统升级超时判断
 * Parameters   : void
 * Returns      : 2017.06.16 外部软件定时器循环调用
*******************************************************************************/
static uint8_t InverterFotaTimeOut_Judge(void)
{
	#define OUTTIME ((20) * (60) * (1000))  // 10分钟 : 1000 = 1s   20201120 10min --> 20min 兼容储能电池升级

	if(((GetSystemTick_ms() - _GuiInverterFotaTime)/1000) >= OUTTIME)
	{
		IOT_Printf("\r\n\r\n\r\n\r\n\r\n\r\n\r\n**************************************************");
		IOT_Printf("InverterFotaTimeOut_Judge OUTTIME = %d\r\n",OUTTIME);
		IOT_Printf("InverterFotaTimeOut_Judge OUTTIME cout = %d\r\n",((GetSystemTick_ms() - _GuiInverterFotaTime)/1000));
		IOT_Printf("InverterFotaTimeOut_Judge IOT_ESP_System_Get_Time() = %d\r\n",GetSystemTick_ms());
		IOT_Printf("InverterFotaTimeOut_Judge _GuiInverterFotaTime = %d\r\n",_GuiInverterFotaTime);
		IOT_Printf("**************************************************\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n");
		_GuiInverterFotaTime = 0;
		_GuInverterFotaStatus = code_end;  // 升级结束
		return 0;
	}

//   if(GucIOTESPInverterFotaStatus == ESP_RESET) // 服务器下发取消--升级操作
//   {
//		IOT_Printf("InverterFotaTimeOut_Judge server cancel fota  !!! \r\n");
//		_GuiInverterFotaTime = 0;
//		_GuInverterFotaStatus = code_end;  // 升级结束
//		return 0;
//   }

	if(_GuInverterFotaStatus == code_end)return 0;

	return 1;
}

/******************************************************************************
 * FunctionName : IOT_ESP_IFota_Dead_Delayms
 * Description  : esp 逆变器升级死亡等待
 * Parameters   : uint32_t nTime   : 延时时间
 * Returns      : none
 * Notice       : none
*******************************************************************************/
static bool IOT_ESP_IFota_Dead_Delayms(uint32_t nTime)
{
	 static unsigned int i=0;
	 uint16_t uiSingleDelayTime = 50;      // 50ms

	 if(!InverterFotaTimeOut_Judge()) // ！！！ 判断是否升级超时
	 {
		 i=0;return 0;// 升级超时不再做死亡延时
	 }

	 if(nTime==0){ i=0;return 0; }

	 if(i >= (nTime/uiSingleDelayTime)){ i=0;return 0; }

     i++;
     IOT_Task_Delay_ms(uiSingleDelayTime );

	 return 1;
}

/******************************************************************************
 * FunctionName : InverterFotaGPCode_Send
 * Description  : 发送逆变器升级协议命令码
 * Parameters   : uint16_t len
 *                uint8_t *src
 * Returns      : none
 * Notice       : 0x03 / 0x06 / 0x17
*******************************************************************************/
static void InverterFotaGPCode_Send(uint16_t len , uint8_t *src)
{
	if(_GuInverterFotaStatus == code_end)return;

	//IOT_ESP_Usart_Write(UART0,len,src);
	IOT_UART1SendData(src,len);
}

/******************************************************************************
 * FunctionName : InverterFotaMsg_Send
 * Description  : 发送逆变器升级信息
 * Parameters   : uint8_t msg
 * Returns      : none
 * Notice       : msg: 0~0x64-升级进度   0x86-固件非法    0x87-逆变器不允许   0x88-烧录超时   0x89-烧录失败
*******************************************************************************/
static void InverterFotaMsg_Send(uint8_t msg)
{
    //状态码 赋值

	IOT_UpdateProgress_record(INVERTRE_PROGRESS  ,msg);		//上传逆变器进度
}

/******************************************************************************
 * FunctionName : InverterFotaFileCRC_Get
 * Description  : 获取升级文件CRC
 * Parameters   : uint32_t addr
 * Returns      : CRC值
*******************************************************************************/
static uint32_t InverterFotaFileCRC_Get(void)
{

	return (SPvFotaParam.uiFileCRC32);

}

/******************************************************************************
 * FunctionName : InverterFotaFileData_Get
 * Description  : 获取升级文件数据
 * Parameters   : uint32_t addr
 * Returns      : CRC值
*******************************************************************************/
static uint32_t InverterFotaFileData_Get(uint32_t addr, uint8_t *src, uint16_t len)
{

	if(NULL != pIOT_SPI_FLASH_BufferRead)
	{
		pIOT_SPI_FLASH_BufferRead(src , addr , len);
	}

	return 1;
}
/******************************************************************************
 * FunctionName : InverterFotaFileSize_Get
 * Description  : 获取升级文件大小
 * Parameters   : void
 * Returns      : 0-文件长度出错         !0-文件正确
*******************************************************************************/
static uint32_t InverterFotaFileSize_Get(void)
{
	uint32_t CRC32Check_1 = 0;
	uint32_t CRC32Check_2 = 0;
	uint32_t APPLength = 0;
	uint32_t APPLength_1 = 0;
	uint8_t buff[10] = {0};

	IOT_Printf("开始进行升级前的文件长度校验 !\r\n");

	CRC32Check_1 = FLASH_GetCRC32(SPvFotaParam.FileAddr, SPvFotaParam.uiFileLen , SPvFotaParam.uiFlashType);// ！！！计算CRC32 crc计算加了偏移
	CRC32Check_2 = SPvFotaParam.uiFileCRC32;


	IOT_Printf("开始进行升级前的文件crc校验   %x %x !\r\n",CRC32Check_1,CRC32Check_2);

	if(CRC32Check_1 == CRC32Check_2)
	{

		IOT_Printf("开始进行升级前的文件CRC校验 Success !\r\n");
		APPLength = SPvFotaParam.uiFileLen;

	}
	else
	{
		IOT_Printf("开始进行升级前的文件CRC校验 Fail !\r\n");
		APPLength = 0;
	}




	return APPLength;



}



/******************************************************************************
 * FunctionName : IOT_ESP_NEW_InvUpdata_CodeRegister_Init
 * Description  : 逆变器升级命令码寄存器初始化
 * Parameters   : none
 * Returns      : none
 * Notice       : none
*******************************************************************************/
void IOT_ESP_NEW_InvUpdata_CodeRegister_Init(GPFotaCode_register *p_register)
{
	memcpy(p_register->Code_06_register, GucCode_06_register, Code_06_register_num);
	memcpy(p_register->Code_17_register, GucCode_17_register, Code_17_register_num);
	memcpy(p_register->Code_03_register, GucCode_03_register, Code_03_register_num);

//	IOT_ESP_Char_printf("Code_06_register",p_register->Code_06_register,Code_06_register_num);
//	IOT_ESP_Char_printf("Code_17_register",p_register->Code_17_register,Code_17_register_num);
//	IOT_ESP_Char_printf("Code_03_register",p_register->Code_03_register,Code_03_register_num);
}


/******************************************************************************
 * FunctionName : IOT_ESP_NEW_InvUpdata_Code_Init
 * Description  : 逆变器升级命令码初始化
 * Parameters   : PFotaParam *p_param
				  unsigned char _code
				  GPFotaCode_register *_coderegister
				  unsigned char _sendtimes
				  unsigned int  _sendinterval
 * Returns      : none
 * Notice       : none
*******************************************************************************/
void IOT_ESP_NEW_InvUpdata_Code_Init(GPFotaParam *p_param,\
									 unsigned char _code,\
									 unsigned char _coderegister,\
									 unsigned char _sendtimes,\
									 unsigned int  _sendinterval,\
									 unsigned char  status)
{
	p_param->code =  _code;
	p_param->coderegister = _coderegister;
	p_param->sendtimes = _sendtimes;
	p_param->sendinterval = _sendinterval;

	IOT_Printf("\r\n*********************************************\r\n");
	IOT_Printf("\r\n_code = %d\r\n",p_param->code);
	IOT_Printf("coderegister = %d\r\n",p_param->coderegister);
	IOT_Printf("sendtimes = %d\r\n",p_param->sendtimes);
	IOT_Printf("sendinterval = %d\r\n",p_param->sendinterval);
	IOT_Printf("status = %d\r\n",p_param->status);
	IOT_Printf("\r\n*********************************************\r\n");
}

/******************************************************************************
 * FunctionName : NEW_InvUpdataParam_Init
 * Description  : 新升级协议升级前，初始化逆变器升级需要的参数
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void NEW_InvUpdataParam_Init(void)
{
	// 更改44寄存器的值,逆变器升级文件类型(hex-'2'/bin-'3'/mot-'4'/out-'5')
	if(SysTAB_Parameter[Parameter_len[REG_INUPGRADE]] == '2')
	{
		_GusiFotaFileType = file_hex;
	}
	else if(SysTAB_Parameter[Parameter_len[REG_INUPGRADE]] == '3')
	{
		_GusiFotaFileType = file_bin;
	}

	IOT_InverterFotaParameter_Config();  //获取升级文件的地址,大小,CRC32

	/* 得到通讯版本号，用来做服务器应答 */
	_GucGprotocolVersion_H = 0x00;
	_GucGprotocolVersion_L = PROTOCOL_VERSION_L;

	/* 得到逆变器地址 */
	_GucInverterAddr = InvCommCtrl.Addr;

	/* 得到逆变器 和采集器序列号 */
	memcpy(_GucInverterID , &InvCommCtrl.SN , 10 );
	memcpy(_GucCollectorID , &SysTAB_Parameter[Parameter_len[REG_SN]] , 10 );

//	strncpy(_GucInverterID, GucIOTESPInverterID,10);
//	strncpy(_GucCollectorID, GucIOTESPCollectorID,10);

	/* 获得升级文件flash存储地址 */
	//_Gui_IAP_CACHE_ADDR = IAP_CACHE_ADDR;  /* 升级数据包包头: 地址偏移 - 数据包长度 - 数据包crc32校验值 - 预留 - 预留 (各4个字节) */
	//_Gui_APPLENGTH_ADDR = APPLENGTH_ADDR;

	/* 获得升级文件大小  (注：_Gui_IAP_CACHE_ADDR + 4 ----> (4) 表示4-7为升级文件实际长度)*/
	_GuiFotaFileSize = InverterFotaFileSize_Get();

	/* 获得升级文件CRC*/
	_GuiFotaFileCRC = InverterFotaFileCRC_Get();

	/* 获得升级初始时间  (注：_Gui_IAP_CACHE_ADDR + 8 ----> (8) 表示8-11为升级文件CRC校验值)*/
	_GuiInverterFotaTime = GetSystemTick_ms();

	/* 升级状态置位 */
	_GuInverterFotaStatus = code_ing;

	IOT_Printf("\r\n");
	IOT_Printf("*********************************************\r\n");
	IOT_Printf("*********NEW_InvUpdataParam_Init*************\r\n");
	IOT_Printf("*********************************************\r\n");
	IOT_Printf("\r\n _GusiFotaFileType = %02x\r\n",_GusiFotaFileType);
	IOT_Printf("_GucGprotocolVersion_L = %d\r\n",_GucGprotocolVersion_L);
	IOT_Printf("_GucInverterAddr = %d\r\n",_GucInverterAddr);
	IOT_Printf("_GucInverterID = %s\r\n",_GucInverterID);
	IOT_Printf("_GucCollectorID = %s\r\n",_GucCollectorID);
	IOT_Printf("_Gui_IAP_CACHE_ADDR = %x\r\n",_Gui_IAP_CACHE_ADDR);
	IOT_Printf("_Gui_APPLENGTH_ADDR = %x\r\n",_Gui_APPLENGTH_ADDR);
	IOT_Printf("_GuiFotaFileSize = %d\r\n",_GuiFotaFileSize);
	IOT_Printf("_GuiFotaFileCRC = %x\r\n",_GuiFotaFileCRC);
	IOT_Printf("_GuiInverterFotaTime = %d\r\n",_GuiInverterFotaTime);
	IOT_Printf("_GuInverterFotaStatus = %d\r\n",_GuInverterFotaStatus);
	IOT_Printf("*********************************************\r\n");
	IOT_Printf("*********NEW_InvUpdataParam_Init*************\r\n");
	IOT_Printf("*********************************************\r\n");

}

/******************************************************************************
 * FunctionName : IOT_ESP_NEW_InvUpdataInit
 * Description  : 逆变器升级初始化
 * Parameters   : none
 * Returns      : none
 * Notice       : none
*******************************************************************************/
void IOT_ESP_NEW_InvUpdataInit(void)
{
	/* 升级命令码寄存器初始化  */
	IOT_ESP_NEW_InvUpdata_CodeRegister_Init(&GPFotaCode_registers);

	/* 升级命令码初始化  */
	IOT_ESP_NEW_InvUpdata_Code_Init(&GPFotaParams[GPFotaCode_06],\
									GPFotaCode_06,\
									GucCode_06_register[Code_06_register_02],\
									FotaSendTimes*3,\
									FotaSendinterval*1,\
									code_wait);

	IOT_ESP_NEW_InvUpdata_Code_Init(&GPFotaParams[GPFotaCode_17],\
									GPFotaCode_17,\
									GucCode_17_register[Code_17_register_02],\
									FotaSendTimes,\
									FotaSendinterval*1,\
									code_wait);

	IOT_ESP_NEW_InvUpdata_Code_Init(&GPFotaParams[GPFotaCode_03],\
									GPFotaCode_03,\
									GucCode_03_register[Code_03_register_1F],\
									FotaSendTimes,\
									FotaSendinterval*1,\
									code_wait);

	NEW_InvUpdataParam_Init();  // 升级参数初始化
}



/******************************************************************************
 * FunctionName : FotaCode_06_xx_Send
 * Description  : 逆变器升级发送0x06_0xxx命令码(xx == 0x02/0x16/0x1f)
 * Parameters   : uint8_t mode
 * Returns      : none
 * Notice       : 发送数据格式：addr 0x06 0x00 0x-- 0x00 0x-- CRC_H CRC_L
 *                应答数据格式：addr 0x06 0x00 0x-- 0x00 0x-- CRC_H CRC_L
 *                错误应答数据格式：addr 0x86 0x01 CRC_H CRC_L
*******************************************************************************/
static bool FotaCode_06_xx_Send(GPFotaParam *p_param, uint8_t *src, uint8_t Addr, uint16_t reg, uint16_t val)
{
	uint16_t checkvalid_len = 0;               // 有效总长度
	uint16_t checksum_len = 0;                 // 总长度(checkvalid_len + 2(crc_len))
	uint16_t err_checkvalid_len = 3;           // 出错校验数据有效总长度
	uint16_t err_checksum_len = 5;             // 出错校验数据总长度(err_checkvalid_len + 2(crc_len))
	uint8_t sendtimes = 0;                     // 统计命令码发送次数
	uint8_t rcvcheckbuff[16] = {Addr};         // 正确应答校验
	uint8_t err_rcvcheckbuff[5] = {Addr,0x86,0x01}; // 错误应答校验
	uint16_t crcval = 0;                            // crc校验码值
	uint16_t send_sum_len = 0;                      // 发送总长度


	src[0] = Addr;                        // 地址
	src[1] = 0x06;                        // 命令码
	src[2] = (uint8_t)((reg>>8)&0x00FF);       // 寄存器
	src[3] = (uint8_t)(reg&0x00FF);
	src[4] = (uint8_t)((val>>8)&0x00FF);       // 写入值
	src[5] = (uint8_t)(val&0x00FF);

	checkvalid_len = 6;

	/* 数据做crc校验 */
	crcval = Modbus_Caluation_CRC16(src,checkvalid_len);
	src[6] = (uint8_t)(crcval&0x00FF);         // crc值低位
	src[7] = (uint8_t)((crcval>>8)&0x00FF);    // crc值高位
	send_sum_len = checkvalid_len + 2;

	/* 得到用来校验接收数据(正确 、错误) */
	checkvalid_len = 6;
	checksum_len = checkvalid_len + 2;
	IOT_ESP_strncpy(rcvcheckbuff, src, checksum_len);

	crcval = Modbus_Caluation_CRC16(err_rcvcheckbuff,err_checkvalid_len);
	err_rcvcheckbuff[3] = (uint8_t)(crcval&0x00FF);         // crc值低位
	err_rcvcheckbuff[4] = (uint8_t)((crcval>>8)&0x00FF);    // crc值高位

	/* 发送数据前清空接收缓冲区数据 */
	memset(IOTESPUsart0RcvBuf , 0, UsartDataMaxLen);
	IOTESPUsart0RcvLen = 0;

	/* 开始发送第一包命令码数据 */
	InverterFotaGPCode_Send(send_sum_len,src);

	/* 命令码数据收发 */
	while(1)
	{
		/* 命令码返回数据应答判断 */
		if(IOT_ESP_strstr(IOTESPUsart0RcvBuf, rcvcheckbuff, IOTESPUsart0RcvLen,checksum_len-2))  // 正常应答 (注：检验所有字节 ，除 crc)
		{
			crcval = Modbus_Caluation_CRC16(IOTESPUsart0RcvBuf,checksum_len-2);// 接收数据进行crc
			if((IOTESPUsart0RcvBuf[checksum_len-2]) == (uint8_t)(crcval&0x00FF) && (IOTESPUsart0RcvBuf[checksum_len-1] == (uint8_t)((crcval>>8)&0x00FF)))
			{
				IOT_Printf("FotaCode_06_%x_Receive !\r\n",reg);
				IOT_ESP_IFota_Dead_Delayms(0);
				return 1;
			}
		}
		else if(IOT_ESP_strstr(IOTESPUsart0RcvBuf, err_rcvcheckbuff, IOTESPUsart0RcvLen,err_checksum_len))// 错误应答
		{

		}

		/* 发送超时判断 */
		if(!IOT_ESP_IFota_Dead_Delayms(p_param->sendinterval))// 0x06 死亡超时设置为3s
		{
			sendtimes++;

			if(sendtimes >= p_param->sendtimes)// 命令码超过发送最大允许次数，结束发送
			{
				p_param->status = code_wait;
				return 0;
			}

			/* 发送数据前清空接收缓冲区数据 */
			memset(IOTESPUsart0RcvBuf,0,UsartDataMaxLen);
			IOTESPUsart0RcvLen = 0;
			/* 开始发送数据 */
			InverterFotaGPCode_Send(send_sum_len , src);  // 命令码无应答后，再次请求命令码
		}
	}

	return 1;
}

/******************************************************************************
 * FunctionName : FotaCode_06_Send
 * Description  : 逆变器升级发送0x06命令码
 * Parameters   : GPFotaParam *p_param
 *                uint8_t mode
 * Returns      : none
 * Notice       : none
*******************************************************************************/
static bool FotaCode_06_Send(GPFotaParam *p_param, uint8_t *src, uint8_t mode, uint8_t Addr)
{
	uint8_t status = 1;
	uint16_t reg_02_val = 0;
	uint16_t reg_16_val = 0;
	uint16_t fotafiletype = 0x0000;  // 升级文件类型(0x0001=bin\0x0010=hex)

	if(mode == 1)
	{
		reg_16_val = 0x0001; // 波特率：38400
	}
	else if(mode == 0)
	{
		reg_16_val = 0x0000; // 波特率：9600
	}

	reg_02_val = 0x0001;               // 波特率属性--保存
	fotafiletype = _GusiFotaFileType;  // 升级文件类型，默认bin文件(0x0001=bin\0x0010=hex)

	if(status)
	{/* 发送请求修改串口波特率并不保存 */
		status &= FotaCode_06_xx_Send(p_param, src, Addr, (uint16_t)GucCode_06_register[Code_06_register_02], reg_02_val);
	}

	if(status)
	{/* 发送请求修改串口波特率 */
		status &= FotaCode_06_xx_Send(p_param, src, Addr, (uint16_t)GucCode_06_register[Code_06_register_16], reg_16_val);
	}

	if(status)
	{
		if(mode == 1)
		{
//			if(GuiIOTESPBardRateTrialSyncVal != 115200) // 遇到波特率为115200时，不修改波特率
//			{
//				/* 修改串口波特率为38400 */
//				IOT_ESP_UsartBaudrate_Set(UART0, 38400);
//			}
//
			status &= FotaCode_06_xx_Send(p_param, src, Addr, (uint16_t)GucCode_06_register[Code_06_register_1F], fotafiletype);

		}
		if(mode == 0)
		{
//			if(GuiIOTESPBardRateTrialSyncVal != 115200) // 遇到波特率为115200时，不修改波特率 LI 2018.08.28
//			{
//				IOT_ESP_UsartBaudrate_Set(UART0, 9600);
//			}
		}
	}

	return status;
}



/******************************************************************************
 * FunctionName : FotaCode_17_xx_Send
 * Description  : 逆变器升级发送0x17_0xxx命令码(0x02/0x03/0x04/0x05/0x06)
 * Parameters   : uint8_t mode
 * Returns      : none
 * Notice       : 1、寄存器不同，对应的应答返回信息也不同

 *                发送数据格式：addr 0x17 0x02 0x00 0x04 0x-- 0x-- 0x-- 0x-- CRC_H CRC_L
 *                应答数据格式：addr 0x17 0x02 0x00 0x02 0x00 0x00 CRC_H CRC_L
 *
 *                发送数据格式：addr 0x17 0x03 0x00 0x04 0x-- 0x-- 0x-- 0x-- CRC_H CRC_L
 *                应答数据格式：addr 0x17 0x03 0x00 0x02 0x00 0x00 CRC_H CRC_L
 *
 *                发送数据格式：addr 0x17 0x04 0x00 0x02 0x00 0x00 CRC_H CRC_L
 *                应答数据格式：addr 0x17 0x04 0x00 0x02 0x00 0x00 CRC_H CRC_L
 *
 *                发送数据格式：addr 0x17 0x05 0x-- 0x-- 0x-- 0x-- Byte0 ... Byte255 CRC_H CRC_L
 *                应答数据格式：addr 0x17 0x05 0x00 0x02 0x-- 0x-- CRC_H CRC_L
 *
 *                发送数据格式：addr 0x17 0x06 0x00 0x02 0x00 0x00 CRC_H CRC_L
 *                应答数据格式：addr 0x17 0x06 0x00 0x02 0x00 0x00 CRC_H CRC_L
 *
 *                错误应答数据格式：addr 0x97 0x01 CRC_H CRC_L
*******************************************************************************/
#define onepacketsize    256              // 每次传输数据包大小(暂定256byte)
static bool FotaCode_17_xx_Send(GPFotaParam *p_param, uint8_t *src, uint8_t Addr, uint16_t reg, uint16_t len,\
		                                       uint32_t data, uint32_t crc, uint16_t num, uint8_t *buf)
{
	uint16_t checkvalid_len = 0;                // 有效总长度
	uint16_t checksum_len = 0;                  // 总长度(checkvalid_len + 2(crc_len))
	uint16_t err_checkvalid_len = 3;            // 出错校验数据有效总长度
	uint16_t err_checksum_len = 5;              // 出错校验数据总长度(err_checkvalid_len + 2(crc_len))
	uint8_t sendtimes = 0;                      // 统计命令码发送次数
	uint8_t rcvcheckbuff[16] = {Addr,0x17,reg};      // 正确应答校验
	uint8_t err_rcvcheckbuff[5] = {Addr,0x97,0x01};  // 错误应答校验
	uint16_t crcval = 0;                             // crc校验值
	uint16_t send_sum_len = 0;                       // 发送总长度

	src[0] = Addr;                   // 命令码
	src[1] = 0x17;                   // 命令码
	src[2] = (uint8_t)reg;                // 寄存器
	src[3] = (uint8_t)((len>>8)&0x00FF);  // 长度高位
	src[4] = (uint8_t)(len&0x00FF);       // 长度低位

	if((reg == GucCode_17_register[Code_17_register_04]) || \
	   (reg == GucCode_17_register[Code_17_register_06]))
	{
		checkvalid_len = 7;

		src[5] = (uint8_t)((data>>8)&0x00FF);  // 数据高位
		src[6] = (uint8_t)(data&0x00FF);       // 数据低位

		/* 数据做crc校验 */
		crcval = Modbus_Caluation_CRC16(src,checkvalid_len);
		src[7] = (uint8_t)(crcval&0x00FF);         // crc值低位
		src[8] = (uint8_t)((crcval>>8)&0x00FF);    // crc值高位
		send_sum_len = checkvalid_len + 2;

		/* 得到用来校验接收数据(正确 ) */
		checkvalid_len = 7;
		checksum_len = checkvalid_len + 2;
		IOT_ESP_strncpy(rcvcheckbuff, src, checksum_len);
		rcvcheckbuff[3] = 0x00;
		rcvcheckbuff[4] = 0x02;
		rcvcheckbuff[5] = 0x00;
		rcvcheckbuff[6] = 0x00;
		crcval = Modbus_Caluation_CRC16(rcvcheckbuff,checkvalid_len);
		rcvcheckbuff[7] = (uint8_t)(crcval&0x00FF);      // crc值低位
		rcvcheckbuff[8] = (uint8_t)((crcval>>8)&0x00FF); // crc值高位
	}
	else if((reg == GucCode_17_register[Code_17_register_02]) || \
			(reg == GucCode_17_register[Code_17_register_03]))
	{
		checkvalid_len = 9;

		if(reg == GucCode_17_register[Code_17_register_02])
		{
			src[5] = (uint8_t)((data>>24)&0x000000FF);  // 数据高位
			src[6] = (uint8_t)((data>>16)&0x000000FF);  // 数据高位
			src[7] = (uint8_t)((data>>8)&0x000000FF);   // 数据高位
			src[8] = (uint8_t)(data&0x000000FF);        // 数据低位
		}
		else if(reg == GucCode_17_register[Code_17_register_03])
		{
			src[5] = (uint8_t)((crc>>24)&0x000000FF);  // 数据高位
			src[6] = (uint8_t)((crc>>16)&0x000000FF);  // 数据高位
			src[7] = (uint8_t)((crc>>8)&0x000000FF);   // 数据高位
			src[8] = (uint8_t)(crc&0x000000FF);        // 数据低位
		}

		/* 数据做crc校验 */
		crcval = Modbus_Caluation_CRC16(src,checkvalid_len);
		src[9] = (uint8_t)(crcval&0x00FF);        // crc值低位
		src[10] = (uint8_t)((crcval>>8)&0x00FF);    // crc值高位
		send_sum_len = checkvalid_len + 2;

		/* 得到用来校验接收数据(正确 ) */
		checkvalid_len = 7;
		checksum_len = checkvalid_len + 2;
		IOT_ESP_strncpy(rcvcheckbuff, src, checkvalid_len);
		rcvcheckbuff[3] = 0x00;
		rcvcheckbuff[4] = 0x02;
		rcvcheckbuff[5] = 0x00;
		rcvcheckbuff[6] = 0x00;
		crcval = Modbus_Caluation_CRC16(rcvcheckbuff,checkvalid_len);
		rcvcheckbuff[7] = (uint8_t)(crcval&0x00FF);      // crc值低位
		rcvcheckbuff[8] = (uint8_t)((crcval>>8)&0x00FF); // crc值高位
	}
	else if(reg == GucCode_17_register[Code_17_register_05])
	{
		checkvalid_len = (7 + (len-2));

		src[5] = (uint8_t)((num>>8)&0x00FF);   // 数据高位
		src[6] = (uint8_t)(num&0x00FF);        // 数据低位

		IOT_ESP_strncpy(&src[7], buf, (len-2)); // 升级数据包大小

		/* 数据做crc校验 */
		crcval = Modbus_Caluation_CRC16(src,checkvalid_len);
		src[checkvalid_len] = (uint8_t)(crcval&0x00FF);         // crc值低位
		src[checkvalid_len+1] = (uint8_t)((crcval>>8)&0x00FF);  // crc值高位
		send_sum_len = checkvalid_len + 2;

		/* 得到用来校验接收数据(正确 ) */
		checkvalid_len = 7;
		checksum_len = checkvalid_len + 2;
		IOT_ESP_strncpy(rcvcheckbuff, src, checkvalid_len);
		rcvcheckbuff[3] = 0x00;
		rcvcheckbuff[4] = 0x02;
		rcvcheckbuff[5] = (uint8_t) HIGH(num);
		rcvcheckbuff[6] = (uint8_t) LOW(num);
		crcval = Modbus_Caluation_CRC16(rcvcheckbuff,checkvalid_len);
		rcvcheckbuff[7] = (uint8_t)(crcval&0x00FF);      // crc值低位
		rcvcheckbuff[8] = (uint8_t)((crcval>>8)&0x00FF); // crc值高位


	}

	/* 得到用来校验接收数据(错误) */
	crcval = Modbus_Caluation_CRC16(err_rcvcheckbuff,err_checkvalid_len);
	err_rcvcheckbuff[3] = (uint8_t)(crcval&0x00FF);      // crc值低位
	err_rcvcheckbuff[4] = (uint8_t)((crcval>>8)&0x00FF); // crc值高位

	/* 发送数据前清空接收缓冲区数据 */
	memset(IOTESPUsart0RcvBuf,0,UsartDataMaxLen);
	IOTESPUsart0RcvLen = 0;

	/* 开始发送第一包命令码数据 */
	InverterFotaGPCode_Send(send_sum_len,src);

	/* 命令码数据收发 */
	while(1)
	{
		/* 命令码返回数据应答判断 */
		if(IOT_ESP_strstr(IOTESPUsart0RcvBuf, rcvcheckbuff, IOTESPUsart0RcvLen,checksum_len-2))  // 正常应答，(所有字节，除crc)
		{
			crcval = Modbus_Caluation_CRC16(IOTESPUsart0RcvBuf,checksum_len-2);// 接收数据进行crc
			if((IOTESPUsart0RcvBuf[checksum_len-2]) == (uint8_t)(crcval&0x00FF) && (IOTESPUsart0RcvBuf[checksum_len-1] == (uint8_t)((crcval>>8)&0x00FF)))
			{
				IOT_Printf("FotaCode_17_%d_Receive !\r\n",reg);
				IOT_ESP_IFota_Dead_Delayms(0);
				return 1;
			}
		}
		else if(IOT_ESP_strstr(IOTESPUsart0RcvBuf, err_rcvcheckbuff, IOTESPUsart0RcvLen,err_checksum_len))// 错误应答
		{

		}

		/* 发送超时判断 */
		if(!IOT_ESP_IFota_Dead_Delayms(p_param->sendinterval))// 0x06 死亡超时设置为3s
		{
			sendtimes++;

			if(sendtimes >= p_param->sendtimes)// 命令码超过发送最大允许次数，结束发送
			{
				p_param->status = code_wait;
				return 0;
			}

			/* 发送数据前清空接收缓冲区数据 */
			memset(IOTESPUsart0RcvBuf,0,UsartDataMaxLen);
			IOTESPUsart0RcvLen = 0;
			/* 开始发送数据 */
			InverterFotaGPCode_Send(send_sum_len,src);  // 命令码无应答后，再次请求命令码
		}
	}

	return 1;
}

/******************************************************************************
 * FunctionName : FotaCode_17_Send
 * Description  : 逆变器升级发送0x17命令码
 * Parameters   : GPFotaParam *p_param
 *                uint8_t mode
 * Returns      : none
 * Notice       : none
*******************************************************************************/
#define onepacketsize 256
static  bool FotaCode_17_Send(GPFotaParam *p_param, uint8_t *src, uint8_t addr)
{
	uint8_t status = 1;
	uint32_t len = 0;    // 功能码发送有效字符串个数
	uint32_t data = 0;   // 升级数据大小值
	uint32_t crc = 0;    // 升级文件crc值
	uint16_t num = 0;    // 升级发送文件分包顺序编号
	uint8_t *buf = (uint8_t*) malloc(onepacketsize);
	uint32_t fota_file_send_size = 0;

	uint16_t i = 0;
	uint16_t sum_sec = 0;
	uint16_t last_sec = 0;

	memset(buf, 0 ,onepacketsize);

	if(status)
	{
		len = 4;
		/* 升级文件大小 */
//		data = _GuiFotaFileSize/1024;   // 注：单位 ：kb
		data = _GuiFotaFileSize;        // 注：单位 ：byte LI 2017.10.21
		status &= FotaCode_17_xx_Send(p_param, src, addr, (uint16_t)GucCode_17_register[Code_17_register_02],len,data,crc,num,buf);
	}
	if(status)
	{/* 升级文件CRC校验值 */
		len = 4;
		crc = _GuiFotaFileCRC;
		status &= FotaCode_17_xx_Send(p_param, src, addr, (uint16_t)GucCode_17_register[Code_17_register_03],len,data,crc,num,buf);
	}
	if(status)
	{/* 擦除指令 */
		len = 2;
		data = 0;
		status &= FotaCode_17_xx_Send(p_param, src, addr, (uint16_t)GucCode_17_register[Code_17_register_04],len,data,crc,num,buf);
	}
	if(status)
	{/* 发送升级文件数据  */

		sum_sec = _GuiFotaFileSize/onepacketsize;
		last_sec = _GuiFotaFileSize%onepacketsize;

		while(status && fota_file_send_size < _GuiFotaFileSize)
		{
			if(i++ < sum_sec)
			{
				InverterFotaFileData_Get((_Gui_IAP_CACHE_ADDR  + fota_file_send_size), buf, onepacketsize);// 读取升级文件数据
				len = 2 + onepacketsize;
				status &= FotaCode_17_xx_Send(p_param, src, addr, (uint16_t)GucCode_17_register[Code_17_register_05],len,data,crc,num,buf);
				num++;   // 升级包数编号

				fota_file_send_size += onepacketsize;
			}

			if((last_sec) && (i == sum_sec))
			{
				InverterFotaFileData_Get((_Gui_IAP_CACHE_ADDR  + fota_file_send_size), buf, onepacketsize);// 读取升级文件数据
				len = 2 + last_sec;
				status &= FotaCode_17_xx_Send(p_param, src, addr, (uint16_t)GucCode_17_register[Code_17_register_05],len,data,crc,num,buf);
				fota_file_send_size += last_sec;
			}
//			IOT_ESP_Char_printf("FotaCode_17_05", buf, onepacketsize);// 打印升级上传数据
			/* 升级进度处理  注意：传输完默认是上传90% 进度*/
			if(fota_file_send_size >= _GuiFotaFileSize)
			{
				InverterFotaMsg_Send(90);
				IOT_Printf("***FotaCode_17_%d - _GuiFotaFileSize = %d％***!\r\n",5,90);
			}
			else if((fota_file_send_size%(5*1024)) == 0)
			{
				InverterFotaMsg_Send((((fota_file_send_size*100)/_GuiFotaFileSize)*90)/100);
				IOT_Printf("***FotaCode_17_%d - _GuiFotaFileSize = %d％***!\r\n",5,(((fota_file_send_size*100)/_GuiFotaFileSize)*90)/100);
				IOT_Task_Delay_ms(200);  //延时200ms 切换到其他任务 上传进度
			}
			IOT_Printf("***FotaCode_17_%d - _GuiFotaFileSize = %d - fota_file_send_size = %d***!\r\n",5,_GuiFotaFileSize,fota_file_send_size);


			IOT_Task_Delay_ms(10);  //延时10ms 切换到其他任务
		}
	}
	if(status)
	{/* 升级文件数据 发送结束 */
		len = 2;
		data = 0;
		status &= FotaCode_17_xx_Send(p_param, src, addr, (uint16_t)GucCode_17_register[Code_17_register_06],len,data,crc,num,buf);
	}
	free(buf);
	buf = NULL;
	return status;
}

/******************************************************************************
 * FunctionName : FotaCode_03_xx_Send
 * Description  : 逆变器升级发送0x03_0xxx(0x1F)命令码
 * Parameters   : GPFotaParam *p_param
 * 				  uint16_t reg
 *                uint8_t *src
 *                uint16_t val
 *                uint8_t *fota_progress
 * Returns      : none
 * Notice       : 发送数据格式：addr 0x03 0x00 0x1F 0x00 0x01 CRC_H CRC_L
 *                正确应答数据格式：addr 0x03 0x00 0x00 0x64 CRC_H CRC_L
 *                错误应答数据格式：addr 0x83 0x01 CRC_H CRC_L
*******************************************************************************/
static bool FotaCode_03_xx_Send(GPFotaParam *p_param, uint8_t *src, uint8_t Addr, uint16_t reg, uint16_t val, uint8_t *fota_progress)
{
	uint16_t checkvalid_len = 0;                // 有效总长度
	uint16_t checksum_len = 0;                  // 总长度(checkvalid_len + 2(crc_len))
	uint16_t err_checkvalid_len = 3;            // 出错校验数据有效总长度
	uint16_t err_checksum_len = 5;              // 出错校验数据总长度(err_checkvalid_len + 2(crc_len))
	uint8_t sendtimes = 0;                      // 统计命令码发送次数
	uint8_t rcvcheckbuff[16] = {Addr};          // 正确应答校验
	uint8_t err_rcvcheckbuff[5] = {Addr,0x83,0x01};  // 错误应答校验
	uint16_t crcval = 0;                             // crc校验码值
	uint16_t send_sum_len = 0;                       // 发送总长度

	src[0] = Addr;                        // 命令码
	src[1] = 0x03;                        // 命令码
	src[2] = (uint8_t)((reg>>8)&0x00FF);       // 寄存器
	src[3] = (uint8_t)(reg&0x00FF);
	src[4] = (uint8_t)((val>>8)&0x00FF);       // 写入值
	src[5] = (uint8_t)(val&0x00FF);

	checkvalid_len = 6;

	/* 数据做crc校验 */
	crcval = Modbus_Caluation_CRC16(src,checkvalid_len);
	src[6] = (uint8_t)(crcval&0x00FF);         // crc值低位
	src[7] = (uint8_t)((crcval>>8)&0x00FF);    // crc值高位
	send_sum_len = checkvalid_len + 2;

	/* 得到用来校验接收数据(正确 、错误) */
	checkvalid_len = 0;
	checksum_len = checkvalid_len + 3;
	rcvcheckbuff[0] = Addr;
	rcvcheckbuff[1] = 0x03;
	rcvcheckbuff[2] = 0x02;

	crcval = Modbus_Caluation_CRC16(err_rcvcheckbuff,err_checksum_len);
	err_rcvcheckbuff[3] = (uint8_t)(crcval&0x00FF);         // crc值低位
	err_rcvcheckbuff[4] = (uint8_t)((crcval>>8)&0x00FF);    // crc值高位

	/* 发送数据前清空接收缓冲区数据 */
	memset(IOTESPUsart0RcvBuf,0,UsartDataMaxLen);
	IOTESPUsart0RcvLen = 0;

	/* 开始发送第一包命令码数据 */
	InverterFotaGPCode_Send(send_sum_len,src);

	/* 命令码数据收发 */
	while(1)
	{
		/* 命令码返回数据应答判断 */
		if(IOT_ESP_strstr(IOTESPUsart0RcvBuf, rcvcheckbuff, IOTESPUsart0RcvLen,checksum_len))  // 正常应答(共7字节，校验前3个字节)
		{
			crcval = Modbus_Caluation_CRC16(IOTESPUsart0RcvBuf,5);
			if((IOTESPUsart0RcvBuf[5]) == (uint8_t)(crcval&0x00FF) && (IOTESPUsart0RcvBuf[6] == (uint8_t)((crcval>>8)&0x00FF)))
			{
				if(IOTESPUsart0RcvBuf[4] <= 255)
				{
					*fota_progress = IOTESPUsart0RcvBuf[4];// 得到进度值

					IOT_Printf("FotaCode_03_%x_Receive !\r\n",reg);

					while(IOT_ESP_IFota_Dead_Delayms(1000)){}// 每秒钟查一次
					IOT_ESP_IFota_Dead_Delayms(0);

					return 1;
				}
			}
		}
		else if(IOT_ESP_strstr(IOTESPUsart0RcvBuf, err_rcvcheckbuff, IOTESPUsart0RcvLen,err_checksum_len))// 错误应答
		{

		}

		/* 发送超时判断 */
		if(!IOT_ESP_IFota_Dead_Delayms(p_param->sendinterval))// 0x06 死亡超时设置为3s
		{
			sendtimes++;

			if(sendtimes >= p_param->sendtimes)// 命令码超过发送最大允许次数，结束发送
			{
				p_param->status = code_wait;
				return 0;
			}

			/* 发送数据前清空接收缓冲区数据 */
			memset(IOTESPUsart0RcvBuf,0,UsartDataMaxLen);
			IOTESPUsart0RcvLen = 0;
			/* 开始发送数据 */
			InverterFotaGPCode_Send(send_sum_len,src);  // 命令码无应答后，再次请求命令码
		}
	}

	return 1;
}

/******************************************************************************
 * FunctionName : FotaCode_03_Send
 * Description  : 逆变器升级发送0x03命令码
 * Parameters   : GPFotaParam *p_param
 *                uint8_t *src
 * Returns      : none
 * Notice       : none
*******************************************************************************/
static bool FotaCode_03_Send(GPFotaParam *p_param, uint8_t *src, uint8_t Addr)
{
	uint8_t status = 1;
	uint16_t reg_1F_val = 0x01;
    uint8_t fota_progress = 0;
    uint8_t fota_progress_buf[1] = {0};// 升级进度保存buf  LI20190528

	if(status)
	{/* 发送请求修改串口波特率并不保存 */

		while(status)
		{
			status &= FotaCode_03_xx_Send(p_param, src, Addr, (uint16_t)GucCode_03_register[Code_03_register_1F], reg_1F_val, &fota_progress);

			if(fota_progress == 100)
			{
				InverterFotaMsg_Send(100);

			//	IOT_ESP_CollectorNewParam_Write((uint8_t*)"O", 1, 1); // 写采集器新参数 -- 逆变器升级成功 LI 20190426

				IOT_Printf("***FotaCode_03_%d - fota_progress = %d％***!\r\n",3,100);
				break;
			}
			else if(fota_progress > 100)
			{
				status = 0;

				_GuInverterFotaERRCode = fota_progress;  // 记录逆变器升级错误码

				fota_progress_buf[0] = fota_progress;
			//	IOT_ESP_CollectorNewParam_Write(fota_progress_buf, 1, 1); // 写采集器新参数 -- 逆变器升级成功 LI 20190528
				IOT_Printf("***FotaCode_03_%d - fota_progress = %d％  fail !!!***!\r\n",3,fota_progress);
				break;
			}
			else if((fota_progress%(4)) == 0)
			{
				InverterFotaMsg_Send(90 + (fota_progress/10));// 90 是传输到逆变器的进度
				IOT_Printf("***FotaCode_03_%d - fota_progress = %d％***!\r\n",3,(90 + (fota_progress/10)));
			}
		}
	}

	return status;
}


/***************************************************
 *
检测逆变器是否处于升级状态
**************************************************/
uint8_t IOT_GetInvUpdtStat(void)
{

    if((SysTAB_Parameter[Parameter_len[REG_INUPGRADE]] != '0') && (INVFOAT.InvUpdateFlag == 1))// 判断是否要升级
	{
	    	return 0;
	}
	return 1;
}


/******************************************************************************
 * FunctionName : IOT_ESP_NEW_UpdataPV
 * Description  : 逆变器升级
 * Parameters   : none
 * Returns      : none
 * Notice       : none
*******************************************************************************/
#define new_IFotaSendLen (256 + 32)
uint8_t IOT_ESP_NEW_UpdataPV(void)
{
	unsigned char status = 0;
	unsigned char fota_status = 0;

//  if(IOT_GetInvUpdtStat() ==1)  //没有在升级
//  {
//     return 0;
//  }

	IOT_ESP_NEW_InvUpdataInit();   //逆变器升级初始化

	uint8_t *src = (uint8_t*)malloc(new_IFotaSendLen);
	memset(src, 0, new_IFotaSendLen);

	if(g_FotaParams.uiFotaProgress >= 100)
	  IOT_UpdateProgress_record(INVERTRE_PROGRESS , 0);		//升级进度清0



	if(!_GuiFotaFileSize)
	{
		free(src);
		src = NULL;
		INVFOAT.InvUpdateFlag = 0;
		InverterFotaMsg_Send(0x86);			// 固件非法
		return 0;
	}
																	//地址由0x11 改回 _GucInverterAddr 不兼容便携式电源
	status = FotaCode_06_Send(&GPFotaParams[GPFotaCode_06], src , 1, _GucInverterAddr);  // 升级开始 设置波特率 和 发送下发升级文件类型（bin 或 hex）

	if(status)
	{
		status = FotaCode_17_Send(&GPFotaParams[GPFotaCode_17], src , _GucInverterAddr); // 发送升级相关数据和文件
	}

	if(status)
	{
		status = FotaCode_03_Send(&GPFotaParams[GPFotaCode_03] , src , _GucInverterAddr);  // 查询升级进度
	}

	fota_status = _GuInverterFotaStatus; // 升级状态-应答超时

	_GuInverterFotaStatus = code_wait;
	_GuiInverterFotaTime = GetSystemTick_ms();
//	FotaCode_06_Send(&GPFotaParams[GPFotaCode_06], src, 0, _GucInverterAddr);  // 升级结束 设置波特率
	_GuInverterFotaStatus = code_end;

	if(!status || fota_status == code_end)// 升级失败 ： 命令码应答失败 或 超时
	{
		if(_GuInverterFotaERRCode)     // 判断是否是逆变器升级反馈错误码
		{
			InverterFotaMsg_Send(_GuInverterFotaERRCode);   // 固件烧录失败 LI 20190302
		}
		else
		{
			InverterFotaMsg_Send(0x89);   // 固件烧录失败
		}
	}

	INVFOAT.InvUpdateFlag  = 0;
	IOT_SystemParameterSet(44,"0", 1);  //保存数据
	IOT_SystemParameterSet(36,"0", 1);  //保存数据

	_GuInverterFotaERRCode = 0; // LI 20190302

	free(src);
	src = NULL;

	return status;
}





/************************** END OF FILE *****************************************/



