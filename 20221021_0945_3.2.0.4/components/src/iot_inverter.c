/*
 * iot_inverter.c
 *
 *  Created on: 2021年12月14日
 *      Author: Administrator
 */


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"
#include "iot_inverter.h"
#include "iot_universal.h"
#include "iot_uart1.h"
#include "iot_system.h"
#include "iot_mqtt.h"
#include "iot_gatts_table.h"
#include "iot_protocol.h"

#include "iot_station.h"
#include "iot_rtc.h"

#include "iot_ammeter.h"
#include "iot_bmsbdc.h"
#include "iot_InvWave.h"



INV_COMM_CTRL InvCommCtrl={0};
volatile uint8_t  _GucInvPollingStatus = 0;			// 轮序逆变器数据状态0610
volatile uint8_t  _GucInvPolling0304ACK = 0;	    // 轮序逆变器0304数据状态
static const char *INV = "INV";
enum {INV_START =0 , INV_END};       	    	//

const DTC_INDEX DTCList[] =
{
	{	SKA_1500LV,										 //	便携式电源 DTC
		2,												 // HoldGrop 03寄存器需要读取的数目
		{{0, 124}, {125, 180}, {0, 0}, {0, 0}, {0, 0}},	 // 03 寄存器分组
		0, 												 // 预留
		1,												 // InputGrop 04寄存器需要读取的数目
		{{0, 99}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},	 	 // 04 寄存器分组
	},
	{pv_mix, // mix/pv_sph4_10k/pv_spa4_10k  / chg LI 20201120
		2,{{0, 124}, {1000, 1124}, {0, 0}, {0, 0}, {0, 0},},
		0, 															//预留
		3,{{0, 124}, {1000, 1124}, {1125, 1249}, {0, 0}, {0, 0}},
	},
	{pv_max, // max
		2,{{0, 124}, {125, 249}, {0, 0}, {0, 0}, {0, 0},},
		0,
		2,{{0, 124}, {125, 249}, {0, 0}, {0, 0}, {0, 0},},
	},
	{pv_sp3000, // spc3000
		2,{{0, 44}, {45, 89}, {0, 0}, {0, 0}, {0, 0},},
		0,
		3,{{0, 44}, {45, 89}, {90, 134}, {0, 0}, {0, 0},},
	},
	{pv_sp2000, // spc2000
		2,{{0, 44}, {45, 89}, {0, 0}, {0, 0}, {0, 0},},
		0,
		3,{{0, 44}, {45, 89}, {90, 134}, {0, 0}, {0, 0},},
	},

	{pv_mtlp_us, // MTLP-US
		2,{{0, 44}, {45, 89}, {0, 0}, {0, 0}, {0, 0},},
		0,
		3,{{0, 44}, {45, 89}, {90, 134}, {0, 0}, {0, 0},},
	},
	{pv_spa, // pv_spa LI 2018.11.19 / chg LI 20201120
		2,{{0, 124}, {1000, 1124}, {0, 0}, {0, 0}, {0, 0},},
		0, 															//预留
		3,{{1000, 1124}, {1125, 1249}, {2000, 2124}, {0, 0}, {0, 0}},
	},
	{pv_mtlx, // pv_mtlx chen 20220812
		2,{{0, 124}, {3000, 3124}, {0, 0}, {0, 0}, {0, 0},},
		0, 															//预留
		2,{{3000, 3124}, {3125, 3249}, {0, 0}, {0, 0}, {0, 0}},
	},
	{pv_mtlxh, // pv_mtlxh chen 20220812
		3,{{0, 124}, {3000, 3124}, {3250, 3374}, {0, 0}, {0, 0},},
		0, 															//预留
		3,{{3000, 3124}, {3125, 3249}, {3250, 3374}, {0, 0}, {0, 0}},
	},
	{pv_max_1500v, // pv_max_1500v zhao 2020.03.28
		2,{{0, 124}, {125, 249}, {0, 0}, {0, 0}, {0, 0},},
		0, 															//预留
		3,{{0, 124}, {125, 249}, {875, 999}, {0, 0}, {0, 0}},
	},
	{pv_mod, // pv_mod zhao 2020.03.28
		3,{{0, 124}, {3000, 3124}, {3250, 3374}, {0, 0}, {0, 0},},
		0, 															//预留
		3,{{3000, 3124}, {3125, 3249}, {3250, 3374}, {0, 0}, {0, 0}},
	},
	{pv_tl_xh_us, // pv_tl_xh_us zhao 2020.03.28
		3,{{0, 124}, {3000, 3124}, {3125, 3249}, {0, 0}, {0, 0},},
		0, 															//预留
		2,{{3000, 3124}, {3125, 3249}, {0, 0}, {0, 0}, {0, 0}},
	},
	{pv_wit_100k_tl3h, // pv_wit_100k_tl3h chen 2022.09.02
		3,{{0, 124}, {125, 249}, {875, 999}, {0, 0}, {0, 0},},
		0, 															//预留
		3,{{0, 124}, {125, 249}, {8000, 8124}, {0, 0}, {0, 0}},
	},
};

/**************************************************************
  *     上电根据DTC，初始化逆变器字段的读取
***************************************************************/

uint8_t IOT_DTCInit(uint16_t tmp)
{
	uint16_t listLen = 0;
	uint16_t i = 0;
	uint16_t j = 0;
	listLen = sizeof(DTCList) / sizeof(DTCList[0]);
	for(i = 0; i < listLen; i++)
	{
		if((tmp) == (DTCList[i].DTC))
		{

			InvCommCtrl.HoldingNum = DTCList[i].HoldGropNum;
			for(j = 0; j < DTCList[i].HoldGropNum; j++)
			{
				InvCommCtrl.HoldingGrop[j][0] = DTCList[i].HoldGrop[j][0];
				InvCommCtrl.HoldingGrop[j][1] = DTCList[i].HoldGrop[j][1];
			}
			InvCommCtrl.InputNum = DTCList[i].InputGropNum;
			for(j = 0; j < DTCList[i].InputGropNum; j++)
			{
				InvCommCtrl.InputGrop[j][0] = DTCList[i].InputGrop[j][0];
				InvCommCtrl.InputGrop[j][1] = DTCList[i].InputGrop[j][1];
			}
			ESP_LOGI(INV, "\r\n********InvCommCtrl.HoldingNum=%d****************\r\n",InvCommCtrl.HoldingNum);
			ESP_LOGI(INV, "\r\n********InvCommCtrl.HoldingGrop[0][0]=%d****************\r\n",InvCommCtrl.HoldingGrop[0][0]);
			ESP_LOGI(INV, "\r\n********InvCommCtrl.HoldingGrop[0][0]=%d****************\r\n",InvCommCtrl.HoldingGrop[0][1]);
			ESP_LOGI(INV, "\r\n********InvCommCtrl.HoldingGrop[1][0]=%d****************\r\n",InvCommCtrl.HoldingGrop[1][0]);
			ESP_LOGI(INV, "\r\n********InvCommCtrl.HoldingGrop[1][1]=%d****************\r\n",InvCommCtrl.HoldingGrop[1][1]);

			ESP_LOGI(INV, "\r\n\r\n\r\n********InvCommCtrl.InputNum=%d****************\r\n",InvCommCtrl.InputNum);
			ESP_LOGI(INV, "\r\n********InvCommCtrl.InputGrop[0][0]=%d****************\r\n",InvCommCtrl.InputGrop[0][0]);
			ESP_LOGI(INV, "\r\n********InvCommCtrl.InputGrop[0][0]=%d****************\r\n",InvCommCtrl.InputGrop[0][1]);
			ESP_LOGI(INV, "\r\n********InvCommCtrl.InputGrop[1][0]=%d****************\r\n",InvCommCtrl.InputGrop[1][0]);
			ESP_LOGI(INV, "\r\n********InvCommCtrl.InputGrop[1][1]=%d****************\r\n",InvCommCtrl.InputGrop[1][1]);


			return 0;
		}
	}
	/* 默认寄存器 */
	InvCommCtrl.HoldingNum = 2;
	InvCommCtrl.HoldingGrop[0][0] = 0;
	InvCommCtrl.HoldingGrop[0][1] = 44;
	InvCommCtrl.HoldingGrop[1][0] = 45;
	InvCommCtrl.HoldingGrop[1][1] = 89;
	InvCommCtrl.InputNum = 2;
	InvCommCtrl.InputGrop[0][0] = 0;
	InvCommCtrl.InputGrop[0][1] = 44;
	InvCommCtrl.InputGrop[1][0] = 45;
	InvCommCtrl.InputGrop[1][1] = 89;

	return 0;
}

