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


#define IOT_BSFILE_HANDLE_GLOBALS
#include "iot_bsfile_handle.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>




/*
功能:绑定Flash内文件的地址和长度等信息至文件描述符并申请写入缓存
输入:
	File_Address:文件存放地址（oldfile/pacthfile/newfile）
	mode:打开模式
输出:
 	file:文件描述符

 注意：oldfile、pacthfile、newfile 的flash地址最好都存放在相同的flash存储芯片上
*/
FILE_ID* file_open(EFILETYPE eFileType_tem, uint8_t mode)
{
    FILE_INFORMATION file_info;
    FILE_ID *file;

    if(Get_File_Information(eFileType_tem, &file_info) == -1)
    {
        return NULL;
    }
    if(file_info.file_status == FILE_OPEN){
        return NULL;
    }
    if(mode == 0)
        return NULL;

    file = (FILE_ID *)IOT_Malloc(sizeof(FILE_ID));

    if(file == NULL)
        return NULL;   

    file->file_address = file_info.file_address;
    file->fileinfo_address = 0;
    file->len = file_info.len;
    file->offset = 0;
    file->mode = mode;
    file->buff_change = 0;
    file->buff_pos = file->file_address - ((file->file_address) & (~(FLASH_ADDRESS)(FLASH_PAGE_SIZE - 1)));
    file->data_buff = (uint8_t *)IOT_Malloc(FLASH_PAGE_SIZE);
    file->ucFileType = eFileType_tem; // 文件类型
    if(File_Data_Read(file->ucFileType,\
    		          (file->file_address + file->offset) & (~(FLASH_ADDRESS)(FLASH_PAGE_SIZE - 1)),   \
                      file->data_buff,     \
                      FLASH_PAGE_SIZE) != FLASH_PAGE_SIZE) // 确认flash(old、pacth、new file)中是否有 有效数据
    {
        IOT_Free(file->data_buff);
        IOT_Free(file);
        return NULL;
    }
    file_info.file_status = FILE_OPEN;
    
    return file;
}


int file_flush(FILE_ID *file)
{
    if(file->buff_change)// 判断是否flash是否需要写入数据
    {
        if(File_Data_Write(file->ucFileType,\
        		          (file->file_address + file->offset - 1) & (~(FLASH_ADDRESS)(FLASH_PAGE_SIZE - 1)),   \
                          file->data_buff, FLASH_PAGE_SIZE) != FLASH_PAGE_SIZE)
            return -1;
        file->buff_change = 0;
    }
    return 0;
}

int file_lseek(FILE_ID *file, off_t offset)
{
    if(file->offset == offset)
        return file->len - file->offset;

    if(((file->file_address + file->offset) & (~(FLASH_ADDRESS)(FLASH_PAGE_SIZE - 1))) == \
            ((file->file_address + offset) & (~(FLASH_ADDRESS)(FLASH_PAGE_SIZE - 1)))){
        if(file->mode & FILEWRITE_ONLY){
            file->offset = offset;
            if(file->len < file->offset){
                file->len = file->offset;
            }
        }
        if(file->len < file->offset){
            return -1;
        }
        file->offset = offset;
        file->buff_pos = (file->file_address + file->offset) -   \
                ((file->file_address + file->offset) & (~(FLASH_ADDRESS)(FLASH_PAGE_SIZE - 1)));
    }
    else{
        if(file_flush((file)) == -1)
            return -1;
        if(file->mode & FILEWRITE_ONLY){
            file->buff_change = 0;
            if(File_Data_Read(file->ucFileType,\
            		          (file->file_address + offset) & (~(FLASH_ADDRESS)(FLASH_PAGE_SIZE - 1)),   \
                              file->data_buff, \
							  FLASH_PAGE_SIZE) != FLASH_PAGE_SIZE)
            {
                File_Data_Read(file->ucFileType,\
                		       (file->file_address + file->offset) & (~(FLASH_ADDRESS)(FLASH_PAGE_SIZE - 1)),   \
                                file->data_buff, \
								FLASH_PAGE_SIZE);
                return -1;
            }
            else{
                file->offset = offset;
                file->buff_pos = (file->file_address + file->offset) -  \
                        ((file->file_address + file->offset) & (~(FLASH_ADDRESS)(FLASH_PAGE_SIZE - 1)));
                if(file->offset > file->len)
                    file->len = file->offset;
            }
        }
        if(file->mode & FILEREAD_ONLY){
            if(file->offset != offset){
                if(offset > file->len){
                    return -1;
                }
                else if(offset == file->len){
                    file->offset = offset;
                }
                else{
                    if(File_Data_Read(file->ucFileType,\
                    		          (file->file_address + offset) & (~(FLASH_ADDRESS)(FLASH_PAGE_SIZE - 1)),   \
                                      file->data_buff,     \
                                      FLASH_PAGE_SIZE) != FLASH_PAGE_SIZE){
                        File_Data_Read(file->ucFileType,\
                        		       (file->file_address + file->offset) & (~(FLASH_ADDRESS)(FLASH_PAGE_SIZE - 1)),   \
									   file->data_buff,     \
									   FLASH_PAGE_SIZE);
                        return -1;
                    }
                    else{
                        file->offset = offset;
                        file->buff_pos = (file->file_address + file->offset) -  \
                                ((file->file_address + file->offset) & (~(FLASH_ADDRESS)(FLASH_PAGE_SIZE - 1)));
                    }
                }
            }
        }
    }
    return (file->len - file->offset);
}

