/*
 * iot_system.h
 *
 *  Created on: 2021��12��1��
 *      Author: Administrator
 */
#ifndef COMPONENTS_INCLUDES_IOT_SYSTEM_H_
#define COMPONENTS_INCLUDES_IOT_SYSTEM_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define Online   1
#define Offline  0

#define DEBUG_HEAPSIZE   0
#define DEBUG_UART		 0
#define DEBUG_BLE		 1
#define DEBUG_INV        0
#define DEBUG_TEST 		 0
#define DEBUG_MQTT		 1
#define test_wifi	   	 1     //0 ��Яʽ  //1 wifi-X2
//#define Kennedy_System
//#define Kennedy_main


#define DEBUG_EN         1    //��ӡ���ݿ���

#if DEBUG_EN

#define IOT_Printf(format, ... )   do { printf(format, ##__VA_ARGS__);}while(0);

#else
#define IOT_Printf(format, ... )   do { (void)0; }while(0);
#endif

#define MIN(a,b) ((a) < (b) ? (a) : (b))

#if test_wifi
#define LOCAL_PROTOCOL_6_EN   1     //����ͨѶЭ�� 6.0 ʹ��
#else
#define LOCAL_PROTOCOL_6_EN   0     //����ͨѶЭ�� 6.0 ʧ��
#endif
/* enum definitions */
typedef enum {DISABLE = 0, ENABLE = !DISABLE} EventStatus,ControlStatus ;


#define USER_DATA_ADDR   288		//�û��Զ�������ƫ��λ�� sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t)
#define USER_DATA_LEN    20		    //�û��Զ������ݳ���



#if test_wifi
#define DATALOG_TYPE "34"          // 13�żĴ������ɼ����豸����  11 wifi�ɼ���  33 ��Яʽ��Դ 34 wifi-x2
#define DATALOG_SVERSION "3.2.0.4" // 21�żĴ������ɼ�������汾
#else
#define DATALOG_TYPE "33"          // 13�żĴ������ɼ����豸����  11 wifi�ɼ���  33 ��Яʽ��Դ  34 wifi-x2
#define DATALOG_SVERSION "3.3.0.1" // 21�żĴ������ɼ�������汾
#endif


#define AES_KEY_128BIT  "growatt_aes16key"

/***************************************************************
  *     ���ݴ洢��ַ��غ궨��
****************************************************************/
/* ��0���� */
#define RW_SIGN_ADDR		0x00				//��д��ʾ�����ڵ�һ������������ĳ�ʼ������
#define RW_SIGN_ADDR1		0x0A				//��д��ʾ1, �Ƿ��ʼ��70��֮��Ĳ���
#define RW_SIGN_ADDR2		0x14				//��д��ʾ1, �Ƿ��ʼ��81��֮��Ĳ���
#define RESEAVE_ADDR		0x100				//Ԥ��

/* ��1���� */
#define REG_LEN_LIST_ADDR	0x1000				//256byte,�ɼ������Ĵ������ݳ��ȼ�¼��������¼ÿ���Ĵ�����ֵ�ĳ���
#define CONFIG_ADDR			0x1100				//3800byte�ɼ������Ĵ������ݴ洢
#define CONFIG_CRC16_ADDR	0x1FF1				//CRCУ��洢��ַ,У��ǰ3800byte

/* ��2���� */
#define BACKUP_CONFIG_ADDR	0x2000				//��Ϊ��һ�����ı�������

/* ��3���� */
#define RESEAVE_ADDR1		0x3000				//������ʾ��
#define RECORD_FULL			0x3000				//��¼���Ƿ�����,һ���ֽ�
#define FULL				0xaa				//����ʾ��

/* ��4���� */
#define DATA_ADDR			0x4000				//�������ݱ�����4080KBYTE
/* ��1028���� */
#define IAP_CACHE_ADDR		0x400000			//IAP���ݻ�����,4Mbyte��


/* �²����������ļ�������Ϣ��¼���Ĵ洢��ַ*/
#define SaveNewParamsStart_Addr		0x606000	 // ռ��һ������4096 = 4k

#define APPCRC32_ADDR	 		0x7FF000		 //APP�ļ�CRC32��ŵ�ַ
#define APPLENGTH_ADDR 		APPCRC32_ADDR+128	 //bootִ��ѡ�񿪹ش�ŵ�ַ


 /****************************************************************
  *     ���ݴ洢��غ궨��
  ***************************************************************
  */

#define REG_LEN				50U							    //�ɼ���ÿ���������ȣ�Ԥ��50byte
#define REGADDR(x)			(CONFIG_ADDR + (REG_LEN * x))	//�Ĵ�����ַ����
#define REG_NUM_MAX			70U								//�ɼ����Ĵ�������
#define BACKREGADDR(x)		BACKUP_CONFIG_ADDR + 0x100U + (REG_LEN * x)		//�Ĵ�����ַ����
#define PARAMETER_LENLIST_LEN	(CONFIG_ADDR - REG_LEN_LIST_ADDR)

#define DATA_SECTOR_MAX		1024			//���ݴ洢��������
#define DATA_SECTOR_START	4				//���ݴ洢����ʼ����
#define DATA_SECTOR_END		1027			//���ݴ洢����������

#define ITEM_SECTOR_MAX		16				//ÿ��233��ֱ��ʹ��һ��ҳ	4096/255

#define CONFIG_CRC16_NUM	4056								//CRC16ҪУ������ݳ���
#define CONFIG_LEN_MAX		REG_LEN*REG_NUM_MAX					//�����ܳ�
#define CONFIG_CRC16_BUFADDR	4081			//USART1_RX_BUF 4096byte�����У�CRCУ���λ��
#define CONFIG_LEN_BUFADDR		0				//USART1_RX_BUF 4096byte�����У�������������λ��
#define CONFIG_BUFADDR			0x100			//USART1_RX_BUF 4096byte�����У�������λ��


/****************************************************************
  *     �洢���ݼ�¼��غ궨��
  ***************************************************************
  */
#define	RECORD_SECTOR_ST			4U
#define	RECORD_SECTOR_END			1024U
#define RECORD_ADDR					RECORD_SECTOR_ST * SECTOR_SIZE
#define RECORD_ITEM_SIZE			256U				//ռ�ô�С
#define RECORD_ITEM_SIZE_TRUE		223U					//ʵ��ʹ�ô�С
#define RECORD_SECTOR_MAX_NUM		16U					//һ��������ߴ洢����

#define RECORD_AMOUNT_NUM			16000U			//��ߴ洢����
#define	RE_SECTOR_MAX	   			(RECORD_AMOUNT_NUM/RECORD_SECTOR_MAX_NUM)


/* ����ÿҳ����ı��λ */
#define RECORD_SVID_H				(223U)						//�洢ID��λ
#define RECORD_SVID_L				(224U)						//�洢ID��λ
#define RECORD_CRC_H				(225U)						//����CRC16��λ
#define RECORD_CRC_L				(226U)						//����CRC16��λ
#define RECORD_STA_SINGLE			(227U)						//���ʴ洢״̬,�Ƿ��Ѷ���д
#define RECORD_PACK_ALL				(228U)						//��ҳ�洢�ܰ�
#define RECORD_PACK_CUR				(229U)						//��ҳ�洢��ǰ��
#define RECORD_LAST_LEN				(230U)						//���һҳ����

#define RECORD_SEC_STA				255						//����������Ƿ��Ѷ���д,Ҳ��ռ����ÿҳʣ��ļ����ֽ��е����һ��

//flashδд��Ϊ0xff��ͬһ��ַ������д��Ϊ0��(�����ϵ�)
#define SEC_FULL	0x5AU			//��������־
#define SEC_NULL	0xFFU			//�����ձ�־
#define SEC_READ	0x00U			//�����Ѷ���־