/******************************************************************************
 * FunctionName : IOT_ESP_InverterType_Get
 * Description  : esp 逆变器类型获取
 * Parameters   : IOTESPInverterParam *TypeParams : 逆变器类型参数结构体
 * Returns      : 0-fail 1-success/同步超时
 * Notice       : 获取逆变器DTC同时进行波特率同步
*******************************************************************************/
uint8_t IOT_ESP_InverterType_Get(void)
{
	uint8_t DTCGETflag=20;

	InvCommCtrl.CMDFlagGrop=0;
	System_state.Host_setflag =0;

	System_state.Host_setflag |= CMD_0X05;
	InvCommCtrl.CMDFlagGrop |= CMD_0X05;
	InvCommCtrl.SetParamStart =  43 ;
	InvCommCtrl.SetParamEnd   =  43 ;	 // send 03() 获取逆变器03寄存器参数
	//_GucInvPollingStatus =INV_END ;

	ESP_LOGI(INV ," GET DTC !!!!!!!!!!****\r\n" );
	static uint8_t baudsetflag=0;

	#if test_wifi
	#else//便携式电源
    IOT_SetUartbaud(UART_NUM_1 ,115200);
	#endif

	IOT_stoppolling();
	while(DTCGETflag--)
	{
	//	ESP_LOGI(INV, "  ****0IOT_ESP_InverterType_Get InvCommCtrl.CMDFlagGrop =%d _GucInvPollingStatus=%d\r\n "  ,InvCommCtrl.CMDFlagGrop ,_GucInvPollingStatus);

		if(System_state.Host_setflag & CMD_0X05 )   	// 服务器查询置位
		{
		   if(!(InvCommCtrl.CMDFlagGrop & CMD_0X05 ) && (InvCommCtrl.GetParamData==7) && (InvCommCtrl.GetParamDataBUF[1]==0X03) )  // 接收逆变器应答清位等待 其他线程完成数据处理
		   {
			  	ESP_LOGI(INV, "\r\n IOT_ESP_InverterType_Get LEN = %d********\r\n",InvCommCtrl.GetParamData);
				for(uint16_t i=0; i<InvCommCtrl.GetParamData; i++)
				{
					IOT_Printf("%02x ",InvCommCtrl.GetParamDataBUF[i]);
				}
				InvCommCtrl.Addr = InvCommCtrl.GetParamDataBUF[ 0];	//通讯地址
				InvCommCtrl.DTC= InvCommCtrl.GetParamDataBUF[ 3];
				InvCommCtrl.DTC= InvCommCtrl.DTC<<8 | InvCommCtrl.GetParamDataBUF[4];
				if( InvCommCtrl.DTC > 0 )
				{
					ESP_LOGI(INV ,"DTC OK****\r\n" );
					System_state.Host_setflag &= ~CMD_0X05;
					break;
				}
				else
				{
					ESP_LOGI(INV ,"DTC ERROR****\r\n" );
				}
		   }
		   else if(!(InvCommCtrl.CMDFlagGrop & CMD_0X05 ) )
		   {
				_GucInvPollingStatus =INV_END ;
			 	InvCommCtrl.CMDFlagGrop |= CMD_0X05;
			 	InvCommCtrl.SetParamStart =  43 ;
			 	InvCommCtrl.SetParamEnd   =  43 ;
		   }
		   else
		   {
//				if((InvCommCtrl.CMDFlagGrop == 0x20 ) || (InvCommCtrl.CMDFlagGrop == 0x28)) // 03 轮训或者 05
//				{
//					_GucInvPollingStatus =INV_END ;
//					InvCommCtrl.SetParamStart =  43 ;
//					InvCommCtrl.SetParamEnd   =  43 ;
//				}
		   }
		}
	#if test_wifi

	#else   //便携式电源
	    if(baudsetflag++ < 10)
	    {
	    	IOT_SetUartbaud(UART_NUM_1 ,115200);
	    }
	    else
	    {

	    	IOT_SetUartbaud(UART_NUM_1 ,9600);
	    }
	#endif
	//	ESP_LOGI(INV, "  ****1IOT_ESP_InverterType_Get InvCommCtrl.CMDFlagGrop =%d _GucInvPollingStatus=%d\r\n "  ,InvCommCtrl.CMDFlagGrop ,_GucInvPollingStatus);

	    vTaskDelay(1000 / portTICK_PERIOD_MS);

	     if( InvCommCtrl.DTC == 0xFEFE)
	     {
	    	 System_state.Host_setflag &= ~CMD_0X05;
	     	 System_state.Host_setflag &= ~ CMD_0X05;
	     	break ;
	     }
	}
#if test_wifi

#else	//便携式电源
	if(DTCGETflag==0)
	{
    	IOT_SetUartbaud(UART_NUM_1 ,9600);
	}
	else
	{
		if(InvCommCtrl.DTC == 0xFEFE)
		{
			IOT_SetUartbaud(UART_NUM_1 ,115200);
			if(System_state.wifi_onoff==0 ) //关闭
			{
				System_state.WIFI_BLEdisplay |=Displayflag_WIFION;  //写便携式显示状态
				IOT_WIFIEvenSetON() ;   	// 按键通知开启WIFI
			}
		}
	}
#endif
	/* 通过DTC确定机型,用来做初始化数据读取缓存用 */
	if(((InvCommCtrl.DTC >= pv_mix) && (InvCommCtrl.DTC <= (pv_mix+99))) ||
	((InvCommCtrl.DTC >= pv_sph4_10k) && (InvCommCtrl.DTC <= (pv_sph4_10k+49))) ||
    ((InvCommCtrl.DTC >= pv_spa4_10k) && (InvCommCtrl.DTC <= (pv_spa4_10k+49))))
	{
		InvCommCtrl.DTC = pv_mix;
	}
	else if((InvCommCtrl.DTC >= pv_max) && (InvCommCtrl.DTC <= (pv_max+99)))
	{
		InvCommCtrl.DTC = pv_max;
	}
	else if((InvCommCtrl.DTC >= pv_sp3000) && (InvCommCtrl.DTC <= (pv_sp3000+0)))
	{
		InvCommCtrl.DTC = pv_sp3000;
	}
	else if((InvCommCtrl.DTC >= pv_sp2000) && (InvCommCtrl.DTC <= (pv_sp2000+100)))
	{
		InvCommCtrl.DTC = pv_sp2000;
	}
	else if((InvCommCtrl.DTC >= pv_spa) && (InvCommCtrl.DTC <= (pv_spa+49)))
	{
		InvCommCtrl.DTC = pv_spa;
	}
	else if((InvCommCtrl.DTC >= pv_mtlx) && (InvCommCtrl.DTC <= (pv_mtlx + 99)))
	{
		InvCommCtrl.DTC = pv_mtlx;
	}
	else if((InvCommCtrl.DTC >= pv_mtlxh) && (InvCommCtrl.DTC <= (pv_mtlxh + 99)))
	{
		InvCommCtrl.DTC = pv_mtlxh;
	}
	else if((InvCommCtrl.DTC >= pv_mtlp_us) && (InvCommCtrl.DTC <= (pv_mtlp_us+0)))
	{
		InvCommCtrl.DTC = pv_mtlp_us;
	}
	else if((InvCommCtrl.DTC >= pv_max_1500v) && (InvCommCtrl.DTC <= (pv_max_1500v +99)))
	{
		InvCommCtrl.DTC = pv_max_1500v;
	}
	else if((InvCommCtrl.DTC >= pv_mod) && (InvCommCtrl.DTC <= (pv_mod+99)))
	{
		InvCommCtrl.DTC = pv_mod;
	}
	else if((InvCommCtrl.DTC >= pv_tl_xh_us) && (InvCommCtrl.DTC <= (pv_tl_xh_us+99)))
	{
		InvCommCtrl.DTC = pv_tl_xh_us;
	}
	else if((InvCommCtrl.DTC >= pv_wit_100k_tl3h) && (InvCommCtrl.DTC <= (pv_wit_100k_tl3h+99)))  //20220902  新增兼容西安储能机
	{
		InvCommCtrl.DTC =pv_wit_100k_tl3h;
	}

	IOT_DTCInit(InvCommCtrl.DTC);		//初始化 轮询的寄存器的字段


    if( InvCommCtrl.DTC == 0xFEFE)
    {

    	return 0;
    }

	return 1;
}

///******************************************************************************
// * FunctionName : IOT_ESP_Dead_Delayms
// * Description  : esp 逆变器命令参数初始化
// * Parameters   : uint32_t nTime   : 延时时间
// * Returns      : none
// * Notice       : none
//*******************************************************************************/
//static bool IOT_ESP_Dead_Delayms(uint32_t nTime)
//{
//	 static unsigned int i=0;
//	 if(nTime==0)
//	 {
//		i=0;
//		return 0;
//	 }
//	 if(i++>=nTime/100)
//	 {
//		i=0;
//		return 0;
//	 }
//     vTaskDelay(100 / portTICK_PERIOD_MS);
//     return 1;
//}

///******************************************************************************
// * FunctionName : IOT_UsartData_Check()
// * Description  : 校验串口数据是或否符合逆变器modbus协议
// * Parameters   : none
// * Returns      : none
// * Notice       : none
//*******************************************************************************/
//uint16_t IOT_UsartData_Check(uint8_t *pbuf, uint16_t len)
//{
//	uint16_t wCRCchecknum = 0;
//	uint16_t _len = len;
//	/* 校验生产上位机设置采集器参数是否符合数服协议 LI 2018.12.01 */
//	/* 00 01 00 05 00 10 01 18 31 32 33 34 35 36 37 38 39 30 00 04 01 35 -- -- */
//	/* 00 01 00 05 00 10 01 19 31 32 33 34 35 36 37 38 39 30 00 04 00 15 -- -- */
//	while(1)
//	{
//		if(((pbuf[7] == 0x18) || (pbuf[7] == 0x19)) && (_len < 90))
//		{
//			_len = (pbuf[5] + 6 + 2); // 接收结束
//			wCRCchecknum = ModbusCrc16(pbuf, _len - 2);
//			if(pbuf[_len-1] == LOW(wCRCchecknum) && pbuf[_len-2] == HIGH(wCRCchecknum)) // crc高位在前，低位在后
//			{
//				IOT_ESP_Dead_Delayms(0);
//				return _len;
//			}
//			if(!IOT_ESP_Dead_Delayms(300))  // 最长等待数据：300ms
//			{
//				break;
//			}
//		}
//		else
//		{
//			break;
//		}
//	}
//	/* 校验modbus协议 */
//	/* 00 03 06 31 32 33 34 35 36 -- -- */
//	/* 00 04 06 31 32 33 34 35 36 -- -- */
//	while(1)
//	{
//		//if(pbuf[0] == InvCommCtrl.Addr) // 逆变器地址   不校验
//		{
//			if((pbuf[1] == 0x03) || (pbuf[1] == 0x04))
//			{
//				_len = (pbuf[2] + 3 + 2); // 接收结束
//				wCRCchecknum = ModbusCrc16(pbuf, _len - 2);
//				if(pbuf[_len-2] == LOW(wCRCchecknum) && pbuf[_len-1] == HIGH(wCRCchecknum))  // crc高位在后，低位在前
//				{
//					IOT_ESP_Dead_Delayms(0);
//					return _len;
//				}
//				if(!IOT_ESP_Dead_Delayms(1000))
//				{
//					return len;
//				}
//			}
//			else
//			{
//				return len;
//			}
//		}
////		else
////		{
////		  return len;
////		}
//	}
//
//}

uint8_t RS232CommHandle(uint8_t *pUartRxbuf, uint16_t iUartRxLen)
{
 //	uint8_t *pSN,*pSN1,*pSN2;
	uint8_t *p;

	uint8_t SNArr_1[30],SNArr_2[30],SNArr_3[30],SNArr_0[10];
	uint16_t crc16 = 0;
	uint16_t len = 0;
	p = pUartRxbuf;  // 得到串口接收的数据


	if(0 == System_state.uiSystemInitStatus)	//add chen 20221018 系统未初始化完成, 不进行生产测试
	{
		return 1;
	}


	if((p[0] == 0)&& (p[1] == 1)&& (p[2] == 0x00) && ((p[3] == 7) ||(p[3] == 6)))
	{
		/* (1)数服协议版本是6.0 */
	//	if((PROTOCOL_VERSION_L == 6) || (PROTOCOL_VERSION_L == 7))
	//	{
			memset(SNArr_1,'0',30);
			memcpy(SNArr_2,"4KXXXXXXXX",10);
			memset(&SNArr_2[10],'X',20);
	//	}
	//	else
	//	{
	//		memcpy(SNArr_1,"0000000000",10);
	//		memcpy(SNArr_2,"4KXXXXXXXX",10);
	//	}
		memcpy(SNArr_3,&SysTAB_Parameter[Parameter_len[REG_SN]],Collector_Config_LenList[REG_SN] );
		/* 处理信息 */
		//time_UDPConfigMode = VALUE_TIME_UDP_CONFIG_MODE;

	    memcpy(SNArr_0,&p[8],10 );


		if( (memcmp(SNArr_0,SNArr_1,10)!=0) && (memcmp(SNArr_0,SNArr_2,10) != 0) && (memcmp(SNArr_0,SNArr_3,10) != 0) )  //相同 ==0
		{
#if DEBUG_TEST
			IOT_Printf(" SNArr_0 SNARR ERR \r\n");
			/* 打印数据,调试用 */
			IOT_Printf("RS232CommHandle = 0x%02x iUartRxLen = %d \r\n",p[7],iUartRxLen);
#endif
			return 1;
		}


		len = 6 + MERGE(p[4], p[5]);

		if( iUartRxLen!= (len + 2))
		{
#if DEBUG_TEST
			IOT_Printf(" iUartRxLen ERR len=%d\r\n",len);
			/* 打印数据,调试用 */
			IOT_Printf("RS232CommHandle = 0x%02x iUartRxLen = %d \r\n",p[7],iUartRxLen);
#endif
			return 1;
		}//		  CRC16校验
		crc16 = Modbus_Caluation_CRC16(p, len);

		if(crc16 != MERGE(p[len], p[len +1]))
		{
#if DEBUG_TEST
			IOT_Printf(" MERGE ERR getcrc16=0x%x   ,MERGE(p[len], p[len +1])=%x   \r\n",crc16 , MERGE(p[len], p[len +1]));
			/* 打印数据,调试用 */
			IOT_Printf("RS232CommHandle = 0x%02x iUartRxLen = %d \r\n",p[7],iUartRxLen);
#endif
			return 1;
		}
#if DEBUG_TEST
		IOT_Printf("date crc16 ok\r\n");
			/* 打印数据,调试用 */
		IOT_Printf("RS232CommHandle = 0x%02x iUartRxLen = %d \r\n",p[7],iUartRxLen);
#endif
		if(p[7] == 0x19) 		//服务器查询采集器参数的指令
		{
			IOT_CommTESTData_0x19(&p[0], iUartRxLen);
			memset(p,0,200);
			return 0;
		}
		else if(p[7]  == 0x18) //Server设定采集器参数
		{
#if DEBUG_TEST
			IOT_Printf("\r\n if(p[7]  == 0x18) *******date : \r\n");
			for(int i=0; i<iUartRxLen; i++)
			{
				IOT_Printf("%02x ",p[i]);
			}
		    ESP_LOGI(INV , " is  ALL \r\n " );
		    IOT_Printf("\r\n set num = %02d \r\n" , p[19+20]);
		    IOT_Printf("\r\n set value = ");
			for(int i=0; i<p[21+20]; i++)
			{
				IOT_Printf("%02x ",p[22+i+20]);
			}
		    ESP_LOGI(INV , " \r\n " );
#endif
			IOT_CommTESTData_0x18(&p[0], iUartRxLen);
			memset(p,0,200);
			return 0;
		}
		else
		{
			return 1;
		}
	}
	else //不满足数服协议最基本的条件
	{
		return 1;
	}
return 1;
}

