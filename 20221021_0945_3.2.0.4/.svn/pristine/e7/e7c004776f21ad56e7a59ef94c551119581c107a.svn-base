/*
 *  @File    : iot_crc32.h
 *  @Author  : isen
 *  @Version : 1.0
 *  @Created : 2021.01.29
 */

#ifndef _IOT_CRC32_H_
#define _IOT_CRC32_H_

#ifndef IOT_CRC32
#define IOT_CRC32_EXTERN extern
#else
#define IOT_CRC32_EXTERN
#endif

#include <stdlib.h>


typedef enum{
	NULL_FLASH = 0,
	External_FLASH = 1,
	System_FLASH,
	FLASH_NUM,
}FlASHTYPE;


IOT_CRC32_EXTERN unsigned int IOT_CRC32_Get_ui(void *pStart_tem, unsigned int uiSize_tem);
IOT_CRC32_EXTERN unsigned int IOT_FlashCRC32_Get_ui(unsigned int uiStartAddr_tem,unsigned int uiLen_tem);

 
int CRC16_1(unsigned char* pchMsg, int wDataLen);
int CRC16_2(unsigned char* pchMsg, int wDataLen);
uint32_t FLASH_GetCRC32 (uint32_t startAddr,uint32_t Length,uint8_t GetPosition);

void IOT_SPI_External_FLASH_BufferRead(uint8_t *buff , uint32_t addr , uint32_t read_len);
void IOT_SPI_System_FLASH_BufferRead(uint8_t *buff , uint32_t addr , uint32_t read_len);

#endif 





