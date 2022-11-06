/*
 * iot_system.c
 *
 *  Created on: 2021年12月1日
 *      Author: Administrator
 */
#include "sdkconfig.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"

#include "iot_system.h"
#include "iot_spi_flash.h"
#include "iot_universal.h"
#include "iot_inverter.h"
#include "iot_rtc.h"
#include "iot_timer.h"
#include "iot_mqtt.h"
#include "iot_station.h"
#include "iot_gatts_table.h"

#include "iot_protocol.h"
#include "iot_fota.h"
#include "iot_net.h"
#include "iot_ammeter.h"
#include "iot_record.h"
#include "esp_wifi.h"

#include "iot_SmartConfig.h"

static const char *SYS = "SYS_Init";


const char *Collector_Set_Tab[Parameter_len_MAX] =
{
    "0",                    //0     采集器界面语言  0:英文，1:中文
    "1",                    //1     数据采集器的协议类型,0：MODBUS_TCP,1：Internal
    "7.0",                  //2  *  数服协议”（即本协议）的版本号
    "10",                   //3     ShinePano背光延时
    "5",                   //4  *  数据采集器向网络服务器传送数据的间隔时间，单位为分钟
    "1",                    //5     数据采集器对逆变器的起始搜索地址
    "32",                   //6     数据采集器对逆变器的结束搜索地址
    "123456",               //7     预留参数,内部已使用，勿做其他用途
    "XXXXXXXXXX",           //8     数据采集器序列号 BLESKA210A BLEKEY0330 WIFIX2SN03 BLEFULL321 WIFIX2SN0E  XXXXXXXXXX
    "2.0",                  //9     逆变器轮询时间延时
    "0",                    //10    对数据采集器的固件进行更新的标识,0：正常工作,1：更新固件
    "NULL",			        //11    wifi模块无法使用FTP,用于数据采集器进行自身固件及光伏设备固件更新的FTP设置（含用户名、密码、IP、端口，各参数间用井号分隔），数据格式为：UserName#Password#IP#Port#
    "202.96.134.133",       //12    数据采集器的DNS配置
	DATALOG_TYPE,       	//13    数据采集器类型,33:内置采集器 ；34：wifi-x2
    "192.168.5.1",          //14    数据采集器的本端IP地址,为空Server不显示
    "80",                	//15    数据采集器的本端端口
    "84:f7:03:3a:3a:xx",    //16 *  数据采集器连入互联网的网卡MAC地址  WiFi mac
	"server-cn.growatt.com",      //17 *  数据采集器的远端（网络服务器）IP地址//"183.62.216.35" ,//"20.6.1.65" , //  "ces.growatt.com", ces-test.growatt.com
	"7006",  				//18 *  数据采集器的远端（网络服务器）端口//"8080",  //  "7006",
    "server-cn.growatt.com", 		//19    数据采集器的远端（网络服务器）域名
    "GTSW0000",             //20    数据采集器的安规类型
    DATALOG_SVERSION,	    //21    数据采集器的固件版本
    "V1.0",                 //22    数据采集器的硬件版本
    "0",                    //23    Zigbee网络ID
    "0",                    //24    Zigbee网络频道号
    "255.255.255.0",        //25    数据采集器的子网掩码
    "192.168.5.1",          //26    数据采集器的默认网关
    "1",                    //27    无线模块类型设置,0：zigbee,1：wifi,3:3G,4:2G
    "NULL",				    //28    不使用,光伏外围设备（外设）的使能设置。这几个外设在出厂状态是不开启的，默认值是0,0,0,# 0,0,0,# 0,0,0,# 0,0,0,#
    "0",                    //29    预留参数
    "GMT+8",                //30    时区
    "2022-06-07 11:20:23",  //31 *  数据采集器的系统时间
    "0",                    //32 *  数据采集器重启标识,0：正常工作,1：重启数据采集器
    "0",                    //33    清除ShinePano所有数据记录的标识,0：正常工作,1：清除ShinePano所有数据记录
    "0",                    //34    校正ShinePano触摸屏标识,0：正常工作
    "0",                    //35    恢复数据采集器的出厂设置标识,0：正常工作
    "0",                    //36    对光伏设备停止数据轮询的标识,0：进行光伏设备的轮询
    "0,44#45,89#",          //37    采集器对逆变器03号功能码的轮询区段定义.默认两个区段：0,44#45,89#
    "0,44#45,89#",          //38    04号功能码的轮询区段定义,同上
    "0",                    //39    预留参数
    "0",                    //40    限制数据采集器对逆变器监控的数量。默认为0，有效值为[1,32]，其它值非法
    "30",					//41    通讯保护时间。当数据采集器从逆变器侧获取不到数据，持续一段时间后，执行自重启操作。该参数用来设置和读取这段时间值。该功能用以缓解逆变器串口被打死的问题。
    "0",                    //42    数据传输模式。0：协议传输模式,1：透明传输模式
    "NULL",					//43    预留,Webbox使用
    "0",           			//44	逆变器升级选择
    "1",                    //45	入网类型,1:WIFI.3:3G,0:自动4:,2G
    "0",                    //46
    "0",                    //47
    "0",                    //48
    "0",                    //49
    "0",                    //50
    "0.2",                  //51
    "0",                    //52
    "84:f7:03:3a:3a:xx",    //53 *  蓝牙 mac
    "growatt_iot_device_common_key_01",	 //54 * 蓝牙授权密钥growatt_iot_device_common_key_01
	"0",					//55 *  wifi连接状态
    "12345678",				//56 *  ShineWIFI连接的无线路由SSID  zktest1
    "0",			        //57 *  要连接的WIFI密码
    "2",					//58	WIFI配置信息  默认加密方式 WIFI_AUTH_WPA2_PSK  zktestzktest   生产测试071207
    "0",					//59	2G,3G信号值
    "0",					//60	采集器和Server的通讯状态  生产测试 3、4、16  APP 读取是否连接服务器
	"IDFSDK:v4.4.2",		//61	使用的三方模块的软件版本    生产测试 IDFSDK:v4.4.1
	"0",					//62	基带IMEI号码
	"0",					//63	SIM ICCID号码
	"0",					//64	SIM卡所属运营商
	"0",					//65	wifi文件升级类型
	"0",					//66	防逆流开关
	"0",					//67	防逆流功率设置
	"0",					//68	RF通讯信道增量,默认为0
	"0",					//69	配对是否成功
	"0",					//70	WIFI模式切换(AP/STA)
	"0",					//71	DHCP使能
	"0",					//72	Meboost工作模式
	"0",					//73	Meboost强制烧水时间
	"0",					//74	Meboost手动模式工作时长
	"0",					//75	获取周围wifi名字   扫描wifi
	"0",					//76	最近200次信号的平均值
	"0",					//77	是否使能连回服务器功能
	"0",					//78	预留服务器地址
	"0",					//79	预留服务器端口
	"0",					//80 *  HTTP下载业务URL
	"0",					//81	系统运行时间（单位：小时）
	"0",					//82	系统上电次数
	"0",					//83	系统软自重启次数
	"0",					//87	逆变器升级成功次数
	"0",					//84	成功连接服务器次数
	"0",					//85	采集器升级成功次数
	"0",					//86	采集器升级失败次数
	"0",					//88	逆变器升级失败次数
	"0",					//89
	"0",					//90
	"0",					//91
	"0",					//92
	"1",					//93
	"0",					//94
	"0",					//95
	"0",					//96
	"97",					//97
	"98",					//98
	"99",					//99
	"100",					//100
	"101",					//101

};


/* LED状态定义 */
enum _IOTESPLEDSTATUS{
	ESP_RGBLED_RESET = 0,  // RGB-led 全灭标志位
	ESP_RGBLED_SET,        // RGB-led 全亮标志位
	ESP_RLED_SET,          // R-led 亮标志位    逆变器通讯正常
	ESP_GLED_SET,          // G-led 亮标志位    wifi连接正常
	ESP_BLED_SET,          // B-led 亮标志位    tcp连接正常
	ESP_GLED_SETWIFI_SET,  // G-led 亮标志位    wifi连接正常
	ESP_RLED_FAULT_SET     // R-led 亮标志位    采集器故障
};
//char System_Config[REG_NUM_MAX][REG_LEN];		// 采集器寄存器参数
const uint8_t Flashparameter[10] = {"PasswordON"};
const char BLEkeytoken[35]={"growatt_iot_device_common_key_01"}; //Grwatt0  growatt_iot_device_common_key_01


uint16_t Parameter_length=0;			//总长度
uint16_t Parameter_len[Parameter_len_MAX]; 	 		//二维长度映射表  数据地址
uint8_t Collector_Config_LenList[Parameter_len_MAX];//记录每个参数数据的长度
char SysTAB_Parameter[SysTAB_Parameter_max];  		//数据存储 一维数组 采集器寄存器参数
SYStem_state System_state={0};						//系统参数结构体


/*******************************************
 * 每次上电读取参数列表参数赋值 IOT_ParameterTABInit
 * 每个参数寄存器参数长度     Collector_Config_LenList
 * 总寄存器参数长度  	       Parameter_length
 * 每个寄存器地址（长度）     Parameter_len
 * 每个参数数据           SysTAB_Parameter
 * *****************************************/
