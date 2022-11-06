/*
 * iot_mqtt.h
 *
 *  Created on: 2021年12月6日
 *      Author: Administrator
 */

#ifndef COMPONENTS_INCLUDES_IOT_MQTT_H_
#define COMPONENTS_INCLUDES_IOT_MQTT_H_
#include <stdint.h>
#include "iot_system.h"
//
//#define Mqtt_PINGTime     60*1    	// MQTT 心跳时间 单位秒
//#define Mqtt_DATATime     61 	    // MQTT 上报040 数据 时间 单位秒
//#define Mqtt_03DATATime   3
//#define	Mqtt_AgainTime	  3			// 重传数据时间 单位秒
//#define	Mqtt_Again_PASS	  0xFF	    // 重传取消
//
//#define TCP_DATABUF_LEN 1024
//#define MQTT_PINGTIME     0x0120	// 0x01A4  // MQTT 心跳包时间(7分钟)(单位：秒) 设置服务器的参数  更新10 S
//#define MQTT_IDENTIFIER   0x000A  // MQTT 协议体报文标识符
//#define MQTT_QoS       1          // MQTT 服务质量等级 0/1/2  注：QoS=0时，server不应答/QoS=2时，需要应答和确认2次比较麻烦；

#define Mqtt_PINGTime     60*1    	// MQTT 心跳时间 单位秒
#define Mqtt_DATATime     5		    // MQTT 上报040 数据 时间 单位秒  -> System_state.Up04dateTime
#define Mqtt_03DATATime   3
#define	Mqtt_AgainTime	  3			// 重传数据时间 单位秒
#define	Mqtt_Again_PASS	  0xFF	    // 重传取消
#define TCP_DATABUF_LEN   1024

#if test_wifi
#define MQTT_PINGTIME     0x01A4      // 0x01A4  // MQTT 心跳包时间(7分钟)(单位：秒) 设置服务器的参数  更新10 S
#else
#define MQTT_PINGTIME     0x03C       // 0x01A4  // MQTT 心跳包时间 便携式电源 60s
#endif

#define MQTT_IDENTIFIER   0x000A    // MQTT 协议体报文标识符
#define MQTT_QoS       1            // MQTT 服务质量等级 0/1/2  注：QoS=0时，server不应答/QoS=2时，需要应答和确认2次比较麻烦；



//#define Mqtt_PINGTime     60*1     // MQTT 心跳时间 单位秒
//#define Mqtt_DATATime     5		 // MQTT 上报040 数据 时间 单位秒
//#define Mqtt_03DATATime   3
//#define	Mqtt_AgainTime	  3		 // 重传数据时间 单位秒
//#define	Mqtt_Again_PASS	  0xFF	 // 重传取消
//
//#define TCP_DATABUF_LEN 1024
//#define MQTT_PINGTIME     0x0020	// 0x01A4  // MQTT 心跳包时间(7分钟)(单位：秒) 设置服务器的参数  更新10 S
//#define MQTT_IDENTIFIER   0x000A  // MQTT 协议体报文标识符
//#define MQTT_QoS       1        // MQTT 服务质量等级 0/1/2  注：QoS=0时，server不应答/QoS=2时，需要应答和确认2次比较麻烦；



typedef enum _MQTTSendStep {
    MQTT_CONNECT=1u,
    MQTT_CONNACK=2u,
    MQTT_PUBLISH=3u,
    MQTT_PUBACK=4u,
    MQTT_PUBREC=5u,
    MQTT_PUBREL=6u,
    MQTT_PUBCOMP=7u,
    MQTT_SUBSCRIBE=8u,
    MQTT_SUBACK=9u,
    MQTT_UNSUBSCRIBE=10u,
    MQTT_UNSUBACK=11u,
    MQTT_PINGREQ=12u,
    MQTT_PINGRESP=13u,
    MQTT_DISCONNECT=14u
}MQTTStep;

