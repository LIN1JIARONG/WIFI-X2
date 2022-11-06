/*
 * iot_InvWave.c
 *
 *  Created on: 2022��6��11��
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
  *     ��ȡ��������Σ�PV���ߡ�����¼����ʵʱ���Ρ�һ����ϡ�AC��ѹг�����迹���Σ�
  *****************************************
  */

WaveParam Wave[TotalWaveNums] =
{
		{250,0,999,60,1},
		{259,1000,3499,5,1},
		{260,3500,5999,25,1},
		{(250+260+265+266),0,6124,(60+25+15+20),5},
		{264,3500,6124,25,1},// 264Ϊ�Զ���ֵ��ʵ��AC����0x10�Ĵ�������ֵ��ʵʱ����һ����(260)������ֻ������������
		{265,6000,6124,15,1},
		{266,6000,6124,20,1},
		{309,6000,6124,30,1},
};

uint8_t _GucSingleWaveSendTimes = 0;        			// ���β������ݷ��ʹ���
uint8_t _GucSingleWaveRegReadSize = 0;     				// ���ζ�ȡ�������ݴ�С
uint8_t GucInverterWaveNum = 0;             			// ��������α��
uint8_t GucInverterWaveDataNum = 0;         			// ������������ݱ��

uint8_t GucInverterWaveGetFlag = 0;        			 	// ��������ȡ���α�־λ
uint8_t _GucInverterWaveQuantity = 0;              		// ������������һ����ϸ�ֵ��
uint8_t _GucInverterWaveMultReqType= 0;            		// �ನ�����������ͣ�һ����ϵ�ֵ��

uint32_t GuiInverterWaveWaitTime = 0;       			// ��ȡ���εȴ�ʱ�䣨��λ��5ms --����ϵͳ��ʱ������
uint32_t GuiInverterWaveDataSignleTCPAckTime = 0;  		// �������ݵ��η�����Ӧ��ʱ�䣨��λ��5ms --����ϵͳ��ʱ������

/* 1500v�����������   2020-05-08*/
uint32_t _g_uiNewPVWaveStartAddr[1] = {7000};      		 // LI 20200313 �������P/IV���߲��λ�ȡ�Ĵ�����ʼ��ַ ������ʼ��ַ(7000)
uint32_t _g_uiNewPVWaveEndAddr[3] = {7999, 8999, 10999}; // LI 20200312 �������P/IV���߲��λ�ȡ�Ĵ���������ַ ������ʵ��03���ݱ�־λ(8/16/32��)ȷ����ȡ���εĽ�����ַ���½�����ַ(7999/8999/10999)