uint8_t IOT_ParameterTABInit(void)
{
	uint8_t i=0;

	uint8_t *pbuf = NULL;
 	uint8_t  uCRC16[2];
	uint16_t CRC16;

	pbuf = W25QXX_BUFFER;
	IOT_Mutex_SPI_FLASH_BufferRead(pbuf,RW_SIGN_ADDR,10);

	ESP_LOGI(SYS,"\r\n*****IOT_ParameterTABInit W25QXX_Read***RW_SIGN_ADDR 10**\r\n");
	for(i=0 ;i<10;i++)
	{
		printf("%02x  ",pbuf[i]);
		//IOT_Printf("%02x",pbuf[i]);
	}
	//ESP_LOGI(SYS,"\r\n**date=***50 61 73 73 77 6f 72 64 4f 4e *****\r\n");

	if(memcmp(pbuf,Flashparameter,10) == 0 )						            // 检测是否是第一次上电
	{
		ESP_LOGI(SYS,"\r\n**********IOT_ParameterTABInit****************\r\n");
		memset(pbuf,0,4096);												// 清缓存
		IOT_Mutex_SPI_FLASH_BufferRead(pbuf,REG_LEN_LIST_ADDR,W25X_FLASH_ErasePageSize);		// 读出参数扇区 读取4K


		CRC16 = GetCRC16(pbuf,CONFIG_CRC16_NUM);
		uCRC16[0] = CRC16>>8;
		uCRC16[1] = CRC16&0x00ff;
	    ESP_LOGI(SYS,  "\r\nREG_LEN_LIST_ADDR GetCRC16= 0x%02x  ReadaCRC= 0x%02x%02x\r\n",CRC16,pbuf[CONFIG_CRC16_BUFADDR],pbuf[CONFIG_CRC16_BUFADDR+1]);
		/* 校验CRC16 */
		if((pbuf[CONFIG_CRC16_BUFADDR] == uCRC16[0]) && (pbuf[CONFIG_CRC16_BUFADDR+1] == uCRC16[1]))
		{
			printf("CRC validates correctly!\r\n");
			memcpy(Collector_Config_LenList, pbuf, Parameter_len_MAX);// 读出长度表
			Parameter_length =0;
			for(  i=0; i < Parameter_len_MAX; i++)
			{
				Parameter_len[i] = Parameter_length ; 			// 数据地址
				Parameter_length = Parameter_length + Collector_Config_LenList[i];   //参数列表实际总长度
			}
			memcpy(SysTAB_Parameter, &pbuf[PARAMETER_LENLIST_LEN],  Parameter_length);	// 读出长度表

			//固化参数  (i==2)|| (i==13)   || (i==21)
			if(Collector_Config_LenList[2]== 1 )
			{
			    memcpy(&SysTAB_Parameter[Parameter_len[2]],Collector_Set_Tab[2],Collector_Config_LenList[2]);	//复制数据
			}
			if(Collector_Config_LenList[13]== 2 )
			{
				memcpy(&SysTAB_Parameter[Parameter_len[13]],Collector_Set_Tab[13],Collector_Config_LenList[13]);//复制数据
			}
			if(Collector_Config_LenList[21]== 7 )
			{
				memcpy(&SysTAB_Parameter[Parameter_len[21]],Collector_Set_Tab[21],Collector_Config_LenList[21]);//复制数据
			}

			if( Collector_Config_LenList[0] ==0 && Collector_Config_LenList[1]==0 && Collector_Config_LenList[2]==0)
			{
				goto BACKUP_ERR;
			}
			return 1;
		}
		else
		{
			printf("CRC validates incorrectly!\r\n");
			BACKUP_ERR:
			memset(pbuf,0,4096);												// 清缓存
			IOT_Mutex_SPI_FLASH_BufferRead(pbuf,BACKUP_CONFIG_ADDR,W25X_FLASH_ErasePageSize);		// 读出参数扇区 读取4K

			CRC16 = GetCRC16(pbuf,CONFIG_CRC16_NUM);
			uCRC16[0] = CRC16>>8;
			uCRC16[1] = CRC16&0x00ff;
			ESP_LOGI(SYS,  "\r\n BACKUP_CONFIG_ADDR GetCRC16= 0x%02x  ReadaCRC= 0x%02x%02x\r\n",CRC16,pbuf[CONFIG_CRC16_BUFADDR],pbuf[CONFIG_CRC16_BUFADDR+1]);
			/* 校验CRC16 */
			if((pbuf[CONFIG_CRC16_BUFADDR] == uCRC16[0]) && (pbuf[CONFIG_CRC16_BUFADDR+1] == uCRC16[1]))
			{
				/* 从备份区复制参数到主参数区 */
				IOT_Mutex_SPI_FLASH_WriteBuff(pbuf,REG_LEN_LIST_ADDR,W25X_FLASH_ErasePageSize);
				memcpy(Collector_Config_LenList, pbuf, Parameter_len_MAX);// 读出长度表
				Parameter_length =0;
				for(  i=0; i < Parameter_len_MAX; i++)
				{
							Parameter_len[i] = Parameter_length ; 			// 数据地址
							Parameter_length = Parameter_length + Collector_Config_LenList[i];   //参数列表实际总长度
				}
				memcpy(SysTAB_Parameter, &pbuf[PARAMETER_LENLIST_LEN],  Parameter_length);	// 读出长度表

						//固化参数  (i==2)|| (i==13)   || (i==21)
				if(Collector_Config_LenList[2]== 1 )
				{
					memcpy(&SysTAB_Parameter[Parameter_len[2]],Collector_Set_Tab[2],Collector_Config_LenList[2]);	//复制数据
				}
				if(Collector_Config_LenList[13]== 2 )
				{
					memcpy(&SysTAB_Parameter[Parameter_len[13]],Collector_Set_Tab[13],Collector_Config_LenList[13]);//复制数据
				}
				if(Collector_Config_LenList[21]== 7 )
				{
					memcpy(&SysTAB_Parameter[Parameter_len[21]],Collector_Set_Tab[21],Collector_Config_LenList[21]);//复制数据
				}
			}
			else
			{
				goto FLASH_ERR;
			}
		}
	}
	else
	{

		FLASH_ERR:
		// 第一次上电 恢复出厂设置
		ESP_LOGI(SYS,"\r\n Param_DeInit ...  \r\n");
		W25QXX_Erase_Block(0x00);
		IOT_Printf("\r\n 1 \r\n");
		Parameter_length=0;
		for(i=0; i<Parameter_len_MAX; i++)
		{
			Collector_Config_LenList[i] = strlen(Collector_Set_Tab[i]);					 //获取每个寄存器数值的长度
			Parameter_len[i]= Parameter_length ; 			// 数据地址
			Parameter_length  =  Parameter_length + Collector_Config_LenList[i];   		 //参数列表实际总长度
//			ESP_LOGI(SYS,"\r\n addr Parameter_len[%d]=%d  sum addr  =Parameter_length=%d ...  \r\n",i,Parameter_len[i],Parameter_length);
			memcpy(&SysTAB_Parameter[Parameter_len[i]],Collector_Set_Tab[i],Collector_Config_LenList[i]);	//复制数据
		}
		IOT_Printf("\r\n 2 \r\n");
	//  Parameter_length = SumLen+ Collector_Config_LenList[i];             			 //参数列表实际总长度
		memset(pbuf,0,W25X_FLASH_ErasePageSize);									     //清缓存
		memcpy(&pbuf[CONFIG_LEN_BUFADDR],&Collector_Config_LenList,Parameter_len_MAX);	 //更新寄存器的长度链表
		memcpy(&pbuf[CONFIG_BUFADDR],&SysTAB_Parameter, Parameter_length);	 			 //更新写入寄存器数据
		/* CRC16校验 */
		IOT_Printf("\r\n 3 \r\n");
		CRC16 = GetCRC16(pbuf,CONFIG_CRC16_NUM);
		uCRC16[0] = CRC16>>8;

		uCRC16[1] = CRC16&0x00ff;
		pbuf[CONFIG_CRC16_BUFADDR] = uCRC16[0];
		pbuf[CONFIG_CRC16_BUFADDR+1] = uCRC16[1];
		/* 写主参数区 */
		IOT_Printf("\r\n 4 \r\n");
		IOT_Mutex_SPI_FLASH_WriteBuff(pbuf,REG_LEN_LIST_ADDR,W25X_FLASH_ErasePageSize);
		/* 写入备份区 */
		IOT_Mutex_SPI_FLASH_WriteBuff(pbuf,BACKUP_CONFIG_ADDR,W25X_FLASH_ErasePageSize);
		/* 写入标示 */
		IOT_Mutex_SPI_FLASH_BufferRead(pbuf, RW_SIGN_ADDR, 100);
		memcpy(pbuf, Flashparameter, 10);
		IOT_Printf("\r\n 5 \r\n");
		IOT_Mutex_SPI_FLASH_WriteBuff(pbuf, RW_SIGN_ADDR, 100);		//写入标识
		ESP_LOGI(SYS,"\r\n W25QXX_Write uCRC16=0x%02x%02x\r\n",uCRC16[0],uCRC16[1]);
		IOT_Mutex_SPI_FLASH_BufferRead(pbuf,CONFIG_CRC16_ADDR,2);
		ESP_LOGI(SYS,"\r\n W25QXX_Read  uCRC16=0x%02x%02x \r\n",pbuf[0],pbuf[1]);
	}


	memset(pbuf,0,4096);									//清缓存
	return 0;
}

uint8_t IOT_BLEkeytokencmp(void)
{
	char SystemDATA_value[50]={0};
	bzero(SystemDATA_value, sizeof(SystemDATA_value));
	memcpy(SystemDATA_value,&SysTAB_Parameter[Parameter_len[REG_BLETOKEN]],Collector_Config_LenList[REG_BLETOKEN]);
	if(strncmp(SystemDATA_value,BLEkeytoken,32) ==0 ) // 输入密钥比较
	{
		ESP_LOGI(SYS,"*g**BLEkeytoken==0**");
		System_state.BLEBindingGg=0;    // ble 初始化绑定设备
	//	System_state.WIFI_BLEdisplay |=Displayflag_SYSreset;  //写便携式显示状态
	}
	else
	{
		ESP_LOGI(SYS,"*G**BLEkeytoken==1**");
		System_state.BLEBindingGg=1;				// ble 绑定设备
	//	System_state.WIFI_BLEdisplay &=~Displayflag_SYSreset;  //写便携式显示状态
	}
	return 0;
}

uint8_t IOT_SystemDATA_init(void)
{
	System_state.Wifi_state = Offline;   		 // wifi 未连接
	System_state.wifi_onoff = 0;					 // wifi 关闭
	System_state.Ble_state = Offline;   		 // ble  未连接
	System_state.ble_onoff = 0;					 // ble  关闭
	System_state.Mbedtls_state = 0;
	System_state.SSL_state = 0;
	System_state.BLEKeysflag = 0; 				 // ble 未通信交互密钥
	System_state.WIFI_BLEdisplay = 0;
	System_state.upHistorytimeflag=0;

	IOT_BLEkeytokencmp();
	IOT_UPDATETime();
	IOT_WiFiScan_uc(ESP_WRITE ,180 );
	IOT_INVgetSNtime_uc(ESP_WRITE ,180 );
	IOT_BLEofftime_uc(ESP_WRITE,180);
	IOT_TCPDelaytimeSet_uc(ESP_WRITE,System_state.Up04dateTime+1);

	IOT_Historydate_us(ESP_WRITE,180);
	IOT_Histotyamemte_datainit();

//	Get_RecordStatus();   // 移动到main。c
//	Get_RecordNum();      // 上电获取flash数据保存数


	return 0;
}

/************************************************************************
 * FunctionName : IOT_SystemParameterSet
 * Description  : 采集器参数列表 数据保存
 * Parameters   : uint8_t ParamValue  : 设置参数序号
 * 				  char * data		  :	保存的数据
 * 				  uint8_t len    	  :	保存的数据长度   长度256+ 限制 防止异常
 * Returns      :  null
 * Notice       :  null
************************************************************************/
uint8_t IOT_SystemParameterSet(uint16_t ParamValue ,char * data,uint8_t len )
{
	uint16_t CRC16;
	uint16_t Remain_length=0;
	uint8_t u8_CRC16[2];
	uint8_t *pbuf = (uint8_t*)malloc(W25X_FLASH_ErasePageSize);;
	uint8_t i=0;
//	pbuf = W25QXX_BUFFER;
	memset(pbuf,0,4096);								//	清缓存

	if(Collector_Config_LenList[ParamValue] == len )  	//	长度不变 直接赋值 指定参数地址  与数据
	{
     #ifdef Kennedy_System
	 ESP_LOGI(SYS,"*******data =%s***************Collector_Config_LenList[ParamValue] *****ParamValue=%d**************\r\n",data,ParamValue );
	 ESP_LOGI(SYS," add =%d   ,Collector_Config_LenList =%d   ,len =%d \r\n", Parameter_len[ParamValue] ,Collector_Config_LenList[ParamValue] , len);
     #endif
	 memcpy(&SysTAB_Parameter[Parameter_len[ParamValue]],data,len);
	}
	else
	{
/**
 * 先读取 存储的寄存器的后面的数据  赋值到 pbuf
 * 再赋值 存储的数据到当前的地址 长度累计 SysTAB_Parameter
 * 最再新赋值SysTAB_Parameter + pbuf
 * */
		Remain_length =0;
		for(i=(ParamValue+1); i<Parameter_len_MAX; i++)
		{
			Remain_length  =  Remain_length + Collector_Config_LenList[i];   //参数列表实际总长度
		}
		memcpy(pbuf,&SysTAB_Parameter[Parameter_len[ParamValue+1]], Remain_length ); // 剩余的赋值到存储备份区域

		memcpy(&SysTAB_Parameter[Parameter_len[ParamValue]],data,len);  // 赋值新参数
	    Collector_Config_LenList[ParamValue]=len;					    // 赋值新长度

		Parameter_length =0;  //重新计算总长度
		for(  i=0; i<Parameter_len_MAX; i++)
		{
			Parameter_len[i]=  Parameter_length; 		 // 数据地址
			Parameter_length  =  Parameter_length + Collector_Config_LenList[i];   //参数列表实际总长度
		}
		memcpy(&SysTAB_Parameter[Parameter_len[ParamValue+1]], pbuf, Remain_length );   //

#ifdef Kennedy_System
		ESP_LOGI(SYS,"*******data =%s***************Collector_Config_LenList[ParamValue] *****ParamValue=%d**************\r\n",data,ParamValue );
		ESP_LOGI(SYS," add =%d   ,Collector_Config_LenList =%d   ,len =%d \r\n", Parameter_len[ParamValue] ,Collector_Config_LenList[ParamValue] , len);
#endif
	}

	memcpy(&pbuf[CONFIG_LEN_BUFADDR],Collector_Config_LenList,Parameter_len_MAX);		// 更新寄存器的长度链表

	memcpy(&pbuf[CONFIG_BUFADDR],SysTAB_Parameter,CONFIG_LEN_MAX);					//	更新写入寄存器数据


	if( ParamValue == 60 || ParamValue ==101 )  // 不存储
	{    free(pbuf);
		return 0;
	}
//	IOT_Mutex_SPI_FLASH_WriteBuff(pbuf,BACKUP_CONFIG_ADDR,W25X_FLASH_ErasePageSize);
	/* CRC16校验 */
	CRC16 = GetCRC16(pbuf,CONFIG_CRC16_NUM);
	u8_CRC16[0] = CRC16>>8;
	u8_CRC16[1] = CRC16&0x00ff;
	pbuf[CONFIG_CRC16_BUFADDR] = u8_CRC16[0];
	pbuf[CONFIG_CRC16_BUFADDR+1] = u8_CRC16[1];
	/* 写主参数区 */
	IOT_Mutex_SPI_FLASH_WriteBuff(pbuf,REG_LEN_LIST_ADDR,W25X_FLASH_ErasePageSize);
	/* 写入备份区 */
	IOT_Mutex_SPI_FLASH_WriteBuff(pbuf,BACKUP_CONFIG_ADDR,W25X_FLASH_ErasePageSize);
    free(pbuf);
	ESP_LOGI(SYS, "IOT_SystemParameterSet  OK\r\n");
	return 0;
}


