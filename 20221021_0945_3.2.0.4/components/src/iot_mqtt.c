/*
 * iot_mqtt.c
 *
 *  Created on: 2021��12��6��
 *      Author: Administrator
 */
#include "iot_mqtt.h"
#include "mqtt.h"
#include <string.h>
#include <stdio.h>
#include "iot_protocol.h"
#include "iot_inverter.h"
#include "iot_system.h"
#include "iot_fota.h"
#include "iot_ammeter.h"

//#include <stdlib.h>
//static const char *TAG = "MQTT";
char _g_cCWTopicCtoS[50] = "c/33/";			// MQTT ���������Ŀͻ��˷�����Ϣ���� �̶�
char _g_cCWTopicStoC[50] = "s/33/";			// MQTT ���������Ŀͻ��˷�����Ϣ���� �̶�
char _g_cMqttClientid[30] = "MQTTWIFI33";   // clientid, ÿ���豸Ψһ "LTE2011230" �ɼ������к�
char _g_cMqttUserName[30] = "MQTTWIFI33";   // �豸���к�

char _g_cMqttPassword[50] = "Growatt";  	// "version=2018-10-31&res=products%2F394201%2Fdevices%2FLTE2011230&et=1924167328&method=md5&sign=Y2kSOx4g6kAxK0sDByOKng%3D%3D"
//static char _g_cMqttData[512] = "2021-11-01 19:25:01";

MQTT_STATE Mqtt_state;
EMQTTREQ _g_EMQTTREQType;       // ƽ̨TCP������������
EMSG _g_EMSGMQTTSendTCPType;    // ƽ̨TCP�����������ͣ�ʵʱ����/������/����/��ȡ...��

uint16_t MQTT_Packet_ID;
uint16_t MQTT_Packet_ACKID;
uint16_t MQTT_Packet_ACKQOS;

//uint32_t 	Mqtt_PINGTime    = 60*1 ;  	// MQTT ����ʱ�� ��λ��
//uint32_t 	Mqtt_DATATime    = 5;		// MQTT �ϱ�040 ���� ʱ�� ��λ��
//uint32_t 	Mqtt_03DATATime  = 3;
//uint32_t	Mqtt_AgainTime	 = 3;			// �ش�����ʱ�� ��λ��
//uint32_t	Mqtt_Again_PASS	 = 0xFF;	    // �ش�ȡ��


/************************************************
 * SSL���ݷ��� ����MQTT���
 * ucSend_Type MQTT ���͵�����  ��¼/����/����
 * pucDataBuf_tem   ���͵�����BUF
 * **********************************************/
unsigned int  IOT_MqttDataPack_ui(unsigned char ucSend_Type,   unsigned char *pucDataBuf_out, unsigned char * pSendTXbuf_in, uint16_t SendTXLen )
{
	uint16_t uiMQTTSendLen = 0;        	// mqtt ���ݷ��ͳ���
	char  pcMQTTTopic[100] =  {0};		// mqtt ���ݷ������⣬�ͻ��˸��ݲ�ͬ����������������(Ĭ�Ϸ���ʵʱ����)

	memcpy(_g_cMqttClientid , &SysTAB_Parameter[Parameter_len[REG_SN]] , Collector_Config_LenList[REG_SN] );
	memcpy(_g_cMqttUserName , &SysTAB_Parameter[Parameter_len[REG_SN]] , Collector_Config_LenList[REG_SN] );

	switch(ucSend_Type)
	{
		case MQTT_CONTROL_CONNECT: // MQTT ��¼  1
		uiMQTTSendLen = mqtt_pack_connection_request(pucDataBuf_out, TCP_DATABUF_LEN,
																	_g_cMqttClientid,
																	NULL, NULL, 0,
																	_g_cMqttUserName,
																	_g_cMqttPassword,
																	0, MQTT_PINGTIME);
			break;
		case MQTT_CONTROL_SUBSCRIBE: // MQTT ���ⶩ�� 8
		 	sprintf(pcMQTTTopic,"%s%s", _g_cCWTopicStoC,_g_cMqttClientid); // �õ���������
		 	if(++MQTT_Packet_ID>=65535)
			{
		 		MQTT_Packet_ID=0;
			}
		 	uiMQTTSendLen = mqtt_pack_subscribe_request(pucDataBuf_out, TCP_DATABUF_LEN,
		 														 MQTT_Packet_ID,
																 pcMQTTTopic, 1,
//																 _g_cCWTopicStoC_Real, 1,
			                                                     NULL);
			break;
		case MQTT_CONTROL_PUBLISH:  // MQTT ���ݷ��� 3  (ע��qos ��= 0 ʱ�����ĲŻ��б�ʶ��ֵ)
			if(++MQTT_Packet_ID>=65535)
			{
				MQTT_Packet_ID=0;
			}
			sprintf(pcMQTTTopic,"%s%s", _g_cCWTopicCtoS,_g_cMqttClientid); 		// �õ���������
			uiMQTTSendLen = mqtt_pack_publish_request(pucDataBuf_out,TCP_DATABUF_LEN,
			                                                   pcMQTTTopic,
															   MQTT_Packet_ID,
															   pSendTXbuf_in,SendTXLen, (MQTT_QoS<<1));   //(MQTT_QoS<<1)
			break;
		case MQTT_CONTROL_PINGREQ:  // MQTT �������� 12
			uiMQTTSendLen = mqtt_pack_ping_request(pucDataBuf_out, TCP_DATABUF_LEN);
			break;

		case MQTT_CONTROL_PINGRESP:
			uiMQTTSendLen = mqtt_pack_ping_requestACK(pucDataBuf_out, TCP_DATABUF_LEN);
			break;
		case MQTT_CONTROL_PUBACK:	// MQTT ACK  4
			IOT_Printf("@@**  MQTT_Packet_ACKID  = %d **@@ \r\n",MQTT_Packet_ACKID);
			uiMQTTSendLen = mqtt_pack_pubxxx_request(pucDataBuf_out ,
													TCP_DATABUF_LEN,
													MQTT_CONTROL_PUBACK,
													MQTT_Packet_ACKID);
			break;
		default:
			break;
	}

	return uiMQTTSendLen;
}

/****************************************************
 * ��ʱ�ϱ�����ʱ��
 * ucMode_tem ��ʱ�ϱ�ʱ�� ����ģʽ
 * uiSetTime_tem �ϱ���ʱ�����  �ڶ�ʱ����
 * **************************************************/
