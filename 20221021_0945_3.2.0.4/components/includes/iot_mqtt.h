/*
 * iot_mqtt.h
 *
 *  Created on: 2021��12��6��
 *      Author: Administrator
 */

#ifndef COMPONENTS_INCLUDES_IOT_MQTT_H_
#define COMPONENTS_INCLUDES_IOT_MQTT_H_
#include <stdint.h>
#include "iot_system.h"
//
//#define Mqtt_PINGTime     60*1    	// MQTT ����ʱ�� ��λ��
//#define Mqtt_DATATime     61 	    // MQTT �ϱ�040 ���� ʱ�� ��λ��
//#define Mqtt_03DATATime   3
//#define	Mqtt_AgainTime	  3			// �ش�����ʱ�� ��λ��
//#define	Mqtt_Again_PASS	  0xFF	    // �ش�ȡ��
//
//#define TCP_DATABUF_LEN 1024
//#define MQTT_PINGTIME     0x0120	// 0x01A4  // MQTT ������ʱ��(7����)(��λ����) ���÷������Ĳ���  ����10 S
//#define MQTT_IDENTIFIER   0x000A  // MQTT Э���屨�ı�ʶ��
//#define MQTT_QoS       1          // MQTT ���������ȼ� 0/1/2  ע��QoS=0ʱ��server��Ӧ��/QoS=2ʱ����ҪӦ���ȷ��2�αȽ��鷳��

#define Mqtt_PINGTime     60*1    	// MQTT ����ʱ�� ��λ��
#define Mqtt_DATATime     5		    // MQTT �ϱ�040 ���� ʱ�� ��λ��  -> System_state.Up04dateTime
#define Mqtt_03DATATime   3
#define	Mqtt_AgainTime	  3			// �ش�����ʱ�� ��λ��
#define	Mqtt_Again_PASS	  0xFF	    // �ش�ȡ��
#define TCP_DATABUF_LEN   1024

#if test_wifi
#define MQTT_PINGTIME     0x01A4      // 0x01A4  // MQTT ������ʱ��(7����)(��λ����) ���÷������Ĳ���  ����10 S
#else
#define MQTT_PINGTIME     0x03C       // 0x01A4  // MQTT ������ʱ�� ��Яʽ��Դ 60s
#endif

#define MQTT_IDENTIFIER   0x000A    // MQTT Э���屨�ı�ʶ��
#define MQTT_QoS       1            // MQTT ���������ȼ� 0/1/2  ע��QoS=0ʱ��server��Ӧ��/QoS=2ʱ����ҪӦ���ȷ��2�αȽ��鷳��



//#define Mqtt_PINGTime     60*1     // MQTT ����ʱ�� ��λ��
//#define Mqtt_DATATime     5		 // MQTT �ϱ�040 ���� ʱ�� ��λ��
//#define Mqtt_03DATATime   3
//#define	Mqtt_AgainTime	  3		 // �ش�����ʱ�� ��λ��
//#define	Mqtt_Again_PASS	  0xFF	 // �ش�ȡ��
//
//#define TCP_DATABUF_LEN 1024
//#define MQTT_PINGTIME     0x0020	// 0x01A4  // MQTT ������ʱ��(7����)(��λ����) ���÷������Ĳ���  ����10 S
//#define MQTT_IDENTIFIER   0x000A  // MQTT Э���屨�ı�ʶ��
//#define MQTT_QoS       1        // MQTT ���������ȼ� 0/1/2  ע��QoS=0ʱ��server��Ӧ��/QoS=2ʱ����ҪӦ���ȷ��2�αȽ��鷳��



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
    MQTTREQ_NULL = 0,          // ������
	MQTTREQ_SET_TIME = 1,      // ����ϵͳʱ��  --- �����ϱ�
	MQTTREQ_CMD_0x05 = 2, 	   // ����05������ --- �����ϱ�
	MQTTREQ_CMD_0x06 = 3,      // ����06������ --- �����ϱ�
	MQTTREQ_CMD_0x10 = 4,      // ����10������ --- �����ϱ�
	MQTTREQ_CMD_0x18 = 5,      // ����18������ --- �����ϱ�   ������ ���� �ɼ���������ָ��
	MQTTREQ_CMD_0x19 = 6,      // ����19������ --- �����ϱ�   ������ ��ѯ �ɼ���������ָ��

	MQTTREQ_SET_STATIONID = 7, // ���õ�վID  --- �����ϱ�
	MQTTREQ_SET_ONLINEDATA = 8,// �������ϴ�һ��04����  --- �����ϱ�
	MQTTREQ_GET_ININFO = 9,    // ��ȡ�������Ϣ  --- �����ϱ�
    MQTTREQ_GET_DLINFO = 10,   // ��ȡ�ɼ�����Ϣ  --- �����ϱ�
	MQTTREQ_SET_INVREG = 11,   // ����������Ĵ�������  --- �����ϱ�
	MQTTREQ_GET_INVREG = 12,  // ��ȡ������Ĵ�������  --- �����ϱ�
	MQTTREQ_CMD_0x17 = 13,    // ����17������ --- ͸�� �����
	MQTTREQ_CMD_0x26 =14      // ����26������ --- ����
}EMQTTREQ;

