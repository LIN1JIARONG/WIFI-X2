/*
 * iot_protocol.c
 *  Created on: 2021年12月9日
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

#define ServerProtocolMAXSIZE  1536   //buff 大小改为1536  20220521 chen
//#define ServerProtocolMAXSIZE  1000
uint8_t ProtocolTXBUF[ServerProtocolMAXSIZE]={0};

/********************************************
 * APP 使用通讯协议命令码（0x17、0x18、0x19、0x26）
 * 0x17 为透传命令，涉及modbus 03/04/06/10功能码
 * 协议版本是5.0 版本 ： SN长度为10位
 *
 * */

/********************************************
	发送服务器数据 协议异或
	6.0 版本协议
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
数服协议 6.0  协议加密  包头包尾
**************************************************************/
uint16_t IOT_Protocol_ToSever(unsigned char *p,unsigned int uLen,unsigned int Vsnum)
{
	uint16_t len = 0;
	uint16_t crc16 = 0;
	uint16_t _num = uLen; // 得到原始数据长度

	char SNbuf[30]={0};

	if(_num > ServerProtocolMAXSIZE)
	{
		_num = ServerProtocolMAXSIZE - 2;
	}
	/* (1)数服协议版本是6.0 */
//	if(!strlen((char*)&SysTAB_Parameter[Parameter_len[REG_SN]]))// 逆变器新序列号是空
//	{
//		memset((char *)&SysTAB_Parameter[Parameter_len[REG_SN]],0,sizeof(&SysTAB_Parameter[Parameter_len[REG_SN]]));
//		memcpy((char *)&SysTAB_Parameter[Parameter_len[REG_SN]],"inversnerr",10);// 得到逆变器序列号-err SN chg LI 20201112
//	}

	len = MERGE(p[4],p[5]);   	// 得到（10-sn）有效长度

	if(	Vsnum  ==  LOCAL_PROTOCOL)		//判断是否为本地通讯数据
	{
		if((Vsnum & 0x00ff) == ProtocolVS05)	//若版本v5 ，则直接复制
		{
			memcpy(ProtocolTXBUF, p, _num);// 得到加密前发送原始数据
		}else	//本地通信版本为 V6 以上的版本
		{
			len =  IOT_Repack_ProtocolData_V5ToV6_us(p , _num , ProtocolTXBUF);	//重新打包数据

			memcpy(p, ProtocolTXBUF, len);
			return len;
		}
	}
	else
	{
		if((p[7] == 0x03) || (p[7] == 0x04) || (p[7] == 0x14) || (p[7] == 0x1C) ||\
			(p[7] == 0x23) || (p[7] == 0x24) || (p[7] == 0x25) ||\
			(p[7] == 0x50) || (p[7] == 0x33) || (p[7] == 0x34) || (p[7] == 0x20)  || (p[7] == 0x22)) // 判断命令码带逆变器序列号
		{
				len += 20*2;											 			 // 增加后（30-sn）数据长度
				memcpy(ProtocolTXBUF, p, 8);                     		 			 // 通讯编号 + 协议编号 + 原数据长度 + 设备地址 + 命令码
				ProtocolTXBUF[4] = HIGH(len);
				ProtocolTXBUF[5] = LOW(len); 										 // 更改数据实际长度
				if(Collector_Config_LenList[REG_SN] < 30  )			 				 // 补齐 30 位长度的序列号
				{
					bzero(SNbuf, sizeof(SNbuf));
					memcpy(SNbuf,&SysTAB_Parameter[Parameter_len[REG_SN]],Collector_Config_LenList[REG_SN] );
					memcpy(&ProtocolTXBUF[8], SNbuf, 30 );  		   				 // 采集器序列号
				}
				else
				{
					memcpy(&ProtocolTXBUF[8], &SysTAB_Parameter[Parameter_len[REG_SN]], 30);  		// 采集器序列号
				}
				memcpy(&ProtocolTXBUF[38], InvCommCtrl.SN, 30);		    			// 逆变器序列号
				memcpy(&ProtocolTXBUF[68], &p[28], _num-28);           				 // 数据区数据
				_num += (20*2); // 发送数据个数
		}
		else // 其他命令码（没有逆变器序列号）//18  19
		{
		//	IOT_Printf("\r\n==============================================\r\n");
		//	IOT_Printf("\r\n==p[7]=0x%02x=======_num=%d===============\r\n",p[7],_num);
				len += 20*1;              								  // 增加后（30-sn）数据长度
				memcpy(ProtocolTXBUF, p, 8);                     		  // 通讯编号 + 协议编号 + 原数据长度 + 设备地址 + 命令码
				ProtocolTXBUF[4] = HIGH(len);
				ProtocolTXBUF[5] = LOW(len); // 更改数据实际长度
				if(Collector_Config_LenList[REG_SN] < 30  )   			  // 补齐 30 位长度的序列号
				{
					bzero(SNbuf, sizeof(SNbuf));
					memcpy(SNbuf,&SysTAB_Parameter[Parameter_len[REG_SN]],Collector_Config_LenList[REG_SN] );
					memcpy(&ProtocolTXBUF[8], SNbuf, 30 );  		// 采集器序列号
				}
				else
				{
					memcpy(&ProtocolTXBUF[8], &SysTAB_Parameter[Parameter_len[REG_SN]], 30);  		// 采集器序列号
				}
				 memcpy(&ProtocolTXBUF[38], &p[18], _num-18);     		// 数据区数据
				 _num += (20*1); // 发送数据个数
		}
	}

	if((Vsnum & 0x00ff) == ProtocolVS05)//协议兼容 5.0
	{
		ProtocolTXBUF[3] =0x05;
	}
	else if((Vsnum & 0x00ff)== ProtocolVS07)//协议兼容 7.0
	{
		ProtocolTXBUF[3]=0x07;
	}
	else//协议兼容 6.0
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

	encrypt(&ProtocolTXBUF[8], (_num - 8));		//异或加密
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
  *     用于网络服务器对数据采集器的相关参数进行查询
  *     响应服务器指令 0x19
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
	uint32_t add_s = 0;           //起始参数编号
	uint32_t add_e = 0;           //终止参数编号
	uint8_t SNLen = 0;
	char SN_Str[30] = {0};
	uint8_t ParamOffset = 0;
	uint8_t Redate[50] = {0};

	if(( PROTOCOL_VERSION_L == 6) || (PROTOCOL_VERSION_L == 7))// LI 2018.05.11
	{
		ParamOffset = 20; // 在原来基础上加了20个采集器序列号字符
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
	add_s = ((unsigned int)pBuf[18+ParamOffset]<<8) + pBuf[19+ParamOffset];           //起始参数编号
	add_e = ((unsigned int)pBuf[20+ParamOffset]<<8) + pBuf[21+ParamOffset];           //终止参数编号

			for(i=0; i<suiLen_tem; i++)
			{
				IOT_Printf("%02x " ,pBuf[i]);
			}
			IOT_Printf(" is pBuf 1\r\n");

#if DEBUG_TEST
			IOT_Printf("起始地址 = %d\r\n" ,add_s);
			IOT_Printf("结束地址 = %d\r\n" ,add_e);
#endif
    for(i=0; i<SNLen; i++)                             //拷贝SN号到数据缓存
    {
        buf[8+i] =p[i];
    }

    if((add_e >= add_s) && (add_e <= 200))            //判断要查询的参数标号是否正确
    {
		for(num=add_s; num <=add_e; num++)          // if(str == set_tab_str[REG_SN] )
		{
		/* 准备数据 */
			if(num == 13)			//特殊处理下13参数，放到最后发
			{
				flag_Send_0x0d = 1;
				IOT_Printf("(num == 13)\r\n");
				continue;
			}
			if(num < 70) // 参数列表1
			{
				IOT_Printf("\r\n uart test get 0x19 date = ",num);
				IOT_Printf("num= %d  len=%d \r\n" ,num,Collector_Config_LenList[num]);
				memcpy(&buf[22+ParamOffset], &SysTAB_Parameter[Parameter_len[num]], Collector_Config_LenList[num]);
				memcpy(Redate, &SysTAB_Parameter[Parameter_len[num]], Collector_Config_LenList[num]);
				if(num ==2 )  //协议版本 与发送的同步 兼容测试软件！！！
				{
					if(buf[22+ParamOffset]== 0x37 )
					{
						buf[22+ParamOffset]= pBuf[3]+0x30;
					}
				}
//				else if( num == 18 )   // 兼容测试软件！！！  端口 5279
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
		else // 参数列表2
		{
		}
		/* 查询时间 */
		if(num == 31)
		{
			buf[22+ParamOffset] = '2';								//2
			buf[23+ParamOffset] = '0';								//0
			buf[24+ParamOffset] = (	sTime.data.Year/10) + 0x30;		//年
			buf[25+ParamOffset] = (	sTime.data.Year%10) + 0x30;
			buf[26+ParamOffset] = '-';
			buf[27+ParamOffset] = (sTime.data.Month/10) + 0x30;		//月
			buf[28+ParamOffset] = (sTime.data.Month%10) + 0x30;
			buf[29+ParamOffset] = '-';
			buf[30+ParamOffset] = (sTime.data.Date/10) + 0x30;		//日
			buf[31+ParamOffset] = (sTime.data.Date%10) + 0x30;
			buf[32+ParamOffset] = ' ';
			buf[33+ParamOffset] = (sTime.data.Hours/10) + 0x30;		//时
			buf[34+ParamOffset] = (sTime.data.Hours%10) + 0x30;
			buf[35+ParamOffset] = ':';
			buf[36+ParamOffset] = (sTime.data.Minutes/10) + 0x30;	//分
			buf[37+ParamOffset] = (sTime.data.Minutes%10) + 0x30;
			buf[38+ParamOffset] = ':';
			buf[40+ParamOffset] = (sTime.data.Seconds%10) + 0x30;
		}
		if(num < 70) // 参数列表1
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
			/* 准备数据 */
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
			IOT_Printf("\r\n响应0x19指令,查询%d寄存器 = ",0x0d);
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
  *     响应0x18指令
  *		Status:	00 	响应成功指令
  *				01	无响应
  *				02	响应失败指令
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
  uint8_t ParamOffset = 0; // 参数偏移值 LI 2018.05.11

	if((PROTOCOL_VERSION_L == 6) || (PROTOCOL_VERSION_L == 7))// LI 2018.05.11
	{
		ParamOffset = 20; // 在原来基础上加了20个采集器序列号字符
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

	regNum = pBuf[18+ParamOffset];			//设置项的编号
	regNum = (regNum << 8) | pBuf[19+ParamOffset];

	len = pBuf[20+ParamOffset];				//设置长度
	len = (len << 8) | pBuf[21+ParamOffset];


	/* 清除数据记录 */
	if((regNum == 33) && (pBuf[22+ParamOffset] == '1'))
	{
		//ClearAllRecord();					//清除记录
	}

	/* 恢复出厂设置 */
	if((regNum == 35) && (pBuf[22+ParamOffset] == '1'))
	{
		CommTxBuf[0] = 0x00;
		CommTxBuf[1] = 0x01;
		CommTxBuf[2] = 0x00;
		CommTxBuf[3] = pBuf[3];
		CommTxBuf[4] = 0x00;
		CommTxBuf[5] = 0x0f+ParamOffset;	 //数据长度

		CommTxBuf[6]  = 0x01;				 //设备地址
		CommTxBuf[7]  = 0x18;

		for(i=0; i<SNLen; i++)			    //采集器SN
		{
			CommTxBuf[8+i] =p[i];
		}

		CommTxBuf[18+ParamOffset] = pBuf[18+ParamOffset];			//设置的编号
		CommTxBuf[19+ParamOffset] = pBuf[19+ParamOffset];
		CommTxBuf[20+ParamOffset] = 0x00;							//成功

		crc16 = Modbus_Caluation_CRC16(CommTxBuf, 21+ParamOffset);
		CommTxBuf[21+ParamOffset] = HIGH(crc16);
		CommTxBuf[22+ParamOffset] = LOW(crc16);
		IOT_UART1SendData(CommTxBuf,23+ParamOffset);

		//IOT_FactoryReset();						//恢复寄存器为出厂设定+删除本地数据
		IOT_KEYlongReset();
 	    //ClearAllRecord();
//		Change_Config_Value("0",35,1);			//清除更新参数并保存
		IOT_Printf("CommSetDatalog  IOT_FactoryReset  \r\n");
		System_state.SYS_RestartFlag=1;
		IOT_SYSRESTTime_uc(ESP_WRITE ,1);

		return;
	}
	if(regNum == 17 || regNum == 18 || regNum == 19)
	{
		//IOT_ServerParamSet_Check(3);

	}

// 存在CRC 校验 最后两位数据bUG
//	if((regNum == 8)  )
//	{
//		len = IOT_NewInverterSN_Check(&pBuf[22+ParamOffset]);  //更新 SN 有效长度
//	}

	IOT_SystemParameterSet(regNum ,(char *) &pBuf[22+ParamOffset],len );  //保存数据

	if(regNum == 17)
		IOT_SystemParameterSet(19 ,(char *) &pBuf[22+ParamOffset],len );  //保存数据
 		//URL和IP都用来保存URL。
	else if(regNum == 19)
		IOT_SystemParameterSet(17 ,(char *) &pBuf[22+ParamOffset],len );  //保存数据
	else if(regNum == 18)
	{
		IOT_SystemParameterSet(18 ,(char *)"7006",4 );  //保存数据
	}
	//URL和IP都用来保存URL。
	if(regNum == 17 || regNum == 18 || regNum == 19)
	{
		//IOT_ServerParamSet_Check(1);
	}
	/* 更新时间 */
	if(regNum == 31)
	{
		Clock_Time_Init[0] = (pBuf[24+ParamOffset]-0x30)*10+(pBuf[25+ParamOffset]-0x30);	//年
		Clock_Time_Init[1] = (pBuf[27+ParamOffset]-0x30)*10+(pBuf[28+ParamOffset]-0x30);	//月
		Clock_Time_Init[2] = (pBuf[30+ParamOffset]-0x30)*10+(pBuf[31+ParamOffset]-0x30);	//日
		Clock_Time_Init[3] = (pBuf[33+ParamOffset]-0x30)*10+(pBuf[34+ParamOffset]-0x30);	//时
		Clock_Time_Init[4] = (pBuf[36+ParamOffset]-0x30)*10+(pBuf[37+ParamOffset]-0x30);	//分
		Clock_Time_Init[5] = (pBuf[39+ParamOffset]-0x30)*10+(pBuf[40+ParamOffset]-0x30);	//秒

		IOT_Printf("\r\n*****************set: %02d:%02d:%02d-%02d:%02d:%02d*****************\r\n" ,Clock_Time_Init[0],Clock_Time_Init[1],Clock_Time_Init[2],Clock_Time_Init[3],Clock_Time_Init[4],Clock_Time_Init[5]);

		IOT_RTCSetTime(Clock_Time_Init);
	}
	CommTxBuf[0] = 0x00;
    CommTxBuf[1] = 0x01;
    CommTxBuf[2] = 0X00;
    CommTxBuf[3] = pBuf[3];
    CommTxBuf[4] = 0x00;
    CommTxBuf[5] = 0x0f+ParamOffset;							//数据长度
    CommTxBuf[6]  = 0X01;		//设备地址
    CommTxBuf[7]  = 0x18;

    for(i=0; i<SNLen; i++)					//采集器SN
    {
        CommTxBuf[8+i] =p[i];
    }
	CommTxBuf[18+ParamOffset] = pBuf[18+ParamOffset];		//设置的编号
	CommTxBuf[19+ParamOffset] = pBuf[19+ParamOffset];		//
	CommTxBuf[20+ParamOffset] = 0x00;		//成功


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

	/* 重启指令 */
	if((regNum == 32) && (pBuf[22+ParamOffset] == '1'))
	{
		System_state.SYS_RestartFlag=1;
		 IOT_SYSRESTTime_uc(ESP_WRITE ,1);
	}
}

/********************************
 * 数服协议 0x03  数据上报
 * 服务器应答
 *
 * *******************************/
void IOT_ServerFunc_0x03(uint8_t *p)
{
//	CommStat.NoReply = 0;
	if(p[8]  ==0x00)        //接收成功
	{
		//服务器应答 OK
		//没有收到服务器应答计数
		//0x03发送完2s后发送一条04；
//		System_Config[REG_Server_60][0] = STATE_0X03;			//记录通讯状态
// 		System_Config[REG_Server_60][1] = '\0';
//		Collector_Config_LenList[REG_Server_60] = 1;
	}
	else if(p[8]  ==0x01)  	//Server解析数据出错，重新扫描逆变器
	{
		//服务器应答 ERROR
		//等待10S后重新上传,正常情况下，一次完整的逆变器数据获取需要8s
	}
	else if(p[8]  ==0x02)   //网络服务器查询03命令，采集器接收后上报对应地址的设备信息，设备地址为0时上报所有已监控上设备的03命令。
	{
	//	sun_pv.send_set_ok_tcp=0;     //server 触发采集器重新上传03
		InvCommCtrl.HoldingSentFlag = 1;
	}
	IOT_Printf("\r\n************IOT_ServerFunc_0x03***************\r\n");
}
/*********************************
 * 数服协议 0x04 数据上报
 * 服务器应答
 * *******************************/
void IOT_ServerFunc_0x04(uint8_t *p)
{
//	CommStat.NoReply = 0;
	if(p[8] == 0x00)          					 //接收成功
	{
		System_state.ServerACK_flag=1;			 //send state OK
//		System_state.ServerACK_DeathTime = 0;    //没有收到服务器应答计数，
//		System_Config[REG_Server_60][0] = STATE_0X04;			//记录通讯状态
//		System_Config[REG_Server_60][1] = '\0';
//		Collector_Config_LenList[REG_Server_60] = 1;
		//IOT_TCPSendCode_Listen_uc(0,ETCPCODE_G04);                   // growatt平台04命令码应答监听计时清空
		//IOT_TCPRunLog_Record_uc(1,ETCPRUNLOGP_G04,(char*)p);// 记录growatt 04命令发送成功log 国网监控优化
	}
	else if(p[8]  ==0x01)          //数据异常
	{
	 	//触发重新扫描PV后重发
		//等待12S后重新上传04
	}
	else if(p[8]  ==0x02)          //Server触发查询 0x04
	{
		//sun_pv.send_state_ok_tcp = 0;    		//触发重发
	}
	IOT_Printf("\r\n************IOT_SeverFunc_0x04***************\r\n");
 }
/********************************
 * 数服协议 0x05
 * 服务器下发 读取单个或多个03逆变器数据
 * 弃用 IOT_ServerReceiveHandle 里面直接使用 不再封装
 * *******************************/
void IOT_ServerFunc_0x05(uint8_t *p)
{
	uint8_t ParamOffset = 0; // 参数偏移量
	ParamOffset = 20;		// 在原来基础上加了20个采集器序列号字符
	System_state.Host_setflag |= CMD_0X05;
	// InvCommCtrl.RespondFlagGrop |= CMD_0X05;
	InvCommCtrl.SetParamStart = MERGE(p[18 + ParamOffset], p[19 + ParamOffset]);
	InvCommCtrl.SetParamEnd = MERGE(p[20 + ParamOffset], p[21 + ParamOffset]);
	IOT_Printf("\r\n************IOT_sver_code_0x05***************\r\n");

}
/********************************
 * 数服协议 0x06
 * 服务器下发 对逆变器进行单个参数的设置
 * 弃用 IOT_ServerReceiveHandle 里面直接使用 不再封装
 * *******************************/
void IOT_ServerFunc_0x06(uint8_t *p)
{
	uint8_t ParamOffset = 0;
	ParamOffset = 20;	// 在原来基础上加了20个采集器序列号字符
	//本次数据长度,这个长度是没有添加CRC校验的2byte
	System_state.Host_setflag |= CMD_0X06;
	InvCommCtrl.Set_InvParameterNum = MERGE(p[18 + ParamOffset], p[19 + ParamOffset]);
	InvCommCtrl.Set_InvParameterData = MERGE(p[20 + ParamOffset], p[21 + ParamOffset]);
	IOT_Printf("\r\n************IOT_ServerFunc_0x06***************\r\n");

}


/********************************
 * 数服协议 0x10
 * 服务器应答 对逆变器进行多个参数的设置
 * 弃用 IOT_ServerReceiveHandle 里面直接使用 不再封装
 * *******************************/
void IOT_ServerFunc_0x10(uint8_t *p)
{
	uint8_t ParamOffset = 0; // 参数偏移量 LI 2018.05.11
	ParamOffset = 20;// 在原来基础上加了20个采集器序列号字符

	System_state.Host_setflag|= CMD_0X10;
 	InvCommCtrl.CMDFlagGrop  |= CMD_0X10;
	InvCommCtrl.SetParamStart = MERGE(p[18 + ParamOffset], p[19 + ParamOffset]);
	InvCommCtrl.SetParamEnd = MERGE(p[20 + ParamOffset], p[21 + ParamOffset]);
	//1500V 多机指令兼容
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
 * 数服协议 0x16
 * 服务器应答 心跳命令码
 * 用于MQTT 心跳Ping
 * *******************************/
extern uint8_t uSetserverflag;
void IOT_ServerFunc_0x16( void )
{
	if(System_state.Server_Online == 0)  //   连网后第一包心跳包进行备份和保存处理
	{
		IOT_SystemParameterSet(60,"4", strlen("4"));  //保存数据
		System_state.UploadParam_Flag=1;

		if(0 == System_state.Server_Online)		//断网后第一次收到心跳包,记录一下当前连接成功的服务器ip地址和端口
		{
			if(1 == uSetserverflag)		//若设置连接新服务器成功,保存新服务器
			{
				IOT_Printf("#**  record new Server IP/PORT **#\r\n");
				uSetserverflag = 0;
				IOT_SystemParameterSet(17,_ucServerIPBuf,strlen(_ucServerIPBuf));
				IOT_SystemParameterSet(18,_ucServerPortBuf,strlen(_ucServerPortBuf));
				IOT_SystemParameterSet(19,_ucServerIPBuf,strlen(_ucServerIPBuf));
			}


			bzero(_ucServerIPBuf, sizeof(_ucServerIPBuf));	  // 服务器IP地址备份区
			memcpy(_ucServerIPBuf , &SysTAB_Parameter[Parameter_len[17]] , Collector_Config_LenList[17] );
			bzero(_ucServerPortBuf, sizeof(_ucServerPortBuf));	// 服务器IP地址备份区
			memcpy(_ucServerPortBuf , &SysTAB_Parameter[Parameter_len[18]] , Collector_Config_LenList[18]);

		}

		IOT_TCPDelaytimeSet_uc(ESP_WRITE,5);// 秒倒计时 触发上报03/04
		IOT_Historydate_us(ESP_WRITE,30);   // 历史数据 开始触发上报

	}

	IOT_Heartbeat_Timeout_Check_uc(ESP_WRITE, HEARTBEAT_TIMEOUT);		//重置心跳包超时时间
	System_state.Server_Online = 1;
}

/********************************
 * 数服协议 0x18
 * 服务器 设置 采集器参数的指令 一次只可对单个参数进行设置
 *
 * *******************************/
//const char BLEkey[8]={"Growatt0"};  //BLEkeytoken
void IOT_ServerFunc_0x18(uint8_t REGNum ,uint8_t REGLen,  uint8_t *pDATA)
{
	uint16_t len=0,regNum=0;
//	uint8_t ParamOffset = 0; //参数偏移值
	char TABBLEkeys[40]={0};
	char setKeys[40]={0};
	bzero(TABBLEkeys, sizeof(TABBLEkeys));
	bzero(setKeys, sizeof(setKeys));

//	ParamOffset = 20; 		 //在原来基础上加了20个采集器序列号字符
//	regNum = p[18 + ParamOffset];
//	regNum = (regNum << 8) | p[19 + ParamOffset];	//设置项的编号
//	len = p[20 + ParamOffset]-0x30;
//	len = (len << 8) |( p[21 + ParamOffset]-0x30);	//设置长度
	regNum =REGNum;
	len=REGLen;
	IOT_Printf("IOT_ServerFunc_0x18 regNum=  %02d, len= %d, SetData=%s \r\n",regNum,len,pDATA);

//	System_state.Host_setflag |= CMD_0X18;
//	InvCommCtrl.CMDFlagGrop |= CMD_0X18;
//	System_state.ServerSET_REGAddrStart = ((unsigned int)p[18 + ParamOffset]<<8) + p[19 + ParamOffset]; //起始参数编号
//	System_state.ServerSET_REGAddrLen =  ((unsigned int)p[20 + ParamOffset]<<8) + p[21 + ParamOffset];

 	System_state.ParamSET_ACKCode=0x00;//成功
	if((regNum == 17)||(regNum == 18)||(regNum == 19)||(regNum == 10)
			||(regNum == 32)||(regNum == 35) || (regNum == 80))  // 不需要保存数据	//80号寄存器也不需要保存 20220524
	{
		// 执行动作

	}
	else
	{	// 保存参数
		if((System_state.ble_onoff == 1) &&(System_state.BLEKeysflag==0) && (regNum==54) )  //第一次连不保存密钥参数
		{
			memcpy(setKeys,pDATA,len);
			memcpy(TABBLEkeys,&SysTAB_Parameter[Parameter_len[REG_BLETOKEN]],Collector_Config_LenList[REG_BLETOKEN]);

			if((strncmp(BLEkeytoken,setKeys,32)==0 ) || (strncmp(TABBLEkeys,setKeys,32)==0)) // 第一次连接wifi 输入密钥比较
			{
				System_state.BLEKeysflag=1;  		//密钥置位
			}
			else
			{
				System_state.ParamSET_ACKCode = 0x01; //失败
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
				System_state.ble_onoff = 2;  //重启
			}
			IOT_SystemParameterSet(regNum ,(char *) &pDATA[0],len );  //保存数据
		}

//		{
//			InvCommCtrl.CMDFlagGrop &= ~CMD_0X18;
//		 	IOT_GattsEvenSetR();    				// 触发应答APP 18命令
//		}

	}
	switch(regNum)
	{
		case 4:  // 上报时间
			IOT_UPDATETime();
			break;
		case 10: // 更新固件
			break;
		case 17: // 服务器 ip
 	    	bzero(_ucServerIPBuf, sizeof(_ucServerIPBuf));	  // 服务器IP地址备份区
			memcpy(_ucServerIPBuf,(char *)pDATA,(uint8_t)len );
			IOT_SETConnect_uc(0);
			break;
		case 18: //服务器 端口
 	    	bzero(_ucServerPortBuf, sizeof(_ucServerPortBuf));	// 服务器IP地址备份区
			memcpy(_ucServerPortBuf,(char *)pDATA,(uint8_t)len );
			IOT_SETConnect_uc(0);
			break;
		case 19: // 服务器域名
 	    	bzero(_ucServerIPBuf, sizeof(_ucServerIPBuf));	// 服务器IP地址备份区
			memcpy(_ucServerIPBuf,(char *)pDATA,(uint8_t)len );
			IOT_SETConnect_uc(0);
			break;
		case 30: // 时区
			break;
		case 31: // 时间
			// 2022-06-02 17:14:38
			Clock_Time_Init[0] = (pDATA[2]-0x30)*10+(pDATA[3]-0x30);	//15年
			Clock_Time_Init[1] = (pDATA[5]-0x30)*10+(pDATA[6]-0x30);	//月
			Clock_Time_Init[2] = (pDATA[8]-0x30)*10+(pDATA[9]-0x30);	//日
			Clock_Time_Init[3] = (pDATA[11]-0x30)*10+(pDATA[12]-0x30);	//时
			Clock_Time_Init[4] = (pDATA[14]-0x30)*10+(pDATA[15]-0x30);	//分
			Clock_Time_Init[5] = (pDATA[17]-0x30)*10+(pDATA[18]-0x30);	//秒

			System_state.ParamSET_ACKCode= IOT_RTCSetTime(Clock_Time_Init);

			if(0 == System_state.ParamSET_ACKCode)		//20220720 chen  触发同步逆变器时间
			{
				InvCommCtrl.ContrastInvTimeFlag = 1;
			}

		//	if(System_state.ParamSET_ACKCode==0 )
		//	  InvCommCtrl.UpdateInvTimeFlag =1;
		// 	IOT_SetInvTime0x10(45,50,Clock_Time_Init);   				//同步 逆变器时间 0x10 命令码

			break;
		case 32: // 重启采集器
			if(pDATA[0] == '1')
			{
				if( uSetserverflag != 0)  // 设置服务器 中 不触发重启
				{
					break;
				}
				System_state.SYS_RestartFlag=1;
				IOT_SYSRESTTime_uc(ESP_WRITE ,5);
			}
			break;
		case 33: // 清除记录
			if(pDATA[0]== '1')
			{
			    //ClearAllRecord();							//清除记录
				iot_Reset_Record_Task();
			}
			break;
		case 35: // 恢复出厂设置
			/* 恢复出厂设置 */
			if(pDATA[0] == '1')
			{
				IOT_FactoryReset();						//恢复寄存器为出厂设定+删除本地数据
#if	test_wifi
				IOT_ESP_Record_ResetSign_Write();							//写入清空 记录区标志
#endif
				System_state.SYS_RestartFlag=1;
				IOT_SYSRESTTime_uc(ESP_WRITE ,5);
			}
			break;
		case 39: // 设置升级的单次下载大小

			break;
		case 44: // 光伏设备固件升级模式

			break;
		case 56: // WIFI 模块要链接热点名称
			break;
		case 57: // WIFI 模块要链接的热点密钥

			IOT_WIFIOnlineFlag(255);
			IOT_WIFIEvenRECONNECTED();

			break;
		case 58: // WIFI模块相关配置信息

			break;
		case 70: // WIFI模式
			break;
		case 71: // DHCP使能
			break;
		case 75: // 获取周围WiFi的名称
				IOT_WIFIEvenSetSCAN();
			break;
		case 77: // 是否连回Server-cn服务器
			break;
		case 78: // 服务器地址
			break;
		case 79: // 服务器端口号
			break;
		case 80: // HTTP 下载文件的URL
		  //命令格式：1#type01#http://cdn.growatt.com/update/device/datalog/ShineGPRS-X_35/1.0.7.4/ShineGPRS_APP_1_3_0_8_G.bin
			if(strstr((char *)&pDATA[0],"#type"))
			{
				IOT_SystemParameterSet(regNum, (char *)&pDATA[9] ,len - 9);		//保存URL
				IOT_UpdateProgress_record(HTTP_PROGRESS , 0);		//升级进度清0

				//确定升级类型
				if((pDATA[0] == '0') || (pDATA[0] == '1'))
				{
					g_FotaParams.ucIsbsdiffFlag = false;	//全量升级
				}
				else if((pDATA[0] == '2') || (pDATA[0] == '3'))
				{
					g_FotaParams.ucIsbsdiffFlag = true;		//差分升级
				}

				if(pDATA[6] == '0' && pDATA[7] == '1' ) //升级类型： 01  采集器
				{

					IOT_SystemParameterSet(10, "1" , 1);
					IOT_SystemParameterSet(36, "1" , 1);	//写入停止轮询标志

					IOT_FotaType_Manage_c(1, EFOTA_TYPR_DATALOGER);
					IOT_Fota_Init();

					IOT_Printf("\r\n 执行升级采集器 \r\n");
					//更改10寄存器参数对应的长度
					//更改10寄存器的值,逆变器升级模式
					//更改36寄存器，停止轮训逆变器数据
				//	Collector_Fota();//采集器升级启动
				}
				else if(pDATA[6] == '0' && (pDATA[7] == '2' ||
						pDATA[7]== '3' ||pDATA[7]== '4'))  //升级类型：02逆变器bin /03逆变器hex /04逆变器mot /05逆变器out
				{
					IOT_Printf("\r\n 执行升级逆变器 \r\n");
					IOT_SystemParameterSet(44, (char *)&pDATA[7] , 1);
					IOT_SystemParameterSet(36, "1" , 1);	//写入停止轮询标志
					IOT_FotaType_Manage_c(1, EFOTA_TYPR_DEVICE );
					IOT_Fota_Init();
					//更改44寄存器参数对应的长度
					//更改44寄存器的值,逆变器升级模式
					//更改36寄存器，停止轮训逆变器数据
				//	Inverter_Fota();//逆变器升级启动
				}

				//保存下载路径
			 //if(WIFI_RxBuf[30] == '#' && WIFI_RxBuf[31] == 'h' && WIFI_RxBuf[32] == 't'&& WIFI_RxBuf[33] == 't'&& WIFI_RxBuf[34] == 'p')
//				{
//					if(Change_Config_Value((char *)&WIFI_RxBuf[31+ParamOffset],(u8)regNum,(u8)(len-9)) == 0)		//更新参数并保存
//					{
//						//CommStat.RespondCode = 0x00;									//成功
//					}
//					else
//					{
//						//CommStat.RespondCode = 0x01;									//失败
//					}
//
//					ESP_printf("\r\n 保存下载路径 \r\n");
//				}
			}
			break;
	    default:
	    	break;
	}

}

/********************************
 * 数服协议 0x19
 * 服务器 查询 采集器参数的指令
 * *******************************/
void IOT_ServerFunc0x19(uint8_t *p)
{
//	uint8_t ParamOffset = 0; // 参数偏移量 LI 2018.05.11

//	ParamOffset = 20;// 在原来基础上加了20个采集器序列号字符
//	System_state.ServerGET_REGAddrStart = ((unsigned int)p[18 + ParamOffset]<<8) + p[19 + ParamOffset];         //起始参数编号
//	System_state.ServerGET_REGAddrEnd = ((unsigned int)p[20 + ParamOffset]<<8) + p[21 + ParamOffset];           //终止参数编号

	if((System_state.ServerGET_REGAddrStart == 31) && (System_state.ServerGET_REGAddrEnd == 31))
	{
//  	查询时间
//		UpLoadTime = 1;
//		CommStat.ServerGetParamFlag = TRUE;
	}
	else
	{
		if((System_state.ServerGET_REGAddrStart == 0x04) && (System_state.ServerGET_REGAddrEnd == 0x15))
		{
//			CommStat.UploadParamSelfAble = DISABLE;
//			CommStat.UploadPresetParamAble = ENABLE;		//使用预置的参数区发送
		}
		else
		{
//			CommStat.ServerGetParamFlag = TRUE;
		}
	}
//	TimeSendInfoToServer = 0;			//触发上传
}
/**********************************
 * IOT_ServerReceiveHandle
 * 服务器publish 接收处理
 * return  _g_EMQTTREQType
 **********************************/
EMQTTREQ IOT_ServerReceiveHandle(uint8_t* pucBuf_tem, uint16_t suiLen_tem)
{
	uint8_t ParamOffset = 0; // SN参数偏移量
	uint8_t DataOffset =0;
	uint8_t *p=NULL;
	uint16_t tempCID = 0;
	uint16_t tempLen = 0;
	EMQTTREQ EMQTTREQTypeVal = MQTTREQ_NULL;

	int i = 0 ,j = 0;
	ParamOffset = 20;// 在原来基础上加了20个采集器序列号字符

	p =pucBuf_tem ;
	/* 通讯编号校验 */
	tempCID = p[0];
	tempCID = (tempCID<<8) | p[1];
	tempLen = p[4];
	tempLen = (tempLen << 8) | p[5];
	tempLen += 6;		//本次数据长度,这个长度是没有添加CRC校验的2byte

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
	encrypt(&p[8], (tempLen - 8));			//解密,-包头8byte

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
			if((System_state.BLEKeysflag==0 ) &&(p[7] != 0x18) )  //没有发送密钥
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
	if(p[7] == 0x19) 	  					// 服务器 查询 采集器参数的指令
	{
		EMQTTREQTypeVal=MQTTREQ_CMD_0x19;
		System_state.Host_setflag |= CMD_0X19;
		InvCommCtrl.CMDFlagGrop |= CMD_0X19;
		System_state.ServerGET_REGNUM = ((unsigned int)p[18 + ParamOffset]<<8) + p[19 + ParamOffset];    		  // 寄存器编号个数
		IOT_Printf("\r\n******************System_state.ServerGET_REGNUM =%d***************\r\n ",System_state.ServerGET_REGNUM );

		if(	System_state.ServerGET_REGNUM ==1)
		{
			System_state.ServerGET_REGAddrStart = ((unsigned int)p[20 + ParamOffset]<<8) + p[21 + ParamOffset];
			System_state.ServerGET_REGAddrEnd = ((unsigned int)p[20 + ParamOffset]<<8) + p[21+ ParamOffset];
		}
		else
		{
			 for(i=0 , j=0 ;i < System_state.ServerGET_REGNUM  ;i++,j++)
			//System_state.ServerGET_REGAddrStart = ((unsigned int)p[18 + ParamOffset]<<8) + p[19 + ParamOffset];     // 起始参数编号
			 {
				System_state.ServerGET_REGAddrStart = ((unsigned int)p[20+i+j + ParamOffset]<<8) + p[21+i+j + ParamOffset];       //终止参数编号
				System_state.ServerGET_REGAddrBUFF[i]=System_state.ServerGET_REGAddrStart ;
//				if(	System_state.ServerGET_REGNUM ==1)
//					System_state.ServerGET_REGAddrEnd = ((unsigned int)p[20+i + ParamOffset]<<8) + p[21+i + ParamOffset];         //终止参数编号
//				else
//					System_state.ServerGET_REGAddrEnd = ((unsigned int)p[22+i + ParamOffset]<<8) + p[23+i + ParamOffset];         //终止参数编号
			 }
		}
		IOT_Printf("\r\n******************System_state.ServerGET_REGAddrStart=%d***************\r\n ",System_state.ServerGET_REGAddrStart);
		IOT_Printf("\r\n******************System_state.ServerGET_REGAddrEnd =%d***************\r\n ",System_state.ServerGET_REGAddrEnd );

 		if(System_state.ServerGET_REGAddrStart == 75)
		{
		// if()//当前扫描未完成或者连接中，不触发扫描
 		   if( (p[2]==0) && (p[3]==5))//校验协议版本号  APP版本协议 ，10位序列号
		   IOT_WIFIEvenSetSCAN();
		}
//		else
//		{
//			IOT_GattsEvenSetR();  //要优化至sys
//		}
	}
	else if(p[7] == 0x18) // 服务器 设置 采集器参数的指令,important
	{
		EMQTTREQTypeVal = MQTTREQ_CMD_0x18;
		System_state.Host_setflag |= CMD_0X18;
	 	InvCommCtrl.CMDFlagGrop |= CMD_0X18;
		System_state.ServerSET_REGNUM = ((p[18 + ParamOffset]<<8) + p[19 + ParamOffset] );
		System_state.ServerSET_REGAddrLen = 0;
		for(i=0; i<System_state.ServerSET_REGNUM ;i++)
		{
			System_state.ServerSET_REGAddrStart = ((unsigned int)p[22  + ParamOffset +DataOffset]<<8) + p[23+ ParamOffset+DataOffset]; //起始参数编号
			System_state.ServerSET_REGAddrLen =  ((unsigned int)p[24 + ParamOffset+DataOffset]<<8) + p[25 + ParamOffset+DataOffset];
			IOT_Printf("\r\n******************ServerSET_REGAddrStart=%d******ServerSET_REGAddrLen=%d*********\r\n ",System_state.ServerSET_REGAddrStart,System_state.ServerSET_REGAddrLen);
			if( (p[2]==0) && (p[3]==5))//校验协议版本号  APP版本协议 ，10位序列号
			{
				if((System_state.ble_onoff==1 )&&(System_state.BLEKeysflag==0 ) &&(System_state.ServerSET_REGAddrStart != 54 ) )  //没有发送密钥
				{
					IOT_Printf("\r\n**********System_state.BLEKeysflag==0****System_state.ServerSET_REGAddrStart!= 54***********\r\n ");
				System_state.Host_setflag  &= ~CMD_0X18;
				InvCommCtrl.CMDFlagGrop &= ~CMD_0X18;
				return EMQTTREQTypeVal;
				}
			}
			 if((p[2]==0) && (p[3]==5)) // 本地 设置直接保存
			 {
				  if(System_state.ServerSET_REGAddrLen>0)//APP高级设置     17 会清空 19    // 长度是 0//APP高级设置     19 会清空 17    // 长度是 0
				  {
					 if((System_state.ServerSET_REGAddrStart == 17 )||(System_state.ServerSET_REGAddrStart==19))
					 {
						 IOT_SystemParameterSet(17 ,(char *) &p[26 + ParamOffset+DataOffset],System_state.ServerSET_REGAddrLen );  //保存数据  同时保存
						 IOT_SystemParameterSet(19 ,(char *) &p[26 + ParamOffset+DataOffset],System_state.ServerSET_REGAddrLen );  //保存数据
					 }
					 else if(System_state.ServerSET_REGAddrStart==18)
					 {
						 IOT_SystemParameterSet(18 ,(char *) &p[26 + ParamOffset+DataOffset],System_state.ServerSET_REGAddrLen );  //保存数据
					 }
					 else
					 {
						 IOT_ServerFunc_0x18(System_state.ServerSET_REGAddrStart,System_state.ServerSET_REGAddrLen,&p[26 + ParamOffset+DataOffset]);
					 }
				  }
			 }
			 else   // 远程操作设置
			 {
					IOT_ServerFunc_0x18(System_state.ServerSET_REGAddrStart,System_state.ServerSET_REGAddrLen,&p[26 + ParamOffset+DataOffset]);
			 }
			DataOffset=System_state.ServerSET_REGAddrLen+4;

			if(80 == System_state.ServerSET_REGAddrStart)	//如果为80号寄存器,需要确认升级的方式
			{
				if( (p[2]==0) && (p[3]==5))
				{
					g_FotaParams.uiWayOfUpdate = ELOCAL_UPDATE;  //本地升级
				}
				else
				{
					g_FotaParams.uiWayOfUpdate = EHTTP_UPDATE;	//远程升级
				}
			}

		}//ACK?
	}
	else if(p[7] == 0x17) // 透传命令
	{
		EMQTTREQTypeVal=MQTTREQ_CMD_0x17;
		System_state.Host_setflag |= CMD_0X17;
		InvCommCtrl.CMDFlagGrop |= CMD_0X17;
		InvCommCtrl.SetParamDataLen=  ((unsigned int)p[18 + ParamOffset]<<8) + p[19 + ParamOffset]; // 透传区数据长度
	    memcpy( InvCommCtrl.SetParamData, &p[20 + ParamOffset], InvCommCtrl.SetParamDataLen );
	}
	else if(p[7] == 0x26) // 升级
	{
		IOT_stoppolling();
		IOT_LocalTCPCode0x26_Handle(&p[18 + ParamOffset]);	//本地升级 数据处理
		EMQTTREQTypeVal=MQTTREQ_CMD_0x26;
		System_state.Host_setflag |= CMD_0X26;
		InvCommCtrl.CMDFlagGrop |= CMD_0X26;
		//IOT_GattsEvenSetR();    				// 触发应答APP 18命令
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
		//1500V 多机指令兼容
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
		/* 判断获取波形  */
		if(InverterWave_Judge(InvCommCtrl.SetParamStart,InvCommCtrl.SetParamEnd))
		{
			IOT_Printf("\r\n\r\n\r\n\r\n*******InverterWave_Judge***************\r\n\r\n\r\n\r\n\r\n");
			IOT_PINGRESPSet_uc(ESP_WRITE,Mqtt_PINGTime);   // 重新装载心跳时间// 如果获取波形请求时，延时心跳包发送，直到波形请求结束。。。
			IOT_TCPDelaytimeSet_uc(ESP_WRITE,System_state.Up04dateTime+1);// 延时04数据上传
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

//	if( (p[2]==0) && ( (p[3]==6) ||  (p[3]==7)))//校验协议版本号     服务器的功能版本 有30位长度序列号
//	{
//		encrypt(&p[8], (tempLen - 8));			//解密,-包头8byte
//		IOT_Printf("\r\n************IOT_ServerReceiveHandle  p[7]= 0x%02x, tempLen= %d************\r\n",p[7], tempLen);
//		for(i=0; i<tempLen; i++)
//		{
//			IOT_Printf("%02x ",p[i]);
//		}
//		if(p[7] == 0x19) 	 					//服务器 查询 采集器参数的指令
//		{
//				EMQTTREQTypeVal=MQTTREQ_CMD_0x19;
//				System_state.Host_setflag |= CMD_0X19;
//				 InvCommCtrl.CMDFlagGrop |= CMD_0X19;
//				System_state.ServerGET_REGNUM = ((unsigned int)p[18 + ParamOffset]<<8) + p[19 + ParamOffset];    		  // 寄存器编号个数
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
//					//System_state.ServerGET_REGAddrStart = ((unsigned int)p[18 + ParamOffset]<<8) + p[19 + ParamOffset];     // 起始参数编号
//					 {
//						System_state.ServerGET_REGAddrStart = ((unsigned int)p[20+i+j + ParamOffset]<<8) + p[21+i+j + ParamOffset];       //终止参数编号
//						System_state.ServerGET_REGAddrBUFF[i]=System_state.ServerGET_REGAddrStart ;
//		//				if(	System_state.ServerGET_REGNUM ==1)
//		//					System_state.ServerGET_REGAddrEnd = ((unsigned int)p[20+i + ParamOffset]<<8) + p[21+i + ParamOffset];         //终止参数编号
//		//				else
//		//					System_state.ServerGET_REGAddrEnd = ((unsigned int)p[22+i + ParamOffset]<<8) + p[23+i + ParamOffset];         //终止参数编号
//					 }
//				}
//			 	IOT_Printf("\r\n******************System_state.ServerGET_REGAddrStart=%d***************\r\n ",System_state.ServerGET_REGAddrStart);
//		 		IOT_Printf("\r\n******************System_state.ServerGET_REGAddrEnd =%d***************\r\n ",System_state.ServerGET_REGAddrEnd );
//		}
//		else if(p[7] == 0x18) 					//服务器 设置 采集器参数的指令
//		{
//			EMQTTREQTypeVal=MQTTREQ_CMD_0x18;
//			System_state.Host_setflag |= CMD_0X18;
//		 	InvCommCtrl.CMDFlagGrop |= CMD_0X18;
//			System_state.ServerSET_REGAddrStart = ((unsigned int)p[18 + ParamOffset]<<8) + p[19 + ParamOffset]; //起始参数编号
//			System_state.ServerSET_REGAddrLen =  ((unsigned int)p[20 + ParamOffset]<<8) + p[21 + ParamOffset];
//			//IOT_ServerFunc_0x18(p);
//			//ACK?
//		}
//		else if(p[7] == 0x05) // 服务器 查询 逆变器03寄存器数据
//		{
//			EMQTTREQTypeVal=MQTTREQ_CMD_0x05;
//			System_state.Host_setflag |= CMD_0X05;
//			InvCommCtrl.CMDFlagGrop |= CMD_0X05;
//			InvCommCtrl.SetParamStart = MERGE(p[18 + ParamOffset], p[19 + ParamOffset]);
//			InvCommCtrl.SetParamEnd = MERGE(p[20 + ParamOffset], p[21 + ParamOffset]);
//		 	//IOT_ServerFunc_0x05(p);
//		}
//		else if(p[7] == 0x06) // Server设定PV单个寄存器
//		{
//			EMQTTREQTypeVal=MQTTREQ_CMD_0x06;
//			System_state.Host_setflag |= CMD_0X06;
//			InvCommCtrl.CMDFlagGrop |= CMD_0X06;
//			InvCommCtrl.Set_InvParameterNum = MERGE(p[18 + ParamOffset], p[19 + ParamOffset]);
//			InvCommCtrl.Set_InvParameterData = MERGE(p[20 + ParamOffset], p[21 + ParamOffset]);
//			//IOT_ServerFunc_0x06(p);
//		}
//		else if(p[7] == 0x10) // Server设定PV多个寄存器
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
////以下指令暂时作废。  服务区修改应答方式   转移到   MQTT  ACK 应答方式
//		else if(p[7] == 0x16) //服务器应答心跳包
//		{
////			Heart_Cnt++;
////			IOT_Printf("接收到第%07ld个心跳包\r\n",Heart_Cnt);
////			IOT_Printf("Server_OnLine = 1 \r\n");
////			IOT_ServerFunc_0x16(p);
//		}
//		else if(p[7] == 0x03) //收到服务器0x03指令
//		{
//			IOT_ServerFunc_0x03(p);
//		}
//		else if((p[7] == 0x04) && (tempCID == System_state.bCID_0x04)) //收到服务器0x04指令，如果一定时间（5秒）内没有收到网络服务器的响应数据，应重发
//		{
//			IOT_ServerFunc_0x04(p);
//		}
//		else if(p[7] == 0x62) // 连回Server-cn服务器
//		{
//
//		}
//		else if(p[7] == 0x14) // 波形诊断
//		{
//
//		}
//		else if(p[7] == 0x20) // 电表数据
//		{
//
//		}
//	}
//	else if( (p[2]==0) && (p[3]==5))//校验协议版本号  APP版本协议 ，10位序列号
//	{
//		ParamOffset=0;
//		encrypt(&p[8], (tempLen - 8));			// 解密,-包头8byte
//
//		if((System_state.BLEKeysflag==0 ) &&(p[7] != 0x18) )  //没有发送密钥
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
//		if(p[7] == 0x19) 	  					// 服务器 查询 采集器参数的指令
//		{
//			EMQTTREQTypeVal=MQTTREQ_CMD_0x19;
//			System_state.Host_setflag |= CMD_0X19;
//			 InvCommCtrl.CMDFlagGrop |= CMD_0X19;
//			System_state.ServerGET_REGNUM = ((unsigned int)p[18 + ParamOffset]<<8) + p[19 + ParamOffset];    		  // 寄存器编号个数
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
//				//System_state.ServerGET_REGAddrStart = ((unsigned int)p[18 + ParamOffset]<<8) + p[19 + ParamOffset];     // 起始参数编号
//				 {
//					System_state.ServerGET_REGAddrStart = ((unsigned int)p[20+i+j + ParamOffset]<<8) + p[21+i+j + ParamOffset];       //终止参数编号
//					System_state.ServerGET_REGAddrBUFF[i]=System_state.ServerGET_REGAddrStart ;
//	//				if(	System_state.ServerGET_REGNUM ==1)
//	//					System_state.ServerGET_REGAddrEnd = ((unsigned int)p[20+i + ParamOffset]<<8) + p[21+i + ParamOffset];         //终止参数编号
//	//				else
//	//					System_state.ServerGET_REGAddrEnd = ((unsigned int)p[22+i + ParamOffset]<<8) + p[23+i + ParamOffset];         //终止参数编号
//				 }
//			}
//		 	IOT_Printf("\r\n******************System_state.ServerGET_REGAddrStart=%d***************\r\n ",System_state.ServerGET_REGAddrStart);
//	 		IOT_Printf("\r\n******************System_state.ServerGET_REGAddrEnd =%d***************\r\n ",System_state.ServerGET_REGAddrEnd );
//			if(System_state.ServerGET_REGAddrStart == 75)
//			{
//			// if()//当前扫描未完成或者连接中，不触发扫描
//			   IOT_WIFIEvenSetSCAN();
//			}
////			else
////			{
////				IOT_GattsEvenSetR();  //要优化至sys
////			}
//		}
//		else if(p[7] == 0x18) // 服务器 设置 采集器参数的指令
//		{
//			EMQTTREQTypeVal=MQTTREQ_CMD_0x18;
//			System_state.Host_setflag |= CMD_0X18;
//		 	InvCommCtrl.CMDFlagGrop |= CMD_0X18;
//			System_state.ServerSET_REGNUM = ((p[18 + ParamOffset]<<8) + p[19 + ParamOffset] );
//			System_state.ServerSET_REGAddrLen = 0;
//			for(i=0; i<System_state.ServerSET_REGNUM ;i++)
//			{
//				System_state.ServerSET_REGAddrStart = ((unsigned int)p[22  + ParamOffset +DataOffset]<<8) + p[23+ ParamOffset+DataOffset]; //起始参数编号
//				System_state.ServerSET_REGAddrLen =  ((unsigned int)p[24 + ParamOffset+DataOffset]<<8) + p[25 + ParamOffset+DataOffset];
//				IOT_Printf("\r\n******************ServerSET_REGAddrStart=%d******ServerSET_REGAddrLen=%d*********\r\n ",System_state.ServerSET_REGAddrStart,System_state.ServerSET_REGAddrLen);
//				if((System_state.BLEKeysflag==0 ) &&(System_state.ServerSET_REGAddrStart != 54 ) )  //没有发送密钥
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
//		else if(p[7] == 0x17) // 透传命令
//		{
//			EMQTTREQTypeVal=MQTTREQ_CMD_0x17;
//			System_state.Host_setflag |= CMD_0X17;
//			InvCommCtrl.CMDFlagGrop |= CMD_0X17;
//			InvCommCtrl.SetParamDataLen=  ((unsigned int)p[18 + ParamOffset]<<8) + p[19 + ParamOffset]; // 透传区数据长度
//		    memcpy( InvCommCtrl.SetParamData, &p[20 + ParamOffset], InvCommCtrl.SetParamDataLen );
//		}
//		else if(p[7] == 0x26) // 升级
//		{
//			EMQTTREQTypeVal=MQTTREQ_CMD_0x26;
//			System_state.Host_setflag |= CMD_0X26;
//		}
//	}

    /* 清空缓存 */
	p = NULL;
	return EMQTTREQTypeVal;
}
/****************
 * 按键触发 写便携式电源寄存器  03 数据
 * 56 号寄存器
 * 低位0-3 蓝牙 bit0  开1/关0   bit1  连接设备1/断开设备0
 * 高位4-7 wifi bit4  开1/关0   bit5  连接设备1/断开设备0
 *
 * 需要机器重启清零不记忆， 否则需要定时检查04数据 设置同步
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
 * HoldingGrop  保持寄存器 打包
 * 03 数据
 * *****************************************/
uint16_t HoldingGrop_DataPacket(uint8_t *outdata)
{
	uint16_t i;
	volatile uint16_t len = 0; 					//缓存计数
	volatile uint16_t dataLen = 0;				//本次发送数据总长
	volatile uint16_t tmpLen = 0;					//用作临时处理使用长度
	len = 0;
	outdata[len++] = 0x00;
	outdata[len++] = 0x01;         //报文编号，固定01
	outdata[len++] = 0x00;
	outdata[len++] = ProtocolVS07;         //使用的协议版本
	for(i = 0; i < InvCommCtrl.HoldingNum; i++)
	{
		dataLen += ((InvCommCtrl.HoldingGrop[i][1] - InvCommCtrl.HoldingGrop[i][0] + 1) * 2);
	}
	dataLen += (InvCommCtrl.HoldingNum * 4);		//每组起始2+结束2

	outdata[len++] = HIGH((29 + dataLen));
	outdata[len++] = LOW((29 + dataLen));

	outdata[len++] = InvCommCtrl.Addr;        	//本机编号
	outdata[len++] = 0x03;         				//命令-上传采集器采集到的逆变器数据。

	memcpy(&outdata[len], &SysTAB_Parameter[Parameter_len[REG_SN]], 10);				//采集器SN号
	len += 10;
	memcpy(&outdata[len], InvCommCtrl.SN, 10);	//逆变器SN号
	len += 10;

	outdata[len++] =0;         //6byte     时间戳
	outdata[len++] =0;
	outdata[len++] =0;
	outdata[len++] =0;
	outdata[len++] =0;
	outdata[len++] =0;
	outdata[len++] = InvCommCtrl.HoldingNum;   // 分段数据
	dataLen = 0;
	tmpLen = 0;
	for(i = 0; i < InvCommCtrl.HoldingNum; i++)
	{
		/* 寄存器编号 */
		outdata[len++] = HIGH(InvCommCtrl.HoldingGrop[i][0]);
		outdata[len++] = LOW(InvCommCtrl.HoldingGrop[i][0]);
		outdata[len++] = HIGH(InvCommCtrl.HoldingGrop[i][1]);
		outdata[len++] = LOW(InvCommCtrl.HoldingGrop[i][1]);

		dataLen = ((InvCommCtrl.HoldingGrop[i][1] - InvCommCtrl.HoldingGrop[i][0] + 1) * 2);	//本组寄存器长度
		memcpy(&outdata[len], &InvCommCtrl.HoldingData[tmpLen], dataLen);

		len += dataLen;
		tmpLen += dataLen;
		if(tmpLen > DATA_MAX)break;// 防止数据溢出
	}
	//IOT_Protocol_ToSever(outdata, len);
	IOT_Printf("\r\n HoldingGrop_DataPacket********len =%d********  \r\n",len);
	return len;
}
/********************************************
 * InputGrop  输入寄存器 打包
 *  04数据
 *******************************************/
uint16_t InputGrop_DataPacket(uint8_t *outdata)
{
	uint16_t i=0;
	uint16_t len = 0; 					//缓存计数
	uint16_t dataLen = 0;				//本次发送数据总长
	uint16_t tmpLen = 0;				//用作临时处理使用长度
	len = 0;
	outdata[len++] = 0x00;				//
	outdata[len++] = 0x01;        		//报文编号，固定01
	outdata[len++] = 0x00;				//
	outdata[len++] = ProtocolVS07;      //使用的协议版本

	for(i = 0; i < InvCommCtrl.InputNum; i++)
	{
		dataLen += ((InvCommCtrl.InputGrop[i][1] - InvCommCtrl.InputGrop[i][0] + 1) * 2);
	}
	dataLen += (InvCommCtrl.InputNum * 4); //每组起始2+结束2

	outdata[len++] = HIGH((29 + dataLen));
	outdata[len++] = LOW((29 + dataLen));

	outdata[len++] = InvCommCtrl.Addr;    //本机编号
	outdata[len++] = 0x04;         		  //命令-上传采集器采集到的逆变器数据。

	memcpy(&outdata[len], &SysTAB_Parameter[Parameter_len[REG_SN]], 10);	 //采集器SN号
	len += 10;

	memcpy(&outdata[len], InvCommCtrl.SN, 10);			 //逆变器SN号
	len += 10;

	outdata[len++] = sTime.data.Year   ;         			  //6byte     时间戳  28 29 30 31 32 33
	outdata[len++] = sTime.data.Month  ;
	outdata[len++] = sTime.data.Date   ;
	outdata[len++] = sTime.data.Hours  ;
	outdata[len++] = sTime.data.Minutes  ;
	outdata[len++] = sTime.data.Seconds  ;

	outdata[len++] = InvCommCtrl.InputNum;
	dataLen = 0;
	tmpLen = 0;
	for(i = 0; i < InvCommCtrl.InputNum; i++)   //125*2  =250 一组数据
	{
		/* 寄存器编号 */
		outdata[len++] = HIGH(InvCommCtrl.InputGrop[i][0]);
		outdata[len++] = LOW(InvCommCtrl.InputGrop[i][0]);
		outdata[len++] = HIGH(InvCommCtrl.InputGrop[i][1]);
		outdata[len++] = LOW(InvCommCtrl.InputGrop[i][1]);

		dataLen = ((InvCommCtrl.InputGrop[i][1] - InvCommCtrl.InputGrop[i][0] + 1) * 2);	//本组寄存器长度
		memcpy(&outdata[len], &InvCommCtrl.InputData[tmpLen], dataLen);

		len += dataLen;
		tmpLen += dataLen;
		if(tmpLen > DATA_MAX)break;// 防止数据溢出
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
 *  HoldingGrop  输入寄存器 打包
 *  03数据
 *******************************************/
uint16_t Server0x05_DataPacket(uint8_t *outdata)
{
	uint8_t Nums = 0;
	uint16_t len = 0; 					//缓存计数
	uint8_t  *pPVackbuf=InvCommCtrl.GetParamDataBUF;
	Nums = ((InvCommCtrl.SetParamEnd - InvCommCtrl.SetParamStart)+1)*2;
	Nums = Nums+6+12+4; 				// 发送数据总个数

	outdata[len++] = 0x00;
	outdata[len++] = 0x01; 				//报文编号，固定01
	outdata[len++] = 0x00;
	outdata[len++] = ProtocolVS07;      //使用的协议版本
	outdata[len++] = HIGH((Nums-6));
	outdata[len++] = LOW((Nums-6));						 // 数据长度
	outdata[len++] = InvCommCtrl.Addr;	// 设备地址
	outdata[len++] = 0x05;

	memcpy(&outdata[len], &SysTAB_Parameter[Parameter_len[REG_SN]], 10);	 //采集器SN号
	len += 10;

	outdata[len++] = HIGH(InvCommCtrl.SetParamStart);    // 起始寄存器地址H
	outdata[len++] = LOW(InvCommCtrl.SetParamStart);	 // 起始寄存器地址L
	outdata[len++] = HIGH(InvCommCtrl.SetParamEnd);		 // 结束寄存器H
	outdata[len++] = LOW(InvCommCtrl.SetParamEnd);		 // 结束寄存器L

	if(InvCommCtrl.Set_InvRespondCode == PV_ACKOK)  //逆变器轮训获取到数据 置为
	{
		memcpy(&outdata[22], &pPVackbuf[3],pPVackbuf[2]); // 01 03 06 01 02 03 04 05 06
		len+=pPVackbuf[2];
	}
	else  //逆变器轮训获取到数据异常
	{
		memset(&outdata[22], 0,(Nums-22));
		len+=(Nums-22);
	}
	return len;
}
/********************************************
 *  设置逆变器数据  应答服务数据状态
 *  与ACK 兼容 服务器未使用
 *******************************************/

uint16_t Server0x06_DataPacket(uint8_t *outdata)
{
	uint8_t Nums = 17;          //数据长度
	uint16_t len = 0; 			//缓存计数

	outdata[len++] = 0x00;		//报文编号
	outdata[len++] = 0x01; 		//报文编号，固定01
	outdata[len++] = 0x00;
	outdata[len++] = ProtocolVS07;      //使用的协议版本

	outdata[len++] = HIGH(Nums);
	outdata[len++] = LOW(Nums);			// 数据长度
	outdata[len++] = InvCommCtrl.Addr;	// 设备地址

	outdata[len++] = 0x06;				//功能码

	memcpy(&outdata[len], &SysTAB_Parameter[Parameter_len[REG_SN]], 10);		 //采集器SN号
	len += 10;

	outdata[len++] = HIGH(InvCommCtrl.Set_InvParameterNum);  // 设置的逆变器参数编号 H
	outdata[len++] = LOW(InvCommCtrl.Set_InvParameterNum);	 // 设置的逆变器参数编号L

	outdata[len++] = InvCommCtrl.Set_InvRespondCode;		 // 06设置成功/失败状态

	outdata[len++] = HIGH(InvCommCtrl.Set_InvParameterData); // 要设置的逆变器参数H
	outdata[len++] = LOW(InvCommCtrl.Set_InvParameterData);	 // 要设置的逆变器参数L
	return len;

}



/********************************************
 *  设置逆变器数据 应答服务数据状态
 *  与ACK 兼容 服务器未使用
 *******************************************/
uint16_t Server0x10_DataPacket(uint8_t *outdata)
{
	uint8_t Nums = 17;          //数据长度
	uint16_t len = 0; 			//缓存计数

	outdata[len++] = 0x00;		//报文编号
	outdata[len++] = 0x01; 		//报文编号，固定01
	outdata[len++] = 0x00;
	outdata[len++] = ProtocolVS07;      //使用的协议版本

	outdata[len++] = HIGH(Nums);
	outdata[len++] = LOW(Nums);		// 数据长度
	outdata[len++] = InvCommCtrl.Addr;	// 设备地址
	outdata[len++] = 0x10;		//功能码
	memcpy(&outdata[len], &SysTAB_Parameter[Parameter_len[REG_SN]], 10);		 //采集器SN号
	len += 10;


	outdata[len++] = HIGH(InvCommCtrl.SetParamStart);  	 // 设置的开始逆变器参数编号 H
	outdata[len++] = LOW(InvCommCtrl.SetParamStart);	 //
	outdata[len++] = HIGH(InvCommCtrl.SetParamEnd);      //
	outdata[len++] = LOW(InvCommCtrl.SetParamEnd);	     //

	outdata[len++] = InvCommCtrl.Set_InvRespondCode;		 // 06设置成功/失败状态

	return len;

}

/********************************************
 *  获取采集器参数数据  应答服务获取的数据
 *  0x19 数据
 *  弃用
 *******************************************/
uint16_t Server0x19_DataPacket(uint8_t *outdata)
{
	uint16_t len = 0; 				 // 缓存计数
	uint16_t TABNumslen =  0 ;       // 数据长度
	uint16_t TABNums = 0 ;			 // 参数编号
	TABNums = System_state.ServerGET_REGAddrStart;
	TABNumslen = IOT_CollectorParam_Get(System_state.ServerGET_REGAddrStart, System_state.Get_TabDataBUF );

	//GetParam();

	outdata[len++] = 0x00;			// 报文编号
	outdata[len++] = 0x01; 			// 报文编号，固定01
	outdata[len++] = 0x00;
	outdata[len++] = ProtocolVS07;      	// 使用的协议版本

	outdata[len++] = HIGH((TABNumslen+16));
	outdata[len++] = LOW((TABNumslen+16));// 数据长度

	outdata[len++] = 1;				// 设备地址

	outdata[len++] = 0x19;			//功能码

	memcpy(&outdata[len], &SysTAB_Parameter[Parameter_len[REG_SN]], 10);		 //采集器SN号
	len += 10;

	IOT_Printf("\r\n Server0x19_DataPacket********TABNums =%d********  \r\n",TABNums);
	IOT_Printf("\r\n ***********************TABNumslen =%d********************* \r\n"  ,TABNumslen );

	outdata[len++] = HIGH(TABNums);  		// 设置的 参数编号 H
	outdata[len++] = LOW(TABNums);	 		// 设置的 参数编号L
	outdata[len++] = HIGH(TABNumslen); 	 	// 要设置的 参数H
	outdata[len++] = LOW(TABNumslen);	 	// 要设置的 参数L

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
 *  获取采集器参数数据  应答服务获取的数据
 *  0x18 数据
 *******************************************/
uint16_t Server0x18_DataPacket(uint8_t *outdata)
{
	uint16_t len = 0; 				 // 缓存计数
	uint16_t TABNumslen =  0 ;       // 数据长度
	uint16_t TABNums = 0 ;			 // 参数编号
	TABNums = System_state.ServerSET_REGAddrStart;
	//TABNumslen = IOT_CollectorParam_Get(System_state.ServerGET_REGAddrStart, System_state.Get_TabDataBUF );
	//GetParam();
	outdata[len++] = 0x00;			// 报文编号
	outdata[len++] = 0x01; 			// 报文编号，固定01
	outdata[len++] = 0x00;
	outdata[len++] = ProtocolVS07;  // 使用的协议版本

	outdata[len++] = 0x00;
	outdata[len++] = 15;		// 数据长度

	outdata[len++] = 0x01;			// 设备地址

	outdata[len++] = 0x18;			//功能码

	memcpy(&outdata[len], &SysTAB_Parameter[Parameter_len[REG_SN]], 10);		 //采集器SN号
	len += 10;

	IOT_Printf("\r\n Server0x18_DataPacket********TABNums =%d********  \r\n",TABNums);
	IOT_Printf("\r\n ***********************TABNumslen =%d********************* \r\n"  ,TABNumslen );

	outdata[len++] = HIGH(TABNums);  		// 设置的 参数编号 H
	outdata[len++] = LOW(TABNums);	 		// 设置的 参数编号L
	outdata[len++] = System_state.ParamSET_ACKCode ; 	 				// 设置 成功
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
 *   采集器参数数据  主动上报参数列表数据
 *   0x19 数据 多参数打包   07协议以上版本支持
 *******************************************/
uint16_t Server0x19_MultiPack(uint8_t *outdata,uint16_t REGAddrStart ,uint16_t REGAddrEnd)
{

//	System_state.ServerGET_REGAddrStart; //服务器查询采集器寄存器地址开始
//	System_state.ServerGET_REGAddrEnd;	 //服务器查询采集器寄存器地址结束
//	System_state.Get_TabDataBUF;		 //要发送的数据
//
//	if(System_state.ServerGET_REGAddrStart-System_state.ServerGET_REGAddrEnd > 0 )  // 多个寄存器获取
//	{
//	}

	uint16_t len = 0; 				 // 缓存计数
	uint16_t TABNumslen =  0 ;       // 读取的 数据长度
	uint16_t TABNowNums = 0;		 // 当前参数序号
	uint16_t TABnumber =0;			 // 上报参数寄存器数量
	uint16_t REGdatalen=0;			 // 参数数据长度

	if((REGAddrEnd==31)&& (REGAddrStart==4)) // 包含31 会同步时间
	{
		uint8_t i = 0;
		TABnumber =  abs(REGAddrEnd -REGAddrStart)+1;
		System_state.ServerGET_REGNUM = abs(REGAddrEnd -REGAddrStart)+1;
		for(i = 0 ; i < System_state.ServerGET_REGNUM ;i++)
		{
			System_state.ServerGET_REGAddrBUFF[i] = i + REGAddrStart;
		}

		System_state.ServerGET_REGNUM += 1;			//新增上传信号值
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
	outdata[len++] = 0x00;			// 报文编号
	outdata[len++] = 0x01; 			// 报文编号，固定01

	outdata[len++] = 0x00;
	outdata[len++] = ProtocolVS07;       // 使用的协议版本

	outdata[len++] = HIGH((TABnumber*4+REGdatalen +15));
	outdata[len++] = LOW((TABnumber*4+REGdatalen +15));// 总数据长度

	outdata[len++] = 1;					 // 设备地址
	outdata[len++] = 0x19;				 // 功能码

	memcpy(&outdata[len], &SysTAB_Parameter[Parameter_len[REG_SN]], 10);		 //采集器SN号
	len += 10;

	outdata[len++] = HIGH(TABnumber); 	 // 要设置的参数编号个数H
	outdata[len++] = LOW(TABnumber);	 // 要设置的参数编号个数L

	outdata[len++] = 0x00;	 			 // 状态码


	if(TABNowNums==75)
	{
		outdata[len++] = HIGH(TABNowNums); 	 // 要设置的编号 H
		outdata[len++] = LOW(TABNowNums);	 // 要设置的编号 L
		outdata[len++] = HIGH(TABNumslen ); 	 // 要设置的编号参数长度 H
		outdata[len++] = LOW(TABNumslen );	 	 // 要设置的编号参数长度 L
		memcpy(&outdata[len] ,System_state.Get_TabDataBUF,TABNumslen);
	 	len+= TABNumslen;
	 	IOT_Printf("\r\n ***********TABNowNums==75************TABNumslen=%d********************* \r\n" ,TABNumslen  );
	}
	else
	{
		if(TABnumber == 1)
		{
			outdata[len++] = HIGH(TABNowNums); 	 // 要设置的编号 H
			outdata[len++] = LOW(TABNowNums);	 // 要设置的编号 L
			outdata[len++] = HIGH(Collector_Config_LenList[TABNowNums]); 	 // 要设置的编号参数长度 H
			outdata[len++] = LOW(Collector_Config_LenList[TABNowNums]);	 	 // 要设置的编号参数长度 L
			memcpy(&outdata[len] ,&SysTAB_Parameter[Parameter_len[TABNowNums]],Collector_Config_LenList[TABNowNums]);		 //复制数据
			len+= Collector_Config_LenList[TABNowNums];
		}
		else
		{
			for(uint16_t i=0; i<TABnumber ;i++)
			{
			outdata[len++] = HIGH(System_state.ServerGET_REGAddrBUFF [i]); 	 // 要设置的编号 H
			outdata[len++] = LOW(System_state.ServerGET_REGAddrBUFF [i]);	 // 要设置的编号 L
			outdata[len++] = HIGH(Collector_Config_LenList[System_state.ServerGET_REGAddrBUFF [i]]); 	 // 要设置的编号参数长度 H
			outdata[len++] = LOW(Collector_Config_LenList[System_state.ServerGET_REGAddrBUFF [i]]);	 	 // 要设置的编号参数长度 L

			memcpy(&outdata[len] ,&SysTAB_Parameter[Parameter_len[System_state.ServerGET_REGAddrBUFF [i]]],Collector_Config_LenList[System_state.ServerGET_REGAddrBUFF [i]]);		 //复制数据

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
 *  透传数据打包
 *  0x17数据
 *******************************************/

uint16_t Server0x17_DataPacket(uint8_t *outdata)
{

	uint8_t Nums = 0;
	uint16_t len = 0; 					// 缓存计数
	uint8_t  *pPVackbuf=InvCommCtrl.GetParamDataBUF;

	Nums = InvCommCtrl.GetParamData ;
 			// 发送数据总个数

	outdata[len++] = 0x00;
	outdata[len++] = 0x01; 				// 报文编号，固定01
	outdata[len++] = 0x00;
	outdata[len++] = ProtocolVS07;      // 使用的协议版本
	outdata[len++] = HIGH((Nums+14));	//
	outdata[len++] = LOW((Nums+14));		// 数据长度
	outdata[len++] = InvCommCtrl.Addr;	// 设备地址
	outdata[len++] = 0x17;

	memcpy(&outdata[len], &SysTAB_Parameter[Parameter_len[REG_SN]], 10);	 //采集器SN号
	len += 10;

	outdata[len++] = HIGH(Nums);    // 起始寄存器地址H
	outdata[len++] = LOW(Nums);	 // 起始寄存器地址L

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
 * Description  : esp 发送数据命令(0x03)
 * Parameters   : char *upSendTXBUF    : 发送缓冲区
 * Returns      : len 要发送的数据长度
 * Notice       : none
*******************************************************************************/
uint16_t SendIDcom_num=0;
uint16_t bCID_0x04=0;
uint16_t IOT_ESPSend_0x03(uint8_t *upSendTXBUF ,unsigned int Vsnum)
{
	uint16_t len = 0;
	uint16_t sendlen=0;

	len = HoldingGrop_DataPacket(upSendTXBUF);			//打包数据
	sendlen =IOT_Protocol_ToSever(upSendTXBUF, len ,Vsnum);    //协议加密
	return sendlen;
}

/********************************************************************
 * FunctionName : IOT_ESPSend_0x04
 * Description  : esp 发送数据命令(0x04)
 * Parameters   : char *upSendTXBUF  : 发送缓冲区
 * Returns      : len 要发送的数据长度
 * Notice       : none
*********************************************************************/
uint16_t IOT_ESPSend_0x04(uint8_t *upSendTXBUF ,unsigned int Vsnum)
{
	uint16_t len = 0;
	uint16_t sendlen=0;

	len = InputGrop_DataPacket(upSendTXBUF);			//打包数据

	if(SendIDcom_num < 125)								//计算CID编号
		SendIDcom_num++;
	else
		SendIDcom_num = 0;

	upSendTXBUF[0] = SendIDcom_num >> 8;				//重新指定通讯编号
	upSendTXBUF[1] = SendIDcom_num & 0x00ff;
	bCID_0x04 = SendIDcom_num;							//缓存0x04CID

	//已经在打包函数里面使用赋值。可以屏蔽
	upSendTXBUF[28] = sTime.Clock_Time_Tab[0];			//年，月日时分秒
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
 * Description  : esp 发送数据命令(0x05)
 * Parameters   : char *upSendTXBUF    : 发送缓冲区
 * Returns      : len 要发送的数据长度
 * Notice       : none
*********************************************************************/
uint16_t IOT_ESPSend_0x05(uint8_t *upSendTXBUF ,unsigned int Vsnum)
{
	uint16_t len = 0;
	uint16_t sendlen=0;
	len = Server0x05_DataPacket(upSendTXBUF);			//打包数据
	sendlen =IOT_Protocol_ToSever(upSendTXBUF, len ,Vsnum);    //协议加密
	return sendlen;

}

/*********************************************************************
 * FunctionName : IOT_ESPSend_0x06
 * Description  : esp 发送数据命令(0x06)
 * Parameters   : char *upSendTXBUF    : 发送缓冲区
 * Returns      : len 要发送的数据长度
 * Notice       : none
**********************************************************************/
uint16_t IOT_ESPSend_0x06(uint8_t *upSendTXBUF ,unsigned int Vsnum)
{
	uint16_t len = 0;
	uint16_t sendlen=0;
	len = Server0x06_DataPacket(upSendTXBUF);			//打包数据
	sendlen =IOT_Protocol_ToSever(upSendTXBUF, len ,Vsnum);    //协议加密
	return sendlen;
}

/*********************************************************************
 * FunctionName : IOT_ESPSend_0x10
 * Description  : esp 发送数据命令(0x05)
 * Parameters   : char *upSendTXBUF    : 发送缓冲区
 * Returns      : len 要发送的数据长度
 * Notice       : none
*********************************************************************/
uint16_t IOT_ESPSend_0x10(uint8_t *upSendTXBUF ,unsigned int Vsnum)
{
	uint16_t len = 0;
	uint16_t sendlen=0;
	len = Server0x10_DataPacket(upSendTXBUF);			//打包数据
	sendlen =IOT_Protocol_ToSever(upSendTXBUF, len, Vsnum);    //协议加密
	return sendlen;

}

/********************************************************************
 * FunctionName : IOT_ESPSend0x19
 * Description  : esp 发送数据命令(0x19)
 * Parameters   : char *upSendTXBUF                      : 发送缓冲区
 *             	  uint16_t sIN_REGAddrStart              : 读取命令的 起始地址
 *                uint16_t sIN_REGAddrEnd				 : 读取命令的 结束地址
 * Returns      : len 要发送的数据长度
 * Notice       : none
*********************************************************************/
uint16_t IOT_ESPSend_0x19(uint8_t *upSendTXBUF ,unsigned int Vsnum)
{
	uint16_t len = 0;
	uint16_t sendlen=0;
 	len = Server0x19_MultiPack(upSendTXBUF, System_state.ServerGET_REGAddrStart, System_state.ServerGET_REGAddrEnd);	//打包数据
 	sendlen =IOT_Protocol_ToSever(upSendTXBUF, len ,Vsnum);    //协议加密
	return sendlen;
}

/********************************************************************
 * FunctionName : IOT_ESPSend0x18
 * Description  : esp 发送数据命令(0x18)
 * Parameters   : char *upSendTXBUF                      : 发送缓冲区
 *             	  uint16_t sIN_REGAddrStart              : 读取命令的 起始地址
 *                uint16_t sIN_REGAddrEnd				 : 读取命令的 结束地址
 * Returns      : len 要发送的数据长度
 * Notice       : none
*********************************************************************/
uint16_t IOT_ESPSend_0x18(uint8_t *upSendTXBUF  ,unsigned int Vsnum)
{
	uint16_t len = 0;
	uint16_t sendlen=0;
//	if( System_state.ServerSET_REGAddrStart ==75)// 获取附近wifi热点 名称,多包数据发送
//		len = Server0x19_MultiPack(upSendTXBUF, System_state.ServerSET_REGAddrStart, System_state.ServerSET_REGAddrStart);			//打包数据
//	else
		len = Server0x18_DataPacket(upSendTXBUF);			//打包数据

	sendlen =IOT_Protocol_ToSever(upSendTXBUF, len ,Vsnum);    //协议加密
	return sendlen;

}

/********************************************************************
 * FunctionName : IOT_ESPSend0x17
 * Description  : esp 发送数据命令(0x17)
 * Parameters   : char *upSendTXBUF                      : 发送缓冲区
 *             	  uint16_t sIN_REGAddrStart              : 读取命令的 起始地址
 *                uint16_t sIN_REGAddrEnd				 : 读取命令的 结束地址
 * Returns      : len 要发送的数据长度
 * Notice       : none
*********************************************************************/
uint16_t IOT_ESPSend_0x17(uint8_t *upSendTXBUF ,unsigned int Vsnum)
{
	uint16_t len = 0;
	uint16_t sendlen=0;
 	len = Server0x17_DataPacket(upSendTXBUF);			//打包数据
//	ESP_LOGI(test1 ,"IOT_ESPSend_0x17 len= %d****\r\n",len);
 	sendlen =IOT_Protocol_ToSever(upSendTXBUF, len ,Vsnum);    //协议加密
	return sendlen;
}



/****************************************************************
 * FunctionName : IOT_UploadParamSelf
自主上传19参数信息给服务器, 不受其他条件影响
*****************************************************************/
#define SELF_UPLOAD_STR		4				//起始上传参数序号
#define SELF_UPLOAD_END		22				//终止上次参数序号
#define TIME_UPLPAD_AGAIN	(10 * 60 * 199)	//10分钟内不允许自主重复上传参数
#define TIME_FIRST_UPLPAD	(3 * 60 * 199)	//10分钟内不允许自主重复上传参数

uint16_t IOT_UploadParamSelf(uint8_t *upSendTXBUF ,unsigned int Vsnum)
{
	uint16_t len = 0;
	uint16_t sendlen=0;
 	len = Server0x19_MultiPack(upSendTXBUF,4,31);		//打包数据
	sendlen =IOT_Protocol_ToSever(upSendTXBUF, len,Vsnum);    //协议加密
	return sendlen;
}

#ifndef HTTP_PROGRESS
#define HTTP_PROGRESS		(0xa2)			//进度类型代码
#define HTTP_DL_ERROR		(0x85)			//下载固件失败
#endif
/********************************************************************
 * FunctionName : IOT_ESPSend_0x25
 * Description  : 升级进度上传
 * Parameters   : char *upSendTXBUF                      : 发送缓冲区
 *             	  uint16_t sIN_REGAddrStart              : 读取命令的 起始地址
 *                uint16_t sIN_REGAddrEnd				 : 读取命令的 结束地址
 * Returns      : len 要发送的数据长度
 * Notice       : none
*********************************************************************/
uint16_t IOT_ESPSend_0x25(uint8_t *upSendTXBUF ,unsigned int Vsnum )
{
	uint16_t sendlen = 0;
	uint8_t *TxBuf = upSendTXBUF;
	/* 上传进度0% */
	TxBuf[0] = 0x00;
	TxBuf[1] = 0x01;
	TxBuf[2] = 0x00;
	TxBuf[3] = LOW(Vsnum);
	TxBuf[4] = 0x00;
	TxBuf[5] = 0x17;
	TxBuf[6] = InvCommCtrl.Addr;
	TxBuf[7] = 0x25;

	memcpy(&TxBuf[8], &SysTAB_Parameter[Parameter_len[REG_SN]], 10);	 //采集器SN号
	memcpy(&TxBuf[18], InvCommCtrl.SN, 10);			 //逆变器SN号

	/* HTTP下载进度 */
	if(g_FotaParams.ProgressType == HTTP_PROGRESS)
	{
		TxBuf[28] = HTTP_PROGRESS;
		TxBuf[29] = g_FotaParams.uiFotaProgress;

		TxBuf[5] = 0x18;
		sendlen = IOT_Protocol_ToSever(TxBuf, 30, Vsnum);    //协议加密
	}
	/* 逆变器升级进度 */
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
		sendlen = IOT_Protocol_ToSever(TxBuf, 29, Vsnum);    //协议加密
	}
	IOT_Printf("\r\n IOT_ESPSend_0x25*****  g_FotaParams.uiFotaProgress  =%d****** \r\n", g_FotaParams.uiFotaProgress);
	return sendlen;
}

/********************************************
 *  APP与设备接收本地升级数据包
 *
 *******************************************/
uint16_t Server0x26_DataPacket(uint8_t *outdata)
{
	uint8_t Nums = 17;          //数据长度
	uint16_t len = 0; 			//缓存计数

	outdata[len++] = 0x00;		//报文编号
	outdata[len++] = 0x01; 		//报文编号，固定01
	outdata[len++] = 0x00;
	outdata[len++] = ProtocolVS05;      //使用的协议版本

	outdata[len++] = HIGH(Nums);
	outdata[len++] = LOW(Nums);			// 数据长度
	outdata[len++] = InvCommCtrl.Addr;	// 设备地址

	outdata[len++] = 0x26;		//功能码

	memcpy(&outdata[len], &SysTAB_Parameter[Parameter_len[REG_SN]], 10);	 //采集器SN号
	len += 10;

	outdata[len++] = HIGH(InvCommCtrl.Set_InvParameterNum);  // 设置的逆变器参数编号 H
	outdata[len++] = LOW(InvCommCtrl.Set_InvParameterNum);	 // 设置的逆变器参数编号L

	outdata[len++] = InvCommCtrl.Set_InvRespondCode;		 // 06设置成功/失败状态

	outdata[len++] = HIGH(InvCommCtrl.Set_InvParameterData); // 要设置的逆变器参数H
	outdata[len++] = LOW(InvCommCtrl.Set_InvParameterData);	 // 要设置的逆变器参数L

	return len;
}

/*****************************************************************
  *     续传数据到服务器
  *     发送采集到的0~44，,45~89的数据给服务器。
  *     指令号0x50，是续传数据
  ****************************************************************
  */
 uint16_t Server0x50_DataPacket(uint8_t *outdata)
{
	uint16_t len = 0;
	uint8_t *TxBuf = outdata;

	if(NULL == TxBuf) return 0;

	//len = readRecord(TxBuf);//  此处添加完整的一条离线数据读取
	len = IOT_ESP_SPI_FLASH_ReUploadData_Read_us(TxBuf , 1024);


	IOT_Printf("==================================\r\n");
	IOT_Printf(" Server0x50_DataPacket() readRecord  len= %d\r\n", len);
	IOT_Printf("==================================\r\n");
	if(len ==0)
	{
		IOT_Printf("===========Server0x50_DataPacket   error  return=======================\r\n");

		return 0;
	}

	TxBuf += 16;  //偏移16字节
	len -= 16;

	/* 检测存储的记录采集器SN和当前是否一致 */
	// 断点续传数据 序列号校验导致不上传数据
	if(memcmp(&TxBuf[8], &SysTAB_Parameter[Parameter_len[REG_SN]],10) != 0)			//保存的不是当前逆变器SN，或采集器SN
	{
		IOT_Printf("======检测存储的记录采集器SN和当前不一致====不上传========\r\n");
		return 0;
	}
	/* 检测存储的记录逆变器SN和当前是否一致 */
    //断点续传数据 序列号校验导致不上传数据
	if(memcmp(&TxBuf[18],InvCommCtrl.SN,10) != 0)			//保存的不是当前逆变器SN，或采集器SN
	{
		IOT_Printf("======检测存储的记录逆变器SN和当前不一致====不上传========\r\n");
		return 0;
	}
	/* 检测版本号是否为当前版本 */
	if(TxBuf[3] != PROTOCOL_VERSION_L)
	{
		IOT_Printf("======检测存储的记录检测版本号和当前不一致====不上传========\r\n");
		return 0;
	}

	memcpy(outdata , TxBuf , len);


	if(SendIDcom_num < 125)								//计算CID编号  与 04 的ID 相同
		SendIDcom_num++;
	else
		SendIDcom_num = 0;

	outdata[0] = SendIDcom_num >> 8;		//重新指定通讯编号
	outdata[1] = SendIDcom_num & 0x00ff;
	outdata[7] = 0x50;			//续传的指令0x50;
	bCID_0x04 = SendIDcom_num;


#if DEBUG_MQTT
	IOT_Printf("发送0x50指令 = \r\n");
	for(uint16_t i=0; i<len; i++)
		IOT_Printf("%02x ",outdata[i]);
	IOT_Printf("\r\n Server0x50_DataPacket\r\n");
#endif



	return len;

}

 /********************************************************************
  * FunctionName : IOT_ESPSend_0x50
  * Description  : esp 发送数据命令(0x50)
  * Parameters   : char *upSendTXBUF                      : 发送缓冲区
 	 	 	 	 	 unsigned int Vsnum 				  : 协议版本
  * Returns      : len 要发送的数据长度
  * Notice       : none
 *********************************************************************/
uint16_t IOT_ESPSend_0x50(uint8_t *upSendTXBUF ,unsigned int Vsnum)
{
 	uint16_t len = 0;
 	uint16_t sendlen=0;
  	len = Server0x50_DataPacket(upSendTXBUF);			//打包数据
  	if(len ==0)
	{
		return 0;
	}
  	sendlen =IOT_Protocol_ToSever(upSendTXBUF, len ,Vsnum);    //协议加密

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
   * Description  : esp 发送数据命令(0x20)
   * Parameters   : char *upSendTXBUF                     : 发送缓冲区
  	 	 	 	 	 unsigned int Vsnum 				  : 协议版本
   * Returns      : len 要发送的数据长度
   * Notice       : none
*********************************************************************/
uint16_t IOT_ESPSend_0x20(uint8_t *upSendTXBUF ,unsigned int Vsnum)
{
  	uint16_t len = 0;
  	uint16_t sendlen=0;
   	len = IOT_Ammeter_DataPacket(upSendTXBUF);			//打包数据
   	if(len ==0)
 	{
 		return 0;
 	}
   	sendlen =IOT_Protocol_ToSever(upSendTXBUF, len ,Vsnum);    //协议加密
  	return sendlen;
}

/********************************************************************
    * FunctionName : IOT_ESPSend_0x22
    * Description  : esp 发送数据命令(0x22)
    * Parameters   : char *upSendTXBUF                     : 发送缓冲区
   	 	 	 	 	 unsigned int Vsnum 				  : 协议版本
    * Returns      : len 要发送的数据长度
    * Notice       : none
*********************************************************************/
uint16_t IOT_ESPSend_0x22(uint8_t *upSendTXBUF ,unsigned int Vsnum)
{
   	uint16_t len = 0;
   	uint16_t sendlen=0;
    len = IOT_History_Ammeter_DataPacket(upSendTXBUF);			//打包数据
    if(len ==0)
  	{
  		return 0;
  	}
    sendlen =IOT_Protocol_ToSever(upSendTXBUF, len ,Vsnum);    //协议加密
   	return sendlen;
}


/********************************************
 * 一键诊断数据
 *
 *******************************************/
uint16_t Server0x14_DataPacket(uint8_t *outdata ,uint16_t start_addr, uint16_t end_addr, uint8_t *msg, uint16_t msg_len)
{
	uint8_t i = 0;


	uint8_t Nums = (8 + 20 + 6 + 1 + 4 + msg_len)-6;          //数据长度
	uint16_t len = 0; 			//缓存计数

	outdata[len++] = 0x00;		//报文编号
	outdata[len++] = 0x01; 		//报文编号，固定01
	outdata[len++] = 0x00;
	outdata[len++] = ProtocolVS07;      //使用的协议版本

	outdata[len++] = HIGH(Nums);
	outdata[len++] = LOW(Nums);			// 数据长度
	outdata[len++] = InvCommCtrl.Addr;	// 设备地址

	outdata[len++] = 0x14;		//功能码

	memcpy(&outdata[len], &SysTAB_Parameter[Parameter_len[REG_SN]], 10);	 //采集器SN号
	len += 10;
	memcpy(&outdata[len], InvCommCtrl.SN, 10);			 //逆变器SN号
	len += 10;

	//时间 上报1500v pv曲线条数    2020-05-09
	for(i=0 ;i < 6 ;i++)
	{
		outdata[len ++] = IOT_InverterPVWaveNums_Handle_uc(1,0);
	}
	outdata[len++] = 0x01;// 数据段数
	outdata[len++] = HIGH(start_addr);			// 数据寄存器起始地址
	outdata[len++] = LOW(start_addr);
	outdata[len++] = HIGH(end_addr);  			// 数据寄存器结束地址
	outdata[len++] = LOW(end_addr);
	memcpy(&outdata[len],(char*)msg,msg_len);	// 得到数据区数据
	len+=msg_len ;

	return len;
}
/********************************************************************
    * FunctionName : IOT_ESPSend_0x14
    * Description  : esp 发送数据命令(0x14)
    * Parameters   : char *upSendTXBUF                     : 发送缓冲区
   	 	 	 	 	 unsigned int Vsnum 				  : 协议版本
    * Returns      : len 要发送的数据长度
    * Notice       : none
*********************************************************************/
uint16_t IOT_ESPSend_0x14(uint8_t *upSendTXBUF ,unsigned int Vsnum)
{
   	uint16_t len = 0;
   	uint16_t sendlen=0;
   	len =Server0x14_DataPacket(upSendTXBUF,InvCommCtrl.SetParamStart,\
   			InvCommCtrl.SetParamEnd ,InvCommCtrl.GetParamDataBUF,InvCommCtrl.GetParamData);// 发送到服务器
    sendlen =IOT_Protocol_ToSever(upSendTXBUF, len ,Vsnum);    //协议加密
   	return sendlen;
}

/*****************************************************************
  *     响应0x14指令
  *		Status:	00 	响应成功指令
  *				01	无响应
  *				02	响应失败指令
  ****************************************************************
  *///(uint8_t *pRead,uint8_t QoCack)
void IOT_ServerACK_0x14(void)
{

//	if(QoCack==0)
//	{
//		if(pRead[8] == 0x00) // 0x14应答状态码
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
	InverterWaveStatus_Judge(0,IOT_ING);	// 设置波形数据获取进行中标志
}

/********************************************************************
    * FunctionName : IOT_ESPSend_0x37
    * Description  : esp 发送数据命令(0x37)
    * Parameters   : char *upSendTXBUF                     : 发送缓冲区
   	 	 	 	 	 unsigned int Vsnum 				  : 协议版本
    * Returns      : len 要发送的数据长度
    * Notice       : none
*********************************************************************/
uint16_t IOT_ESPSend_0x37(uint8_t *upSendTXBUF ,unsigned int Vsnum)
{
   	uint16_t len = 0;
   	uint16_t sendlen=0;

    sendlen =IOT_Protocol_ToSever(upSendTXBUF, len ,Vsnum);    //协议加密
   	return sendlen;
}
/********************************************************************
    * FunctionName : IOT_ESPSend_0x38
    * Description  : esp 发送数据命令(0x38)
    * Parameters   : char *upSendTXBUF                     : 发送缓冲区
   	 	 	 	 	 unsigned int Vsnum 				  : 协议版本
    * Returns      : len 要发送的数据长度
    * Notice       : none
*********************************************************************/
uint16_t IOT_ESPSend_0x38(uint8_t *upSendTXBUF ,unsigned int Vsnum)
{
   	uint16_t len = 0;
   	uint16_t sendlen=0;

    sendlen =IOT_Protocol_ToSever(upSendTXBUF, len ,Vsnum);    //协议加密
   	return sendlen;
}
#if 0
/******************************************************************************
 * FunctionName : IOT_LocalTCPCode0x26_Handle() add LI 20210115
 * Description  : 接收本地 升级数据包
 * Parameters   : unsigned char *pucSrc_tem : 升级数据包
 * Returns      : none
 * Notice       : 0x19:
 * 	 请求：0001 0005 0014 01 26 30303030303030303030 [0006 00FF 0001 0001 2*crc] - 下发总包 255/第一个包 /数据 0001
 * 	 应答：0001 0005 0013 01 26 30303030303030303030 [00FF 0001 00 2*crc] - 下发总包 255/第一个包 /状态 00
*******************************************************************************/
void IOT_LocalTCPCode0x26_Handle(unsigned char *pucSrc_tem)
{
/*
	unsigned char ucAckBuf[5] = {0};
	unsigned char ucAcklen = 0;
	unsigned char ucDataOffset = 6;         // 数据偏移地址，6-除去数据长度2b / 总包数2b / 当前包数2b
	unsigned char i = 0;
	unsigned int  uiPackageLen = 0;         // 升级包长度
	unsigned int  uiSumPackageNums = 0;     // 总升级包数
	unsigned int  uiCurrentPackageNums = 0; // 当前升级包编号

	ucAckBuf[0] = pucSrc_tem[2];
	ucAckBuf[1] = pucSrc_tem[3]; 			// 总包数据
	ucAckBuf[2] = pucSrc_tem[4];
	ucAckBuf[3] = pucSrc_tem[5]; 			// 当前包数
	ucAckBuf[4] = 0x00;         			// 应答状态 -- 成功
	ucAcklen = 7;                			// 应答长度

	uiPackageLen = IOT_ESP_htoi(&pucSrc_tem[0],2);
	uiSumPackageNums = IOT_ESP_htoi(&pucSrc_tem[2],2);
	uiCurrentPackageNums = IOT_ESP_htoi(&pucSrc_tem[4],2);

	IOT_ESP_Char_printf("IOT_LocalTCPCode0x26_Handle()_1", &pucSrc_tem[ucDataOffset], uiPackageLen-4);

	if((uiPackageLen > (1024 + 4)) || \
	   (uiSumPackageNums > 512))     // 单包升级数据包长度大于 1024字节 / 总升级数据包大小> 512k
	{
		ucAckBuf[4] = 0x02;          // 应答状态 -- 单包超长度失败失败
		goto ACK_FAIL_0X26;
	}
	if(uiCurrentPackageNums == 0x00) // 第一包数据  开始读取断点数据包编号  开始做升级准备
	{
		if(IOT_LocalFota_Handle_uc(0, &pucSrc_tem[ucDataOffset], (uiPackageLen-4))) // 写入第一包数据, 升级初始化
		{
			ucAckBuf[4] = 0x01;      // 应答状态 -- 单包超长度失败失败
			goto ACK_FAIL_0X26;
		}
		if((GuiIOTESPHTTPBreakPointSaveSize >= 8*1024) && (GuiIOTESPHTTPBreakPointSaveSize)) // 有断点数据
		{
			ucAckBuf[2] = U16_HIGH((GuiIOTESPHTTPBreakPointSaveSize/1024));
			ucAckBuf[3] = U16_LOW((GuiIOTESPHTTPBreakPointSaveSize/1024));   // 当前包数
		}
	}
	else // 开始保存升级文件包
	{
		if(IOT_LocalFota_Handle_uc(1, &pucSrc_tem[ucDataOffset], (uiPackageLen-4))) // 写入升级包数据
		{
			ucAckBuf[4] = 0x01;      // 应答状态 -- 单包超长度失败失败
			goto ACK_FAIL_0X26;
		}

		if(uiSumPackageNums == uiCurrentPackageNums) // 升级文件包下发完成，开始触发升级
		{}
	}

//IOT_ESP_Char_printf("IOT_LocalTCPCode0x26_Handle()", &pucSrc_tem[0], uiPackageLen+4);
ACK_FAIL_0X26:

	IOT_LocalTCPDataAck_Package(0x26, ucAckBuf, ucAcklen);

	if(_g_ucLocalFotaStatus)
	{
		_g_ucLocalFotaStatus = 0;
		vTaskDelay(1000 / portTICK_RATE_MS); // 延时 1S 等待数据发送完成
		IOT_ESP_Fota_Recycle(); // 升级失败/成功释放升级参数空间，，注： 升级成功后会马上重启系统，执行新固件
	}
*/
}
#endif