#define PAGE_WR		0x5AU			//ҳ��д��־
#define PAGE_NULL	0xFFU			//ҳ�ѿձ�־
#define PAGE_RD		0x00U			//ҳ�Ѷ���־

#define PAGE_SIGN_ADDR	10U				//ҳ��д��־����ÿҳ������ʮ����¼
#define SEC_SIGN_ADDR	1U			//������д��־��������������1����¼

//�߼��ϵ�
#define STATUS_SEC_FULL	0x5AU			//��������־
#define STATUS_SEC_NULL	0xFFU			//�����ձ�־
#define STATUS_SEC_READ	0xDDU			//�����Ѷ���־

#define SAVANUM_START	1111U
#define SAVANUM_END		61111U

/*stationģʽ�����ӵ�WiFi����(�ȵ�����/�ȵ�����) */
#pragma pack(4)
typedef struct  _IOTsystem{
	uint8_t SysWorkMode;			 //�豸��ѵ���ݹ���ģʽ�� ������ģʽ   ����ģʽ
	uint8_t BLEKeysflag;		     //������ԿУ���־λ
	uint8_t BLEBindingGg;			 //�������豸G 1 �� ;g 0δ��
	uint8_t Pv_state;				 //�����ͨѶ

	volatile uint8_t Pv_Senderrflag; //�����������쳣��־
	uint8_t UploadParam_Flag;   	 //�����ϱ�0x19 TAB����
	uint8_t Wifi_state;		    	 //wifi����״̬
	uint8_t Ble_state;		    	 //BLE����״̬

	uint8_t Mbedtls_state;			 //TCP����״̬	 //MQTT����״̬  ��mqtt.h
	uint8_t SSL_state;				 //SSL ��ʼ���������״̬
	uint8_t Server_Online;			 //������ͨ����������״̬
	uint8_t ServerACK_flag;			 //������Ӧ���־
//	uint8_t ServerACK_DeathTime;	 //������Ӧ������ʱ��
	uint8_t ParamSET_ACKCode;		 //���������� Ӧ��״̬
	uint8_t SYS_RestartFlag;		 //�ɼ�������״̬λ
	uint8_t up14dateflag;			//������������ϴ���־λ
	uint8_t up00dateflah;			//��������

	uint8_t upBMS03dateflag;		//BMS������ 03�����ϴ�
	uint8_t upBMS04dateflag;		//BMS������ 04�����ϴ�
	uint8_t upBDC03dateflag;		//BDC���� 03�����ϴ�
	uint8_t upBDC04dateflag;		//BDC���� 04�����ϴ�

	uint8_t Get_TabDataBUF[300];	 //Ҫ���͵�����
	uint8_t updatetimeflag;			 //��ʱ�� �ϴ�04����  ��־λ
    uint8_t upHistorytimeflag;		 // ��ʱ���ϴ� 0x50 ��ʷ���� ��־λ
	uint8_t wifi_onoff;						//����WIFI
	uint8_t ble_onoff;						//����BLE
	uint8_t BLE_TimeoutOFFflag;				//�ر�BLE ��ʱ��־���Զ��ر�
	uint8_t BLE_ReadDone;                   //������ɱ�־;
	uint8_t SmartConfig_OnOff_Flag;

	uint16_t ServerGET_REGNUM;		 		//��������ѯ�ɼ����Ĵ�������
	uint16_t ServerGET_REGAddrStart ; 		//��������ѯ�ɼ����Ĵ�����ַ��ʼ
	uint16_t ServerGET_REGAddrEnd ;	 		//��������ѯ�ɼ����Ĵ�����ַ����
	uint16_t ServerGET_REGAddrBUFF[30]; 	//��������ѯ�ɼ����Ĵ���������ȡ

	uint16_t ServerSET_REGNUM;				//��������ѯ�ɼ����Ĵ�������
	uint16_t ServerSET_REGAddrStart; 		//���������òɼ����Ĵ�����ַ��ʼ
	uint16_t ServerSET_REGAddrLen;   		//���������òɼ����Ĵ������ݳ���

	uint16_t CID ;					 		//ͨѶ���
	uint16_t bCID_0x04  ;			 		//04ָ��ͨѶ��Ż���
    uint16_t Up04dateTime ;			 		//�ɼ����ϴ�04���ʱ��
    uint16_t Up03dateTime ;			 		//�ɼ����ϴ�03���ʱ��

	uint16_t Parameter_length ;		 		//�����б�ʵ���ܳ���
	uint16_t blemtu_value;					//BLE MTU ��С
	uint16_t Wifi_ActiveReset;				//�ֶ���������wifi
	uint16_t WIFI_BLEdisplay;				//֪ͨ��ʾ��Ļ��״̬	��λ0-3 ���� bit0  ��1/��0   bit1  �����豸1/�Ͽ��豸0��λ4-7 wifi bit4  ��1/��0   bit5  �����豸1/�Ͽ��豸0

	unsigned int  uiPackageLen ;         	//����������
	unsigned int  uiSumPackageNums ;     	//����������
	unsigned int  uiCurrentPackageNums ; 	//��ǰ���������
	uint32_t Host_setflag;			 		//�������ò���
	uint32_t SoftClockTick;					//���ʱ��  ��
	uint32_t BLEread_waitTime;				//�ȵȽ��ճ�ʱ
	uint32_t RSAEDSS;

	uint32_t uiSystemInitStatus;

} SYStem_state;


