/*
 * iot_params.c
 *
 *  Created on: 2021年12月20日
 *      Author: Administrator
 */
/*
 * iot_params.c
 *
 *  Created on: 2021年12月8日
 *      Author: Administrator
 */
#define  IOT_PARAMS_GLOBAL
#include "iot_params.h"
#include "iot_universal.h"

#include <string.h>
#include "esp_spi_flash.h"
#include "esp_system.h"
#include "iot_system.h"

#define MAX_PARAMS_LEN  50	// 每个参数最大长度，预留50byte

IOT_PARAMS_EXTERN unsigned char g_ucFlashBuf[PARAMS_ZONE_PAGE_SIZE];


/******************************************************************************
 * FunctionName : IOT_AddParam_Init
 * Description  : 附加参数初始化外部flash
 * Parameters   : none
 * Returns      : none
 * Notice       : 新增加参数flash地址分配：
 *							0x00007000 - 0x00007FFF : 4K数据空间
 * 				  			0x00007000 - 0x000070FF ：存放参数个数地址（预留80个参数，每个参数预留长度为50字节）0-255 == 256字节
 * 				  			0x00007100 - 0x00007FF9 ：存放参数数据        256-4089 -->80*50 == (4096 - 256 - 6)字节
 * 				  			0x00007FFA - 0x00007FFD ：存放数据校验位       4090-4093 == 4字节
 * 				  			0x00007FFE - 0x00007FFF ：存放crc校验值       4094-4095 == 2字节
 * 				            |——数据长度地址--|--数据值--|--标记位--|--数据crc值--| = 4096字节
 *
 * 				      参数编号num分配：
 * 				      NO   : 偏移地址 256字节 -- 注意：每个参数预留最长保存50字节长度
 *					  NO.0 ： 升级包长度 长度4个字节
 *                    NO.1 : 升级文件CRC32 长度1个字节
 *                    NO.2 : 升级文件预留 长度1个字节
 *                    NO.3 : 升级文件预留 长度1个字节
 *                    NO.4 : 升级文件断点记录下载长度值
 *                    NO.5 : 升级状态记录值
 *                    NO.6 : 下载文件保存的Flash类型
*******************************************************************************/
void IOT_AddParam_Init(void)
{
	unsigned char *pucParamBuf = g_ucFlashBuf;// 参数备份区 |——数据长度地址--|--数据值--|--标记位--|--数据crc值--| = 4096字节
	unsigned int uiParamsOffsetAddr = PARAMS_LEN_OFFSET;
	unsigned int uiParamsSignOffsetAddr = PARAMS_SIGN_OFFSET;
	unsigned int uiParamsCrcOffsetAddr = PARAMS_CRC_OFFSET;
    unsigned int uiParamsPageSize = PARAMS_ZONE_PAGE_SIZE;
	unsigned int uiSaveParamsAddr = FOTA_PARAMS_ADDR; // 保存参数地址
	unsigned short int usiCRC16Val = 0;

	spi_flash_read(uiSaveParamsAddr, pucParamBuf, uiParamsPageSize);	  // 一次性将原有数据读出得到参数
	if(memcmp(&pucParamBuf[uiParamsSignOffsetAddr],PARAMS_ZONE_WRITED_SIGN,4))// 检测是否写入过FLASH
	{
		memset(pucParamBuf,0,uiParamsPageSize);
		memset(pucParamBuf,1,uiParamsOffsetAddr);// 长度默认是1， 数据默认是：0
		memcpy(&pucParamBuf[uiParamsSignOffsetAddr],PARAMS_ZONE_WRITED_SIGN,4); // 写入数据区标志位

		/* CRC16校验 */
		usiCRC16Val = Modbus_Caluation_CRC16(pucParamBuf, uiParamsPageSize-2);
		pucParamBuf[uiParamsCrcOffsetAddr] = HIGH(usiCRC16Val);
		pucParamBuf[uiParamsCrcOffsetAddr+1] = LOW(usiCRC16Val);

		spi_flash_erase_sector(FOTA_PARAMS_ADDR/PARAMS_ZONE_PAGE_SIZE);     // 写入参数前需要先擦初页
		spi_flash_write(uiSaveParamsAddr, pucParamBuf, uiParamsPageSize);   // 一次性将写入参数

		IOT_Printf("\r\n IOT_AddParam_Init()  usiCRC16Val = %d \r\n",usiCRC16Val);
	}
}