uint8_t IOT_TCPDelaytimeSet_uc(uint8_t ucMode_tem, uint32_t uiSetTime_tem)
{
	if((uiSetTime_tem)&& (ucMode_tem == ESP_WRITE ))   // д0 ����ֵд���� 0��ֵ�Ÿ�ֵ
	{
		 Mqtt_state.MqttDATA_Time = uiSetTime_tem;
	}
	else if(ucMode_tem == ESP_WRITE)
	{
		if(Mqtt_state.MqttDATA_Time>0)
		 Mqtt_state.MqttDATA_Time = Mqtt_state.MqttDATA_Time-1;    	 //MQTT 04����
		if(Mqtt_state.MqttPING_Time>0)
		 Mqtt_state.MqttPING_Time =Mqtt_state.MqttPING_Time-1;       //MQTT ������
		if(Mqtt_state.Inverter_CMDTime>0)
		 Mqtt_state.Inverter_CMDTime =Mqtt_state.Inverter_CMDTime -1;//�������ѵʱ��
		if(Mqtt_state.AgainTime>0 && Mqtt_state.AgainTime!=0xFF)
		 Mqtt_state.AgainTime =Mqtt_state.AgainTime -1;				 //MQTT ��Ӧ�� �ط�����
		if(Mqtt_state.WifiScan_Time>0 )
		 Mqtt_state.WifiScan_Time=Mqtt_state.WifiScan_Time-1;	 	 //WiFi ����
		if(Mqtt_state.WifiConnect_Timeout > 0 )
			Mqtt_state.WifiConnect_Timeout = Mqtt_state.WifiConnect_Timeout-1;	 	 //WiFi ����
		if(Mqtt_state.IPPORTset_Time >0 )
		 Mqtt_state.IPPORTset_Time=Mqtt_state.IPPORTset_Time-1;	 	 //�޸� IP �� PORT
		if(Mqtt_state.RestartTime >0 )								 //�豸������ʱ��
		Mqtt_state.RestartTime =Mqtt_state.RestartTime -1;
		if(Mqtt_state.InverterGetSN_Time >0 )							 //��������кŻ�ȡ  /����
		 Mqtt_state.InverterGetSN_Time =Mqtt_state.InverterGetSN_Time-1;
		if(Mqtt_state.BLEOFF_Time > 0)
		Mqtt_state.BLEOFF_Time =Mqtt_state.BLEOFF_Time-1;

		if(Mqtt_state.Historydate_Time > 0)
		Mqtt_state.Historydate_Time =Mqtt_state.Historydate_Time-1;

		if(Mqtt_state.Heartbeat_Timeout > 0)				//���������ͳ�ʱʱ��
		{
			Mqtt_state.Heartbeat_Timeout--;
		}

		if(Mqtt_state.UpgradeDelay_Time > 0)				//������ʱ
		{
			Mqtt_state.UpgradeDelay_Time--;
		}
		if(Mqtt_state.InverterRTData_TimeOut > 0)
		{
			Mqtt_state.InverterRTData_TimeOut--;
		}
		if(Mqtt_state.WifiConnect_Sever_Timeout > 0)
		{
			Mqtt_state.WifiConnect_Sever_Timeout--;
		}
	}
	else if(ucMode_tem == ESP_READ)           // ��ѯ��ʱʱ��
	{
		if(Mqtt_state.MqttDATA_Time == 0)
		{
			return ESP_OK1 ; // ��ʱ����
		}
	}
	return ESP_FAIL1;
}

uint8_t IOT_PINGRESPSet_uc(uint8_t ucMode_tem, uint32_t uiSetTime_tem)
{
	if((uiSetTime_tem)&& (ucMode_tem == ESP_WRITE ))
	{
		Mqtt_state.MqttPING_Time= uiSetTime_tem;
	}
	else if(ucMode_tem == ESP_READ)           // ��ѯ��ʱʱ��
	{
		if(Mqtt_state.MqttPING_Time == 0)
		{
			return ESP_OK1 ; // ��ʱ����
		}
	}
	return ESP_FAIL1;
}

uint8_t IOT_InCMDTime_uc(uint8_t ucMode_tem, uint32_t uiSetTime_tem)
{
	if((uiSetTime_tem)&& (ucMode_tem == ESP_WRITE ))
	{
		Mqtt_state.Inverter_CMDTime= uiSetTime_tem;
	}
	else if(ucMode_tem == ESP_READ)           // ��ѯ��ʱʱ��
	{
		if(Mqtt_state.Inverter_CMDTime == 0)
		{
			return ESP_OK1 ; // ��ʱ����
		}
	}
	return ESP_FAIL1;
}
uint8_t IOT_INVgetSNtime_uc(uint8_t ucMode_tem, uint32_t uiSetTime_tem)
{
	if((uiSetTime_tem)&& (ucMode_tem == ESP_WRITE ))
	{
		Mqtt_state.InverterGetSN_Time = uiSetTime_tem;
	}
	else if(ucMode_tem == ESP_READ)           // ��ѯ��ʱʱ��
	{
		if(Mqtt_state.InverterGetSN_Time == 0)
		{
			return ESP_OK1 ; // ��ʱ����
		}
	}
	return ESP_FAIL1;
}

uint8_t IOT_WiFiScan_uc(uint8_t ucMode_tem, uint32_t uiSetTime_tem)
{
	if((uiSetTime_tem)&& (ucMode_tem == ESP_WRITE ))
	{
		Mqtt_state.WifiScan_Time= uiSetTime_tem;
	}
	else if(ucMode_tem == ESP_READ)           // ��ѯ��ʱʱ��
	{
		if(Mqtt_state.WifiScan_Time == 0)
		{
			return ESP_OK1 ; // ��ʱ����
		}
	}
	return ESP_FAIL1;
}

uint8_t IOT_UpgradeDelayTime_uc(uint8_t ucMode_tem, uint32_t uiSetTime_tem)
{
	if((uiSetTime_tem)&& (ucMode_tem == ESP_WRITE ))
	{
		Mqtt_state.UpgradeDelay_Time =  uiSetTime_tem;
	}
	else if(ucMode_tem == ESP_READ)           // ��ѯ��ʱʱ��
	{
		if(Mqtt_state.UpgradeDelay_Time == 0)
		{
			return ESP_OK1 ; // ��ʱ����
		}
	}
	return ESP_FAIL1;
}



uint8_t IOT_WiFiConnectTimeout_uc(uint8_t ucMode_tem, uint32_t uiSetTime_tem)
{
	if((uiSetTime_tem)&& (ucMode_tem == ESP_WRITE ))
	{
		Mqtt_state.WifiConnect_Timeout= uiSetTime_tem;
	}
	else if(ucMode_tem == ESP_READ)           // ��ѯ��ʱʱ��
	{
		if(Mqtt_state.WifiConnect_Timeout == 0)
		{
			return ESP_OK1 ; // ��ʱ����
		}
	}
	return ESP_FAIL1;
}


uint8_t IOT_WiFiConnect_Server_Timeout_uc(uint8_t ucMode_tem, uint32_t uiSetTime_tem)
{
	if((uiSetTime_tem)&& (ucMode_tem == ESP_WRITE ))
	{
		Mqtt_state.WifiConnect_Sever_Timeout= uiSetTime_tem;
	}
	else if(ucMode_tem == ESP_READ)           // ��ѯ��ʱʱ��
	{
		if(Mqtt_state.WifiConnect_Sever_Timeout == 0)
		{
			return ESP_OK1 ; // ��ʱ����
		}
	}
	return ESP_FAIL1;
}


