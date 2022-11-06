/*-
 * Copyright 2022 isen
 * All rights reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted providing that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define IOT_BSDIFF_API_GLOBALS
#include "iot_bsdiff_api.h"
#include "iot_bsfile_handle.h"
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_spi_flash.h"
//#include "iot_params.h"
//#include "iot_fota.h"

#include "iot_spi_flash.h"
#include "iot_crc32.h"

#define   BSDIFF_DEBUG     0


/* 设置读写flash api */
//差分到内部flash
#if 1
#define spi_old_file_flash_read(src , dstv, size)      spi_flash_read((src),(dstv), (size))	//旧文件所在flash 读取api
#define spi_patch_file_flash_read(src , dstv, size)    IOT_Mutex_SPI_FLASH_BufferRead((dstv), (src), (size))	//补丁文件文件所在flash 读取api
#define spi_new_file_flash_read(src , dstv, size)      spi_flash_read((src),(dstv), (size))	//存放新文件所在flash 读取api

#define spi_new_file_flash_erase_range(start_addr, size)  spi_flash_erase_range((start_addr), (size)) //存放新文件所在flash 擦写flash api
#define spi_new_file_flash_write(src , dstv, size)        spi_flash_write((src),(dstv), (size))			//存放新文件所在flash 写数据api
#else	//差分到外部flash
#define spi_old_file_flash_read(src , dstv, size)      spi_flash_read((src),(dstv), (size))	//旧文件所在flash 读取api
#define spi_patch_file_flash_read(src , dstv, size)    spi_flash_read((src),(dstv), (size))	//补丁文件所在flash 读取api
#define spi_new_file_flash_read(src , dstv, size)      W25QXX_Read((dstv), (src), (size))	//存放新文件所在flash 读取api

#define spi_new_file_flash_erase_range(start_addr, size)  IOT_spi_flash_erase_range((start_addr), (size)) //存放新文件所在flash 擦写flash api
#define spi_new_file_flash_write(src , dstv, size)        SPI_FLASH_WriteBuff((dstv),(src), (size))			//存放新文件所在flash 写数据api

#endif



/******************************************************************************
 * FunctionName : IOT_FileInfo_Init
 * Description  : 初始化old/patch/new 文件的flash保存地址和长度值
 * Parameters   : EFILETYPE eType_tem     : 文件类型(old/patch/new)EFILETYPE_OLD/EFILETYPE_PATCH/EFILETYPE_NEW
 *                unsigned int uiAddr_tem : 文件的flash保存地址
 *                long lLen_tem           : 文件的长度值
 *                uint8_t ucStatus_tem    : 文件状态 FILE_OPEN/FILE_CLOSE
 * Returns      : none
 * Notice       : 1、该函数api被用户函数调用，下载完文件进行查分解压数据前调用设置
*******************************************************************************/
void IOT_FileInfo_Init(EFILETYPE eType_tem, unsigned int uiAddr_tem, long lLen_tem, uint8_t ucStatus_tem)
{
	if(eType_tem == EFILETYPE_OLD)
	{
		g_SOldFileInfo.file_address = uiAddr_tem;
		g_SOldFileInfo.len = lLen_tem;
		g_SOldFileInfo.file_status = ucStatus_tem;
	}
	else if(eType_tem == EFILETYPE_PATCH)
	{
		g_SPatchFileInfo.file_address = uiAddr_tem;
		g_SPatchFileInfo.len = lLen_tem;
		g_SPatchFileInfo.file_status = ucStatus_tem;
	}
	else if(eType_tem == EFILETYPE_NEW)
	{
		g_SNewFileInfo.file_address = uiAddr_tem;
		g_SNewFileInfo.len = lLen_tem;
		g_SNewFileInfo.file_status = ucStatus_tem;
	}

#if	BSDIFF_DEBUG
	printf("IOT_FileInfo_Init(), %d-0x%02x-%d-%d \r\n", eType_tem, uiAddr_tem,lLen_tem,ucStatus_tem);
#endif
}

