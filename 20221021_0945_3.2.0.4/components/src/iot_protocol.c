/*
 * iot_protocol.c
 *  Created on: 2021��12��9��
 *      Author: Administrator
 */

#include "iot_protocol.h"
#include "iot_universal.h"
#include "iot_net.h"
#include "iot_uart1.h"
#include "iot_rtc.h"
#include "iot_inverter.h"

#include "iot_system.h"
#include "iot_station.h"
#include "iot_gatts_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"

#include "iot_ammeter.h"

#include "iot_fota.h"
#include "iot_local_update.h"
#include "iot_InvWave.h"
#include "iot_record.h"

#include "iot_Local_Protocol.h"

#define ServerProtocolMAXSIZE  1536   //buff ��С��Ϊ1536  20220521 chen
//#define ServerProtocolMAXSIZE  1000
uint8_t ProtocolTXBUF[ServerProtocolMAXSIZE]={0};

/********************************************
 * APP ʹ��ͨѶЭ�������루0x17��0x18��0x19��0x26��
 * 0x17 Ϊ͸������漰modbus 03/04/06/10������
 * Э��汾��5.0 �汾 �� SN����Ϊ10λ
 *
 * */

/********************************************
	���ͷ��������� Э�����
	6.0 �汾Э��
******************************************/
#define GRT_KEY_LEN		7
const char GrowattKey[GRT_KEY_LEN] = {'G', 'r', 'o', 'w', 'a', 't', 't'};
uint8_t encrypt(uint8_t *pdata, uint16_t length)
{
	uint8_t i = 0;
	if(length >= ServerProtocolMAXSIZE)
	{
		return 1;
	}
	while(length--)
	{
		*pdata++ ^= GrowattKey[i++];
		if(i == GRT_KEY_LEN)
			i = 0;
	}
	return 0;
}

/*************************************************************
����Э�� 6.0  Э�����  ��ͷ��β
**************************************************************/
uint16_t IOT_Protocol_ToSever(unsigned char *p,unsigned int uLen,unsigned int Vsnum)
{
	uint16_t len = 0;
	uint16_t crc16 = 0;
	uint16_t _num = uLen; // �õ�ԭʼ���ݳ���

	char SNbuf[30]={0};

	if(_num > ServerProtocolMAXSIZE)
	{
		_num = ServerProtocolMAXSIZE - 2;
	}
	/* (1)����Э��汾��6.0 */
//	if(!strlen((char*)&SysTAB_Parameter[Parameter_len[REG_SN]]))// ����������к��ǿ�
//	{
//		memset((char *)&SysTAB_Parameter[Parameter_len[REG_SN]],0,sizeof(&SysTAB_Parameter[Parameter_len[REG_SN]]));
//		memcpy((char *)&SysTAB_Parameter[Parameter_len[REG_SN]],"inversnerr",10);// �õ���������к�-err SN chg LI 20201112
//	}

	len = MERGE(p[4],p[5]);   	// �õ���10-sn����Ч����

	if(	Vsnum  ==  LOCAL_PROTOCOL)		//�ж��Ƿ�Ϊ����ͨѶ����
	{
		if((Vsnum & 0x00ff) == ProtocolVS05)	//���汾v5 ����ֱ�Ӹ���
		{
			memcpy(ProtocolTXBUF, p, _num);// �õ�����ǰ����ԭʼ����
		}else	//����ͨ�Ű汾Ϊ V6 ���ϵİ汾
		{
			len =  IOT_Repack_ProtocolData_V5ToV6_us(p , _num , ProtocolTXBUF);	//���´������

			memcpy(p, ProtocolTXBUF, len);
			return len;
		}
	}
	else
	{
		if((p[7] == 0x03) || (p[7] == 0x04) || (p[7] == 0x14) || (p[7] == 0x1C) ||\
			(p[7] == 0x23) || (p[7] == 0x24) || (p[7] == 0x25) ||\
			(p[7] == 0x50) || (p[7] == 0x33) || (p[7] == 0x34) || (p[7] == 0x20)  || (p[7] == 0x22)) // �ж����������������к�
		{
				len += 20*2;											 			 // ���Ӻ�30-sn�����ݳ���
				memcpy(ProtocolTXBUF, p, 8);                     		 			 // ͨѶ��� + Э���� + ԭ���ݳ��� + �豸��ַ + ������
				ProtocolTXBUF[4] = HIGH(len);
				ProtocolTXBUF[5] = LOW(len); 										 // ��������ʵ�ʳ���
				if(Collector_Config_LenList[REG_SN] < 30  )			 				 // ���� 30 λ���ȵ����к�
				{
					bzero(SNbuf, sizeof(SNbuf));
					memcpy(SNbuf,&SysTAB_Parameter[Parameter_len[REG_SN]],Collector_Config_LenList[REG_SN] );
					memcpy(&ProtocolTXBUF[8], SNbuf, 30 );  		   				 // �ɼ������к�
				}
				else
				{
					memcpy(&ProtocolTXBUF[8], &SysTAB_Parameter[Parameter_len[REG_SN]], 30);  		// �ɼ������к�
				}
				memcpy(&ProtocolTXBUF[38], InvCommCtrl.SN, 30);		    			// ��������к�
				memcpy(&ProtocolTXBUF[68], &p[28], _num-28);           				 // ����������
				_num += (20*2); // �������ݸ���
		}
		else // ���������루û����������кţ�//18  19
		{
		//	IOT_Printf("\r\n==============================================\r\n");
		//	IOT_Printf("\r\n==p[7]=0x%02x=======_num=%d===============\r\n",p[7],_num);
				len += 20*1;              								  // ���Ӻ�30-sn�����ݳ���
				memcpy(ProtocolTXBUF, p, 8);                     		  // ͨѶ��� + Э���� + ԭ���ݳ��� + �豸��ַ + ������
				ProtocolTXBUF[4] = HIGH(len);
				ProtocolTXBUF[5] = LOW(len); // ��������ʵ�ʳ���
				if(Collector_Config_LenList[REG_SN] < 30  )   			  // ���� 30 λ���ȵ����к�
				{
					bzero(SNbuf, sizeof(SNbuf));
					memcpy(SNbuf,&SysTAB_Parameter[Parameter_len[REG_SN]],Collector_Config_LenList[REG_SN] );
					memcpy(&ProtocolTXBUF[8], SNbuf, 30 );  		// �ɼ������к�
				}
				else
				{
					memcpy(&ProtocolTXBUF[8], &SysTAB_Parameter[Parameter_len[REG_SN]], 30);  		// �ɼ������к�
				}
				 memcpy(&ProtocolTXBUF[38], &p[18], _num-18);     		// ����������
				 _num += (20*1); // �������ݸ���
		}
	}

	if((Vsnum & 0x00ff) == ProtocolVS05)//Э����� 5.0
	{
		ProtocolTXBUF[3] =0x05;
	}
	else if((Vsnum & 0x00ff)== ProtocolVS07)//Э����� 7.0
	{
		ProtocolTXBUF[3]=0x07;
	}
	else//Э����� 6.0
	{
		ProtocolTXBUF[3]=0x06;
	}


	len = 0;
	if(_num < 8)
	{
		return 2;
	}

#if DEBUG_MQTT
	uint16_t i=0;
	IOT_Printf("\r\nIOT_Protocol_ToSever*************len =%d********  \r\n",_num);
	for( i=0; i<_num; i++)
	{
		IOT_Printf("%02x ",ProtocolTXBUF[i]);
	}
#endif

	encrypt(&ProtocolTXBUF[8], (_num - 8));		//������
	IOT_Printf("\r\n***********************encrypt**************************  \r\n",_num);

	crc16 = Modbus_Caluation_CRC16(ProtocolTXBUF, _num);
	ProtocolTXBUF[_num] = HIGH(crc16);
	ProtocolTXBUF[_num+1] = LOW(crc16);
	len = _num + 2;
	IOT_Printf(" \r\n  ********len =%d********  \r\n",len);
#if DEBUG_MQTT
	IOT_Printf("\r\nIOT_Protocol_ToSever*************len =%d********  \r\n",_num);
	for(i=0; i<_num; i++)
	{
		IOT_Printf("%02x ",ProtocolTXBUF[i]);
	}
#endif
	memcpy(p, ProtocolTXBUF, len);

	return len;

}

/*****************************************************************
  *     ������������������ݲɼ�������ز������в�ѯ
  *     ��Ӧ������ָ�� 0x19
  ****************************************************************
  */
void IOT_CommTESTData_0x19(uint8_t* pucBuf_tem, uint16_t suiLen_tem)
{
	unsigned char buf[200] = {0};
	unsigned char i,num;
	char *p ;

	uint16_t crc16 = 0;
	uint8_t *pBuf = pucBuf_tem;
	uint8_t flag_Send_0x0d = 0;
	uint32_t add_s = 0;           //��ʼ�������
	uint32_t add_e = 0;           //��ֹ�������
	uint8_t SNLen = 0;
	char SN_Str[30] = {0};
	uint8_t ParamOffset = 0;
	uint8_t Redate[50] = {0};

	if(( PROTOCOL_VERSION_L == 6) || (PROTOCOL_VERSION_L == 7))// LI 2018.05.11
	{
		ParamOffset = 20; // ��ԭ�������ϼ���20���ɼ������к��ַ�
		SNLen = 10+ParamOffset;
		memcpy(SN_Str,&pBuf[8],SNLen);
		p = SN_Str;
	}
	else
	{
		SNLen = 10;
		memcpy(SN_Str,&pBuf[8],SNLen);
		p = SN_Str;
	}
	add_s = ((unsigned int)pBuf[18+ParamOffset]<<8) + pBuf[19+ParamOffset];           //��ʼ�������
	add_e = ((unsigned int)pBuf[20+ParamOffset]<<8) + pBuf[21+ParamOffset];           //��ֹ�������

			for(i=0; i<suiLen_tem; i++)
			{
				IOT_Printf("%02x " ,pBuf[i]);
			}
			IOT_Printf(" is pBuf 1\r\n");

#if DEBUG_TEST
			IOT_Printf("��ʼ��ַ = %d\r\n" ,add_s);
			IOT_Printf("������ַ = %d\r\n" ,add_e);
#endif
    for(i=0; i<SNLen; i++)                             //����SN�ŵ����ݻ���
    {
        buf[8+i] =p[i];
    }

    if((add_e >= add_s) && (add_e <= 200))            //�ж�Ҫ��ѯ�Ĳ�������Ƿ���ȷ
    {
		for(num=add_s; num <=add_e; num++)          // if(str == set_tab_str[REG_SN] )
		{
		/* ׼������ */
			if(num == 13)			//���⴦����13�������ŵ����
			{
				flag_Send_0x0d = 1;
				IOT_Printf("(num == 13)\r\n");
				continue;
			}
			if(num < 70) // �����б�1
			{
				IOT_Printf("\r\n uart test get 0x19 date = ",num);
				IOT_Printf("num= %d  len=%d \r\n" ,num,Collector_Config_LenList[num]);
				memcpy(&buf[22+ParamOffset], &SysTAB_Parameter[Parameter_len[num]], Collector_Config_LenList[num]);
				memcpy(Redate, &SysTAB_Parameter[Parameter_len[num]], Collector_Config_LenList[num]);
				if(num ==2 )  //Э��汾 �뷢�͵�ͬ�� ���ݲ������������
				{
					if(buf[22+ParamOffset]== 0x37 )
					{
						buf[22+ParamOffset]= pBuf[3]+0x30;
					}
				}
//				else if( num == 18 )   // ���ݲ������������  �˿� 5279
//				{
//					buf[22+ParamOffset]= 0x35;
//					buf[23+ParamOffset]= 0x32;
//					buf[24+ParamOffset]= 0x37;
//					buf[25+ParamOffset]= 0x39;
//				}

#if DEBUG_TEST
			for(i=0; i<Collector_Config_LenList[num]; i++)
			{
				IOT_Printf("%02x " ,Redate[i]);
			}
			IOT_Printf(" is all 1\r\n");

			for(i=0; i<Collector_Config_LenList[num]; i++)
			{
				IOT_Printf("%02x " ,buf[22+i+ParamOffset]);
			}
			IOT_Printf("is all 2\r\n");
#endif
		}
		else // �����б�2
		{
		}
		/* ��ѯʱ�� */
		if(num == 31)
		{
			buf[22+ParamOffset] = '2';								//2
			buf[23+ParamOffset] = '0';								//0
			buf[24+ParamOffset] = (	sTime.data.Year/10) + 0x30;		//��
			buf[25+ParamOffset] = (	sTime.data.Year%10) + 0x30;
			buf[26+ParamOffset] = '-';
			buf[27+ParamOffset] = (sTime.data.Month/10) + 0x30;		//��
			buf[28+ParamOffset] = (sTime.data.Month%10) + 0x30;
			buf[29+ParamOffset] = '-';
			buf[30+ParamOffset] = (sTime.data.Date/10) + 0x30;		//��
			buf[31+ParamOffset] = (sTime.data.Date%10) + 0x30;
			buf[32+ParamOffset] = ' ';
			buf[33+ParamOffset] = (sTime.data.Hours/10) + 0x30;		//ʱ
			buf[34+ParamOffset] = (sTime.data.Hours%10) + 0x30;
			buf[35+ParamOffset] = ':';
			buf[36+ParamOffset] = (sTime.data.Minutes/10) + 0x30;	//��
			buf[37+ParamOffset] = (sTime.data.Minutes%10) + 0x30;
			buf[38+ParamOffset] = ':';
			buf[40+ParamOffset] = (sTime.data.Seconds%10) + 0x30;
		}
		if(num < 70) // �����б�1
		{
			buf[0] =0x00;
			buf[1] =0x01;
			buf[2] =0x00;
			buf[3] =pBuf[3];
			buf[4] =0x00;
			buf[5] =Collector_Config_LenList[num]+16+ParamOffset;
			buf[6] =1;
			buf[7] =0x19;
			buf[18+ParamOffset] =0;
			buf[19+ParamOffset] =num;
			buf[20+ParamOffset] =0;
			buf[21+ParamOffset] =Collector_Config_LenList[num];
		}

		crc16 = Modbus_Caluation_CRC16(buf, buf[5] + 6);
		buf[buf[5] + 6] = HIGH(crc16);
		buf[buf[5] + 7] = LOW(crc16);
		IOT_UART1SendData(buf, buf[5] + 8);
	  }
		if(flag_Send_0x0d == 1)
		{
			/* ׼������ */
//			set_str = System_Config[0x0d];
//			for(i=0; i<Collector_Config_LenList[0x0d]; i++)
//			{
//				buf[22+i+ParamOffset] =set_str[i];
//			}
			memcpy(&buf[22+ParamOffset], &SysTAB_Parameter[Parameter_len[0x0d]], Collector_Config_LenList[0x0d]);

			buf[0] =0x00;
			buf[1] =0x01;
			buf[2] =0x00;
			buf[3] =pBuf[3];
			buf[4] =0x00;
			buf[5] =Collector_Config_LenList[0x0d]+16+ParamOffset;
			buf[6] =0x01;
			buf[7] =0x19;

			buf[18+ParamOffset] =0x00;
			buf[19+ParamOffset] =0x0d;
			buf[20+ParamOffset] =0x00;
			buf[21+ParamOffset] =Collector_Config_LenList[0x0d];

#if DEBUG_TEST
			IOT_Printf("\r\n��Ӧ0x19ָ��,��ѯ%d�Ĵ��� = ",0x0d);
			for(i=0; i<buf[5]+6; i++)
			{

				IOT_Printf("%02x " ,buf[i]);
			}
			IOT_Printf(" is ack \r\n");
#endif
			crc16 = Modbus_Caluation_CRC16(buf, buf[5] + 6);
			buf[buf[5] + 6] = HIGH(crc16);
			buf[buf[5] + 7] = LOW(crc16);
			IOT_UART1SendData(buf,buf[5] + 8);
		}

    }

}