uint8_t IOT_SYSRESTTime_uc(uint8_t ucMode_tem, uint32_t uiSetTime_tem)
{
	if((uiSetTime_tem)&& (ucMode_tem == ESP_WRITE ))
	{
		Mqtt_state.RestartTime= uiSetTime_tem;
	}
	else if(ucMode_tem == ESP_READ)           // ��ѯ��ʱʱ��
	{
		if(Mqtt_state.RestartTime == 0)
		{
			return ESP_OK1 ; // ��ʱ����
		}
	}
	return ESP_FAIL1;
}
uint8_t IOT_BLEofftime_uc(uint8_t ucMode_tem, uint32_t uiSetTime_tem)
{
	if((uiSetTime_tem)&& (ucMode_tem == ESP_WRITE ))
	{
		Mqtt_state.BLEOFF_Time= uiSetTime_tem;
	}
	else if(ucMode_tem == ESP_READ)           // ��ѯ��ʱʱ��
	{
		if(Mqtt_state.BLEOFF_Time == 0)
		{
			return ESP_OK1 ; 			// ��ʱ����
		}
	}
	return ESP_FAIL1;
}
//������ѵ�������ݲ�ѯ
uint8_t IOT_AgainTime_uc(uint8_t ucMode_tem, uint32_t uiSetTime_tem)
{
	if((uiSetTime_tem)&& (ucMode_tem == ESP_WRITE ))
	{
		Mqtt_state.AgainTime = uiSetTime_tem;
	}
	else if(ucMode_tem == ESP_READ)           // ��ѯ��ʱʱ��
	{
		if(Mqtt_state.AgainTime == 0)
		{
			return ESP_OK1 ; // ��ʱ����
		}
	}
	return ESP_FAIL1;
}

uint8_t IOT_IPPORTSet_us(uint8_t ucMode_tem, uint32_t uiSetTime_tem)
{
	if((uiSetTime_tem)&& (ucMode_tem == ESP_WRITE ))
	{
		Mqtt_state.IPPORTset_Time = uiSetTime_tem;
	}
	else if(ucMode_tem == ESP_READ)           // ��ѯ��ʱʱ��
	{
		if(Mqtt_state.IPPORTset_Time == 0)
		{
			return ESP_OK1 ; // ��ʱ����
		}
	}
	return ESP_FAIL1;

}

uint8_t IOT_Historydate_us(uint8_t ucMode_tem, uint32_t uiSetTime_tem)
{
	if((uiSetTime_tem)&& (ucMode_tem == ESP_WRITE ))
	{
		Mqtt_state.Historydate_Time = uiSetTime_tem;
	}
	else if(ucMode_tem == ESP_READ)           // ��ѯ��ʱʱ��
	{
		if(Mqtt_state.Historydate_Time == 0)
		{
			return ESP_OK1 ; // ��ʱ����
		}
	}
	return ESP_FAIL1;

}


uint8_t IOT_Heartbeat_Timeout_Check_uc(uint8_t ucMode_tem, uint32_t uiSetTime_tem)
{
	if((uiSetTime_tem)&& (ucMode_tem == ESP_WRITE ))
	{
		Mqtt_state.Heartbeat_Timeout = uiSetTime_tem;
	}
	else if(ucMode_tem == ESP_READ)           // ��ѯ��ʱʱ��
	{
		if(Mqtt_state.Heartbeat_Timeout == 0)
		{
			return ESP_OK1 ; // ��ʱ����
		}
	}
	return ESP_FAIL1;

}


uint8_t IOT_Inverter_RTData_TimeOut_Check_uc(uint8_t ucMode_tem, uint32_t uiSetTime_tem)
{
	if((uiSetTime_tem)&& (ucMode_tem == ESP_WRITE ))
	{
		Mqtt_state.InverterRTData_TimeOut = uiSetTime_tem;
	}
	else if(ucMode_tem == ESP_READ)           // ��ѯ��ʱʱ��
	{
		if(Mqtt_state.InverterRTData_TimeOut == 0)
		{
			return ESP_OK1 ; // ��ʱ����
		}
	}
	return ESP_FAIL1;

}

