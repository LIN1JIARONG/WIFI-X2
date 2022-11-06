/*
 * iot_universal.h
 *
 *  Created on: 2021Äê12ÔÂ1ÈÕ
 *      Author: Administrator
 */

#ifndef COMPONENTS_INCLUDES_IOT_UNIVERSAL_H_
#define COMPONENTS_INCLUDES_IOT_UNIVERSAL_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


unsigned short crc16_ccitt(const unsigned char *buf, int len);
unsigned short int  Modbus_Caluation_CRC16(unsigned char *Buff, unsigned int nSize);
uint32_t GetCRC32 (void *pStart, uint32_t uSize);
int GetCRC16(unsigned char* pchMsg, int wDataLen);
uint16_t MERGE(uint8_t x,uint8_t y);
void IOT_ESP_strncpy(unsigned char *dst, unsigned char *src, uint32_t n);

#define LOW(a) 		(a & 0xFF)
#define HIGH(a) 	(((unsigned int)a >> 8) & 0xFF)


unsigned int IOT_htoi(unsigned char *hex, unsigned char length);
unsigned int IOT_ctoi(unsigned char Number,unsigned char *Chr);
void IOT_itoh(unsigned char *hex, unsigned int dn, unsigned char length);
const char* IOT_ESP_strstr(const unsigned char *FoundBuf, const unsigned char *FindStr,uint16_t sumnum,uint8_t findnum);

#endif /* COMPONENTS_INCLUDES_IOT_UNIVERSAL_H_ */
