///*
// * iot_ammeter.c
// *
// *  Created on: 2022年6月9日
// *      Author: Administrator
// */

#include "iot_inverter.h"
#include "iot_universal.h"
#include "iot_uart1.h"
#include "iot_system.h"
#include "iot_protocol.h"
#include "iot_rtc.h"
#include "iot_ammeter.h"


Ammeter_date uAmmeter={0};

//电表寄存器组
InverterMeterGroup InvMeter=
{
  1,
  {{180, 199}, {0, 0}, {0, 0}, {0, 0}, {0, 0},},

  1,
  {{0, 99}, {0, 0}, {0, 0}, {0, 0}, {0, 0},},
};

void IOT_Histotyamemte_datainit(void)
{
	uAmmeter.Flag_history_meter=0;         		//上报电表标志
	uAmmeter.Flag_GETPV_Historymeter188 =0;    //服务器不在线，告诉发送逆变器 1
	uAmmeter.uSETPV_Historymeter188=0;
	uAmmeter.Flag_GEThistorymeter =0;		   // 0x20 获取 77 地址判断是否有历史数据
}

//获取的标志状态
uint8_t IOT_GET_Histotymeter_Flag(void )
{
	return uAmmeter.Flag_history_meter;
}

// 设置标志状态
uint8_t IOT_SET_Histotymeter_Flag( uint8_t  uFlaghammeter)
{
	uAmmeter.Flag_history_meter = uFlaghammeter;
//	IOT_Printf("\r\n\r\n************** ****\r\n IOT_SET_Histotymeter_Flag Flag_history_meter=%d",uFlaghammeter);
	return uAmmeter.Flag_history_meter;

}

uint8_t IOT_Histotymeter_Timer(void )
{
 	if(uAmmeter.Flag_GEThistorymeter>0)
	{
		IOT_SET_Histotymeter_Flag(1);	// 读取历史数据电表
		return 1;
	}
	return 0;
}

//采集器与服务器通讯断开发送 电表存历史数据
void IOT_Send_PV_0x06_188( uint8_t ucENflag)
{

	InvCommCtrl.CMDFlagGrop |= CMD_0X06;
//	InvCommCtrl.RespondFlagGrop |= CMD_0X06;

	InvCommCtrl.Set_InvParameterNum = 188;
	InvCommCtrl.Set_InvParameterData = ucENflag;
	//SetInvOutTime_RESET();			//重置超时时间

}

void IOT_clear_Historymeter188(void)
{
	// 如果 设置了 188  但是 采集器离线 且状态位 是0   重新设置
	if((uAmmeter.uSETPV_Historymeter188>0 )&& (((System_state.Server_Online==0 )&& ( uAmmeter.Flag_GETPV_Historymeter188==Flag_sever_online) )
			||((System_state.Server_Online==1 )&& ( uAmmeter.Flag_GETPV_Historymeter188==Flag_sever_offline) ) ))
	{	uAmmeter.uSETPV_Historymeter188=0;
//	IOT_Printf("  ****IOT_clear_Historymeter188*********uAmmeter.Flag_GETPV_Historymeter188= %d\r\n",uAmmeter.Flag_GETPV_Historymeter188); // 调试用
	}
}

uint8_t IOT_GET_SEND_188FLAG(void)
{
	if((System_state.Server_Online==0 )&& ( uAmmeter.Flag_GETPV_Historymeter188==Flag_sever_online) )
	{return 1;	}
	else if((System_state.Server_Online==1 )&& ( uAmmeter.Flag_GETPV_Historymeter188==Flag_sever_offline) )
	{return 1;	}
	else
	{return 0;	}

}

