/*
 * iot_InvWave.c
 *
 *  Created on: 2022年6月11日
 *      Author: Administrator
 */

#include "iot_InvWave.h"
#include "iot_inverter.h"
#include "iot_universal.h"

#include "iot_system.h"
#include "iot_protocol.h"
#include "iot_uart1.h"
#include "driver/gpio.h"
/******************************************
  *     获取逆变器波形（PV曲线、故障录波、实时波形、一键诊断、AC电压谐波、阻抗波形）
  *****************************************
  */

WaveParam Wave[TotalWaveNums] =
{
		{250,0,999,60,1},
		{259,1000,3499,5,1},
		{260,3500,5999,25,1},
		{(250+260+265+266),0,6124,(60+25+15+20),5},
		{264,3500,6124,25,1},// 264为自定义值，实际AC波形0x10寄存器设置值和实时波形一样的(260)，这里只是用来做区分
		{265,6000,6124,15,1},
		{266,6000,6124,20,1},
		{309,6000,6124,30,1},
};

uint8_t _GucSingleWaveSendTimes = 0;        			// 单次波形数据发送次数
uint8_t _GucSingleWaveRegReadSize = 0;     				// 单次读取波形数据大小
uint8_t GucInverterWaveNum = 0;             			// 逆变器波形编号
uint8_t GucInverterWaveDataNum = 0;         			// 逆变器波形数据编号

uint8_t GucInverterWaveGetFlag = 0;        			 	// 服务器获取波形标志位
uint8_t _GucInverterWaveQuantity = 0;              		// 请求波形条数（一键诊断赋值）
uint8_t _GucInverterWaveMultReqType= 0;            		// 多波形请求波形类型（一键诊断等值）

uint32_t GuiInverterWaveWaitTime = 0;       			// 获取波形等待时间（单位：5ms --根据系统定时器定）
uint32_t GuiInverterWaveDataSignleTCPAckTime = 0;  		// 波形数据单次服务器应答时间（单位：5ms --根据系统定时器定）

/* 1500v机器波形诊断   2020-05-08*/
uint32_t _g_uiNewPVWaveStartAddr[1] = {7000};      		 // LI 20200313 逆变器新P/IV曲线波形获取寄存器起始地址 ，新起始地址(7000)
uint32_t _g_uiNewPVWaveEndAddr[3] = {7999, 8999, 10999}; // LI 20200312 逆变器新P/IV曲线波形获取寄存器结束地址 ，根据实际03数据标志位(8/16/32条)确定读取波形的结束地址，新结束地址(7999/8999/10999)