uint32_t GetSystemTick_ms(void)
{
	//   GetSystemTick_ms 溢出 0 清空有bug
	return esp_timer_get_time()/1000;   //um  转   ms  单位
}
/****************************************************************
  *     校验时间(时间必须是数字或'.')
        *src：sn号  len：sn号长度
  *		  0-成功 1-失败
****************************************************************/
//static uint8_t IOT_TimeCheck(uint8_t *src, uint8_t len)
//{
//	uint8_t i = 0;
//	for(i=0; i<len; i++)
//	{
//		if((src[i] == '.') ||((src[i] >= '0') && (src[i] <= '9')))
//		{
//		}
//		else
//		{
//			return 1;
//		}
//	}
//	return 0;
//}
/****************************************************************
  *     手动更新FLASH参数区
  *		将代码区的参数表更新到FLASH中
  *		恢复出厂设置 保留参数的服务器 和SN  ，上报时间  ，时间
*****************************************************************/

char FctRstBackupBuf[8][256]={0};	//50改成 256
void IOT_FactoryReset(void)
{
	uint8_t i;
	uint16_t CRC16;
	uint8_t uCRC16[2];
	uint8_t *pbuf = NULL;

	char BackupLen[8] = {0};


	BackupLen[0] = Collector_Config_LenList[8];
	BackupLen[1] = Collector_Config_LenList[17];
	BackupLen[2] = Collector_Config_LenList[18];
	BackupLen[3] = Collector_Config_LenList[19];
	BackupLen[4] = Collector_Config_LenList[31];
	BackupLen[5] = Collector_Config_LenList[56];
	BackupLen[6] = Collector_Config_LenList[57];

	memcpy(FctRstBackupBuf[0] , &SysTAB_Parameter[Parameter_len[8]]  , BackupLen[0]);
	memcpy(FctRstBackupBuf[1] , &SysTAB_Parameter[Parameter_len[17]] , BackupLen[1]);
	memcpy(FctRstBackupBuf[2] , &SysTAB_Parameter[Parameter_len[18]] , BackupLen[2]);
	memcpy(FctRstBackupBuf[3] , &SysTAB_Parameter[Parameter_len[19]] , BackupLen[3]);
	memcpy(FctRstBackupBuf[4] , &SysTAB_Parameter[Parameter_len[31]] , BackupLen[4]);
	memcpy(FctRstBackupBuf[5] , &SysTAB_Parameter[Parameter_len[56]] , BackupLen[5]);
	memcpy(FctRstBackupBuf[6] , &SysTAB_Parameter[Parameter_len[57]] , BackupLen[6]);


	IOT_Printf("IOT_FactoryReset !!!!!!!\r\n" );
	IOT_ParameterTABInit();    // 初始化 参数表
	pbuf = W25QXX_BUFFER;
	Parameter_length =0;
	for(i=0; i<Parameter_len_MAX; i++)
	{
		/* 一些特殊的参数，出厂时设定，不会被更改。 */
		if(i == 8)		//序列号
		{
			Collector_Config_LenList[i] = BackupLen[0];					 		 //获取每个寄存器数值的长度
			Parameter_len[i]= Parameter_length ; 										 //数据地址
			Parameter_length  =  Parameter_length + Collector_Config_LenList[i];   		 //参数列表实际总长度
			memcpy(&SysTAB_Parameter[Parameter_len[i]], FctRstBackupBuf[0], Collector_Config_LenList[i]);	//复制数据
			continue;
		}
		else if(i == 17)//IP地址
		{
			Collector_Config_LenList[i] = BackupLen[1];					 		 //获取每个寄存器数值的长度
			Parameter_len[i]= Parameter_length ; 										 //数据地址
			Parameter_length  =  Parameter_length + Collector_Config_LenList[i];   		 //参数列表实际总长度
			memcpy(&SysTAB_Parameter[Parameter_len[i]], FctRstBackupBuf[1], Collector_Config_LenList[i]);	//复制数据
			continue;
		}
		else if(i == 18)
		{
			Collector_Config_LenList[i] = BackupLen[2];					 		 //获取每个寄存器数值的长度
			Parameter_len[i]= Parameter_length ; 										 //数据地址
			Parameter_length  =  Parameter_length + Collector_Config_LenList[i];   		 //参数列表实际总长度
			memcpy(&SysTAB_Parameter[Parameter_len[i]], FctRstBackupBuf[2], Collector_Config_LenList[i]);	//复制数据
			continue;
		}
		else if(i == 19)
		{
			Collector_Config_LenList[i] = BackupLen[3];					 		 //获取每个寄存器数值的长度
			Parameter_len[i]= Parameter_length ; 										 //数据地址
			Parameter_length  =  Parameter_length + Collector_Config_LenList[i];   		 //参数列表实际总长度
			memcpy(&SysTAB_Parameter[Parameter_len[i]], FctRstBackupBuf[3], Collector_Config_LenList[i]);	//复制数据
			continue;
		}
		else if(i == 31)
		{
			Collector_Config_LenList[i] = BackupLen[4];					 		 //获取每个寄存器数值的长度
			Parameter_len[i]= Parameter_length ; 										 //数据地址
			Parameter_length  =  Parameter_length + Collector_Config_LenList[i];   		 //参数列表实际总长度
			memcpy(&SysTAB_Parameter[Parameter_len[i]], FctRstBackupBuf[4], Collector_Config_LenList[i]);	//复制数据
			continue;
		}
		else if(i == 56)
		{
			Collector_Config_LenList[i] = BackupLen[5];					 		 //获取每个寄存器数值的长度
			Parameter_len[i]= Parameter_length ; 										 //数据地址
			Parameter_length  =  Parameter_length + Collector_Config_LenList[i];   		 //参数列表实际总长度
			memcpy(&SysTAB_Parameter[Parameter_len[i]], FctRstBackupBuf[5], Collector_Config_LenList[i]);	//复制数据
			continue;
		}
		else if(i == 57)
		{
			Collector_Config_LenList[i] = BackupLen[6];					 		 //获取每个寄存器数值的长度
			Parameter_len[i]= Parameter_length ; 										 //数据地址
			Parameter_length  =  Parameter_length + Collector_Config_LenList[i];   		 //参数列表实际总长度
			memcpy(&SysTAB_Parameter[Parameter_len[i]], FctRstBackupBuf[6], Collector_Config_LenList[i]);	//复制数据
			continue;
		}

		Collector_Config_LenList[i] = strlen(Collector_Set_Tab[i]);					 //获取每个寄存器数值的长度
		Parameter_len[i]= Parameter_length ; 										 //数据地址
		Parameter_length  =  Parameter_length + Collector_Config_LenList[i];   		 //参数列表实际总长度
		memcpy(&SysTAB_Parameter[Parameter_len[i]],Collector_Set_Tab[i],Collector_Config_LenList[i]);	//复制数据
	}
	memset(pbuf,0,W25X_FLASH_ErasePageSize);									     //清缓存

	memcpy(&pbuf[CONFIG_LEN_BUFADDR],&Collector_Config_LenList,Parameter_len_MAX);		     //更新寄存器的长度链表
	memcpy(&pbuf[CONFIG_BUFADDR],&SysTAB_Parameter, Parameter_length);	 //更新写入寄存器数据
	/* CRC16校验 */
	CRC16 = GetCRC16(pbuf,CONFIG_CRC16_NUM);
	uCRC16[0] = CRC16>>8;
	uCRC16[1] = CRC16&0x00ff;
	pbuf[CONFIG_CRC16_BUFADDR] = uCRC16[0];
	pbuf[CONFIG_CRC16_BUFADDR+1] = uCRC16[1];
	/* 写主参数区 */
	IOT_Mutex_SPI_FLASH_WriteBuff(pbuf,REG_LEN_LIST_ADDR,W25X_FLASH_ErasePageSize);
	/* 写入备份区 */
	IOT_Mutex_SPI_FLASH_WriteBuff(pbuf,BACKUP_CONFIG_ADDR,W25X_FLASH_ErasePageSize);
	memset(pbuf,0,SECTOR_SIZE);			//清缓存

	// W25QXX_Read(pbuf,RW_SIGN_ADDR,10);
	// ESP_LOGI(SYS,"\r\n*****IOT_FactoryReset W25QXX_Read***RW_SIGN_ADDR 10**\r\n");
	// for(i=0 ;i<10;i++)
	// {
	//   IOT_Printf(" %02x",pbuf[i]);
	// }
	// ESP_LOGI(SYS,"\r\n**date=***50 61 73 73 77 6f 72 64 4f 4e *****\r\n");

}
void IOT_KEYlongReset(void)
{
	uint8_t i  = 0;
	uint16_t CRC16;
	uint8_t uCRC16[2];
	uint8_t *pbuf = NULL;

	wifi_config_t conf = {0};

	//char BackupBuf[5][256]={0};	//50改成 256

	char BackupLen[5] = {0};

	IOT_Printf("IOT_KEYlongReset !!!!!!!\r\n" );
	IOT_ParameterTABInit();    // 初始化 参数表
	pbuf = W25QXX_BUFFER;
	Parameter_length =0;

	BackupLen[0] = Collector_Config_LenList[8];
	BackupLen[1] = Collector_Config_LenList[17];
	BackupLen[2] = Collector_Config_LenList[18];
	BackupLen[3] = Collector_Config_LenList[19];
	BackupLen[4] = Collector_Config_LenList[31];

	memcpy(FctRstBackupBuf[0] , &SysTAB_Parameter[Parameter_len[8]]  , BackupLen[0]);
	memcpy(FctRstBackupBuf[1] , &SysTAB_Parameter[Parameter_len[17]] , BackupLen[1]);
	memcpy(FctRstBackupBuf[2] , &SysTAB_Parameter[Parameter_len[18]] , BackupLen[2]);
	memcpy(FctRstBackupBuf[3] , &SysTAB_Parameter[Parameter_len[19]] , BackupLen[3]);
	memcpy(FctRstBackupBuf[4] , &SysTAB_Parameter[Parameter_len[31]] , BackupLen[4]);

	for(i=0; i<Parameter_len_MAX; i++)
	{

		if(i == 8)		//序列号
		{
			Collector_Config_LenList[i] = BackupLen[0];					 		 //获取每个寄存器数值的长度
			Parameter_len[i]= Parameter_length ; 										 //数据地址
			Parameter_length  =  Parameter_length + Collector_Config_LenList[i];   		 //参数列表实际总长度
			memcpy(&SysTAB_Parameter[Parameter_len[i]], FctRstBackupBuf[0], Collector_Config_LenList[i]);	//复制数据
			continue;
		}
		else if(i == 17)//IP地址
		{
			Collector_Config_LenList[i] = BackupLen[1];					 		 //获取每个寄存器数值的长度
			Parameter_len[i]= Parameter_length ; 										 //数据地址
			Parameter_length  =  Parameter_length + Collector_Config_LenList[i];   		 //参数列表实际总长度
			memcpy(&SysTAB_Parameter[Parameter_len[i]], FctRstBackupBuf[1], Collector_Config_LenList[i]);	//复制数据
			continue;
		}
		else if(i == 18)
		{
			Collector_Config_LenList[i] = BackupLen[2];					 		 //获取每个寄存器数值的长度
			Parameter_len[i]= Parameter_length ; 										 //数据地址
			Parameter_length  =  Parameter_length + Collector_Config_LenList[i];   		 //参数列表实际总长度
			memcpy(&SysTAB_Parameter[Parameter_len[i]], FctRstBackupBuf[2], Collector_Config_LenList[i]);	//复制数据
			continue;
		}
		else if(i == 19)
		{
			Collector_Config_LenList[i] = BackupLen[3];					 		 //获取每个寄存器数值的长度
			Parameter_len[i]= Parameter_length ; 										 //数据地址
			Parameter_length  =  Parameter_length + Collector_Config_LenList[i];   		 //参数列表实际总长度
			memcpy(&SysTAB_Parameter[Parameter_len[i]], FctRstBackupBuf[3], Collector_Config_LenList[i]);	//复制数据
			continue;
		}
		else if(i == 31)
		{
			Collector_Config_LenList[i] = BackupLen[4];					 		 //获取每个寄存器数值的长度
			Parameter_len[i]= Parameter_length ; 										 //数据地址
			Parameter_length  =  Parameter_length + Collector_Config_LenList[i];   		 //参数列表实际总长度
			memcpy(&SysTAB_Parameter[Parameter_len[i]], FctRstBackupBuf[4], Collector_Config_LenList[i]);	//复制数据
			continue;
		}

		Collector_Config_LenList[i] = strlen(Collector_Set_Tab[i]);					 //获取每个寄存器数值的长度
		Parameter_len[i]= Parameter_length ; 										 //数据地址
		Parameter_length  =  Parameter_length + Collector_Config_LenList[i];   		 //参数列表实际总长度
		memcpy(&SysTAB_Parameter[Parameter_len[i]],Collector_Set_Tab[i],Collector_Config_LenList[i]);	//复制数据
	}
	memset(pbuf, 0  ,W25X_FLASH_ErasePageSize);									     //清缓存

	memcpy(&pbuf[CONFIG_LEN_BUFADDR],&Collector_Config_LenList,Parameter_len_MAX);		     //更新寄存器的长度链表
	memcpy(&pbuf[CONFIG_BUFADDR],&SysTAB_Parameter, Parameter_length);	 //更新写入寄存器数据
	/* CRC16校验 */
	CRC16 = GetCRC16(pbuf,CONFIG_CRC16_NUM);
	uCRC16[0] = CRC16>>8;
	uCRC16[1] = CRC16&0x00ff;
	pbuf[CONFIG_CRC16_BUFADDR] = uCRC16[0];
	pbuf[CONFIG_CRC16_BUFADDR+1] = uCRC16[1];
	/* 写主参数区 */
	IOT_Mutex_SPI_FLASH_WriteBuff(pbuf,REG_LEN_LIST_ADDR,W25X_FLASH_ErasePageSize);
	/* 写入备份区 */
	IOT_Mutex_SPI_FLASH_WriteBuff(pbuf,BACKUP_CONFIG_ADDR,W25X_FLASH_ErasePageSize);
	memset(pbuf,0,SECTOR_SIZE);			//清缓存

	IOT_Set_OTA_breakpoint_ui(0); 	//断点位置 清0

	i = 0;
	do{
		 ESP_ERROR_CHECK( esp_wifi_restore());	//清除已配置的wifi参数

		 memset(&conf , 0x00 , sizeof(wifi_config_t));
		 esp_wifi_get_config(ESP_IF_WIFI_STA , &conf);
		 IOT_Printf("conf.sta.ssid = %s \r\n",conf.sta.ssid);

		 i++;
		 if(i == 10)
		 {
			 IOT_Printf("wifi stop \r\n");
			 ESP_ERROR_CHECK( esp_wifi_stop());
		 }

	 }while((conf.sta.ssid[0] != 0x00) && (i <= 10));

}
uint8_t _GucServerParamSetFlag = 0;         // 设置服务器参数：IP或端口标志位
char _GucServerIPBackupBuf[50] = {0}; 		// 服务器IP地址备份区
char _GucServerPortBackupBuf[6] = {0};		// 服务器端口备份区
// 78、79服务器参数备份 LI 2020.01.06
/* TCP_num_null/TCP_num_2 设置“17、18、19”或 TCP_num_1设置“78、79”*/
uint8_t g_ucServerParamSetType = 0;  		// 设置服务器参数类型（17、18、19 或 78、79）