/******************************************************************************
 * FunctionName : IOT_AddParam_Write
 * Description  : 附加参数写入外部flash
 * Parameters   : none
 * Returns      : none
 * Notice       : 新增加参数flash地址分配：
 *							0x00007000 - 0x00007FFF : 4K数据空间
 * 				  			0x00007000 - 0x000070FF ：存放参数个数地址（预留80个参数，每个参数预留长度为50字节）0-255 == 256字节
 * 				  			0x00007100 - 0x00007FF9 ：存放参数数据         256-4089 -->80*50 == (4096 - 256 - 6)字节
 * 				  			0x00007FFA - 0x00007FFD ：存放数据校验位       4090-4093 == 4字节
 * 				  			0x00007FFE - 0x00007FFF ：存放crc校验值        4094-4095 == 2字节
 * 				           |——数据长度地址--|--数据值--|--标记位--|--数据crc值--| = 4096字节
 *
 * 				      参数编号num分配：
 * 				      NO   : 偏移地址 256字节 -- 注意：每个参数预留最长保存50字节长度
 *					  NO.0 ： 升级包长度 长度4个字节
 *                    NO.1 : 升级文件CRC32 长度1个字节
 *                    NO.2 : 升级文件预留 长度1个字节
 *                    NO.3 : 升级文件预留 长度1个字节
 *                    NO.4 : 升级文件断点记录下载长度值
 *                    NO.5 : 升级状态记录值
 *                    NO.6 : 下载文件保存的Flash类型
 *                    NO.7 : 下载文件保存的起始地址
*******************************************************************************/
void IOT_AddParam_Write(unsigned char *pucSrc_tem, unsigned char ucNum_tem, unsigned char ucLen_tem)
{
	unsigned char *pucParamBuf = g_ucFlashBuf;// 参数备份区 |——数据长度地址--|--数据值--|--标记位--|--数据crc值--| = 4096字节
	unsigned int uiParamsOffsetAddr = PARAMS_LEN_OFFSET;
//	unsigned int uiParamsSignOffsetAddr = PARAMS_SIGN_OFFSET;
	unsigned int uiParamsCrcOffsetAddr = PARAMS_CRC_OFFSET;
    unsigned int uiParamsPageSize = PARAMS_ZONE_PAGE_SIZE;
	unsigned int uiSaveParamsAddr = FOTA_PARAMS_ADDR; // 保存参数地址
	char cSingleParamLen  = MAX_PARAMS_LEN;           // 单个参数数据长度
	unsigned short int usiCRC16Val = 0;

	spi_flash_read(uiSaveParamsAddr, pucParamBuf, uiParamsPageSize);// 一次性将原有数据读出得到参数
	usiCRC16Val = Modbus_Caluation_CRC16(pucParamBuf, uiParamsPageSize-2);           // 读取原有参数 (长度 + 参数)crc校验

	/* 检验保存数据crc值  */
	if((pucParamBuf[uiParamsCrcOffsetAddr] == HIGH(usiCRC16Val)) &&\
 	   (pucParamBuf[uiParamsCrcOffsetAddr+1] == LOW(usiCRC16Val)))
	{
		pucParamBuf[ucNum_tem] = ucLen_tem;
		memcpy(&pucParamBuf[(uiParamsOffsetAddr+(ucNum_tem*cSingleParamLen))],pucSrc_tem,ucLen_tem); // 更新写入参数值
		usiCRC16Val = Modbus_Caluation_CRC16(pucParamBuf, uiParamsPageSize-2);                                    // 新写入参数 (长度 + 参数)crc校验
		pucParamBuf[uiParamsCrcOffsetAddr] = HIGH(usiCRC16Val);
		pucParamBuf[uiParamsCrcOffsetAddr+1] = LOW(usiCRC16Val);
		spi_flash_erase_sector(FOTA_PARAMS_ADDR/PARAMS_ZONE_PAGE_SIZE);     // 写入参数前需要先擦初页
		spi_flash_write(uiSaveParamsAddr, pucParamBuf, uiParamsPageSize);   // 一次性将写入参数

		IOT_Printf("\r\n IOT_AddParam_Write() num = %d, len = %d CRC = %x src = ", ucNum_tem,ucLen_tem,usiCRC16Val);
		{for(int i=0; i<ucLen_tem; i++)IOT_Printf(" %02x", pucSrc_tem[i]); IOT_Printf("\r\n");}
	}
	else
	{
		IOT_Printf("\r\n IOT_AddParam_Write() saved crc  = %x%x \r\n",pucParamBuf[uiParamsCrcOffsetAddr], pucParamBuf[uiParamsCrcOffsetAddr]+1);
		IOT_Printf("\r\n IOT_AddParam_Write() Read crc fail, rewrite = %x \r\n",usiCRC16Val);

		pucParamBuf[uiParamsCrcOffsetAddr] = HIGH(usiCRC16Val);
		pucParamBuf[uiParamsCrcOffsetAddr+1] = LOW(usiCRC16Val);
		spi_flash_erase_sector(FOTA_PARAMS_ADDR/PARAMS_ZONE_PAGE_SIZE);     // 写入参数前需要先擦初页
		spi_flash_write(uiSaveParamsAddr, pucParamBuf, uiParamsPageSize);   // 一次性将写入参数
	}
}