/*****************************************************************
  *     ��Ӧ0x18ָ��
  *		Status:	00 	��Ӧ�ɹ�ָ��
  *				01	����Ӧ
  *				02	��Ӧʧ��ָ��
  ****************************************************************
  */
void IOT_CommTESTData_0x18(uint8_t* pucBuf_tem, uint16_t suiLen_tem)
{
  char *p;
  uint8_t i;
  uint16_t len,regNum;
  uint8_t *pBuf = pucBuf_tem;
  uint8_t CommTxBuf[100] = {0};
  uint16_t crc16 = 0;

  uint8_t SNLen = 0;
//uint8_t SETSNlen=0;
  char SN_Str[30] = {0};
//uint8_t uSetTIMEdate[8];
  uint8_t ParamOffset = 0; // ����ƫ��ֵ LI 2018.05.11

	if((PROTOCOL_VERSION_L == 6) || (PROTOCOL_VERSION_L == 7))// LI 2018.05.11
	{
		ParamOffset = 20; // ��ԭ�������ϼ���20���ɼ������к��ַ�
		SNLen = 10+ParamOffset;
		memcpy(SN_Str,&pBuf[8],SNLen);
		p = SN_Str;
	}
	else
	{
		SNLen = 10;
		memcpy(SN_Str,&pBuf[8],SNLen);
		p = SN_Str;
	}

	regNum = pBuf[18+ParamOffset];			//������ı��
	regNum = (regNum << 8) | pBuf[19+ParamOffset];

	len = pBuf[20+ParamOffset];				//���ó���
	len = (len << 8) | pBuf[21+ParamOffset];


	/* ������ݼ�¼ */
	if((regNum == 33) && (pBuf[22+ParamOffset] == '1'))
	{
		//ClearAllRecord();					//�����¼
	}

	/* �ָ��������� */
	if((regNum == 35) && (pBuf[22+ParamOffset] == '1'))
	{
		CommTxBuf[0] = 0x00;
		CommTxBuf[1] = 0x01;
		CommTxBuf[2] = 0x00;
		CommTxBuf[3] = pBuf[3];
		CommTxBuf[4] = 0x00;
		CommTxBuf[5] = 0x0f+ParamOffset;	 //���ݳ���

		CommTxBuf[6]  = 0x01;				 //�豸��ַ
		CommTxBuf[7]  = 0x18;

		for(i=0; i<SNLen; i++)			    //�ɼ���SN
		{
			CommTxBuf[8+i] =p[i];
		}

		CommTxBuf[18+ParamOffset] = pBuf[18+ParamOffset];			//���õı��
		CommTxBuf[19+ParamOffset] = pBuf[19+ParamOffset];
		CommTxBuf[20+ParamOffset] = 0x00;							//�ɹ�

		crc16 = Modbus_Caluation_CRC16(CommTxBuf, 21+ParamOffset);
		CommTxBuf[21+ParamOffset] = HIGH(crc16);
		CommTxBuf[22+ParamOffset] = LOW(crc16);
		IOT_UART1SendData(CommTxBuf,23+ParamOffset);

		//IOT_FactoryReset();						//�ָ��Ĵ���Ϊ�����趨+ɾ����������
		IOT_KEYlongReset();
 	    //ClearAllRecord();
//		Change_Config_Value("0",35,1);			//������²���������
		IOT_Printf("CommSetDatalog  IOT_FactoryReset  \r\n");
		System_state.SYS_RestartFlag=1;
		IOT_SYSRESTTime_uc(ESP_WRITE ,1);

		return;
	}
	if(regNum == 17 || regNum == 18 || regNum == 19)
	{
		//IOT_ServerParamSet_Check(3);

	}

// ����CRC У�� �����λ����bUG
//	if((regNum == 8)  )
//	{
//		len = IOT_NewInverterSN_Check(&pBuf[22+ParamOffset]);  //���� SN ��Ч����
//	}

	IOT_SystemParameterSet(regNum ,(char *) &pBuf[22+ParamOffset],len );  //��������

	if(regNum == 17)
		IOT_SystemParameterSet(19 ,(char *) &pBuf[22+ParamOffset],len );  //��������
 		//URL��IP����������URL��
	else if(regNum == 19)
		IOT_SystemParameterSet(17 ,(char *) &pBuf[22+ParamOffset],len );  //��������
	else if(regNum == 18)
	{
		IOT_SystemParameterSet(18 ,(char *)"7006",4 );  //��������
	}
	//URL��IP����������URL��
	if(regNum == 17 || regNum == 18 || regNum == 19)
	{
		//IOT_ServerParamSet_Check(1);
	}
	/* ����ʱ�� */
	if(regNum == 31)
	{
		Clock_Time_Init[0] = (pBuf[24+ParamOffset]-0x30)*10+(pBuf[25+ParamOffset]-0x30);	//��
		Clock_Time_Init[1] = (pBuf[27+ParamOffset]-0x30)*10+(pBuf[28+ParamOffset]-0x30);	//��
		Clock_Time_Init[2] = (pBuf[30+ParamOffset]-0x30)*10+(pBuf[31+ParamOffset]-0x30);	//��
		Clock_Time_Init[3] = (pBuf[33+ParamOffset]-0x30)*10+(pBuf[34+ParamOffset]-0x30);	//ʱ
		Clock_Time_Init[4] = (pBuf[36+ParamOffset]-0x30)*10+(pBuf[37+ParamOffset]-0x30);	//��
		Clock_Time_Init[5] = (pBuf[39+ParamOffset]-0x30)*10+(pBuf[40+ParamOffset]-0x30);	//��

		IOT_Printf("\r\n*****************set: %02d:%02d:%02d-%02d:%02d:%02d*****************\r\n" ,Clock_Time_Init[0],Clock_Time_Init[1],Clock_Time_Init[2],Clock_Time_Init[3],Clock_Time_Init[4],Clock_Time_Init[5]);

		IOT_RTCSetTime(Clock_Time_Init);
	}
	CommTxBuf[0] = 0x00;
    CommTxBuf[1] = 0x01;
    CommTxBuf[2] = 0X00;
    CommTxBuf[3] = pBuf[3];
    CommTxBuf[4] = 0x00;
    CommTxBuf[5] = 0x0f+ParamOffset;							//���ݳ���
    CommTxBuf[6]  = 0X01;		//�豸��ַ
    CommTxBuf[7]  = 0x18;

    for(i=0; i<SNLen; i++)					//�ɼ���SN
    {
        CommTxBuf[8+i] =p[i];
    }
	CommTxBuf[18+ParamOffset] = pBuf[18+ParamOffset];		//���õı��
	CommTxBuf[19+ParamOffset] = pBuf[19+ParamOffset];		//
	CommTxBuf[20+ParamOffset] = 0x00;		//�ɹ�


	crc16 = Modbus_Caluation_CRC16(CommTxBuf, 21+ParamOffset);
	CommTxBuf[21+ParamOffset] = HIGH(crc16);
	CommTxBuf[22+ParamOffset] = LOW(crc16);

#if DEBUG_TEST
//			for(i=0; i<23+ParamOffset; i++)
//			{
//
//				IOT_Printf("%02x " ,CommTxBuf[i]);
//			}
//			IOT_Printf("\r\n");
	IOT_Printf("\r\n************IOT_CommTESTData_0x18 ***************\r\n");
#endif
	IOT_UART1SendData(CommTxBuf,23+ParamOffset);

	/* ����ָ�� */
	if((regNum == 32) && (pBuf[22+ParamOffset] == '1'))
	{
		System_state.SYS_RestartFlag=1;
		 IOT_SYSRESTTime_uc(ESP_WRITE ,1);
	}
}

/********************************
 * ����Э�� 0x03  �����ϱ�
 * ������Ӧ��
 *
 * *******************************/
void IOT_ServerFunc_0x03(uint8_t *p)
{
//	CommStat.NoReply = 0;
	if(p[8]  ==0x00)        //���ճɹ�
	{
		//������Ӧ�� OK
		//û���յ�������Ӧ�����
		//0x03������2s����һ��04��
//		System_Config[REG_Server_60][0] = STATE_0X03;			//��¼ͨѶ״̬
// 		System_Config[REG_Server_60][1] = '\0';
//		Collector_Config_LenList[REG_Server_60] = 1;
	}
	else if(p[8]  ==0x01)  	//Server�������ݳ�������ɨ�������
	{
		//������Ӧ�� ERROR
		//�ȴ�10S�������ϴ�,��������£�һ����������������ݻ�ȡ��Ҫ8s
	}
	else if(p[8]  ==0x02)   //�����������ѯ03����ɼ������պ��ϱ���Ӧ��ַ���豸��Ϣ���豸��ַΪ0ʱ�ϱ������Ѽ�����豸��03���
	{
	//	sun_pv.send_set_ok_tcp=0;     //server �����ɼ��������ϴ�03
		InvCommCtrl.HoldingSentFlag = 1;
	}
	IOT_Printf("\r\n************IOT_ServerFunc_0x03***************\r\n");
}
/*********************************
 * ����Э�� 0x04 �����ϱ�
 * ������Ӧ��
 * *******************************/
void IOT_ServerFunc_0x04(uint8_t *p)
{
//	CommStat.NoReply = 0;
	if(p[8] == 0x00)          					 //���ճɹ�
	{
		System_state.ServerACK_flag=1;			 //send state OK
//		System_state.ServerACK_DeathTime = 0;    //û���յ�������Ӧ�������
//		System_Config[REG_Server_60][0] = STATE_0X04;			//��¼ͨѶ״̬
//		System_Config[REG_Server_60][1] = '\0';
//		Collector_Config_LenList[REG_Server_60] = 1;
		//IOT_TCPSendCode_Listen_uc(0,ETCPCODE_G04);                   // growattƽ̨04������Ӧ�������ʱ���
		//IOT_TCPRunLog_Record_uc(1,ETCPRUNLOGP_G04,(char*)p);// ��¼growatt 04����ͳɹ�log ��������Ż�
	}
	else if(p[8]  ==0x01)          //�����쳣
	{
	 	//��������ɨ��PV���ط�
		//�ȴ�12S�������ϴ�04
	}
	else if(p[8]  ==0x02)          //Server������ѯ 0x04
	{
		//sun_pv.send_state_ok_tcp = 0;    		//�����ط�
	}
	IOT_Printf("\r\n************IOT_SeverFunc_0x04***************\r\n");
 }
/********************************
 * ����Э�� 0x05
 * �������·� ��ȡ��������03���������
 * ���� IOT_ServerReceiveHandle ����ֱ��ʹ�� ���ٷ�װ
 * *******************************/
void IOT_ServerFunc_0x05(uint8_t *p)
{
	uint8_t ParamOffset = 0; // ����ƫ����
	ParamOffset = 20;		// ��ԭ�������ϼ���20���ɼ������к��ַ�
	System_state.Host_setflag |= CMD_0X05;
	// InvCommCtrl.RespondFlagGrop |= CMD_0X05;
	InvCommCtrl.SetParamStart = MERGE(p[18 + ParamOffset], p[19 + ParamOffset]);
	InvCommCtrl.SetParamEnd = MERGE(p[20 + ParamOffset], p[21 + ParamOffset]);
	IOT_Printf("\r\n************IOT_sver_code_0x05***************\r\n");

}
/********************************
 * ����Э�� 0x06
 * �������·� ����������е�������������
 * ���� IOT_ServerReceiveHandle ����ֱ��ʹ�� ���ٷ�װ
 * *******************************/
void IOT_ServerFunc_0x06(uint8_t *p)
{
	uint8_t ParamOffset = 0;
	ParamOffset = 20;	// ��ԭ�������ϼ���20���ɼ������к��ַ�
	//�������ݳ���,���������û�����CRCУ���2byte
	System_state.Host_setflag |= CMD_0X06;
	InvCommCtrl.Set_InvParameterNum = MERGE(p[18 + ParamOffset], p[19 + ParamOffset]);
	InvCommCtrl.Set_InvParameterData = MERGE(p[20 + ParamOffset], p[21 + ParamOffset]);
	IOT_Printf("\r\n************IOT_ServerFunc_0x06***************\r\n");

}


/********************************
 * ����Э�� 0x10
 * ������Ӧ�� ����������ж������������
 * ���� IOT_ServerReceiveHandle ����ֱ��ʹ�� ���ٷ�װ
 * *******************************/
void IOT_ServerFunc_0x10(uint8_t *p)
{
	uint8_t ParamOffset = 0; // ����ƫ���� LI 2018.05.11
	ParamOffset = 20;// ��ԭ�������ϼ���20���ɼ������к��ַ�

	System_state.Host_setflag|= CMD_0X10;
 	InvCommCtrl.CMDFlagGrop  |= CMD_0X10;
	InvCommCtrl.SetParamStart = MERGE(p[18 + ParamOffset], p[19 + ParamOffset]);
	InvCommCtrl.SetParamEnd = MERGE(p[20 + ParamOffset], p[21 + ParamOffset]);
	//1500V ���ָ�����
	if(p[22+ParamOffset] == '#')
	{
		InvCommCtrl.SetParamDataLen = (InvCommCtrl.SetParamEnd - InvCommCtrl.SetParamStart + 1) * 2 ;
		p[22+ParamOffset] = 0x00;
		p[23+ParamOffset] = 0x01;
		memcpy(InvCommCtrl.SetParamData, &p[22+ParamOffset], InvCommCtrl.SetParamDataLen);
	}
	else
	{
		InvCommCtrl.SetParamDataLen = (InvCommCtrl.SetParamEnd - InvCommCtrl.SetParamStart + 1) * 2;
		memcpy(InvCommCtrl.SetParamData, &p[22+ParamOffset], InvCommCtrl.SetParamDataLen);
	}
	IOT_Printf("\r\n************IOT_ServerFunc_0x10***************\r\n");
}

/********************************
 * ����Э�� 0x16
 * ������Ӧ�� ����������
 * ����MQTT ����Ping
 * *******************************/
extern uint8_t uSetserverflag;
void IOT_ServerFunc_0x16( void )
{
	if(System_state.Server_Online == 0)  //   �������һ�����������б��ݺͱ��洦��
	{
		IOT_SystemParameterSet(60,"4", strlen("4"));  //��������
		System_state.UploadParam_Flag=1;

		if(0 == System_state.Server_Online)		//�������һ���յ�������,��¼һ�µ�ǰ���ӳɹ��ķ�����ip��ַ�Ͷ˿�
		{
			if(1 == uSetserverflag)		//�����������·������ɹ�,�����·�����
			{
				IOT_Printf("#**  record new Server IP/PORT **#\r\n");
				uSetserverflag = 0;
				IOT_SystemParameterSet(17,_ucServerIPBuf,strlen(_ucServerIPBuf));
				IOT_SystemParameterSet(18,_ucServerPortBuf,strlen(_ucServerPortBuf));
				IOT_SystemParameterSet(19,_ucServerIPBuf,strlen(_ucServerIPBuf));
			}


			bzero(_ucServerIPBuf, sizeof(_ucServerIPBuf));	  // ������IP��ַ������
			memcpy(_ucServerIPBuf , &SysTAB_Parameter[Parameter_len[17]] , Collector_Config_LenList[17] );
			bzero(_ucServerPortBuf, sizeof(_ucServerPortBuf));	// ������IP��ַ������
			memcpy(_ucServerPortBuf , &SysTAB_Parameter[Parameter_len[18]] , Collector_Config_LenList[18]);

		}

		IOT_TCPDelaytimeSet_uc(ESP_WRITE,5);// �뵹��ʱ �����ϱ�03/04
		IOT_Historydate_us(ESP_WRITE,30);   // ��ʷ���� ��ʼ�����ϱ�

	}

	IOT_Heartbeat_Timeout_Check_uc(ESP_WRITE, HEARTBEAT_TIMEOUT);		//������������ʱʱ��
	System_state.Server_Online = 1;
}