//#define   Serverflag_NULL  		0x00    // NULL 状态
//#define   Serverflag_disable    0x01      // 禁止保存IP和端口
//#define   Serverflag_enable     0x02      // 允许设置IP和端口，但是需要网络连接成功后才能保存
//#define   Serverflag_savetest   0x03      // 无条件允许设置和保存IP和端口，主要用于生产设置用
//#define   Serverflag_backoldip   	0x04    // 备份正在连接的旧IP和端口
//#define   Serverflag_savenowip   	0x05    // 无条件允许设置和保存IP和端口，主要用于设置IP成功连接网络后保存用
//#define   Serverflag_GETflag   	0xff    // 获取设置参数状态


/************************************************************************
 * FunctionName : IOT_ServerParamSet_Check LI 2018.08.08
 * Description  : 服务器参数设置校验
 * Parameters   : uint8_t mode  : 设置参数方式
 * Returns      : 解包状态
 * Notice       : 1/2/3
************************************************************************/
uint8_t IOT_ServerParamSet_Check(uint8_t mode)
{
	if(mode == 0xff)     			// 获取设置参数状态
	{
		return _GucServerParamSetFlag;
	}
	if(mode == Serverflag_NULL)          			// NULL 状态
	{
		_GucServerParamSetFlag = Serverflag_NULL;
	}
	else if(mode == Serverflag_disable)
	{
		_GucServerParamSetFlag = Serverflag_disable; // 禁止保存IP和端口
	}
	else if(mode == Serverflag_enable)
	{
		_GucServerParamSetFlag = Serverflag_enable; // 允许设置IP和端口，但是需要网络连接成功后才能保存
	}
	else if(mode == Serverflag_savetest)
	{
		_GucServerParamSetFlag = Serverflag_savetest; // 无条件允许设置和保存IP和端口，主要用于生产设置用
	}
	else if(mode == Serverflag_backoldip)            	// 备份正在连接的旧IP和端口
	{
		memset(_GucServerIPBackupBuf,0,sizeof(_GucServerIPBackupBuf));
		memset(_GucServerPortBackupBuf,0,sizeof(_GucServerPortBackupBuf));
 		memcpy(_GucServerIPBackupBuf,&SysTAB_Parameter[Parameter_len[REG_ServerIP]],Collector_Config_LenList[REG_ServerIP]);  // 备份旧IP 17
 		memcpy(_GucServerPortBackupBuf,&SysTAB_Parameter[Parameter_len[REG_ServerPORT]],Collector_Config_LenList[REG_ServerPORT]);// 备份旧Port 18
 		IOT_Printf("IOT_ServerParamSet_Check() Backup 17 ip:%s  \r\n",_GucServerIPBackupBuf );
 		IOT_Printf("IOT_ServerParamSet_Check() Backup 18 port:%s   \r\n",_GucServerPortBackupBuf );
	}
	else if(mode == Serverflag_savenowip)             // 无条件允许设置和保存IP和端口，主要用于设置IP成功连接网络后保存用
	{
		IOT_SystemParameterSet(REG_ServerIP,_GucServerIPBackupBuf,strlen(_GucServerIPBackupBuf));        // 保存新IP 17
		IOT_SystemParameterSet(REG_ServerPORT,_GucServerPortBackupBuf,strlen(_GucServerPortBackupBuf));    // 保存新IP 18
		IOT_SystemParameterSet(REG_ServerURL,_GucServerIPBackupBuf,strlen(_GucServerIPBackupBuf));		   // 保存新IP 19

		IOT_Printf("IOT_ServerParamSet_Check() set 17 ip:%s \r\n",_GucServerIPBackupBuf  );
		IOT_Printf("IOT_ServerParamSet_Check() set 18 port:%s  \r\n",_GucServerPortBackupBuf );

	}
	return _GucServerParamSetFlag;
}
/*************************************************************************
 * FunctionName : IOT_ServerConnect_Check LI 2018.08.08
 * Description  : 服务器连接检验，避免服务器ip或port出错掉网
 * Parameters   : u8 type  : 计算方式 0-清空连接次数  1-计算连接次数  0xFF-检查连接状态
 * Returns      : 解包状态
 * Notice       : 0/1/2/3
*************************************************************************/
uint8_t IOT_ServerConnect_Check(uint8_t type)
{
	#define IPConnectFailTimes  5  // 服务器地址连接失败允许次数	LI 2018.08.08
	static uint8_t sucServerIPConnectTimes = 0;

	#define IPSetWaitTime (30*1000) // 30S 预留ip与port一起设置的时间
	static uint32_t ServerIPSetWaitTime = 0;
	static uint8_t WaitFlag = 0;

	if(type == 0) // 清空连接次数
	{
		sucServerIPConnectTimes = 0;
		WaitFlag = 0;
		ESP_LOGI(SYS,"IOT_TSServerConnect_Check() 清空连接次数计数！\r\n");
		return 0;
	}
	if(IOT_ServerParamSet_Check(Serverflag_GETflag) == Serverflag_enable) 	// 判断是否在设置IP或端口，服务器进行重连
	{
		if(WaitFlag == 0)
		{
		    ESP_LOGI(SYS,"IOT_TSServerConnect_Check() 设置服务器ip，等待连接新服务器地址\r\n");
 			ServerIPSetWaitTime = GetSystemTick_ms();
			WaitFlag = 1;
			return 0;
		}
		else if(WaitFlag == 1)
		{
 			if((GetSystemTick_ms () - ServerIPSetWaitTime) > IPSetWaitTime)
			{
				ESP_LOGI(SYS,"IOT_TSServerConnect_Check() 设置服务器ip，连接新服务器地址开始 !!! \r\n");
				WaitFlag = 2;
				if(System_state.Server_Online==1 )
				{
					System_state.Server_Online = 0;
				}
			}
			return 0;
		}
		if(type == 1)
		{
			sucServerIPConnectTimes++; 						 // 计算服务器连接次数
			ESP_LOGI(SYS,"IOT_TSServerConnect_Check() 连接新服务器地址失败次数 = %d\r\n",sucServerIPConnectTimes);
			if(sucServerIPConnectTimes > IPConnectFailTimes) // 连接服务器多次失败，转回上一次旧服务器
			{
//				IOT_ServerParamSet_Check(1); // 禁止保存IP和端口
//				sucServerIPConnectTimes = 0;
//				IOT_Change_GPRS_Steps(cGPRS_Restart_Step);
    			System_state.Server_Online   = 0;
    			IOT_Printf("IOT_TSServerConnect_Check() 服务器ip错误，连接服务器地址失败，转回默认服务器 ！！！ \r\n");
				return 1;	// 新增在还原旧IP和旧Port后 return 1 用于重启采集器
			}
		}
	}
	else
	{
		sucServerIPConnectTimes = 0;
		WaitFlag = 0;
	}

	return 0;
}


