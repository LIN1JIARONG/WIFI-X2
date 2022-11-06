/*
 * iot_gatts_server.h
 *
 *  Created on: 2021��12��1��
 *      Author: Administrator
 */

#ifndef COMPONENTS_INCLUDES_IOT_GATTS_SERVER_H_
#define COMPONENTS_INCLUDES_IOT_GATTS_SERVER_H_

#define IOT_BLE_RX_MAX_LEN  200 // BLE��������ȣ� 200�ֽ�
#define IOT_BLE_TX_MAX_LEN  500 // BLE��������ȣ� 200�ֽ�

extern unsigned char g_ucBLERxBuf[IOT_BLE_RX_MAX_LEN];
extern unsigned char g_ucBLETxBuf[IOT_BLE_TX_MAX_LEN];

extern unsigned int g_uiBLERxLen;
extern unsigned int g_uiBLETxLen;

void IOT_Gatts_Serve_Init(void );

void IOT_Gatts_server_task(void *arg);
void IOT_Gatts_send_data(uint8_t *Sendbuf ,uint32_t Sendlen);




#endif /* COMPONENTS_INCLUDES_IOT_GATTS_SERVER_H_ */