/********************************
 * ����Э�� 0x18
 * ������ ���� �ɼ���������ָ�� һ��ֻ�ɶԵ���������������
 *
 * *******************************/
//const char BLEkey[8]={"Growatt0"};  //BLEkeytoken
void IOT_ServerFunc_0x18(uint8_t REGNum ,uint8_t REGLen,  uint8_t *pDATA)
{
	uint16_t len=0,regNum=0;
//	uint8_t ParamOffset = 0; //����ƫ��ֵ
	char TABBLEkeys[40]={0};
	char setKeys[40]={0};
	bzero(TABBLEkeys, sizeof(TABBLEkeys));
	bzero(setKeys, sizeof(setKeys));

//	ParamOffset = 20; 		 //��ԭ�������ϼ���20���ɼ������к��ַ�
//	regNum = p[18 + ParamOffset];
//	regNum = (regNum << 8) | p[19 + ParamOffset];	//������ı��
//	len = p[20 + ParamOffset]-0x30;
//	len = (len << 8) |( p[21 + ParamOffset]-0x30);	//���ó���
	regNum =REGNum;
	len=REGLen;
	IOT_Printf("IOT_ServerFunc_0x18 regNum=  %02d, len= %d, SetData=%s \r\n",regNum,len,pDATA);

//	System_state.Host_setflag |= CMD_0X18;
//	InvCommCtrl.CMDFlagGrop |= CMD_0X18;
//	System_state.ServerSET_REGAddrStart = ((unsigned int)p[18 + ParamOffset]<<8) + p[19 + ParamOffset]; //��ʼ�������
//	System_state.ServerSET_REGAddrLen =  ((unsigned int)p[20 + ParamOffset]<<8) + p[21 + ParamOffset];

 	System_state.ParamSET_ACKCode=0x00;//�ɹ�
	if((regNum == 17)||(regNum == 18)||(regNum == 19)||(regNum == 10)
			||(regNum == 32)||(regNum == 35) || (regNum == 80))  // ����Ҫ��������	//80�żĴ���Ҳ����Ҫ���� 20220524
	{
		// ִ�ж���

	}
	else
	{	// �������
		if((System_state.ble_onoff == 1) &&(System_state.BLEKeysflag==0) && (regNum==54) )  //��һ������������Կ����
		{
			memcpy(setKeys,pDATA,len);
			memcpy(TABBLEkeys,&SysTAB_Parameter[Parameter_len[REG_BLETOKEN]],Collector_Config_LenList[REG_BLETOKEN]);

			if((strncmp(BLEkeytoken,setKeys,32)==0 ) || (strncmp(TABBLEkeys,setKeys,32)==0)) // ��һ������wifi ������Կ�Ƚ�
			{
				System_state.BLEKeysflag=1;  		//��Կ��λ
			}
			else
			{
				System_state.ParamSET_ACKCode = 0x01; //ʧ��
			}
			IOT_Printf("IOT_ServerFunc_0x18 BLEKeysflag= %d  ,len=%d\r\n",System_state.BLEKeysflag,len);
			IOT_Printf("BLEkeytoken=%s\r\n",BLEkeytoken);
			IOT_Printf("TABBLEkeys=%s\r\n",TABBLEkeys);
			IOT_Printf("SetData=%s\r\n",setKeys);

		}
		else
		{
			if((System_state.ble_onoff == 1)&&(regNum==54))
			{
				System_state.ble_onoff = 2;  //����
			}
			IOT_SystemParameterSet(regNum ,(char *) &pDATA[0],len );  //��������
		}

//		{
//			InvCommCtrl.CMDFlagGrop &= ~CMD_0X18;
//		 	IOT_GattsEvenSetR();    				// ����Ӧ��APP 18����
//		}

	}
	switch(regNum)
	{
		case 4:  // �ϱ�ʱ��
			IOT_UPDATETime();
			break;
		case 10: // ���¹̼�
			break;
		case 17: // ������ ip
 	    	bzero(_ucServerIPBuf, sizeof(_ucServerIPBuf));	  // ������IP��ַ������
			memcpy(_ucServerIPBuf,(char *)pDATA,(uint8_t)len );
			IOT_SETConnect_uc(0);
			break;
		case 18: //������ �˿�
 	    	bzero(_ucServerPortBuf, sizeof(_ucServerPortBuf));	// ������IP��ַ������
			memcpy(_ucServerPortBuf,(char *)pDATA,(uint8_t)len );
			IOT_SETConnect_uc(0);
			break;
		case 19: // ����������
 	    	bzero(_ucServerIPBuf, sizeof(_ucServerIPBuf));	// ������IP��ַ������
			memcpy(_ucServerIPBuf,(char *)pDATA,(uint8_t)len );
			IOT_SETConnect_uc(0);
			break;
		case 30: // ʱ��
			break;
		case 31: // ʱ��
			// 2022-06-02 17:14:38
			Clock_Time_Init[0] = (pDATA[2]-0x30)*10+(pDATA[3]-0x30);	//15��
			Clock_Time_Init[1] = (pDATA[5]-0x30)*10+(pDATA[6]-0x30);	//��
			Clock_Time_Init[2] = (pDATA[8]-0x30)*10+(pDATA[9]-0x30);	//��
			Clock_Time_Init[3] = (pDATA[11]-0x30)*10+(pDATA[12]-0x30);	//ʱ
			Clock_Time_Init[4] = (pDATA[14]-0x30)*10+(pDATA[15]-0x30);	//��
			Clock_Time_Init[5] = (pDATA[17]-0x30)*10+(pDATA[18]-0x30);	//��

			System_state.ParamSET_ACKCode= IOT_RTCSetTime(Clock_Time_Init);

			if(0 == System_state.ParamSET_ACKCode)		//20220720 chen  ����ͬ�������ʱ��
			{
				InvCommCtrl.ContrastInvTimeFlag = 1;
			}

		//	if(System_state.ParamSET_ACKCode==0 )
		//	  InvCommCtrl.UpdateInvTimeFlag =1;
		// 	IOT_SetInvTime0x10(45,50,Clock_Time_Init);   				//ͬ�� �����ʱ�� 0x10 ������

			break;
		case 32: // �����ɼ���
			if(pDATA[0] == '1')
			{
				if( uSetserverflag != 0)  // ���÷����� �� ����������
				{
					break;
				}
				System_state.SYS_RestartFlag=1;
				IOT_SYSRESTTime_uc(ESP_WRITE ,5);
			}
			break;
		case 33: // �����¼
			if(pDATA[0]== '1')
			{
			    //ClearAllRecord();							//�����¼
				iot_Reset_Record_Task();
			}
			break;
		case 35: // �ָ���������
			/* �ָ��������� */
			if(pDATA[0] == '1')
			{
				IOT_FactoryReset();						//�ָ��Ĵ���Ϊ�����趨+ɾ����������
#if	test_wifi
				IOT_ESP_Record_ResetSign_Write();							//д����� ��¼����־
#endif
				System_state.SYS_RestartFlag=1;
				IOT_SYSRESTTime_uc(ESP_WRITE ,5);
			}
			break;
		case 39: // ���������ĵ������ش�С

			break;
		case 44: // ����豸�̼�����ģʽ

			break;
		case 56: // WIFI ģ��Ҫ�����ȵ�����
			break;
		case 57: // WIFI ģ��Ҫ���ӵ��ȵ���Կ

			IOT_WIFIOnlineFlag(255);
			IOT_WIFIEvenRECONNECTED();

			break;
		case 58: // WIFIģ�����������Ϣ

			break;
		case 70: // WIFIģʽ
			break;
		case 71: // DHCPʹ��
			break;
		case 75: // ��ȡ��ΧWiFi������
				IOT_WIFIEvenSetSCAN();
			break;
		case 77: // �Ƿ�����Server-cn������
			break;
		case 78: // ��������ַ
			break;
		case 79: // �������˿ں�
			break;
		case 80: // HTTP �����ļ���URL
		  //�����ʽ��1#type01#http://cdn.growatt.com/update/device/datalog/ShineGPRS-X_35/1.0.7.4/ShineGPRS_APP_1_3_0_8_G.bin
			if(strstr((char *)&pDATA[0],"#type"))
			{
				IOT_SystemParameterSet(regNum, (char *)&pDATA[9] ,len - 9);		//����URL
				IOT_UpdateProgress_record(HTTP_PROGRESS , 0);		//����������0

				//ȷ����������
				if((pDATA[0] == '0') || (pDATA[0] == '1'))
				{
					g_FotaParams.ucIsbsdiffFlag = false;	//ȫ������
				}
				else if((pDATA[0] == '2') || (pDATA[0] == '3'))
				{
					g_FotaParams.ucIsbsdiffFlag = true;		//�������
				}

				if(pDATA[6] == '0' && pDATA[7] == '1' ) //�������ͣ� 01  �ɼ���
				{

					IOT_SystemParameterSet(10, "1" , 1);
					IOT_SystemParameterSet(36, "1" , 1);	//д��ֹͣ��ѯ��־

					IOT_FotaType_Manage_c(1, EFOTA_TYPR_DATALOGER);
					IOT_Fota_Init();

					IOT_Printf("\r\n ִ�������ɼ��� \r\n");
					//����10�Ĵ���������Ӧ�ĳ���
					//����10�Ĵ�����ֵ,���������ģʽ
					//����36�Ĵ�����ֹͣ��ѵ���������
				//	Collector_Fota();//�ɼ�����������
				}
				else if(pDATA[6] == '0' && (pDATA[7] == '2' ||
						pDATA[7]== '3' ||pDATA[7]== '4'))  //�������ͣ�02�����bin /03�����hex /04�����mot /05�����out
				{
					IOT_Printf("\r\n ִ����������� \r\n");
					IOT_SystemParameterSet(44, (char *)&pDATA[7] , 1);
					IOT_SystemParameterSet(36, "1" , 1);	//д��ֹͣ��ѯ��־
					IOT_FotaType_Manage_c(1, EFOTA_TYPR_DEVICE );
					IOT_Fota_Init();
					//����44�Ĵ���������Ӧ�ĳ���
					//����44�Ĵ�����ֵ,���������ģʽ
					//����36�Ĵ�����ֹͣ��ѵ���������
				//	Inverter_Fota();//�������������
				}

				//��������·��
			 //if(WIFI_RxBuf[30] == '#' && WIFI_RxBuf[31] == 'h' && WIFI_RxBuf[32] == 't'&& WIFI_RxBuf[33] == 't'&& WIFI_RxBuf[34] == 'p')
//				{
//					if(Change_Config_Value((char *)&WIFI_RxBuf[31+ParamOffset],(u8)regNum,(u8)(len-9)) == 0)		//���²���������
//					{
//						//CommStat.RespondCode = 0x00;									//�ɹ�
//					}
//					else
//					{
//						//CommStat.RespondCode = 0x01;									//ʧ��
//					}
//
//					ESP_printf("\r\n ��������·�� \r\n");
//				}
			}
			break;
	    default:
	    	break;
	}

}

/********************************
 * ����Э�� 0x19
 * ������ ��ѯ �ɼ���������ָ��
 * *******************************/
void IOT_ServerFunc0x19(uint8_t *p)
{
//	uint8_t ParamOffset = 0; // ����ƫ���� LI 2018.05.11

//	ParamOffset = 20;// ��ԭ�������ϼ���20���ɼ������к��ַ�
//	System_state.ServerGET_REGAddrStart = ((unsigned int)p[18 + ParamOffset]<<8) + p[19 + ParamOffset];         //��ʼ�������
//	System_state.ServerGET_REGAddrEnd = ((unsigned int)p[20 + ParamOffset]<<8) + p[21 + ParamOffset];           //��ֹ�������

	if((System_state.ServerGET_REGAddrStart == 31) && (System_state.ServerGET_REGAddrEnd == 31))
	{
//  	��ѯʱ��
//		UpLoadTime = 1;
//		CommStat.ServerGetParamFlag = TRUE;
	}
	else
	{
		if((System_state.ServerGET_REGAddrStart == 0x04) && (System_state.ServerGET_REGAddrEnd == 0x15))
		{
//			CommStat.UploadParamSelfAble = DISABLE;
//			CommStat.UploadPresetParamAble = ENABLE;		//ʹ��Ԥ�õĲ���������
		}
		else
		{
//			CommStat.ServerGetParamFlag = TRUE;
		}
	}
//	TimeSendInfoToServer = 0;			//�����ϴ�
}
/**********************************
 * IOT_ServerReceiveHandle
 * ������publish ���մ���
 * return  _g_EMQTTREQType
 **********************************/