typedef enum _EMQTTREQ{
    MQTTREQ_NULL = 0,          // 空类型
	MQTTREQ_SET_TIME = 1,      // 设置系统时间  --- 被动上报
	MQTTREQ_CMD_0x05 = 2, 	   // 设置05命令码 --- 被动上报
	MQTTREQ_CMD_0x06 = 3,      // 设置06命令码 --- 被动上报
	MQTTREQ_CMD_0x10 = 4,      // 设置10命令码 --- 被动上报
	MQTTREQ_CMD_0x18 = 5,      // 设置18命令码 --- 被动上报   服务器 设置 采集器参数的指令
	MQTTREQ_CMD_0x19 = 6,      // 设置19命令码 --- 被动上报   服务器 查询 采集器参数的指令

	MQTTREQ_SET_STATIONID = 7, // 设置电站ID  --- 被动上报
	MQTTREQ_SET_ONLINEDATA = 8,// 设置马上传一笔04数据  --- 被动上报
	MQTTREQ_GET_ININFO = 9,    // 获取逆变器信息  --- 被动上报
    MQTTREQ_GET_DLINFO = 10,   // 获取采集器信息  --- 被动上报
	MQTTREQ_SET_INVREG = 11,   // 设置逆变器寄存器数据  --- 被动上报
	MQTTREQ_GET_INVREG = 12,  // 获取逆变器寄存器数据  --- 被动上报
	MQTTREQ_CMD_0x17 = 13,    // 设置17命令码 --- 透传 逆变器
	MQTTREQ_CMD_0x26 =14      // 设置26命令码 --- 升级
}EMQTTREQ;

typedef enum _EMSG{
  	EMSG_NULL = 0,                // 空类型
	EMSG_ONLINE_POST = 1,         // 实时数据  --- 自动上报 0x04
	EMSG_OFFLINE_POST = 2,        // 续传数据  --- 自动上报 0x50
	EMSG_FAULTE_WARNING_POST = 3, // 故障和告警信息  --- 自动上报
	EMSG_INVPARAMS_POST = 4,      // 逆变器参数 --- 自动上报 0x03
	EMSG_LOCALTIME_POST = 5,      // 本地时间 --- 自动上报
	EMSG_SYSTEM_POST = 6,   	  // 采集器参数  --- 自动上报 0x19
	EMSG_GET_INVINFO_RESPONSE = 7,// 逆变器参数获取应答  --- 被动上报
	EMSG_GET_DLINFO_RESPONSE = 8, // 采集器参数获取应答  --- 被动上报
	EMSG_GET_INVREG_RESPONSE = 9, // 逆变器参数获取设置应答  --- 被动上报
	EMSG_GET_DLIREG_RESPONSE = 10,// 采集器参数获取设置应答  --- 被动上报
	EMSG_PROGRESS_POST =11,		  // 升级进度上传
	EMSG_AMMTER_POST =12,		  // 电表数据上传 --- 自动上报 0x20
	EMSG_HISTORY_AMMTER_POST =13, // 电表历史数据上传 --- 自动上报 0x22
	EMSG_INVWAVE_POST =14,		  // 一键诊断  0x14
	EMSG_BDC03_POST =15, 		  // BDC数据 0x37
	EMSG_BDC04_POST =16, 		  // BDC数据 0x38
	EMSG_BMS03_POST =17, 		  // BMS数据 0x37
	EMSG_BMS04_POST =18, 		  // BMS数据 0x38


}EMSG;


typedef struct  _IOTmqtttem{

	uint8_t MQTT_connet;			//MQTT 登录成功
	uint8_t MqttSend_step;			//发送数据MQTT类型 步骤 ,步骤有哪些

	uint8_t Reserve1;				//保留
	uint8_t Reserve2;				//保留
	uint32_t MqttDATA_Time;			//发送服务器04数据延时
	uint32_t MqttPING_Time; 		//MQTT心跳时间
	uint32_t Inverter_CMDTime;      //查询逆变器轮训03 04 时间延时

	uint32_t AgainTime;				//轮训重发时间
	uint32_t RestartTime;			//设备重启时间
	uint32_t WifiScan_Time;			//WIFI 离线定时扫描
	uint32_t WifiConnect_Timeout;			//WIFI 离线定时扫描
	uint32_t WifiConnect_Sever_Timeout;			//WIFI 离线定时扫描
	uint32_t IPPORTset_Time;		//IP 设置重连时间
	uint32_t InverterGetSN_Time;	//获取逆变器序列号时间
	uint32_t BLEOFF_Time;			//BLE 无连接关闭等待时间
	uint32_t Historydate_Time;		//发送服务器历史数据 时间
	uint32_t Heartbeat_Timeout;    //心跳包检测超时时间
	uint32_t InverterRTData_TimeOut;
	uint32_t UpgradeDelay_Time;     //升级延时

} MQTT_STATE;