/******************************************************************************
 * FunctionName : IOT_NewInverterSN_Check
 * Description  : esp 逆变器序列号校验
 * Parameters   : u8 *sn : 逆变器序列号
 * Returns      : 1-"16序列号空"  10-"10位码序列号" X(X>10)-"16位码序列号"
 * Notice       : 1、前面10位字符相同也是默认为无效新逆变器序列号
*******************************************************************************/
uint8_t IOT_NewInverterSN_Check(uint8_t *sn)// 30位码 逆变器序列号判断，避免误判10位为30位
{
	uint8_t i=0, j=0;
	for(i=0; i<30 ; i++) // 得到序列号连续字符有效个数
	{
		if(((sn[i] >= '0') && (sn[i] <= '9')) || ((sn[i] >= 'A') && (sn[i] <= 'Z')))
		{
			j++;      // 计算逆变器序列号的有效个数 LI 2019.11.21
		}
		else break; // 统计到非字符序列号值
	}
	if(j < 10)return 1; // "16序列号空" LI 2019.11.21

	for(i=0; i<10; i++) // 判断前面10位是否相同
	{
     if(sn[0] != sn[i])
	 {
		goto SN_Second_Check;
	 }
	}
	return 1;
SN_Second_Check:
	 for(i=0; i<(j-10); i++)// 判断后面20位是否相同
	 {
		 if(sn[10] != sn[10+i])
		 {
				return j;  // 返回序列号有效个数 >10
		 }
	 }
	return 10; // 返回序列号有效个数 10
}
//序列号地址
//30位序列号 SN(209-223)
//10位序列号 SN(23-27)
#if 0
uint8_t IOT_ESP_InverterSN_Get(void)
{
	uint8_t INV_SNlen10[10]={0};
	uint8_t INV_SNlen30[30]={0};
	uint8_t INV_LEN10=0;
	uint8_t INV_LEN30=0;
	uint8_t Breakflag=10;
	uint8_t SNaddr23flag=0;
	uint8_t SNaddr209flag=0;
	System_state.Host_setflag |= CMD_0X05;
	InvCommCtrl.CMDFlagGrop |= CMD_0X05;

#if	(test_wifi == 0)  //便携式电源
	SNaddr209flag = 1;
	INV_LEN30=1;
#endif

	InvCommCtrl.SetParamStart = 23 ;
	InvCommCtrl.SetParamEnd   = 27 ;	// send 03() 获取逆变器03寄存器参数
	_GucInvPollingStatus =INV_END ;
	ESP_LOGI(INV ,"  0!!!!!!!!!!****\r\n" );
	if(InvCommCtrl.DTC== 0xFEFE)
	{	IOT_InCMDTime_uc(ESP_WRITE,2);
	 	 System_state.Host_setflag &= ~CMD_0X05;
	 	 InvCommCtrl.CMDFlagGrop &= ~CMD_0X05;
		return 0 ;
	}
	while(Breakflag--)
	{
		IOT_stoppolling();
	    if( InvCommCtrl.DTC == 0xfefe)		//治具DTC
	    {
	    	 System_state.Host_setflag &= ~CMD_0X05;
	    	 InvCommCtrl.CMDFlagGrop &= ~CMD_0X05;
	    	 return 0;
	    }
	 	ESP_LOGI(INV ," InvCommCtrl.DTC =%d!!!!!!!!!!****\r\n",InvCommCtrl.DTC  );
		if(System_state.Host_setflag & CMD_0X05 )   	// 服务器查询置位
		{
			 if(!(InvCommCtrl.CMDFlagGrop & CMD_0X05 ) && (InvCommCtrl.GetParamData==15) && (SNaddr23flag==0 ))  // 接收逆变器应答清位等待 其他线程完成数据处理
			 {
				 IOT_Printf("\r\n IOT_ESP_InverterSN_Getlen =%d********  \r\n",InvCommCtrl.GetParamData);
				 for( uint16_t i=0; i<InvCommCtrl.GetParamData; i++)
				 {
					 IOT_Printf("%02x ",InvCommCtrl.GetParamDataBUF[i]);
				 }
				 ESP_LOGI(INV ,"****\r\n" );
				 memcpy(INV_SNlen10, &InvCommCtrl.GetParamDataBUF[3], 10);
				 INV_LEN10 = IOT_NewInverterSN_Check(INV_SNlen10);
				 ESP_LOGI(INV ,"INV_SNlen10=%s***INV_LEN10=%d*\r\n",INV_SNlen10  ,INV_LEN10 );
			//	 SNaddr23flag =1;
				 if(INV_LEN10==10)
				 {
					 SNaddr23flag =1;
				 }
				 else  if((InvCommCtrl.GetParamDataBUF[5]=0x00)&& (INV_LEN10==1)) //数据异常
				 {
					 SNaddr23flag =0;
				 }
				 else if( (Breakflag ==5 )&& (INV_LEN10==1) )
				 {
					 SNaddr23flag =1;
				 }
			//	 System_state.Host_setflag &= ~CMD_0X05;
			//	 break;
			 }
			 else if(!(InvCommCtrl.CMDFlagGrop & CMD_0X05 ) && (InvCommCtrl.GetParamData==35 ) && (SNaddr209flag==0))  // 接收逆变器应答清位等待 其他线程完成数据处理
			 {
				 IOT_Printf("\r\n IOT_ESP_InverterSN_Getlen =%d********  \r\n",InvCommCtrl.GetParamData);
				 for( uint16_t i=0; i<InvCommCtrl.GetParamData; i++)
				 {
					 IOT_Printf("%02x ",InvCommCtrl.GetParamDataBUF[i]);
				 }
				 ESP_LOGI(INV ,"  ***\r\n" );
				 memcpy(INV_SNlen30, &InvCommCtrl.GetParamDataBUF[3], 30);
				 INV_LEN30 = IOT_NewInverterSN_Check(INV_SNlen30);
				 ESP_LOGI(INV ,"INV_SNlen30=%s***INV_LEN30=%d*\r\n",INV_SNlen30  ,INV_LEN30 );
				 SNaddr209flag =1;
 			//	 System_state.Host_setflag &= ~CMD_0X05;
 			//	 break;
			}
#if test_wifi
			else  if((Breakflag<5) && (SNaddr23flag==0)) //切换读取30位的
			{
				 SNaddr23flag =1;
				 INV_LEN10=1;
			}
#endif
			if((SNaddr209flag ==1 )&&(SNaddr23flag ==1))
			{
				if(INV_LEN10!=1 && INV_LEN30!=1)
				{
					if(memcmp(INV_SNlen10,INV_SNlen30,10) == 0 )
					{
						if((INV_LEN10==10) && (INV_LEN30 ==10))
						{
							memcpy(InvCommCtrl.SN, &INV_SNlen10[0], INV_LEN10);
						}
						else
						{
							memcpy(InvCommCtrl.SN, &INV_SNlen30[0], INV_LEN30);
						}
					}
					else
					{
						if((INV_LEN10==10) )
						{
							memcpy(InvCommCtrl.SN, &INV_SNlen10[0], INV_LEN10);
						}
						else
						{
							memcpy(InvCommCtrl.SN, &INV_SNlen30[0], INV_LEN30);
						}
					}
				}
				else if((INV_LEN10 ==1 )&&( INV_LEN30!=1))
				{
					memcpy(InvCommCtrl.SN, &INV_SNlen30[0], INV_LEN30);
				}
				else if((INV_LEN10!=1) && (INV_LEN30 ==1))
				{
					memcpy(InvCommCtrl.SN, &INV_SNlen10[0], INV_LEN10);
#if	(test_wifi == 0)  //便携式电源
				    if(memcmp(INV_SNlen10,&SysTAB_Parameter[Parameter_len[REG_SN]],10)  !=0 )
					{
									IOT_SystemParameterSet(REG_SN ,(char *) &INV_SNlen10[0],10 );  //保存数据
					}
#endif
				}
				else
				{
					memcpy((uint8_t *)InvCommCtrl.SN, "INVERSNERR", 10);
				}
				ESP_LOGI(INV ,"InvCommCtrl.SN=%s****\r\n",InvCommCtrl.SN );
				System_state.Host_setflag &= ~CMD_0X05;
	 			 break;
			}
			else if(!(InvCommCtrl.CMDFlagGrop & CMD_0X05 ) )
			{
				_GucInvPollingStatus =INV_END ;
		 	 	InvCommCtrl.CMDFlagGrop |= CMD_0X05;
		 	 	if(SNaddr23flag==0)
		 	 	{
			 	 	InvCommCtrl.SetParamStart =  23 ;
				 	InvCommCtrl.SetParamEnd   =  27 ;
		 	 	}
		 	 	else
		 	 	{
		 	 		InvCommCtrl.SetParamStart =  209 ;
		 	 		InvCommCtrl.SetParamEnd   =  223 ;
		 	 	}
			}
			else
			{
				if((InvCommCtrl.CMDFlagGrop == 0x20 ) || (InvCommCtrl.CMDFlagGrop == 0x28)) // 03 轮训或者 05
				{
					_GucInvPollingStatus =INV_END ;
					if(SNaddr23flag==0)
			 	 	{
				 	 	InvCommCtrl.SetParamStart =  23 ;
					 	InvCommCtrl.SetParamEnd   =  27 ;
			 	 	}
			 	 	else
			 	 	{
			 	 		InvCommCtrl.SetParamStart =  209 ;
			 	 		InvCommCtrl.SetParamEnd   =  223 ;
			 	 	}
				}
			}
		}
	 	vTaskDelay(1000 / portTICK_PERIOD_MS);
	 	//send 03() 获取逆变器03寄存器参数
       ESP_LOGI(INV ,"  1****_GucInvPollingStatus %d  SNaddr209flag%d SNaddr23flag%d\r\n", _GucInvPollingStatus,SNaddr209flag,SNaddr23flag );
	}

	return 0;
}
#else
uint8_t IOT_ESP_InverterSN_Get(void)
{
	uint8_t INV_SNlen10[10]={0};
	uint8_t INV_SNlen30[30]={0};
	uint8_t INV_LEN10=0;
	uint8_t INV_LEN30=0;
	uint8_t Breakflag=3;
	uint8_t SNaddr23flag=0;
	uint8_t SNaddr209flag=0;
	System_state.Host_setflag |= CMD_0X05;
	InvCommCtrl.CMDFlagGrop |= CMD_0X05;

#if	(test_wifi == 0)  //便携式电源
	SNaddr209flag = 1;
	INV_LEN30=1;
#endif

	InvCommCtrl.SetParamStart = 209 ;
	InvCommCtrl.SetParamEnd   = 223;	// send 03() 获取逆变器03寄存器参数
	_GucInvPollingStatus =INV_END ;
	ESP_LOGI(INV ,"  0!!!!!!!!!!****\r\n" );
	if(InvCommCtrl.DTC== 0xFEFE)
	{	IOT_InCMDTime_uc(ESP_WRITE,2);
	 	 System_state.Host_setflag &= ~CMD_0X05;
	 	 InvCommCtrl.CMDFlagGrop &= ~CMD_0X05;
		return 0 ;
	}
	while(Breakflag--)
	{
		printf("Breakflay = %d.\r\n",Breakflag);
		printf("Before IOT_stoppolling(),System_state.Host_setflag = %d.\r\n",System_state.Host_setflag);
		IOT_stoppolling();
		printf("After IOT_stoppolling(),System_state.Host_setflag = %d.\r\n",System_state.Host_setflag);
	    if( InvCommCtrl.DTC == 0xfefe)		//治具DTC
	    {
	    	 System_state.Host_setflag &= ~CMD_0X05;
	    	 InvCommCtrl.CMDFlagGrop &= ~CMD_0X05;
	    	 return 0;
	    }
	 	ESP_LOGI(INV," InvCommCtrl.DTC =%d!!!!!!!!!!****\r\n",InvCommCtrl.DTC  );
	 	ESP_LOGI(INV,"Before server searches,InvCommCtrl.CMDFlagGrop = %d,InvCommCtrl.GetParamData = %d,SNaddr209flag = %d\r\n",InvCommCtrl.CMDFlagGrop,InvCommCtrl.GetParamData,SNaddr209flag);
	 	ESP_LOGI(INV,"Before server searches,SNaddr23flag = %d,SNaddr209flag = %d\r\n",SNaddr23flag,SNaddr209flag);
	 	if(System_state.Host_setflag & CMD_0X05 )   	// 服务器查询置位
		{

			if(!(InvCommCtrl.CMDFlagGrop & CMD_0X05 ) && (InvCommCtrl.GetParamData==35 ) && (SNaddr209flag==0))  // 接收逆变器应答清位等待 其他线程完成数据处理
				//优先级：！ > ==
			 {
				 IOT_Printf("\r\n IOT_ESP_InverterSN_Getlen =%d********  \r\n",InvCommCtrl.GetParamData);
				 for( uint16_t i=0; i<InvCommCtrl.GetParamData; i++)
				 {
					 IOT_Printf("%02x ",InvCommCtrl.GetParamDataBUF[i]);
				 }
				 ESP_LOGI(INV ,"  ***\r\n" );
				 memcpy(INV_SNlen30, &InvCommCtrl.GetParamDataBUF[3], 30);
				 INV_LEN30 = IOT_NewInverterSN_Check(INV_SNlen30);
				 ESP_LOGI(INV ,"INV_SNlen30=%s***INV_LEN30=%d*\r\n",INV_SNlen30  ,INV_LEN30 );

				 if( (Breakflag ==5 )&& (INV_LEN30 == 1) )
				 {
					 SNaddr209flag = 1;
				 }else if(INV_LEN30 >= 10)
				 {
					 SNaddr209flag =1;
				 }

			}
			else if(!(InvCommCtrl.CMDFlagGrop & CMD_0X05 ) && (InvCommCtrl.GetParamData==15) && (SNaddr23flag==0 ))  // 接收逆变器应答清位等待 其他线程完成数据处理
			{
				 IOT_Printf("\r\n IOT_ESP_InverterSN_Getlen =%d********  \r\n",InvCommCtrl.GetParamData);
				 for( uint16_t i=0; i<InvCommCtrl.GetParamData; i++)
				 {
					 IOT_Printf("%02x ",InvCommCtrl.GetParamDataBUF[i]);
				 }
				 ESP_LOGI(INV ,"****\r\n" );
				 memcpy(INV_SNlen10, &InvCommCtrl.GetParamDataBUF[3], 10);
				 INV_LEN10 = IOT_NewInverterSN_Check(INV_SNlen10);
				 ESP_LOGI(INV ,"INV_SNlen10=%s***INV_LEN10=%d*\r\n",INV_SNlen10  ,INV_LEN10 );
			//	 SNaddr23flag =1;
				 if(INV_LEN10==10)
				 {
					 SNaddr23flag =1;
				 }
				 else  if((InvCommCtrl.GetParamDataBUF[5]=0x00)&& (INV_LEN10==1)) //数据异常
				 {
					 SNaddr23flag =0;
				 }
			}
			ESP_LOGI(INV,"After server searches,InvCommCtrl.CMDFlagGrop = %d,InvCommCtrl.GetParamData = %d,SNaddr209flag = %d\r\n",InvCommCtrl.CMDFlagGrop,InvCommCtrl.GetParamData,SNaddr209flag);
			ESP_LOGI(INV,"After server searches,SNaddr23flag = %d,SNaddr209flag = %d\r\n",SNaddr23flag,SNaddr209flag);
#if test_wifi

			if((Breakflag < 5) && (SNaddr209flag==0)) //切换读取10位的
			{
				SNaddr209flag =1;
				INV_LEN30 = 1;
			}
			else if((Breakflag == 0) && (SNaddr23flag == 0) ) //切换读取10位的
			{
				 SNaddr23flag = 1;
				 INV_LEN10 = 1;
			}
#endif
			if((SNaddr209flag ==1 )&&(SNaddr23flag ==1))
			{
				if(INV_LEN10!=1 && INV_LEN30!=1)
				{
					bzero(InvCommCtrl.SN , sizeof(InvCommCtrl.SN));
					memcpy(InvCommCtrl.SN, &INV_SNlen30[0], INV_LEN30);	//若获取到10 位序列号 和30位序列号  , 优先使用30位序列号
				}
				else if((INV_LEN10 ==1 )&&( INV_LEN30!=1))	//只获取到30位序列号 , 使用30位序列号
				{
					bzero(InvCommCtrl.SN , sizeof(InvCommCtrl.SN));
					memcpy(InvCommCtrl.SN, &INV_SNlen30[0], INV_LEN30);
				}
				else if((INV_LEN10!=1) && (INV_LEN30 ==1))		//只获取到10位序列号 , 使用10位序列号
				{
					bzero(InvCommCtrl.SN , sizeof(InvCommCtrl.SN));
					memcpy(InvCommCtrl.SN, &INV_SNlen10[0], INV_LEN10);
				}
				else
				{
					bzero(InvCommCtrl.SN , sizeof(InvCommCtrl.SN));
					memcpy((uint8_t *)InvCommCtrl.SN, "INVERSNERR", 10);
				}

				ESP_LOGI(INV ,"InvCommCtrl.SN = %s ****\r\n",InvCommCtrl.SN );
				System_state.Host_setflag &= ~CMD_0X05;

	 			break;
			}
			else if(!(InvCommCtrl.CMDFlagGrop & CMD_0X05 ) )
			{
				_GucInvPollingStatus =INV_END ;
		 	 	InvCommCtrl.CMDFlagGrop |= CMD_0X05;
		 	 	if(SNaddr209flag==0)
		 	 	{
		 	 		InvCommCtrl.SetParamStart =  209 ;
					InvCommCtrl.SetParamEnd   =  223 ;
		 	 	}
		 	 	else
		 	 	{
		 	 		InvCommCtrl.SetParamStart =  23 ;
					InvCommCtrl.SetParamEnd   =  27 ;
		 	 	}
			}
			else
			{
				if((InvCommCtrl.CMDFlagGrop == 0x20 ) || (InvCommCtrl.CMDFlagGrop == 0x28)) // 03 轮训或者 05
				{
					_GucInvPollingStatus =INV_END ;
					if(SNaddr209flag == 0)
			 	 	{
						InvCommCtrl.SetParamStart =  209 ;
						InvCommCtrl.SetParamEnd   =  223 ;
			 	 	}
			 	 	else
			 	 	{
			 	 		InvCommCtrl.SetParamStart =  23 ;
						InvCommCtrl.SetParamEnd   =  27 ;
			 	 	}
				}
			}
		}

	 	vTaskDelay(1000 / portTICK_PERIOD_MS);
	 	//send 03() 获取逆变器03寄存器参数
       ESP_LOGI(INV ,"  1****_GucInvPollingStatus = %d  SNaddr209flag = %d SNaddr23flag = %d\r\n", _GucInvPollingStatus,SNaddr209flag,SNaddr23flag );
	}

	return 0;
}

