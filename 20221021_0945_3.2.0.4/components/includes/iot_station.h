/*
 * iot_station.h
 *
 *  Created on: 2021年11月22日
 *      Author: Administrator
 */

#ifndef COMPONENTS_INCLUDES_IOT_STATION_H_
#define COMPONENTS_INCLUDES_IOT_STATION_H_


#define WIFI_DISCONNET			((uint16_t)0x0001)		//WIFI 关闭连接
#define WIFI_CONNETING          ((uint16_t)0x0002)  	//WIFI 启动连接
#define WIFI_SCAN				((uint16_t)0x0004)  	//WIFI 扫描
#define WIFI_STOP				((uint16_t)0x0008)  	//WIFI 关闭
#define WIFI_isDISCONNET			((uint16_t)0x0010)  //WIFI 已经手动关闭


volatile uint32_t ScanIntervalTime;		//WIFI 扫描时间间隔

void wifi_init_sta(void);
void IOT_WiFi_Connect(void);
void IOT_WiFistationInit(void);
void IOT_WIFIstation_Task(void *arg);


void IOT_WIFIEvenSetCONNECTED(void);
void IOT_GETMAC(void);
void IOT_WIFIEvenSetSCAN(void);
void IOT_WaitOTAback( void );
void IOT_WIFIEvenSetOAT(void);   // 通知连接
void IOT_WIFIOnlineFlag(uint8_t uWififlag);
void IOT_WIFIEvenSetON(void) ;
void IOT_WIFIEvenSetOFF(void) ;

void IOT_WIFIEven_StartSmartConfig(void);
void IOT_WIFIEven_CloseSmartConfig(void);
void IOT_SmartConfig_Restart(void);
void IOT_WIFIEvenRECONNECTED(void);   // 通知重新连接
#endif /* COMPONENTS_INCLUDES_IOT_STATION_H_ */





