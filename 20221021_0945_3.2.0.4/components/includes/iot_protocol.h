/*
 * iot_protocol.h
 *
 *  Created on: 2021Äê12ÔÂ9ÈÕ
 *      Author: Administrator
 */

#ifndef COMPONENTS_INCLUDES_IOT_PROTOCOL_H_
#define COMPONENTS_INCLUDES_IOT_PROTOCOL_H_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "iot_mqtt.h"
#include "iot_system.h"

#define STATE_0X03 			'3'
#define STATE_0X04 			'4'

#define ProtocolVS05	0x05
#define ProtocolVS06	0x06
#define ProtocolVS07	0x07

#if LOCAL_PROTOCOL_6_EN
#define LOCAL_PROTOCOL 			(0xA000 | ProtocolVS06)

#else
#define LOCAL_PROTOCOL 			(0xA000 | ProtocolVS05)
#endif

#define PROTOCOL_VERSION_L       ProtocolVS07
#define REMOTE_SERVER_PROTOCOL 	(0xB000 | PROTOCOL_VERSION_L)

void IOT_ServerFunc_0x18(uint8_t REGNum ,uint8_t REGLen,  uint8_t *pDATA);
EMQTTREQ IOT_ServerReceiveHandle(uint8_t* pucBuf_tem, uint16_t suiLen_tem);

uint16_t IOT_ESPSend_0x03(uint8_t *upSendTXBUF ,unsigned int Vsnum);
uint16_t IOT_ESPSend_0x04(uint8_t *upSendTXBUF  ,unsigned int Vsnum);
uint16_t IOT_ESPSend_0x05(uint8_t *upSendTXBUF ,unsigned int Vsnum);
uint16_t IOT_ESPSend_0x06(uint8_t *upSendTXBUF  ,unsigned int Vsnum);
uint16_t IOT_ESPSend_0x10(uint8_t *upSendTXBUF  ,unsigned int Vsnum);
uint16_t IOT_ESPSend_0x18(uint8_t *upSendTXBUF ,unsigned int Vsnum);
uint16_t IOT_ESPSend_0x19(uint8_t *upSendTXBUF  ,unsigned int Vsnum);
void IOT_ServerFunc_0x16( void );
uint16_t IOT_UploadParamSelf(uint8_t *upSendTXBUF,unsigned int Vsnum);
uint16_t IOT_ESPSend_0x17(uint8_t *upSendTXBUF  ,unsigned int Vsnum);
void IOT_KEYSET0X06( uint16_t uiSetREG,uint16_t  uiSetdate);

uint16_t Server0x19_MultiPack(uint8_t *outdata,uint16_t REGAddrStart ,uint16_t REGAddrEnd);

void IOT_CommTESTData_0x18(uint8_t* pucBuf_tem, uint16_t suiLen_tem);
void IOT_CommTESTData_0x19(uint8_t* pucBuf_tem, uint16_t suiLen_tem);
uint16_t IOT_ESPSend_0x25(uint8_t *upSendTXBUF ,unsigned int Vsnum );
uint16_t IOT_ESPSend_0x50(uint8_t *upSendTXBUF ,unsigned int Vsnum);

uint16_t InputGrop_DataPacket(uint8_t *outdata);
uint16_t IOT_ESPSend_0x20(uint8_t *upSendTXBUF ,unsigned int Vsnum);
uint16_t IOT_ESPSend_0x22(uint8_t *upSendTXBUF ,unsigned int Vsnum);

uint16_t IOT_ESPSend_0x14(uint8_t *upSendTXBUF ,unsigned int Vsnum);
uint16_t IOT_ESPSend_0x37(uint8_t *upSendTXBUF ,unsigned int Vsnum);
uint16_t IOT_ESPSend_0x38(uint8_t *upSendTXBUF ,unsigned int Vsnum);

void IOT_ServerACK_0x14(void);

#endif /* COMPONENTS_INCLUDES_IOT_PROTOCOL_H_ */