EMQTTREQ IOT_ServerReceiveHandle(uint8_t* pucBuf_tem, uint16_t suiLen_tem)
{
	uint8_t ParamOffset = 0; // SN����ƫ����
	uint8_t DataOffset =0;
	uint8_t *p=NULL;
	uint16_t tempCID = 0;
	uint16_t tempLen = 0;
	EMQTTREQ EMQTTREQTypeVal = MQTTREQ_NULL;

	int i = 0 ,j = 0;
	ParamOffset = 20;// ��ԭ�������ϼ���20���ɼ������к��ַ�

	p =pucBuf_tem ;
	/* ͨѶ���У�� */
	tempCID = p[0];
	tempCID = (tempCID<<8) | p[1];
	tempLen = p[4];
	tempLen = (tempLen << 8) | p[5];
	tempLen += 6;		//�������ݳ���,���������û�����CRCУ���2byte

	IOT_Printf("\r\n*******IOT_ServerReceiveHandle --a********ReceiveHandle***suiLen_tem=%d****************\r\n ",suiLen_tem);
   	/* Print response directly to stdout as it is read */
   	for(int i = 0; i < suiLen_tem; i++) {
   		IOT_Printf("%02x ",pucBuf_tem[i]);
   	}

	if(tempLen < ServerProtocolMAXSIZE)
	{
		if(Modbus_Caluation_CRC16(p, tempLen) != MERGE(p[tempLen], p[tempLen+1]))
		{
			IOT_Printf("\r\n************server publish CRC error*************\r\n");
		 	 return  EMQTTREQTypeVal;
		}
	}
	else
	{
		return EMQTTREQTypeVal;
	}


	if( (p[2]==0) && ( (p[3]==6) ||  (p[3]==7)))
	{
		ParamOffset = 20;
	}
	else
	{
		ParamOffset = 0;
	}
	encrypt(&p[8], (tempLen - 8));			//����,-��ͷ8byte

#if DEBUG_MQTT
	IOT_Printf("\r\n************IOT_ServerReceiveHandle  encrypt CMD_ = 0x%02x, tempLen= %d************\r\n",p[7], tempLen);
	for(i=0; i<tempLen; i++)
	{
		IOT_Printf("%02x ",p[i]);
	}
#endif



	if( (p[2]==0) && ( (p[3]==5) || (p[3]==6) ||  (p[3]==7)))
	{
		if( (p[2]==0) &&  (p[3]==5) )
		{
			if((System_state.BLEKeysflag==0 ) &&(p[7] != 0x18) )  //û�з�����Կ
			{
				IOT_Printf("\r\n**********System_state.BLEKeysflag==0****p[7] != 0x18***********\r\n ");
				return EMQTTREQTypeVal;
			}
		}
#if DEBUG_BLE
		IOT_Printf("\r\n**********IOT_ServerReceiveHandle********suiLen_tem=%d***************\r\n ",suiLen_tem);
   	/* Print response directly to stdout as it is read */
   	for(int i = 0; i < suiLen_tem; i++) {
   		IOT_Printf("%02x ",pucBuf_tem[i]);
   	}
#endif
	if(p[7] == 0x19) 	  					// ������ ��ѯ �ɼ���������ָ��
	{
		EMQTTREQTypeVal=MQTTREQ_CMD_0x19;
		System_state.Host_setflag |= CMD_0X19;
		InvCommCtrl.CMDFlagGrop |= CMD_0X19;
		System_state.ServerGET_REGNUM = ((unsigned int)p[18 + ParamOffset]<<8) + p[19 + ParamOffset];    		  // �Ĵ�����Ÿ���
		IOT_Printf("\r\n******************System_state.ServerGET_REGNUM =%d***************\r\n ",System_state.ServerGET_REGNUM );

		if(	System_state.ServerGET_REGNUM ==1)
		{
			System_state.ServerGET_REGAddrStart = ((unsigned int)p[20 + ParamOffset]<<8) + p[21 + ParamOffset];
			System_state.ServerGET_REGAddrEnd = ((unsigned int)p[20 + ParamOffset]<<8) + p[21+ ParamOffset];
		}
		else
		{
			 for(i=0 , j=0 ;i < System_state.ServerGET_REGNUM  ;i++,j++)
			//System_state.ServerGET_REGAddrStart = ((unsigned int)p[18 + ParamOffset]<<8) + p[19 + ParamOffset];     // ��ʼ�������
			 {
				System_state.ServerGET_REGAddrStart = ((unsigned int)p[20+i+j + ParamOffset]<<8) + p[21+i+j + ParamOffset];       //��ֹ�������
				System_state.ServerGET_REGAddrBUFF[i]=System_state.ServerGET_REGAddrStart ;
//				if(	System_state.ServerGET_REGNUM ==1)
//					System_state.ServerGET_REGAddrEnd = ((unsigned int)p[20+i + ParamOffset]<<8) + p[21+i + ParamOffset];         //��ֹ�������
//				else
//					System_state.ServerGET_REGAddrEnd = ((unsigned int)p[22+i + ParamOffset]<<8) + p[23+i + ParamOffset];         //��ֹ�������
			 }
		}
		IOT_Printf("\r\n******************System_state.ServerGET_REGAddrStart=%d***************\r\n ",System_state.ServerGET_REGAddrStart);
		IOT_Printf("\r\n******************System_state.ServerGET_REGAddrEnd =%d***************\r\n ",System_state.ServerGET_REGAddrEnd );

 		if(System_state.ServerGET_REGAddrStart == 75)
		{
		// if()//��ǰɨ��δ��ɻ��������У�������ɨ��
 		   if( (p[2]==0) && (p[3]==5))//У��Э��汾��  APP�汾Э�� ��10λ���к�
		   IOT_WIFIEvenSetSCAN();
		}
//		else
//		{
//			IOT_GattsEvenSetR();  //Ҫ�Ż���sys
//		}
	}
	else if(p[7] == 0x18) // ������ ���� �ɼ���������ָ��,important
	{
		EMQTTREQTypeVal = MQTTREQ_CMD_0x18;
		System_state.Host_setflag |= CMD_0X18;
	 	InvCommCtrl.CMDFlagGrop |= CMD_0X18;
		System_state.ServerSET_REGNUM = ((p[18 + ParamOffset]<<8) + p[19 + ParamOffset] );
		System_state.ServerSET_REGAddrLen = 0;
		for(i=0; i<System_state.ServerSET_REGNUM ;i++)
		{
			System_state.ServerSET_REGAddrStart = ((unsigned int)p[22  + ParamOffset +DataOffset]<<8) + p[23+ ParamOffset+DataOffset]; //��ʼ�������
			System_state.ServerSET_REGAddrLen =  ((unsigned int)p[24 + ParamOffset+DataOffset]<<8) + p[25 + ParamOffset+DataOffset];
			IOT_Printf("\r\n******************ServerSET_REGAddrStart=%d******ServerSET_REGAddrLen=%d*********\r\n ",System_state.ServerSET_REGAddrStart,System_state.ServerSET_REGAddrLen);
			if( (p[2]==0) && (p[3]==5))//У��Э��汾��  APP�汾Э�� ��10λ���к�
			{
				if((System_state.ble_onoff==1 )&&(System_state.BLEKeysflag==0 ) &&(System_state.ServerSET_REGAddrStart != 54 ) )  //û�з�����Կ
				{
					IOT_Printf("\r\n**********System_state.BLEKeysflag==0****System_state.ServerSET_REGAddrStart!= 54***********\r\n ");
				System_state.Host_setflag  &= ~CMD_0X18;
				InvCommCtrl.CMDFlagGrop &= ~CMD_0X18;
				return EMQTTREQTypeVal;
				}
			}
			 if((p[2]==0) && (p[3]==5)) // ���� ����ֱ�ӱ���
			 {
				  if(System_state.ServerSET_REGAddrLen>0)//APP�߼�����     17 ����� 19    // ������ 0//APP�߼�����     19 ����� 17    // ������ 0
				  {
					 if((System_state.ServerSET_REGAddrStart == 17 )||(System_state.ServerSET_REGAddrStart==19))
					 {
						 IOT_SystemParameterSet(17 ,(char *) &p[26 + ParamOffset+DataOffset],System_state.ServerSET_REGAddrLen );  //��������  ͬʱ����
						 IOT_SystemParameterSet(19 ,(char *) &p[26 + ParamOffset+DataOffset],System_state.ServerSET_REGAddrLen );  //��������
					 }
					 else if(System_state.ServerSET_REGAddrStart==18)
					 {
						 IOT_SystemParameterSet(18 ,(char *) &p[26 + ParamOffset+DataOffset],System_state.ServerSET_REGAddrLen );  //��������
					 }
					 else
					 {
						 IOT_ServerFunc_0x18(System_state.ServerSET_REGAddrStart,System_state.ServerSET_REGAddrLen,&p[26 + ParamOffset+DataOffset]);
					 }
				  }
			 }
			 else   // Զ�̲�������
			 {
					IOT_ServerFunc_0x18(System_state.ServerSET_REGAddrStart,System_state.ServerSET_REGAddrLen,&p[26 + ParamOffset+DataOffset]);
			 }
			DataOffset=System_state.ServerSET_REGAddrLen+4;

			if(80 == System_state.ServerSET_REGAddrStart)	//���Ϊ80�żĴ���,��Ҫȷ�������ķ�ʽ
			{
				if( (p[2]==0) && (p[3]==5))
				{
					g_FotaParams.uiWayOfUpdate = ELOCAL_UPDATE;  //��������
				}
				else
				{
					g_FotaParams.uiWayOfUpdate = EHTTP_UPDATE;	//Զ������
				}
			}

		}//ACK?
	}
	else if(p[7] == 0x17) // ͸������
	{
		EMQTTREQTypeVal=MQTTREQ_CMD_0x17;
		System_state.Host_setflag |= CMD_0X17;
		InvCommCtrl.CMDFlagGrop |= CMD_0X17;
		InvCommCtrl.SetParamDataLen=  ((unsigned int)p[18 + ParamOffset]<<8) + p[19 + ParamOffset]; // ͸�������ݳ���
	    memcpy( InvCommCtrl.SetParamData, &p[20 + ParamOffset], InvCommCtrl.SetParamDataLen );
	}
	else if(p[7] == 0x26) // ����
	{
		IOT_stoppolling();
		IOT_LocalTCPCode0x26_Handle(&p[18 + ParamOffset]);	//�������� ���ݴ���
		EMQTTREQTypeVal=MQTTREQ_CMD_0x26;
		System_state.Host_setflag |= CMD_0X26;
		InvCommCtrl.CMDFlagGrop |= CMD_0X26;
		//IOT_GattsEvenSetR();    				// ����Ӧ��APP 18����
	}
	else if(p[7] == 0x05)
	{
		IOT_stoppolling();
		EMQTTREQTypeVal=MQTTREQ_CMD_0x05;
		System_state.Host_setflag |= CMD_0X05;
		InvCommCtrl.CMDFlagGrop |= CMD_0X05;
		InvCommCtrl.SetParamStart = MERGE(p[18 + ParamOffset], p[19 + ParamOffset]);
		InvCommCtrl.SetParamEnd = MERGE(p[20 + ParamOffset], p[21 + ParamOffset]);
		IOT_Printf("\r\n*******CMD_0x05***********InvCommCtrl.SetParamStart=%d***************\r\n ",InvCommCtrl.SetParamStart);
		IOT_Printf("\r\n******************InvCommCtrl.SetParamEnd =%d***************\r\n ",InvCommCtrl.SetParamEnd  );
	}
	else if(p[7] == 0x06)
	{
		IOT_stoppolling();
		EMQTTREQTypeVal=MQTTREQ_CMD_0x06;
		System_state.Host_setflag |= CMD_0X06;
		InvCommCtrl.CMDFlagGrop |= CMD_0X06;

		InvCommCtrl.Set_InvParameterNum = MERGE(p[18 + ParamOffset], p[19 + ParamOffset]);
		InvCommCtrl.Set_InvParameterData = MERGE(p[20 + ParamOffset], p[21 + ParamOffset]);
		IOT_Printf("\r\n******CMD_0x06************InvCommCtrl.Set_InvParameterNum=%d***************\r\n ",InvCommCtrl.Set_InvParameterNum);
		IOT_Printf("\r\n******************InvCommCtrl.Set_InvParameterData =%d***************\r\n ",InvCommCtrl.Set_InvParameterData  );

	}
	else if(p[7] == 0x10)
	{
		IOT_stoppolling();
		EMQTTREQTypeVal=MQTTREQ_CMD_0x10;

		InvCommCtrl.SetParamStart = MERGE(p[18 + ParamOffset], p[19 + ParamOffset]);
		InvCommCtrl.SetParamEnd = MERGE(p[20 + ParamOffset], p[21 + ParamOffset]);
		//1500V ���ָ�����
		if(p[22+ParamOffset] == '#')
		{
			InvCommCtrl.SetParamDataLen = (InvCommCtrl.SetParamEnd - InvCommCtrl.SetParamStart + 1) * 2 ;
			p[22+ParamOffset] = 0x00;
			p[23+ParamOffset] = 0x01;
			memcpy(InvCommCtrl.SetParamData, &p[22+ParamOffset], InvCommCtrl.SetParamDataLen);
		}
		else
		{
			InvCommCtrl.SetParamDataLen = (InvCommCtrl.SetParamEnd - InvCommCtrl.SetParamStart + 1) * 2;
			memcpy(InvCommCtrl.SetParamData, &p[22+ParamOffset], InvCommCtrl.SetParamDataLen);
		}
		/* �жϻ�ȡ����  */
		if(InverterWave_Judge(InvCommCtrl.SetParamStart,InvCommCtrl.SetParamEnd))
		{
			IOT_Printf("\r\n\r\n\r\n\r\n*******InverterWave_Judge***************\r\n\r\n\r\n\r\n\r\n");
			IOT_PINGRESPSet_uc(ESP_WRITE,Mqtt_PINGTime);   // ����װ������ʱ��// �����ȡ��������ʱ����ʱ���������ͣ�ֱ�������������������
			IOT_TCPDelaytimeSet_uc(ESP_WRITE,System_state.Up04dateTime+1);// ��ʱ04�����ϴ�
		}

		System_state.Host_setflag|= CMD_0X10;
		InvCommCtrl.CMDFlagGrop |= CMD_0X10;

		IOT_Printf("\r\n*******CMD_0x10***********InvCommCtrl.SetParamStart=%d***************\r\n ",InvCommCtrl.SetParamStart);
		IOT_Printf("\r\n******************InvCommCtrl.SetParamEnd =%d***************\r\n ",InvCommCtrl.SetParamEnd  );
		IOT_Printf("\r\n******************InvCommCtrl.SetParamDataLen =%d***************\r\n ",InvCommCtrl.SetParamDataLen  );
	}
	else if(p[7] == 0x14)
	{
		IOT_ServerACK_0x14();
	}
   }

//	if( (p[2]==0) && ( (p[3]==6) ||  (p[3]==7)))//У��Э��汾��     �������Ĺ��ܰ汾 ��30λ�������к�
//	{
//		encrypt(&p[8], (tempLen - 8));			//����,-��ͷ8byte
//		IOT_Printf("\r\n************IOT_ServerReceiveHandle  p[7]= 0x%02x, tempLen= %d************\r\n",p[7], tempLen);
//		for(i=0; i<tempLen; i++)
//		{
//			IOT_Printf("%02x ",p[i]);
//		}
//		if(p[7] == 0x19) 	 					//������ ��ѯ �ɼ���������ָ��
//		{
//				EMQTTREQTypeVal=MQTTREQ_CMD_0x19;
//				System_state.Host_setflag |= CMD_0X19;
//				 InvCommCtrl.CMDFlagGrop |= CMD_0X19;
//				System_state.ServerGET_REGNUM = ((unsigned int)p[18 + ParamOffset]<<8) + p[19 + ParamOffset];    		  // �Ĵ�����Ÿ���
//				IOT_Printf("\r\n******************System_state.ServerGET_REGNUM =%d***************\r\n ",System_state.ServerGET_REGNUM );
//
//				if(	System_state.ServerGET_REGNUM ==1)
//				{
//					System_state.ServerGET_REGAddrStart = ((unsigned int)p[20+i + ParamOffset]<<8) + p[21+i + ParamOffset];
//					System_state.ServerGET_REGAddrEnd = ((unsigned int)p[20+i + ParamOffset]<<8) + p[21+i + ParamOffset];
//				}
//				else
//				{
//					 for(i=0,j=0;i<System_state.ServerGET_REGNUM  ;i++,j++)
//					//System_state.ServerGET_REGAddrStart = ((unsigned int)p[18 + ParamOffset]<<8) + p[19 + ParamOffset];     // ��ʼ�������
//					 {
//						System_state.ServerGET_REGAddrStart = ((unsigned int)p[20+i+j + ParamOffset]<<8) + p[21+i+j + ParamOffset];       //��ֹ�������
//						System_state.ServerGET_REGAddrBUFF[i]=System_state.ServerGET_REGAddrStart ;
//		//				if(	System_state.ServerGET_REGNUM ==1)
//		//					System_state.ServerGET_REGAddrEnd = ((unsigned int)p[20+i + ParamOffset]<<8) + p[21+i + ParamOffset];         //��ֹ�������
//		//				else
//		//					System_state.ServerGET_REGAddrEnd = ((unsigned int)p[22+i + ParamOffset]<<8) + p[23+i + ParamOffset];         //��ֹ�������
//					 }
//				}
//			 	IOT_Printf("\r\n******************System_state.ServerGET_REGAddrStart=%d***************\r\n ",System_state.ServerGET_REGAddrStart);
//		 		IOT_Printf("\r\n******************System_state.ServerGET_REGAddrEnd =%d***************\r\n ",System_state.ServerGET_REGAddrEnd );
//		}
//		else if(p[7] == 0x18) 					//������ ���� �ɼ���������ָ��
//		{
//			EMQTTREQTypeVal=MQTTREQ_CMD_0x18;
//			System_state.Host_setflag |= CMD_0X18;
//		 	InvCommCtrl.CMDFlagGrop |= CMD_0X18;
//			System_state.ServerSET_REGAddrStart = ((unsigned int)p[18 + ParamOffset]<<8) + p[19 + ParamOffset]; //��ʼ�������
//			System_state.ServerSET_REGAddrLen =  ((unsigned int)p[20 + ParamOffset]<<8) + p[21 + ParamOffset];
//			//IOT_ServerFunc_0x18(p);
//			//ACK?
//		}
//		else if(p[7] == 0x05) // ������ ��ѯ �����03�Ĵ�������
//		{
//			EMQTTREQTypeVal=MQTTREQ_CMD_0x05;
//			System_state.Host_setflag |= CMD_0X05;
//			InvCommCtrl.CMDFlagGrop |= CMD_0X05;
//			InvCommCtrl.SetParamStart = MERGE(p[18 + ParamOffset], p[19 + ParamOffset]);
//			InvCommCtrl.SetParamEnd = MERGE(p[20 + ParamOffset], p[21 + ParamOffset]);
//		 	//IOT_ServerFunc_0x05(p);
//		}
//		else if(p[7] == 0x06) // Server�趨PV�����Ĵ���
//		{
//			EMQTTREQTypeVal=MQTTREQ_CMD_0x06;
//			System_state.Host_setflag |= CMD_0X06;
//			InvCommCtrl.CMDFlagGrop |= CMD_0X06;
//			InvCommCtrl.Set_InvParameterNum = MERGE(p[18 + ParamOffset], p[19 + ParamOffset]);
//			InvCommCtrl.Set_InvParameterData = MERGE(p[20 + ParamOffset], p[21 + ParamOffset]);
//			//IOT_ServerFunc_0x06(p);
//		}
//		else if(p[7] == 0x10) // Server�趨PV����Ĵ���
//		{
//			EMQTTREQTypeVal=MQTTREQ_CMD_0x10;
//			System_state.Host_setflag|= CMD_0X10;
//			InvCommCtrl.CMDFlagGrop |= CMD_0X10;
//		  //InvCommCtrl.RespondFlagGrop |= CMD_0X10;
//			InvCommCtrl.SetParamStart = MERGE(p[18 + ParamOffset], p[19 + ParamOffset]);
//			InvCommCtrl.SetParamEnd = MERGE(p[20 + ParamOffset], p[21 + ParamOffset]);
//			InvCommCtrl.SetParamDataLen = (InvCommCtrl.SetParamEnd - InvCommCtrl.SetParamStart + 1) * 2;
//			memcpy(InvCommCtrl.SetParamData, &p[22+ParamOffset], InvCommCtrl.SetParamDataLen);
//			//	IOT_ServerFunc_0x10(p);
//		}
////����ָ����ʱ���ϡ�  �������޸�Ӧ��ʽ   ת�Ƶ�   MQTT  ACK Ӧ��ʽ
//		else if(p[7] == 0x16) //������Ӧ��������
//		{
////			Heart_Cnt++;
////			IOT_Printf("���յ���%07ld��������\r\n",Heart_Cnt);
////			IOT_Printf("Server_OnLine = 1 \r\n");
////			IOT_ServerFunc_0x16(p);
//		}
//		else if(p[7] == 0x03) //�յ�������0x03ָ��
//		{
//			IOT_ServerFunc_0x03(p);
//		}
//		else if((p[7] == 0x04) && (tempCID == System_state.bCID_0x04)) //�յ�������0x04ָ����һ��ʱ�䣨5�룩��û���յ��������������Ӧ���ݣ�Ӧ�ط�
//		{
//			IOT_ServerFunc_0x04(p);
//		}
//		else if(p[7] == 0x62) // ����Server-cn������
//		{
//
//		}
//		else if(p[7] == 0x14) // �������
//		{
//
//		}
//		else if(p[7] == 0x20) // �������
//		{
//
//		}
//	}
//	else if( (p[2]==0) && (p[3]==5))//У��Э��汾��  APP�汾Э�� ��10λ���к�
//	{
//		ParamOffset=0;
//		encrypt(&p[8], (tempLen - 8));			// ����,-��ͷ8byte
//
//		if((System_state.BLEKeysflag==0 ) &&(p[7] != 0x18) )  //û�з�����Կ
//		{	IOT_Printf("\r\n**********System_state.BLEKeysflag==0****p[7] != 0x18***********\r\n ");
//			return EMQTTREQTypeVal;
//		}
//#if DEBUG_BLE
//		IOT_Printf("\r\n******************suiLen_tem=%d***************\r\n ",suiLen_tem);
//	   	/* Print response directly to stdout as it is read */
//	   	for(int i = 0; i < suiLen_tem; i++) {
//	   	    IOT_Printf("%02x ",pucBuf_tem[i]);
//	   	}
//#endif
//		if(p[7] == 0x19) 	  					// ������ ��ѯ �ɼ���������ָ��
//		{
//			EMQTTREQTypeVal=MQTTREQ_CMD_0x19;
//			System_state.Host_setflag |= CMD_0X19;
//			 InvCommCtrl.CMDFlagGrop |= CMD_0X19;
//			System_state.ServerGET_REGNUM = ((unsigned int)p[18 + ParamOffset]<<8) + p[19 + ParamOffset];    		  // �Ĵ�����Ÿ���
//			IOT_Printf("\r\n******************System_state.ServerGET_REGNUM =%d***************\r\n ",System_state.ServerGET_REGNUM );
//
//			if(	System_state.ServerGET_REGNUM ==1)
//			{
//				System_state.ServerGET_REGAddrStart = ((unsigned int)p[20+i + ParamOffset]<<8) + p[21+i + ParamOffset];
//				System_state.ServerGET_REGAddrEnd = ((unsigned int)p[20+i + ParamOffset]<<8) + p[21+i + ParamOffset];
//			}
//			else
//			{
//				 for(i=0,j=0;i<System_state.ServerGET_REGNUM  ;i++,j++)
//				//System_state.ServerGET_REGAddrStart = ((unsigned int)p[18 + ParamOffset]<<8) + p[19 + ParamOffset];     // ��ʼ�������
//				 {
//					System_state.ServerGET_REGAddrStart = ((unsigned int)p[20+i+j + ParamOffset]<<8) + p[21+i+j + ParamOffset];       //��ֹ�������
//					System_state.ServerGET_REGAddrBUFF[i]=System_state.ServerGET_REGAddrStart ;
//	//				if(	System_state.ServerGET_REGNUM ==1)
//	//					System_state.ServerGET_REGAddrEnd = ((unsigned int)p[20+i + ParamOffset]<<8) + p[21+i + ParamOffset];         //��ֹ�������
//	//				else
//	//					System_state.ServerGET_REGAddrEnd = ((unsigned int)p[22+i + ParamOffset]<<8) + p[23+i + ParamOffset];         //��ֹ�������
//				 }
//			}
//		 	IOT_Printf("\r\n******************System_state.ServerGET_REGAddrStart=%d***************\r\n ",System_state.ServerGET_REGAddrStart);
//	 		IOT_Printf("\r\n******************System_state.ServerGET_REGAddrEnd =%d***************\r\n ",System_state.ServerGET_REGAddrEnd );
//			if(System_state.ServerGET_REGAddrStart == 75)
//			{
//			// if()//��ǰɨ��δ��ɻ��������У�������ɨ��
//			   IOT_WIFIEvenSetSCAN();
//			}
////			else
////			{
////				IOT_GattsEvenSetR();  //Ҫ�Ż���sys
////			}
//		}
//		else if(p[7] == 0x18) // ������ ���� �ɼ���������ָ��
//		{
//			EMQTTREQTypeVal=MQTTREQ_CMD_0x18;
//			System_state.Host_setflag |= CMD_0X18;
//		 	InvCommCtrl.CMDFlagGrop |= CMD_0X18;
//			System_state.ServerSET_REGNUM = ((p[18 + ParamOffset]<<8) + p[19 + ParamOffset] );
//			System_state.ServerSET_REGAddrLen = 0;
//			for(i=0; i<System_state.ServerSET_REGNUM ;i++)
//			{
//				System_state.ServerSET_REGAddrStart = ((unsigned int)p[22  + ParamOffset +DataOffset]<<8) + p[23+ ParamOffset+DataOffset]; //��ʼ�������
//				System_state.ServerSET_REGAddrLen =  ((unsigned int)p[24 + ParamOffset+DataOffset]<<8) + p[25 + ParamOffset+DataOffset];
//				IOT_Printf("\r\n******************ServerSET_REGAddrStart=%d******ServerSET_REGAddrLen=%d*********\r\n ",System_state.ServerSET_REGAddrStart,System_state.ServerSET_REGAddrLen);
//				if((System_state.BLEKeysflag==0 ) &&(System_state.ServerSET_REGAddrStart != 54 ) )  //û�з�����Կ
//				{
//					IOT_Printf("\r\n**********System_state.BLEKeysflag==0****System_state.ServerSET_REGAddrStart!= 54***********\r\n ");
//					System_state.Host_setflag  &= ~CMD_0X18;
//					InvCommCtrl.CMDFlagGrop &= ~CMD_0X18;
//					return EMQTTREQTypeVal;
//				}
//				IOT_ServerFunc_0x18(System_state.ServerSET_REGAddrStart,System_state.ServerSET_REGAddrLen,&p[26 + ParamOffset+DataOffset]);
//				DataOffset=System_state.ServerSET_REGAddrLen+4;
//			}
//			//ACK?
//		}
//		else if(p[7] == 0x17) // ͸������
//		{
//			EMQTTREQTypeVal=MQTTREQ_CMD_0x17;
//			System_state.Host_setflag |= CMD_0X17;
//			InvCommCtrl.CMDFlagGrop |= CMD_0X17;
//			InvCommCtrl.SetParamDataLen=  ((unsigned int)p[18 + ParamOffset]<<8) + p[19 + ParamOffset]; // ͸�������ݳ���
//		    memcpy( InvCommCtrl.SetParamData, &p[20 + ParamOffset], InvCommCtrl.SetParamDataLen );
//		}
//		else if(p[7] == 0x26) // ����
//		{
//			EMQTTREQTypeVal=MQTTREQ_CMD_0x26;
//			System_state.Host_setflag |= CMD_0X26;
//		}
//	}

    /* ��ջ��� */
	p = NULL;
	return EMQTTREQTypeVal;
}
/****************
 * �������� д��Яʽ��Դ�Ĵ���  03 ����
 * 56 �żĴ���
 * ��λ0-3 ���� bit0  ��1/��0   bit1  �����豸1/�Ͽ��豸0
 * ��λ4-7 wifi bit4  ��1/��0   bit5  �����豸1/�Ͽ��豸0
 *
 * ��Ҫ�����������㲻���䣬 ������Ҫ��ʱ���04���� ����ͬ��
 * ****************/
