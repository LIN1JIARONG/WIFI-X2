/*
 * iot_system.c
 *
 *  Created on: 2021��12��1��
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
    "0",                    //0     �ɼ�����������  0:Ӣ�ģ�1:����
    "1",                    //1     ���ݲɼ�����Э������,0��MODBUS_TCP,1��Internal
    "7.0",                  //2  *  ����Э�顱������Э�飩�İ汾��
    "10",                   //3     ShinePano������ʱ
    "5",                   //4  *  ���ݲɼ���������������������ݵļ��ʱ�䣬��λΪ����
    "1",                    //5     ���ݲɼ��������������ʼ������ַ
    "32",                   //6     ���ݲɼ�����������Ľ���������ַ
    "123456",               //7     Ԥ������,�ڲ���ʹ�ã�����������;
    "XXXXXXXXXX",           //8     ���ݲɼ������к� BLESKA210A BLEKEY0330 WIFIX2SN03 BLEFULL321 WIFIX2SN0E  XXXXXXXXXX
    "2.0",                  //9     �������ѯʱ����ʱ
    "0",                    //10    �����ݲɼ����Ĺ̼����и��µı�ʶ,0����������,1�����¹̼�
    "NULL",			        //11    wifiģ���޷�ʹ��FTP,�������ݲɼ�����������̼�������豸�̼����µ�FTP���ã����û��������롢IP���˿ڣ����������þ��ŷָ��������ݸ�ʽΪ��UserName#Password#IP#Port#
    "202.96.134.133",       //12    ���ݲɼ�����DNS����
	DATALOG_TYPE,       	//13    ���ݲɼ�������,33:���òɼ��� ��34��wifi-x2
    "192.168.5.1",          //14    ���ݲɼ����ı���IP��ַ,Ϊ��Server����ʾ
    "80",                	//15    ���ݲɼ����ı��˶˿�
    "84:f7:03:3a:3a:xx",    //16 *  ���ݲɼ������뻥����������MAC��ַ  WiFi mac
	"server-cn.growatt.com",      //17 *  ���ݲɼ�����Զ�ˣ������������IP��ַ//"183.62.216.35" ,//"20.6.1.65" , //  "ces.growatt.com", ces-test.growatt.com
	"7006",  				//18 *  ���ݲɼ�����Զ�ˣ�������������˿�//"8080",  //  "7006",
    "server-cn.growatt.com", 		//19    ���ݲɼ�����Զ�ˣ����������������
    "GTSW0000",             //20    ���ݲɼ����İ�������
    DATALOG_SVERSION,	    //21    ���ݲɼ����Ĺ̼��汾
    "V1.0",                 //22    ���ݲɼ�����Ӳ���汾
    "0",                    //23    Zigbee����ID
    "0",                    //24    Zigbee����Ƶ����
    "255.255.255.0",        //25    ���ݲɼ�������������
    "192.168.5.1",          //26    ���ݲɼ�����Ĭ������
    "1",                    //27    ����ģ����������,0��zigbee,1��wifi,3:3G,4:2G
    "NULL",				    //28    ��ʹ��,�����Χ�豸�����裩��ʹ�����á��⼸�������ڳ���״̬�ǲ������ģ�Ĭ��ֵ��0,0,0,# 0,0,0,# 0,0,0,# 0,0,0,#
    "0",                    //29    Ԥ������
    "GMT+8",                //30    ʱ��
    "2022-06-07 11:20:23",  //31 *  ���ݲɼ�����ϵͳʱ��
    "0",                    //32 *  ���ݲɼ���������ʶ,0����������,1���������ݲɼ���
    "0",                    //33    ���ShinePano�������ݼ�¼�ı�ʶ,0����������,1�����ShinePano�������ݼ�¼
    "0",                    //34    У��ShinePano��������ʶ,0����������
    "0",                    //35    �ָ����ݲɼ����ĳ������ñ�ʶ,0����������
    "0",                    //36    �Թ���豸ֹͣ������ѯ�ı�ʶ,0�����й���豸����ѯ
    "0,44#45,89#",          //37    �ɼ����������03�Ź��������ѯ���ζ���.Ĭ���������Σ�0,44#45,89#
    "0,44#45,89#",          //38    04�Ź��������ѯ���ζ���,ͬ��
    "0",                    //39    Ԥ������
    "0",                    //40    �������ݲɼ������������ص�������Ĭ��Ϊ0����ЧֵΪ[1,32]������ֵ�Ƿ�
    "30",					//41    ͨѶ����ʱ�䡣�����ݲɼ�������������ȡ�������ݣ�����һ��ʱ���ִ���������������ò����������úͶ�ȡ���ʱ��ֵ���ù������Ի�����������ڱ����������⡣
    "0",                    //42    ���ݴ���ģʽ��0��Э�鴫��ģʽ,1��͸������ģʽ
    "NULL",					//43    Ԥ��,Webboxʹ��
    "0",           			//44	���������ѡ��
    "1",                    //45	��������,1:WIFI.3:3G,0:�Զ�4:,2G
    "0",                    //46
    "0",                    //47
    "0",                    //48
    "0",                    //49
    "0",                    //50
    "0.2",                  //51
    "0",                    //52
    "84:f7:03:3a:3a:xx",    //53 *  ���� mac
    "growatt_iot_device_common_key_01",	 //54 * ������Ȩ��Կgrowatt_iot_device_common_key_01
	"0",					//55 *  wifi����״̬
    "12345678",				//56 *  ShineWIFI���ӵ�����·��SSID  zktest1
    "0",			        //57 *  Ҫ���ӵ�WIFI����
    "2",					//58	WIFI������Ϣ  Ĭ�ϼ��ܷ�ʽ WIFI_AUTH_WPA2_PSK  zktestzktest   ��������071207
    "0",					//59	2G,3G�ź�ֵ
    "0",					//60	�ɼ�����Server��ͨѶ״̬  �������� 3��4��16  APP ��ȡ�Ƿ����ӷ�����
	"IDFSDK:v4.4.2",		//61	ʹ�õ�����ģ�������汾    �������� IDFSDK:v4.4.1
	"0",					//62	����IMEI����
	"0",					//63	SIM ICCID����
	"0",					//64	SIM��������Ӫ��
	"0",					//65	wifi�ļ���������
	"0",					//66	����������
	"0",					//67	��������������
	"0",					//68	RFͨѶ�ŵ�����,Ĭ��Ϊ0
	"0",					//69	����Ƿ�ɹ�
	"0",					//70	WIFIģʽ�л�(AP/STA)
	"0",					//71	DHCPʹ��
	"0",					//72	Meboost����ģʽ
	"0",					//73	Meboostǿ����ˮʱ��
	"0",					//74	Meboost�ֶ�ģʽ����ʱ��
	"0",					//75	��ȡ��Χwifi����   ɨ��wifi
	"0",					//76	���200���źŵ�ƽ��ֵ
	"0",					//77	�Ƿ�ʹ�����ط���������
	"0",					//78	Ԥ����������ַ
	"0",					//79	Ԥ���������˿�
	"0",					//80 *  HTTP����ҵ��URL
	"0",					//81	ϵͳ����ʱ�䣨��λ��Сʱ��
	"0",					//82	ϵͳ�ϵ����
	"0",					//83	ϵͳ������������
	"0",					//87	����������ɹ�����
	"0",					//84	�ɹ����ӷ���������
	"0",					//85	�ɼ��������ɹ�����
	"0",					//86	�ɼ�������ʧ�ܴ���
	"0",					//88	���������ʧ�ܴ���
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


/* LED״̬���� */
enum _IOTESPLEDSTATUS{
	ESP_RGBLED_RESET = 0,  // RGB-led ȫ���־λ
	ESP_RGBLED_SET,        // RGB-led ȫ����־λ
	ESP_RLED_SET,          // R-led ����־λ    �����ͨѶ����
	ESP_GLED_SET,          // G-led ����־λ    wifi��������
	ESP_BLED_SET,          // B-led ����־λ    tcp��������
	ESP_GLED_SETWIFI_SET,  // G-led ����־λ    wifi��������
	ESP_RLED_FAULT_SET     // R-led ����־λ    �ɼ�������
};
//char System_Config[REG_NUM_MAX][REG_LEN];		// �ɼ����Ĵ�������
const uint8_t Flashparameter[10] = {"PasswordON"};
const char BLEkeytoken[35]={"growatt_iot_device_common_key_01"}; //Grwatt0  growatt_iot_device_common_key_01


uint16_t Parameter_length=0;			//�ܳ���
uint16_t Parameter_len[Parameter_len_MAX]; 	 		//��ά����ӳ���  ���ݵ�ַ
uint8_t Collector_Config_LenList[Parameter_len_MAX];//��¼ÿ���������ݵĳ���
char SysTAB_Parameter[SysTAB_Parameter_max];  		//���ݴ洢 һά���� �ɼ����Ĵ�������
SYStem_state System_state={0};						//ϵͳ�����ṹ��


/*******************************************
 * ÿ���ϵ��ȡ�����б������ֵ IOT_ParameterTABInit
 * ÿ�������Ĵ�����������     Collector_Config_LenList
 * �ܼĴ�����������  	       Parameter_length
 * ÿ���Ĵ�����ַ�����ȣ�     Parameter_len
 * ÿ����������           SysTAB_Parameter
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

	if(memcmp(pbuf,Flashparameter,10) == 0 )						            // ����Ƿ��ǵ�һ���ϵ�
	{
		ESP_LOGI(SYS,"\r\n**********IOT_ParameterTABInit****************\r\n");
		memset(pbuf,0,4096);												// �建��
		IOT_Mutex_SPI_FLASH_BufferRead(pbuf,REG_LEN_LIST_ADDR,W25X_FLASH_ErasePageSize);		// ������������ ��ȡ4K


		CRC16 = GetCRC16(pbuf,CONFIG_CRC16_NUM);
		uCRC16[0] = CRC16>>8;
		uCRC16[1] = CRC16&0x00ff;
	    ESP_LOGI(SYS,  "\r\nREG_LEN_LIST_ADDR GetCRC16= 0x%02x  ReadaCRC= 0x%02x%02x\r\n",CRC16,pbuf[CONFIG_CRC16_BUFADDR],pbuf[CONFIG_CRC16_BUFADDR+1]);
		/* У��CRC16 */
		if((pbuf[CONFIG_CRC16_BUFADDR] == uCRC16[0]) && (pbuf[CONFIG_CRC16_BUFADDR+1] == uCRC16[1]))
		{
			printf("CRC validates correctly!\r\n");
			memcpy(Collector_Config_LenList, pbuf, Parameter_len_MAX);// �������ȱ�
			Parameter_length =0;
			for(  i=0; i < Parameter_len_MAX; i++)
			{
				Parameter_len[i] = Parameter_length ; 			// ���ݵ�ַ
				Parameter_length = Parameter_length + Collector_Config_LenList[i];   //�����б�ʵ���ܳ���
			}
			memcpy(SysTAB_Parameter, &pbuf[PARAMETER_LENLIST_LEN],  Parameter_length);	// �������ȱ�

			//�̻�����  (i==2)|| (i==13)   || (i==21)
			if(Collector_Config_LenList[2]== 1 )
			{
			    memcpy(&SysTAB_Parameter[Parameter_len[2]],Collector_Set_Tab[2],Collector_Config_LenList[2]);	//��������
			}
			if(Collector_Config_LenList[13]== 2 )
			{
				memcpy(&SysTAB_Parameter[Parameter_len[13]],Collector_Set_Tab[13],Collector_Config_LenList[13]);//��������
			}
			if(Collector_Config_LenList[21]== 7 )
			{
				memcpy(&SysTAB_Parameter[Parameter_len[21]],Collector_Set_Tab[21],Collector_Config_LenList[21]);//��������
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
			memset(pbuf,0,4096);												// �建��
			IOT_Mutex_SPI_FLASH_BufferRead(pbuf,BACKUP_CONFIG_ADDR,W25X_FLASH_ErasePageSize);		// ������������ ��ȡ4K

			CRC16 = GetCRC16(pbuf,CONFIG_CRC16_NUM);
			uCRC16[0] = CRC16>>8;
			uCRC16[1] = CRC16&0x00ff;
			ESP_LOGI(SYS,  "\r\n BACKUP_CONFIG_ADDR GetCRC16= 0x%02x  ReadaCRC= 0x%02x%02x\r\n",CRC16,pbuf[CONFIG_CRC16_BUFADDR],pbuf[CONFIG_CRC16_BUFADDR+1]);
			/* У��CRC16 */
			if((pbuf[CONFIG_CRC16_BUFADDR] == uCRC16[0]) && (pbuf[CONFIG_CRC16_BUFADDR+1] == uCRC16[1]))
			{
				/* �ӱ��������Ʋ������������� */
				IOT_Mutex_SPI_FLASH_WriteBuff(pbuf,REG_LEN_LIST_ADDR,W25X_FLASH_ErasePageSize);
				memcpy(Collector_Config_LenList, pbuf, Parameter_len_MAX);// �������ȱ�
				Parameter_length =0;
				for(  i=0; i < Parameter_len_MAX; i++)
				{
							Parameter_len[i] = Parameter_length ; 			// ���ݵ�ַ
							Parameter_length = Parameter_length + Collector_Config_LenList[i];   //�����б�ʵ���ܳ���
				}
				memcpy(SysTAB_Parameter, &pbuf[PARAMETER_LENLIST_LEN],  Parameter_length);	// �������ȱ�

						//�̻�����  (i==2)|| (i==13)   || (i==21)
				if(Collector_Config_LenList[2]== 1 )
				{
					memcpy(&SysTAB_Parameter[Parameter_len[2]],Collector_Set_Tab[2],Collector_Config_LenList[2]);	//��������
				}
				if(Collector_Config_LenList[13]== 2 )
				{
					memcpy(&SysTAB_Parameter[Parameter_len[13]],Collector_Set_Tab[13],Collector_Config_LenList[13]);//��������
				}
				if(Collector_Config_LenList[21]== 7 )
				{
					memcpy(&SysTAB_Parameter[Parameter_len[21]],Collector_Set_Tab[21],Collector_Config_LenList[21]);//��������
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
		// ��һ���ϵ� �ָ���������
		ESP_LOGI(SYS,"\r\n Param_DeInit ...  \r\n");
		W25QXX_Erase_Block(0x00);
		IOT_Printf("\r\n 1 \r\n");
		Parameter_length=0;
		for(i=0; i<Parameter_len_MAX; i++)
		{
			Collector_Config_LenList[i] = strlen(Collector_Set_Tab[i]);					 //��ȡÿ���Ĵ�����ֵ�ĳ���
			Parameter_len[i]= Parameter_length ; 			// ���ݵ�ַ
			Parameter_length  =  Parameter_length + Collector_Config_LenList[i];   		 //�����б�ʵ���ܳ���
//			ESP_LOGI(SYS,"\r\n addr Parameter_len[%d]=%d  sum addr  =Parameter_length=%d ...  \r\n",i,Parameter_len[i],Parameter_length);
			memcpy(&SysTAB_Parameter[Parameter_len[i]],Collector_Set_Tab[i],Collector_Config_LenList[i]);	//��������
		}
		IOT_Printf("\r\n 2 \r\n");
	//  Parameter_length = SumLen+ Collector_Config_LenList[i];             			 //�����б�ʵ���ܳ���
		memset(pbuf,0,W25X_FLASH_ErasePageSize);									     //�建��
		memcpy(&pbuf[CONFIG_LEN_BUFADDR],&Collector_Config_LenList,Parameter_len_MAX);	 //���¼Ĵ����ĳ�������
		memcpy(&pbuf[CONFIG_BUFADDR],&SysTAB_Parameter, Parameter_length);	 			 //����д��Ĵ�������
		/* CRC16У�� */
		IOT_Printf("\r\n 3 \r\n");
		CRC16 = GetCRC16(pbuf,CONFIG_CRC16_NUM);
		uCRC16[0] = CRC16>>8;

		uCRC16[1] = CRC16&0x00ff;
		pbuf[CONFIG_CRC16_BUFADDR] = uCRC16[0];
		pbuf[CONFIG_CRC16_BUFADDR+1] = uCRC16[1];
		/* д�������� */
		IOT_Printf("\r\n 4 \r\n");
		IOT_Mutex_SPI_FLASH_WriteBuff(pbuf,REG_LEN_LIST_ADDR,W25X_FLASH_ErasePageSize);
		/* д�뱸���� */
		IOT_Mutex_SPI_FLASH_WriteBuff(pbuf,BACKUP_CONFIG_ADDR,W25X_FLASH_ErasePageSize);
		/* д���ʾ */
		IOT_Mutex_SPI_FLASH_BufferRead(pbuf, RW_SIGN_ADDR, 100);
		memcpy(pbuf, Flashparameter, 10);
		IOT_Printf("\r\n 5 \r\n");
		IOT_Mutex_SPI_FLASH_WriteBuff(pbuf, RW_SIGN_ADDR, 100);		//д���ʶ
		ESP_LOGI(SYS,"\r\n W25QXX_Write uCRC16=0x%02x%02x\r\n",uCRC16[0],uCRC16[1]);
		IOT_Mutex_SPI_FLASH_BufferRead(pbuf,CONFIG_CRC16_ADDR,2);
		ESP_LOGI(SYS,"\r\n W25QXX_Read  uCRC16=0x%02x%02x \r\n",pbuf[0],pbuf[1]);
	}


	memset(pbuf,0,4096);									//�建��
	return 0;
}

uint8_t IOT_BLEkeytokencmp(void)
{
	char SystemDATA_value[50]={0};
	bzero(SystemDATA_value, sizeof(SystemDATA_value));
	memcpy(SystemDATA_value,&SysTAB_Parameter[Parameter_len[REG_BLETOKEN]],Collector_Config_LenList[REG_BLETOKEN]);
	if(strncmp(SystemDATA_value,BLEkeytoken,32) ==0 ) // ������Կ�Ƚ�
	{
		ESP_LOGI(SYS,"*g**BLEkeytoken==0**");
		System_state.BLEBindingGg=0;    // ble ��ʼ�����豸
	//	System_state.WIFI_BLEdisplay |=Displayflag_SYSreset;  //д��Яʽ��ʾ״̬
	}
	else
	{
		ESP_LOGI(SYS,"*G**BLEkeytoken==1**");
		System_state.BLEBindingGg=1;				// ble ���豸
	//	System_state.WIFI_BLEdisplay &=~Displayflag_SYSreset;  //д��Яʽ��ʾ״̬
	}
	return 0;
}

uint8_t IOT_SystemDATA_init(void)
{
	System_state.Wifi_state = Offline;   		 // wifi δ����
	System_state.wifi_onoff = 0;					 // wifi �ر�
	System_state.Ble_state = Offline;   		 // ble  δ����
	System_state.ble_onoff = 0;					 // ble  �ر�
	System_state.Mbedtls_state = 0;
	System_state.SSL_state = 0;
	System_state.BLEKeysflag = 0; 				 // ble δͨ�Ž�����Կ
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

//	Get_RecordStatus();   // �ƶ���main��c
//	Get_RecordNum();      // �ϵ��ȡflash���ݱ�����


	return 0;
}

/************************************************************************
 * FunctionName : IOT_SystemParameterSet
 * Description  : �ɼ��������б� ���ݱ���
 * Parameters   : uint8_t ParamValue  : ���ò������
 * 				  char * data		  :	���������
 * 				  uint8_t len    	  :	��������ݳ���   ����256+ ���� ��ֹ�쳣
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
	memset(pbuf,0,4096);								//	�建��

	if(Collector_Config_LenList[ParamValue] == len )  	//	���Ȳ��� ֱ�Ӹ�ֵ ָ��������ַ  ������
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
 * �ȶ�ȡ �洢�ļĴ����ĺ��������  ��ֵ�� pbuf
 * �ٸ�ֵ �洢�����ݵ���ǰ�ĵ�ַ �����ۼ� SysTAB_Parameter
 * �����¸�ֵSysTAB_Parameter + pbuf
 * */
		Remain_length =0;
		for(i=(ParamValue+1); i<Parameter_len_MAX; i++)
		{
			Remain_length  =  Remain_length + Collector_Config_LenList[i];   //�����б�ʵ���ܳ���
		}
		memcpy(pbuf,&SysTAB_Parameter[Parameter_len[ParamValue+1]], Remain_length ); // ʣ��ĸ�ֵ���洢��������

		memcpy(&SysTAB_Parameter[Parameter_len[ParamValue]],data,len);  // ��ֵ�²���
	    Collector_Config_LenList[ParamValue]=len;					    // ��ֵ�³���

		Parameter_length =0;  //���¼����ܳ���
		for(  i=0; i<Parameter_len_MAX; i++)
		{
			Parameter_len[i]=  Parameter_length; 		 // ���ݵ�ַ
			Parameter_length  =  Parameter_length + Collector_Config_LenList[i];   //�����б�ʵ���ܳ���
		}
		memcpy(&SysTAB_Parameter[Parameter_len[ParamValue+1]], pbuf, Remain_length );   //

#ifdef Kennedy_System
		ESP_LOGI(SYS,"*******data =%s***************Collector_Config_LenList[ParamValue] *****ParamValue=%d**************\r\n",data,ParamValue );
		ESP_LOGI(SYS," add =%d   ,Collector_Config_LenList =%d   ,len =%d \r\n", Parameter_len[ParamValue] ,Collector_Config_LenList[ParamValue] , len);
#endif
	}

	memcpy(&pbuf[CONFIG_LEN_BUFADDR],Collector_Config_LenList,Parameter_len_MAX);		// ���¼Ĵ����ĳ�������

	memcpy(&pbuf[CONFIG_BUFADDR],SysTAB_Parameter,CONFIG_LEN_MAX);					//	����д��Ĵ�������


	if( ParamValue == 60 || ParamValue ==101 )  // ���洢
	{    free(pbuf);
		return 0;
	}
//	IOT_Mutex_SPI_FLASH_WriteBuff(pbuf,BACKUP_CONFIG_ADDR,W25X_FLASH_ErasePageSize);
	/* CRC16У�� */
	CRC16 = GetCRC16(pbuf,CONFIG_CRC16_NUM);
	u8_CRC16[0] = CRC16>>8;
	u8_CRC16[1] = CRC16&0x00ff;
	pbuf[CONFIG_CRC16_BUFADDR] = u8_CRC16[0];
	pbuf[CONFIG_CRC16_BUFADDR+1] = u8_CRC16[1];
	/* д�������� */
	IOT_Mutex_SPI_FLASH_WriteBuff(pbuf,REG_LEN_LIST_ADDR,W25X_FLASH_ErasePageSize);
	/* д�뱸���� */
	IOT_Mutex_SPI_FLASH_WriteBuff(pbuf,BACKUP_CONFIG_ADDR,W25X_FLASH_ErasePageSize);
    free(pbuf);
	ESP_LOGI(SYS, "IOT_SystemParameterSet  OK\r\n");
	return 0;
}