#endif


void IOT_INVPollingEN(void)
{
	InvCommCtrl.CMDFlagGrop |= CMD_0X03;    //定时获取03/04数据
	_GucInvPolling0304ACK = INV_END ;

}
void IOT_stoppolling(void)
{
	InvCommCtrl.CMDFlagGrop &= ~CMD_0X03;  //定时获取03/04数据
	_GucInvPolling0304ACK = INV_END ;
}

/******************************************
  *		轮询逆变器, 指令和寄存器组切换
******************************************/
void PollingSwitch(void)
{
	if(InvCommCtrl.CMDCur == HOLDING_GROP)					//当前轮询03指令
	{
		if(++InvCommCtrl.GropCur >= InvCommCtrl.HoldingNum)
		{
			InvCommCtrl.CMDCur = INPUT_GROP;
			InvCommCtrl.GropCur = 0;
		}
	}
	else if(InvCommCtrl.CMDCur == INPUT_GROP)				//当前轮询04指令
	{
		if(++InvCommCtrl.GropCur >= InvCommCtrl.InputNum)
		{
			/* ！！！产线测试时，不做逆变器寄存器状态轮询 */
			if(InvCommCtrl.DTC == 0xFEFE)  // 注： 0xFEFE 逆变器专用DTC，写到生产测试治具中
			{
				InvCommCtrl.CMDCur = HOLDING_GROP;
				InvCommCtrl.CMDFlagGrop &= ~CMD_0X03;   /////////停止轮训

				 IOT_InCMDTime_uc(ESP_WRITE,10);  //生产治具 1分钟轮训一次


			}
			else
			{
#if test_wifi
				InvCommCtrl.CMDCur =STATUS_GROUP ;
#else	//便携式电源 不轮询电表状态
				InvCommCtrl.CMDCur = HOLDING_GROP;
#endif
			}
			InvCommCtrl.GropCur = 0;

#if test_wifi
#else		//便携式电源 轮询结束
			InvCommCtrl.CMDFlagGrop &= ~CMD_0X03;
#endif

			InvCommCtrl.SetUpInvDATAFlag = 1;
			if(System_state.Ble_state == 0 )  // 本地不在线 远程模式 ，1秒轮训
			{
#if test_wifi
//				if(InvCommCtrl.DTC== 0xFEFE)
//				{
//					IOT_InCMDTime_uc(ESP_WRITE,60);  //生产治具 1分钟轮训一次
//				}
//				else
//				{
//					IOT_InCMDTime_uc(ESP_WRITE,5);   //正常逆变器 论时间 3秒    -----5 S 是因为电表的 没轮训没算
//				}
#else
				if(InvCommCtrl.DTC== 0xFEFE)
				{
					IOT_InCMDTime_uc(ESP_WRITE,60);  //生产治具 1分钟轮训一次
				}
				else
				{
					IOT_InCMDTime_uc(ESP_WRITE,2);   //便携式2秒轮训一次
				}
#endif
			}
			else							  		// BLE APP 读取 1分钟轮训17命令，与APP 读取错开
			{
#if test_wifi
#else
				IOT_InCMDTime_uc(ESP_WRITE,60);
#endif
			}
		}
	}
#if test_wifi
	else if(InvCommCtrl.CMDCur == STATUS_GROUP)
	{
		if(++InvCommCtrl.GropCur >= InvMeter.StatusGroupNum)
		{
			InvCommCtrl.CMDCur = METER_GROUP;
			InvCommCtrl.GropCur = 0;
		}
	}
	else if(InvCommCtrl.CMDCur==METER_GROUP)
	{
		if(++InvCommCtrl.GropCur >= InvMeter.MeterGroupNum)
		{
 			if(( uAmmeter.Flag_GEThistorymeter > 0  )||(uAmmeter.uSETPV_Historymeter188==0))//77寄存器 有历史数据 轮训
			{
				InvCommCtrl.CMDCur = History_Merer_GROUP;
				InvCommCtrl.GropCur = 0;
			}
			else
			{
				InvCommCtrl.CMDCur = HOLDING_GROP;
				InvCommCtrl.GropCur = 0;

				InvCommCtrl.CMDFlagGrop &= ~CMD_0X03;   /////////停止轮训
				if(InvCommCtrl.DTC== 0xFEFE)
				{
					IOT_InCMDTime_uc(ESP_WRITE,10);  //生产治具 1分钟轮训一次
				}
				else
				{
					IOT_InCMDTime_uc(ESP_WRITE,5);   //正常逆变器 论时间 3秒    -----5 S 是因为电表的 没轮训没算
				}
			}
		}
	}
	else if(InvCommCtrl.CMDCur==History_Merer_GROUP)
	{
		if(++InvCommCtrl.GropCur >= InvMeter.MeterGroupNum)
		{
			InvCommCtrl.CMDCur = HOLDING_GROP;
			InvCommCtrl.GropCur = 0;

			InvCommCtrl.CMDFlagGrop &= ~CMD_0X03;   /////////停止轮训
			if(InvCommCtrl.DTC== 0xFEFE)
			{
				IOT_InCMDTime_uc(ESP_WRITE,10);  //生产治具 1分钟轮训一次
			}
			else
			{
				IOT_InCMDTime_uc(ESP_WRITE,5);   //正常逆变器 论时间 3秒    -----5 S 是因为电表的 没轮训没算
			}
		}
	}
#endif
	else
	{
		InvCommCtrl.CMDCur = HOLDING_GROP;
		InvCommCtrl.GropCur = 0;
		InvCommCtrl.CMDFlagGrop &= ~CMD_0X03;   /////////停止轮训
		if(InvCommCtrl.DTC== 0xFEFE)
		{
			IOT_InCMDTime_uc(ESP_WRITE,10);  //生产治具 1分钟轮训一次
		}
		else
		{
			IOT_InCMDTime_uc(ESP_WRITE,5);   //正常逆变器 论时间 3秒    -----5 S 是因为电表的 没轮训没算
		}
	}
}