/********************
 * 设备更改服务器连接次数计数
 *
 * *******************/
uint8_t uSetserverflag=0; 			// 判断是否使用备份的IP 连接
char _ucServerIPBuf[50] = {0}; 		// 远程设置服务器IP地址
char _ucServerPortBuf[6] = {0};		// 远程设置服务器端口
uint8_t IOT_SETConnect_uc(uint8_t Settype)
{
	static uint8_t Again_connectnum=0;
	if(Settype ==0 )
	{
		uSetserverflag=1;
		Again_connectnum=0;

		IOT_IPPORTSet_us(ESP_WRITE,30); // 30 秒后触发连接服务器
	}
	else
	{
		Again_connectnum++;
		if( Again_connectnum > 5)   	// 重连 次数 大于 5 次  重启设备  恢复端口
		{
			uSetserverflag=0;

			//IOT_SYSRESTART();			// 重启设备  自动恢复 17 / 18 寄存器服务器
		}
	}
	return uSetserverflag;
}

/****************************************************************
  *     加载采集器参数
  *		此函数调用应放在，SPI FLASH参数表加载完成后
****************************************************************/
void IOT_UPDATETime(void)
{
	uint8_t i=0;
	char p[4] = {0};
//	/* 加载轮询时间 */
	memcpy(p,&SysTAB_Parameter[Parameter_len[REG_Interval]],4);
	for(i=0; i < Collector_Config_LenList[REG_Interval]; i++)
	{
		if(((p[i] >= '0') && (p[i] <= '9')) || p[i] == '.')
			continue;
		else
		{
			Collector_Config_LenList[REG_Interval] = 0;
		}
	}
	if(Collector_Config_LenList[REG_Interval] == 0)
	{
//		Change_Config_Value("5",4,1);
		System_state.Up04dateTime = 300  ;// 5*60 = 5min  单位秒
	}
	else if(Collector_Config_LenList[REG_Interval] == 1)
		System_state.Up04dateTime = ( (p[0]-0x30) * 60 )  ;					 //一位数min  转换  单位秒
	else if(Collector_Config_LenList[REG_Interval] == 2)
		System_state.Up04dateTime = ( (((p[0]-0x30)*10) +(p[1]-0x30)) * 60 )  ;//两位数min  转换  单位秒 （max最大值）
	else if(Collector_Config_LenList[REG_Interval] == 3 && ( p[1]== '.'))
		System_state.Up04dateTime = (p[0]-0x30)*60 + ((p[2]-0x30)*60)/10;		 //三位数min  转换  单位秒 （小数点数 一位小数点  小数点数/6s）
	else if(Collector_Config_LenList[REG_Interval] == 4 && ( p[2]== '.'))
		System_state.Up04dateTime = ( (((p[0]-0x30)*10) + (p[1]-0x30)) * 60  + ((p[3]-0x30)*60/10)) ;//四位数min  转换  单位秒 （小数点数 一位小数点）
	else
	{
		//Change_Config_Value("5",4,1);
		System_state.Up04dateTime = 300  ;// 5*60 = 5min  单位秒
	}
	//Mqtt_DATATime = System_state.Up04dateTime;	//04数据上传的间隔与4号寄存器关联 chen 20220524
	ESP_LOGI(SYS,"\r\nLoadingConfig string = %s\r\n",p);
	ESP_LOGI(SYS,"\r\nLoadingConfig int = %d\r\n",System_state.Up04dateTime);
	IOT_TCPDelaytimeSet_uc(ESP_WRITE,System_state.Up04dateTime);

}

void IOT_SetInvTime0x10(uint16_t TimeStartaddr,uint16_t TimeEndaddr ,uint8_t *pSetTimer)
{
	/* 不允许设置, 或设置的寄存器错误, 或逆变器不在线 */
	if(InvCommCtrl.UpdateInvTimeFlag == 0)
		return;

	InvCommCtrl.SetParamStart = TimeStartaddr;
	InvCommCtrl.SetParamEnd = TimeEndaddr;
	InvCommCtrl.SetParamDataLen = (InvCommCtrl.SetParamEnd - InvCommCtrl.SetParamStart + 1) * 2;
	for(uint8_t i=0 ,j=0 ; i<InvCommCtrl.SetParamDataLen ;i++)
	{
		InvCommCtrl.SetParamData[j]=0;
		InvCommCtrl.SetParamData[j+1]=pSetTimer[i];
		j=j+2;
	}

	System_state.Host_setflag |= CMD_0X10;
	InvCommCtrl.CMDFlagGrop   |= CMD_0X10;

	//memcpy(InvCommCtrl.SetParamData, pSetTimer , InvCommCtrl.SetParamDataLen);

}

void PrintfParam(void)
{
	uint8_t i = 0;
	//uint8_t PrintfBUF[80];
	uint8_t PrintfBUF[256] = {0};  //buff大小由80  改成 256

	ESP_LOGI(SYS,"***********************IOT_ParameterTABInit *******************\r\n" );
	for(i = 0; i < Parameter_len_MAX ; i++)
	{
		memset(PrintfBUF,0,sizeof(PrintfBUF));
		memcpy(&PrintfBUF,&SysTAB_Parameter[Parameter_len[i]],Collector_Config_LenList[i]);
		ESP_LOGI(SYS,"SysTAB_Parameter %d = %-20s    addr = %-2d    SUMlen =%-2d \r\n", i, PrintfBUF, Parameter_len[i] ,Collector_Config_LenList[i]);
	}
	ESP_LOGI(SYS,"***********************IOT_ParameterTABInit *******************\r\n" );
}

/******************************************************************************
 * FunctionName : IOT_SystemFunc_Init
 * Description  : 系统功能模块初始化
 * Parameters   : none
 * Returns      : none
 * Notice       : none
*******************************************************************************/
void IOT_SystemFunc_Init(void)
{
	IOT_AppxFlashMutex();
	ESP_LOGI(SYS,"\r\n APP IOT_SystemFunc_Init...\r\n");
	IOT_ParameterTABInit();
	/* 打印参数表 */
	PrintfParam();
 	ESP_LOGI(SYS, "main minimum free heap size = %d !!!\r\n", esp_get_minimum_free_heap_size());

 	IOT_SystemDATA_init();
	/* 根据DTC，确定数据字段（0x03 0x04）读取初始化 */
//	if(IOTESPBaudRateSyncStatus == ESP_ON)
//	{
//		dtcInit(InvCommCtrl.DTC);
//	}
//	else if(IOTESPBaudRateSyncStatus == ESP_OFF)
//	{
//		InvCommCtrl.DTC = 0xFFFF; // 默认字段：0-44 45-89
//		dtcInit(InvCommCtrl.DTC);
//	}
/*******************************************************/
/************兼容逆变器/6.0新协议后添加************************/
/*******************************************************/
}

/****************
 * 重启
 *
 * **************/
void IOT_SYSRESTART(void)
{
	esp_restart();
}


/****************
 * 重启
 *
 * **************/

void IOT_Reset_Reason_Printf(void)
{
	esp_reset_reason_t _reason_t =  esp_reset_reason();

	IOT_Printf("\r\n");
	IOT_Printf("-*****************************************\r\n");
	IOT_Printf("ESP Restart Reason : %d !\r\n" , _reason_t);
	IOT_Printf("-*****************************************\r\n");
	IOT_Printf("\r\n");

}

#if 0
/****************************************************************
  *     重置记录存储区
  ***************************************************************
  */
void ClearAllRecord(void)
{
	uint16_t i=0;
	//第四个扇区开始
	//先擦扇 同步到块 0x4000  开始
	//再擦块
	for(i = RECORD_SECTOR_ST; i < 16; i ++)
	{
		W25QXX_Erase_Sector(i * SECTOR_SIZE);
	}
	for(i=16; i<RECORD_SECTOR_END;)			//block = 16sec
	{
	 W25QXX_Erase_Block((i/16) * BLOCK_SIZE);  //64K   65536 /4096 =16
	 i += 16;
	}
	ESP_LOGI(SYS,"\r\n*****ClearAllRecord over  **\r\n");

}
#endif

/******************************************************
 * 获取数据采集器参数
 * IOT_CollectorParam_Get
 *
 * ****************************************************/

uint16_t IOT_CollectorParam_Get(uint8_t paramnum,uint8_t *src )
{
//	uint8_t i;
//	uint8_t *pbuf ;
//	uint16_t CRC16;
//	uint8_t uCRC16[2];
//	static uint8_t s_ucReadingData_Timers = 0;  	// 系统参数读取计数
	char NOWTime[20];
//  System_Config  				// 采集器寄存器参数
//  Collector_Config_LenList  	// 记录每个参数数据的长度
//  参数读取flash更新
	if(paramnum == 31)   // 获取时间
 	{
 	    NOWTime[0]='2';
 	    NOWTime[1]='0';
 	  	NOWTime[2]= sTime.data.Year /10 +0x30;
 	 	NOWTime[3]= sTime.data.Year %10 +0x30;
 		NOWTime[4]='-';
 		NOWTime[5]= sTime.data.Month /10 +0x30;
 		NOWTime[6]= sTime.data.Month %10 +0x30;
 		NOWTime[7]='-';
 		NOWTime[8]= sTime.data.Date /10 +0x30;
 		NOWTime[9]= sTime.data.Date %10 +0x30;
 		NOWTime[10]=' ';
 		NOWTime[11]= sTime.data.Hours /10 +0x30;
 		NOWTime[12]= sTime.data.Hours %10 +0x30;
 		NOWTime[13]= ':';
 		NOWTime[14]= sTime.data.Minutes /10 +0x30;
 		NOWTime[15]= sTime.data.Minutes %10 +0x30;
 		NOWTime[16]= ':';
 		NOWTime[17]= sTime.data.Seconds /10 +0x30;
 		NOWTime[18]= sTime.data.Seconds %10 +0x30;
 		memcpy(&SysTAB_Parameter[Parameter_len[31]],NOWTime,19);
 		ESP_LOGI(SYS,"\r\n IOT_CollectorParam_Get NOWTime =%s \r\n",NOWTime);
 	}
 //	memcpy(src ,&SysTAB_Parameter[Parameter_len[paramnum]],Collector_Config_LenList[paramnum]);	//复制数据
	IOT_Printf(" Collector_Config_LenList[paramnum]=%d \r\n",  Collector_Config_LenList[paramnum]);
 	return Collector_Config_LenList[paramnum];
}

#if 0  //20220727  chen  弃用
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/****************************************************************
  *     存入单条数据记录
  *		ItemPoint:要存入的数据指针
  ***************************************************************
  */