/******************************************************
 * FunctionName : IOT_MqttRev_ui
 * Description  :  MQTT���ݰ����ս���  ��ʲô��������
 * Parameters   : uint8_t *dst        : ����Ҫ����������server��������
                  uint16_t suiLen_tem : ���������������ܳ���
 * Returns      : 0 - ����ʧ��/1 - �����ɹ�
 * Notice       : none
*******************************************************/
uint8_t IOT_MqttRev_ui(unsigned char *dst, uint16_t suiLen_tem) // dst:MQTT_RXbuf,suiLen_tem: len;
{

	switch((dst[0]>>4))	   			 //
	{
		case MQTT_CONTROL_CONNACK:	 // MQTT��¼ ������Ӧ�� 2       server����ֵ��20 02 00 00 (���ȣ�4)
			printf("Jump here:MQTT_CONTROL_CONNACK!\r\n");
			printf("Packet identifier is unneeded.\r\n");
			printf("Payload is unneeded.\r\n");
			if((suiLen_tem == 4)&& \
			((dst[0] == 0x20)  && (dst[1] == 0x02)  && \
			((dst[2] == 0x00) || (dst[2] == 0x01))  ))
			{
				Mqtt_state.MQTT_connet = 1;
				IOT_MqttSendStep_Set(1,8); 	 					// ��¼������� -- ������������
				return 	1;
		    }
			break;
		case MQTT_CONTROL_SUBACK:   // MQTT���ⶩ��  ������Ӧ�� 9   90 3 0 a 1 (3����)  +����Qos+����Qos
			printf("Jump here:MQTT_CONTROL_SUBACK!\r\n");
			printf("Packet identifier is needed.\r\n");
			printf("Payload is needed.\r\n");
		// server����ֵ��90 03 (uint8_t)(MQTT_IDENTIFIER>>4) (uint8_t)MQTT_IDENTIFIER 00 (���ȣ�5) 00
			if((suiLen_tem == 5)&& \
			((dst[0] == 0x90) && (dst[1] == 0x03)  && \
			/*20220829  MQTT_Packet_ID ����4λ�ĳ� ���� 8λ*/
			( dst[2] == (uint8_t)(MQTT_Packet_ID >> 8))   && (dst[3] == (uint8_t)MQTT_Packet_ID) &&\
			((dst[4] == MQTT_QoS) || (dst[4] == 0x80))))
			{

//				_g_EMSGMQTTSendTCPType = EMSG_SYSTEM_POST;	 // MQTT��������������󣬶��巢��publishһ�����ݷ�������  �����ϱ��ɼ�����Ϣ
//				IOT_MqttSendStep_Set(1,3);                	 // ��������� -- ������������   MQTT_CONTROL_PUBLISH
//				��¼�ɹ�����������֮��  �Ż�  �ȷ�����  ͨ���������� �ϱ����ݡ�

				if(1 == System_state.Pv_state)				//����������ϴ�һ��03����
				{
					InvCommCtrl.HoldingSentFlag = 1 ;  		//20220826  �����ڶ�������������ϴ�03����
				}

				IOT_MqttSendStep_Set(1,12);
				return 	1;
			}
			break;
		case MQTT_CONTROL_PUBACK:   // MQTT���� 	  ������Ӧ�� 4      (ע��qos ��= 0 ʱ�����ĲŻ���ID )
			printf("Jump here:MQTT_CONTROL_PUBACK!\r\n");
			printf("Packet identifier is needed.\r\n");
			printf("Payload is unneeded.\r\n");
		// server����ֵ��40 02 (uint8_t)(MQTT_IDENTIFIER>>4) (uint8_t)MQTT_IDENTIFIER (���ȣ�4)
			if (IOT_MQTTPublishACK_uc(dst,suiLen_tem)== ESP_OK1)   //  03 / 04  / 19  / 18  �����ϱ�  ������ACK
			{
				IOT_AgainTime_uc(ESP_WRITE,Mqtt_Again_PASS);
			   	return 	1;
			}
			break;
		case MQTT_CONTROL_PINGREQ:  //  12
			printf("Jump here:MQTT_CONTROL_PINGREQ!\r\n");
			printf("Packet identifier is unneeded.\r\n");
			printf("Payload is unneeded.\r\n");
		//	if(	)
			{
				IOT_MqttSendStep_Set(1,13);
				return 	1;
			}

			break;
		case MQTT_CONTROL_PINGRESP: // MQTT��������  ������Ӧ�� 13    server����ֵ��D0 00 (���ȣ�2)
			printf("Jump here:MQTT_CONTROL_PINGRESP!\r\n");
			printf("Packet identifier is unneeded.\r\n");
			printf("Payload is unneeded.\r\n");
			if((suiLen_tem == 2)&& \
			((dst[0] == 0xD0) && (dst[1] == 0x00)))
			{
				IOT_PINGRESPSet_uc(ESP_WRITE,Mqtt_PINGTime);   // ����װ������ʱ��
				IOT_ServerFunc_0x16( );   //���� Ӧ���־λ��Ϊ
				return 1;
			}
			break;
		case MQTT_CONTROL_PUBLISH:  // MQTT �յ��������������ݣ�3x xx (���ȣ�x)
			printf("Jump here:MQTT_CONTROL_PUBLISH!\r\n");
			printf("Packet identifier is needed if Qos > 0.\r\n");
			printf("Payload is able to choose.\r\n");
			if( (dst[0] & 0x30) == 0x30)
			{
				printf("Not jump here!\r\n");
				MQTT_Packet_ACKQOS = (dst[0]& 0x06) >> 1; //0x32 & 0x06 == 0x00

				printf("MQTT_Packet_ACKQOS = %d\r\n",MQTT_Packet_ACKQOS);

				for(int i = 0 ; i < suiLen_tem ; i++)
					printf("%02x ",dst[i]);

				printf(" / **************************** \r\n");

				if( IOT_MQTTPublishReq_uc(dst, suiLen_tem,MQTT_Packet_ACKQOS)==ESP_OK1) //important
				{
					return 	1;
				}
			}
			break;
		default:
			printf("Jump here:default!\r\n");
			break;
	}
	return 0;
}

void IOT_SetUpHolddate(void)
{

	IOT_InCMDTime_uc(ESP_WRITE,1);          //��ѵ 03 ����
	InvCommCtrl.HoldingSentFlag = 1 ;  		// �����ϱ��� ���ò�����ͬ���ϴ�  //�Ż� �ȴ�3�����ϴ�  �ϴ�03
	InvCommCtrl.SetUpInvDATAFlag =0;

	IOT_TCPDelaytimeSet_uc(ESP_WRITE,5); // ���¸�ֵ  �����ϱ�ʱ�� --03 //
}

/************************************************************
 * MQTT ��ѵ��� �Ƿ���Ҫ��������
 * _g_EMQTTREQType MQTT �յ�������Ϣ ��ҪӦ��
 * ��λ����״̬ Mqtt_state.MqttSend_step = 3����  / 0 ����
 * **********************************************************/
