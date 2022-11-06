/*
 * iot_inverter.h
 *
 *  Created on: 2021年12月14日
 *      Author: Administrator
 */

#ifndef COMPONENTS_INCLUDES_IOT_INVERTER_H_
#define COMPONENTS_INCLUDES_IOT_INVERTER_H_

#include <stdbool.h>
#include <stdint.h>

#define CMD_0X06			((uint16_t)0x0001)		//设置单个逆变器参数
#define CMD_0X10			((uint16_t)0x0002)  	//设置多个逆变器参数
#define CMD_0X17			((uint16_t)0x0004)  	//完全透传指令
#define CMD_0X03			((uint16_t)0x0008)  	//03读holding寄存器
#define CMD_0X04			((uint16_t)0x0010)  	//04读input寄存器
#define CMD_0X05			((uint16_t)0x0020)  	//05选择性读holding寄存器
#define CMD_0X18			((uint16_t)0x0040)  	//18设置采集器参数
#define CMD_0X19			((uint16_t)0x0080)  	//19读取采集器参数
#define CMD_0X26			((uint16_t)0x0100)  	//26升级

#define PV_ACKOK			0		//根据数服协议定的 应答情况
#define PV_ACKOUTTIME		1
#define PV_ACKERROR			2
#define PV_ACKNULL			0xfe
#define DATA_MAX 			900

#define PARAM_DATABUF_MAXLEN 500


#pragma pack(4)
typedef struct
{

	volatile uint32_t CMDFlagGrop;			//指令组 服务器下发读取/05/06/10/17命令  ->获取数据之后 清位
//	volatile uint32_t RespondFlagGrop;		//要回复的指令组
	/* 逆变器属性的配置 */
	volatile uint16_t SetParamStart;		//要设置的逆变器参数起始 针对设置逆变器多个参数    //05命令
	volatile uint16_t SetParamEnd;			//要设置的逆变器参数结束
	volatile uint16_t Set_InvParameterNum;	//要设置的逆变器参数编号  					//06命令
	volatile uint16_t Set_InvParameterData;	//要设置的逆变器参数
	volatile uint16_t Set_InvRespondCode;	//要应答给Server的状态
//	uint8_t RespondAble;					//应答使能

	uint8_t SetParamDataLen;				//要设置的多个参数或透传指令的长度
	uint8_t SetParamData[100];				//要设置的多个参数或透传指令的数据
	uint8_t GetParamDataBUF[PARAM_DATABUF_MAXLEN];			//05数据接收
	uint16_t GetParamData;					//接收长度


	uint16_t HoldingGrop[5][2];				//要获取的寄存器编号03寄存器组
	uint16_t InputGrop[5][2];				//04寄存器组
	uint16_t MeterGroup[5][2];				//电表寄存器组
	uint16_t CMDCur;						//当前轮序哪个指令
	uint16_t GropCur;						//当前轮询到哪一组
	uint16_t DTC;							//逆变器设备类型
	uint8_t  Addr;						    //逆变器通讯地址
	char 	SN[30];							//逆变器设备序列号

	uint8_t UpdateInvTimeFlag;				//同步逆变器设备的时间  状态（1/需要同步 ，轮训设备时间检测时间是否超过x—minutes ，否则  0  不需要同步）

//	uint8_t UpdateInvTimeStep;				//2,
//	uint8_t UpdateInvTimeOnce;				//3,==TRUE需要更新
	uint8_t ContrastInvTimeFlag;			//4,是否对比逆变器时间
											//1用于流程控制,2用于步骤控制,
											//3用于标示是否同步一次时间,4用来标示是否执行一次时间对

	uint8_t SetUpInvDATAFlag;			//设置读取，同步获取03数据标志

	uint8_t	HoldingSentFlag;            //1:服务器确认收到采集器03指令的数据,0:重新上传
	uint8_t	InputSentFlag;          	//1:服务器确认收到采集器04指令的数据
	uint8_t	HoldingNum;					//03寄存器需要读几组
	uint8_t	InputNum;					//04寄存器需要读几组

	uint8_t	InputData[DATA_MAX];	 	//04 实时数据
	uint8_t	HoldingData[DATA_MAX];		//03 实时数据
//电表的数据移到 iot_ammerter.h
	uint8_t GetDtcFlag;

	uint8_t NoReplyCnt;					//通讯未应答计数

//	uint32_t TimePolling;				//轮训时间
//	uint32_t Bound;						//通讯波特率
//	uint8_t	 OfflineTxBuf[DATA_MAX];
	uint8_t PVClock_Time_Tab[6];
	uint8_t	Reserved;
	uint8_t tsetasdasd;

}INV_COMM_CTRL;
extern INV_COMM_CTRL InvCommCtrl;
extern volatile uint8_t  _GucInvPollingStatus  ;	// 轮序逆变器数据状态
extern volatile uint8_t  _GucInvPolling0304ACK  ;	// 轮序逆变器数据状态
#pragma pack(1)
typedef struct
{
	uint16_t DTC;						//DTC
	uint16_t HoldGropNum;				//03寄存器需要读取的数目
	uint16_t HoldGrop[5][2];			//03寄存器组

	uint16_t Reserved;					//保持
	uint16_t InputGropNum;				//04寄存器读取的数目
	uint16_t InputGrop[5][2];			//04寄存器组

}DTC_INDEX;