/***************************************
 * ��Яʽ��ʾʹ��
 * */
#define Displayflag_BLEON       (uint16_t)0x0001
#define Displayflag_BLECON		(uint16_t)0x0002
#define Displayflag_SYSreset	(uint16_t)0x0004
#define Displayflag_WIFION	 	(uint16_t)0x0010
#define Displayflag_WIFICON		(uint16_t)0x0020

#define   Serverflag_NULL  		0x00    // NULL ״̬
#define   Serverflag_disable    0x01      // ��ֹ����IP�Ͷ˿�
#define   Serverflag_enable     0x02      // ��������IP�Ͷ˿ڣ�������Ҫ�������ӳɹ�����ܱ���
#define   Serverflag_savetest   0x03      // �������������úͱ���IP�Ͷ˿ڣ���Ҫ��������������
#define   Serverflag_backoldip   	0x04    // �����������ӵľ�IP�Ͷ˿�
#define   Serverflag_savenowip   	0x05    // �������������úͱ���IP�Ͷ˿ڣ���Ҫ��������IP�ɹ���������󱣴���
#define   Serverflag_GETflag   	0xff    // ��ȡ���ò���״̬

//typedef struct _TAB{
//char Language[4];
//char Protocol_Type[4];
//char Protocol_Version[4];
//char Backlight_Delay[4];
//char Data_Interval[4];
//char Inverter_RangeStart[4];
//char Inverter_RangeEnd[4];
//char Reservedx1[4];
//char DataloggerSN[50];
//char RollDelayTime[4];
//char UpdateDataloggerFirmware[4];
//}SysConfig;

enum SYSRegister{