uint8_t InverterWave_Judge(uint16_t Addr_Start, uint16_t Addr_End)
{
	/* �õ����α�� */
	if((Addr_Start == Wave[PVWaveCurve].WaveType) && (Addr_End == Wave[PVWaveCurve].WaveType))
	{// pv����
		GucInverterWaveNum = PVWaveCurve;               // �õ����α��
		_GucSingleWaveRegReadSize = SingleRegReadNum_1; // �õ����ζ�ȡ�Ĵ�����Сֵ
	}
	else if((Addr_Start == Wave[FaultWave].WaveType) && (Addr_End == Wave[FaultWave].WaveType))
	{// �����˲�
		GucInverterWaveNum = FaultWave;                 // �õ����α��
		_GucSingleWaveRegReadSize = SingleRegReadNum_1; // �õ����ζ�ȡ�Ĵ�����Сֵ
	}
	else if((Addr_Start == Wave[RealTimeWave].WaveType) && (Addr_End == Wave[RealTimeWave].WaveType +4))
	{// ʵʱ¼��
		GucInverterWaveNum = RealTimeWave; 							// �õ����α��
		_GucSingleWaveRegReadSize = SingleRegReadNum_1; // �õ����ζ�ȡ�Ĵ�����Сֵ
	}
	else if((Addr_Start == Wave[OneKeyDiagnosis].WaveType) && (Addr_End == Wave[OneKeyDiagnosis].WaveType))
	{
		// һ�����
//		GucInverterWaveNum = OneKeyDiagnosis; 			// �õ����α��
//		_GucSingleWaveRegReadSize = SingleRegReadNum_1; // �õ����ζ�ȡ�Ĵ�����Сֵ
		/* һ����ϣ���һ�����Ȼ�ȡ�迹���� */
		GucInverterWaveNum = Impedance; 				// �õ����α��
		_GucSingleWaveRegReadSize = SingleRegReadNum_1; // �õ����ζ�ȡ�Ĵ�����Сֵ

		_GucInverterWaveQuantity = Wave[OneKeyDiagnosis].WaveQuantity;// �õ�Ҫ����Ĳ�������
    	_GucInverterWaveMultReqType = OneKeyDiagnosis;                // �ನ������Ϊ��һ�����

		InvCommCtrl.SetParamStart = Wave[Impedance].WaveType;
		InvCommCtrl.SetParamEnd = Wave[Impedance].WaveType;
		InvCommCtrl.SetParamData[0] = 0x00;
		InvCommCtrl.SetParamData[1] = 0x01;

		_GucInverterWaveQuantity -=1;// ����Ĳ��������Լ�
	}
	else if((Addr_Start == Wave[ACWave].WaveType) && (Addr_End == Wave[ACWave].WaveType))	 //һ�����bug zhao 2020-03-23
	{// AC����
		GucInverterWaveNum = ACWave; 				            // �õ����α��
		_GucSingleWaveRegReadSize = SingleRegReadNum_1; // �õ����ζ�ȡ�Ĵ�����Сֵ
	}
	else if((Addr_Start == Wave[ACHarmonic].WaveType) && (Addr_End == Wave[ACHarmonic].WaveType))
	{// AC��ѹг��
		GucInverterWaveNum = ACHarmonic; 				        // �õ����α��
		_GucSingleWaveRegReadSize = SingleRegReadNum_1; // �õ����ζ�ȡ�Ĵ�����Сֵ
	}
	else if((Addr_Start == Wave[Impedance].WaveType) && (Addr_End == Wave[Impedance].WaveType))
	{// �迹����
		GucInverterWaveNum = Impedance; 				        // �õ����α��
		_GucSingleWaveRegReadSize = SingleRegReadNum_1; // �õ����ζ�ȡ�Ĵ�����Сֵ
	}
	else if((Addr_Start == Wave[PVISOCheck].WaveType) && (Addr_End == Wave[PVISOCheck].WaveType))
	{// PVISO���
		GucInverterWaveNum = PVISOCheck; 				        // �õ����α��
		_GucSingleWaveRegReadSize = SingleRegReadNum_1; // �õ����ζ�ȡ�Ĵ�����Сֵ
	}
	else
	{
		return 0; // 0x10�����Ƿǲ��λ�ȡ����
	}

	IOT_Printf("\r\n InverterWave_Judge() wave = %d\r\n",GucInverterWaveNum);


    InverterWaveDataNum_Count(0);          // ��ղ������ݱ��
	InverterWaveStatus_Judge(0,IOT_READY); // ���û�ȡ����״̬׼��

	return 1;
}



/******************************************
  *     ����0x14�Ĵ���ָ��������
  *****************************************
  */
void Send_PV_0x14(uint16_t reg, uint16_t num)
{
	uint16_t wCRCchecknum = 0;
	uint8_t  com_tab[10]={0};			// ���֧��10���Ĵ���ͬʱ�趨
	com_tab[0] = InvCommCtrl.Addr;		// �����ͨѶ��ַ
	com_tab[1] = 0x14;		          	// ָ��
	com_tab[2] = HIGH(reg);			    // ��ʼ�Ĵ���H
	com_tab[3] = LOW(reg);			    // ��ʼ�Ĵ���L
	com_tab[4] = HIGH(num);			    // �Ĵ�����ĿH
	com_tab[5] = LOW(num);			    // �Ĵ�����ĿL

	wCRCchecknum = Modbus_Caluation_CRC16(com_tab, 6);
	com_tab[6] = wCRCchecknum&0x0ff;      //
	com_tab[7] = (wCRCchecknum>>8)&0x0ff; //

	IOT_UART1SendData(com_tab,8);

#if  1
	IOT_Printf("\r\nSend_PV_0x14() ��ȡ��������� = ");
	for(uint8_t i=0; i<8; i++)
	{
		IOT_Printf("%02x ",com_tab[i]);
	}
#endif

}