void IOT_KEYSET0X06( uint16_t uiSetREG,uint16_t  uiSetdate)
{

#if	test_wifi==0
	if(InvCommCtrl.DTC == 0xFEFE)
	{

	}
	else
	{
	System_state.Host_setflag |= CMD_0X06;
	InvCommCtrl.CMDFlagGrop |= CMD_0X06;
	InvCommCtrl.Set_InvParameterNum =uiSetREG;
	InvCommCtrl.Set_InvParameterData =uiSetdate;
	IOT_Printf("\r\n*IOT_KEYSET0X06**InvCommCtrl.Set_InvParameterNum=%d******Set_InvParameterData=%d*********\r\n ",InvCommCtrl.Set_InvParameterNum,InvCommCtrl.Set_InvParameterData);
	}
#endif

}

/********************************************
 * HoldingGrop  ���ּĴ��� ���
 * 03 ����
 * *****************************************/
uint16_t HoldingGrop_DataPacket(uint8_t *outdata)
{
	uint16_t i;
	volatile uint16_t len = 0; 					//�������
	volatile uint16_t dataLen = 0;				//���η��������ܳ�
	volatile uint16_t tmpLen = 0;					//������ʱ����ʹ�ó���
	len = 0;
	outdata[len++] = 0x00;
	outdata[len++] = 0x01;         //���ı�ţ��̶�01
	outdata[len++] = 0x00;
	outdata[len++] = ProtocolVS07;         //ʹ�õ�Э��汾
	for(i = 0; i < InvCommCtrl.HoldingNum; i++)
	{
		dataLen += ((InvCommCtrl.HoldingGrop[i][1] - InvCommCtrl.HoldingGrop[i][0] + 1) * 2);
	}
	dataLen += (InvCommCtrl.HoldingNum * 4);		//ÿ����ʼ2+����2

	outdata[len++] = HIGH((29 + dataLen));
	outdata[len++] = LOW((29 + dataLen));

	outdata[len++] = InvCommCtrl.Addr;        	//�������
	outdata[len++] = 0x03;         				//����-�ϴ��ɼ����ɼ�������������ݡ�

	memcpy(&outdata[len], &SysTAB_Parameter[Parameter_len[REG_SN]], 10);				//�ɼ���SN��
	len += 10;
	memcpy(&outdata[len], InvCommCtrl.SN, 10);	//�����SN��
	len += 10;

	outdata[len++] =0;         //6byte     ʱ���
	outdata[len++] =0;
	outdata[len++] =0;
	outdata[len++] =0;
	outdata[len++] =0;
	outdata[len++] =0;
	outdata[len++] = InvCommCtrl.HoldingNum;   // �ֶ�����
	dataLen = 0;
	tmpLen = 0;
	for(i = 0; i < InvCommCtrl.HoldingNum; i++)
	{
		/* �Ĵ������ */
		outdata[len++] = HIGH(InvCommCtrl.HoldingGrop[i][0]);
		outdata[len++] = LOW(InvCommCtrl.HoldingGrop[i][0]);
		outdata[len++] = HIGH(InvCommCtrl.HoldingGrop[i][1]);
		outdata[len++] = LOW(InvCommCtrl.HoldingGrop[i][1]);

		dataLen = ((InvCommCtrl.HoldingGrop[i][1] - InvCommCtrl.HoldingGrop[i][0] + 1) * 2);	//����Ĵ�������
		memcpy(&outdata[len], &InvCommCtrl.HoldingData[tmpLen], dataLen);

		len += dataLen;
		tmpLen += dataLen;
		if(tmpLen > DATA_MAX)break;// ��ֹ�������
	}
	//IOT_Protocol_ToSever(outdata, len);
	IOT_Printf("\r\n HoldingGrop_DataPacket********len =%d********  \r\n",len);
	return len;
}
/********************************************
 * InputGrop  ����Ĵ��� ���
 *  04����
 *******************************************/
uint16_t InputGrop_DataPacket(uint8_t *outdata)
{
	uint16_t i=0;
	uint16_t len = 0; 					//�������
	uint16_t dataLen = 0;				//���η��������ܳ�
	uint16_t tmpLen = 0;				//������ʱ����ʹ�ó���
	len = 0;
	outdata[len++] = 0x00;				//
	outdata[len++] = 0x01;        		//���ı�ţ��̶�01
	outdata[len++] = 0x00;				//
	outdata[len++] = ProtocolVS07;      //ʹ�õ�Э��汾

	for(i = 0; i < InvCommCtrl.InputNum; i++)
	{
		dataLen += ((InvCommCtrl.InputGrop[i][1] - InvCommCtrl.InputGrop[i][0] + 1) * 2);
	}
	dataLen += (InvCommCtrl.InputNum * 4); //ÿ����ʼ2+����2

	outdata[len++] = HIGH((29 + dataLen));
	outdata[len++] = LOW((29 + dataLen));

	outdata[len++] = InvCommCtrl.Addr;    //�������
	outdata[len++] = 0x04;         		  //����-�ϴ��ɼ����ɼ�������������ݡ�

	memcpy(&outdata[len], &SysTAB_Parameter[Parameter_len[REG_SN]], 10);	 //�ɼ���SN��
	len += 10;

	memcpy(&outdata[len], InvCommCtrl.SN, 10);			 //�����SN��
	len += 10;

	outdata[len++] = sTime.data.Year   ;         			  //6byte     ʱ���  28 29 30 31 32 33
	outdata[len++] = sTime.data.Month  ;
	outdata[len++] = sTime.data.Date   ;
	outdata[len++] = sTime.data.Hours  ;
	outdata[len++] = sTime.data.Minutes  ;
	outdata[len++] = sTime.data.Seconds  ;

	outdata[len++] = InvCommCtrl.InputNum;
	dataLen = 0;
	tmpLen = 0;
	for(i = 0; i < InvCommCtrl.InputNum; i++)   //125*2  =250 һ������
	{
		/* �Ĵ������ */
		outdata[len++] = HIGH(InvCommCtrl.InputGrop[i][0]);
		outdata[len++] = LOW(InvCommCtrl.InputGrop[i][0]);
		outdata[len++] = HIGH(InvCommCtrl.InputGrop[i][1]);
		outdata[len++] = LOW(InvCommCtrl.InputGrop[i][1]);

		dataLen = ((InvCommCtrl.InputGrop[i][1] - InvCommCtrl.InputGrop[i][0] + 1) * 2);	//����Ĵ�������
		memcpy(&outdata[len], &InvCommCtrl.InputData[tmpLen], dataLen);

		len += dataLen;
		tmpLen += dataLen;
		if(tmpLen > DATA_MAX)break;// ��ֹ�������
	}
#if DEBUG_MQTT
	IOT_Printf("\r\n InputGrop_DataPacket*************len =%d********  \r\n",len);
	for( i=0; i<len; i++)
	{
		IOT_Printf("%02x ",outdata[i]);
	}
	IOT_Printf("\r\n InputGrop_DataPacket********len =%d********  \r\n",len);

#endif

	return len;
}
/********************************************
 *  HoldingGrop  ����Ĵ��� ���
 *  03����
 *******************************************/