#define MAX_PACK		(((DATA_MAX) / (RECORD_ITEM_SIZE_TRUE)) + 1)  	//一笔数据最多存几页
#define MAX_ERR_RD		(10)				//一次最多读取十个错误记录
#define FLAG_WR	1
#define FLAG_RD	0

static uint16_t storageID = 1111;	//存储计数,以1111开始，651111作为结束
volatile uint32_t u32TrRecCnt = 0;		//已存记录
volatile uint32_t u32TrOutCnt;		//已读记录
volatile uint32_t u32TrOutSave;
volatile uint8_t  Flag_RecordFull = 0;	//数据满标志位

void Write_RecordItem(uint8_t *ItemPoint)
{
	uint16_t crc16Check=0;
	uint32_t u32Addr=0;
	uint8_t uStatus[1]={0};
	uint16_t len = 0;
	uint8_t buf[256]={0};

	uint8_t i = 0;
	uint8_t packNum = 0;
	uint8_t lastLen = 0;

 	len = InputGrop_DataPacket(ItemPoint);							//打包数据

 	IOT_Printf("==================================\r\n");
 	IOT_Printf(" Write_RecordItem() 存储0x04 数据长度 = %d\r\n", len);
 	IOT_Printf("==================================\r\n");

//	/* 如果时间错误，则本条不存储 */
//	if(RTC_GetTime(Clock_Time_Tab) == 1)        // delete LI 20210427 因为系统续传逆变时间，防止采集器RTC时间读取出错
//		return;

	ItemPoint[28] = InvCommCtrl.PVClock_Time_Tab[0]; 			//历史数据 用逆变器数据年，月日时分秒
	ItemPoint[29] = InvCommCtrl.PVClock_Time_Tab[1];
	ItemPoint[30] = InvCommCtrl.PVClock_Time_Tab[2];

	ItemPoint[31] = InvCommCtrl.PVClock_Time_Tab[3];
	ItemPoint[32] = InvCommCtrl.PVClock_Time_Tab[4];
	ItemPoint[33] = InvCommCtrl.PVClock_Time_Tab[5];
//	IOT_Printf("\r\n记录04续传数据使用逆变器时间\r\n");

	if(len > RECORD_ITEM_SIZE_TRUE)
	{
		crc16Check = Modbus_Caluation_CRC16(ItemPoint, len);			//对于多页的,额外增加一个CRC16校验
		ItemPoint[len] = HIGH(crc16Check);
		ItemPoint[len+1] = LOW(crc16Check);
		len += 2;
	}

	packNum = (len / RECORD_ITEM_SIZE_TRUE);
	lastLen = (len % RECORD_ITEM_SIZE_TRUE);

	for(i = 0; i < packNum; i++)
	{
		/* 存序号 */
		if(storageID <= SAVANUM_END)
			storageID++;
		else
			storageID = SAVANUM_START;
		/* 拷贝缓存 */
		memset(buf, 0, 256);
		memcpy(buf, (ItemPoint + (i * RECORD_ITEM_SIZE_TRUE)), RECORD_ITEM_SIZE_TRUE);

		buf[RECORD_ITEM_SIZE_TRUE] = (storageID >> 8) & 0x00ff;
		buf[RECORD_ITEM_SIZE_TRUE+1] = (storageID) & 0x00ff;

		/* 存校验 */
		crc16Check = GetCRC16(buf,RECORD_ITEM_SIZE_TRUE+2);
		buf[RECORD_ITEM_SIZE_TRUE+2] = (crc16Check >> 8) & 0x00ff;
		buf[RECORD_ITEM_SIZE_TRUE+3] = (crc16Check) & 0x00ff;

		if(u32TrRecCnt >= RECORD_AMOUNT_NUM)
		{
			u32TrRecCnt = 0;			   		//出入记录记录条数
			Flag_RecordFull = 1; 				//出入记录已满

			uStatus[0] = FULL;
			IOT_Mutex_SPI_FLASH_WriteBuff(uStatus,RECORD_FULL,1);
		}

		buf[RECORD_ITEM_SIZE_TRUE+4] = PAGE_WR;				//读写标记位
		if(lastLen == 0)
		{
			buf[RECORD_PACK_ALL] = packNum;							//总包
		}
		else
		{
			buf[RECORD_PACK_ALL] = packNum + 1;						//总包
		}
		buf[RECORD_PACK_CUR] = i;								//当前包
		buf[RECORD_LAST_LEN] = RECORD_ITEM_SIZE_TRUE;		//当前长度

		u32Addr = RECORD_ADDR + (u32TrRecCnt * RECORD_ITEM_SIZE);
		IOT_Mutex_SPI_FLASH_WriteBuff(buf, u32Addr, RECORD_ITEM_SIZE_TRUE+5+3);


		u32TrRecCnt++;										//记录增加
//		IWDG_Feed();
		if(u32TrRecCnt%RECORD_SECTOR_MAX_NUM == 0)			//写扇区满标志位
		{
			uStatus[0] = SEC_FULL;
			IOT_Mutex_SPI_FLASH_WriteBuff(uStatus, u32Addr+RECORD_ITEM_SIZE-SEC_SIGN_ADDR,1);
		}
//		IWDG_Feed();
	}

	/* 最后一包 */
	if(lastLen > 0)
	{
				/* 存序号 */
		if(storageID <= SAVANUM_END)
			storageID++;
		else
			storageID = SAVANUM_START;

		/* 拷贝缓存 */
		memset(buf, 0, 256);
		memcpy(buf, (ItemPoint + (i * RECORD_ITEM_SIZE_TRUE)), lastLen);

		buf[RECORD_ITEM_SIZE_TRUE] = (storageID >> 8) & 0x00ff;
		buf[RECORD_ITEM_SIZE_TRUE+1] = (storageID) & 0x00ff;

		/* 存校验 */
		crc16Check = GetCRC16(buf,RECORD_ITEM_SIZE_TRUE+2);
		buf[RECORD_ITEM_SIZE_TRUE+2] = (crc16Check >> 8) & 0x00ff;
		buf[RECORD_ITEM_SIZE_TRUE+3] = (crc16Check) & 0x00ff;


		if(u32TrRecCnt >= RECORD_AMOUNT_NUM)
		{
			u32TrRecCnt = 0;			   		//出入记录记录条数
			Flag_RecordFull = 1; 				//出入记录已满

			uStatus[0] = FULL;
			IOT_Mutex_SPI_FLASH_WriteBuff(uStatus, RECORD_FULL, 1);
		}

		buf[RECORD_ITEM_SIZE_TRUE+4] = PAGE_WR;				//读写标记位

		if(lastLen == 0)
		{
			buf[RECORD_PACK_ALL] = packNum;							//总包
			buf[RECORD_PACK_CUR] = (i-1);					//没有第三个包,但上面for循环最后++了
		}
		else
		{
			buf[RECORD_PACK_ALL] = packNum + 1;						//总包
			buf[RECORD_PACK_CUR] = i;								//当前包
		}
		buf[RECORD_LAST_LEN] = lastLen;								//当前长度

		u32Addr = RECORD_ADDR + (u32TrRecCnt * RECORD_ITEM_SIZE);
		IOT_Mutex_SPI_FLASH_WriteBuff(buf, u32Addr, RECORD_ITEM_SIZE_TRUE+5+3);

		u32TrRecCnt++;										//记录增加
//		IWDG_Feed();
		if(u32TrRecCnt%RECORD_SECTOR_MAX_NUM == 0)			//写扇区满标志位
		{
			uStatus[0] = SEC_FULL;
			IOT_Mutex_SPI_FLASH_WriteBuff(uStatus, u32Addr+RECORD_ITEM_SIZE-SEC_SIGN_ADDR,1);
		}
//		IWDG_Feed();
	}
}

/****************************************************************
  *     读单条数据记录
  *		ItemPoint:要存入的数据指针
****************************************************************/
uint16_t readRecord(uint8_t *tmp)
{
	uint8_t buf[256]={0};

	uint16_t crc = 0;
	uint16_t len = 0;

	uint16_t packAll = 0, packCur = 0, packAllPrev = 0, packCurPrev = 0, packLen = 0;
	uint8_t i = 0;
	uint8_t normalFlag = 0;

	if(Read_RecordItem(buf) == 0)
	{
		/*****************************************************/
		IOT_Printf("\r\n Read_RecordItem() u32TrRecCnt = %d -- u32TrOutCnt = %d\r\n",u32TrRecCnt,u32TrOutCnt);
		FlashData_Check(buf);
		/*****************************************************/
		packAll = buf[RECORD_PACK_ALL];
		packCur = buf[RECORD_PACK_CUR];
		/* 只有一条记录的情况 */
		if((packAll == 0) || (packAll > MAX_PACK) ||((packAll == 1) && (packCur == 0)))
		{
			IOT_Printf("\r\n\r\n==================================\r\n");
			IOT_Printf(" readRecord() 读已存储0x04 数据 单次 = %d 条记录\r\n", 1);
			IOT_Printf("==================================\r\n\r\n");
			packLen = buf[RECORD_LAST_LEN];
			memcpy(tmp, buf, packLen);

			return buf[RECORD_LAST_LEN];
		}
		else if(packCur != 0)/* 当前记录不是完整包 */
		{
			for(i = 0; i < MAX_ERR_RD; i++)
			{
				if(Read_RecordItem(buf) == 0)
				{
					packAll = buf[RECORD_PACK_ALL];
					packCur = buf[RECORD_PACK_CUR];

					if((packAll > 0) && (packCur == 0) && (packAll < MAX_PACK))
					{
						normalFlag = 1;
						break;
					}
				}
			}
			/* 没有找到需要拼接的记录 */
			if(normalFlag != 1)
			{
				return 0;
			}
		}
		/* 开始拼接数据 */
		if(packAll > packCur)
		{
			packLen = RECORD_ITEM_SIZE_TRUE;
		}
		else
		{
			packLen = buf[RECORD_LAST_LEN];
		}

		packAllPrev = packAll;
		packCurPrev = packCur;

		memcpy(tmp, buf, packLen);
		len += packLen;

		for(i = 1; i < MAX_PACK; i++)			//最多读几笔数据
		{
			if(Read_RecordItem(buf) == 0)
			{
				packAll = buf[RECORD_PACK_ALL];
				packCur = buf[RECORD_PACK_CUR];
																//例如占三页,为0,1,2
				if((packAll == packAllPrev) && (packCur == (packCurPrev + 1)))
				{
					packAllPrev = packAll;
					packCurPrev = packCur;
					packLen = buf[RECORD_LAST_LEN];

					memcpy(&tmp[len], buf, packLen);		//拷贝内存
					len += packLen;


					if(packAll == (packCur+1))
					{
						crc = Modbus_Caluation_CRC16(tmp, len - 2);

						if(MERGE(tmp[len-2], tmp[len-1]) == crc)
						{
							/* 检测存储的记录采集器SN和当前是否一致 */
							if(memcmp(&tmp[8],&SysTAB_Parameter[Parameter_len[REG_SN]],10) != 0)			//保存的不是当前逆变器SN，或采集器SN
								return 0;
							/* 检测存储的记录逆变器SN和当前是否一致 */
							if(memcmp(&tmp[18], (char *)InvCommCtrl.SN, 10) != 0)			//保存的不是当前逆变器SN，或采集器SN
								return 0;

							return len - 2;
						}
					}
					continue;
				}
			}

			return 0;			//读失败
		}
		IOT_Printf("\r\n\r\n==================================\r\n");
		IOT_Printf(" readRecord() 读已存储0x04 数据 单次 = %d 条记录\r\n", i);
		IOT_Printf("==================================\r\n\r\n");

	}

	return 0;

}