int file_read(FILE_ID *file, uint8_t *data, off_t len)
{
    int real_len = 0;
    int buff_len;

    if((!(file->mode & FILEREAD_ONLY)) || file->offset >= file->len)
        return -1;
    len = (len > (file->len - file->offset)) ? (file->len - file->offset) : len;
    while(len > 0)
    {
        buff_len = (len > (FLASH_PAGE_SIZE - file->buff_pos)) ?   \
                    (FLASH_PAGE_SIZE - file->buff_pos) : len;
        memcpy(data + real_len, &file->data_buff[file->buff_pos], buff_len);
        file->buff_pos += buff_len;
        file->offset += buff_len;
        real_len += buff_len;
        len -= buff_len;
        if(file->buff_pos >= FLASH_PAGE_SIZE)
        {
            if(file_flush(file) == -1)
                return -1;
            if((file->offset < file->len) || (file->mode & FILEWRITE_ONLY))
            {
                if(File_Data_Read(file->ucFileType,\
                		          (file->file_address + file->offset) & (~(FLASH_ADDRESS)(FLASH_PAGE_SIZE - 1)), /*确保每次读取的位置4K对齐*/\
                                  file->data_buff,     \
                                  FLASH_PAGE_SIZE) != FLASH_PAGE_SIZE)
                {
                	/*读取失败*/
                    file->buff_pos -= buff_len;
                    file->offset -= buff_len;
                    File_Data_Read(file->ucFileType,\
                    		       (file->file_address + file->offset) & (~(FLASH_ADDRESS)(FLASH_PAGE_SIZE - 1)),   \
                                   file->data_buff, FLASH_PAGE_SIZE);
                    return -1;
                }
                file->buff_pos = 0;
            }

        }
    }
    
    return real_len;
}

int file_write(FILE_ID *file, uint8_t *data, off_t len)
{
    int real_len = 0;
    int buff_len;
    if(!(file->mode & FILEWRITE_ONLY))
        return -1;
    while(len > 0)
    {
        buff_len = (len > (FLASH_PAGE_SIZE - file->buff_pos)) ?   \
                    (FLASH_PAGE_SIZE - file->buff_pos) : len;
        memcpy(&file->data_buff[file->buff_pos], data + real_len, buff_len);
        file->buff_pos += buff_len;
        file->offset += buff_len;
        real_len += buff_len;
        len -= buff_len;
        file->buff_change = 1;  // 标记可以flash可以写入数据，file_flush()中调用
        if(file->offset > file->len)
            file->len = file->offset;
        if(file->buff_pos >= FLASH_PAGE_SIZE)
        {
            if(file_flush(file) == -1) // 新文件数据包写入flash
                return -1;
            if(File_Data_Read(file->ucFileType,\
            		          (file->file_address + file->offset) & (~(FLASH_ADDRESS)(FLASH_PAGE_SIZE - 1)),\
                              file->data_buff, FLASH_PAGE_SIZE) != FLASH_PAGE_SIZE)
            {
                file->buff_pos -= buff_len;
                file->offset -= buff_len;
                File_Data_Read(file->ucFileType,\
                		       (file->file_address + file->offset) & (~(FLASH_ADDRESS)(FLASH_PAGE_SIZE - 1)),\
                               file->data_buff, FLASH_PAGE_SIZE);
                return -1;
            }
            file->buff_pos = 0;
        }
    }
    return real_len;
}

int file_close(FILE_ID *file)
{
    FILE_INFORMATION file_info;

    file_flush(file);
    IOT_Free(file->data_buff);
    file_info.file_address = file->file_address;
    file_info.len = file->len;
    file_info.file_status = FILE_CLOSE;

    if(Save_File_Information(file->ucFileType, &file_info) == -1)
    {
        return -1;
    }
    IOT_Free(file);
    return 0;
}