typedef enum _EMSG{
  	EMSG_NULL = 0,                // ������
	EMSG_ONLINE_POST = 1,         // ʵʱ����  --- �Զ��ϱ� 0x04
	EMSG_OFFLINE_POST = 2,        // ��������  --- �Զ��ϱ� 0x50
	EMSG_FAULTE_WARNING_POST = 3, // ���Ϻ͸澯��Ϣ  --- �Զ��ϱ�
	EMSG_INVPARAMS_POST = 4,      // ��������� --- �Զ��ϱ� 0x03
	EMSG_LOCALTIME_POST = 5,      // ����ʱ�� --- �Զ��ϱ�
	EMSG_SYSTEM_POST = 6,   	  // �ɼ�������  --- �Զ��ϱ� 0x19
	EMSG_GET_INVINFO_RESPONSE = 7,// �����������ȡӦ��  --- �����ϱ�
	EMSG_GET_DLINFO_RESPONSE = 8, // �ɼ���������ȡӦ��  --- �����ϱ�
	EMSG_GET_INVREG_RESPONSE = 9, // �����������ȡ����Ӧ��  --- �����ϱ�
	EMSG_GET_DLIREG_RESPONSE = 10,// �ɼ���������ȡ����Ӧ��  --- �����ϱ�
	EMSG_PROGRESS_POST =11,		  // ���������ϴ�
	EMSG_AMMTER_POST =12,		  // ��������ϴ� --- �Զ��ϱ� 0x20
	EMSG_HISTORY_AMMTER_POST =13, // �����ʷ�����ϴ� --- �Զ��ϱ� 0x22
	EMSG_INVWAVE_POST =14,		  // һ�����  0x14
	EMSG_BDC03_POST =15, 		  // BDC���� 0x37
	EMSG_BDC04_POST =16, 		  // BDC���� 0x38
	EMSG_BMS03_POST =17, 		  // BMS���� 0x37
	EMSG_BMS04_POST =18, 		  // BMS���� 0x38


}EMSG;


typedef struct  _IOTmqtttem{

	uint8_t MQTT_connet;			//MQTT ��¼�ɹ�
	uint8_t MqttSend_step;			//��������MQTT���� ���� ,��������Щ

	uint8_t Reserve1;				//����
	uint8_t Reserve2;				//����
	uint32_t MqttDATA_Time;			//���ͷ�����04������ʱ
	uint32_t MqttPING_Time; 		//MQTT����ʱ��
	uint32_t Inverter_CMDTime;      //��ѯ�������ѵ03 04 ʱ����ʱ

	uint32_t AgainTime;				//��ѵ�ط�ʱ��
	uint32_t RestartTime;			//�豸����ʱ��
	uint32_t WifiScan_Time;			//WIFI ���߶�ʱɨ��
	uint32_t WifiConnect_Timeout;			//WIFI ���߶�ʱɨ��
	uint32_t WifiConnect_Sever_Timeout;			//WIFI ���߶�ʱɨ��
	uint32_t IPPORTset_Time;		//IP ��������ʱ��
	uint32_t InverterGetSN_Time;	//��ȡ��������к�ʱ��
	uint32_t BLEOFF_Time;			//BLE �����ӹرյȴ�ʱ��
	uint32_t Historydate_Time;		//���ͷ�������ʷ���� ʱ��
	uint32_t Heartbeat_Timeout;    //��������ⳬʱʱ��
	uint32_t InverterRTData_TimeOut;
	uint32_t UpgradeDelay_Time;     //������ʱ

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

extern EMQTTREQ _g_EMQTTREQType;       // ƽ̨TCP������������
extern EMSG _g_EMSGMQTTSendTCPType;    // ƽ̨TCP�����������ͣ�ʵʱ����/������/����/��ȡ...��

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



