/*
 * iot_ammeter.h
 *
 *  Created on: 2022年6月9日
 *      Author: Administrator
 */

#ifndef COMPONENTS_INCLUDES_IOT_AMMETER_H_
#define COMPONENTS_INCLUDES_IOT_AMMETER_H_

#include <stdbool.h>
#include <stdint.h>

#define Flag_sever_online    0      //在线
#define Flag_sever_offline   1		//不在线
#pragma pack(1)
typedef struct
{
	uint8_t UpdateMeterFlag;			    //上传服务器电表数据
	uint8_t Flag_history_meter ;	 		//上报电表标志
	uint8_t Flag_GETPV_Historymeter188 ;    //服务器不在线，告诉发送逆变器 1
	uint8_t uSETPV_Historymeter188;         //设置了188寄存器标志位，反复设置
	uint8_t StatusData[256];				//180-200状态寄存器读取  03 命令
	uint8_t	MeterGroupData[256];			//电表 实时数据
	uint8_t HistorymeterGroupData[256];     //历史电表 数据
	uint16_t Flag_GEThistorymeter ;		    // 0x20 获取 77 地址判断是否有历史数据   pom 20210926
}Ammeter_date;

#pragma pack(1)
typedef struct
{
	uint8_t  StatusGroupNum;		//电表标志寄存器需要读取的数目
	uint16_t StatusGroup[5][2];		// 状态寄存器组

	uint8_t  MeterGroupNum;			//电表数据寄存器需要读取的数目
	uint16_t MeterGroup[5][2];		//电表寄存器组

}InverterMeterGroup;

#pragma pack()

extern InverterMeterGroup InvMeter;
extern Ammeter_date uAmmeter;
uint8_t IOT_GET_Histotymeter_Flag(void );
uint8_t IOT_SET_Histotymeter_Flag( uint8_t  uFlaghammeter);
void IOT_Send_PV_0x06_188( uint8_t ucENflag);
uint8_t IOT_Send_PV_meterFlag(void);

void IOT_clear_Historymeter188(void);
uint8_t IOT_Histotymeter_Timer(void );
uint8_t IOT_GET_SEND_188FLAG(void);

void IOT_Histotyamemte_datainit(void);
void IOT_GetModbusMeter_20(uint8_t addr, uint16_t start_reg, uint16_t end_reg);
void IOT_GetModbusHistoryMeter_22(uint8_t addr, uint16_t start_reg, uint16_t end_reg);
uint8_t IOT_MeterReceive(uint8_t *pbuf, uint16_t len);
uint8_t IOT_HistorymeterReceive(uint8_t *pbuf, uint16_t len);

uint8_t IOT_History_Ammeter_DataPacket(uint8_t *outdata);
uint8_t IOT_Ammeter_DataPacket(uint8_t *outdata);



#endif /* COMPONENTS_INCLUDES_IOT_AMMETER_H_ */