/******************************************************************************
 * FunctionName : IOT_FileInfo_Get_i
 * Description  : 获取old/patch/new 文件的flash保存地址和长度值、文件状态
 * Parameters   : EFILETYPE eType_tem     : 文件类型(old/patch/new)EFILETYPE_OLD/EFILETYPE_PATCH/EFILETYPE_NEW
 *                FILE_INFORMATION *psFileInfo_tem: 文件信息结构体指针
 * Returns      : none
 * Notice       : 1、该函数api被用户函数调用，下载完文件进行查分解压数据前调用设置
*******************************************************************************/
int IOT_FileInfo_Get_i(EFILETYPE eType_tem, FILE_INFORMATION *psFileInfo_tem)
{
	if(eType_tem == EFILETYPE_OLD)
	{
		psFileInfo_tem->file_address = g_SOldFileInfo.file_address;
		psFileInfo_tem->len = g_SOldFileInfo.len;
		psFileInfo_tem->file_status = g_SOldFileInfo.file_status;
	}
	else if(eType_tem == EFILETYPE_PATCH)
	{
		psFileInfo_tem->file_address = g_SPatchFileInfo.file_address;
		psFileInfo_tem->len = g_SPatchFileInfo.len;
		psFileInfo_tem->file_status = g_SPatchFileInfo.file_status;
	}
	else if(eType_tem == EFILETYPE_NEW)
	{
		psFileInfo_tem->file_address = g_SNewFileInfo.file_address;
		psFileInfo_tem->len = g_SNewFileInfo.len;
		psFileInfo_tem->file_status = g_SNewFileInfo.file_status;
	}

#if	BSDIFF_DEBUG
	printf("IOT_FileInfo_Get_i(), %d-0x%02x-%d-%d \r\n", eType_tem,
														 psFileInfo_tem->file_address,
														 psFileInfo_tem->len,
														 psFileInfo_tem->file_status);
#endif
	return 1;
}

/******************************************************************************
 * FunctionName : IOT_FileInfo_Set_i
 * Description  : 设置old/patch/new 文件的flash保存地址和长度值、文件状态
 * Parameters   : EFILETYPE eType_tem     : 文件类型(old/patch/new)EFILETYPE_OLD/EFILETYPE_PATCH/EFILETYPE_NEW
 *                FILE_INFORMATION *psFileInfo_tem: 文件信息结构体指针
 * Returns      : none
 * Notice       : 1、该函数api被用户函数调用，下载完文件进行查分解压数据前调用设置
*******************************************************************************/
int IOT_FileInfo_Set_i(EFILETYPE eType_tem, FILE_INFORMATION *psFileInfo_tem)
{
	if(eType_tem == EFILETYPE_OLD)
	{
		g_SOldFileInfo.file_address = psFileInfo_tem->file_address;
		g_SOldFileInfo.len = psFileInfo_tem->len;
		g_SOldFileInfo.file_status = psFileInfo_tem->file_status;
	}
	else if(eType_tem == EFILETYPE_PATCH)
	{
		g_SPatchFileInfo.file_address = psFileInfo_tem->file_address;
		g_SPatchFileInfo.len = psFileInfo_tem->len;
		g_SPatchFileInfo.file_status = psFileInfo_tem->file_status;
	}
	else if(eType_tem == EFILETYPE_NEW)
	{
		g_SNewFileInfo.file_address = psFileInfo_tem->file_address;
		g_SNewFileInfo.len = psFileInfo_tem->len;
		g_SNewFileInfo.file_status = psFileInfo_tem->file_status;
	}

#if	BSDIFF_DEBUG
	printf("IOT_FileInfo_Set_i(), %d-0x%02x-%d-%d \r\n", eType_tem,
														 psFileInfo_tem->file_address,
														 psFileInfo_tem->len,
														 psFileInfo_tem->file_status);
#endif
	return 1;
}

/******************************************************************************
 * FunctionName : IOT_File_Read_i
 * Description  : 读取old/patch/new 文件
 * Parameters   : EFILETYPE eType_tem : 文件类型(old/patch/new)EFILETYPE_OLD/EFILETYPE_PATCH/EFILETYPE_NEW
 *                unsigned int address: 文件地址
 *                uint8_t *data       : 单次读取文件存放buf
 *                int data_size       : 单次读取文件长度
 * Returns      : none
 * Notice       : 1、该函数api子文件解压过程中被调用
*******************************************************************************/
int IOT_File_Read_i(EFILETYPE eType_tem, unsigned int address, uint8_t *data, int data_size)
{
	if(eType_tem == EFILETYPE_OLD)
	{
		spi_old_file_flash_read(address, data, data_size);
	}
	else if(eType_tem == EFILETYPE_PATCH)
	{
		spi_patch_file_flash_read(address, data, data_size);
	}
	else if(eType_tem == EFILETYPE_NEW)
	{
		spi_new_file_flash_read(address,(uint8_t *) data,data_size);
	}

#if	BSDIFF_DEBUG
	printf("IOT_File_Read_i(), %d-0x%02x-%d \r\n", eType_tem, address, data_size);
#endif
    return data_size;
}