uint8_t Read_RecordItem(uint8_t *ItemPoint)
{
	uint32_t u32Addr=0;
	uint8_t uStatus[1]={0};
	uint16_t crc161,crc162 = 0;
//	uint16_t  packThis=0,packAl=0, lastLen=0;
	/* 数据区未满 && 记录为空*/
	if((Flag_RecordFull==0) && (u32TrRecCnt==0))
	{
		IOT_Printf("*****Read_RecordItem***************(Flag_RecordFull==0) && (u32TrRecCnt==0)**************************\r\n");
		return 1;
	}
	/* 数据区未满 && 已读>=已存 */
	if((Flag_RecordFull==0) && (u32TrOutCnt >= u32TrRecCnt))
	{
		IOT_Printf("*****Read_RecordItem*************(Flag_RecordFull==0) && (u32TrOutCnt >= u32TrRecCnt)**************\r\n");

		//ClrAllTransRecord();
		return 1;
	}
	/* 数据区已满 && (已读-16)<=已存 */						//存满后，重头开始存的，又追上了正在读的，这种情况数据丢失不可避免
	if(
			(Flag_RecordFull==1) &&
			(((u32TrOutCnt/RECORD_SECTOR_MAX_NUM) < (u32TrRecCnt/RECORD_SECTOR_MAX_NUM)) ||
				(((u32TrOutCnt/RECORD_SECTOR_MAX_NUM) == (u32TrRecCnt/RECORD_SECTOR_MAX_NUM)) &&
					(u32TrRecCnt%RECORD_SECTOR_MAX_NUM != 0)))
	  )									//记录区已满&&(已读-16)<=已存，这样会导致当前扇区数据丢失，跳过这一扇区
		{
				u32TrOutCnt = ((u32TrRecCnt/RECORD_SECTOR_MAX_NUM) + 1) * RECORD_SECTOR_MAX_NUM;
		}
	/* 数据区已满 && 已读>=最高存储数目 */
	if(u32TrOutCnt >= RECORD_AMOUNT_NUM)
	{
			u32TrOutCnt = 0;
			Flag_RecordFull = 0;		//已读循环，已经追上已存的循环

			uStatus[0] = 0;
			W25QXX_Erase_Sector(RECORD_FULL);			//清除记录区的标示
	}

	u32Addr = RECORD_ADDR + (u32TrOutCnt * RECORD_ITEM_SIZE);
	IOT_Mutex_SPI_FLASH_BufferRead(ItemPoint, u32Addr, RECORD_LAST_LEN+1);

	uStatus[0] = PAGE_RD;				//记录已读
	IOT_Mutex_SPI_FLASH_WriteBuff(uStatus, u32Addr+RECORD_STA_SINGLE, 1);		//写标志，此时未清就写，数据变为0

	u32TrOutCnt++;


	if((u32TrOutCnt%RECORD_SECTOR_MAX_NUM == 0) && (u32TrOutCnt != 0))			//一个扇区读完
	{
		uStatus[0] = SEC_READ;
		IOT_Mutex_SPI_FLASH_WriteBuff(uStatus, u32Addr + RECORD_ITEM_SIZE - SEC_SIGN_ADDR, 1);		//读完一个扇区后，写标志，此时未清就写，数据变为0
	}


	crc161 = GetCRC16(ItemPoint,RECORD_ITEM_SIZE_TRUE+2);
	crc162 = ItemPoint[RECORD_ITEM_SIZE_TRUE+2];
	crc162 = (crc162 << 8) | ItemPoint[RECORD_ITEM_SIZE_TRUE+3];

	if(crc161 != crc162)
	{
		IOT_Printf("****Read_RecordItem***********(crc161 != crc162)******************\r\n");
		return 1;
	}
	/* 校验数据准确性 */
	return 0;

}

/****************************************************************
  *     获取系统已存，已读的记录等状态
  *		获取已读，已存指针的位置
  ***************************************************************
  */
uint8_t Get_RecordStatus(void)
{
	uint16_t i;
	uint8_t uStatus[1];
	uint8_t flag_Break;
	uint16_t crc16Check1,crc16Check2;
	uint16_t savaID;
	uint8_t flag_BrokenPoint = 0;
	uint8_t flag_FirstFind = 0xaa;
	static uint8_t GET_StatusFLAG=0;

	if(GET_StatusFLAG==1)
	{
		return 0;
	}
	uint8_t *pbuf= (uint8_t*)malloc(W25X_FLASH_ErasePageSize);
	//pbuf = W25QXX_BUFFER;
	memset(pbuf,0,W25X_FLASH_ErasePageSize);

	u32TrRecCnt = 0;
	u32TrOutCnt = 0;
	Flag_RecordFull = 0;
	storageID = 0;
	flag_BrokenPoint = 0;
	flag_Break = 0;

	ESP_LOGI(SYS,"\r\n Get_RecordStatus init ...  \r\n");

	for(i=0; i<=RECORD_AMOUNT_NUM; i++) // 读取flash保存数据记录
	{

		IOT_Mutex_SPI_FLASH_BufferRead(pbuf,RECORD_ADDR+i*RECORD_ITEM_SIZE,RECORD_ITEM_SIZE_TRUE+5);		//读标志位
		uStatus[0] = pbuf[RECORD_ITEM_SIZE_TRUE+4];

		crc16Check1 = pbuf[RECORD_ITEM_SIZE_TRUE+2];
		crc16Check1 = (crc16Check1 << 8) | pbuf[RECORD_ITEM_SIZE_TRUE+3];
		crc16Check2 = GetCRC16(pbuf,RECORD_ITEM_SIZE_TRUE+2);
	//	ESP_LOGI(SYS,"\r\n crc16Check1 =%d ...crc16Check2=%d  i=%d\r\n",crc16Check1,crc16Check2,i);

		if(crc16Check1 == crc16Check2)		//校验正确
		{
			savaID = pbuf[RECORD_ITEM_SIZE_TRUE];
			savaID = (savaID << 8) | pbuf[RECORD_ITEM_SIZE_TRUE+1];
			if(storageID == 0)
			{
				storageID = savaID;
//				u32TrRecCnt = i+1;
				if(uStatus[0] == PAGE_WR)
				{
					flag_FirstFind = FLAG_WR;
					u32TrRecCnt = i+1;
				}
				else
				{
					flag_FirstFind = FLAG_RD;
					u32TrOutCnt = i+1;
					u32TrRecCnt = i+1;
				}
			}
			else if(((storageID > savaID-10) && (storageID < savaID+10))||
					(((storageID < SAVANUM_END+10) && (storageID > SAVANUM_END-10)) &&((SAVANUM_START+10 > savaID) && (SAVANUM_START-10 < savaID)))
					)
			{
				storageID = savaID;
				u32TrRecCnt = i+1;
				if(flag_FirstFind == FLAG_RD)		//如果最开始是已读
				{
					if(uStatus[0] == PAGE_RD)
						u32TrOutCnt = i+1;
				}
			}
			else
			{
				flag_BrokenPoint = 1;
				if(flag_FirstFind == FLAG_RD)		//如果最开始是已读
				{
					if(flag_Break == 0)				//最开始是0，并且没有从0-1；
					{
						u32TrOutCnt = i;
					}
				break;
				}
			}

			if(((flag_FirstFind == FLAG_WR) && flag_BrokenPoint) || ((flag_FirstFind == FLAG_RD) && (flag_Break == 0)))
			{
				if(uStatus[0] == PAGE_WR)
				{
					u32TrOutCnt = i;
					if(flag_FirstFind == FLAG_RD)
					{
						flag_Break = 1;
					}
					else if(flag_FirstFind == FLAG_WR)
					{
						break;
					}
				}
			}

		//	ESP_LOGI(SYS,"\r\n savaID =%d ...storageID=%d  flag_FirstFind=%d\r\n",savaID,storageID,flag_FirstFind);
		}
		else if((flag_FirstFind == FLAG_RD) && (crc16Check1 == 0xffff))
		{
			break;					//RD在前面，说明未满一个周期，出现0xff说明结束.
		}
		else if( (crc16Check1 == 0xffff) )
		{
			break;
		}

		vTaskDelay(100 / portTICK_PERIOD_MS);
	}

	IOT_Printf("\r\n/******************************************/\r\n");
	IOT_Printf("u32TrRecCnt = %d,u32TrOutCnt = %d\r\n",u32TrRecCnt,u32TrOutCnt);
	IOT_Printf("\r\n/******************************************/\r\n");

	if(flag_FirstFind == FLAG_WR)
	{
//		u32TrRecCnt += 1;			//没有拿出来加是防止记录为0的情况
		if((u32TrRecCnt == RECORD_AMOUNT_NUM) || (u32TrOutCnt != 0))
		{
			Flag_RecordFull = 1;
		}

	}
	else if(flag_FirstFind == FLAG_RD)
	{
//		u32TrRecCnt += 1;
	}
    free(pbuf);
	GET_StatusFLAG=1;

	return 1;
}

/****************************************************************
  *     获取当前存储数目
  ***************************************************************
  */
uint16_t Get_RecordNum(void)
{
	uint16_t NumberBuf=0;
	if(Flag_RecordFull == 0)
	{
		if(u32TrRecCnt>=u32TrOutCnt)
			NumberBuf = u32TrRecCnt - u32TrOutCnt;
		else
			NumberBuf = 0;
	}
	else if(Flag_RecordFull == 1)
	{
		NumberBuf = (RECORD_AMOUNT_NUM - u32TrOutCnt) + u32TrRecCnt;
	}

	if(NumberBuf)
    ESP_LOGI(SYS,"\r\n *** Get_RecordNum() flash data saved %d strips***\r\n",NumberBuf);

	return NumberBuf;
}



/*
//FLASH数据区可能存在的几种状态
//未写满一个循环

|-----------|----------------|-------------------|		←------|
	0x00	已读		0x5a	已存		0xff||0x00			|
																|
|----------------------------|-------------------|				|
	0x00					已存		0xff					|
							已读								|
																|
																|
																|
//已写满一个循环												|
|------------|---|-------------|-------------------|			|
	0x5a	已存	0xff	0x00  已读		0x5a				|
																|
|------------------------------|--|-----------------|			|
							  已存0xff							|
				0x5a		  已读		0x5a					|
																|
|------------|-----------------|-------------------|		____|
	0x00	已读		0x5a  	  已存		0x00

|------------|---|-------------|-------------------|
	0x00	已读0x5a	0x5a  已存		0x5a
*/
/****************************************************************
  *     校验已保存flash数据是否为同一个逆变器SN或采集器SN保存的数据
****************************************************************/
void FlashData_Check(uint8_t *buf)
{
	/* 1、检测存储的记录采集器SN和当前是否一致 2、检测存储的记录逆变器SN和当前是否一致 */

	if((memcmp(&buf[8],&SysTAB_Parameter[Parameter_len[REG_SN]],10) != 0) || \
		(memcmp(&buf[18], (char *)InvCommCtrl.SN, 10) != 0))
	{
			ClearAllRecord(); // SN号不匹配，清空所有flash数据记录
	//		SystemRestart();  // 清空完flash数据，重启采集器
			IOT_Printf("FlashData_Check 重启\r\n");
	//		IOT_SYSTEM_RESET();

	}
}
#endif
///////////////////////////////////////end//////////////////////////////////////////////////////////////////////