/******************************************************************************
 * FunctionName : IOT_ESP_Wave_Dead_Delayms
 * Description  : esp ��������������ȴ�
 * Parameters   : u32 nTime   : ��ʱʱ��
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
  *     ���������״��ȡ�ͷ���    LI 2018.04.04
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
		(InverterWaveDataSignleTCPAckTime_Set(1,0) == 0))// ���β��������������ʱ��Ӧ��
	{
		_GucSingleWaveSendTimes = 0;          // ���β������ݷ��ʹ�������
		InverterWaveStatus_Judge(0,IOT_FREE); // ���ò������ݻ�ȡ��ʼ����־(0x14�����봦�����ĸñ�־λ)
		IOT_Printf("\r\n\r\n\r\n\r\n\r\n���β��������������ʱ��Ӧ�� !!!\r\n\r\n\r\n\r\n\r\n");
	}

	if(InverterWaveStatus_Judge(1,0) != IOT_ING)
		return; // �������Ƿ���������

	if(InverterWaveWaitTime_Set(1,0))
		return;             // �ȴ����������׼�����

	_usiRegAddrStart = Wave[GucInverterWaveNum].RegAddrStart; // �õ����ε�ַ
	_usiRegAddrEnd = Wave[GucInverterWaveNum].RegAddrEnd;
	//       ���������������
	Uart_Rxtype.Read_count=0 ; // ���ݽ������ ���㣬�´���ѵ
    Uart_Rxtype.Size=0;		  // ���ݳ�����0
	memset(RX_BUF,0, Uart_Rxtype.Size);

	if((_GucSingleWaveSendTimes <= 5) &&\
		((_usiRegAddrStart + GucInverterWaveDataNum*_GucSingleWaveRegReadSize) < _usiRegAddrEnd))
	{
		IOT_Printf("\r\n\r\n\r\n if�����������ݣ�����������������������������\r\n\r\n\r\n" );
		Send_PV_0x14((_usiRegAddrStart+(GucInverterWaveDataNum*_GucSingleWaveRegReadSize)),\
								  _GucSingleWaveRegReadSize);
		while(1)
		{
			if((5+_GucSingleWaveRegReadSize*2) == Uart_Rxtype.Size) // ���յ���������ز������� ��5-����ͷ SingleRegReadNum*2-���ݣ�
			{
				IOT_Printf(" InverterWave_GetAndSend() ok!!!\r\n");
				IOT_ESP_Wave_Dead_Delayms(0);
				break;
			}
			if(IOT_ESP_Wave_Dead_Delayms(3000) == 0)  // ��ʱ���ղ�����������ز�������
			{
				InverterWaveDataNum_Count(2); // ���㳬ʱ�����������
				IOT_Printf(" InverterWave_GetAndSend() Timeout !!!\r\n");
				return;  // ��һ�μ�����ȡ
			}
		}

#if 1
		IOT_Printf("\r\n InverterWave_GetAndSend() �ӵ�PV���ص����ݸ��� = %d.\r\n",Uart_Rxtype.Size);
		for(uint16_t j = 0; j<Uart_Rxtype.Size; j++)
		{
			IOT_Printf("%02x ",RX_BUF[j]);
		}
		IOT_Printf("\r\n");
#endif

		/* ��������ݷ��ظ�ʽ��0x01 0x14 0xfa ...(����)...CRC_1 CRC_2 ---��255�ֽ�*/
		/* TCP���ݷ���ʱ��ȥ��0x14����ͷ��crcֵ--5�ֽ� */
