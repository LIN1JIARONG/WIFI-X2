#ifndef _IOT_OTA_H_
#define _IOT_OTA_H_

#include <stdlib.h>
#include <stdbool.h>
#include "iot_crc32.h"


typedef enum{
	EUPDADTE_NULL = 0,
	EUPDADTE_COLLACTOR,		//采集器
	EUPDADTE_INERTER,		//逆变器
	EUPDADTE_PORTABLE_POWER,	//便携式电源
	EUPDADTE_TYPE_NUM,
}UPDADTE_TYPE_t;		//升级类型


extern int IOT_OTA_Init_uc(UPDADTE_TYPE_t OTA_TYPE_tem , bool is_Bsdiff_tem);
extern uint32_t IOT_OTA_Get_Written_Size_uc(void);
extern int IOT_OTA_Write_uc(const uint8_t* data , size_t size);
extern int IOT_OTA_Finish_uc(void);
extern uint32_t IOT_OTA_Get_Written_Size_uc(void);
extern void IOT_OTA_Set_Written_Offset_uc(uint32_t uiSavedOffset_tem);
extern FlASHTYPE IOT_OTA_Get_Save_Flash_Type_uc(void);


uint32_t IOT_OTA_Get_Running_app_addr_uc(void);
uint32_t IOT_OTA_Get_Update_Partition_addr_uc(void);
uint32_t IOT_OTA_Get_Save_File_addr_uc(void);
int IOT_OTA_Set_Boot(void);

#endif