uint8_t InverterWave_Judge(uint16_t Addr_Start, uint16_t Addr_End)
{
	/* 得到波形编号 */
	if((Addr_Start == Wave[PVWaveCurve].WaveType) && (Addr_End == Wave[PVWaveCurve].WaveType))
	{// pv曲线
		GucInverterWaveNum = PVWaveCurve;               // 得到波形编号
		_GucSingleWaveRegReadSize = SingleRegReadNum_1; // 得到单次读取寄存器大小值
	}
	else if((Addr_Start == Wave[FaultWave].WaveType) && (Addr_End == Wave[FaultWave].WaveType))
	{// 故障滤波
		GucInverterWaveNum = FaultWave;                 // 得到波形编号
		_GucSingleWaveRegReadSize = SingleRegReadNum_1; // 得到单次读取寄存器大小值
	}
	else if((Addr_Start == Wave[RealTimeWave].WaveType) && (Addr_End == Wave[RealTimeWave].WaveType +4))
	{// 实时录波
		GucInverterWaveNum = RealTimeWave; 							// 得到波形编号
		_GucSingleWaveRegReadSize = SingleRegReadNum_1; // 得到单次读取寄存器大小值
	}
	else if((Addr_Start == Wave[OneKeyDiagnosis].WaveType) && (Addr_End == Wave[OneKeyDiagnosis].WaveType))
	{
		// 一键诊断
//		GucInverterWaveNum = OneKeyDiagnosis; 			// 得到波形编号
//		_GucSingleWaveRegReadSize = SingleRegReadNum_1; // 得到单次读取寄存器大小值
		/* 一键诊断，第一条首先获取阻抗波形 */
		GucInverterWaveNum = Impedance; 				// 得到波形编号
		_GucSingleWaveRegReadSize = SingleRegReadNum_1; // 得到单次读取寄存器大小值

		_GucInverterWaveQuantity = Wave[OneKeyDiagnosis].WaveQuantity;// 得到要请求的波形条数
    	_GucInverterWaveMultReqType = OneKeyDiagnosis;                // 多波形请求为：一键诊断

		InvCommCtrl.SetParamStart = Wave[Impedance].WaveType;
		InvCommCtrl.SetParamEnd = Wave[Impedance].WaveType;
		InvCommCtrl.SetParamData[0] = 0x00;
		InvCommCtrl.SetParamData[1] = 0x01;

		_GucInverterWaveQuantity -=1;// 请求的波形条数自减
	}
	else if((Addr_Start == Wave[ACWave].WaveType) && (Addr_End == Wave[ACWave].WaveType))	 //一键诊断bug zhao 2020-03-23
	{// AC波型
		GucInverterWaveNum = ACWave; 				            // 得到波形编号
		_GucSingleWaveRegReadSize = SingleRegReadNum_1; // 得到单次读取寄存器大小值
	}
	else if((Addr_Start == Wave[ACHarmonic].WaveType) && (Addr_End == Wave[ACHarmonic].WaveType))
	{// AC电压谐波
		GucInverterWaveNum = ACHarmonic; 				        // 得到波形编号
		_GucSingleWaveRegReadSize = SingleRegReadNum_1; // 得到单次读取寄存器大小值
	}
	else if((Addr_Start == Wave[Impedance].WaveType) && (Addr_End == Wave[Impedance].WaveType))
	{// 阻抗波形
		GucInverterWaveNum = Impedance; 				        // 得到波形编号
		_GucSingleWaveRegReadSize = SingleRegReadNum_1; // 得到单次读取寄存器大小值
	}
	else if((Addr_Start == Wave[PVISOCheck].WaveType) && (Addr_End == Wave[PVISOCheck].WaveType))
	{// PVISO检测
		GucInverterWaveNum = PVISOCheck; 				        // 得到波形编号
		_GucSingleWaveRegReadSize = SingleRegReadNum_1; // 得到单次读取寄存器大小值
	}
	else
	{
		return 0; // 0x10命令是非波形获取请求
	}

	IOT_Printf("\r\n InverterWave_Judge() wave = %d\r\n",GucInverterWaveNum);


    InverterWaveDataNum_Count(0);          // 清空波形数据编号
	InverterWaveStatus_Judge(0,IOT_READY); // 设置获取波形状态准备

	return 1;
}



/******************************************
  *     发送0x14寄存器指令给逆变器
  *****************************************
  */
void Send_PV_0x14(uint16_t reg, uint16_t num)
{
	uint16_t wCRCchecknum = 0;
	uint8_t  com_tab[10]={0};			// 最多支持10个寄存器同时设定
	com_tab[0] = InvCommCtrl.Addr;		// 逆变器通讯地址
	com_tab[1] = 0x14;		          	// 指令
	com_tab[2] = HIGH(reg);			    // 起始寄存器H
	com_tab[3] = LOW(reg);			    // 起始寄存器L
	com_tab[4] = HIGH(num);			    // 寄存器数目H
	com_tab[5] = LOW(num);			    // 寄存器数目L

	wCRCchecknum = Modbus_Caluation_CRC16(com_tab, 6);
	com_tab[6] = wCRCchecknum&0x0ff;      //
	com_tab[7] = (wCRCchecknum>>8)&0x0ff; //

	IOT_UART1SendData(com_tab,8);

#if  1
	IOT_Printf("\r\nSend_PV_0x14() 获取逆变器波形 = ");
	for(uint8_t i=0; i<8; i++)
	{
		IOT_Printf("%02x ",com_tab[i]);
	}
#endif

}


/******************************************************************************
 * FunctionName : IOT_ESP_Wave_Dead_Delayms
 * Description  : esp 逆变器波形死亡等待
 * Parameters   : u32 nTime   : 延时时间
 * Returns      : none
 * Notice       : none
*******************************************************************************/
static bool IOT_ESP_Wave_Dead_Delayms(uint32_t nTime)
{
 static unsigned int i=0;
 if(nTime==0)
 {
	i=0;
	return 0;
 }
 if(i++>=nTime/100)
 {
	i=0;
	return 0;
 }
 vTaskDelay(100 / portTICK_PERIOD_MS);
	return 1;
}