void IOT_SysTEM_Task(void *arg)
{
	static long long uiTime =0;
	IOT_WiFiScan_uc(ESP_WRITE ,180 );

#if DEBUG_HEAPSIZE
	  static uint16_t timer_logoc=0;
#endif

	ESP_LOGI(SYS, "IN IOT_SysTEM_Task uxTaskGetStackHighWaterMark= %d !!!\r\n", uxTaskGetStackHighWaterMark(NULL));
	while(1)
	{
		if( (System_state.Wifi_state==0 )&& (IOT_WiFiScan_uc(ESP_READ ,0 ) ==ESP_OK1 ))  		// 定时扫描附件wifi
		{
			//ESP_LOGI(SYS, "IOT_WiFiScan_uc 180 sec \r\n" );
			IOT_WiFiScan_uc(ESP_WRITE , ScanIntervalTime ); //扫描时间设置为 动态  20220810 chen

			if(ESP_OK1 == IOT_WiFiConnectTimeout_uc(ESP_READ , 0 ))		//WIFI连接超时
			{
				System_state.Wifi_ActiveReset &= ~WIFI_CONNETING;
			}

			if((System_state.Wifi_ActiveReset  & WIFI_CONNETING) || (1 == System_state.SmartConfig_OnOff_Flag))	 //当前APP 设置了连接wifi 不进行扫描
			{																									//如果开启了标准模式,不扫描

			}
			else if(System_state.wifi_onoff==1)
			{
				IOT_WIFIEvenSetSCAN();
			}
		}
		if((IOT_SYSRESTTime_uc(ESP_READ ,0 ) ==ESP_OK1) &&(  System_state.SYS_RestartFlag==1 ))  // 重启设备
		{
			ESP_LOGI(SYS, "\r\n\r\n\r\n\r\n\r\nIN IOT_SysTEM_Task IOT_SYSRESTART System_state.SYS_RestartFlag= %d !!!\r\n\r\n\r\n\r\n\r\n",System_state.SYS_RestartFlag);

			IOT_SYSRESTART();
		}
#if test_wifi
		if((IOT_BLEofftime_uc(ESP_READ,0)== ESP_OK1) && (System_state.ble_onoff==1 ))
		{
			ESP_LOGI(SYS, "BLE is being close!!!");
			IOT_GattsEvenOFFBLE();   		// 触发关闭蓝牙
		}
#endif
		if(InvCommCtrl.CMDFlagGrop & CMD_0X18  )
		{
		//	IOT_CollectorParam_Set();
			InvCommCtrl.CMDFlagGrop &= ~CMD_0X18;
			IOT_GattsEvenSetR();    				// 触发应答APP 18命令
		}
		else if(InvCommCtrl.CMDFlagGrop & CMD_0X19)   	// 服务器查询 0x19 数据  同步一些寄存器参数
		{
			switch(System_state.ServerGET_REGAddrStart)
			{
				case 31 :	//获取时间
					IOT_CollectorParam_Get(31,NULL);
					InvCommCtrl.CMDFlagGrop &= ~CMD_0X19;
					IOT_GattsEvenSetR();
					break;
				case 75 :
					//在 scan 处理
					break;

				default:
					InvCommCtrl.CMDFlagGrop &= ~CMD_0X19;
					IOT_GattsEvenSetR();    				// 触发应答APP 18命令

					break;
			}
		}
		if(InvCommCtrl.CMDFlagGrop & CMD_0X26)
		{
			InvCommCtrl.CMDFlagGrop &= ~CMD_0X26;
			/**  差分升级判断 执行位置未定,需要优化**/
			IOT_GattsEvenSetR();    				// 触发应答APP 18命令
		}
		if((IOT_IPPORTSet_us(ESP_READ ,0 ) ==ESP_OK1) && (uSetserverflag==1) && (System_state.Wifi_state==1 ) ) // 远程配置服务器
		{
			System_state.Server_Online = 0;		//服务器在线状态复位,用于连接新的服务器成功后保存服务器IP和端口

			IOT_Printf(" close old sever \r\n ");

			if((1 == System_state.Mbedtls_state) || (1 == System_state.SSL_state))
			{
				IOT_Connet_Close();
				System_state.Mbedtls_state = 0;
				System_state.Mbedtls_state = 0;

			}

			IOT_IPPORTSet_us(ESP_WRITE ,300); //5分钟算连接一次失败
			System_state.Mbedtls_state=0;

			IOT_SETConnect_uc(0xff);
		}

		if((IOT_TCPDelaytimeSet_uc(ESP_READ , 0)==ESP_OK1 )  && (System_state.Pv_state == 1 ) )
		{
#if		test_wifi
			IOT_TCPDelaytimeSet_uc(ESP_WRITE,System_state.Up04dateTime+1);
#else
			IOT_TCPDelaytimeSet_uc(ESP_WRITE,System_state.Up04dateTime);
#endif
			if((System_state.Mbedtls_state == 1) && (Mqtt_state.MQTT_connet == 1))
			{
				System_state.updatetimeflag = 1 ;
//				ESP_LOGI(SYS," System_state.updatetimeflag %d  \r\n",System_state.updatetimeflag);
			}
			else
			{
#if test_wifi
				uint16_t DataLen = 0;
			 	uint8_t *TxBuf = MQTT_DATAbuf;
				//Write_RecordItem(TxBuf);		//离线保存

				memcpy(TxBuf , InvCommCtrl.SN , 10);
				TxBuf[10] = InvCommCtrl.PVClock_Time_Tab[0]; 			//历史数据 用逆变器数据年，月日时分秒
				TxBuf[11] = InvCommCtrl.PVClock_Time_Tab[1];
				TxBuf[12] = InvCommCtrl.PVClock_Time_Tab[2];

				TxBuf[13] = InvCommCtrl.PVClock_Time_Tab[3];
				TxBuf[14] = InvCommCtrl.PVClock_Time_Tab[4];
				TxBuf[15] = InvCommCtrl.PVClock_Time_Tab[5];

				DataLen = InputGrop_DataPacket(&TxBuf[16]);
				memcpy(&TxBuf[16 + 28] , &TxBuf[10], 6);

				IOT_ESP_SPI_FLASH_ReUploadData_Write(TxBuf,  DataLen + 16);	//预留16字节用于另外保存逆变器序列号和时间

				ESP_LOGI(SYS,"保存离线数据 DataLen = %d \r\n" , DataLen);
#endif
			}
		}

		if((IOT_Historydate_us(ESP_READ,0)==ESP_OK1 )  && (System_state.Pv_state==1 ) && ( System_state.Server_Online==1  ) )
		{
			//if(Get_RecordNum()>0)
			if(IOT_Get_Record_Num() > 0)
			{
				System_state.upHistorytimeflag=1;
			//	IOT_Historydate_us(ESP_WRITE,10);  //10秒一笔历史数据
			}
			if(uAmmeter.Flag_GEThistorymeter>0)
			{
				IOT_Histotymeter_Timer();
			//IOT_Historydate_us(ESP_WRITE,10);  //10秒一笔历史数据
			}
			IOT_Historydate_us(ESP_WRITE,10);  //10秒一笔历史数据
		}
#if BLE_Readmode
		if(  System_state.BLE_ReadDone > 0)  // 写事件置位
	    {
	    	if(((  System_state.BLE_ReadDone ==1) &&( GetSystemTick_ms() - System_state.BLEread_waitTime )>100) ||
	    	  ((  System_state.BLE_ReadDone ==2) &&( GetSystemTick_ms() - System_state.BLEread_waitTime )>3000)
	    		)  //实际存在风险，接收超时实际有可能与蓝牙接收的距离，影响  ,可以加信号检测，超时自适应分包率会更好
	    	{
	    		 ESP_LOGE(SYS, "IOT_GattsEvenGATTS_WRITE waittime=%d, Get=%d" , System_state.BLEread_waitTime , GetSystemTick_ms());
	    		 System_state.BLE_ReadDone = 0;
	    		 IOT_GattsEvenGATTS_WRITE();
	    	}
	    }
#endif

		IOT_Update_Process_Judge();   //升级处理

		/* 标准配网事项 */
		if((IOT_GetTick() - uiTime > 10 * 1000) && (1 == System_state.wifi_onoff))	//每 10 s 查询一次
		{
			uiTime = IOT_GetTick();

			if(0 == System_state.ble_onoff)
			{
				if(0 == System_state.SmartConfig_OnOff_Flag )//若准配网模式关闭
				{
					//标准模式获取到wifi密码后  System_state.SmartConfig_OnOff_Flag = 0  但 wifi可能还没有配置连接 , 有风险需要优化
					wifi_config_t conf_t = {0};
					esp_wifi_get_config(ESP_IF_WIFI_STA , &conf_t);
					if(0x00 == conf_t.sta.ssid[0])	//若config 为空
					{
						System_state.SmartConfig_OnOff_Flag = 1;
						IOT_WIFIEven_StartSmartConfig();	//开启标准配网模式
					}else
					{
						ESP_LOGI(SYS, " 标准配网关闭 不开启标准配网模式\r\n");
					}
				}
			}else
			{
				if(1 == System_state.SmartConfig_OnOff_Flag )//若准配网模式已开启
				{
					ESP_LOGI(SYS, "蓝牙 开启 准备关闭标准配网模式\r\n");
					IOT_WIFIEven_CloseSmartConfig();
				}
			}
		}

		IOT_WIFI_SmartConfig_Close_Delay(0xff , 0);		//标准配网 延时 关闭


		if(ESP_OK1 == IOT_Heartbeat_Timeout_Check_uc(ESP_READ,0) && ( System_state.Server_Online == 1 ))
		{
			ESP_LOGI(SYS, "Heartbeat_Timeout ,Connet_Close !! \r\n");
			//System_state.Server_Online = 0;
			IOT_Connet_Close();
		}

		/*判断逆变器实时数据读取是否超时*/
		if((1 == System_state.SmartConfig_OnOff_Flag) \
				|| (1 == System_state.ble_onoff) || (EFOTA_NULL != IOT_MasterFotaStatus_SetAndGet_ui(0xff , EFOTA_NULL)))
		{
			//标准配网开启 , 蓝牙开启 , 正在升级等情况,需要重置超时时间,不触发重启
			IOT_Inverter_RTData_TimeOut_Check_uc(ESP_WRITE,INVERTER_RT_DATA_TIMEOUT);  //重置实时数据超时时间
			IOT_WiFiConnect_Server_Timeout_uc(ESP_WRITE , CONNECT_SERVER_TIMEOUT);
		}
		else if(ESP_OK1 == IOT_Inverter_RTData_TimeOut_Check_uc(ESP_READ, 0))
		{
			//IOT_SYSRESTTime_uc(ESP_WRITE , 10 );  // 10s 后重启
			System_state.SYS_RestartFlag = 1;
		}
		else if(ESP_OK1 == IOT_WiFiConnect_Server_Timeout_uc(ESP_READ ,0))
		{
			//IOT_SYSRESTTime_uc(ESP_WRITE , 10 );  // 10s 后重启
			System_state.SYS_RestartFlag = 1;
		}
		else
		{
			if(1 == System_state.Pv_state)
			{
				//标准配网开启 , 蓝牙开启 , 正在升级等情况,需要重置超时时间,不触发重启
				IOT_Inverter_RTData_TimeOut_Check_uc(ESP_WRITE,INVERTER_RT_DATA_TIMEOUT);  //重置实时数据超时时间
			}

			if(1 == System_state.Server_Online)
			{
				IOT_WiFiConnect_Server_Timeout_uc(ESP_WRITE , CONNECT_SERVER_TIMEOUT);
			}
		}

#if DEBUG_HEAPSIZE
		if(timer_logoc++>500)
		{
			timer_logoc=0;
			ESP_LOGI(SYS, " out IOT_SysTEM_Task uxTaskGetStackHighWaterMark= %d !!!\r\n", uxTaskGetStackHighWaterMark(NULL));
			ESP_LOGI(SYS, " main minimum free heap size = %d !!!\r\n", esp_get_minimum_free_heap_size());
			ESP_LOGI(SYS, " esp_get_free_heap_size size = %d !!!\r\n", esp_get_free_heap_size());
		}
#endif

		vTaskDelay(10 / portTICK_PERIOD_MS);
	}
}