//		Replay_0x14((_usiRegAddrStart + GucInverterWaveDataNum*_GucSingleWaveRegReadSize),
//					(_usiRegAddrStart + (GucInverterWaveDataNum+1)*_GucSingleWaveRegReadSize - 1),
//					&RX_BUF[3],Uart_Rxtype.Size-5);// ���͵�������

		memcpy(  &InvCommCtrl.GetParamDataBUF[0],&RX_BUF[3],Uart_Rxtype.Size-5);// �õ�����������
		InvCommCtrl.GetParamData=Uart_Rxtype.Size-5;
		InvCommCtrl.SetParamStart =(_usiRegAddrStart + GucInverterWaveDataNum*_GucSingleWaveRegReadSize);
		InvCommCtrl.SetParamEnd = (_usiRegAddrStart + (GucInverterWaveDataNum+1)*_GucSingleWaveRegReadSize - 1);

		System_state.up14dateflag=1;  //���� ����0x14
		uswaitsendserver=10;
	 	InverterWaveStatus_Judge(0,IOT_BUSY); // ���ò������ݻ�ȡ��æ��־(0x14�����봦�����ĸñ�־λ)
		InverterWaveDataSignleTCPAckTime_Set(0,20);// ���ò������ݵȴ�������tcpӦ��ʱ��
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
	else // ���β����������
	{
		IOT_Printf("\r\n\r\n\r\n else�����������������������������������������\r\n\r\n\r\n" );
		_GucSingleWaveSendTimes = 0;          // ���β������ݷ��ʹ�������
		InverterWaveStatus_Judge(0,IOT_FREE); // ���ò������ݻ�ȡ��ʼ����־(0x14�����봦�����ĸñ�־λ)

		/* �ж��Ƿ��ж�����������һ����ϣ� */
		if((_usiRegAddrStart + GucInverterWaveDataNum*_GucSingleWaveRegReadSize) > _usiRegAddrEnd)
		{
			IOT_Printf("\r\n\r\n\r\n else�ж��Ƿ��ж�����������һ����ϣ�������������_GucInverterWaveQuantity=%d������������������\r\n\r\n\r\n" ,_GucInverterWaveQuantity);
			if(_GucInverterWaveQuantity)// �Ƿ��Ƕ�����������
			{

				if(_GucInverterWaveMultReqType == OneKeyDiagnosis)// ������������-һ�����
				{
					if(_GucInverterWaveQuantity==4)// pv���� (ʵʱ�����غ�)
					{
						InvCommCtrl.SetParamStart = Wave[PVWaveCurve].WaveType;
						InvCommCtrl.SetParamEnd = Wave[PVWaveCurve].WaveType;
						InvCommCtrl.SetParamData[0] = 0x00;
						InvCommCtrl.SetParamData[1] = 0x01;

						usiRegValStart = Wave[PVWaveCurve].WaveType;
						usiRegValEnd = Wave[PVWaveCurve].WaveType;
					}
					else if(_GucInverterWaveQuantity==3)// AC��ѹг��
					{
						InvCommCtrl.SetParamStart = Wave[ACHarmonic].WaveType;
						InvCommCtrl.SetParamEnd = Wave[ACHarmonic].WaveType;

						InvCommCtrl.SetParamData[0] = 0x00;
						InvCommCtrl.SetParamData[1] = 0x01;

						usiRegValStart = Wave[ACHarmonic].WaveType;
						usiRegValEnd = Wave[ACHarmonic].WaveType;
					}
					else if(_GucInverterWaveQuantity==2)// AC����
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
					else if(_GucInverterWaveQuantity==1)// PVISO���
					{
						InvCommCtrl.SetParamStart = Wave[PVISOCheck].WaveType;
						InvCommCtrl.SetParamEnd = Wave[PVISOCheck].WaveType;

						InvCommCtrl.SetParamData[0] = 0x00;
						InvCommCtrl.SetParamData[1] = 0x01;

						usiRegValStart = Wave[PVISOCheck].WaveType;
						usiRegValEnd = Wave[PVISOCheck].WaveType;
					}

					InvCommCtrl.CMDFlagGrop |= CMD_0X10;
	//				InvCommCtrl.RespondFlagGrop |= CMD_0X10;// ����Ҫ�ϱ�0x10Ӧ�������
					InvCommCtrl.SetParamDataLen = (InvCommCtrl.SetParamEnd - InvCommCtrl.SetParamStart + 1) * 2;

					/* �жϻ�ȡ����  */
					if(InverterWave_Judge(usiRegValStart,usiRegValEnd))
					{

						IOT_PINGRESPSet_uc(ESP_WRITE,Mqtt_PINGTime);   // ����װ������ʱ��// �����ȡ��������ʱ����ʱ���������ͣ�ֱ�������������������
						IOT_TCPDelaytimeSet_uc(ESP_WRITE,System_state.Up04dateTime+1);// ��ʱ04�����ϴ�
					}

				 	//SetInvOutTime_RESET();	// ���ó�ʱʱ��

					_GucInverterWaveQuantity--;// ���������Լ�

					if(!_GucInverterWaveQuantity)_GucInverterWaveMultReqType = 0;
				}
			}
		}
	}
}