uint8_t IOT_ReadyForData(void)
{
	static uint8_t Mqtt_Againnum=0;

	/* ��������ҪӦ��ʱ����Ӧ�� */
 	if(_g_EMQTTREQType) 			// IOT_ServerReceiveHandle  ���յ�������publish ����
	{
 		IOT_Printf( "*****IOT_ReadyForData** ****** _g_EMQTTREQType=%d*****System_state.Host_setflag=%x******\r\n",_g_EMQTTREQType,System_state.Host_setflag);

		switch(_g_EMQTTREQType)
		{
			case MQTTREQ_SET_TIME:
				break;
			case MQTTREQ_CMD_0x05 :
				if(System_state.Host_setflag & CMD_0X05 )   	// ��������ѯ��λ
				{
					if(!(InvCommCtrl.CMDFlagGrop & CMD_0X05 ))  // ���������Ӧ����λ  �ȴ� �����߳�������ݴ���
					{
						if(MQTT_Packet_ACKQOS && (InvCommCtrl.Set_InvRespondCode == PV_ACKOK))  //�����Ϣ����  > 1 �ظ� ACK �¼ӻ��ƣ�������ʧ�� ��Ӧ����������ACK �� publish
						{
							IOT_MqttSendStep_Set(1,4);			// ACK
						}
						else
						{
							_g_EMSGMQTTSendTCPType =  EMSG_GET_INVINFO_RESPONSE ;
							IOT_MqttSendStep_Set(1,3);			// publish
						}
						 _g_EMQTTREQType = MQTTREQ_NULL;		//
						 IOT_Printf( "************* MQTTREQ_CMD_0x05   Ready Send*************\r\n");
					}
				}
				break;
			case MQTTREQ_CMD_0x06 :
				if(System_state.Host_setflag & CMD_0X06 )   	// ������������λ
				{
					if(!(InvCommCtrl.CMDFlagGrop & CMD_0X06 ))  // ���������Ӧ����λ �ȴ� �����߳�������ݴ���
					{
						if(MQTT_Packet_ACKQOS  && (InvCommCtrl.Set_InvRespondCode == PV_ACKOK) ) //�����Ϣ����>1�ظ� ACK�¼ӻ��ƣ�������ʧ�� ��Ӧ����������ACK �� publish
						{
							IOT_MqttSendStep_Set(1,4);			// ACK
						}
						else
						{
							_g_EMSGMQTTSendTCPType =  EMSG_GET_INVREG_RESPONSE ;
							IOT_MqttSendStep_Set(1,3);			// publish
						}
						_g_EMQTTREQType = MQTTREQ_NULL;
						IOT_Printf( "************* MQTTREQ_CMD_0x06   Ready Send*************\r\n");

					    IOT_SetUpHolddate();
//						IOT_InCMDTime_uc(ESP_WRITE,1);          //��ѵ 03 ����
//						InvCommCtrl.HoldingSentFlag = 1 ;  		// �����ϱ��� ���ò�����ͬ���ϴ�  //�Ż� �ȴ�3�����ϴ�  �ϴ�03
//						IOT_TCPDelaytimeSet_uc(ESP_WRITE,5); // ���¸�ֵ  �����ϱ�ʱ�� --03 //
//						InvCommCtrl.SetUpInvDATAFlag =0;

					}
				}
				break;
			case MQTTREQ_CMD_0x10 :
				if(System_state.Host_setflag & CMD_0X10   )   	// ������������λ
				{
					if(!(InvCommCtrl.CMDFlagGrop & CMD_0X10 ))  // ���������Ӧ����λ �ȴ� �����߳�������ݴ���
					{
						if(MQTT_Packet_ACKQOS && (InvCommCtrl.Set_InvRespondCode == PV_ACKOK))    				//�����Ϣ����  > 1 �ظ�    ACK
						{
							IOT_MqttSendStep_Set(1,4);			// ACK
						}
						else
						{
							_g_EMSGMQTTSendTCPType =  EMSG_GET_INVREG_RESPONSE ;
							IOT_MqttSendStep_Set(1,3);			// publish
						}
						_g_EMQTTREQType = MQTTREQ_NULL;
						IOT_Printf( "************* MQTTREQ_CMD_0x10   Ready Send*************\r\n");
					}
				}
				break;
			case MQTTREQ_CMD_0x18 :
				if(System_state.Host_setflag & CMD_0X18)   		// ��������ѯ��λ
				{
				//	if(!(InvCommCtrl.CMDFlagGrop & CMD_0X18))   // ���������Ӧ����λ �ȴ� �����߳�������ݴ���
					{
						if(MQTT_Packet_ACKQOS)    				// �����Ϣ����  > 1 �ظ�    ACK
						{
							IOT_MqttSendStep_Set(1,4);			// ACK
						}
						else
						{
							_g_EMSGMQTTSendTCPType =  EMSG_GET_DLIREG_RESPONSE ;
							IOT_MqttSendStep_Set(1,3);			// publish
						}
						 _g_EMQTTREQType = MQTTREQ_NULL;
						 IOT_Printf( "************* MQTTREQ_CMD_0x18   Ready Send*************\r\n");

					}
				}
				break;
			case MQTTREQ_CMD_0x19 :
				if(System_state.Host_setflag & CMD_0X19 )   	// ��������ѯ��λ
				{
				//	if(!(InvCommCtrl.CMDFlagGrop & CMD_0X19 ))  // ���������Ӧ����λ �ȴ� �����߳�������ݴ���
					{
						if(MQTT_Packet_ACKQOS )    			// �����Ϣ����  > 1 �ظ� ACK �¼ӻ��ƣ�������ʧ�� ��Ӧ����������ACK �� publish
						{
							IOT_MqttSendStep_Set(1,4);			// ACK
						}
						else
						{
							_g_EMSGMQTTSendTCPType =  EMSG_GET_DLINFO_RESPONSE ;
							IOT_MqttSendStep_Set(1,3);			// publish
							_g_EMQTTREQType = MQTTREQ_NULL;
						}

						IOT_Printf( "************* MQTTREQ_CMD_0x19   Ready Send**MQTT_Packet_ACKQOS=%d***********\r\n",MQTT_Packet_ACKQOS);
					}
				}
				break;
			default:
				break;
		}
	  // IOT_MqttSendStep_Set(1,3); // ��������publish����

		if(Mqtt_state.MqttSend_step != 0x00)
		{
			IOT_Printf( "*******Mqtt_state.MqttSend_step=%d****** _g_EMSGMQTTSendTCPType=%d***********\r\n",Mqtt_state.MqttSend_step,_g_EMSGMQTTSendTCPType);

		   return ESP_READY;// ����׼������
		}
	}

 	/* �����ϴ����ݸ������� */
	if(_g_EMSGMQTTSendTCPType == EMSG_NULL )
	{
		if(IOT_PINGRESPSet_uc(ESP_READ,0)==ESP_OK1)          // ��������
		{
			IOT_AgainTime_uc(ESP_WRITE,Mqtt_AgainTime);
			IOT_PINGRESPSet_uc(ESP_WRITE,Mqtt_PINGTime);  	 //	���¸�ֵ����500ms��ѵ
			IOT_MqttSendStep_Set(1,12); 		  			 //	��������PING����
			IOT_Printf( "*************PING Ready Send*******Mqtt_PINGTime=%d******\r\n",Mqtt_PINGTime);
			return ESP_READY;	// ����׼������
		}
		else if((System_state.UploadParam_Flag == 1) && ( System_state.Server_Online == 1)) // ��������������ϱ��ɼ�������
		{
			IOT_Printf( "\r\nIOT_ReadyForData ESP_READY*******Mqtt_state.MqttSend_step=%d****** _g_EMSGMQTTSendTCPType=%d***********\r\n",Mqtt_state.MqttSend_step,_g_EMSGMQTTSendTCPType);

			 IOT_MqttSendStep_Set(1,3);
			 IOT_AgainTime_uc(ESP_WRITE,Mqtt_AgainTime);
			 _g_EMSGMQTTSendTCPType = EMSG_SYSTEM_POST;
			 IOT_Printf( "*************0x19 IOT_UploadParamSelf Ready Send*************\r\n");
			 return ESP_READY;	// ����׼������
		}
		else if( (System_state.updatetimeflag == 1) && (InvCommCtrl.HoldingSentFlag == 1 )  &&  (System_state.Server_Online==1 )  && (InvCommCtrl.SetUpInvDATAFlag == 1 )  )	  // ���豸��ȡ��03���� ��λ �����ϱ�0x03�����������Ҫ�Ż���
		{//&& (IOT_TCPDelaytimeSet_uc(ESP_READ,0)==ESP_OK1 )
			 IOT_MqttSendStep_Set(1,3);
			 IOT_AgainTime_uc(ESP_WRITE,Mqtt_AgainTime);	 // �����ش�ʱ��  --- ���AgainTime ֮ǰû��ACK �����½����ϱ�ʱ��׼�����
			 _g_EMSGMQTTSendTCPType = EMSG_INVPARAMS_POST;
			 IOT_Printf("\r\n*************0X03 Ready Send*************\r\n");
			 return ESP_READY;	// ����׼������
		}
		else if((System_state.updatetimeflag==1) && (System_state.Pv_state==1 )  )//	��ʱ�ϱ�0x04���������(IOT_TCPDelaytimeSet_uc(ESP_READ,0)==ESP_OK1 )
		{
			 System_state.updatetimeflag=0;
			 _g_EMSGMQTTSendTCPType=EMSG_ONLINE_POST;		 //	0x04��������
			 IOT_MqttSendStep_Set(1,3);						 //	��������publish����
			 IOT_AgainTime_uc(ESP_WRITE,Mqtt_AgainTime);	 // �����ش�ʱ��  --- ���AgainTime ֮ǰû��ACK �����½����ϱ�ʱ��׼�����
		   //IOT_TCPDelaytimeSet_uc(ESP_WRITE,Mqtt_DATATime);// ���¼�ʱ  --- Ӧ���� ACK ֮��
			 IOT_Printf( "*************0X04 Ready Send******Mqtt_DATATime=%d*******\r\n",Mqtt_DATATime);
			 return ESP_READY;	// ����׼������
		}
		else if(1 == IOT_UpdateProgress_Upload_Handle(0xff))		//��ѯ�Ƿ���Ҫ�ϴ���������		/*#include "iot_fota.h"*/
		{
			 _g_EMSGMQTTSendTCPType = EMSG_PROGRESS_POST;	 //	������������
			 IOT_MqttSendStep_Set(1,3);						 //	��������publish����
			 IOT_AgainTime_uc(ESP_WRITE,Mqtt_AgainTime);	 // �����ش�ʱ��  --- ���AgainTime ֮ǰû��ACK �����½����ϱ�ʱ��׼�����
		   //IOT_TCPDelaytimeSet_uc(ESP_WRITE,Mqtt_DATATime);// ���¼�ʱ  --- Ӧ���� ACK ֮��
			 IOT_Printf( "*************upload progress Ready Send******Mqtt_DATATime=%d*******\r\n",Mqtt_DATATime);
			 return ESP_READY;	// ����׼������
		}
#if	 test_wifi
		else if((System_state.upHistorytimeflag==1)  &&  (System_state.Server_Online==1 ) )
		{
			System_state.upHistorytimeflag=0;
			 _g_EMSGMQTTSendTCPType = EMSG_OFFLINE_POST;
			 IOT_MqttSendStep_Set(1,3);						 //	��������publish����
			 IOT_AgainTime_uc(ESP_WRITE,Mqtt_AgainTime);	 //
			 IOT_Printf( "*************upHistorytimeflag  Ready Send******Mqtt_DATATime=%d*******\r\n",Mqtt_DATATime);
			 return ESP_READY;	// ����׼������
		}
		else if((uAmmeter.UpdateMeterFlag ==2  ) && (System_state.Server_Online==1 )  )
		{
			 uAmmeter.UpdateMeterFlag=3;
			 _g_EMSGMQTTSendTCPType = EMSG_AMMTER_POST;
			 IOT_MqttSendStep_Set(1,3);						 //	��������publish����
			 IOT_AgainTime_uc(ESP_WRITE,Mqtt_AgainTime);	 //
			 IOT_Printf( "************* EMSG_AMMTER_POST Ready Send******Mqtt_DATATime=%d*******\r\n",Mqtt_DATATime);
			 return ESP_READY;	// ����׼������
		}
		else if((IOT_GET_Histotymeter_Flag()==2  ) && (System_state.Server_Online==1 )  )
		{
			 IOT_SET_Histotymeter_Flag(3);
			 _g_EMSGMQTTSendTCPType = EMSG_HISTORY_AMMTER_POST;
			 IOT_MqttSendStep_Set(1,3);						 //	��������publish����
			 IOT_AgainTime_uc(ESP_WRITE,Mqtt_AgainTime);	 //
			 IOT_Printf( "************* EMSG_HISTORY_AMMTER_POST Ready Send******Mqtt_DATATime=%d*******\r\n",Mqtt_DATATime);
			 return ESP_READY;	// ����׼������
		}
		else if( System_state.up14dateflag == 1 )
		{
			 //IOT_SET_Histotymeter_Flag(3);
			 _g_EMSGMQTTSendTCPType = EMSG_INVWAVE_POST;
			 IOT_MqttSendStep_Set(1,3);						 //	��������publish����
			 IOT_AgainTime_uc(ESP_WRITE,Mqtt_AgainTime);	 //
			 IOT_Printf( "************* EMSG_INVWAVE_POST Ready Send******Mqtt_DATATime=%d*******\r\n",Mqtt_DATATime);
			 return ESP_READY;	// ����׼������
		}
		else if( System_state.upBDC03dateflag == 1 )
		{
			 _g_EMSGMQTTSendTCPType = EMSG_BDC03_POST;
			 IOT_MqttSendStep_Set(1,3);						 //	��������publish����
			 IOT_AgainTime_uc(ESP_WRITE,Mqtt_AgainTime);	 //
			 IOT_Printf( "************* EMSG_BDC03_POST Ready Send******Mqtt_DATATime=%d*******\r\n",Mqtt_DATATime);
			 return ESP_READY;	// ����׼������
		}
		else if( System_state.upBDC04dateflag == 1 )
		{
			 _g_EMSGMQTTSendTCPType = EMSG_BDC04_POST;
			 IOT_MqttSendStep_Set(1,3);						 //	��������publish����
			 IOT_AgainTime_uc(ESP_WRITE,Mqtt_AgainTime);	 //
			 IOT_Printf( "************* EMSG_BDC04_POST Ready Send******Mqtt_DATATime=%d*******\r\n",Mqtt_DATATime);
			 return ESP_READY;	// ����׼������
		}
		else if( System_state.upBMS03dateflag == 1 )
		{
			 _g_EMSGMQTTSendTCPType = EMSG_BMS03_POST;
			 IOT_MqttSendStep_Set(1,3);						 //	��������publish����
			 IOT_AgainTime_uc(ESP_WRITE,Mqtt_AgainTime);	 //
			 IOT_Printf( "************* EMSG_BMS03_POST Ready Send******Mqtt_DATATime=%d*******\r\n",Mqtt_DATATime);
			 return ESP_READY;	// ����׼������
		}
		else if( System_state.upBMS04dateflag == 1 )
		{
			 _g_EMSGMQTTSendTCPType = EMSG_BMS04_POST;
			 IOT_MqttSendStep_Set(1,3);						 //	��������publish����
			 IOT_AgainTime_uc(ESP_WRITE,Mqtt_AgainTime);	 //
			 IOT_Printf( "************* EMSG_BMS04_POST Ready Send******Mqtt_DATATime=%d*******\r\n",Mqtt_DATATime);
			 return ESP_READY;	// ����׼������
		}
#endif
	}
	else
	{
		if( IOT_AgainTime_uc(ESP_READ,0)==ESP_OK1)			 //	�Ƿ���Է�����
		{
			 if(Mqtt_Againnum++ >5 )  //�ط�5�� �Ͳ�����
			 {
				 Mqtt_Againnum =0 ;
				 _g_EMSGMQTTSendTCPType = EMSG_NULL;
				 IOT_AgainTime_uc(ESP_WRITE,Mqtt_Again_PASS);
			 }
			 else
			 {
				 IOT_AgainTime_uc(ESP_WRITE,Mqtt_AgainTime + Mqtt_Againnum);
				 IOT_MqttSendStep_Set(1,3);
			 }
			 IOT_Printf(" ESP_READY  Mqtt_state.AgainTime =%d******\r\n",Mqtt_state.AgainTime);
			 return ESP_READY;	// ����׼������
		}
	}
	return ESP_ONREADY;// �����ݷ���
}