/******************************************
  *     获取03h（读Holding寄存器）指令给逆变器
  *     @addr:			设备地址, 0为广播地址
  *     @start_reg: 	起始寄存器
  *		@end_reg:		结束寄存器
  *		对GRT的逆变器, 起始和结束要在45的整数倍之间, 例:0-44, 45-89, 90-134...
 ******************************************/

void IOT_GetModbusHolding_03(uint8_t addr, uint16_t start_reg, uint16_t end_reg)
{
	unsigned int wCRCchecknum=0;
	unsigned char  com_tab[8]= {0x00,0x03,0x00,0,0x00,0x2d,0x00,0x00};

	com_tab[0] = addr;
	com_tab[1] = 0x03;
	com_tab[2] = HIGH(start_reg);
	com_tab[3] = LOW(start_reg);
	com_tab[4] = HIGH((end_reg - start_reg + 1));
	com_tab[5] = LOW((end_reg - start_reg + 1));

	wCRCchecknum = Modbus_Caluation_CRC16(com_tab, 6);

	com_tab[6] = LOW(wCRCchecknum);					//需要注意的是, CRC校验是低位在前
	com_tab[7] = HIGH(wCRCchecknum);

	IOT_UART1SendData(com_tab,8);

}

/*****************************************************
  *     获取04h（读Input寄存器）指令给逆变器
  *     @addr:			设备地址, 0为广播地址
  *     @start_reg: 	起始寄存器
  *		@end_reg:		结束寄存器
  *		对GRT的逆变器, 起始和结束要在45的整数倍之间, 例:0-44, 45-89, 90-134...
  ****************************************************
  */