uint16_t Server0x05_DataPacket(uint8_t *outdata)
{
	uint8_t Nums = 0;
	uint16_t len = 0; 					//�������
	uint8_t  *pPVackbuf=InvCommCtrl.GetParamDataBUF;
	Nums = ((InvCommCtrl.SetParamEnd - InvCommCtrl.SetParamStart)+1)*2;
	Nums = Nums+6+12+4; 				// ���������ܸ���

	outdata[len++] = 0x00;
	outdata[len++] = 0x01; 				//���ı�ţ��̶�01
	outdata[len++] = 0x00;
	outdata[len++] = ProtocolVS07;      //ʹ�õ�Э��汾
	outdata[len++] = HIGH((Nums-6));
	outdata[len++] = LOW((Nums-6));						 // ���ݳ���
	outdata[len++] = InvCommCtrl.Addr;	// �豸��ַ
	outdata[len++] = 0x05;

	memcpy(&outdata[len], &SysTAB_Parameter[Parameter_len[REG_SN]], 10);	 //�ɼ���SN��
	len += 10;

	outdata[len++] = HIGH(InvCommCtrl.SetParamStart);    // ��ʼ�Ĵ�����ַH
	outdata[len++] = LOW(InvCommCtrl.SetParamStart);	 // ��ʼ�Ĵ�����ַL
	outdata[len++] = HIGH(InvCommCtrl.SetParamEnd);		 // �����Ĵ���H
	outdata[len++] = LOW(InvCommCtrl.SetParamEnd);		 // �����Ĵ���L

	if(InvCommCtrl.Set_InvRespondCode == PV_ACKOK)  //�������ѵ��ȡ������ ��Ϊ
	{
		memcpy(&outdata[22], &pPVackbuf[3],pPVackbuf[2]); // 01 03 06 01 02 03 04 05 06
		len+=pPVackbuf[2];
	}
	else  //�������ѵ��ȡ�������쳣
	{
		memset(&outdata[22], 0,(Nums-22));
		len+=(Nums-22);
	}
	return len;
}
/********************************************
 *  �������������  Ӧ���������״̬
 *  ��ACK ���� ������δʹ��
 *******************************************/

uint16_t Server0x06_DataPacket(uint8_t *outdata)
{
	uint8_t Nums = 17;          //���ݳ���
	uint16_t len = 0; 			//�������

	outdata[len++] = 0x00;		//���ı��
	outdata[len++] = 0x01; 		//���ı�ţ��̶�01
	outdata[len++] = 0x00;
	outdata[len++] = ProtocolVS07;      //ʹ�õ�Э��汾

	outdata[len++] = HIGH(Nums);
	outdata[len++] = LOW(Nums);			// ���ݳ���
	outdata[len++] = InvCommCtrl.Addr;	// �豸��ַ

	outdata[len++] = 0x06;				//������

	memcpy(&outdata[len], &SysTAB_Parameter[Parameter_len[REG_SN]], 10);		 //�ɼ���SN��
	len += 10;

	outdata[len++] = HIGH(InvCommCtrl.Set_InvParameterNum);  // ���õ������������� H
	outdata[len++] = LOW(InvCommCtrl.Set_InvParameterNum);	 // ���õ�������������L

	outdata[len++] = InvCommCtrl.Set_InvRespondCode;		 // 06���óɹ�/ʧ��״̬

	outdata[len++] = HIGH(InvCommCtrl.Set_InvParameterData); // Ҫ���õ����������H
	outdata[len++] = LOW(InvCommCtrl.Set_InvParameterData);	 // Ҫ���õ����������L
	return len;

}



/********************************************
 *  ������������� Ӧ���������״̬
 *  ��ACK ���� ������δʹ��
 *******************************************/
uint16_t Server0x10_DataPacket(uint8_t *outdata)
{
	uint8_t Nums = 17;          //���ݳ���
	uint16_t len = 0; 			//�������

	outdata[len++] = 0x00;		//���ı��
	outdata[len++] = 0x01; 		//���ı�ţ��̶�01
	outdata[len++] = 0x00;
	outdata[len++] = ProtocolVS07;      //ʹ�õ�Э��汾

	outdata[len++] = HIGH(Nums);
	outdata[len++] = LOW(Nums);		// ���ݳ���
	outdata[len++] = InvCommCtrl.Addr;	// �豸��ַ
	outdata[len++] = 0x10;		//������
	memcpy(&outdata[len], &SysTAB_Parameter[Parameter_len[REG_SN]], 10);		 //�ɼ���SN��
	len += 10;


	outdata[len++] = HIGH(InvCommCtrl.SetParamStart);  	 // ���õĿ�ʼ������������ H
	outdata[len++] = LOW(InvCommCtrl.SetParamStart);	 //
	outdata[len++] = HIGH(InvCommCtrl.SetParamEnd);      //
	outdata[len++] = LOW(InvCommCtrl.SetParamEnd);	     //

	outdata[len++] = InvCommCtrl.Set_InvRespondCode;		 // 06���óɹ�/ʧ��״̬

	return len;

}

/********************************************
 *  ��ȡ�ɼ�����������  Ӧ������ȡ������
 *  0x19 ����
 *  ����
 *******************************************/
uint16_t Server0x19_DataPacket(uint8_t *outdata)
{
	uint16_t len = 0; 				 // �������
	uint16_t TABNumslen =  0 ;       // ���ݳ���
	uint16_t TABNums = 0 ;			 // �������
	TABNums = System_state.ServerGET_REGAddrStart;
	TABNumslen = IOT_CollectorParam_Get(System_state.ServerGET_REGAddrStart, System_state.Get_TabDataBUF );

	//GetParam();

	outdata[len++] = 0x00;			// ���ı��
	outdata[len++] = 0x01; 			// ���ı�ţ��̶�01
	outdata[len++] = 0x00;
	outdata[len++] = ProtocolVS07;      	// ʹ�õ�Э��汾

	outdata[len++] = HIGH((TABNumslen+16));
	outdata[len++] = LOW((TABNumslen+16));// ���ݳ���

	outdata[len++] = 1;				// �豸��ַ

	outdata[len++] = 0x19;			//������

	memcpy(&outdata[len], &SysTAB_Parameter[Parameter_len[REG_SN]], 10);		 //�ɼ���SN��
	len += 10;

	IOT_Printf("\r\n Server0x19_DataPacket********TABNums =%d********  \r\n",TABNums);
	IOT_Printf("\r\n ***********************TABNumslen =%d********************* \r\n"  ,TABNumslen );

	outdata[len++] = HIGH(TABNums);  		// ���õ� ������� H
	outdata[len++] = LOW(TABNums);	 		// ���õ� �������L
	outdata[len++] = HIGH(TABNumslen); 	 	// Ҫ���õ� ����H
	outdata[len++] = LOW(TABNumslen);	 	// Ҫ���õ� ����L

	memcpy(&outdata[len], System_state.Get_TabDataBUF, TABNumslen);

	len+= TABNumslen;
	IOT_Printf("\r\n Server0x19_DataPacket********System_state.Get_TabDataBUF len=%d********  \r\n",len);
	for( uint16_t a=0; a<len; a++)
	{
		IOT_Printf("%02x ",outdata[a]);
	}
	IOT_Printf("\r\n ***********************TABNumslen =%d********************* \r\n"  ,TABNumslen );

	return len;
}

/********************************************
 *  ��ȡ�ɼ�����������  Ӧ������ȡ������
 *  0x18 ����
 *******************************************/
uint16_t Server0x18_DataPacket(uint8_t *outdata)
{
	uint16_t len = 0; 				 // �������
	uint16_t TABNumslen =  0 ;       // ���ݳ���
	uint16_t TABNums = 0 ;			 // �������
	TABNums = System_state.ServerSET_REGAddrStart;
	//TABNumslen = IOT_CollectorParam_Get(System_state.ServerGET_REGAddrStart, System_state.Get_TabDataBUF );
	//GetParam();
	outdata[len++] = 0x00;			// ���ı��
	outdata[len++] = 0x01; 			// ���ı�ţ��̶�01
	outdata[len++] = 0x00;
	outdata[len++] = ProtocolVS07;  // ʹ�õ�Э��汾

	outdata[len++] = 0x00;
	outdata[len++] = 15;		// ���ݳ���

	outdata[len++] = 0x01;			// �豸��ַ

	outdata[len++] = 0x18;			//������

	memcpy(&outdata[len], &SysTAB_Parameter[Parameter_len[REG_SN]], 10);		 //�ɼ���SN��
	len += 10;

	IOT_Printf("\r\n Server0x18_DataPacket********TABNums =%d********  \r\n",TABNums);
	IOT_Printf("\r\n ***********************TABNumslen =%d********************* \r\n"  ,TABNumslen );

	outdata[len++] = HIGH(TABNums);  		// ���õ� ������� H
	outdata[len++] = LOW(TABNums);	 		// ���õ� �������L
	outdata[len++] = System_state.ParamSET_ACKCode ; 	 				// ���� �ɹ�
#if DEBUG_MQTT
	IOT_Printf("\r\n Server0x18_DataPacket*******outdata len=%d********  \r\n",len);
	for( uint16_t a=0; a<len; a++)
	{
		IOT_Printf("%02x ",outdata[a]);
	}
	IOT_Printf("\r\n ***********************TABNumslen =%d********************* \r\n"  ,TABNumslen );
#endif

	return len;
}
/********************************************
 *   �ɼ�����������  �����ϱ������б�����
 *   0x19 ���� ��������   07Э�����ϰ汾֧��
 *******************************************/
uint16_t Server0x19_MultiPack(uint8_t *outdata,uint16_t REGAddrStart ,uint16_t REGAddrEnd)
{

//	System_state.ServerGET_REGAddrStart; //��������ѯ�ɼ����Ĵ�����ַ��ʼ
//	System_state.ServerGET_REGAddrEnd;	 //��������ѯ�ɼ����Ĵ�����ַ����
//	System_state.Get_TabDataBUF;		 //Ҫ���͵�����
//
//	if(System_state.ServerGET_REGAddrStart-System_state.ServerGET_REGAddrEnd > 0 )  // ����Ĵ�����ȡ
//	{
//	}

	uint16_t len = 0; 				 // �������
	uint16_t TABNumslen =  0 ;       // ��ȡ�� ���ݳ���
	uint16_t TABNowNums = 0;		 // ��ǰ�������
	uint16_t TABnumber =0;			 // �ϱ������Ĵ�������
	uint16_t REGdatalen=0;			 // �������ݳ���

	if((REGAddrEnd==31)&& (REGAddrStart==4)) // ����31 ��ͬ��ʱ��
	{
		uint8_t i = 0;
		TABnumber =  abs(REGAddrEnd -REGAddrStart)+1;
		System_state.ServerGET_REGNUM = abs(REGAddrEnd -REGAddrStart)+1;
		for(i = 0 ; i < System_state.ServerGET_REGNUM ;i++)
		{
			System_state.ServerGET_REGAddrBUFF[i] = i + REGAddrStart;
		}

		System_state.ServerGET_REGNUM += 1;			//�����ϴ��ź�ֵ
		System_state.ServerGET_REGAddrBUFF[i] = 76;

	}
//	TABnumber =  abs(REGAddrEnd -REGAddrStart)+1;
	TABnumber =  System_state.ServerGET_REGNUM ;


	if(TABnumber == 1)
	{
		TABNowNums = REGAddrStart;
	}
	else
	{
		TABNowNums = System_state.ServerGET_REGAddrBUFF [0];
	}


	if(TABNowNums==75)
	{
		TABNumslen = strlen((char *)System_state.Get_TabDataBUF);
		REGdatalen=REGdatalen+ TABNumslen;
	}
	else
	{
//		TABNumslen = IOT_CollectorParam_Get(REGAddrStart, System_state.Get_TabDataBUF );
//		for(uint16_t j=0; j<TABnumber ;j++)
//		{
//			REGdatalen += Collector_Config_LenList[TABNowNums];
//		}
//		TABNumslen=0;

		if(TABnumber == 1)
		{
			REGdatalen += Collector_Config_LenList[TABNowNums];
		}
		else
		{
			for(uint16_t j=0; j<TABnumber ;j++)
			{
				REGdatalen += Collector_Config_LenList[System_state.ServerGET_REGAddrBUFF [j]];
			}
		}
	}
	//GetParam();
	TABNowNums = REGAddrStart;
	outdata[len++] = 0x00;			// ���ı��
	outdata[len++] = 0x01; 			// ���ı�ţ��̶�01

	outdata[len++] = 0x00;
	outdata[len++] = ProtocolVS07;       // ʹ�õ�Э��汾

	outdata[len++] = HIGH((TABnumber*4+REGdatalen +15));
	outdata[len++] = LOW((TABnumber*4+REGdatalen +15));// �����ݳ���

	outdata[len++] = 1;					 // �豸��ַ
	outdata[len++] = 0x19;				 // ������

	memcpy(&outdata[len], &SysTAB_Parameter[Parameter_len[REG_SN]], 10);		 //�ɼ���SN��
	len += 10;

	outdata[len++] = HIGH(TABnumber); 	 // Ҫ���õĲ�����Ÿ���H
	outdata[len++] = LOW(TABnumber);	 // Ҫ���õĲ�����Ÿ���L

	outdata[len++] = 0x00;	 			 // ״̬��


	if(TABNowNums==75)
	{
		outdata[len++] = HIGH(TABNowNums); 	 // Ҫ���õı�� H
		outdata[len++] = LOW(TABNowNums);	 // Ҫ���õı�� L
		outdata[len++] = HIGH(TABNumslen ); 	 // Ҫ���õı�Ų������� H
		outdata[len++] = LOW(TABNumslen );	 	 // Ҫ���õı�Ų������� L
		memcpy(&outdata[len] ,System_state.Get_TabDataBUF,TABNumslen);
	 	len+= TABNumslen;
	 	IOT_Printf("\r\n ***********TABNowNums==75************TABNumslen=%d********************* \r\n" ,TABNumslen  );
	}
	else
	{
		if(TABnumber == 1)
		{
			outdata[len++] = HIGH(TABNowNums); 	 // Ҫ���õı�� H
			outdata[len++] = LOW(TABNowNums);	 // Ҫ���õı�� L
			outdata[len++] = HIGH(Collector_Config_LenList[TABNowNums]); 	 // Ҫ���õı�Ų������� H
			outdata[len++] = LOW(Collector_Config_LenList[TABNowNums]);	 	 // Ҫ���õı�Ų������� L
			memcpy(&outdata[len] ,&SysTAB_Parameter[Parameter_len[TABNowNums]],Collector_Config_LenList[TABNowNums]);		 //��������
			len+= Collector_Config_LenList[TABNowNums];
		}
		else
		{
			for(uint16_t i=0; i<TABnumber ;i++)
			{
			outdata[len++] = HIGH(System_state.ServerGET_REGAddrBUFF [i]); 	 // Ҫ���õı�� H
			outdata[len++] = LOW(System_state.ServerGET_REGAddrBUFF [i]);	 // Ҫ���õı�� L
			outdata[len++] = HIGH(Collector_Config_LenList[System_state.ServerGET_REGAddrBUFF [i]]); 	 // Ҫ���õı�Ų������� H
			outdata[len++] = LOW(Collector_Config_LenList[System_state.ServerGET_REGAddrBUFF [i]]);	 	 // Ҫ���õı�Ų������� L

			memcpy(&outdata[len] ,&SysTAB_Parameter[Parameter_len[System_state.ServerGET_REGAddrBUFF [i]]],Collector_Config_LenList[System_state.ServerGET_REGAddrBUFF [i]]);		 //��������

			len+= Collector_Config_LenList[System_state.ServerGET_REGAddrBUFF [i]];

#if DEBUG_MQTT
			for( uint16_t b=0; b< 4+Collector_Config_LenList[System_state.ServerGET_REGAddrBUFF [i]]; b++)
			{
				IOT_Printf("%02x ",outdata[(len-(4+Collector_Config_LenList[System_state.ServerGET_REGAddrBUFF [i]]) )+b ]);
			}
			IOT_Printf("\r\n **********************b=%d*****len=%d**************** \r\n",i  ,len );
#endif

			}
		}

	}
#if DEBUG_MQTT
	IOT_Printf("\r\n Server0x19_MultiPack**REGAddrStart=%d***REGAddrEnd=%d**len =%d********  \r\n",REGAddrStart,REGAddrEnd,len);
	for( uint16_t a=0; a<len; a++)
	{
		IOT_Printf("%02x ",outdata[a]);
	}
	IOT_Printf("\r\n **********Server0x19_MultiPack*********return len=%d************************* \r\n" ,len  );
#endif

	return len;
}
/********************************************
 *  ͸�����ݴ��
 *  0x17����
 *******************************************/