/*********************************************
 * MQTT ���Ͳ��� ����
 * Mqtt_state.MqttSend_step =  ��¼/����/����/����
 * ******************************************/
void IOT_MqttSendStep_Set(uint8_t ucMode_tem, uint8_t ucStep_tem)  //IOT_MqttSendStep_Set(1,1);
{
	if(ucMode_tem == 0)
	{
		if(Mqtt_state.MqttSend_step == 1)Mqtt_state.MqttSend_step = 8;         // ��¼������� -- ������������
		else if(Mqtt_state.MqttSend_step == 8)Mqtt_state.MqttSend_step = 3;    // �������ⷢ����� -- ��������
		else if(Mqtt_state.MqttSend_step == 3)Mqtt_state.MqttSend_step = 0;    // ���ݷ������ -- ��������
		else if(Mqtt_state.MqttSend_step == 12)Mqtt_state.MqttSend_step = 0;   // ����������� -- ��������
	}
	else if(ucMode_tem == 1 )
	{
		if((ucStep_tem == 0) || (ucStep_tem == 1))  // ���MQTT�������� / ����Ϸ�������ʼ����MQTT��¼������
		{
			printf("Jump here!\r\n");
			Mqtt_state.MqttSend_step = ucStep_tem;  // ��� / �õ�MQTT���ݷ��͵�����
		}
		else if(Mqtt_state.MqttSend_step == 0)  	// ��������ʱ��Ҫ��ǰ����������Ϊ�ղ������ã��������ݷ��ʹ���
		{
			Mqtt_state.MqttSend_step = ucStep_tem;  // �õ�MQTT���ݷ��͵�����
		}
	}
}