/******************************************************************************
 * FunctionName : IOT_AddParam_Read
 * Description  : 附加参数读取外部flash
 * Parameters   : none
 * Returns      : none
 * Notice       : 新增加参数flash地址分配：
 *							0x00007000 - 0x00007FFF : 4K数据空间
 * 				  			0x00007000 - 0x000070FF ：存放参数个数地址（预留80个参数，每个参数预留长度为50字节）0-255 == 256字节
 * 				  			0x00007100 - 0x00007FF9 ：存放参数数据         256-4089 -->80*50 == (4096 - 256 - 6)字节
 * 				  			0x00007FFA - 0x00007FFD ：存放数据校验位       4090-4093 == 4字节
 * 				  			0x00007FFE - 0x00007FFF ：存放crc校验值        4094-4095 == 2字节
 * 				            |——数据长度地址--|--数据值--|--标记位--|--数据crc值--| = 4096字节
 *
 * 				      参数编号num分配：
 * 				      NO   : 偏移地址 256字节 -- 注意：每个参数预留最长保存50字节长度
 *					  NO.0 ： 升级包长度 长度4个字节
 *                    NO.1 : 升级文件CRC32 长度4个字节
 *                    NO.2 : 升级文件预留 长度1个字节
 *                    NO.3 : 升级文件预留 长度1个字节
 *                    NO.4 : 升级文件断点记录下载长度值
 *                    NO.5 : 升级状态记录值
 *                    NO.6 : 下载文件保存的Flash类型	长度4个字节
*******************************************************************************/
void IOT_AddParam_Read(unsigned char *pucSrc_tem, unsigned char ucNum_tem)
{
    unsigned char *pucParamBuf = g_ucFlashBuf;// 参数备份区 |——数据长度地址--|--数据值--|--标记位--|--数据crc值--| = 4096字节
	unsigned int uiParamsOffsetAddr = PARAMS_LEN_OFFSET;
	//	unsigned int uiParamsSignOffsetAddr = PARAMS_SIGN_OFFSET;
	unsigned int uiParamsCrcOffsetAddr = PARAMS_CRC_OFFSET;
	unsigned int uiParamsPageSize = PARAMS_ZONE_PAGE_SIZE;
	unsigned int uiSaveParamsAddr = FOTA_PARAMS_ADDR; // 保存参数地址
	char cSingleParamLen  = MAX_PARAMS_LEN;           // 单个参数数据长度
	unsigned short int usiCRC16Val = 0;

	spi_flash_read(uiSaveParamsAddr, pucParamBuf, uiParamsPageSize);	   // 一次性将原有数据读出得到参数
	usiCRC16Val = Modbus_Caluation_CRC16(pucParamBuf, uiParamsPageSize-2); // 读取原有参数 (长度 + 参数)crc校验

	/* 检验保存数据crc值  */
	if((pucParamBuf[uiParamsCrcOffsetAddr] == HIGH(usiCRC16Val)) &&\
 		 (pucParamBuf[uiParamsCrcOffsetAddr+1] == LOW(usiCRC16Val)))
	{
		memcpy(pucSrc_tem,&pucParamBuf[(uiParamsOffsetAddr+(ucNum_tem*cSingleParamLen))], pucParamBuf[ucNum_tem]); // 得到读取值


		IOT_Printf("\r\n IOT_AddParam_Read() num = %d, len = %d src = ", ucNum_tem,pucParamBuf[ucNum_tem]);
		{for(int i=0; i<pucParamBuf[ucNum_tem]; i++)IOT_Printf(" %02x", pucSrc_tem[i]); IOT_Printf("\r\n");}
	}
	else // crc fail
	{
		spi_flash_erase_sector(FOTA_PARAMS_ADDR/PARAMS_ZONE_PAGE_SIZE); // !!! crc出错重新初始化所有参数
		IOT_Printf("IOT_AddParam_Read() num = %d CRC fail = %x , rewrite addparams again! \r\n", ucNum_tem, usiCRC16Val);
	}
}