/*****************************************************************
  *     逆变器波形状获取和发送    LI 2018.04.04
  *				none
  *				none
  *				none
  ****************************************************************
  */
void InverterWave_GetAndSend(void)
{
	uint16_t _usiRegAddrStart = 0;
	uint16_t _usiRegAddrEnd = 0;
	uint16_t usiRegValStart = 0;   // LI 20200313
	uint16_t usiRegValEnd = 0;
	uint16_t uswaitsendserver=0;
	if((InverterWaveStatus_Judge(1,0) == IOT_BUSY)&&\
		(InverterWaveDataSignleTCPAckTime_Set(1,0) == 0))// 单次波形请求服务器超时无应答
	{
		_GucSingleWaveSendTimes = 0;          // 单次波形数据发送次数清零
		InverterWaveStatus_Judge(0,IOT_FREE); // 设置波形数据获取初始化标志(0x14命令码处理会更改该标志位)
		IOT_Printf("\r\n\r\n\r\n\r\n\r\n单次波形请求服务器超时无应答 !!!\r\n\r\n\r\n\r\n\r\n");
	}

	if(InverterWaveStatus_Judge(1,0) != IOT_ING)
		return; // 服务器是否有请求波形

	if(InverterWaveWaitTime_Set(1,0))
		return;             // 等待逆变器波形准备完成

	_usiRegAddrStart = Wave[GucInverterWaveNum].RegAddrStart; // 得到波形地址
	_usiRegAddrEnd = Wave[GucInverterWaveNum].RegAddrEnd;
	//       处理完成数据清零
	Uart_Rxtype.Read_count=0 ; // 数据解析完成 清零，下次轮训
    Uart_Rxtype.Size=0;		  // 数据长度清0
	memset(RX_BUF,0, Uart_Rxtype.Size);

	if((_GucSingleWaveSendTimes <= 5) &&\
		((_usiRegAddrStart + GucInverterWaveDataNum*_GucSingleWaveRegReadSize) < _usiRegAddrEnd))
	{
		IOT_Printf("\r\n\r\n\r\n if单条波形数据！！！！！！！！！！！！！！！\r\n\r\n\r\n" );
		Send_PV_0x14((_usiRegAddrStart+(GucInverterWaveDataNum*_GucSingleWaveRegReadSize)),\
								  _GucSingleWaveRegReadSize);
		while(1)
		{
			if((5+_GucSingleWaveRegReadSize*2) == Uart_Rxtype.Size) // 接收到逆变器返回波形数据 （5-命令头 SingleRegReadNum*2-数据）
			{
				IOT_Printf(" InverterWave_GetAndSend() ok!!!\r\n");
				IOT_ESP_Wave_Dead_Delayms(0);
				break;
			}
			if(IOT_ESP_Wave_Dead_Delayms(3000) == 0)  // 超时接收不到逆变器返回波形数据
			{
				InverterWaveDataNum_Count(2); // 计算超时发送命令次数
				IOT_Printf(" InverterWave_GetAndSend() Timeout !!!\r\n");
				return;  // 下一次继续读取
			}
		}

#if 1
		IOT_Printf("\r\n InverterWave_GetAndSend() 接到PV返回的数据个数 = %d.\r\n",Uart_Rxtype.Size);
		for(uint16_t j = 0; j<Uart_Rxtype.Size; j++)
		{
			IOT_Printf("%02x ",RX_BUF[j]);
		}
		IOT_Printf("\r\n");
#endif

		/* 逆变器数据返回格式：0x01 0x14 0xfa ...(数据)...CRC_1 CRC_2 ---共255字节*/
		/* TCP数据发送时，去除0x14命令头和crc值--5字节 */
//		Replay_0x14((_usiRegAddrStart + GucInverterWaveDataNum*_GucSingleWaveRegReadSize),
//					(_usiRegAddrStart + (GucInverterWaveDataNum+1)*_GucSingleWaveRegReadSize - 1),
//					&RX_BUF[3],Uart_Rxtype.Size-5);// 发送到服务器

		memcpy(  &InvCommCtrl.GetParamDataBUF[0],&RX_BUF[3],Uart_Rxtype.Size-5);// 得到数据区数据
		InvCommCtrl.GetParamData=Uart_Rxtype.Size-5;
		InvCommCtrl.SetParamStart =(_usiRegAddrStart + GucInverterWaveDataNum*_GucSingleWaveRegReadSize);
		InvCommCtrl.SetParamEnd = (_usiRegAddrStart + (GucInverterWaveDataNum+1)*_GucSingleWaveRegReadSize - 1);

		System_state.up14dateflag=1;  //触发 发送0x14
		uswaitsendserver=10;
	 	InverterWaveStatus_Judge(0,IOT_BUSY); // 设置波形数据获取繁忙标志(0x14命令码处理会更改该标志位)
		InverterWaveDataSignleTCPAckTime_Set(0,20);// 设置波形数据等待服务器tcp应答时间
		while( uswaitsendserver -- )
		{
			IOT_ESP_Wave_Dead_Delayms(1000);
			if(System_state.up14dateflag==0)
			{
				break;
			}
		}

		Uart_Rxtype.Size = 0;
		memset(RX_BUF,0,DATA_MAX);
		memset( InvCommCtrl.GetParamDataBUF,0,500);
		InvCommCtrl.GetParamData=0;


	}
	else // 单次波形请求结束
	{
		IOT_Printf("\r\n\r\n\r\n else单条请求结束！！！！！！！！！！！！！！！\r\n\r\n\r\n" );
		_GucSingleWaveSendTimes = 0;          // 单次波形数据发送次数清零
		InverterWaveStatus_Judge(0,IOT_FREE); // 设置波形数据获取初始化标志(0x14命令码处理会更改该标志位)

		/* 判断是否有多条波形请求（一键诊断） */
		if((_usiRegAddrStart + GucInverterWaveDataNum*_GucSingleWaveRegReadSize) > _usiRegAddrEnd)
		{
			IOT_Printf("\r\n\r\n\r\n else判断是否有多条波形请求（一键诊断）！！！！！！_GucInverterWaveQuantity=%d！！！！！！！！！\r\n\r\n\r\n" ,_GucInverterWaveQuantity);
			if(_GucInverterWaveQuantity)// 是否是多条波形请求
			{

				if(_GucInverterWaveMultReqType == OneKeyDiagnosis)// 多条波形请求-一键诊断
				{
					if(_GucInverterWaveQuantity==4)// pv波形 (实时波形重合)
					{
						InvCommCtrl.SetParamStart = Wave[PVWaveCurve].WaveType;
						InvCommCtrl.SetParamEnd = Wave[PVWaveCurve].WaveType;
						InvCommCtrl.SetParamData[0] = 0x00;
						InvCommCtrl.SetParamData[1] = 0x01;

						usiRegValStart = Wave[PVWaveCurve].WaveType;
						usiRegValEnd = Wave[PVWaveCurve].WaveType;
					}
					else if(_GucInverterWaveQuantity==3)// AC电压谐波
					{
						InvCommCtrl.SetParamStart = Wave[ACHarmonic].WaveType;
						InvCommCtrl.SetParamEnd = Wave[ACHarmonic].WaveType;

						InvCommCtrl.SetParamData[0] = 0x00;
						InvCommCtrl.SetParamData[1] = 0x01;

						usiRegValStart = Wave[ACHarmonic].WaveType;
						usiRegValEnd = Wave[ACHarmonic].WaveType;
					}
					else if(_GucInverterWaveQuantity==2)// AC波形
					{
						InvCommCtrl.SetParamStart = Wave[ACWave].WaveType-4;
						InvCommCtrl.SetParamEnd = Wave[ACWave].WaveType;

						InvCommCtrl.SetParamData[0] = 0x00;
						InvCommCtrl.SetParamData[1] = 0x04;
						InvCommCtrl.SetParamData[2] = 0x00;
						InvCommCtrl.SetParamData[3] = 0x05;
						InvCommCtrl.SetParamData[4] = 0x00;
						InvCommCtrl.SetParamData[5] = 0x06;
						InvCommCtrl.SetParamData[6] = 0x00;
						InvCommCtrl.SetParamData[7] = 0x07;
						InvCommCtrl.SetParamData[8] = 0x00;
						InvCommCtrl.SetParamData[9] = 0x01;

						usiRegValStart = Wave[ACWave].WaveType;
						usiRegValEnd = Wave[ACWave].WaveType;
					}
					else if(_GucInverterWaveQuantity==1)// PVISO检测
					{
						InvCommCtrl.SetParamStart = Wave[PVISOCheck].WaveType;
						InvCommCtrl.SetParamEnd = Wave[PVISOCheck].WaveType;

						InvCommCtrl.SetParamData[0] = 0x00;
						InvCommCtrl.SetParamData[1] = 0x01;

						usiRegValStart = Wave[PVISOCheck].WaveType;
						usiRegValEnd = Wave[PVISOCheck].WaveType;
					}

					InvCommCtrl.CMDFlagGrop |= CMD_0X10;
	//				InvCommCtrl.RespondFlagGrop |= CMD_0X10;// 不需要上报0x10应答服务器
					InvCommCtrl.SetParamDataLen = (InvCommCtrl.SetParamEnd - InvCommCtrl.SetParamStart + 1) * 2;

					/* 判断获取波形  */
					if(InverterWave_Judge(usiRegValStart,usiRegValEnd))
					{

						IOT_PINGRESPSet_uc(ESP_WRITE,Mqtt_PINGTime);   // 重新装载心跳时间// 如果获取波形请求时，延时心跳包发送，直到波形请求结束。。。
						IOT_TCPDelaytimeSet_uc(ESP_WRITE,System_state.Up04dateTime+1);// 延时04数据上传
					}

				 	//SetInvOutTime_RESET();	// 重置超时时间

					_GucInverterWaveQuantity--;// 波形条数自减

					if(!_GucInverterWaveQuantity)_GucInverterWaveMultReqType = 0;
				}
			}
		}
	}
}