uint8_t IOT_Send_PV_meterFlag(void)
{
   // 服务器离线   要写 1
	if((System_state.Server_Online == 0 )&& ( uAmmeter.Flag_GETPV_Historymeter188 == Flag_sever_online) )
	{
		if(uAmmeter.uSETPV_Historymeter188 == 0)
		{
			IOT_Send_PV_0x06_188(Flag_sever_offline);
			uAmmeter.uSETPV_Historymeter188=1;
			IOT_Printf("\r\n\r\n============(System_state.Server_Online == 0 ) IOT_Send_PV_0x06_188=====1===============\r\n");
		}
		return 1;
	}
	else if((System_state.Server_Online==1 )&& ( uAmmeter.Flag_GETPV_Historymeter188==Flag_sever_offline) )
	{
		if(uAmmeter.uSETPV_Historymeter188==0)
		{
			IOT_Send_PV_0x06_188(Flag_sever_online);
			uAmmeter.uSETPV_Historymeter188=1;
			IOT_Printf("\r\n\r\n============(System_state.Server_Online==1 ) IOT_Send_PV_0x06_188=====0===============\r\n");
		}
		return 1;
	}
	else
	{
		return 0;
	}
}

/******************************************
  *     获取History电表（读Input寄存器）指令给逆变器
  *     @addr:			设备地址, 0为广播地址
  *     @start_reg: 	起始寄存器
  *		@end_reg:		结束寄存器
  *		对GRT的逆变器, 起始和结束要在45的整数倍之间, 例:0-44, 45-89, 90-134...
  *****************************************
  */
void IOT_GetModbusHistoryMeter_22(uint8_t addr, uint16_t start_reg, uint16_t end_reg)
{
	unsigned int wCRCchecknum=0;
	unsigned char  com_tab[8]= {0x00,0x03,0x00,0,0x00,0x2d,0x00,0x00};

	com_tab[0] = addr;
	com_tab[1] = 0x22;
	com_tab[2] = HIGH(start_reg);
	com_tab[3] = LOW(start_reg);
	com_tab[4] = HIGH((end_reg - start_reg + 1));
	com_tab[5] = LOW((end_reg - start_reg + 1));

	wCRCchecknum = Modbus_Caluation_CRC16(com_tab, 6);

	com_tab[6] = LOW(wCRCchecknum);					//需要注意的是, CRC校验是低位在前
	com_tab[7] = HIGH(wCRCchecknum);

	IOT_UART1SendData(com_tab,8);
}
/******************************************
  *     获取电表（读Input寄存器）指令给逆变器
  *     @addr:			设备地址, 0为广播地址
  *     @start_reg: 	起始寄存器
  *		@end_reg:		结束寄存器
  *		对GRT的逆变器, 起始和结束要在45的整数倍之间, 例:0-44, 45-89, 90-134...
  *****************************************
  */
void IOT_GetModbusMeter_20(uint8_t addr, uint16_t start_reg, uint16_t end_reg)
{
	unsigned int wCRCchecknum=0;
	unsigned char  com_tab[8]= {0x00,0x20,0x00,0x00,0x00,0x2d,0x00,0x00};

	com_tab[0] = addr;
	com_tab[1] = 0x20;
	com_tab[2] = HIGH(start_reg);
	com_tab[3] = LOW(start_reg);
	com_tab[4] = HIGH((end_reg - start_reg + 1));
	com_tab[5] = LOW((end_reg - start_reg + 1));

	wCRCchecknum = Modbus_Caluation_CRC16(com_tab, 6);

	com_tab[6] = LOW(wCRCchecknum);					//需要注意的是, CRC校验是低位在前
	com_tab[7] = HIGH(wCRCchecknum);

	IOT_UART1SendData(com_tab,8);

}
/******************************************
  *     电表数据接收处理
  *     @pbuf:	输入缓存
  *     @len: 	输入长度
******************************************/
uint8_t IOT_HistorymeterReceive(uint8_t *pbuf, uint16_t len)
{
	uint16_t *pGropNumCur = 0;
	uint16_t cacheLoc = 0;
	uint8_t *pCache = NULL;

	/* 当前轮询的指令是否为History_Merer_GROUP */
	if(InvCommCtrl.CMDCur != History_Merer_GROUP)
		return 1;

	/* 传递当前轮询的参数指针 */
	pGropNumCur = (uint16_t *)&InvMeter.MeterGroup[InvCommCtrl.GropCur][0];		//当前轮序第几组

	/* 数据缓存的位置 */
//	cacheLoc = (*pGropNumCur) * 2;// LI 2017.10.14
	cacheLoc = ((*(pGropNumCur+1) - *pGropNumCur + 1)*(InvCommCtrl.GropCur))* 2; // LI 2017.10.14
	pCache = &uAmmeter.HistorymeterGroupData[cacheLoc];

	//mprintf(LOG_INFO,"接收到逆变器电表数据 pbuf[2] = %d cacheLoc = %d = %d\r\n",pbuf[2],cacheLoc,InvCommCtrl.GropCur); // 调试用

	if((pbuf == NULL) || (pCache == NULL) || (len > DATA_MAX))
	{
		return 1;
	}

	/* 缓存数据 */
	memcpy(pCache, &pbuf[3], pbuf[2]);		//数据区


	if(IOT_GET_Histotymeter_Flag() == 1)
		IOT_SET_Histotymeter_Flag(2);  //接收到电表数据

//	printf("\r\n IOT_HistorymeterReceive Histotymeter_Flag = %d\r\n",IOT_GET_Histotymeter_Flag());

	InvCommCtrl.NoReplyCnt = 0;
	PollingSwitch();

	return 0;
}