	REG_language = 0,  	 	 //0     �ɼ�����������  0:Ӣ�ģ�1:����
	REG_ProtocolType,        //1     ���ݲɼ�����Э������,0��MODBUS_TCP,1��Internal
	REG_ProtocolVersion,     //2     ����Э�顱������Э�飩�İ汾��
	REG_BacklitDelay,        //3     ShinePano������ʱ
	REG_Interval,        	 //4     ���ݲɼ���������������������ݵļ��ʱ�䣬��λΪ����
	REG_StartAddress,  	     //5     ���ݲɼ��������������ʼ������ַ
	REG_EndAddress,     	 //6     ���ݲɼ�����������Ľ���������ַ
	REG_Reserved,  	     	 //7     Ԥ������,�ڲ���ʹ�ã�����������;
	REG_SN,  	     		 //8     ���ݲɼ������к�
	REG_PollingTime,  	     //9     �������ѯʱ����ʱ
	REG_Updated,  	     	 //10    �����ݲɼ����Ĺ̼����и��µı�ʶ,0����������,1�����¹̼�
	REG_FTP,  	     	     //11    wifiģ���޷�ʹ��FTP,�������ݲɼ�����������̼�������豸�̼����µ�FTP���ã����û��������롢IP���˿ڣ����������þ��ŷָ��������ݸ�ʽΪ��UserName#Password#IP#Port#
	REG_DNS,  	     		 //12    ���ݲɼ�����DNS����
	REG_CollectorType,  	 //13    ���ݲɼ�������
	REG_ClientIP,  	     	 //14    ���ݲɼ����ı���IP��ַ,Ϊ��Server����ʾ
	REG_ClientPORT,  	     //15    ���ݲɼ����ı��˶˿�
	REG_WIFIMAC,  	     		 //16    ���ݲɼ������뻥����������MAC��ַ  WiFi mac
	REG_ServerIP,  	     	 //17    ���ݲɼ�����Զ�ˣ������������IP��ַ
	REG_ServerPORT,		 	 //18    ���ݲɼ�����Զ�ˣ�������������˿�
	REG_ServerURL, 		 	 //19    ���ݲɼ�����Զ�ˣ����������������

	REG_GTSW, 				 //20    ���ݲɼ����İ�������
	REG_SVersion,	    	 //21    ���ݲɼ����Ĺ̼��汾
	REG_HVersion,            //22    ���ݲɼ�����Ӳ���汾
	REG_ZigbeeID,            //23    Zigbee����ID
	REG_ZigbeePV,            //24    Zigbee����Ƶ����
	REG_MASK,        		 //25    ���ݲɼ�������������
	REG_DG,          		 //26    ���ݲɼ�����Ĭ������
	REG_WIRELESSTYPE,        //27    ����ģ����������,0��zigbee,1��wifi,3:3G,4:2G
	REG_NULL1,				 //28    ��ʹ��,�����Χ�豸�����裩��ʹ�����á��⼸�������ڳ���״̬�ǲ������ģ�Ĭ��ֵ��0,0,0,# 0,0,0,# 0,0,0,# 0,0,0,#
	REG_NULL2,               //29    Ԥ������
	REG_GMT8,                //30    ʱ��
	REG_TIME,  				 //31    ���ݲɼ�����ϵͳʱ��
	REG_RESETFLAG,           //32    ���ݲɼ���������ʶ,0����������,1���������ݲɼ���
	REG_CLEAR,               //33    ���ShinePano�������ݼ�¼�ı�ʶ,0����������,1�����ShinePano�������ݼ�¼
	REG_CORRECTING,          //34    У��ShinePano��������ʶ,0����������
	REG_FACTORY,             //35    �ָ����ݲɼ����ĳ������ñ�ʶ,0����������
	REG_PollingFLAG,         //36    �Թ���豸ֹͣ������ѯ�ı�ʶ,0�����й���豸����ѯ
	REG_03SUBSECTION,        //37    �ɼ����������03�Ź��������ѯ���ζ���.Ĭ���������Σ�0,44#45,89#
	REG_04SUBSECTION,        //38    04�Ź��������ѯ���ζ���,ͬ��
	REG_NULL3,               //39    Ԥ������
	REG_MONITORINGNUM,       //40    �������ݲɼ������������ص�������Ĭ��Ϊ0����ЧֵΪ[1,32]������ֵ�Ƿ�
	REG_COMRESET,			 //41    ͨѶ����ʱ�䡣�����ݲɼ�������������ȡ�������ݣ�����һ��ʱ���ִ���������������ò����������úͶ�ȡ���ʱ��ֵ���ù������Ի�����������ڱ����������⡣
	REG_ProtocolsMode,       //42    ���ݴ���ģʽ��0��Э�鴫��ģʽ,1��͸������ģʽ
	REG_NULL4,			     //43    Ԥ��,Webboxʹ��
	REG_INUPGRADE,           //44	���������ѡ��
	REG_NETTYPE,             //45	��������,1:WIFI.3:3G,0:�Զ�4:,2G
	REG_NULL5,               //46
	REG_NULL6,               //47
	REG_NULL7,               //48
	REG_NULL8,               //49
	REG_NULL9,               //50
	REG_NULL10,              //51
	REG_NULL11,              //52
	REG_BLEMAC,    		     //53   ���� mac
	REG_BLETOKEN,			 //54   ������Ȩ��Կ
	REG_WIFISTATUS,			 //55   wifi����״̬
	REG_WIFISSID,			 //56   ShineWIFI���ӵ�����·��SSID
	REG_WIFIPASSWORD,		 //57	Ҫ���ӵ�WIFI����
	REG_WIFIPSK,			 //58	WIFI������Ϣ  Ĭ�ϼ��ܷ�ʽ WIFI_AUTH_WPA2_PSK
	REG_3GSIGNAL,			 //59	2G,3G�ź�ֵ
	REG_LINKSTATUS,			 //60	�ɼ�����server ͨ��״̬03/04/16
	REG_FirmwareVersion,	 //61	ʹ�õ�����ģ�������汾
	REG_SIMIMEI,			 //62	����IMEI����
	REG_SIMICCID,			 //63	SIM ICCID����
	REG_SIMMMO,				 //64	SIM��������Ӫ��
	REG_WIFIupdate,			 //65	wifi�ļ���������
	REG_ANTIREFLUX,			 //66	����������
	REG_ANTIREFLUXPOWER,	 //67	��������������
	REG_RFCHANNEL,			 //68	RFͨѶ�ŵ�����,Ĭ��Ϊ0
	REG_RFPAIR,				 //69	����Ƿ�ɹ�
	REG_WIFIMODE,			 //70	WIFIģʽ�л�(AP/STA)
	REG_WIFIDHCP,			 //71	DHCPʹ��
	REG_MeboostMODE,		 //72	Meboost����ģʽ
	REG_MeboostFORCE,		 //73	Meboostǿ����ˮʱ��
	REG_MeboostHAND,		 //74	Meboost�ֶ�ģʽ����ʱ��
	REG_WIFISCAN,			 //75	��ȡ��Χwifi����   ɨ��wifi
	REG_SIGNAL,				 //76	���200���źŵ�ƽ��ֵ
	REG_RECOVERIP,			 //77	�Ƿ�ʹ�����ط���������
	REG_ServerIP2,			 //78	Ԥ����������ַ
	REG_ServerPORT2,		 //79	Ԥ���������˿�
	REG_ServerURL2,			 //80	HTTP����ҵ��URL
	REG_DEBUGNUM1,			 //81	ϵͳ����ʱ�䣨��λ��Сʱ��
	REG_DEBUGNUM2,			 //82	ϵͳ�ϵ����
	REG_DEBUGNUM3,			 //83	ϵͳ������������
	REG_DEBUGNUM4,			 //84	�ɹ����ӷ���������
	REG_DEBUGNUM5,			 //85	�ɼ��������ɹ�����
	REG_DEBUGNUM6,			 //86	�ɼ�������ʧ�ܴ���
	REG_DEBUGNUM7,			 //87	����������ɹ�����
	REG_DEBUGNUM8,			 //88	���������ʧ�ܴ���
	REG_DEBUGNUM9,			 //89
	REG_DEBUGNUM10,			 //90
	REG_DEBUGNUM11,			 //91
	REG_DEBUGNUM12,			 //92
	REG_DEBUGNUM13,			 //93
	REG_DEBUGNUM14,			 //94
	REG_DEBUGNUM15,			 //95
	REG_DEBUGNUM16,			 //96
	REG_DEBUGNUM17,			 //97
	REG_DEBUGNUM18,			 //98
	REG_DEBUGNUM19,			 //99
	REG_PROGRESS,			 //100

};

#define Parameter_len_MAX  102
#define SysTAB_Parameter_max  4096