/*****************************************************************
  *     逆变器波形数据编号计算    LI 2018.04.04
  *		    none
  *				mode = 0 	波形数据编号清零
  *				mode = 1	波形数据编号累加
  *       mode >= 2	波形数据编号保持
  ****************************************************************
  */
uint8_t InverterWaveDataNum_Count(uint8_t mode)
{
	if(mode == 0)
	{
		GucInverterWaveDataNum = 0;
	}
	else if(mode == 1)
	{
		GucInverterWaveDataNum++;
		_GucSingleWaveSendTimes = 0;// 单次波形数据发送次数清零
	}
	else
	{
		_GucSingleWaveSendTimes++; // 计算单次波形数据发送次数（默认单笔不超过：5次）
	}

	return GucInverterWaveGetFlag;
}

/*****************************************************************
  *     逆变器波形状态判断    LI 2018.04.04
  *		    none
  *			mode = 0/2 	切换状态
  *			mode = 1	获取状态
  ****************************************************************
  */
uint8_t InverterWaveStatus_Judge(uint8_t mode, uint8_t status)
{
	if((mode == 0) || (mode == 2))
	{
		/* 逆变器的0x10命令应答完成，开始设置波形获取时间 */
		if(GucInverterWaveGetFlag == IOT_READY)
		{
		    InverterWaveWaitTime_Set(0,Wave[GucInverterWaveNum].Code0x10WaitTime);// 设置获取波形等待时间
		}
		/* 判断多波形请求，0x10应答服务器地址要和请求一致（一键诊断） */
		if(mode == 2)// 用于得到逆变器应答后，进行状态切换用
		{
			if(_GucInverterWaveMultReqType == OneKeyDiagnosis)
			{
				InvCommCtrl.SetParamStart = Wave[OneKeyDiagnosis].WaveType;
				InvCommCtrl.SetParamEnd = Wave[OneKeyDiagnosis].WaveType;
			}
		}
		GucInverterWaveGetFlag = status; 	// 设置状态
		IOT_Printf("\r\n InverterWaveStatus_Judge(%d,%d)GucInverterWaveGetFlag = %d.\r\n",mode,status,GucInverterWaveGetFlag);

	}
	else
	{

	}

	return GucInverterWaveGetFlag;
}