typedef enum _RW_flag{
	ESP_OK1    = 0,
	ESP_FAIL1,
    ESP_READ,
    ESP_WRITE,
	ESP_READY,
	ESP_ONREADY
}RW_flag;

extern RW_flag ESP_RW;
extern MQTT_STATE Mqtt_state;

extern EMQTTREQ _g_EMQTTREQType;       // 平台TCP请求数据类型
extern EMSG _g_EMSGMQTTSendTCPType;    // 平台TCP发送数据类型（实时数据/故障码/设置/获取...）

//unsigned int IOT_MqttDataPack_ui(unsigned char ucSend_Type,  unsigned char *pucDataBuf_tem);
unsigned int IOT_Mqtt_Test_ui(unsigned char ucType_tem, unsigned char *pucDataBuf_tem);


uint8_t IOT_ReadyForData(void);

void IOT_MqttSendStep_Set(uint8_t ucMode_tem, uint8_t ucStep_tem);

uint8_t IOT_MqttRev_ui(unsigned char *dst, uint16_t suiLen_tem);
unsigned int  IOT_MqttDataPack_ui(unsigned char ucSend_Type,   unsigned char *pucDataBuf_out, unsigned char * pSendTXbuf_in, uint16_t SendTXLen );

uint8_t IOT_MQTTPublishReq_uc(uint8_t *pucDst_tem, uint16_t suiLen_tem, uint8_t uQoc);



void IOT_SetUpHolddate(void);

uint8_t IOT_TCPDelaytimeSet_uc(uint8_t ucMode_tem, uint32_t uiSetTime_tem);
uint8_t IOT_InCMDTime_uc(uint8_t ucMode_tem, uint32_t uiSetTime_tem);
uint8_t IOT_PINGRESPSet_uc(uint8_t ucMode_tem, uint32_t uiSetTime_tem);
uint8_t IOT_MQTTPublishACK_uc(uint8_t *pucDst_tem, uint16_t suiLen_tem);
uint8_t IOT_WiFiScan_uc(uint8_t ucMode_tem, uint32_t uiSetTime_tem);
uint8_t IOT_IPPORTSet_us(uint8_t ucMode_tem, uint32_t uiSetTime_tem);
uint8_t IOT_SYSRESTTime_uc(uint8_t ucMode_tem, uint32_t uiSetTime_tem);
uint8_t IOT_INVgetSNtime_uc(uint8_t ucMode_tem, uint32_t uiSetTime_tem);
uint8_t IOT_BLEofftime_uc(uint8_t ucMode_tem, uint32_t uiSetTime_tem);
uint8_t IOT_Historydate_us(uint8_t ucMode_tem, uint32_t uiSetTime_tem);

uint8_t IOT_Heartbeat_Timeout_Check_uc(uint8_t ucMode_tem, uint32_t uiSetTime_tem);
uint8_t IOT_WiFiConnectTimeout_uc(uint8_t ucMode_tem, uint32_t uiSetTime_tem);
uint8_t IOT_Inverter_RTData_TimeOut_Check_uc(uint8_t ucMode_tem, uint32_t uiSetTime_tem);
uint8_t IOT_UpgradeDelayTime_uc(uint8_t ucMode_tem, uint32_t uiSetTime_tem);
uint8_t IOT_WiFiConnect_Server_Timeout_uc(uint8_t ucMode_tem, uint32_t uiSetTime_tem);


#endif /* COMPONENTS_INCLUDES_IOT_MQTT_H_ */



