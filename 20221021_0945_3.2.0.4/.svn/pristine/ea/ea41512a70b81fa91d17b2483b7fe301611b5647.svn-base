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
 *
 * File name: iot_bsdiff_api.h
 *
 * Description:
 *
 * Author: isen
 * Date:2022/01/19
 */

#ifndef IOT_BSDIFF_API_H
#define IOT_BSDIFF_API_H

#ifdef IOT_BSDIFF_API_GLOBALS
#define IOT_BSDIFF_API_GLOBAL
#else
#define IOT_BSDIFF_API_GLOBAL extern
#endif

#include <stdlib.h>

typedef unsigned char uint8_t;


typedef long    off_t;
typedef unsigned int FLASH_ADDRESS;
#define EEPROM_ADDRESS FLASH_ADDRESS


#define FLASH_PAGE_SIZE 4096  // 256 -- 4096  // 根据mcu - ram大小开辟空间

typedef enum _EFILETYPE{
	EFILETYPE_NULL = 0,
	EFILETYPE_OLD = 1,
	EFILETYPE_PATCH = 2,
	EFILETYPE_NEW = 3,
}EFILETYPE;

typedef struct{
    unsigned int file_address;
    unsigned int fileinfo_address;
    uint8_t mode;
    long len;
    long offset;
    uint8_t *data_buff;
    uint8_t buff_change;
    long buff_pos;
    EFILETYPE ucFileType;  // 文件 old/patch/new file
} FILE_ID;



typedef struct{
    unsigned int file_address;
    long len;
    uint8_t file_status;
} FILE_INFORMATION;

IOT_BSDIFF_API_GLOBAL FILE_INFORMATION g_SOldFileInfo, g_SPatchFileInfo, g_SNewFileInfo; // old/patch/new file淇℃伅


typedef struct{
	uint32_t uiOldFileAddr;
	uint32_t uiOldFileSize;
	uint32_t uiPatchFileAddr;
	uint32_t uiPatchFileSize;
	uint32_t uiNewFileAddr;
	uint32_t uiNewFileSize;

}BsPatchFile_t;


#define IOT_Malloc(a)  malloc(a)
#define IOT_Free(a)    free(a)


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
void IOT_FileInfo_Init(EFILETYPE eType_tem, unsigned int uiAddr_tem, long lLen_tem, uint8_t ucStatus_tem);

/******************************************************************************
 * FunctionName : IOT_FileInfo_Get_i
 * Description  : 获取old/patch/new 文件的flash保存地址和长度值、文件状态
 * Parameters   : EFILETYPE eType_tem     : 文件类型(old/patch/new)EFILETYPE_OLD/EFILETYPE_PATCH/EFILETYPE_NEW
 *                FILE_INFORMATION *psFileInfo_tem: 文件信息结构体指针
 * Returns      : none
 * Notice       : 1、该函数api被用户函数调用，下载完文件进行查分解压数据前调用设置
*******************************************************************************/
int IOT_FileInfo_Get_i(EFILETYPE eType_tem, FILE_INFORMATION *psFileInfo_tem);

/******************************************************************************
 * FunctionName : IOT_FileInfo_Set_i
 * Description  : 设置old/patch/new 文件的flash保存地址和长度值、文件状态
 * Parameters   : EFILETYPE eType_tem     : 文件类型(old/patch/new)EFILETYPE_OLD/EFILETYPE_PATCH/EFILETYPE_NEW
 *                FILE_INFORMATION *psFileInfo_tem: 文件信息结构体指针
 * Returns      : none
 * Notice       : 1、该函数api被用户函数调用，下载完文件进行查分解压数据前调用设置
*******************************************************************************/
int IOT_FileInfo_Set_i(EFILETYPE eType_tem, FILE_INFORMATION *psFileInfo_tem);

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
int IOT_File_Read_i(EFILETYPE eType_tem, unsigned int address, uint8_t *data, int data_size);

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
int IOT_File_Write_i(EFILETYPE eType_tem, unsigned int address, uint8_t *data, int data_size);


#define Get_File_Information(FileType_tem, FileInfo) IOT_FileInfo_Get_i((FileType_tem), (FileInfo))
#define Save_File_Information(FileType_tem, FileInfo) IOT_FileInfo_Set_i((FileType_tem), (FileInfo))
#define File_Data_Read(FileType_tem, address, data, data_size) IOT_File_Read_i((FileType_tem), (address), (data), (data_size))
#define File_Data_Write(FileType_tem, address, data, data_size) IOT_File_Write_i((FileType_tem), (address), (data), (data_size))

void IOT_BsPatch_Run(BsPatchFile_t FileInfo_tem);
//void IOT_BsPatch_Run(void);

#endif // ROM_API_H