uint16_t Server0x17_DataPacket(uint8_t *outdata)
{

	uint8_t Nums = 0;
	uint16_t len = 0; 					// �������
	uint8_t  *pPVackbuf=InvCommCtrl.GetParamDataBUF;

	Nums = InvCommCtrl.GetParamData ;
 			// ���������ܸ���

	outdata[len++] = 0x00;
	outdata[len++] = 0x01; 				// ���ı�ţ��̶�01
	outdata[len++] = 0x00;
	outdata[len++] = ProtocolVS07;      // ʹ�õ�Э��汾
	outdata[len++] = HIGH((Nums+14));	//
	outdata[len++] = LOW((Nums+14));		// ���ݳ���
	outdata[len++] = InvCommCtrl.Addr;	// �豸��ַ
	outdata[len++] = 0x17;

	memcpy(&outdata[len], &SysTAB_Parameter[Parameter_len[REG_SN]], 10);	 //�ɼ���SN��
	len += 10;

	outdata[len++] = HIGH(Nums);    // ��ʼ�Ĵ�����ַH
	outdata[len++] = LOW(Nums);	 // ��ʼ�Ĵ�����ַL

	memcpy(&outdata[len], &pPVackbuf[0],Nums); // 01 03 06 01 02 03 04 05 06
	len+= Nums;

//	IOT_Printf("\r\n IOT_0x17Receive*************len =%d********  \r\n",len);
//	for( uint16_t i=0; i<len; i++)
//	{
//			 IOT_Printf("%02x ",outdata[i]);
//	}
//	ESP_LOGI(test1 ,"****\r\n");

	return len;
}

/*******************************************************************************
 * FunctionName : IOT_ESPSend_0x03
 * Description  : esp ������������(0x03)
 * Parameters   : char *upSendTXBUF    : ���ͻ�����
 * Returns      : len Ҫ���͵����ݳ���
 * Notice       : none
*******************************************************************************/
uint16_t SendIDcom_num=0;
uint16_t bCID_0x04=0;
uint16_t IOT_ESPSend_0x03(uint8_t *upSendTXBUF ,unsigned int Vsnum)
{
	uint16_t len = 0;
	uint16_t sendlen=0;

	len = HoldingGrop_DataPacket(upSendTXBUF);			//�������
	sendlen =IOT_Protocol_ToSever(upSendTXBUF, len ,Vsnum);    //Э�����
	return sendlen;
}

/********************************************************************
 * FunctionName : IOT_ESPSend_0x04
 * Description  : esp ������������(0x04)
 * Parameters   : char *upSendTXBUF  : ���ͻ�����
 * Returns      : len Ҫ���͵����ݳ���
 * Notice       : none
*********************************************************************/
uint16_t IOT_ESPSend_0x04(uint8_t *upSendTXBUF ,unsigned int Vsnum)
{
	uint16_t len = 0;
	uint16_t sendlen=0;

	len = InputGrop_DataPacket(upSendTXBUF);			//�������

	if(SendIDcom_num < 125)								//����CID���
		SendIDcom_num++;
	else
		SendIDcom_num = 0;

	upSendTXBUF[0] = SendIDcom_num >> 8;				//����ָ��ͨѶ���
	upSendTXBUF[1] = SendIDcom_num & 0x00ff;
	bCID_0x04 = SendIDcom_num;							//����0x04CID

	//�Ѿ��ڴ����������ʹ�ø�ֵ����������
	upSendTXBUF[28] = sTime.Clock_Time_Tab[0];			//�꣬����ʱ����
	upSendTXBUF[29] = sTime.Clock_Time_Tab[1];
	upSendTXBUF[30] = sTime.Clock_Time_Tab[2];

	upSendTXBUF[31] = sTime.Clock_Time_Tab[3];
	upSendTXBUF[32] = sTime.Clock_Time_Tab[4];
	upSendTXBUF[33] = sTime.Clock_Time_Tab[5];

	sendlen =IOT_Protocol_ToSever(upSendTXBUF, len ,Vsnum);
	return sendlen;

}

/********************************************************************
 * FunctionName : IOT_ESPSend_0x05
 * Description  : esp ������������(0x05)
 * Parameters   : char *upSendTXBUF    : ���ͻ�����
 * Returns      : len Ҫ���͵����ݳ���
 * Notice       : none
*********************************************************************/
uint16_t IOT_ESPSend_0x05(uint8_t *upSendTXBUF ,unsigned int Vsnum)
{
	uint16_t len = 0;
	uint16_t sendlen=0;
	len = Server0x05_DataPacket(upSendTXBUF);			//�������
	sendlen =IOT_Protocol_ToSever(upSendTXBUF, len ,Vsnum);    //Э�����
	return sendlen;

}

/*********************************************************************
 * FunctionName : IOT_ESPSend_0x06
 * Description  : esp ������������(0x06)
 * Parameters   : char *upSendTXBUF    : ���ͻ�����
 * Returns      : len Ҫ���͵����ݳ���
 * Notice       : none
**********************************************************************/
uint16_t IOT_ESPSend_0x06(uint8_t *upSendTXBUF ,unsigned int Vsnum)
{
	uint16_t len = 0;
	uint16_t sendlen=0;
	len = Server0x06_DataPacket(upSendTXBUF);			//�������
	sendlen =IOT_Protocol_ToSever(upSendTXBUF, len ,Vsnum);    //Э�����
	return sendlen;
}

/*********************************************************************
 * FunctionName : IOT_ESPSend_0x10
 * Description  : esp ������������(0x05)
 * Parameters   : char *upSendTXBUF    : ���ͻ�����
 * Returns      : len Ҫ���͵����ݳ���
 * Notice       : none
*********************************************************************/
uint16_t IOT_ESPSend_0x10(uint8_t *upSendTXBUF ,unsigned int Vsnum)
{
	uint16_t len = 0;
	uint16_t sendlen=0;
	len = Server0x10_DataPacket(upSendTXBUF);			//�������
	sendlen =IOT_Protocol_ToSever(upSendTXBUF, len, Vsnum);    //Э�����
	return sendlen;

}

/********************************************************************
 * FunctionName : IOT_ESPSend0x19
 * Description  : esp ������������(0x19)
 * Parameters   : char *upSendTXBUF                      : ���ͻ�����
 *             	  uint16_t sIN_REGAddrStart              : ��ȡ����� ��ʼ��ַ
 *                uint16_t sIN_REGAddrEnd				 : ��ȡ����� ������ַ
 * Returns      : len Ҫ���͵����ݳ���
 * Notice       : none
*********************************************************************/
uint16_t IOT_ESPSend_0x19(uint8_t *upSendTXBUF ,unsigned int Vsnum)
{
	uint16_t len = 0;
	uint16_t sendlen=0;
 	len = Server0x19_MultiPack(upSendTXBUF, System_state.ServerGET_REGAddrStart, System_state.ServerGET_REGAddrEnd);	//�������
 	sendlen =IOT_Protocol_ToSever(upSendTXBUF, len ,Vsnum);    //Э�����
	return sendlen;
}

/********************************************************************
 * FunctionName : IOT_ESPSend0x18
 * Description  : esp ������������(0x18)
 * Parameters   : char *upSendTXBUF                      : ���ͻ�����
 *             	  uint16_t sIN_REGAddrStart              : ��ȡ����� ��ʼ��ַ
 *                uint16_t sIN_REGAddrEnd				 : ��ȡ����� ������ַ
 * Returns      : len Ҫ���͵����ݳ���
 * Notice       : none
*********************************************************************/
uint16_t IOT_ESPSend_0x18(uint8_t *upSendTXBUF  ,unsigned int Vsnum)
{
	uint16_t len = 0;
	uint16_t sendlen=0;
//	if( System_state.ServerSET_REGAddrStart ==75)// ��ȡ����wifi�ȵ� ����,������ݷ���
//		len = Server0x19_MultiPack(upSendTXBUF, System_state.ServerSET_REGAddrStart, System_state.ServerSET_REGAddrStart);			//�������
//	else
		len = Server0x18_DataPacket(upSendTXBUF);			//�������

	sendlen =IOT_Protocol_ToSever(upSendTXBUF, len ,Vsnum);    //Э�����
	return sendlen;

}

/********************************************************************
 * FunctionName : IOT_ESPSend0x17
 * Description  : esp ������������(0x17)
 * Parameters   : char *upSendTXBUF                      : ���ͻ�����
 *             	  uint16_t sIN_REGAddrStart              : ��ȡ����� ��ʼ��ַ
 *                uint16_t sIN_REGAddrEnd				 : ��ȡ����� ������ַ
 * Returns      : len Ҫ���͵����ݳ���
 * Notice       : none
*********************************************************************/
uint16_t IOT_ESPSend_0x17(uint8_t *upSendTXBUF ,unsigned int Vsnum)
{
	uint16_t len = 0;
	uint16_t sendlen=0;
 	len = Server0x17_DataPacket(upSendTXBUF);			//�������
//	ESP_LOGI(test1 ,"IOT_ESPSend_0x17 len= %d****\r\n",len);
 	sendlen =IOT_Protocol_ToSever(upSendTXBUF, len ,Vsnum);    //Э�����
	return sendlen;
}



/****************************************************************
 * FunctionName : IOT_UploadParamSelf
�����ϴ�19������Ϣ��������, ������������Ӱ��
*****************************************************************/
#define SELF_UPLOAD_STR		4				//��ʼ�ϴ��������
#define SELF_UPLOAD_END		22				//��ֹ�ϴβ������
#define TIME_UPLPAD_AGAIN	(10 * 60 * 199)	//10�����ڲ����������ظ��ϴ�����
#define TIME_FIRST_UPLPAD	(3 * 60 * 199)	//10�����ڲ����������ظ��ϴ�����

uint16_t IOT_UploadParamSelf(uint8_t *upSendTXBUF ,unsigned int Vsnum)
{
	uint16_t len = 0;
	uint16_t sendlen=0;
 	len = Server0x19_MultiPack(upSendTXBUF,4,31);		//�������
	sendlen =IOT_Protocol_ToSever(upSendTXBUF, len,Vsnum);    //Э�����
	return sendlen;
}

#ifndef HTTP_PROGRESS
#define HTTP_PROGRESS		(0xa2)			//�������ʹ���
#define HTTP_DL_ERROR		(0x85)			//���ع̼�ʧ��
#endif
/********************************************************************
 * FunctionName : IOT_ESPSend_0x25
 * Description  : ���������ϴ�
 * Parameters   : char *upSendTXBUF                      : ���ͻ�����
 *             	  uint16_t sIN_REGAddrStart              : ��ȡ����� ��ʼ��ַ
 *                uint16_t sIN_REGAddrEnd				 : ��ȡ����� ������ַ
 * Returns      : len Ҫ���͵����ݳ���
 * Notice       : none
*********************************************************************/
uint16_t IOT_ESPSend_0x25(uint8_t *upSendTXBUF ,unsigned int Vsnum )
{
	uint16_t sendlen = 0;
	uint8_t *TxBuf = upSendTXBUF;
	/* �ϴ�����0% */
	TxBuf[0] = 0x00;
	TxBuf[1] = 0x01;
	TxBuf[2] = 0x00;
	TxBuf[3] = LOW(Vsnum);
	TxBuf[4] = 0x00;
	TxBuf[5] = 0x17;
	TxBuf[6] = InvCommCtrl.Addr;
	TxBuf[7] = 0x25;

	memcpy(&TxBuf[8], &SysTAB_Parameter[Parameter_len[REG_SN]], 10);	 //�ɼ���SN��
	memcpy(&TxBuf[18], InvCommCtrl.SN, 10);			 //�����SN��

	/* HTTP���ؽ��� */
	if(g_FotaParams.ProgressType == HTTP_PROGRESS)
	{
		TxBuf[28] = HTTP_PROGRESS;
		TxBuf[29] = g_FotaParams.uiFotaProgress;

		TxBuf[5] = 0x18;
		sendlen = IOT_Protocol_ToSever(TxBuf, 30, Vsnum);    //Э�����
	}
	/* ������������� */
	else
	{
//		if(g_FotaParams.uiFotaProgress > 95)
//		{
//			TxBuf[28] = 100;
//
//		}
//		else
//		{
//			TxBuf[28] = g_FotaParams.uiFotaProgress;
//		}

		TxBuf[28] = g_FotaParams.uiFotaProgress;
		sendlen = IOT_Protocol_ToSever(TxBuf, 29, Vsnum);    //Э�����
	}
	IOT_Printf("\r\n IOT_ESPSend_0x25*****  g_FotaParams.uiFotaProgress  =%d****** \r\n", g_FotaParams.uiFotaProgress);
	return sendlen;
}

/********************************************
 *  APP���豸���ձ����������ݰ�
 *
 *******************************************/
uint16_t Server0x26_DataPacket(uint8_t *outdata)
{
	uint8_t Nums = 17;          //���ݳ���
	uint16_t len = 0; 			//�������

	outdata[len++] = 0x00;		//���ı��
	outdata[len++] = 0x01; 		//���ı�ţ��̶�01
	outdata[len++] = 0x00;
	outdata[len++] = ProtocolVS05;      //ʹ�õ�Э��汾

	outdata[len++] = HIGH(Nums);
	outdata[len++] = LOW(Nums);			// ���ݳ���
	outdata[len++] = InvCommCtrl.Addr;	// �豸��ַ

	outdata[len++] = 0x26;		//������

	memcpy(&outdata[len], &SysTAB_Parameter[Parameter_len[REG_SN]], 10);	 //�ɼ���SN��
	len += 10;

	outdata[len++] = HIGH(InvCommCtrl.Set_InvParameterNum);  // ���õ������������� H
	outdata[len++] = LOW(InvCommCtrl.Set_InvParameterNum);	 // ���õ�������������L

	outdata[len++] = InvCommCtrl.Set_InvRespondCode;		 // 06���óɹ�/ʧ��״̬

	outdata[len++] = HIGH(InvCommCtrl.Set_InvParameterData); // Ҫ���õ����������H
	outdata[len++] = LOW(InvCommCtrl.Set_InvParameterData);	 // Ҫ���õ����������L

	return len;
}