/*****************************************************************
  *     ������������ݱ�ż���    LI 2018.04.04
  *		    none
  *				mode = 0 	�������ݱ������
  *				mode = 1	�������ݱ���ۼ�
  *       mode >= 2	�������ݱ�ű���
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
		_GucSingleWaveSendTimes = 0;// ���β������ݷ��ʹ�������
	}
	else
	{
		_GucSingleWaveSendTimes++; // ���㵥�β������ݷ��ʹ�����Ĭ�ϵ��ʲ�������5�Σ�
	}

	return GucInverterWaveGetFlag;
}

/*****************************************************************
  *     ���������״̬�ж�    LI 2018.04.04
  *		    none
  *			mode = 0/2 	�л�״̬
  *			mode = 1	��ȡ״̬
  ****************************************************************
  */
uint8_t InverterWaveStatus_Judge(uint8_t mode, uint8_t status)
{
	if((mode == 0) || (mode == 2))
	{
		/* �������0x10����Ӧ����ɣ���ʼ���ò��λ�ȡʱ�� */
		if(GucInverterWaveGetFlag == IOT_READY)
		{
		    InverterWaveWaitTime_Set(0,Wave[GucInverterWaveNum].Code0x10WaitTime);// ���û�ȡ���εȴ�ʱ��
		}
		/* �ж϶ನ������0x10Ӧ���������ַҪ������һ�£�һ����ϣ� */
		if(mode == 2)// ���ڵõ������Ӧ��󣬽���״̬�л���
		{
			if(_GucInverterWaveMultReqType == OneKeyDiagnosis)
			{
				InvCommCtrl.SetParamStart = Wave[OneKeyDiagnosis].WaveType;
				InvCommCtrl.SetParamEnd = Wave[OneKeyDiagnosis].WaveType;
			}
		}
		GucInverterWaveGetFlag = status; 	// ����״̬
		IOT_Printf("\r\n InverterWaveStatus_Judge(%d,%d)GucInverterWaveGetFlag = %d.\r\n",mode,status,GucInverterWaveGetFlag);

	}
	else
	{

	}

	return GucInverterWaveGetFlag;
}

/******************************************************************************
 * FunctionName : IOT_INVAdditionalFunc_Handle LI 2020.01.07
 * Description  : ��������ӹ��ܣ�������ϡ�ip/port���ã�
 * Parameters   : none
 * Returns      : none
 * Notice       : none
*******************************************************************************/
void IOT_INVAdditionalFunc_Handle(void)
{
//	if(IOT_INVTpye_Get_uc())     // LI 2018.07.23 / chg LI 20201031
	{
		InverterWave_GetAndSend(); // LI 2018.04.04 ��ȡ��������β��ϴ�
	}
}

/*****************************************************************************
  *     ��������λ�ȡ�ȴ�ʱ��   LI 2018.04.04
  *		    none
  *				mode = 0 	����ʱ��
  *				mode = 1	��ȡʱ��
					uint8_t time ����λ����
  ****************************************************************
  */
