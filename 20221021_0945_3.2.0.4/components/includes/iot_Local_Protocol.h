#ifndef _IOT_PROTOCOL_H_
#define _IOT_PROTOCOL_H_


#include <stdint.h>
#include "iot_mqtt.h"

uint32_t IOT_AES_CBC_encrypt_ui(uint8_t ucmode_tem ,unsigned char *pucAesInputData_tem , \
								uint32_t InputDataLen , unsigned char *pucAesOutputData);

uint16_t IOT_Repack_ProtocolData_V5ToV6_us(uint8_t *pucData_tem , uint16_t usDataLen_tem , uint8_t *pucOut_tem);
uint16_t IOT_Repack_LocalProtocolData_Decrypt_us(uint8_t *pucData_tem , uint16_t usDataLen_tem , uint8_t *pucOut_tem);
EMQTTREQ IOT_LocalReceiveHandle(uint8_t* pucBuf_tem, uint16_t suiLen_tem);


#endif


