/*
 * iot_uart1.h
 *
 *  Created on: 2021年11月16日
 *      Author: Administrator
 */

#ifndef COMPONENTS_INCLUDES_IOT_UART1_H_
#define COMPONENTS_INCLUDES_IOT_UART1_H_

#include "driver/uart.h"
#define TXD1_PIN (GPIO_NUM_0)
#define RXD1_PIN (GPIO_NUM_1)

#define TXD0_PIN (GPIO_NUM_21)
#define RXD0_PIN (GPIO_NUM_20)

#define UART1_EVENTS  1


#pragma pack(4)
typedef struct
{
//	uint8_t Buffer[512];
	volatile uint32_t Size;			 //接收实际长度
 	volatile uint32_t ReadDone;      //接收完成标志
	volatile uint32_t Read_waitTime;   //接收超时时间
	volatile uint32_t Read_count; //接收超时时间
} s_RxBuffer;



extern uint8_t RX_BUF[1024];

extern s_RxBuffer Uart_Rxtype;
void IOT_UART0_Init(void);
void IOT_UART1_Init(void);
uint8_t IOT_SetUartbaud( uart_port_t uart_num, int Setbaud_rate);

int sendData(const char* logName, const char* data);
void tx_task(void *arg);
void rx_task(void *arg);


void IOT_InvCommCtrl_Init(void);

void IOT_ModbusRXTask(void *arg);
void IOT_ModbusTXTask(void *arg);


uint16_t IOT_UART1SendData(uint8_t *uSendDATABuf, uint16_t uSendLen );
void IOT_ModbusRXeventsTask(void *arg);
void IOT_UART1_events_init(void );


#if 0
/******************电源寄存器列表**************
uPowerRegister
*/
		//Contents		Addr_dec
#define System_Status	0
#define Vpv1			1
#define wCarVolt		2
#define Ppv1_H			3
#define Ppv1_L			4
#define wCarWatt		5
#define wCarCurr		6
#define Buck1Curr		7
#define reserved_01				8
#define OP_Watt_H		9
#define OP_Watt_L		10
#define reserved_02				11
#define reserved_03			12
#define ACChr_Watt_H	13
#define ACChr_Watt_L	14
#define reserved_04			15
#define reserved_05				16
#define Bat Volt		17
#define BatterySOC		18
#define reserved_06				19
#define Grid Volt		20
#define Line Freq		21
#define OutputVolt		22
#define OutputFreq		23
#define reserved_07				24
#define InvTemp			25
#define DcDc Temp		26
#define LoadPercent		27
#define wThermal1(PD)	28
#define wThermal2(PD)	29
#define wReleaseChgMin	30
#define wReleaseDisChgMin	31
#define Buck1_NTC		32
#define wTxtThermal		33
#define OP_Curr			34
#define Inv_Curr		35
#define AC_InWatt_H		36
#define AC_InWatt_L				37
#define reserved_08				38
#define reserved_09				39
#define Fault bit				40
#define Warning bit				41
#define uwWarnBit_H				42
#define uwWarnBit3				43
#define DTC						44
#define wUsb1Volt				45
#define wUsb2Volt				46
#define wUsb3Volt				47
#define wUsb4Volt(TypeC)		48
#define wUsb5Volt(TypeC)		49
#define wUsb1Curr				50
#define wUsb2Curr				51
#define wUsb3Curr				52
#define wUsb4Curr(TypeC)		53
#define wUsb5Curr(TypeC)		54
#define wUsb1Watt				55
#define wUsb2Watt				56
#define wUsb3Watt				57
#define wUsb4Watt(TypeC)		58
#define wUsb5Watt(TypeC)		59
#define wChgWattAll				60
#define wIpWattAll				61
#define wOpWattAll				62
#define reserved_10				63
#define reserved_11				64
#define reserved_12				65
#define reserved_13				66
#define reserved_14				67
#define ACChrCurr				68
#define AC_DisChrWatt_H 		69
#define AC_DisChrWatt_L			70
#define reserved_15				71
#define reserved_16				72
#define Bat_DisChrWatt_H		73
#define Bat_DisChrWatt_L		74
#define reserved_17				75
#define reserved_18				76
#define Bat_Watt_H				77
#define Bat_Watt_L				78
#define reserved_19				79
#define reserved_20				80
#define reserved_21				81
#define reserved_22				82
#define ChgCurr					83
#define DischgCurr				84
#define reserved_23				85
#define reserved_24				86
#define reserved_25				87
#define reserved_26				88
#define Bms warning				89
#define reserved_27				90
#define reserved_28				91

#endif



#endif /* COMPONENTS_INCLUDES_IOT_UART1_H_ */