uint32_t InverterWaveWaitTime_Set(uint8_t mode, uint8_t time)
{
	if(mode == 0) // ����ʱ��
	{
		GuiInverterWaveWaitTime = time ; // ��ȡ���εȴ�ʱ�� *200
	}
	else if(mode == 1)// ��ȡʱ��
	{}

	return GuiInverterWaveWaitTime;
}


/*****************************************************************
  *     ��������β����������÷�����Ӧ��ʱ������   LI 2018.04.04
  *		    none
  *				mode = 0 	����ʱ��
  *				mode = 1	��ȡʱ��
			 uint8_t time ����λ���루Ĭ�ϵȴ���20s��
  ****************************************************************
  */
uint32_t InverterWaveDataSignleTCPAckTime_Set(uint8_t mode, uint8_t time)
{
	if(mode == 0) // ����ʱ��
	{
		GuiInverterWaveDataSignleTCPAckTime = time; // ��ȡ���εȴ�ʱ��*200
	}
	else if(mode == 1)// ��ȡʱ��
	{}

	return GuiInverterWaveDataSignleTCPAckTime;
}

/*****************************************************************
  *     ��������εȴ�ʱ���ʱ   LI 2018.04.04
  *		    none
  *				none
  *				none
  *       ע�⣺��ʱҪ���ݶ�ʱƵ��������5ms��
  ****************************************************************
  */
void InverterWaveWaitTime_Count(void)
{
	/* ���εȴ�interverʱ�� */
	if(GuiInverterWaveWaitTime > 0)
		GuiInverterWaveWaitTime--;
	/* ���εȴ�TCP������Ӧ��ʱ�� */
	if(GuiInverterWaveDataSignleTCPAckTime > 0)
		GuiInverterWaveDataSignleTCPAckTime--;
}



/******************************************************************************
 * FunctionName : IOT_InverterPVWaveNums_Handle_uc  LI 20200312
 * Description  : �����I/PV����������
 * Parameters   : uint8_t ucTpye_tem : �������ܲ�������   0-����ֵ   1-��ȡֵ
 * 				  uint8_t ucNums_tem : ����I/PV��������ֵ
 * Returns      : 0-���óɹ�  !0-I/PV��������
 * Notice       : 1����������鴮·����ԭ����8·���ӵ�16/32·������I/PV���ߵĵ�ַ��Ҫ�Ӵ���Ҫ��ԭ���ĵ�ַ��������̬����
 *                2���ڷ��������ݷ��͵�ʱ����Ҫͨ����������Э�������ʱ��ֵ(��:ʵ��ʱ��/6��0  �£�6��16/6��32)���������¾ɻ��͵�i/pv�����������
*******************************************************************************/
uint8_t IOT_InverterPVWaveNums_Handle_uc(uint8_t ucTpye_tem, uint8_t ucNums_tem)
{
	static uint8_t s_ucPVWaveNums = 0;   // I/PV������ (8/16/32)

	if(ucTpye_tem == 0)
	{
		if(ucNums_tem == 8)
		{
			s_ucPVWaveNums = ucNums_tem;
			Wave[PVWaveCurve].RegAddrStart = _g_uiNewPVWaveStartAddr[0]; // ����I/PV���߲��λ�ȡ���ݽ�����ַ
			Wave[PVWaveCurve].RegAddrEnd = _g_uiNewPVWaveEndAddr[0];     // ����I/PV���߲��λ�ȡ���ݽ�����ַ
		}
		if(ucNums_tem == 16)
		{
			s_ucPVWaveNums = ucNums_tem;
			Wave[PVWaveCurve].RegAddrStart = _g_uiNewPVWaveStartAddr[0]; // ����I/PV���߲��λ�ȡ���ݽ�����ַ
			Wave[PVWaveCurve].RegAddrEnd = _g_uiNewPVWaveEndAddr[1];     // ����I/PV���߲��λ�ȡ���ݽ�����ַ
		}
		else if(ucNums_tem == 32)
		{
			s_ucPVWaveNums = ucNums_tem;
			Wave[PVWaveCurve].RegAddrStart = _g_uiNewPVWaveStartAddr[0]; // ����I/PV���߲��λ�ȡ���ݽ�����ַ
			Wave[PVWaveCurve].RegAddrEnd = _g_uiNewPVWaveEndAddr[2];     // ����I/PV���߲��λ�ȡ���ݽ�����ַ
		}
	}
	else if(ucTpye_tem == 1)
	{
		return s_ucPVWaveNums;
	}
	return 0;
}