/******************************************************************************
 * FunctionName : IOT_INVAdditionalFunc_Handle LI 2020.01.07
 * Description  : 逆变器附加功能（波形诊断、ip/port设置）
 * Parameters   : none
 * Returns      : none
 * Notice       : none
*******************************************************************************/
void IOT_INVAdditionalFunc_Handle(void)
{
//	if(IOT_INVTpye_Get_uc())     // LI 2018.07.23 / chg LI 20201031
	{
		InverterWave_GetAndSend(); // LI 2018.04.04 获取逆变器波形并上传
	}
}

/*****************************************************************************
  *     逆变器波形获取等待时间   LI 2018.04.04
  *		    none
  *				mode = 0 	设置时间
  *				mode = 1	获取时间
					uint8_t time ：单位：秒
  ****************************************************************
  */
uint32_t InverterWaveWaitTime_Set(uint8_t mode, uint8_t time)
{
	if(mode == 0) // 设置时间
	{
		GuiInverterWaveWaitTime = time ; // 获取波形等待时间 *200
	}
	else if(mode == 1)// 获取时间
	{}

	return GuiInverterWaveWaitTime;
}


/*****************************************************************
  *     逆变器单次波形数据设置服务器应答时间设置   LI 2018.04.04
  *		    none
  *				mode = 0 	设置时间
  *				mode = 1	获取时间
			 uint8_t time ：单位：秒（默认等待：20s）
  ****************************************************************
  */