/*****************************************************************
  *     �������ݵ�������
  *     ���Ͳɼ�����0~44��,45~89�����ݸ���������
  *     ָ���0x50������������
  ****************************************************************
  */
 uint16_t Server0x50_DataPacket(uint8_t *outdata)
{
	uint16_t len = 0;
	uint8_t *TxBuf = outdata;

	if(NULL == TxBuf) return 0;

	//len = readRecord(TxBuf);//  �˴����������һ���������ݶ�ȡ
	len = IOT_ESP_SPI_FLASH_ReUploadData_Read_us(TxBuf , 1024);


	IOT_Printf("==================================\r\n");
	IOT_Printf(" Server0x50_DataPacket() readRecord  len= %d\r\n", len);
	IOT_Printf("==================================\r\n");
	if(len ==0)
	{
		IOT_Printf("===========Server0x50_DataPacket   error  return=======================\r\n");

		return 0;
	}

	TxBuf += 16;  //ƫ��16�ֽ�
	len -= 16;

	/* ���洢�ļ�¼�ɼ���SN�͵�ǰ�Ƿ�һ�� */
	// �ϵ��������� ���к�У�鵼�²��ϴ�����
	if(memcmp(&TxBuf[8], &SysTAB_Parameter[Parameter_len[REG_SN]],10) != 0)			//����Ĳ��ǵ�ǰ�����SN����ɼ���SN
	{
		IOT_Printf("======���洢�ļ�¼�ɼ���SN�͵�ǰ��һ��====���ϴ�========\r\n");
		return 0;
	}
	/* ���洢�ļ�¼�����SN�͵�ǰ�Ƿ�һ�� */
    //�ϵ��������� ���к�У�鵼�²��ϴ�����
	if(memcmp(&TxBuf[18],InvCommCtrl.SN,10) != 0)			//����Ĳ��ǵ�ǰ�����SN����ɼ���SN
	{
		IOT_Printf("======���洢�ļ�¼�����SN�͵�ǰ��һ��====���ϴ�========\r\n");
		return 0;
	}
	/* ���汾���Ƿ�Ϊ��ǰ�汾 */
	if(TxBuf[3] != PROTOCOL_VERSION_L)
	{
		IOT_Printf("======���洢�ļ�¼���汾�ź͵�ǰ��һ��====���ϴ�========\r\n");
		return 0;
	}

	memcpy(outdata , TxBuf , len);


	if(SendIDcom_num < 125)								//����CID���  �� 04 ��ID ��ͬ
		SendIDcom_num++;
	else
		SendIDcom_num = 0;

	outdata[0] = SendIDcom_num >> 8;		//����ָ��ͨѶ���
	outdata[1] = SendIDcom_num & 0x00ff;
	outdata[7] = 0x50;			//������ָ��0x50;
	bCID_0x04 = SendIDcom_num;


#if DEBUG_MQTT
	IOT_Printf("����0x50ָ�� = \r\n");
	for(uint16_t i=0; i<len; i++)
		IOT_Printf("%02x ",outdata[i]);
	IOT_Printf("\r\n Server0x50_DataPacket\r\n");
#endif



	return len;

}

 /********************************************************************
  * FunctionName : IOT_ESPSend_0x50
  * Description  : esp ������������(0x50)
  * Parameters   : char *upSendTXBUF                      : ���ͻ�����
 	 	 	 	 	 unsigned int Vsnum 				  : Э��汾
  * Returns      : len Ҫ���͵����ݳ���
  * Notice       : none
 *********************************************************************/
uint16_t IOT_ESPSend_0x50(uint8_t *upSendTXBUF ,unsigned int Vsnum)
{
 	uint16_t len = 0;
 	uint16_t sendlen=0;
  	len = Server0x50_DataPacket(upSendTXBUF);			//�������
  	if(len ==0)
	{
		return 0;
	}
  	sendlen =IOT_Protocol_ToSever(upSendTXBUF, len ,Vsnum);    //Э�����

#if 1
  	IOT_Printf("IOT_ESPSend_0x50 :\r\n");
  	for(int i = 0 ; i < sendlen;i++)
  	{
  		IOT_Printf("%02x ",upSendTXBUF[i]);
  	}
  	IOT_Printf("\r\n");
#endif

 	return sendlen;
}


/********************************************************************
   * FunctionName : IOT_ESPSend_0x20
   * Description  : esp ������������(0x20)
   * Parameters   : char *upSendTXBUF                     : ���ͻ�����
  	 	 	 	 	 unsigned int Vsnum 				  : Э��汾
   * Returns      : len Ҫ���͵����ݳ���
   * Notice       : none
*********************************************************************/
uint16_t IOT_ESPSend_0x20(uint8_t *upSendTXBUF ,unsigned int Vsnum)
{
  	uint16_t len = 0;
  	uint16_t sendlen=0;
   	len = IOT_Ammeter_DataPacket(upSendTXBUF);			//�������
   	if(len ==0)
 	{
 		return 0;
 	}
   	sendlen =IOT_Protocol_ToSever(upSendTXBUF, len ,Vsnum);    //Э�����
  	return sendlen;
}

/********************************************************************
    * FunctionName : IOT_ESPSend_0x22
    * Description  : esp ������������(0x22)
    * Parameters   : char *upSendTXBUF                     : ���ͻ�����
   	 	 	 	 	 unsigned int Vsnum 				  : Э��汾
    * Returns      : len Ҫ���͵����ݳ���
    * Notice       : none
*********************************************************************/
uint16_t IOT_ESPSend_0x22(uint8_t *upSendTXBUF ,unsigned int Vsnum)
{
   	uint16_t len = 0;
   	uint16_t sendlen=0;
    len = IOT_History_Ammeter_DataPacket(upSendTXBUF);			//�������
    if(len ==0)
  	{
  		return 0;
  	}
    sendlen =IOT_Protocol_ToSever(upSendTXBUF, len ,Vsnum);    //Э�����
   	return sendlen;
}


/********************************************
 * һ���������
 *
 *******************************************/
uint16_t Server0x14_DataPacket(uint8_t *outdata ,uint16_t start_addr, uint16_t end_addr, uint8_t *msg, uint16_t msg_len)
{
	uint8_t i = 0;


	uint8_t Nums = (8 + 20 + 6 + 1 + 4 + msg_len)-6;          //���ݳ���
	uint16_t len = 0; 			//�������

	outdata[len++] = 0x00;		//���ı��
	outdata[len++] = 0x01; 		//���ı�ţ��̶�01
	outdata[len++] = 0x00;
	outdata[len++] = ProtocolVS07;      //ʹ�õ�Э��汾

	outdata[len++] = HIGH(Nums);
	outdata[len++] = LOW(Nums);			// ���ݳ���
	outdata[len++] = InvCommCtrl.Addr;	// �豸��ַ

	outdata[len++] = 0x14;		//������

	memcpy(&outdata[len], &SysTAB_Parameter[Parameter_len[REG_SN]], 10);	 //�ɼ���SN��
	len += 10;
	memcpy(&outdata[len], InvCommCtrl.SN, 10);			 //�����SN��
	len += 10;

	//ʱ�� �ϱ�1500v pv��������    2020-05-09
	for(i=0 ;i < 6 ;i++)
	{
		outdata[len ++] = IOT_InverterPVWaveNums_Handle_uc(1,0);
	}
	outdata[len++] = 0x01;// ���ݶ���
	outdata[len++] = HIGH(start_addr);			// ���ݼĴ�����ʼ��ַ
	outdata[len++] = LOW(start_addr);
	outdata[len++] = HIGH(end_addr);  			// ���ݼĴ���������ַ
	outdata[len++] = LOW(end_addr);
	memcpy(&outdata[len],(char*)msg,msg_len);	// �õ�����������
	len+=msg_len ;

	return len;
}
/********************************************************************
    * FunctionName : IOT_ESPSend_0x14
    * Description  : esp ������������(0x14)
    * Parameters   : char *upSendTXBUF                     : ���ͻ�����
   	 	 	 	 	 unsigned int Vsnum 				  : Э��汾
    * Returns      : len Ҫ���͵����ݳ���
    * Notice       : none
*********************************************************************/
uint16_t IOT_ESPSend_0x14(uint8_t *upSendTXBUF ,unsigned int Vsnum)
{
   	uint16_t len = 0;
   	uint16_t sendlen=0;
   	len =Server0x14_DataPacket(upSendTXBUF,InvCommCtrl.SetParamStart,\
   			InvCommCtrl.SetParamEnd ,InvCommCtrl.GetParamDataBUF,InvCommCtrl.GetParamData);// ���͵�������
    sendlen =IOT_Protocol_ToSever(upSendTXBUF, len ,Vsnum);    //Э�����
   	return sendlen;
}

/*****************************************************************
  *     ��Ӧ0x14ָ��
  *		Status:	00 	��Ӧ�ɹ�ָ��
  *				01	����Ӧ
  *				02	��Ӧʧ��ָ��
  ****************************************************************
  *///(uint8_t *pRead,uint8_t QoCack)
void IOT_ServerACK_0x14(void)
{

//	if(QoCack==0)
//	{
//		if(pRead[8] == 0x00) // 0x14Ӧ��״̬��
//		{
//			InverterWaveDataNum_Count(1);
//		}
//		else
//		{
//			InverterWaveDataNum_Count(2);
//		}
//	}
//	else
	{
		InverterWaveDataNum_Count(1);
	}
	InverterWaveStatus_Judge(0,IOT_ING);	// ���ò������ݻ�ȡ�����б�־
}

/********************************************************************
    * FunctionName : IOT_ESPSend_0x37
    * Description  : esp ������������(0x37)
    * Parameters   : char *upSendTXBUF                     : ���ͻ�����
   	 	 	 	 	 unsigned int Vsnum 				  : Э��汾
    * Returns      : len Ҫ���͵����ݳ���
    * Notice       : none
*********************************************************************/
uint16_t IOT_ESPSend_0x37(uint8_t *upSendTXBUF ,unsigned int Vsnum)
{
   	uint16_t len = 0;
   	uint16_t sendlen=0;

    sendlen =IOT_Protocol_ToSever(upSendTXBUF, len ,Vsnum);    //Э�����
   	return sendlen;
}
/********************************************************************
    * FunctionName : IOT_ESPSend_0x38
    * Description  : esp ������������(0x38)
    * Parameters   : char *upSendTXBUF                     : ���ͻ�����
   	 	 	 	 	 unsigned int Vsnum 				  : Э��汾
    * Returns      : len Ҫ���͵����ݳ���
    * Notice       : none
*********************************************************************/
uint16_t IOT_ESPSend_0x38(uint8_t *upSendTXBUF ,unsigned int Vsnum)
{
   	uint16_t len = 0;
   	uint16_t sendlen=0;

    sendlen =IOT_Protocol_ToSever(upSendTXBUF, len ,Vsnum);    //Э�����
   	return sendlen;
}
#if 0
/******************************************************************************
 * FunctionName : IOT_LocalTCPCode0x26_Handle() add LI 20210115
 * Description  : ���ձ��� �������ݰ�
 * Parameters   : unsigned char *pucSrc_tem : �������ݰ�
 * Returns      : none
 * Notice       : 0x19:
 * 	 ����0001 0005 0014 01 26 30303030303030303030 [0006 00FF 0001 0001 2*crc] - �·��ܰ� 255/��һ���� /���� 0001
 * 	 Ӧ��0001 0005 0013 01 26 30303030303030303030 [00FF 0001 00 2*crc] - �·��ܰ� 255/��һ���� /״̬ 00
*******************************************************************************/
void IOT_LocalTCPCode0x26_Handle(unsigned char *pucSrc_tem)
{
/*
	unsigned char ucAckBuf[5] = {0};
	unsigned char ucAcklen = 0;
	unsigned char ucDataOffset = 6;         // ����ƫ�Ƶ�ַ��6-��ȥ���ݳ���2b / �ܰ���2b / ��ǰ����2b
	unsigned char i = 0;
	unsigned int  uiPackageLen = 0;         // ����������
	unsigned int  uiSumPackageNums = 0;     // ����������
	unsigned int  uiCurrentPackageNums = 0; // ��ǰ���������

	ucAckBuf[0] = pucSrc_tem[2];
	ucAckBuf[1] = pucSrc_tem[3]; 			// �ܰ�����
	ucAckBuf[2] = pucSrc_tem[4];
	ucAckBuf[3] = pucSrc_tem[5]; 			// ��ǰ����
	ucAckBuf[4] = 0x00;         			// Ӧ��״̬ -- �ɹ�
	ucAcklen = 7;                			// Ӧ�𳤶�

	uiPackageLen = IOT_ESP_htoi(&pucSrc_tem[0],2);
	uiSumPackageNums = IOT_ESP_htoi(&pucSrc_tem[2],2);
	uiCurrentPackageNums = IOT_ESP_htoi(&pucSrc_tem[4],2);

	IOT_ESP_Char_printf("IOT_LocalTCPCode0x26_Handle()_1", &pucSrc_tem[ucDataOffset], uiPackageLen-4);

	if((uiPackageLen > (1024 + 4)) || \
	   (uiSumPackageNums > 512))     // �����������ݰ����ȴ��� 1024�ֽ� / ���������ݰ���С> 512k
	{
		ucAckBuf[4] = 0x02;          // Ӧ��״̬ -- ����������ʧ��ʧ��
		goto ACK_FAIL_0X26;
	}
	if(uiCurrentPackageNums == 0x00) // ��һ������  ��ʼ��ȡ�ϵ����ݰ����  ��ʼ������׼��
	{
		if(IOT_LocalFota_Handle_uc(0, &pucSrc_tem[ucDataOffset], (uiPackageLen-4))) // д���һ������, ������ʼ��
		{
			ucAckBuf[4] = 0x01;      // Ӧ��״̬ -- ����������ʧ��ʧ��
			goto ACK_FAIL_0X26;
		}
		if((GuiIOTESPHTTPBreakPointSaveSize >= 8*1024) && (GuiIOTESPHTTPBreakPointSaveSize)) // �жϵ�����
		{
			ucAckBuf[2] = U16_HIGH((GuiIOTESPHTTPBreakPointSaveSize/1024));
			ucAckBuf[3] = U16_LOW((GuiIOTESPHTTPBreakPointSaveSize/1024));   // ��ǰ����
		}
	}
	else // ��ʼ���������ļ���
	{
		if(IOT_LocalFota_Handle_uc(1, &pucSrc_tem[ucDataOffset], (uiPackageLen-4))) // д������������
		{
			ucAckBuf[4] = 0x01;      // Ӧ��״̬ -- ����������ʧ��ʧ��
			goto ACK_FAIL_0X26;
		}

		if(uiSumPackageNums == uiCurrentPackageNums) // �����ļ����·���ɣ���ʼ��������
		{}
	}

//IOT_ESP_Char_printf("IOT_LocalTCPCode0x26_Handle()", &pucSrc_tem[0], uiPackageLen+4);
ACK_FAIL_0X26:

	IOT_LocalTCPDataAck_Package(0x26, ucAckBuf, ucAcklen);

	if(_g_ucLocalFotaStatus)
	{
		_g_ucLocalFotaStatus = 0;
		vTaskDelay(1000 / portTICK_RATE_MS); // ��ʱ 1S �ȴ����ݷ������
		IOT_ESP_Fota_Recycle(); // ����ʧ��/�ɹ��ͷ����������ռ䣬��ע�� �����ɹ������������ϵͳ��ִ���¹̼�
	}
*/
}
#endif