/* �ϴ���������� LI 2018.04.04 */
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
//	TxBuf[4] = HIGH((len-6));       // ����������
//	TxBuf[5] = LOW((len-6));
//
//	TxBuf[6] = InvCommCtrl.Addr;    //�������ַ
//	TxBuf[7] = 0x14;				//������
//
//	memcpy(&TxBuf[8], SysTAB_Parameter[Parameter_len[REG_SN] ], 10);	// �ɼ���SN��
//	memcpy(&TxBuf[18], InvCommCtrl.SN, 10);	        // �����SN��
//	//ʱ�� �ϱ�1500v pv��������  zhao 2020-05-09
//	for(i=0 ;i < 6 ;i++)
//	{
//		TxBuf[28 + i] = IOT_InverterPVWaveNums_Handle_uc(1,0);
//	}
//	TxBuf[34] = 0x01;// ���ݶ���
//	TxBuf[35] = HIGH(start_addr);			// ���ݼĴ�����ʼ��ַ
//	TxBuf[36] = LOW(start_addr);
//	TxBuf[37] = HIGH(end_addr);  			// ���ݼĴ���������ַ
//	TxBuf[38] = LOW(end_addr);
//	memcpy(&TxBuf[39],(char*)msg,msg_len);// �õ�����������
//  printf("\r\n Replay_0x14\r\n" );
//	//IOT_Protocol_ToSever(TxBuf, len  ); // ������Ϣ��������
//	printf("\r\n Replay_0x14() �ϴ�0x14ָ�� ����len = %d\r\n",len);
	return 0;
}

/******************************************************************************
 * FunctionName : IOT_System_Debug LI 2020.01.07
 * Description  : ϵͳ����
 * Parameters   : none
 * Returns      : none
 * Notice       : none
*******************************************************************************/
void IOT_System_DebugInvWave(void)
{
	/* ϵͳ������ */
	if(gpio_get_level(GPIO_NUM_5) == 0)
	{
		InvCommCtrl.CMDFlagGrop |= CMD_0X10;
	//	InvCommCtrl.RespondFlagGrop |= CMD_0X10;
		InvCommCtrl.SetParamStart = (250+260+265+266);
		InvCommCtrl.SetParamEnd = (250+260+265+266);
		InvCommCtrl.SetParamDataLen = (InvCommCtrl.SetParamEnd - InvCommCtrl.SetParamStart + 1) * 2;
		InvCommCtrl.SetParamData[0] = 0x00;
		InvCommCtrl.SetParamData[1] = 0x01;

    	/* �жϻ�ȡ����  */
		if(InverterWave_Judge(InvCommCtrl.SetParamStart,InvCommCtrl.SetParamEnd))
		{

			IOT_PINGRESPSet_uc(ESP_WRITE,Mqtt_PINGTime);   // ����װ������ʱ��// �����ȡ��������ʱ����ʱ���������ͣ�ֱ�������������������
			IOT_TCPDelaytimeSet_uc(ESP_WRITE,System_state.Up04dateTime+1);// ��ʱ04�����ϴ�
		}
		//SetInvOutTime_RESET();	// ���ó�ʱʱ��

//		InverterWave_Judge(259,259);
//		if(InverterWaveStatus_Judge(1,0) == IOT_READY)
//		InverterWaveStatus_Judge(0,IOT_ING); // ���ò������ݿ�ʼ��ȡ���б�־ LI 2018.04.04
	}
}