uint32_t InverterWaveDataSignleTCPAckTime_Set(uint8_t mode, uint8_t time)
{
	if(mode == 0) // 设置时间
	{
		GuiInverterWaveDataSignleTCPAckTime = time; // 获取波形等待时间*200
	}
	else if(mode == 1)// 获取时间
	{}

	return GuiInverterWaveDataSignleTCPAckTime;
}

/*****************************************************************
  *     逆变器波形等待时间计时   LI 2018.04.04
  *		    none
  *				none
  *				none
  *       注意：计时要根据定时频率来定（5ms）
  ****************************************************************
  */
void InverterWaveWaitTime_Count(void)
{
	/* 波形等待interver时间 */
	if(GuiInverterWaveWaitTime > 0)
		GuiInverterWaveWaitTime--;
	/* 波形等待TCP服务器应答时间 */
	if(GuiInverterWaveDataSignleTCPAckTime > 0)
		GuiInverterWaveDataSignleTCPAckTime--;
}



/******************************************************************************
 * FunctionName : IOT_InverterPVWaveNums_Handle_uc  LI 20200312
 * Description  : 逆变器I/PV曲线数处理
 * Parameters   : uint8_t ucTpye_tem : 函数功能操作类型   0-设置值   1-获取值
 * 				  uint8_t ucNums_tem : 设置I/PV曲线条数值
 * Returns      : 0-设置成功  !0-I/PV曲线条数
 * Notice       : 1、新逆变器组串路数由原来的8路增加到16/32路，所有I/PV曲线的地址需要加大，需要在原来的地址上面做动态调整
 *                2、在服务器数据发送的时候，需要通过更改数服协议上面的时间值(旧:实际时间/6个0  新：6个16/6个32)用来区分新旧机型的i/pv波形诊断数据
*******************************************************************************/
uint8_t IOT_InverterPVWaveNums_Handle_uc(uint8_t ucTpye_tem, uint8_t ucNums_tem)
{
	static uint8_t s_ucPVWaveNums = 0;   // I/PV曲线数 (8/16/32)

	if(ucTpye_tem == 0)
	{
		if(ucNums_tem == 8)
		{
			s_ucPVWaveNums = ucNums_tem;
			Wave[PVWaveCurve].RegAddrStart = _g_uiNewPVWaveStartAddr[0]; // 更新I/PV曲线波形获取数据结束地址
			Wave[PVWaveCurve].RegAddrEnd = _g_uiNewPVWaveEndAddr[0];     // 更新I/PV曲线波形获取数据结束地址
		}
		if(ucNums_tem == 16)
		{
			s_ucPVWaveNums = ucNums_tem;
			Wave[PVWaveCurve].RegAddrStart = _g_uiNewPVWaveStartAddr[0]; // 更新I/PV曲线波形获取数据结束地址
			Wave[PVWaveCurve].RegAddrEnd = _g_uiNewPVWaveEndAddr[1];     // 更新I/PV曲线波形获取数据结束地址
		}
		else if(ucNums_tem == 32)
		{
			s_ucPVWaveNums = ucNums_tem;
			Wave[PVWaveCurve].RegAddrStart = _g_uiNewPVWaveStartAddr[0]; // 更新I/PV曲线波形获取数据结束地址
			Wave[PVWaveCurve].RegAddrEnd = _g_uiNewPVWaveEndAddr[2];     // 更新I/PV曲线波形获取数据结束地址
		}
	}
	else if(ucTpye_tem == 1)
	{
		return s_ucPVWaveNums;
	}
	return 0;
}