/******************************************************************************
 * FunctionName : IOT_File_Write_i
 * Description  : 写入old/patch/new 文件
 * Parameters   : EFILETYPE eType_tem : 文件类型(old/patch/new)EFILETYPE_OLD/EFILETYPE_PATCH/EFILETYPE_NEW
 *                unsigned int address: 文件地址
 *                uint8_t *data       : 单次读取文件存放buf
 *                int data_size       : 单次读取文件长度
 * Returns      : none
 * Notice       : 1、该函数api子文件解压过程中被调用
*******************************************************************************/
int IOT_File_Write_i(EFILETYPE eType_tem, unsigned int address, uint8_t *data, int data_size)
{
	int err = 0;
	static char s_cEraseFlag = 0;

	if(eType_tem == EFILETYPE_OLD)
	{}
	else if(eType_tem == EFILETYPE_PATCH)
	{}
	else if(eType_tem == EFILETYPE_NEW)
	{
		if(s_cEraseFlag == 0) // 第一次写时进行擦除flash区
		{
			s_cEraseFlag = 1;
			spi_new_file_flash_erase_range(address, ((g_SNewFileInfo.len/4096)+1)*4096);
#if	BSDIFF_DEBUG
			printf("spi_flash_erase_sector %d-%d-%d", err, address/4096, ((g_SNewFileInfo.len/4096)+1)*4096);
#endif
		}

		err = spi_new_file_flash_write(address,data,data_size);
#if	BSDIFF_DEBUG
		printf("\r\nflash_write() NewFileLenAll = %d \r\n", err);
#endif
	}

#if	BSDIFF_DEBUG
	printf("IOT_File_Write_i(), address: %02x-%d \r\n", address, data_size);
#endif
	vTaskDelay(10 / portTICK_PERIOD_MS);// 跑系统时，需要定时做任务调度延时

    return data_size;
}

/******************************************************************************
 * FunctionName : IOT_BsPatch_Run
 * Description  : 查分解压执行
 * Parameters   : void
 * Returns      : none
 * Notice       : 1、升级文件下载完成校验通过后执行该函数开始进行文件解压还原新文件
*******************************************************************************/
void IOT_BsPatch_Run(BsPatchFile_t FileInfo_tem)
{
   /******************************
   !!! 注意：本例程可以直接移植大带flash的muc或rom比较大的mcu上做查分升级

   **************************/

   FILE_ID *File_id_Old, *File_id_diff, *File_id_New; // 定义对应文件结构体id值，解压还原文件时用到

   /////////调试用//////////
   /* 设置对应文件 flash地址-需要根据实际存放文件位置赋值 和 长度值--需要根据实际大小赋值 */
   IOT_FileInfo_Init(EFILETYPE_OLD, FileInfo_tem.uiOldFileAddr, FileInfo_tem.uiOldFileSize, FILE_CLOSE);
   IOT_FileInfo_Init(EFILETYPE_PATCH, FileInfo_tem.uiPatchFileAddr, FileInfo_tem.uiPatchFileSize, FILE_CLOSE);
   IOT_FileInfo_Init(EFILETYPE_NEW, FileInfo_tem.uiNewFileAddr, FileInfo_tem.uiNewFileSize, FILE_CLOSE);
   /////////调试用//////////

   /* 解压前打开对应文件，获取flash地址、长度等 */
   File_id_Old = file_open(EFILETYPE_OLD, FILEREAD_ONLY);
   File_id_diff = file_open(EFILETYPE_PATCH, FILEREAD_ONLY);
   File_id_New = file_open(EFILETYPE_NEW, FILEWRITE_ONLY);
   
   /* 开始解压还原文件 */
   BsPatch(File_id_Old, File_id_diff, File_id_New); // 这里是死循环，跑系统注意看门狗超时和加延时做任务调度 -- 可以在IOT_File_Write_i()做系统延时
   

   /* 解压完成，关闭文件 */
   file_close(File_id_Old);
   file_close(File_id_diff);
   file_close(File_id_New);
   
#if	BSDIFF_DEBUG
   printf("IOT_BsPatch_Run() end!\n");
#endif

}