/******************************************************************************
 * FunctionName : IOT_MQTTPublishReq_uc
 * Description  : ���շ����� PUBLISH �����ж�
 * Parameters   : uint8_t *pucDst_tem : ����Ҫ����������server��������
                  uint16_t suiLen_tem : ���������������ܳ���
                  uint8_t uQoc : Qos
 * Returns      : 0 - ����ʧ�� / 1 - �����ɹ�
 * Notice       : none
*******************************************************************************/
uint8_t IOT_MQTTPublishReq_uc(uint8_t *pucDst_tem, uint16_t suiLen_tem, uint8_t uQoc)
{
	uint8_t ucHeadLen = 0;   	 // �̶�ͷ + ���ݰ������ܳ���  2/3byte
	uint8_t Publish_dataaddr=0;  // ʵ�ʷ��������ݿ�ʼ��ַ
	uint16_t usiDataLen =0; 	 // topic���� + topic�ַ��� + ��Ч�غ�-���� nbyte
	uint16_t Publish_Len=0;  	 // �ܰ�����
	uint16_t Publish_TopicLen=0; // �������ⳤ��
	uint16_t Publish_dataLen=0;  // ʵ�����ݵĳ��� = ��- �������� - ID
//	EMQTTREQ MQTTREQTypeVal=MQTTREQ_NULL;
	struct mqtt_response SMQTTResponse;
//	uint8_t *pPublishbuf = NULL;
//	uint8_t pcANDTopic[50];
//	uint8_t ucANDTopicLen=0;
//	sprintf(pcANDTopic,"%s%s", _g_cCWTopicStoC,_g_cMqttClientid); // �õ���������
//	ucANDTopicLen =strlen(pcANDTopic);  //�õ���������ĳ���
	ucHeadLen = mqtt_unpack_fixed_header(&SMQTTResponse, (const uint8_t*)pucDst_tem, suiLen_tem);
	usiDataLen = mqtt_unpack_publish_response(&SMQTTResponse,(const uint8_t*)&pucDst_tem[ucHeadLen]);
	IOT_Printf("\r\n**********************IOT_MQTTPublishReq_uc**************************\r\n");
	IOT_Printf("\r\n**********************ucHeadLen = %d**************************\r\n",ucHeadLen);
	IOT_Printf("\r\n**********************usiDataLen = %d**************************\r\n",usiDataLen);
//	pPublishbuf=pucDst_tem+ucHeadLen;
	if((ucHeadLen+usiDataLen) == suiLen_tem) // �������ݷ���mqtt publish��ʽ
	{
// ͷ + ͷlen��2�� + ����len��2�� +���� + ID��2�� + data
		#define _SQUARE_128  (16384)	//128 ƽ��
		#define _CUBE_128  	(2097152)	//128 ����
		uint32_t uiBUffOffset = 0;
		Publish_Len = 0;
		if(pucDst_tem[1] > 0x7F)	// �ܰ����� (��ȥͷ+len)  =usiDataLen
		{
			if(pucDst_tem[2] > 0x7F)
			{
				if(pucDst_tem[3] > 0x7F)
				{
					Publish_Len = pucDst_tem[4] * _CUBE_128;
					uiBUffOffset++;
				}
				Publish_Len += (pucDst_tem[3] & 0x7F) * _SQUARE_128;
				uiBUffOffset++;
			}
			Publish_Len += (pucDst_tem[2] & 0x7F) * 128;
			uiBUffOffset++;
		}

		Publish_Len += pucDst_tem[1] & 0x7F;


		Publish_TopicLen = pucDst_tem[2 + uiBUffOffset];
		Publish_TopicLen = (Publish_TopicLen << 8) +pucDst_tem[3 + uiBUffOffset];	// �������ⳤ��
		if(uQoc == 0)  						 						// ��Ϣ������ 0  ��׼MQTT ������ϢID
		{
			Publish_dataLen = abs( Publish_Len - Publish_TopicLen -2);  // ȱ��  ID
			Publish_dataaddr =  Publish_TopicLen + 4;  					// ʵ�ʷ��������ݿ�ʼ��ַ
		}
		else
		{
			Publish_dataLen  =  abs(Publish_Len - Publish_TopicLen -4) ;    // ʵ�����ݵĳ��� = ��- �������� - ��������len - ID
			MQTT_Packet_ACKID = pucDst_tem[Publish_TopicLen + ucHeadLen +2 ];  //19 20
			MQTT_Packet_ACKID = (MQTT_Packet_ACKID<< 8 ) + pucDst_tem[Publish_TopicLen + ucHeadLen + 3 ] ;
			Publish_dataaddr =  Publish_TopicLen+6;  	 // ʵ�ʷ��������ݿ�ʼ��ַ
		}

//		IOT_Printf("\r\n******************************************************************\r\n ");
//	   	/* Print response directly to stdout as it is read */
//	   	for(int i = 0; i < suiLen_tem; i++) {
//	   	   IOT_Printf("%02x ",pucDst_tem[i]);
//	   	}

		IOT_Printf("\r\n**********************uiBUffOffset =%d**************************\r\n",uiBUffOffset);
		IOT_Printf("\r\n**********************Publish_Len =%d**************************\r\n",Publish_Len);
		IOT_Printf("\r\n**********************Publish_Topic_Len=%d**************************\r\n",Publish_TopicLen);
		IOT_Printf("\r\n**********************Publish_data_Len=%d**************************\r\n",Publish_dataLen);
		IOT_Printf("\r\n**********************MQTT_Packet_ACKID=%d**************************\r\n",MQTT_Packet_ACKID);
		IOT_Printf("\r\n**********************Publish_dataaddr =%d**************************\r\n",Publish_dataaddr);
		IOT_Printf("\r\n**********************uQoc =%d**************************\r\n",uQoc);
		_g_EMQTTREQType =  IOT_ServerReceiveHandle(&pucDst_tem[Publish_dataaddr + uiBUffOffset],Publish_dataLen );  //important

//		if(uQoc)   // �����������1 ��ֱ��Ӧ��ACK
//		{
//			_g_EMQTTREQType= MQTTREQ_NULL;
//			IOT_MqttSendStep_Set(1,4);     //  ��Ҫ�Ż�����ѵ�������
//		}
//		else
//		{
//			_g_EMQTTREQType= MQTTREQTypeVal;
//		}
//�Ż�Ӧ��
		IOT_Printf("\r\n**********************_g_EMQTTREQType =%d**************************\r\n",_g_EMQTTREQType);
		return ESP_OK1;
	}
	return ESP_FAIL1;
}