/******************************************
  *     电表数据接收处理
  *     @pbuf:	输入缓存
  *     @len: 	输入长度
  *****************************************
  */
uint8_t IOT_MeterReceive(uint8_t *pbuf, uint16_t len)
{
	uint16_t *pGropNumCur = 0;
	uint16_t cacheLoc = 0;
	uint8_t *pCache = NULL;
	uint16_t Historynum=0;
	/* 当前轮询的指令是否为METER_GROUP */
	if(InvCommCtrl.CMDCur != METER_GROUP)
		return 1;
	/* 传递当前轮询的参数指针 */
	pGropNumCur = (uint16_t *)&InvCommCtrl.MeterGroup[InvCommCtrl.GropCur][0];	//当前轮序第几组
	/* 数据缓存的位置 */
	cacheLoc = ((*(pGropNumCur+1) - *pGropNumCur + 1)*(InvCommCtrl.GropCur))* 2;
	pCache = &uAmmeter.MeterGroupData[cacheLoc];
  	if((pbuf == NULL) || (pCache == NULL) || (len > DATA_MAX))
	{
		return 1;
	}
	/* 缓存数据 */
	memcpy(pCache, &pbuf[3], pbuf[2]);		//数据区

	Historynum = pCache[77*2];		//数据区  77寄存器数据  历史数据
	uAmmeter.Flag_GEThistorymeter = ((Historynum<<8) + pCache[(77*2)+1]);

	//printf("电表历史数据数量 = %d \r\n",uAmmeter.Flag_GEThistorymeter);


	if(uAmmeter.UpdateMeterFlag == 1)		//服务器设置
		uAmmeter.UpdateMeterFlag = 2;		//接收到电表数据
//	 printf( "IOT_MeterReceive pbuf[2] = %d UpdateMeterFlag = %d   uAmmeter.Flag_GEThistorymeter=%d\r\n",pbuf[2], uAmmeter.UpdateMeterFlag, uAmmeter.Flag_GEThistorymeter); // 调试用

	InvCommCtrl.NoReplyCnt = 0;
	PollingSwitch();

	return 0;
}
/******************************************************************************
 * FunctionName : History_Ammeter_send
 * Description  : 电表标志、数据获取与发送
 * Parameters   : IOT_TCP_Params xx_tcp : tcp连接类型
 *                uint8_t num                : tcp编号
 * Returns      : none
 * Notice       : none
*******************************************************************************/
uint8_t IOT_History_Ammeter_DataPacket(uint8_t *outdata)
{
	/*电表数据接收完成*/
//	if(IOT_GET_Histotymeter_Flag() != 2)
//	{
//		return 1;
//	}
//	else if(IOT_GET_Histotymeter_Flag() == 2)
//	{
//		IOT_SET_Histotymeter_Flag(3);
//	}
//	printf("\r\n IOT_History_Ammeter_DataPacket Flag_history_meter = %d\r\n",uAmmeter.Flag_history_meter);
	/* 上传状态	 */
	uint16_t len = 0; 					//缓存计数
	//uint16_t dataLen = 0;				//本次发送数据总长
	//uint16_t tmpLen = 0;				//用作临时处理使用长度

	outdata[len++] = 0x00;				//
	outdata[len++] = 0x01;        		//报文编号，固定01
	outdata[len++] = 0x00;				//
	outdata[len++] = PROTOCOL_VERSION_L;  //使用的协议版本

	outdata[len++] = 0x00 ;// HIGH((29 + dataLen));
	outdata[len++] = 0xE6 ;//LOW((29 + dataLen));    //0xE6

	outdata[len++] = InvCommCtrl.Addr;    //本机编号
	outdata[len++] = 0x22;         		  //命令-上传采集器采集到的逆变器数据。

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
	outdata[len++] =0;
	outdata[len++] =0xC8;
	memcpy(&outdata[len],(const char *)uAmmeter.HistorymeterGroupData,200);// 得到特殊逆变器序列号
	len += 200;
	return len;
}