//BLE �����ر�ʱ��
#define DELAY_SCALE_MINUTE		       			60  						 //1����Ϊ60s
#define BLE_CLOSE_TIME_NOT_CONNECTED   			(15 * DELAY_SCALE_MINUTE)   //������������ 15 ���Ӻ� �ر�
#define BLE_CLOSE_TIME_CONNECTED_WHEN_RECV_DATA  (15 * DELAY_SCALE_MINUTE)    //�����յ����ݺ� 15 ���� �ر�
#define BLE_CLOSE_TIME_DISCONNECT   			(10)  			 			//�������ӶϿ� 10s ��ر�


#define SMARTCONFIG_CLOSE_DELAY_TIME   			(30)  			 			//��׼����ģʽ��ȡ��wifi���ƺ������ �� �Զ��ر�ʱ��
#define HEARTBEAT_TIMEOUT						(5 * DELAY_SCALE_MINUTE)	//��������ⳬʱʱ�� 5 ����
#define INVERTER_RT_DATA_TIMEOUT				(30 * DELAY_SCALE_MINUTE)	//ʵʱ���ݳ�ʱʱ�� 30 ����
#define CONNECT_SERVER_TIMEOUT				    (2 * 60  * DELAY_SCALE_MINUTE)	//ʵʱ���ݳ�ʱʱ�� 30 ����


extern uint16_t Parameter_len[Parameter_len_MAX]; 	 				//��ά����ӳ���  ���ݵ�ַ
extern SYStem_state System_state;
extern uint8_t Collector_Config_LenList[Parameter_len_MAX]; 		//��¼ÿ���������ݵĳ���
//extern char System_Config[REG_NUM_MAX][REG_LEN];					//�ɼ����Ĵ�������
extern char SysTAB_Parameter[4096];

extern uint8_t _GucServerParamSetFlag  ;        // ���÷�����������IP��˿ڱ�־λ
extern char _GucServerIPBackupBuf[50] ; 		// ������IP��ַ������
extern char _GucServerPortBackupBuf[6]  ;		// �������˿ڱ�����
extern const char BLEkeytoken[35] ;

extern char _ucServerIPBuf[50] ; 		// Զ�����÷�����IP��ַ
extern char _ucServerPortBuf[6] ;		// Զ�����÷������˿�
extern uint8_t uSetserverflag; 			// �ж��Ƿ�ʹ�ñ��ݵ�IP ����

uint8_t IOT_ParameterTABInit(void);
uint8_t IOT_SystemParameterSet(uint16_t ParamValue ,char* data,uint8_t len);
uint8_t IOT_SystemDATA_init(void);

uint8_t IOT_SETConnect_uc(uint8_t Settype);


void IOT_FactoryReset(void);
void IOT_KEYlongReset(void);

void IOT_SystemFunc_Init(void);

void IOT_SetInvTime0x10(uint16_t TimeStartaddr,uint16_t TimeEndaddr ,uint8_t *pSetTimer);
uint8_t IOT_ServerConnect_Check(uint8_t type) ;
uint8_t IOT_ServerParamSet_Check(uint8_t mode);
uint16_t IOT_CollectorParam_Get(uint8_t paramnum,uint8_t *src);

void IOT_SysTEM_Task(void *arg);

void IOT_UPDATETime(void);
void ClearAllRecord(void);

void IOT_SYSRESTART(void);
uint8_t IOT_BLEkeytokencmp(void);

uint32_t GetSystemTick_ms(void);

//////////�������ݺ���//////////////
void Write_RecordItem(uint8_t *ItemPoint);
uint16_t readRecord(uint8_t *tmp);
uint8_t Read_RecordItem(uint8_t *ItemPoint);
uint8_t Get_RecordStatus(void);
uint16_t Get_RecordNum(void);
void FlashData_Check(uint8_t *buf);
void IOT_Reset_Reason_Printf(void);

#endif /* COMPONENTS_INCLUDES_IOT_SYSTEM_H_ */