/* 上传逆变器波形 LI 2018.04.04 */
uint8_t Replay_0x14(uint16_t start_addr, uint16_t end_addr, uint8_t *msg, uint16_t msg_len)
{
//	uint8_t i = 0;
//	uint16_t len = (8 + 20 + 6 + 1 + 4 + msg_len);
//	uint8_t *TxBuf   ;
//
//	TxBuf[0] = 0x00;
//	TxBuf[1] = 0x01;
//	TxBuf[2] = 0X00;
//	TxBuf[3] = PROTOCOL_VERSION_L;
//
//	TxBuf[4] = HIGH((len-6));       // 数据区长度
//	TxBuf[5] = LOW((len-6));
//
//	TxBuf[6] = InvCommCtrl.Addr;    //逆变器地址
//	TxBuf[7] = 0x14;				//功能码
//
//	memcpy(&TxBuf[8], SysTAB_Parameter[Parameter_len[REG_SN] ], 10);	// 采集器SN号
//	memcpy(&TxBuf[18], InvCommCtrl.SN, 10);	        // 逆变器SN号
//	//时间 上报1500v pv曲线条数  zhao 2020-05-09
//	for(i=0 ;i < 6 ;i++)
//	{
//		TxBuf[28 + i] = IOT_InverterPVWaveNums_Handle_uc(1,0);
//	}
//	TxBuf[34] = 0x01;// 数据段数
//	TxBuf[35] = HIGH(start_addr);			// 数据寄存器起始地址
//	TxBuf[36] = LOW(start_addr);
//	TxBuf[37] = HIGH(end_addr);  			// 数据寄存器结束地址
//	TxBuf[38] = LOW(end_addr);
//	memcpy(&TxBuf[39],(char*)msg,msg_len);// 得到数据区数据
//  printf("\r\n Replay_0x14\r\n" );
//	//IOT_Protocol_ToSever(TxBuf, len  ); // 发送信息到服务器
//	printf("\r\n Replay_0x14() 上传0x14指令 长度len = %d\r\n",len);
	return 0;
}

/******************************************************************************
 * FunctionName : IOT_System_Debug LI 2020.01.07
 * Description  : 系统调试
 * Parameters   : none
 * Returns      : none
 * Notice       : none
*******************************************************************************/
void IOT_System_DebugInvWave(void)
{
	/* 系统调试用 */
	if(gpio_get_level(GPIO_NUM_5) == 0)
	{
		InvCommCtrl.CMDFlagGrop |= CMD_0X10;
	//	InvCommCtrl.RespondFlagGrop |= CMD_0X10;
		InvCommCtrl.SetParamStart = (250+260+265+266);
		InvCommCtrl.SetParamEnd = (250+260+265+266);
		InvCommCtrl.SetParamDataLen = (InvCommCtrl.SetParamEnd - InvCommCtrl.SetParamStart + 1) * 2;
		InvCommCtrl.SetParamData[0] = 0x00;
		InvCommCtrl.SetParamData[1] = 0x01;

    	/* 判断获取波形  */
		if(InverterWave_Judge(InvCommCtrl.SetParamStart,InvCommCtrl.SetParamEnd))
		{

			IOT_PINGRESPSet_uc(ESP_WRITE,Mqtt_PINGTime);   // 重新装载心跳时间// 如果获取波形请求时，延时心跳包发送，直到波形请求结束。。。
			IOT_TCPDelaytimeSet_uc(ESP_WRITE,System_state.Up04dateTime+1);// 延时04数据上传
		}
		//SetInvOutTime_RESET();	// 重置超时时间

//		InverterWave_Judge(259,259);
//		if(InverterWaveStatus_Judge(1,0) == IOT_READY)
//		InverterWaveStatus_Judge(0,IOT_ING); // 设置波形数据开始获取进行标志 LI 2018.04.04
	}
}