/******************************************************************************
 * FunctionName : IOT_MQTTPublishACK_uc
 * Description  : �ɼ���MQTTPublish  ������ACK ����
 * Parameters   : uint8_t *dst        : ����Ҫ����������server��������
                  uint16_t suiLen_tem : ���������������ܳ���
 * Returns      : 0 - ����ʧ�� / 1 - �����ɹ�
 * Notice       : ������//  03 / 04  / 19  / 18  �����ϱ�    �Ƚ������ϱ���ID  ��ACK�� ID �Ƿ�һ��
*******************************************************************************/
uint8_t IOT_MQTTPublishACK_uc(uint8_t *pucDst_tem, uint16_t suiLen_tem)
{
	if((suiLen_tem == 4)&&(pucDst_tem[0] == 0x40)&&(pucDst_tem[1] == 0x02))
	{
		MQTT_Packet_ACKID = pucDst_tem[2];
		MQTT_Packet_ACKID = ((uint16_t )MQTT_Packet_ACKID << 8) + pucDst_tem[3];
		IOT_Printf("\r\n*************MQTT_Packet_ACKID=%d*************MQTT_Packet_ID=%d*********_g_EMSGMQTTSendTCPType%d****\r\n",MQTT_Packet_ACKID,MQTT_Packet_ID,_g_EMSGMQTTSendTCPType);
		if(MQTT_Packet_ACKID == MQTT_Packet_ID  )  //System_state.bCID_0x04
		{
			if(_g_EMSGMQTTSendTCPType == EMSG_INVPARAMS_POST)    // ��0x03 ����Ӧ��
			{
				InvCommCtrl.HoldingSentFlag = 0;    			 // �������
				//_g_EMSGMQTTSendTCPType = EMSG_ONLINE_POST;
				//IOT_MqttSendStep_Set(1,3);                  	 // ��������� -- ��������04����

				_g_EMSGMQTTSendTCPType = EMSG_NULL;
				IOT_TCPDelaytimeSet_uc(ESP_WRITE,Mqtt_DATATime); // ���¸�ֵ 04 �����ϱ�ʱ��
				//IOT_TCPDelaytimeSet_uc(ESP_WRITE,System_state.Up04dateTime+1);

			}
			else if (_g_EMSGMQTTSendTCPType == EMSG_ONLINE_POST )// ��04 ���� Ӧ��
			{
				_g_EMSGMQTTSendTCPType = EMSG_NULL;
				uAmmeter.UpdateMeterFlag = 1 ;  //�����ϱ����
			// 	IOT_TCPDelaytimeSet_uc(ESP_WRITE,Mqtt_DATATime); // ���¸�ֵ 04 �����ϱ�ʱ��
			// 	IOT_TCPDelaytimeSet_uc(ESP_WRITE,System_state.Up04dateTime+1);
			}
			else if(_g_EMSGMQTTSendTCPType == EMSG_SYSTEM_POST ) // �����ϱ� 19 ���ݲɼ�������
			{
				System_state.UploadParam_Flag =0;
				_g_EMSGMQTTSendTCPType = EMSG_NULL;
			}
			else if(  _g_EMSGMQTTSendTCPType == EMSG_GET_INVINFO_RESPONSE) // �����������ȡ 0x05��ѯ����
			{
				System_state.Host_setflag &= ~CMD_0X05;
				_g_EMSGMQTTSendTCPType = EMSG_NULL;
			}
			else  if(_g_EMSGMQTTSendTCPType == EMSG_GET_DLINFO_RESPONSE )  // ��������ѯ�ɼ���������ָ�� 0x19
			{
				System_state.Host_setflag &= ~CMD_0X19;
				_g_EMSGMQTTSendTCPType = EMSG_NULL;
			}
			else if(_g_EMSGMQTTSendTCPType == EMSG_OFFLINE_POST )
			{
				_g_EMSGMQTTSendTCPType = EMSG_NULL;
			}
			else if(_g_EMSGMQTTSendTCPType == EMSG_AMMTER_POST )
			{
				_g_EMSGMQTTSendTCPType = EMSG_NULL;
			}
			else if(_g_EMSGMQTTSendTCPType == EMSG_HISTORY_AMMTER_POST )
			{
				_g_EMSGMQTTSendTCPType = EMSG_NULL;
				IOT_SET_Histotymeter_Flag(0);
			}
			else if(_g_EMSGMQTTSendTCPType == EMSG_INVWAVE_POST)
			{
				System_state.up14dateflag=0;
				IOT_ServerACK_0x14();
				_g_EMSGMQTTSendTCPType = EMSG_NULL;
			}
			else if(_g_EMSGMQTTSendTCPType == EMSG_BDC03_POST)
			{
				_g_EMSGMQTTSendTCPType = EMSG_NULL;
			}
			else if(_g_EMSGMQTTSendTCPType == EMSG_BDC04_POST)
			{	_g_EMSGMQTTSendTCPType = EMSG_NULL;
			}
			else
			{
				_g_EMSGMQTTSendTCPType = EMSG_NULL;
			}
		}
		return ESP_OK1;
	}
	return ESP_FAIL1;
}