uint32_t GetSystemTick_ms(void)
{
	//   GetSystemTick_ms ��� 0 �����bug
	return esp_timer_get_time()/1000;   //um  ת   ms  ��λ
}
/****************************************************************
  *     У��ʱ��(ʱ����������ֻ�'.')
        *src��sn��  len��sn�ų���
  *		  0-�ɹ� 1-ʧ��
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
  *     �ֶ�����FLASH������
  *		���������Ĳ�������µ�FLASH��
  *		�ָ��������� ���������ķ����� ��SN  ���ϱ�ʱ��  ��ʱ��
*****************************************************************/

char FctRstBackupBuf[8][256]={0};	//50�ĳ� 256
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
	IOT_ParameterTABInit();    // ��ʼ�� ������
	pbuf = W25QXX_BUFFER;
	Parameter_length =0;
	for(i=0; i<Parameter_len_MAX; i++)
	{
		/* һЩ����Ĳ���������ʱ�趨�����ᱻ���ġ� */
		if(i == 8)		//���к�
		{
			Collector_Config_LenList[i] = BackupLen[0];					 		 //��ȡÿ���Ĵ�����ֵ�ĳ���
			Parameter_len[i]= Parameter_length ; 										 //���ݵ�ַ
			Parameter_length  =  Parameter_length + Collector_Config_LenList[i];   		 //�����б�ʵ���ܳ���
			memcpy(&SysTAB_Parameter[Parameter_len[i]], FctRstBackupBuf[0], Collector_Config_LenList[i]);	//��������
			continue;
		}
		else if(i == 17)//IP��ַ
		{
			Collector_Config_LenList[i] = BackupLen[1];					 		 //��ȡÿ���Ĵ�����ֵ�ĳ���
			Parameter_len[i]= Parameter_length ; 										 //���ݵ�ַ
			Parameter_length  =  Parameter_length + Collector_Config_LenList[i];   		 //�����б�ʵ���ܳ���
			memcpy(&SysTAB_Parameter[Parameter_len[i]], FctRstBackupBuf[1], Collector_Config_LenList[i]);	//��������
			continue;
		}
		else if(i == 18)
		{
			Collector_Config_LenList[i] = BackupLen[2];					 		 //��ȡÿ���Ĵ�����ֵ�ĳ���
			Parameter_len[i]= Parameter_length ; 										 //���ݵ�ַ
			Parameter_length  =  Parameter_length + Collector_Config_LenList[i];   		 //�����б�ʵ���ܳ���
			memcpy(&SysTAB_Parameter[Parameter_len[i]], FctRstBackupBuf[2], Collector_Config_LenList[i]);	//��������
			continue;
		}
		else if(i == 19)
		{
			Collector_Config_LenList[i] = BackupLen[3];					 		 //��ȡÿ���Ĵ�����ֵ�ĳ���
			Parameter_len[i]= Parameter_length ; 										 //���ݵ�ַ
			Parameter_length  =  Parameter_length + Collector_Config_LenList[i];   		 //�����б�ʵ���ܳ���
			memcpy(&SysTAB_Parameter[Parameter_len[i]], FctRstBackupBuf[3], Collector_Config_LenList[i]);	//��������
			continue;
		}
		else if(i == 31)
		{
			Collector_Config_LenList[i] = BackupLen[4];					 		 //��ȡÿ���Ĵ�����ֵ�ĳ���
			Parameter_len[i]= Parameter_length ; 										 //���ݵ�ַ
			Parameter_length  =  Parameter_length + Collector_Config_LenList[i];   		 //�����б�ʵ���ܳ���
			memcpy(&SysTAB_Parameter[Parameter_len[i]], FctRstBackupBuf[4], Collector_Config_LenList[i]);	//��������
			continue;
		}
		else if(i == 56)
		{
			Collector_Config_LenList[i] = BackupLen[5];					 		 //��ȡÿ���Ĵ�����ֵ�ĳ���
			Parameter_len[i]= Parameter_length ; 										 //���ݵ�ַ
			Parameter_length  =  Parameter_length + Collector_Config_LenList[i];   		 //�����б�ʵ���ܳ���
			memcpy(&SysTAB_Parameter[Parameter_len[i]], FctRstBackupBuf[5], Collector_Config_LenList[i]);	//��������
			continue;
		}
		else if(i == 57)
		{
			Collector_Config_LenList[i] = BackupLen[6];					 		 //��ȡÿ���Ĵ�����ֵ�ĳ���
			Parameter_len[i]= Parameter_length ; 										 //���ݵ�ַ
			Parameter_length  =  Parameter_length + Collector_Config_LenList[i];   		 //�����б�ʵ���ܳ���
			memcpy(&SysTAB_Parameter[Parameter_len[i]], FctRstBackupBuf[6], Collector_Config_LenList[i]);	//��������
			continue;
		}

		Collector_Config_LenList[i] = strlen(Collector_Set_Tab[i]);					 //��ȡÿ���Ĵ�����ֵ�ĳ���
		Parameter_len[i]= Parameter_length ; 										 //���ݵ�ַ
		Parameter_length  =  Parameter_length + Collector_Config_LenList[i];   		 //�����б�ʵ���ܳ���
		memcpy(&SysTAB_Parameter[Parameter_len[i]],Collector_Set_Tab[i],Collector_Config_LenList[i]);	//��������
	}
	memset(pbuf,0,W25X_FLASH_ErasePageSize);									     //�建��

	memcpy(&pbuf[CONFIG_LEN_BUFADDR],&Collector_Config_LenList,Parameter_len_MAX);		     //���¼Ĵ����ĳ�������
	memcpy(&pbuf[CONFIG_BUFADDR],&SysTAB_Parameter, Parameter_length);	 //����д��Ĵ�������
	/* CRC16У�� */
	CRC16 = GetCRC16(pbuf,CONFIG_CRC16_NUM);
	uCRC16[0] = CRC16>>8;
	uCRC16[1] = CRC16&0x00ff;
	pbuf[CONFIG_CRC16_BUFADDR] = uCRC16[0];
	pbuf[CONFIG_CRC16_BUFADDR+1] = uCRC16[1];
	/* д�������� */
	IOT_Mutex_SPI_FLASH_WriteBuff(pbuf,REG_LEN_LIST_ADDR,W25X_FLASH_ErasePageSize);
	/* д�뱸���� */
	IOT_Mutex_SPI_FLASH_WriteBuff(pbuf,BACKUP_CONFIG_ADDR,W25X_FLASH_ErasePageSize);
	memset(pbuf,0,SECTOR_SIZE);			//�建��

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

	//char BackupBuf[5][256]={0};	//50�ĳ� 256

	char BackupLen[5] = {0};

	IOT_Printf("IOT_KEYlongReset !!!!!!!\r\n" );
	IOT_ParameterTABInit();    // ��ʼ�� ������
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

		if(i == 8)		//���к�
		{
			Collector_Config_LenList[i] = BackupLen[0];					 		 //��ȡÿ���Ĵ�����ֵ�ĳ���
			Parameter_len[i]= Parameter_length ; 										 //���ݵ�ַ
			Parameter_length  =  Parameter_length + Collector_Config_LenList[i];   		 //�����б�ʵ���ܳ���
			memcpy(&SysTAB_Parameter[Parameter_len[i]], FctRstBackupBuf[0], Collector_Config_LenList[i]);	//��������
			continue;
		}
		else if(i == 17)//IP��ַ
		{
			Collector_Config_LenList[i] = BackupLen[1];					 		 //��ȡÿ���Ĵ�����ֵ�ĳ���
			Parameter_len[i]= Parameter_length ; 										 //���ݵ�ַ
			Parameter_length  =  Parameter_length + Collector_Config_LenList[i];   		 //�����б�ʵ���ܳ���
			memcpy(&SysTAB_Parameter[Parameter_len[i]], FctRstBackupBuf[1], Collector_Config_LenList[i]);	//��������
			continue;
		}
		else if(i == 18)
		{
			Collector_Config_LenList[i] = BackupLen[2];					 		 //��ȡÿ���Ĵ�����ֵ�ĳ���
			Parameter_len[i]= Parameter_length ; 										 //���ݵ�ַ
			Parameter_length  =  Parameter_length + Collector_Config_LenList[i];   		 //�����б�ʵ���ܳ���
			memcpy(&SysTAB_Parameter[Parameter_len[i]], FctRstBackupBuf[2], Collector_Config_LenList[i]);	//��������
			continue;
		}
		else if(i == 19)
		{
			Collector_Config_LenList[i] = BackupLen[3];					 		 //��ȡÿ���Ĵ�����ֵ�ĳ���
			Parameter_len[i]= Parameter_length ; 										 //���ݵ�ַ
			Parameter_length  =  Parameter_length + Collector_Config_LenList[i];   		 //�����б�ʵ���ܳ���
			memcpy(&SysTAB_Parameter[Parameter_len[i]], FctRstBackupBuf[3], Collector_Config_LenList[i]);	//��������
			continue;
		}
		else if(i == 31)
		{
			Collector_Config_LenList[i] = BackupLen[4];					 		 //��ȡÿ���Ĵ�����ֵ�ĳ���
			Parameter_len[i]= Parameter_length ; 										 //���ݵ�ַ
			Parameter_length  =  Parameter_length + Collector_Config_LenList[i];   		 //�����б�ʵ���ܳ���
			memcpy(&SysTAB_Parameter[Parameter_len[i]], FctRstBackupBuf[4], Collector_Config_LenList[i]);	//��������
			continue;
		}

		Collector_Config_LenList[i] = strlen(Collector_Set_Tab[i]);					 //��ȡÿ���Ĵ�����ֵ�ĳ���
		Parameter_len[i]= Parameter_length ; 										 //���ݵ�ַ
		Parameter_length  =  Parameter_length + Collector_Config_LenList[i];   		 //�����б�ʵ���ܳ���
		memcpy(&SysTAB_Parameter[Parameter_len[i]],Collector_Set_Tab[i],Collector_Config_LenList[i]);	//��������
	}
	memset(pbuf, 0  ,W25X_FLASH_ErasePageSize);									     //�建��

	memcpy(&pbuf[CONFIG_LEN_BUFADDR],&Collector_Config_LenList,Parameter_len_MAX);		     //���¼Ĵ����ĳ�������
	memcpy(&pbuf[CONFIG_BUFADDR],&SysTAB_Parameter, Parameter_length);	 //����д��Ĵ�������
	/* CRC16У�� */
	CRC16 = GetCRC16(pbuf,CONFIG_CRC16_NUM);
	uCRC16[0] = CRC16>>8;
	uCRC16[1] = CRC16&0x00ff;
	pbuf[CONFIG_CRC16_BUFADDR] = uCRC16[0];
	pbuf[CONFIG_CRC16_BUFADDR+1] = uCRC16[1];
	/* д�������� */
	IOT_Mutex_SPI_FLASH_WriteBuff(pbuf,REG_LEN_LIST_ADDR,W25X_FLASH_ErasePageSize);
	/* д�뱸���� */
	IOT_Mutex_SPI_FLASH_WriteBuff(pbuf,BACKUP_CONFIG_ADDR,W25X_FLASH_ErasePageSize);
	memset(pbuf,0,SECTOR_SIZE);			//�建��

	IOT_Set_OTA_breakpoint_ui(0); 	//�ϵ�λ�� ��0

	i = 0;
	do{
		 ESP_ERROR_CHECK( esp_wifi_restore());	//��������õ�wifi����

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
uint8_t _GucServerParamSetFlag = 0;         // ���÷�����������IP��˿ڱ�־λ
char _GucServerIPBackupBuf[50] = {0}; 		// ������IP��ַ������
char _GucServerPortBackupBuf[6] = {0};		// �������˿ڱ�����
// 78��79�������������� LI 2020.01.06
/* TCP_num_null/TCP_num_2 ���á�17��18��19���� TCP_num_1���á�78��79��*/
uint8_t g_ucServerParamSetType = 0;  		// ���÷������������ͣ�17��18��19 �� 78��79��

//#define   Serverflag_NULL  		0x00    // NULL ״̬
//#define   Serverflag_disable    0x01      // ��ֹ����IP�Ͷ˿�
//#define   Serverflag_enable     0x02      // ��������IP�Ͷ˿ڣ�������Ҫ�������ӳɹ�����ܱ���
//#define   Serverflag_savetest   0x03      // �������������úͱ���IP�Ͷ˿ڣ���Ҫ��������������
//#define   Serverflag_backoldip   	0x04    // �����������ӵľ�IP�Ͷ˿�
//#define   Serverflag_savenowip   	0x05    // �������������úͱ���IP�Ͷ˿ڣ���Ҫ��������IP�ɹ���������󱣴���
//#define   Serverflag_GETflag   	0xff    // ��ȡ���ò���״̬


/************************************************************************
 * FunctionName : IOT_ServerParamSet_Check LI 2018.08.08
 * Description  : ��������������У��
 * Parameters   : uint8_t mode  : ���ò�����ʽ
 * Returns      : ���״̬
 * Notice       : 1/2/3
************************************************************************/
uint8_t IOT_ServerParamSet_Check(uint8_t mode)
{
	if(mode == 0xff)     			// ��ȡ���ò���״̬
	{
		return _GucServerParamSetFlag;
	}
	if(mode == Serverflag_NULL)          			// NULL ״̬
	{
		_GucServerParamSetFlag = Serverflag_NULL;
	}
	else if(mode == Serverflag_disable)
	{
		_GucServerParamSetFlag = Serverflag_disable; // ��ֹ����IP�Ͷ˿�
	}
	else if(mode == Serverflag_enable)
	{
		_GucServerParamSetFlag = Serverflag_enable; // ��������IP�Ͷ˿ڣ�������Ҫ�������ӳɹ�����ܱ���
	}
	else if(mode == Serverflag_savetest)
	{
		_GucServerParamSetFlag = Serverflag_savetest; // �������������úͱ���IP�Ͷ˿ڣ���Ҫ��������������
	}
	else if(mode == Serverflag_backoldip)            	// �����������ӵľ�IP�Ͷ˿�
	{
		memset(_GucServerIPBackupBuf,0,sizeof(_GucServerIPBackupBuf));
		memset(_GucServerPortBackupBuf,0,sizeof(_GucServerPortBackupBuf));
 		memcpy(_GucServerIPBackupBuf,&SysTAB_Parameter[Parameter_len[REG_ServerIP]],Collector_Config_LenList[REG_ServerIP]);  // ���ݾ�IP 17
 		memcpy(_GucServerPortBackupBuf,&SysTAB_Parameter[Parameter_len[REG_ServerPORT]],Collector_Config_LenList[REG_ServerPORT]);// ���ݾ�Port 18
 		IOT_Printf("IOT_ServerParamSet_Check() Backup 17 ip:%s  \r\n",_GucServerIPBackupBuf );
 		IOT_Printf("IOT_ServerParamSet_Check() Backup 18 port:%s   \r\n",_GucServerPortBackupBuf );
	}
	else if(mode == Serverflag_savenowip)             // �������������úͱ���IP�Ͷ˿ڣ���Ҫ��������IP�ɹ���������󱣴���
	{
		IOT_SystemParameterSet(REG_ServerIP,_GucServerIPBackupBuf,strlen(_GucServerIPBackupBuf));        // ������IP 17
		IOT_SystemParameterSet(REG_ServerPORT,_GucServerPortBackupBuf,strlen(_GucServerPortBackupBuf));    // ������IP 18
		IOT_SystemParameterSet(REG_ServerURL,_GucServerIPBackupBuf,strlen(_GucServerIPBackupBuf));		   // ������IP 19

		IOT_Printf("IOT_ServerParamSet_Check() set 17 ip:%s \r\n",_GucServerIPBackupBuf  );
		IOT_Printf("IOT_ServerParamSet_Check() set 18 port:%s  \r\n",_GucServerPortBackupBuf );

	}
	return _GucServerParamSetFlag;
}
/*************************************************************************
 * FunctionName : IOT_ServerConnect_Check LI 2018.08.08
 * Description  : ���������Ӽ��飬���������ip��port�������
 * Parameters   : u8 type  : ���㷽ʽ 0-������Ӵ���  1-�������Ӵ���  0xFF-�������״̬
 * Returns      : ���״̬
 * Notice       : 0/1/2/3
*************************************************************************/
uint8_t IOT_ServerConnect_Check(uint8_t type)
{
	#define IPConnectFailTimes  5  // ��������ַ����ʧ���������	LI 2018.08.08
	static uint8_t sucServerIPConnectTimes = 0;

	#define IPSetWaitTime (30*1000) // 30S Ԥ��ip��portһ�����õ�ʱ��
	static uint32_t ServerIPSetWaitTime = 0;
	static uint8_t WaitFlag = 0;

	if(type == 0) // ������Ӵ���
	{
		sucServerIPConnectTimes = 0;
		WaitFlag = 0;
		ESP_LOGI(SYS,"IOT_TSServerConnect_Check() ������Ӵ���������\r\n");
		return 0;
	}
	if(IOT_ServerParamSet_Check(Serverflag_GETflag) == Serverflag_enable) 	// �ж��Ƿ�������IP��˿ڣ���������������
	{
		if(WaitFlag == 0)
		{
		    ESP_LOGI(SYS,"IOT_TSServerConnect_Check() ���÷�����ip���ȴ������·�������ַ\r\n");
 			ServerIPSetWaitTime = GetSystemTick_ms();
			WaitFlag = 1;
			return 0;
		}
		else if(WaitFlag == 1)
		{
 			if((GetSystemTick_ms () - ServerIPSetWaitTime) > IPSetWaitTime)
			{
				ESP_LOGI(SYS,"IOT_TSServerConnect_Check() ���÷�����ip�������·�������ַ��ʼ !!! \r\n");
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
			sucServerIPConnectTimes++; 						 // ������������Ӵ���
			ESP_LOGI(SYS,"IOT_TSServerConnect_Check() �����·�������ַʧ�ܴ��� = %d\r\n",sucServerIPConnectTimes);
			if(sucServerIPConnectTimes > IPConnectFailTimes) // ���ӷ��������ʧ�ܣ�ת����һ�ξɷ�����
			{
//				IOT_ServerParamSet_Check(1); // ��ֹ����IP�Ͷ˿�
//				sucServerIPConnectTimes = 0;
//				IOT_Change_GPRS_Steps(cGPRS_Restart_Step);
    			System_state.Server_Online   = 0;
    			IOT_Printf("IOT_TSServerConnect_Check() ������ip�������ӷ�������ַʧ�ܣ�ת��Ĭ�Ϸ����� ������ \r\n");
				return 1;	// �����ڻ�ԭ��IP�;�Port�� return 1 ���������ɼ���
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
 * �豸���ķ��������Ӵ�������
 *
 * *******************/
uint8_t uSetserverflag=0; 			// �ж��Ƿ�ʹ�ñ��ݵ�IP ����
char _ucServerIPBuf[50] = {0}; 		// Զ�����÷�����IP��ַ
char _ucServerPortBuf[6] = {0};		// Զ�����÷������˿�
uint8_t IOT_SETConnect_uc(uint8_t Settype)
{
	static uint8_t Again_connectnum=0;
	if(Settype ==0 )
	{
		uSetserverflag=1;
		Again_connectnum=0;

		IOT_IPPORTSet_us(ESP_WRITE,30); // 30 ��󴥷����ӷ�����
	}
	else
	{
		Again_connectnum++;
		if( Again_connectnum > 5)   	// ���� ���� ���� 5 ��  �����豸  �ָ��˿�
		{
			uSetserverflag=0;

			//IOT_SYSRESTART();			// �����豸  �Զ��ָ� 17 / 18 �Ĵ���������
		}
	}
	return uSetserverflag;
}

/****************************************************************
  *     ���زɼ�������
  *		�˺�������Ӧ���ڣ�SPI FLASH�����������ɺ�
****************************************************************/
void IOT_UPDATETime(void)
{
	uint8_t i=0;
	char p[4] = {0};
//	/* ������ѯʱ�� */
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
		System_state.Up04dateTime = 300  ;// 5*60 = 5min  ��λ��
	}
	else if(Collector_Config_LenList[REG_Interval] == 1)
		System_state.Up04dateTime = ( (p[0]-0x30) * 60 )  ;					 //һλ��min  ת��  ��λ��
	else if(Collector_Config_LenList[REG_Interval] == 2)
		System_state.Up04dateTime = ( (((p[0]-0x30)*10) +(p[1]-0x30)) * 60 )  ;//��λ��min  ת��  ��λ�� ��max���ֵ��
	else if(Collector_Config_LenList[REG_Interval] == 3 && ( p[1]== '.'))
		System_state.Up04dateTime = (p[0]-0x30)*60 + ((p[2]-0x30)*60)/10;		 //��λ��min  ת��  ��λ�� ��С������ һλС����  С������/6s��
	else if(Collector_Config_LenList[REG_Interval] == 4 && ( p[2]== '.'))
		System_state.Up04dateTime = ( (((p[0]-0x30)*10) + (p[1]-0x30)) * 60  + ((p[3]-0x30)*60/10)) ;//��λ��min  ת��  ��λ�� ��С������ һλС���㣩
	else
	{
		//Change_Config_Value("5",4,1);
		System_state.Up04dateTime = 300  ;// 5*60 = 5min  ��λ��
	}
	//Mqtt_DATATime = System_state.Up04dateTime;	//04�����ϴ��ļ����4�żĴ������� chen 20220524
	ESP_LOGI(SYS,"\r\nLoadingConfig string = %s\r\n",p);
	ESP_LOGI(SYS,"\r\nLoadingConfig int = %d\r\n",System_state.Up04dateTime);
	IOT_TCPDelaytimeSet_uc(ESP_WRITE,System_state.Up04dateTime);

}

void IOT_SetInvTime0x10(uint16_t TimeStartaddr,uint16_t TimeEndaddr ,uint8_t *pSetTimer)
{
	/* ����������, �����õļĴ�������, ������������� */
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
	uint8_t PrintfBUF[256] = {0};  //buff��С��80  �ĳ� 256

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
 * Description  : ϵͳ����ģ���ʼ��
 * Parameters   : none
 * Returns      : none
 * Notice       : none
*******************************************************************************/
void IOT_SystemFunc_Init(void)
{
	IOT_AppxFlashMutex();
	ESP_LOGI(SYS,"\r\n APP IOT_SystemFunc_Init...\r\n");
	IOT_ParameterTABInit();
	/* ��ӡ������ */
	PrintfParam();
 	ESP_LOGI(SYS, "main minimum free heap size = %d !!!\r\n", esp_get_minimum_free_heap_size());

 	IOT_SystemDATA_init();
	/* ����DTC��ȷ�������ֶΣ�0x03 0x04����ȡ��ʼ�� */
//	if(IOTESPBaudRateSyncStatus == ESP_ON)
//	{
//		dtcInit(InvCommCtrl.DTC);
//	}
//	else if(IOTESPBaudRateSyncStatus == ESP_OFF)
//	{
//		InvCommCtrl.DTC = 0xFFFF; // Ĭ���ֶΣ�0-44 45-89
//		dtcInit(InvCommCtrl.DTC);
//	}
/*******************************************************/
/************���������/6.0��Э������************************/
/*******************************************************/
}

/****************
 * ����
 *
 * **************/
void IOT_SYSRESTART(void)
{
	esp_restart();
}


/****************
 * ����
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
  *     ���ü�¼�洢��
  ***************************************************************
  */
void ClearAllRecord(void)
{
	uint16_t i=0;
	//���ĸ�������ʼ
	//�Ȳ��� ͬ������ 0x4000  ��ʼ
	//�ٲ���
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
 * ��ȡ���ݲɼ�������
 * IOT_CollectorParam_Get
 *
 * ****************************************************/

uint16_t IOT_CollectorParam_Get(uint8_t paramnum,uint8_t *src )
{
//	uint8_t i;
//	uint8_t *pbuf ;
//	uint16_t CRC16;
//	uint8_t uCRC16[2];
//	static uint8_t s_ucReadingData_Timers = 0;  	// ϵͳ������ȡ����
	char NOWTime[20];
//  System_Config  				// �ɼ����Ĵ�������
//  Collector_Config_LenList  	// ��¼ÿ���������ݵĳ���
//  ������ȡflash����
	if(paramnum == 31)   // ��ȡʱ��
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
 //	memcpy(src ,&SysTAB_Parameter[Parameter_len[paramnum]],Collector_Config_LenList[paramnum]);	//��������
	IOT_Printf(" Collector_Config_LenList[paramnum]=%d \r\n",  Collector_Config_LenList[paramnum]);
 	return Collector_Config_LenList[paramnum];
}

#if 0  //20220727  chen  ����
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/****************************************************************
  *     ���뵥�����ݼ�¼
  *		ItemPoint:Ҫ���������ָ��
  ***************************************************************
  */
#define MAX_PACK		(((DATA_MAX) / (RECORD_ITEM_SIZE_TRUE)) + 1)  	//һ���������漸ҳ
#define MAX_ERR_RD		(10)				//һ������ȡʮ�������¼
#define FLAG_WR	1
#define FLAG_RD	0

static uint16_t storageID = 1111;	//�洢����,��1111��ʼ��651111��Ϊ����
volatile uint32_t u32TrRecCnt = 0;		//�Ѵ��¼
volatile uint32_t u32TrOutCnt;		//�Ѷ���¼
volatile uint32_t u32TrOutSave;
volatile uint8_t  Flag_RecordFull = 0;	//��������־λ

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

 	len = InputGrop_DataPacket(ItemPoint);							//�������

 	IOT_Printf("==================================\r\n");
 	IOT_Printf(" Write_RecordItem() �洢0x04 ���ݳ��� = %d\r\n", len);
 	IOT_Printf("==================================\r\n");

//	/* ���ʱ������������洢 */
//	if(RTC_GetTime(Clock_Time_Tab) == 1)        // delete LI 20210427 ��Ϊϵͳ�������ʱ�䣬��ֹ�ɼ���RTCʱ���ȡ����
//		return;

	ItemPoint[28] = InvCommCtrl.PVClock_Time_Tab[0]; 			//��ʷ���� ������������꣬����ʱ����
	ItemPoint[29] = InvCommCtrl.PVClock_Time_Tab[1];
	ItemPoint[30] = InvCommCtrl.PVClock_Time_Tab[2];

	ItemPoint[31] = InvCommCtrl.PVClock_Time_Tab[3];
	ItemPoint[32] = InvCommCtrl.PVClock_Time_Tab[4];
	ItemPoint[33] = InvCommCtrl.PVClock_Time_Tab[5];
//	IOT_Printf("\r\n��¼04��������ʹ�������ʱ��\r\n");

	if(len > RECORD_ITEM_SIZE_TRUE)
	{
		crc16Check = Modbus_Caluation_CRC16(ItemPoint, len);			//���ڶ�ҳ��,��������һ��CRC16У��
		ItemPoint[len] = HIGH(crc16Check);
		ItemPoint[len+1] = LOW(crc16Check);
		len += 2;
	}

	packNum = (len / RECORD_ITEM_SIZE_TRUE);
	lastLen = (len % RECORD_ITEM_SIZE_TRUE);

	for(i = 0; i < packNum; i++)
	{
		/* ����� */
		if(storageID <= SAVANUM_END)
			storageID++;
		else
			storageID = SAVANUM_START;
		/* �������� */
		memset(buf, 0, 256);
		memcpy(buf, (ItemPoint + (i * RECORD_ITEM_SIZE_TRUE)), RECORD_ITEM_SIZE_TRUE);

		buf[RECORD_ITEM_SIZE_TRUE] = (storageID >> 8) & 0x00ff;
		buf[RECORD_ITEM_SIZE_TRUE+1] = (storageID) & 0x00ff;

		/* ��У�� */
		crc16Check = GetCRC16(buf,RECORD_ITEM_SIZE_TRUE+2);
		buf[RECORD_ITEM_SIZE_TRUE+2] = (crc16Check >> 8) & 0x00ff;
		buf[RECORD_ITEM_SIZE_TRUE+3] = (crc16Check) & 0x00ff;

		if(u32TrRecCnt >= RECORD_AMOUNT_NUM)
		{
			u32TrRecCnt = 0;			   		//�����¼��¼����
			Flag_RecordFull = 1; 				//�����¼����

			uStatus[0] = FULL;
			IOT_Mutex_SPI_FLASH_WriteBuff(uStatus,RECORD_FULL,1);
		}

		buf[RECORD_ITEM_SIZE_TRUE+4] = PAGE_WR;				//��д���λ
		if(lastLen == 0)
		{
			buf[RECORD_PACK_ALL] = packNum;							//�ܰ�
		}
		else
		{
			buf[RECORD_PACK_ALL] = packNum + 1;						//�ܰ�
		}
		buf[RECORD_PACK_CUR] = i;								//��ǰ��
		buf[RECORD_LAST_LEN] = RECORD_ITEM_SIZE_TRUE;		//��ǰ����

		u32Addr = RECORD_ADDR + (u32TrRecCnt * RECORD_ITEM_SIZE);
		IOT_Mutex_SPI_FLASH_WriteBuff(buf, u32Addr, RECORD_ITEM_SIZE_TRUE+5+3);


		u32TrRecCnt++;										//��¼����
//		IWDG_Feed();
		if(u32TrRecCnt%RECORD_SECTOR_MAX_NUM == 0)			//д��������־λ
		{
			uStatus[0] = SEC_FULL;
			IOT_Mutex_SPI_FLASH_WriteBuff(uStatus, u32Addr+RECORD_ITEM_SIZE-SEC_SIGN_ADDR,1);
		}
//		IWDG_Feed();
	}

	/* ���һ�� */
	if(lastLen > 0)
	{
				/* ����� */
		if(storageID <= SAVANUM_END)
			storageID++;
		else
			storageID = SAVANUM_START;

		/* �������� */
		memset(buf, 0, 256);
		memcpy(buf, (ItemPoint + (i * RECORD_ITEM_SIZE_TRUE)), lastLen);

		buf[RECORD_ITEM_SIZE_TRUE] = (storageID >> 8) & 0x00ff;
		buf[RECORD_ITEM_SIZE_TRUE+1] = (storageID) & 0x00ff;

		/* ��У�� */
		crc16Check = GetCRC16(buf,RECORD_ITEM_SIZE_TRUE+2);
		buf[RECORD_ITEM_SIZE_TRUE+2] = (crc16Check >> 8) & 0x00ff;
		buf[RECORD_ITEM_SIZE_TRUE+3] = (crc16Check) & 0x00ff;


		if(u32TrRecCnt >= RECORD_AMOUNT_NUM)
		{
			u32TrRecCnt = 0;			   		//�����¼��¼����
			Flag_RecordFull = 1; 				//�����¼����

			uStatus[0] = FULL;
			IOT_Mutex_SPI_FLASH_WriteBuff(uStatus, RECORD_FULL, 1);
		}

		buf[RECORD_ITEM_SIZE_TRUE+4] = PAGE_WR;				//��д���λ

		if(lastLen == 0)
		{
			buf[RECORD_PACK_ALL] = packNum;							//�ܰ�
			buf[RECORD_PACK_CUR] = (i-1);					//û�е�������,������forѭ�����++��
		}
		else
		{
			buf[RECORD_PACK_ALL] = packNum + 1;						//�ܰ�
			buf[RECORD_PACK_CUR] = i;								//��ǰ��
		}
		buf[RECORD_LAST_LEN] = lastLen;								//��ǰ����

		u32Addr = RECORD_ADDR + (u32TrRecCnt * RECORD_ITEM_SIZE);
		IOT_Mutex_SPI_FLASH_WriteBuff(buf, u32Addr, RECORD_ITEM_SIZE_TRUE+5+3);

		u32TrRecCnt++;										//��¼����
//		IWDG_Feed();
		if(u32TrRecCnt%RECORD_SECTOR_MAX_NUM == 0)			//д��������־λ
		{
			uStatus[0] = SEC_FULL;
			IOT_Mutex_SPI_FLASH_WriteBuff(uStatus, u32Addr+RECORD_ITEM_SIZE-SEC_SIGN_ADDR,1);
		}
//		IWDG_Feed();
	}
}

/****************************************************************
  *     ���������ݼ�¼
  *		ItemPoint:Ҫ���������ָ��
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
		/* ֻ��һ����¼����� */
		if((packAll == 0) || (packAll > MAX_PACK) ||((packAll == 1) && (packCur == 0)))
		{
			IOT_Printf("\r\n\r\n==================================\r\n");
			IOT_Printf(" readRecord() ���Ѵ洢0x04 ���� ���� = %d ����¼\r\n", 1);
			IOT_Printf("==================================\r\n\r\n");
			packLen = buf[RECORD_LAST_LEN];
			memcpy(tmp, buf, packLen);

			return buf[RECORD_LAST_LEN];
		}
		else if(packCur != 0)/* ��ǰ��¼���������� */
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
			/* û���ҵ���Ҫƴ�ӵļ�¼ */
			if(normalFlag != 1)
			{
				return 0;
			}
		}
		/* ��ʼƴ������ */
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

		for(i = 1; i < MAX_PACK; i++)			//������������
		{
			if(Read_RecordItem(buf) == 0)
			{
				packAll = buf[RECORD_PACK_ALL];
				packCur = buf[RECORD_PACK_CUR];
																//����ռ��ҳ,Ϊ0,1,2
				if((packAll == packAllPrev) && (packCur == (packCurPrev + 1)))
				{
					packAllPrev = packAll;
					packCurPrev = packCur;
					packLen = buf[RECORD_LAST_LEN];

					memcpy(&tmp[len], buf, packLen);		//�����ڴ�
					len += packLen;


					if(packAll == (packCur+1))
					{
						crc = Modbus_Caluation_CRC16(tmp, len - 2);

						if(MERGE(tmp[len-2], tmp[len-1]) == crc)
						{
							/* ���洢�ļ�¼�ɼ���SN�͵�ǰ�Ƿ�һ�� */
							if(memcmp(&tmp[8],&SysTAB_Parameter[Parameter_len[REG_SN]],10) != 0)			//����Ĳ��ǵ�ǰ�����SN����ɼ���SN
								return 0;
							/* ���洢�ļ�¼�����SN�͵�ǰ�Ƿ�һ�� */
							if(memcmp(&tmp[18], (char *)InvCommCtrl.SN, 10) != 0)			//����Ĳ��ǵ�ǰ�����SN����ɼ���SN
								return 0;

							return len - 2;
						}
					}
					continue;
				}
			}

			return 0;			//��ʧ��
		}
		IOT_Printf("\r\n\r\n==================================\r\n");
		IOT_Printf(" readRecord() ���Ѵ洢0x04 ���� ���� = %d ����¼\r\n", i);
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
	/* ������δ�� && ��¼Ϊ��*/
	if((Flag_RecordFull==0) && (u32TrRecCnt==0))
	{
		IOT_Printf("*****Read_RecordItem***************(Flag_RecordFull==0) && (u32TrRecCnt==0)**************************\r\n");
		return 1;
	}
	/* ������δ�� && �Ѷ�>=�Ѵ� */
	if((Flag_RecordFull==0) && (u32TrOutCnt >= u32TrRecCnt))
	{
		IOT_Printf("*****Read_RecordItem*************(Flag_RecordFull==0) && (u32TrOutCnt >= u32TrRecCnt)**************\r\n");

		//ClrAllTransRecord();
		return 1;
	}
	/* ���������� && (�Ѷ�-16)<=�Ѵ� */						//��������ͷ��ʼ��ģ���׷�������ڶ��ģ�����������ݶ�ʧ���ɱ���
	if(
			(Flag_RecordFull==1) &&
			(((u32TrOutCnt/RECORD_SECTOR_MAX_NUM) < (u32TrRecCnt/RECORD_SECTOR_MAX_NUM)) ||
				(((u32TrOutCnt/RECORD_SECTOR_MAX_NUM) == (u32TrRecCnt/RECORD_SECTOR_MAX_NUM)) &&
					(u32TrRecCnt%RECORD_SECTOR_MAX_NUM != 0)))
	  )									//��¼������&&(�Ѷ�-16)<=�Ѵ棬�����ᵼ�µ�ǰ�������ݶ�ʧ��������һ����
		{
				u32TrOutCnt = ((u32TrRecCnt/RECORD_SECTOR_MAX_NUM) + 1) * RECORD_SECTOR_MAX_NUM;
		}
	/* ���������� && �Ѷ�>=��ߴ洢��Ŀ */
	if(u32TrOutCnt >= RECORD_AMOUNT_NUM)
	{
			u32TrOutCnt = 0;
			Flag_RecordFull = 0;		//�Ѷ�ѭ�����Ѿ�׷���Ѵ��ѭ��

			uStatus[0] = 0;
			W25QXX_Erase_Sector(RECORD_FULL);			//�����¼���ı�ʾ
	}

	u32Addr = RECORD_ADDR + (u32TrOutCnt * RECORD_ITEM_SIZE);
	IOT_Mutex_SPI_FLASH_BufferRead(ItemPoint, u32Addr, RECORD_LAST_LEN+1);

	uStatus[0] = PAGE_RD;				//��¼�Ѷ�
	IOT_Mutex_SPI_FLASH_WriteBuff(uStatus, u32Addr+RECORD_STA_SINGLE, 1);		//д��־����ʱδ���д�����ݱ�Ϊ0

	u32TrOutCnt++;


	if((u32TrOutCnt%RECORD_SECTOR_MAX_NUM == 0) && (u32TrOutCnt != 0))			//һ����������
	{
		uStatus[0] = SEC_READ;
		IOT_Mutex_SPI_FLASH_WriteBuff(uStatus, u32Addr + RECORD_ITEM_SIZE - SEC_SIGN_ADDR, 1);		//����һ��������д��־����ʱδ���д�����ݱ�Ϊ0
	}


	crc161 = GetCRC16(ItemPoint,RECORD_ITEM_SIZE_TRUE+2);
	crc162 = ItemPoint[RECORD_ITEM_SIZE_TRUE+2];
	crc162 = (crc162 << 8) | ItemPoint[RECORD_ITEM_SIZE_TRUE+3];

	if(crc161 != crc162)
	{
		IOT_Printf("****Read_RecordItem***********(crc161 != crc162)******************\r\n");
		return 1;
	}
	/* У������׼ȷ�� */
	return 0;

}

/****************************************************************
  *     ��ȡϵͳ�Ѵ棬�Ѷ��ļ�¼��״̬
  *		��ȡ�Ѷ����Ѵ�ָ���λ��
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

	for(i=0; i<=RECORD_AMOUNT_NUM; i++) // ��ȡflash�������ݼ�¼
	{

		IOT_Mutex_SPI_FLASH_BufferRead(pbuf,RECORD_ADDR+i*RECORD_ITEM_SIZE,RECORD_ITEM_SIZE_TRUE+5);		//����־λ
		uStatus[0] = pbuf[RECORD_ITEM_SIZE_TRUE+4];

		crc16Check1 = pbuf[RECORD_ITEM_SIZE_TRUE+2];
		crc16Check1 = (crc16Check1 << 8) | pbuf[RECORD_ITEM_SIZE_TRUE+3];
		crc16Check2 = GetCRC16(pbuf,RECORD_ITEM_SIZE_TRUE+2);
	//	ESP_LOGI(SYS,"\r\n crc16Check1 =%d ...crc16Check2=%d  i=%d\r\n",crc16Check1,crc16Check2,i);

		if(crc16Check1 == crc16Check2)		//У����ȷ
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
				if(flag_FirstFind == FLAG_RD)		//����ʼ���Ѷ�
				{
					if(uStatus[0] == PAGE_RD)
						u32TrOutCnt = i+1;
				}
			}
			else
			{
				flag_BrokenPoint = 1;
				if(flag_FirstFind == FLAG_RD)		//����ʼ���Ѷ�
				{
					if(flag_Break == 0)				//�ʼ��0������û�д�0-1��
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
			break;					//RD��ǰ�棬˵��δ��һ�����ڣ�����0xff˵������.
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
//		u32TrRecCnt += 1;			//û���ó������Ƿ�ֹ��¼Ϊ0�����
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
  *     ��ȡ��ǰ�洢��Ŀ
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
//FLASH���������ܴ��ڵļ���״̬
//δд��һ��ѭ��

|-----------|----------------|-------------------|		��------|
	0x00	�Ѷ�		0x5a	�Ѵ�		0xff||0x00			|
																|
|----------------------------|-------------------|				|
	0x00					�Ѵ�		0xff					|
							�Ѷ�								|
																|
																|
																|
//��д��һ��ѭ��												|
|------------|---|-------------|-------------------|			|
	0x5a	�Ѵ�	0xff	0x00  �Ѷ�		0x5a				|
																|
|------------------------------|--|-----------------|			|
							  �Ѵ�0xff							|
				0x5a		  �Ѷ�		0x5a					|
																|
|------------|-----------------|-------------------|		____|
	0x00	�Ѷ�		0x5a  	  �Ѵ�		0x00

|------------|---|-------------|-------------------|
	0x00	�Ѷ�0x5a	0x5a  �Ѵ�		0x5a
*/
/****************************************************************
  *     У���ѱ���flash�����Ƿ�Ϊͬһ�������SN��ɼ���SN���������
****************************************************************/
void FlashData_Check(uint8_t *buf)
{
	/* 1�����洢�ļ�¼�ɼ���SN�͵�ǰ�Ƿ�һ�� 2�����洢�ļ�¼�����SN�͵�ǰ�Ƿ�һ�� */

	if((memcmp(&buf[8],&SysTAB_Parameter[Parameter_len[REG_SN]],10) != 0) || \
		(memcmp(&buf[18], (char *)InvCommCtrl.SN, 10) != 0))
	{
			ClearAllRecord(); // SN�Ų�ƥ�䣬�������flash���ݼ�¼
	//		SystemRestart();  // �����flash���ݣ������ɼ���
			IOT_Printf("FlashData_Check ����\r\n");
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
		if( (System_state.Wifi_state==0 )&& (IOT_WiFiScan_uc(ESP_READ ,0 ) ==ESP_OK1 ))  		// ��ʱɨ�踽��wifi
		{
			//ESP_LOGI(SYS, "IOT_WiFiScan_uc 180 sec \r\n" );
			IOT_WiFiScan_uc(ESP_WRITE , ScanIntervalTime ); //ɨ��ʱ������Ϊ ��̬  20220810 chen

			if(ESP_OK1 == IOT_WiFiConnectTimeout_uc(ESP_READ , 0 ))		//WIFI���ӳ�ʱ
			{
				System_state.Wifi_ActiveReset &= ~WIFI_CONNETING;
			}

			if((System_state.Wifi_ActiveReset  & WIFI_CONNETING) || (1 == System_state.SmartConfig_OnOff_Flag))	 //��ǰAPP ����������wifi ������ɨ��
			{																									//��������˱�׼ģʽ,��ɨ��

			}
			else if(System_state.wifi_onoff==1)
			{
				IOT_WIFIEvenSetSCAN();
			}
		}
		if((IOT_SYSRESTTime_uc(ESP_READ ,0 ) ==ESP_OK1) &&(  System_state.SYS_RestartFlag==1 ))  // �����豸
		{
			ESP_LOGI(SYS, "\r\n\r\n\r\n\r\n\r\nIN IOT_SysTEM_Task IOT_SYSRESTART System_state.SYS_RestartFlag= %d !!!\r\n\r\n\r\n\r\n\r\n",System_state.SYS_RestartFlag);

			IOT_SYSRESTART();
		}
#if test_wifi
		if((IOT_BLEofftime_uc(ESP_READ,0)== ESP_OK1) && (System_state.ble_onoff==1 ))
		{
			ESP_LOGI(SYS, "BLE is being close!!!");
			IOT_GattsEvenOFFBLE();   		// �����ر�����
		}
#endif
		if(InvCommCtrl.CMDFlagGrop & CMD_0X18  )
		{
		//	IOT_CollectorParam_Set();
			InvCommCtrl.CMDFlagGrop &= ~CMD_0X18;
			IOT_GattsEvenSetR();    				// ����Ӧ��APP 18����
		}
		else if(InvCommCtrl.CMDFlagGrop & CMD_0X19)   	// ��������ѯ 0x19 ����  ͬ��һЩ�Ĵ�������
		{
			switch(System_state.ServerGET_REGAddrStart)
			{
				case 31 :	//��ȡʱ��
					IOT_CollectorParam_Get(31,NULL);
					InvCommCtrl.CMDFlagGrop &= ~CMD_0X19;
					IOT_GattsEvenSetR();
					break;
				case 75 :
					//�� scan ����
					break;

				default:
					InvCommCtrl.CMDFlagGrop &= ~CMD_0X19;
					IOT_GattsEvenSetR();    				// ����Ӧ��APP 18����

					break;
			}
		}
		if(InvCommCtrl.CMDFlagGrop & CMD_0X26)
		{
			InvCommCtrl.CMDFlagGrop &= ~CMD_0X26;
			/**  ��������ж� ִ��λ��δ��,��Ҫ�Ż�**/
			IOT_GattsEvenSetR();    				// ����Ӧ��APP 18����
		}
		if((IOT_IPPORTSet_us(ESP_READ ,0 ) ==ESP_OK1) && (uSetserverflag==1) && (System_state.Wifi_state==1 ) ) // Զ�����÷�����
		{
			System_state.Server_Online = 0;		//����������״̬��λ,���������µķ������ɹ��󱣴������IP�Ͷ˿�

			IOT_Printf(" close old sever \r\n ");

			if((1 == System_state.Mbedtls_state) || (1 == System_state.SSL_state))
			{
				IOT_Connet_Close();
				System_state.Mbedtls_state = 0;
				System_state.Mbedtls_state = 0;

			}

			IOT_IPPORTSet_us(ESP_WRITE ,300); //5����������һ��ʧ��
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
				//Write_RecordItem(TxBuf);		//���߱���

				memcpy(TxBuf , InvCommCtrl.SN , 10);
				TxBuf[10] = InvCommCtrl.PVClock_Time_Tab[0]; 			//��ʷ���� ������������꣬����ʱ����
				TxBuf[11] = InvCommCtrl.PVClock_Time_Tab[1];
				TxBuf[12] = InvCommCtrl.PVClock_Time_Tab[2];

				TxBuf[13] = InvCommCtrl.PVClock_Time_Tab[3];
				TxBuf[14] = InvCommCtrl.PVClock_Time_Tab[4];
				TxBuf[15] = InvCommCtrl.PVClock_Time_Tab[5];

				DataLen = InputGrop_DataPacket(&TxBuf[16]);
				memcpy(&TxBuf[16 + 28] , &TxBuf[10], 6);

				IOT_ESP_SPI_FLASH_ReUploadData_Write(TxBuf,  DataLen + 16);	//Ԥ��16�ֽ��������Ᵽ����������кź�ʱ��

				ESP_LOGI(SYS,"������������ DataLen = %d \r\n" , DataLen);
#endif
			}
		}

		if((IOT_Historydate_us(ESP_READ,0)==ESP_OK1 )  && (System_state.Pv_state==1 ) && ( System_state.Server_Online==1  ) )
		{
			//if(Get_RecordNum()>0)
			if(IOT_Get_Record_Num() > 0)
			{
				System_state.upHistorytimeflag=1;
			//	IOT_Historydate_us(ESP_WRITE,10);  //10��һ����ʷ����
			}
			if(uAmmeter.Flag_GEThistorymeter>0)
			{
				IOT_Histotymeter_Timer();
			//IOT_Historydate_us(ESP_WRITE,10);  //10��һ����ʷ����
			}
			IOT_Historydate_us(ESP_WRITE,10);  //10��һ����ʷ����
		}
#if BLE_Readmode
		if(  System_state.BLE_ReadDone > 0)  // д�¼���λ
	    {
	    	if(((  System_state.BLE_ReadDone ==1) &&( GetSystemTick_ms() - System_state.BLEread_waitTime )>100) ||
	    	  ((  System_state.BLE_ReadDone ==2) &&( GetSystemTick_ms() - System_state.BLEread_waitTime )>3000)
	    		)  //ʵ�ʴ��ڷ��գ����ճ�ʱʵ���п������������յľ��룬Ӱ��  ,���Լ��źż�⣬��ʱ����Ӧ�ְ��ʻ����
	    	{
	    		 ESP_LOGE(SYS, "IOT_GattsEvenGATTS_WRITE waittime=%d, Get=%d" , System_state.BLEread_waitTime , GetSystemTick_ms());
	    		 System_state.BLE_ReadDone = 0;
	    		 IOT_GattsEvenGATTS_WRITE();
	    	}
	    }
#endif

		IOT_Update_Process_Judge();   //��������

		/* ��׼�������� */
		if((IOT_GetTick() - uiTime > 10 * 1000) && (1 == System_state.wifi_onoff))	//ÿ 10 s ��ѯһ��
		{
			uiTime = IOT_GetTick();

			if(0 == System_state.ble_onoff)
			{
				if(0 == System_state.SmartConfig_OnOff_Flag )//��׼����ģʽ�ر�
				{
					//��׼ģʽ��ȡ��wifi�����  System_state.SmartConfig_OnOff_Flag = 0  �� wifi���ܻ�û���������� , �з�����Ҫ�Ż�
					wifi_config_t conf_t = {0};
					esp_wifi_get_config(ESP_IF_WIFI_STA , &conf_t);
					if(0x00 == conf_t.sta.ssid[0])	//��config Ϊ��
					{
						System_state.SmartConfig_OnOff_Flag = 1;
						IOT_WIFIEven_StartSmartConfig();	//������׼����ģʽ
					}else
					{
						ESP_LOGI(SYS, " ��׼�����ر� ��������׼����ģʽ\r\n");
					}
				}
			}else
			{
				if(1 == System_state.SmartConfig_OnOff_Flag )//��׼����ģʽ�ѿ���
				{
					ESP_LOGI(SYS, "���� ���� ׼���رձ�׼����ģʽ\r\n");
					IOT_WIFIEven_CloseSmartConfig();
				}
			}
		}

		IOT_WIFI_SmartConfig_Close_Delay(0xff , 0);		//��׼���� ��ʱ �ر�


		if(ESP_OK1 == IOT_Heartbeat_Timeout_Check_uc(ESP_READ,0) && ( System_state.Server_Online == 1 ))
		{
			ESP_LOGI(SYS, "Heartbeat_Timeout ,Connet_Close !! \r\n");
			//System_state.Server_Online = 0;
			IOT_Connet_Close();
		}

		/*�ж������ʵʱ���ݶ�ȡ�Ƿ�ʱ*/
		if((1 == System_state.SmartConfig_OnOff_Flag) \
				|| (1 == System_state.ble_onoff) || (EFOTA_NULL != IOT_MasterFotaStatus_SetAndGet_ui(0xff , EFOTA_NULL)))
		{
			//��׼�������� , �������� , �������������,��Ҫ���ó�ʱʱ��,����������
			IOT_Inverter_RTData_TimeOut_Check_uc(ESP_WRITE,INVERTER_RT_DATA_TIMEOUT);  //����ʵʱ���ݳ�ʱʱ��
			IOT_WiFiConnect_Server_Timeout_uc(ESP_WRITE , CONNECT_SERVER_TIMEOUT);
		}
		else if(ESP_OK1 == IOT_Inverter_RTData_TimeOut_Check_uc(ESP_READ, 0))
		{
			//IOT_SYSRESTTime_uc(ESP_WRITE , 10 );  // 10s ������
			System_state.SYS_RestartFlag = 1;
		}
		else if(ESP_OK1 == IOT_WiFiConnect_Server_Timeout_uc(ESP_READ ,0))
		{
			//IOT_SYSRESTTime_uc(ESP_WRITE , 10 );  // 10s ������
			System_state.SYS_RestartFlag = 1;
		}
		else
		{
			if(1 == System_state.Pv_state)
			{
				//��׼�������� , �������� , �������������,��Ҫ���ó�ʱʱ��,����������
				IOT_Inverter_RTData_TimeOut_Check_uc(ESP_WRITE,INVERTER_RT_DATA_TIMEOUT);  //����ʵʱ���ݳ�ʱʱ��
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