#define HOLDING_GROP				(0)			//03组
#define INPUT_GROP					(1)			//04组
#define STATUS_GROUP				(2)			//状态寄存器数据  180- 200
#define METER_GROUP				  	(3)			//电表数据
#define History_Merer_GROUP         (4)			//电表历史数据


#define SKA_1500LV 	 20502   // 便携式电源

#define pv_mix 		 3501   // 3500~3599
#define pv_max 		 5000   // 5000~5099  MAX(5000)、MID机型(5001)、MOD机型(5002)、MAC机型(5003)
#define pv_sp3000    3300
#define pv_sp2000    3100   // 3100~3200
#define pv_mtlp_us 	 210
/*add by Cc 20190611*/
#define pv_mtlx   	 5200   // (0x03-2字段   0x04-2字段) -(mtl-x->5200~5299   mtl-xh->5100~5199)
#define pv_mtlxh   	 5100   // (0x03-2字段   0x04-3字段) -(mtl-x->5200~5299   mtl-xh->5100~5199)
#define pv_spa    	 3701   // (0x03-2字段   0x04-2字段) -(3700~3749)  LI 2018.09.21
#define pv_sph4_10k  3600 	// (0x03-2字段   0x04-2字段) -(3600~3649)  LI 2018.10.10
#define pv_spa4_10k  3650 	// (0x03-2字段   0x04-2字段) -(3650~3699)  LI 2018.10.10
#define pv_max_1500v 5500   // (0x03-2字段   0x04-3字段) -(5500~5599)  zhao 2020.03.28
#define pv_mod  	 5400 	// (0x03-2字段   0x04-3字段) -(5400~5499)  zhao 2020.03.28
#define pv_tl_xh_us  5300  	// (0x03-3字段   0x04-2字段) -(5300~5399)  zhao 2020.03.28
#define pv_wit_100k_tl3h  	5600  	// (0x03-3字段   0x04-3字段) -(5600~5699)  zhao 2020.03.28    //20220902  新增兼容西安储能机

uint8_t IOT_DTCInit(uint16_t tmp);

uint8_t IOT_ModbusReceiveHandle(uint8_t *pUartRxbuf, uint16_t iUartRxLen);
uint8_t IOT_ModbusSendHandle(void );
void IOT_INVPollingEN(void);
uint8_t IOT_ESP_InverterType_Get(void);
uint8_t IOT_ESP_InverterSN_Get(void);

uint8_t RS232CommHandle(uint8_t *pUartRxbuf, uint16_t iUartRxLen);

uint8_t IOT_NewInverterSN_Check(uint8_t *sn);// 30位码 逆变器序列号判断，避免误判10位为30位
void IOT_stoppolling(void);

void PollingSwitch(void);
uint8_t IOT_InverterTime_Check(uint8_t num,uint8_t addr);




#endif /* COMPONENTS_INCLUDES_IOT_INVERTER_H_ */
