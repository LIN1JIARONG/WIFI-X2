/*
 * iot_record.h
 *
 *  Created on: 2022年7月15日
 *      Author: grt-chenyan
 */

#ifndef COMPONENTS_INCLUDES_IOT_RECORD_H_
#define COMPONENTS_INCLUDES_IOT_RECORD_H_

#include <stdint.h>
#include <stdbool.h>

#define  LOAD_FORMAT_PARAM   0



typedef struct {
	uint32_t uiPacketSize;
	uint32_t WriteAddr;
	uint32_t ReadAddr;
	uint32_t reserve;
	char ucIOTESPInverterID[11];

} __attribute__((packed)) record_storage_format_param_t;	//强制以1字节地址对齐



void IOT_ReadRecord_Init(void);
uint16_t IOT_Get_Record_Num(void);
void iot_Reset_Record(void);
void iot_Record_storageFormat_Write(record_storage_format_param_t  * FormatParam_tem );
void iot_Record_storageFormat_init(void);
bool IOT_ESP_InverterFlashDataRWAddr_Get(void);
void IOT_ESP_SPI_FLASH_ReUploadData_Write(uint8_t *src,uint16_t len);
uint16_t IOT_ESP_SPI_FLASH_ReUploadData_Read_us(uint8_t *output,uint16_t BUffLen);
bool IOT_ESP_Record_ResetSign_Check_b(uint8_t *pucData_tem);
void IOT_ESP_Record_ResetSign_Write(void);
void iot_Reset_Record_Task(void);
#endif /* COMPONENTS_INCLUDES_IOT_RECORD_H_ */