/**********************************************************************************
 * FunctionName : Ammeter_send
 * Description  : 电表标志、数据获取与发送
 * Parameters   : IOT_TCP_Params xx_tcp : tcp连接类型
 *                u8 num                : tcp编号
 * Returns      : none
 * Notice       : none
**********************************************************************************/
uint8_t IOT_Ammeter_DataPacket(uint8_t *outdata)
{
	/*电表数据接收完成*/
//	if(uAmmeter.UpdateMeterFlag != 2)
//	{
//		return 1;
//	}
//	else if(uAmmeter.UpdateMeterFlag == 2)
//	{
//		uAmmeter.UpdateMeterFlag = 3;
//	}
//	printf("\r\n IOT_Ammeter_DataPacket Flag_history_meter = %d\r\n",uAmmeter.Flag_history_meter);
	/* 上传状态 */
	uint16_t len = 0; 					//缓存计数
	//uint16_t dataLen = 0;				//本次发送数据总长
	//uint16_t tmpLen = 0;				//用作临时处理使用长度

	outdata[len++] = 0x00;				//
	outdata[len++] = 0x01;        		//报文编号，固定01
	outdata[len++] = 0x00;				//
	outdata[len++] = PROTOCOL_VERSION_L;  //使用的协议版本

	outdata[len++] = 0x00 ;// HIGH((29 + dataLen));
	outdata[len++] = 0xE6 ;//LOW((29 + dataLen));    //0xE6

	outdata[len++] = InvCommCtrl.Addr;    //本机编号
	outdata[len++] = 0x20;         		  //命令-上传采集器采集到的逆变器数据。

	memcpy(&outdata[len], &SysTAB_Parameter[Parameter_len[REG_SN]], 10);	 //采集器SN号
	len += 10;

	memcpy(&outdata[len], InvCommCtrl.SN, 10);			//逆变器SN号
	len += 10;

	outdata[len++] = sTime.data.Year     ;         		//6byte时间戳 28 29 30 31 32 33
	outdata[len++] = sTime.data.Month    ;
	outdata[len++] = sTime.data.Date     ;
	outdata[len++] = sTime.data.Hours    ;
	outdata[len++] = sTime.data.Minutes  ;
	outdata[len++] = sTime.data.Seconds  ;
	outdata[len++] = 0x00 ;
	outdata[len++] = 0xC8 ;
	memcpy(&outdata[len],(const char *)uAmmeter.MeterGroupData,200);// 得到特殊逆变器序列号
	len += 200;

	return len;
}







