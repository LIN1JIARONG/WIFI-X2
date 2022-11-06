/*-
 * Copyright 2022   isen
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
 * File name: iot_bsfile_handle.h
 *
 * Description:
 *
 * Author: 2022 isen
 * Date:2022/01/19
 */



#ifndef IOT_BSFILE_HANDLE_H
#define IOT_BSFILE_HANDLE_H

#ifdef IOT_BSFILE_HANDLE_GLOBALS
#define IOT_BSFILE_HANDLE_GLOBAL
#else
#define IOT_BSFILE_HANDLE_GLOBAL extern
#endif

#include "iot_bsdiff_api.h"


#define PATCHDATA_LEN 256

#define FILEREAD_ONLY 0X01
#define FILEWRITE_ONLY 0X02
#define FILEREAD_WRITE 0X03
#define FILE_OPEN   0x01
#define FILE_CLOSE  0x0

typedef struct{
    off_t patchdata1_pos;
    off_t patchdata1_len;
    off_t patchdata2_len;
} patch_ctrl_data;

typedef struct{
    patch_ctrl_data patchctrl_data;
    uint8_t patchdata[PATCHDATA_LEN];
} structpatch_data;

typedef struct{
    off_t Olddata_Index;
    off_t Diffdata_Len;
    off_t Extradata_Len;
} Unzip_Ctrl_Data;

typedef enum{
    Handle_Ctrl_Ctrl_Byte,
    Handle_Ctrl_Index_Byte,
    Handle_Ctrl_DiffLen_Byte,
    Handle_Ctrl_ExtraLen_Byte,
    Handle_Unzip_Data,
    Handle_Extra_Data
} Patch_Status;

/*************************************************
Function: int BsPatch(FILE_ID *old_file, FILE_ID *patch_file, FILE_ID *new_file)
Description: 用于对原来代码打补丁生成新的代码
Input:
    old_file: 原来数据的文件指针。
    patch_file: 补丁数据的文件指针
    new_file: 生成新数据放置的文件指针
Output:
Return:
    success: 返回 1
    fail: 返回 -1
Others: 无
*************************************************/
int BsPatch(FILE_ID *old_file, FILE_ID *patch_file, FILE_ID *new_file);


/*************************************************
Function: FILE_ID* file_open(EEPROM_ADDRESS fileinfo_address, uint8_t mode)
Description: 以mode格式打开文件，并改变文件信息的文件状态为打开。
Input:
    fileinfo_address: 要打开的文件的信息位置
    mode: 打开格式（FILEREAD_ONLY,FILEWRITE_ONLY）
Output:
Return:
    success: 指向该文件信息的FILE_ID指针
    fail: 返回 NULL
Others: 入口第一个参数一般存于eeprom或rom，主要存储数据起始地址和长度。且返回的FILE_ID
    指针指向的空间为函数内部申请，当不需要时，需要调用file_close来释放掉这个空间。
*************************************************/
FILE_ID* file_open(EFILETYPE eFileType_tem, uint8_t mode);


/*************************************************
Function: int file_flush(FILE_ID *file)
Description: 冲洗缓冲，同步缓冲和flash中的数据
Input:
    file: 指向该文件信息的FILE_ID指针
Output:
Return:
    success: 返回0
    fail: 返回-1
Others: 无
*************************************************/
int file_flush(FILE_ID *file);


/*************************************************
Function: int file_lseek(FILE_ID *file, off_t offset)
Description: 更改文件指针，指向我们需要的偏移地址
Input:
    file: 指向该文件信息的FILE_ID指针
    offset: 文件内部指针需要偏移的地址
Output:
Return:
    success: 返回该偏移量后面剩余文件长度
    fail: 返回-1
Others: 无
*************************************************/
int file_lseek(FILE_ID *file, off_t offset);


/*************************************************
Function: int file_read(FILE_ID *file, uint8_t *data, off_t len)
Description: 从文件中读出len长度的数据放到data指向地址，如果剩余长度小
    于len，则返回实际读取长度。
Input:
    file: 指向该文件信息的FILE_ID指针
    data: 读取的数据放置的地址
    len：要读取的数据长度
Output:
Return:
    success: 返回实际读取的长度
    fail: 返回-1
Others: 无
*************************************************/
int file_read(FILE_ID *file, uint8_t *data, off_t len);


/*************************************************
Function: int file_write(FILE_ID *file, uint8_t *data, off_t len)
Description: 向文件中写入len长度的数据。
Input:
    file: 指向该文件信息的FILE_ID指针
    data: 写入的数据放置的地址
    len：要写入的数据长度
Output:
Return:
    success: 返回读取的长度
    fail: 返回-1
Others: 当文件可写时，如果文件剩余长度小于要写入的长度，则会继续写入，并增大
    文件长度。当以只写模式打开文件时，会截断文件长度为0。
*************************************************/
int file_write(FILE_ID *file, uint8_t *data, off_t len);


/*************************************************
Function: int file_close(FILE_ID *file)
Description: 关闭文件，释放空间，并改变文件状态。
Input:
    file: 指向该文件信息的FILE_ID指针
Output:
Return:
    success: 返回0
    fail: 返回-1
Others: 文件打开后，一定要调用close，否则分配的空间不会被释放。
*************************************************/
int file_close(FILE_ID *file);



#endif // FILE_HANDLER_H
