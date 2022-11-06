/*
 * iot_net.h
 *
 *  Created on: 2021Äê12ÔÂ2ÈÕ
 *      Author: Administrator
 */

#ifndef COMPONENTS_INCLUDES_IOT_NET_H_
#define COMPONENTS_INCLUDES_IOT_NET_H_


extern unsigned char MQTT_TXbuf[1024] ;
extern unsigned char MQTT_RXbuf[512] ;
extern unsigned char MQTT_DATAbuf[1024] ;

uint8_t IOT_SSL_Init(void);
uint8_t IOT_Mbedtls_Init(void);
uint8_t IOT_Mbdetls_senddata(unsigned char *pucSendDatabuf ,uint32_t ucSend_len);
void IOT_MbedtlsRead_task(void *pvParameters);
void IOT_MbedtlsWrite_task(void *pvParameters);
void https_get_task(void *pvParameters);

void IOT_Connet_Close(void);

#endif /* COMPONENTS_INCLUDES_IOT_NET_H_ */