void IOT_GetModbusInput_04(uint8_t addr, uint16_t start_reg, uint16_t end_reg)
{
	unsigned int wCRCchecknum;
	unsigned char  com_tab[8]= {0x00,0x03,0x00,0,0x00,0x2d,0x00,0x00};

	com_tab[0] = addr;
	com_tab[1] = 0x04;
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
  *     03h指令接收处理
  ******************************************/
uint8_t IOT_HoldingReceive(uint8_t *pbuf, uint16_t len)
{
	uint16_t i = 0;
	uint16_t cacheLoc = 0;
	uint16_t *pGropNumCur = NULL;
	uint8_t *pCache = NULL;
#if test_wifi   //WIFI 采集器才判断电表状态
	/* 当前轮询的指令是否为0x03 */
	if(InvCommCtrl.CMDCur != HOLDING_GROP)
	{
		if(InvCommCtrl.CMDCur == STATUS_GROUP)
		{
		//	IOT_Printf(" (InvCommCtrl.CMDCur == STATUS_GROUP) \r\n");
			/* 传递当前轮询的参数指针 */
			pGropNumCur = (uint16_t *)&InvMeter.StatusGroup[InvCommCtrl.GropCur][0];		//当前轮序第几组
			/* 数据缓存的位置 */
			cacheLoc = ((*(pGropNumCur+1) - *pGropNumCur + 1)*(InvCommCtrl.GropCur))* 2;// LI 2017.10.14
			pCache = &uAmmeter.StatusData[cacheLoc];
			/* 缓存数据 */
			memcpy(pCache, &pbuf[3], pbuf[2]);		//数据区  赋值到 HoldingData

			InvCommCtrl.Addr = pbuf[0];				//通讯地址

			InvCommCtrl.NoReplyCnt = 0;

//180 开始 01 0203 0405 0607
			bdcdate.BDClinknum_184 = uAmmeter.StatusData[8] ;
			bdcdate.BDClinknum_184 = (bdcdate.BDClinknum_184<<8)| uAmmeter.StatusData[9] ;    //大于1 就是BDC多台并机
			bdcdate.BMSpacknum_185 = uAmmeter.StatusData[10] ;
			bdcdate.BMSpacknum_185 = (bdcdate.BMSpacknum_185<<8)| uAmmeter.StatusData[11] ;   //电池模块数量

			uAmmeter.Flag_GETPV_Historymeter188 = uAmmeter.StatusData[17];  // pom 188 --> 8*2+1   u16

		//IOT_Printf("  *************uAmmeter.Flag_GETPV_Historymeter188= %d\r\n",uAmmeter.Flag_GETPV_Historymeter188);     //调试用
			IOT_clear_Historymeter188();

		 	IOT_Send_PV_meterFlag();
			//InvMeter.StatusData[1] = 1;//zhaotest 2019-11-14
		 	//IOT_InverterPVWaveNums_Handle_uc(0, uAmmeter.StatusData[7]); //设置I/PV曲线条数值 1500v 组串机器波形 zhao 2020-05-08
			PollingSwitch();

		}
		return 1;
	}


#endif
	/* 传递当前轮询的参数指针 */
	pGropNumCur = (uint16_t *)&InvCommCtrl.HoldingGrop[InvCommCtrl.GropCur][0];	 //当前轮序第几组
	/* 数据缓存的位置 */
	//cacheLoc = ((*(pGropNumCur+1) - *pGropNumCur + 1)*(InvCommCtrl.GropCur))* 2;

	for(i=0;i<InvCommCtrl.GropCur;i++ )  //  1  2
	{
		cacheLoc += (InvCommCtrl.HoldingGrop[i][1]-InvCommCtrl.HoldingGrop[i][0]+1)*2;
	}
	if(InvCommCtrl.GropCur==0)
		cacheLoc =0;

//	IOT_Printf("\r\n----------GropCur =%d pGropNumCur[0]=%d cacheLoc=%d ---------\r\n",InvCommCtrl.GropCur,pGropNumCur[0],cacheLoc); // 调试用
	pCache = &InvCommCtrl.HoldingData[cacheLoc];

	if((pbuf == NULL) || (pCache == NULL) || (len > DATA_MAX))
	{
		return 1;
	}
//	IOT_Printf("holdingReceive() 0x03 pbuf[2] = %d cacheLoc = %d times = %d\r\n",pbuf[2],cacheLoc,InvCommCtrl.GropCur); // 调试用
//	for(i = 0; i < 3+pbuf[2]; i++)			 //
//	{
//		IOT_Printf("%02x ",pbuf[i]);
//	}
//	IOT_Printf("\r\n");
	/* 缓存数据 */
	memcpy(pCache, &pbuf[3], pbuf[2]);		//数据区  赋值到 HoldingData

	InvCommCtrl.Addr = pbuf[0];				//通讯地址
#if test_wifi
	InvCommCtrl.PVClock_Time_Tab[0]=MERGE(InvCommCtrl.HoldingData[90+2*0],InvCommCtrl.HoldingData[90+2*0+1])-2000;
	InvCommCtrl.PVClock_Time_Tab[1]=MERGE(InvCommCtrl.HoldingData[90+2*1],InvCommCtrl.HoldingData[90+2*1+1]);
	InvCommCtrl.PVClock_Time_Tab[2]=MERGE(InvCommCtrl.HoldingData[90+2*2],InvCommCtrl.HoldingData[90+2*2+1]);
	InvCommCtrl.PVClock_Time_Tab[3]=MERGE(InvCommCtrl.HoldingData[90+2*3],InvCommCtrl.HoldingData[90+2*3+1]);
	InvCommCtrl.PVClock_Time_Tab[4]=MERGE(InvCommCtrl.HoldingData[90+2*4],InvCommCtrl.HoldingData[90+2*4+1]);
	InvCommCtrl.PVClock_Time_Tab[5]=MERGE(InvCommCtrl.HoldingData[90+2*5],InvCommCtrl.HoldingData[90+2*5+1]);
//	IOT_Printf("  holdingReceive 逆变器时间：%02d-%02d-%02d %02d:%02d:%02d \r\n",
//			InvCommCtrl.PVClock_Time_Tab[0]+2000,
//			InvCommCtrl.PVClock_Time_Tab[1], 	InvCommCtrl.PVClock_Time_Tab[2],
//			InvCommCtrl.PVClock_Time_Tab[3], 	InvCommCtrl.PVClock_Time_Tab[4], 	InvCommCtrl.PVClock_Time_Tab[5]);

	//校验时间是否需要同步
	if(InvCommCtrl.HoldingGrop[0][1] < 45)
	{
		if(InvCommCtrl.GropCur == 1)
		{
			IOT_InverterTime_Check(1,0); // 校验时间
		}

	}
	else
	{
		if(InvCommCtrl.GropCur == 0)
		{
			IOT_InverterTime_Check(0,45*2);
		}


	}

#endif
// 	InvCommCtrl.DTC= InvCommCtrl.HoldingData[43*2];
// 	InvCommCtrl.DTC= InvCommCtrl.DTC<<8 | InvCommCtrl.HoldingData[43*2+1];

// 	for(i = 0; i < 256+76; i++)			//
//	{
//		printf("%02x ",InvCommCtrl.HoldingData[i]);
//	}
 //printf("  is over .HoldingData[43*2]=%x , .HoldingData[43*2+1] =%x \r\n",InvCommCtrl.HoldingData[43*2],InvCommCtrl.HoldingData[43*2+1]);
// printf("  is over InvCommCtrl.DTC =%d \r\n", InvCommCtrl.DTC);

	if(InvCommCtrl.DTC== 0xFEFE)
	{
		/* 信息中包涵逆变器序列号 */
		if((pGropNumCur[0] <= 23) && (pGropNumCur[1] >= 27))  // 旧的序列号地址
		{
			memcpy((uint8_t *)InvCommCtrl.SN, &pbuf[3 + ((23 - pGropNumCur[0]) * 2)], 10);
			for(i = 0; i < 10; i++)				//简单验证SN正确性
			{
				if((InvCommCtrl.SN[i] < '0') || (InvCommCtrl.SN[i] > 'z'))
				{
					memcpy((uint8_t *)InvCommCtrl.SN, "INVERSNERR", 10);
					break;
				}
			}
		//IOT_Printf(" InvCommCtrl.DTC = FEFE   InvCommCtrl.SN  is  =%s\r\n", InvCommCtrl.SN);
		}
	}


// 校验逆变器时间是否需要同步
	InvCommCtrl.NoReplyCnt = 0;
	PollingSwitch();

	return 0;
}

/******************************************
  *     04h指令接收处理
  *     @pbuf:	输入缓存
  *     @len: 	输入长度
  *****************************************
  */
uint8_t IOT_InputReceive(uint8_t *pbuf, uint16_t len)
{
	uint16_t *pGropNumCur = 0;
	uint16_t cacheLoc = 0;
	uint8_t *pCache = NULL;

	/* 当前轮询的指令是否为0x04 */
	if(InvCommCtrl.CMDCur != INPUT_GROP)
		return 1;

	/* 传递当前轮询的参数指针 */
	pGropNumCur = (uint16_t *)&InvCommCtrl.InputGrop[InvCommCtrl.GropCur][0];		//当前轮序第几组

	/* 数据缓存的位置 */
	cacheLoc = ((*(pGropNumCur+1) - *pGropNumCur + 1)*(InvCommCtrl.GropCur))* 2; 	// LI 2017.10.14
	pCache = &InvCommCtrl.InputData[cacheLoc];

//	IOT_Printf("inputReceive() 0x04 pbuf[2] = %d cacheLoc = %d times= %d\r\n",pbuf[2],cacheLoc,InvCommCtrl.GropCur); // 调试用

	if((pbuf == NULL) || (pCache == NULL) || (len > DATA_MAX))
	{
		return 1;
	}

	/* 缓存数据 */
	memcpy(pCache, &pbuf[3], pbuf[2]);		//数据区

	InvCommCtrl.NoReplyCnt = 0;
	PollingSwitch();

	return 0;
}

void InvAckFlagSet(uint16_t respond_code)
{
	InvCommCtrl.Set_InvRespondCode = respond_code;		//回复状态码
//	InvCommCtrl.RespondAble = 1;						//回复使能
}

uint8_t IOT_0x05Receive(uint8_t *pbuf, uint16_t len)
{
	unsigned int wCRCchecknum;
	unsigned char  lCrcBuff,hCrcBuff;
	if((len > 4) && (pbuf[1]&0x03) == 0x03) //报错情况下只有5个字节
	{
#if DEBUG_UART
		ESP_LOGI(INV, "\r\n IOT_0x05Receive*************len =%d********  \r\n",len);
		for( uint16_t i=0; i<len; i++)
		{
			IOT_Printf("%02x ",pbuf[i]);
		}
		ESP_LOGI(INV ,"  ****\r\n" );
#endif
		if(len <  PARAM_DATABUF_MAXLEN )  					//GetParamDataBUF [160]
			memcpy(InvCommCtrl.GetParamDataBUF, pbuf, len);
		else
			memcpy(InvCommCtrl.GetParamDataBUF, pbuf, PARAM_DATABUF_MAXLEN);
		InvCommCtrl.GetParamData =len ;


		if((pbuf[1]&0x80) == 0)			    //正常情况未报错
		{
			wCRCchecknum=Modbus_Caluation_CRC16(pbuf,pbuf[2]+3);
			lCrcBuff = wCRCchecknum&0x0ff;
			hCrcBuff = (wCRCchecknum>>8)&0x0ff;
			if((pbuf[(pbuf[2]+3)]==lCrcBuff)&&(pbuf[(pbuf[2]+4)]==hCrcBuff))	//校验
			{
//				if(InvCommCtrl.RespondFlagGrop & CMD_0X05)
 				InvAckFlagSet(PV_ACKOK);
				InvCommCtrl.CMDFlagGrop &= ~CMD_0X05;
			}
		}
		else if((pbuf[1]&0x80) != 0)		//逆变器报错
		{
			wCRCchecknum=Modbus_Caluation_CRC16(pbuf,3);
			lCrcBuff = wCRCchecknum&0x0ff;
			hCrcBuff = (wCRCchecknum>>8)&0x0ff;
			if((pbuf[3]==lCrcBuff)&&(pbuf[4]==hCrcBuff)) //校验
			{
//				if(InvCommCtrl.RespondFlagGrop & CMD_0X05)
 				InvAckFlagSet(PV_ACKERROR);
				InvCommCtrl.CMDFlagGrop &= ~CMD_0X05;
			}
		}
	}
	else
	{

	}
	return 0;
}

uint8_t IOT_0x06Receive(uint8_t *pbuf, uint16_t len)
{
	unsigned int wCRCchecknum;
    unsigned char lCrcBuff,hCrcBuff;

    if((len > 4) && (pbuf[1]&0x06) == 0x06)					//报错情况下只有5个字节
    {
#if DEBUG_UART
		ESP_LOGI(INV, "\r\n IOT_0x06Receive*************len =%d********  \r\n",len);
		for( uint16_t i=0; i<len; i++)
		{
			IOT_Printf("%02x ",pbuf[i]);
		}
		ESP_LOGI(INV ,"  ****\r\n" );
#endif

		if((pbuf[1]&0x80) == 0)								//正常情况未报错
		{
			wCRCchecknum = Modbus_Caluation_CRC16(pbuf,6);
			lCrcBuff = wCRCchecknum&0x0ff;
			hCrcBuff = (wCRCchecknum>>8)&0x0ff;
			if((pbuf[6]==lCrcBuff)&&(pbuf[7]==hCrcBuff))	//校验
			{
//				if(InvCommCtrl.RespondFlagGrop & CMD_0X06)  //避免同步逆变器时间时发送0x06命令
 				InvAckFlagSet(PV_ACKOK);
				InvCommCtrl.CMDFlagGrop &= ~CMD_0X06;
			}
		}
		else if((pbuf[1]&0x80) != 0)			//逆变器报错
		{
			wCRCchecknum = Modbus_Caluation_CRC16(pbuf,3);
			lCrcBuff = wCRCchecknum&0x0ff;
			hCrcBuff = (wCRCchecknum>>8)&0x0ff;
			if((pbuf[3]==lCrcBuff)&&(pbuf[4]==hCrcBuff))	//校验
			{
//				if(InvCommCtrl.RespondFlagGrop & CMD_0X06)
 				InvAckFlagSet(PV_ACKERROR);
				InvCommCtrl.CMDFlagGrop &= ~CMD_0X06;
			}
		}
//		else if(InvCommCtrl.RespondFlagGrop & CMD_0X06)
//		InvAckFlagSet(PV_ACKERROR);

    }
	else				//无响应
	{
//		if(InvCommCtrl.RespondFlagGrop & CMD_0X06)
//		 InvAckFlagSet(SERVER_SEND_PV_OUTTIME);
	}

	return 0;
}

uint8_t IOT_0x10Receive(uint8_t *pbuf, uint16_t len)
{
    unsigned int wCRCchecknum;
    unsigned char lCrcBuff,hCrcBuff;
	if((len > 4) && (pbuf[1]&0x10) == 0x10)						//报错情况下只有5个字节
	{
#if DEBUG_UART
		ESP_LOGI(INV, "\r\n IOT_0x10Receive*************len =%d********  \r\n",len);
			for( uint16_t i=0; i<len; i++)
			{
				IOT_Printf("%02x ",pbuf[i]);
			}
		ESP_LOGI(INV ,"  ****\r\n" );
#endif
		if((pbuf[1]&0x80) == 0)			//正常情况未报错
		{
			wCRCchecknum=Modbus_Caluation_CRC16(pbuf,6);
			lCrcBuff = wCRCchecknum&0x0ff;
			hCrcBuff = (wCRCchecknum>>8)&0x0ff;
			if((pbuf[6]==lCrcBuff)&&(pbuf[7]==hCrcBuff))	//校验
			{
//				if(InvCommCtrl.RespondFlagGrop & CMD_0X10)
 				InvAckFlagSet(PV_ACKOK);
				InvCommCtrl.CMDFlagGrop &= ~CMD_0X10;

				if(InverterWaveStatus_Judge(1,0) == IOT_READY)
				  InverterWaveStatus_Judge(2,IOT_ING); 			//  设置波形数据开始获取进行标志


			}
		}
		else if((pbuf[1]&0x80) != 0)						//逆变器报错
		{
			wCRCchecknum=Modbus_Caluation_CRC16(pbuf,3);
			lCrcBuff = wCRCchecknum&0x0ff;
			hCrcBuff = (wCRCchecknum>>8)&0x0ff;
			if((pbuf[3]==lCrcBuff)&&(pbuf[4]==hCrcBuff))	//校验
			{
//				if(InvCommCtrl.RespondFlagGrop & CMD_0X10)
 				InvAckFlagSet(PV_ACKERROR);
				InvCommCtrl.CMDFlagGrop &= ~CMD_0X10;

				if(InverterWaveStatus_Judge(1,0) == IOT_READY)    //  一键诊断失败 不发数据
 				  InverterWaveStatus_Judge(2,IOT_FREE);

			}
		}

		InvCommCtrl.ContrastInvTimeFlag = 0;
	}
	else
	{
	  //PV_ReturnStatus.sCMD_0x10 = SERVER_SEND_PV_OUTTIME;
	}

	return 0;
}

uint8_t IOT_0x17Receive(uint8_t *pbuf, uint16_t len)
{
	if( len>500 )
	  len=500;
	InvCommCtrl.GetParamData =len ;

	memset(&InvCommCtrl.GetParamDataBUF, 0, 500);
	memcpy(InvCommCtrl.GetParamDataBUF, &pbuf[0],len);
	InvCommCtrl.CMDFlagGrop &= ~CMD_0X17;

#if DEBUG_MQTT
	ESP_LOGI(INV, "\r\n IOT_0x17Receive*************len =%d********  \r\n",len);
		for( uint16_t i=0; i<len; i++)
		{
			IOT_Printf("%02x ",InvCommCtrl.GetParamDataBUF[i]);
		}
	ESP_LOGI(INV ,"  ****\r\n" );
#endif
	IOT_GattsEvenSetR();
	if(InvCommCtrl.GetParamDataBUF[1]==0x06) 				// APP 设置 同步触发 读取上报服务器
	{
		IOT_SetUpHolddate();
//		IOT_InCMDTime_uc(ESP_WRITE,2);          			// 轮训 03 参数
//		InvCommCtrl.HoldingSentFlag = 1 ;  					// 主动上报， 设置参数，同步上传 03数据 //优化 等待3秒再上传
//		IOT_TCPDelaytimeSet_uc(ESP_WRITE,Mqtt_03DATATime);  // 重新赋值 03 数据上报时间
//		InvCommCtrl.SetUpInvDATAFlag =0;
	}
	return 0;
}


/******************************************
  *     逆变器时间校验
  *****************************************
  */
uint8_t IOT_InverterTime_Check(uint8_t num,uint8_t addr)
{

	uint8_t i = 0;

	uint8_t ucCmpTimeErrorFlag = 0;           // 比较时间出错标志位
	static uint8_t sucTimeSyncErrorTimes = 0; // 时间同步比较出错次数

	// 得到逆变器时间
//	InvCommCtrl.PVClock_Time_Tab[0];
//	 sTime.Clock_Time_Tab[0];
	/* 读取RTC */

	/* 对比年月日,时 */
	for(i = 0; i < 4; i++)
	{
		if(InvCommCtrl.PVClock_Time_Tab[i] !=  sTime.Clock_Time_Tab[i])
		{
			ucCmpTimeErrorFlag++; // 逆变器时间和采集器时间比较出错状态
		}
	}
	/* 对比分钟数 */
	if(InvCommCtrl.PVClock_Time_Tab[4] >  sTime.Clock_Time_Tab[4])
	{
		if(InvCommCtrl.PVClock_Time_Tab[4] -  sTime.Clock_Time_Tab[4] > 5) // 相差5分钟--逆变器时间快
		{
			ucCmpTimeErrorFlag++;
		}
	}
	else if(InvCommCtrl.PVClock_Time_Tab[4] <  sTime.Clock_Time_Tab[4])
	{
		if( sTime.Clock_Time_Tab[4] - InvCommCtrl.PVClock_Time_Tab[4] > 5) // 相差5分钟--采集器时间快
		{
			ucCmpTimeErrorFlag++;
		}
	}

	if(ucCmpTimeErrorFlag != 0) // 说明逆变器时间和采集器时间对应不上
	{
		IOT_Printf("逆变器时间超过5分钟误差 , 累计次数 %d/%d \r\n" ,sucTimeSyncErrorTimes ,  10*15);
		sucTimeSyncErrorTimes++;

		if(sucTimeSyncErrorTimes >= (10*15))  // 10/12分钟（15*4/5=60/75s）
		{
			sucTimeSyncErrorTimes = 0;
			System_state.UploadParam_Flag =  1 ;	//上报19 包含31寄存器
			IOT_Printf("\r\n ***IOT_InverterTime_Check() 逆变器和采集器时间多次对应不上，触发同步服务器时间机制***\r\n");
		}
	}
	else                       // 说明逆变器时间和采集器时间一致
	{
		sucTimeSyncErrorTimes = 0;
	}

	/* 是否需要同步逆变器时间 */
	if(InvCommCtrl.ContrastInvTimeFlag == 1)
	{
		IOT_ESP_RTCGetTime();		//20220720  chen 同步逆变器时间前 获取一次RTC时间
		sucTimeSyncErrorTimes = 0;
		InvCommCtrl.UpdateInvTimeFlag =1;
		IOT_SetInvTime0x10(45,50,sTime.Clock_Time_Tab);   				//同步 逆变器时间 0x10 命令码


		IOT_Printf("Inverter time: %02d %02d %02d %02d %02d %02d\r\n",InvCommCtrl.PVClock_Time_Tab[0],InvCommCtrl.PVClock_Time_Tab[1],InvCommCtrl.PVClock_Time_Tab[2],\
				InvCommCtrl.PVClock_Time_Tab[3],InvCommCtrl.PVClock_Time_Tab[4],InvCommCtrl.PVClock_Time_Tab[5]);

		IOT_Printf("RTC time: %02d %02d %02d %02d %02d %02d\r\n",sTime.Clock_Time_Tab[0],sTime.Clock_Time_Tab[1],sTime.Clock_Time_Tab[2],\
				sTime.Clock_Time_Tab[3],sTime.Clock_Time_Tab[4],sTime.Clock_Time_Tab[5]);

		IOT_Printf("\r\n ***IOT_InverterTime_Check() 系统开始触发同步逆变器时间***\r\n");

	}

//	IOT_Printf("\r\n ***IOT_InverterTime_Check()  ContrastInvTimeFlag=%d ********sucTimeSyncErrorTimes=%d**\r\n",InvCommCtrl.ContrastInvTimeFlag,sucTimeSyncErrorTimes);


    return 0;
}


uint8_t IOT_ModbusReceiveHandle(uint8_t *pUartRxbuf, uint16_t iUartRxLen)
{

	static uint8_t PV_receiveok=0;
	unsigned int wCRCchecknum;
	static uint8_t uERR_Returnnum=0;
	uint8_t uCMD_date=0;

	uCMD_date=pUartRxbuf[1]&0x7F; 				   //&~0x80 ;

	//IOT_UsartData_Check(pUartRxbuf,iUartRxLen);  //CRC 校验，好像没什么用
	 if(RS232CommHandle(&pUartRxbuf[0] ,iUartRxLen )==0)
	 {
		 InvCommCtrl.DTC = 0xFEFE ;
		 if(InvCommCtrl.DTC == 0xFEFE)
		   IOT_SetUartbaud(UART_NUM_1 ,115200);
		 return 0;
	 }
 // ESP_LOGI(INV, "  ***** uCMD_date =%d  InvCommCtrl.CMDFlagGrop =%d _GucInvPollingStatus=%d   iUartRxLen=%d   \r\n " ,uCMD_date ,InvCommCtrl.CMDFlagGrop ,_GucInvPollingStatus,iUartRxLen);
	 switch(uCMD_date )  //
	 {
	 	 case 03:
	 		if((InvCommCtrl.CMDFlagGrop & CMD_0X05) &&	(_GucInvPollingStatus == INV_START))
	 		{
//	 			_GucInvPollingStatus = INV_END;
	 			IOT_0x05Receive(pUartRxbuf,iUartRxLen);
	 			IOT_Printf(" IOT_ModbusReceiveHandle IOT_0x05Receive iUartRxLen=%d \r\n",iUartRxLen);
	 		}
	 		break;
	 	 case 06:
	 		if((InvCommCtrl.CMDFlagGrop & CMD_0X06) &&	(_GucInvPollingStatus == INV_START))
	 			{
//	 				_GucInvPollingStatus = INV_END;
	 				IOT_0x06Receive(pUartRxbuf,iUartRxLen);
	 				IOT_Printf(" IOT_ModbusReceiveHandle IOT_0x06Receive iUartRxLen=%d \r\n",iUartRxLen);
	 			}
	 		 break;
	 	 case 16:
	 		if((InvCommCtrl.CMDFlagGrop & CMD_0X10) &&	(_GucInvPollingStatus == INV_START))
			{
//	 			_GucInvPollingStatus = INV_END;
				IOT_0x10Receive(pUartRxbuf,iUartRxLen);
				IOT_Printf(" IOT_ModbusReceiveHandle IOT_0x10Receive iUartRxLen=%d \r\n",iUartRxLen);
			}
	 		 break;
	 	 case 04:
	 		 break;
	 	default:
	 		break;
	 }
	/* 长度错误 */
//	if((InvCommCtrl.CMDFlagGrop & CMD_0X05) &&	(_GucInvPollingStatus == INV_START))
//	{
//		_GucInvPollingStatus = INV_END;
//		IOT_0x05Receive(pUartRxbuf,iUartRxLen);
//		IOT_Printf(" IOT_ModbusReceiveHandle IOT_0x05Receive iUartRxLen=%d \r\n",iUartRxLen);
//	}
//	else if((InvCommCtrl.CMDFlagGrop & CMD_0X06) &&	(_GucInvPollingStatus == INV_START))
//	{
//		_GucInvPollingStatus = INV_END;
//		IOT_0x06Receive(pUartRxbuf,iUartRxLen);
//	}
//	else if((InvCommCtrl.CMDFlagGrop & CMD_0X10) &&	(_GucInvPollingStatus == INV_START))
//	{
//		_GucInvPollingStatus = INV_END;
//		IOT_0x10Receive(pUartRxbuf,iUartRxLen);
//	}
	if((InvCommCtrl.CMDFlagGrop & CMD_0X17) &&	(_GucInvPollingStatus == INV_START))
	{
		_GucInvPollingStatus = INV_END;
		IOT_0x17Receive(pUartRxbuf,iUartRxLen);
	 //	IOT_Printf("\r\n********IOT_0x17Receive iUartRxLen=%d****************\r\n",iUartRxLen);
	}
	else
	{
		if(iUartRxLen < 5)	 //某些指令, 报错情况下只有5个字节
			return 1;
		/* 长度错误 */
		if(iUartRxLen != (pUartRxbuf[2] + 5))
		{
			if(uERR_Returnnum++>=2)
			{
				PollingSwitch();
				uERR_Returnnum=0;
			}
			_GucInvPollingStatus  = INV_END;
			_GucInvPolling0304ACK = INV_END;
		//	ESP_LOGI(INV, "  ***** date err !!!! (iUartRxLen %d != (pUartRxbuf[2] + 5) %d ) *********PollingSwitch++******\r\n " ,iUartRxLen ,(pUartRxbuf[2] + 5) );
			return 1;
		}
//		IOT_InCMDTime_uc(ESP_WRITE,2);
//		InvCommCtrl.CMDFlagGrop |= CMD_0X03;   //定时获取03/04数据
		/* 地址验证通过 */
		if((InvCommCtrl.Addr == 0) || (InvCommCtrl.Addr == pUartRxbuf[0]))
		{
		/*************CRC16验证通过***************/
		wCRCchecknum = Modbus_Caluation_CRC16(pUartRxbuf, iUartRxLen - 2);
		if(pUartRxbuf[iUartRxLen-2] == LOW(wCRCchecknum) && pUartRxbuf[iUartRxLen-1] == HIGH(wCRCchecknum))
		{
			System_state.Pv_Senderrflag = 0 ;
			switch(pUartRxbuf[1])
			{
				case 0x03:
				{
					if(InvCommCtrl.DTC !=0 &&  PV_receiveok++>= 4 )
					{
						PV_receiveok=0;
						System_state.Pv_state=1;
					}
					IOT_HoldingReceive(pUartRxbuf, iUartRxLen);
 					_GucInvPollingStatus = INV_END;			 // 轮序结束
 					_GucInvPolling0304ACK = INV_END;
					break;
				}
				case 0x04:
				{
					IOT_InputReceive(pUartRxbuf, iUartRxLen);
 					_GucInvPollingStatus = INV_END;			// 轮序结束
 					_GucInvPolling0304ACK = INV_END;
					break;
				}
				case 0x06:
				{
					break;
				}
				case 0x10:
				{
					break;
				}
				case 0x20:
				{
					IOT_MeterReceive(pUartRxbuf,iUartRxLen);
 					_GucInvPollingStatus = INV_END;
 					_GucInvPolling0304ACK = INV_END;
					break;
				}
				case 0x22:   //读取电表历史数据
				{
					IOT_HistorymeterReceive(pUartRxbuf,iUartRxLen);
 					_GucInvPollingStatus = INV_END;
 					_GucInvPolling0304ACK = INV_END;
					break;
				}
				default:
					if(pUartRxbuf[1] &0x80 )  // ACK 错误
					{
						PollingSwitch();
						_GucInvPollingStatus = INV_END;
	 					_GucInvPolling0304ACK = INV_END;
					}
					break;
			}
		 }
		}
	}
	return 0;
}

/******************************************
  *     发送03命令码
  *     服务器  0x05 命令 查询指令
  *****************************************
*/
void IOT_SetModbusdata_0x03(unsigned char addr, uint16_t start_reg, uint16_t end_reg)
{
	unsigned int wCRCchecknum;
	unsigned char  lCrcBuff,hCrcBuff;
	unsigned char  com_tab[8]={0x00,0x03,0x00,0x00,0x00,0x2d,0x00,0x00};
	unsigned char num = 0;

	num = (end_reg - start_reg + 1);
	com_tab[0] = addr;         				//通讯地址
	com_tab[1] = 0x03;		 				//指令
	com_tab[2] = HIGH(start_reg);			//寄存器H
	com_tab[3] = LOW(start_reg);			//寄存器L
	com_tab[4] = HIGH(num);					//寄存器数目H
	com_tab[5] = LOW(num);					//寄存器数目H
	wCRCchecknum = Modbus_Caluation_CRC16(com_tab,6);
	lCrcBuff = wCRCchecknum&0x0ff;
	hCrcBuff = (wCRCchecknum>>8)&0x0ff;
	com_tab[6] = lCrcBuff;
	com_tab[7] = hCrcBuff;

	IOT_UART1SendData( com_tab,8);
}


/******************************************
  *     发送06命令码 写单个寄存器
  *****************************************
*/
void IOT_SetModbusdata_0x06(unsigned char addr, uint16_t start_reg, uint16_t uSenddata)
{
	unsigned int wCRCchecknum;
	unsigned char  lCrcBuff,hCrcBuff;
	unsigned char  com_tab[8]={0x00,0x06,0x00,0x00,0x00,0x2d,0x00,0x00};

	com_tab[0] = addr;         				//通讯地址
	com_tab[1] = 0x06;		 				//指令
	com_tab[2] = HIGH(start_reg);			//寄存器H
	com_tab[3] = LOW(start_reg);			//寄存器L
	com_tab[4] = HIGH(uSenddata);			//设置数据H
	com_tab[5] = LOW(uSenddata);			//设置数据L

	wCRCchecknum = Modbus_Caluation_CRC16(com_tab,6);
	lCrcBuff = wCRCchecknum&0x0ff;
	hCrcBuff = (wCRCchecknum>>8)&0x0ff;

	com_tab[6] = lCrcBuff;
	com_tab[7] = hCrcBuff;

	IOT_UART1SendData( com_tab,8);
}

/******************************************
  *     发送0x10寄存器指令给逆变器
  *****************************************
  */
void IOT_SetModbusdata_0x10(unsigned char addr, uint16_t start_reg,uint16_t reg_num, uint8_t* pSenddata)
{
	unsigned int wCRCchecknum=0;
	unsigned char  lCrcBuff=0,hCrcBuff=0;
	unsigned char  com_tab[200]={0};						//最多支持100个寄存器同时设定

	com_tab[0] = addr;        								//逆变器通讯地址
	com_tab[1] = 0x10;										//指令
	com_tab[2] = HIGH(start_reg);							//起始寄存器H
	com_tab[3] = LOW(start_reg);							//起始寄存器L

	com_tab[4] = HIGH(reg_num);								//寄存器数目H
	com_tab[5] = LOW(reg_num);								//寄存器数目L

	com_tab[6] = reg_num * 2;								//字节数目，每个寄存器占两个字节

	memcpy(&com_tab[7], pSenddata, (reg_num * 2));

	wCRCchecknum = Modbus_Caluation_CRC16(com_tab, 7 + (reg_num * 2));
	lCrcBuff = wCRCchecknum&0x0ff;      //
	hCrcBuff = (wCRCchecknum>>8)&0x0ff; //

	com_tab[7 + (reg_num * 2)] = lCrcBuff; 	//
	com_tab[8 + (reg_num * 2)] = hCrcBuff; 	//

	IOT_UART1SendData(com_tab,9+(reg_num * 2));

}

void IOT_SetModbusdata_0x17(  uint8_t* pSenddata,uint16_t uSendlen)
{

	IOT_UART1SendData(&pSenddata[0],uSendlen);

}


uint8_t IOT_ModbusSendHandle(void )
{
	uint8_t *pAPPRXBUF;
 	static uint8_t PV_SENDnum=0;
 	if((InverterWaveStatus_Judge(1,0) == IOT_ING) || \
 	  (InverterWaveStatus_Judge(1,0) == IOT_BUSY)) return 0;// LI 2018.04.08 是否有波形在请求

#if test_wifi
#else
 	InvCommCtrl.Addr = 1;	//便携式电源地址固定为1 测试用
#endif
	pAPPRXBUF = InvCommCtrl.SetParamData;
	PV_SENDnum++;
	if((InvCommCtrl.CMDFlagGrop & CMD_0X06) &&(_GucInvPolling0304ACK == INV_END)&& ( PV_SENDnum >3)  )
	{
		PV_SENDnum =0 ;
		_GucInvPollingStatus = INV_START;
		IOT_SetModbusdata_0x06(InvCommCtrl.Addr,
				InvCommCtrl.Set_InvParameterNum,
				InvCommCtrl.Set_InvParameterData);		//send 06
		ESP_LOGI(INV, "\r\n*******IOT_SetModbusdata_0x06********Set_InvParameterNum =%d    \r\n ",InvCommCtrl.Set_InvParameterNum   );
	}
	else if((InvCommCtrl.CMDFlagGrop & CMD_0X10) &&	(_GucInvPolling0304ACK == INV_END) && ( PV_SENDnum >10))
	{
		PV_SENDnum =0 ;
		_GucInvPollingStatus = INV_START;
		IOT_SetModbusdata_0x10(InvCommCtrl.Addr,
				InvCommCtrl.SetParamStart,
				(InvCommCtrl.SetParamEnd - InvCommCtrl.SetParamStart + 1),
				InvCommCtrl.SetParamData );				//send 10
		ESP_LOGI(INV, "\r\n*******IOT_SetModbusdata_0x10********SetParamStart =%d  SetParamEnd=%d \r\n ",InvCommCtrl.SetParamStart ,InvCommCtrl.SetParamEnd );
	}
	else if((InvCommCtrl.CMDFlagGrop & CMD_0X05) && (_GucInvPolling0304ACK == INV_END)&& ( PV_SENDnum > 3))
	{
		PV_SENDnum =0 ;
		_GucInvPollingStatus = INV_START;
		IOT_SetModbusdata_0x03(InvCommCtrl.Addr,
				InvCommCtrl.SetParamStart,
				InvCommCtrl.SetParamEnd );				//send 03() 获取逆变器03寄存器参数
		ESP_LOGI(INV,  "\r\n*************** CMD_0X05 send SetParamStart =%d  SetParamEnd=%d \r\n ",InvCommCtrl.SetParamStart ,InvCommCtrl.SetParamEnd );
	}
	else if((InvCommCtrl.CMDFlagGrop & CMD_0X17) && (_GucInvPolling0304ACK == INV_END)&& ( PV_SENDnum > 2))// && (_GucInvPollingStatus == INV_END))
	{
		PV_SENDnum =0 ;
		_GucInvPollingStatus = INV_START;
		IOT_SetModbusdata_0x17(pAPPRXBUF,InvCommCtrl.SetParamDataLen);
		ESP_LOGI(INV, " ******** InvCommCtrl.SetParamDataLen =%d *********************** \r\n ", InvCommCtrl.SetParamDataLen);
	}
	else if((InvCommCtrl.CMDFlagGrop & CMD_0X03)  && (_GucInvPolling0304ACK == INV_END) && ( PV_SENDnum >6))
	{
		PV_SENDnum =0 ;
    //    ESP_LOGI(INV , " ******** InvCommCtrl.CMDFlagGrop =%d *********************** \r\n ",InvCommCtrl.CMDFlagGrop);
		//InvCommCtrl.CMDFlagGrop &= ~CMD_0X03;    // 优化至读取04完成，
	    if(	System_state.Pv_Senderrflag ++  > 5 )  // 发送 5 次轮训03/04数据没有应答
		{
	      System_state.Pv_state = 0;  		   // 设备离线
	  //  IOT_Printf("\r\n System_state.Pv_state=0  \r\n" );
		}
		/* 数据获取轮询 */
		switch(InvCommCtrl.CMDCur)
		{
			case HOLDING_GROP:    // 03
			{
//				_GucInvPollingStatus = INV_START;   // 轮序开始 等待当次轮寻数据结束后才能处理应答数据
				_GucInvPolling0304ACK = INV_START;
				IOT_GetModbusHolding_03(InvCommCtrl.Addr,
						InvCommCtrl.HoldingGrop[InvCommCtrl.GropCur][0],
						InvCommCtrl.HoldingGrop[InvCommCtrl.GropCur][1]);
				break;
			}
			case INPUT_GROP:     // 04
			{
//				_GucInvPollingStatus = INV_START;   // 轮序开始 等待当次轮寻数据结束后才能处理应答数据
				_GucInvPolling0304ACK = INV_START;
				IOT_GetModbusInput_04(InvCommCtrl.Addr,
						InvCommCtrl.InputGrop[InvCommCtrl.GropCur][0],
						InvCommCtrl.InputGrop[InvCommCtrl.GropCur][1]);
				break;
			}
#if test_wifi
			case STATUS_GROUP:
			{
				_GucInvPolling0304ACK = INV_START;
			 //   IOT_Printf("\r\n STATUS_GROUP \r\n");
				IOT_GetModbusHolding_03(InvCommCtrl.Addr,
						InvMeter.StatusGroup[InvCommCtrl.GropCur][0],
						InvMeter.StatusGroup[InvCommCtrl.GropCur][1]);
				break;
			}
			case METER_GROUP:
			{
				if(uAmmeter.StatusData[1]==1)
				{
					_GucInvPolling0304ACK = INV_START; // 轮序开始 LI 2018.03.31 // LI 2018.03.31 等待当次轮寻数据结束后才能处理应答数据
					IOT_GetModbusMeter_20(InvCommCtrl.Addr,
						InvMeter.MeterGroup[InvCommCtrl.GropCur][0],
						InvMeter.MeterGroup[InvCommCtrl.GropCur][1]);
				}
				else
				{
				  //  IOT_Printf("\r\n METER_GROUP1 Offline \r\n");
					PollingSwitch();
				}
				break;
			}
			case History_Merer_GROUP:
			{
				if((uAmmeter.StatusData[1]==1 )  && (IOT_GET_Histotymeter_Flag()==1))
				{
					if(IOT_GET_Histotymeter_Flag()==1)
					{
						_GucInvPolling0304ACK = INV_START; // LI 2018.03.31  轮序开始，等待当次轮寻数据结束后才能处理应答数据
						IOT_GetModbusHistoryMeter_22( InvCommCtrl.Addr,
								InvMeter.MeterGroup[InvCommCtrl.GropCur][0],
								InvMeter.MeterGroup[InvCommCtrl.GropCur][1]);
					}
				}
				else
				{
				   // IOT_Printf("\r\n History_Merer_GROUP Offline \r\n");
					PollingSwitch();
				}
				break;
			}
#endif
			default:
			{
				InvCommCtrl.CMDCur = HOLDING_GROP;
				InvCommCtrl.GropCur = 0;
				break;
			}
		}
	}

	return 0;

}





