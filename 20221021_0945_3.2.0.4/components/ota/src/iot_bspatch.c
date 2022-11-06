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


#include "iot_bsfile_handle.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define BSPatch_DeBug 		0

static off_t unzip_data_pos;
static off_t unzip_data_maxlen;
static structpatch_data handle_data;
static Unzip_Ctrl_Data Patch_Unzip_Ctrl_Data;
static Patch_Status handle_data_status;
static int data_counter = 0;

/* 打补丁的处理(根据旧文件和补丁文件还原新文件) */
int Patchdata_handler(structpatch_data handle_data, FILE_ID *old_file, FILE_ID *new_file)
{
    uint8_t read_data;
    uint8_t write_data;
    int i;
    if(handle_data.patchctrl_data.patchdata1_len)
    {
        if(handle_data.patchctrl_data.patchdata1_pos != old_file->offset)
        {
            file_lseek(old_file, handle_data.patchctrl_data.patchdata1_pos);
        }
        for(i = 0; i < handle_data.patchctrl_data.patchdata1_len; i++)
        {
            if(file_read(old_file, &read_data, 1) != 1)
                return -1;
            write_data = read_data + handle_data.patchdata[i]; // 通过旧文件数据开始组装新文件
            if(file_write(new_file, &write_data, 1) != 1)
                return -1;
        }
    }
    if(handle_data.patchctrl_data.patchdata2_len) // 还原新文件，开始写入新文件数据到flash
    {
        if(file_write(new_file, &(handle_data.patchdata[handle_data.patchctrl_data.patchdata1_len]),  \
                      handle_data.patchctrl_data.patchdata2_len) != handle_data.patchctrl_data.patchdata2_len)
            return -1;
    }
    return 0;
}

/* 开始解压还原文件前，设置新文件最大长度并初始化其他变量 */
void Handle_Data_Init(off_t data)
{
    unzip_data_pos = 0;
    data_counter = 0;
    unzip_data_maxlen = data; // 得到新文件数大小长度值
    memset(&handle_data, 0, sizeof(structpatch_data));
    memset(&Patch_Unzip_Ctrl_Data, 0, sizeof(Unzip_Ctrl_Data));
    handle_data_status = Handle_Ctrl_Ctrl_Byte;// 置位-处理控制块-计算各个数据块大小(数据索引、 差分块、额外块大小)
}

/*
解压后的数据处理
还原差分数据
*/
int Unzipdata_Handle(uint8_t data, FILE_ID *old_file, FILE_ID *new_file)
{
    static int ctrl_size = 0;
    static int diff_size = 0;
    static int extra_size = 0;
    switch(handle_data_status){
    case Handle_Ctrl_Ctrl_Byte:          // 处理控制块-计算各个数据块大小
        ctrl_size = (data >> 6) + 1;     // 数据索引大小1
        diff_size = (data >> 3) & 0x07;  // 差分块大小2
        extra_size = data & 0x07;        // 额外块大小1
        handle_data_status = Handle_Ctrl_Index_Byte;
        data_counter = 0;
        break;
    case Handle_Ctrl_Index_Byte:         // 获取旧数据的索引
        switch(data_counter){
        case 0:
            Patch_Unzip_Ctrl_Data.Olddata_Index = data;
            data_counter = 1;
            break;
        case 1:
            Patch_Unzip_Ctrl_Data.Olddata_Index += data << 8;
            data_counter = 2;
            break;
        case 2:
            Patch_Unzip_Ctrl_Data.Olddata_Index += data << 16;
            data_counter = 3;
            break;
        case 3:
            Patch_Unzip_Ctrl_Data.Olddata_Index += data << 24;
            data_counter = 4;
            break;
        default:
            return -1;
        }
        if(data_counter == ctrl_size){
            handle_data.patchctrl_data.patchdata1_pos = Patch_Unzip_Ctrl_Data.Olddata_Index;
            handle_data.patchctrl_data.patchdata1_len = 0;
            handle_data.patchctrl_data.patchdata2_len = 0;

            if(diff_size){
                handle_data_status = Handle_Ctrl_DiffLen_Byte;
            }
            else if(extra_size){
                handle_data_status = Handle_Ctrl_ExtraLen_Byte;
            }
            else{
                handle_data_status = Handle_Ctrl_Ctrl_Byte;
            }
            data_counter = 0;
        }
        break;
    case Handle_Ctrl_DiffLen_Byte:       // 处理差分数据块
        switch(data_counter){
        case 0:
            Patch_Unzip_Ctrl_Data.Diffdata_Len = data;
            data_counter = 1;
            break;
        case 1:
            Patch_Unzip_Ctrl_Data.Diffdata_Len += data << 8;
            data_counter = 2;
            break;
        case 2:
            Patch_Unzip_Ctrl_Data.Diffdata_Len += data << 16;
            data_counter = 3;
            break;
        case 3:
            Patch_Unzip_Ctrl_Data.Diffdata_Len += data << 24;
            data_counter = 4;
            break;
        default:
            return -1;
        }
        if(data_counter == diff_size){
            if(extra_size){
                handle_data_status = Handle_Ctrl_ExtraLen_Byte;
            }
            else{
                handle_data_status = Handle_Unzip_Data;
            }
            data_counter = 0;
        }
        break;
    case Handle_Ctrl_ExtraLen_Byte:      // 处理额外数据块
        switch(data_counter){
        case 0:
            Patch_Unzip_Ctrl_Data.Extradata_Len = data;
            data_counter = 1;
            break;
        case 1:
            Patch_Unzip_Ctrl_Data.Extradata_Len += data << 8;
            data_counter = 2;
            break;
        case 2:
            Patch_Unzip_Ctrl_Data.Extradata_Len += data << 16;
            data_counter = 3;
            break;
        case 3:
            Patch_Unzip_Ctrl_Data.Extradata_Len += data << 24;
            data_counter = 4;
            break;
        default:
            return -1;
        }
        if(data_counter == extra_size){
            handle_data_status = Handle_Unzip_Data;
            data_counter = 0;
        }
        break;
    case Handle_Unzip_Data:            // 处理解压数据
        if(Patch_Unzip_Ctrl_Data.Diffdata_Len > 0){
            Patch_Unzip_Ctrl_Data.Diffdata_Len--;
            handle_data.patchctrl_data.patchdata1_len++;
            handle_data.patchdata[data_counter++] = data;
        }
        else if(Patch_Unzip_Ctrl_Data.Extradata_Len > 0){
            Patch_Unzip_Ctrl_Data.Extradata_Len--;
            handle_data.patchctrl_data.patchdata2_len++;
            handle_data.patchdata[data_counter++] = data;
        }

        if((Patch_Unzip_Ctrl_Data.Diffdata_Len <= 0) && (Patch_Unzip_Ctrl_Data.Extradata_Len <= 0))
        {
            handle_data_status = Handle_Ctrl_Ctrl_Byte;
            if(Patchdata_handler(handle_data, old_file, new_file) == -1)
            {
                return -1;
            }
            handle_data.patchctrl_data.patchdata1_pos += handle_data.patchctrl_data.patchdata1_len;
            handle_data.patchctrl_data.patchdata1_len = 0;
            handle_data.patchctrl_data.patchdata2_len = 0;
            data_counter = 0;
        }
        if(data_counter == PATCHDATA_LEN)
        {
            if(Patchdata_handler(handle_data, old_file, new_file) == -1)
            {
                return -1;
            }
            handle_data.patchctrl_data.patchdata1_pos += handle_data.patchctrl_data.patchdata1_len;
            handle_data.patchctrl_data.patchdata1_len = 0;
            handle_data.patchctrl_data.patchdata2_len = 0;
            data_counter = 0;
        }

        break;
    default:
        break;
    }
    return 0;
}



/*解压补丁文件,通过回调函数(解耦合)输出字节流数据
输入:旧文件、补丁、新文件文件描述符
输出:-1失败,1成功
*/
int BsPatch(FILE_ID *old_file, FILE_ID *patch_file, FILE_ID *new_file)
{
    uint8_t *data_buff;
    uint8_t zip_flag;
    uint8_t read_data;
    uint8_t out_data;
    off_t databuff_len;
    off_t databuff_pos;
    int index_pos;
    int match_len;
    uint8_t matchlen_compen;
    int Index_Mask;
    int Match_Mask;
    int ctrl_data;
    int Singledata_Len;
    uint8_t right_len;
    int i = 0;
    
#if BSPatch_DeBug
    printf("BsPatch() START!\n");
#endif

    file_lseek(patch_file, 4); // diff文件首地址开始-偏移4个地址 addr：0-3
    if(file_read(patch_file, &read_data, 1) != 1){// 获取newfile文件的总长度 addr：4-7
        return -1;
    } 
    ctrl_data = read_data;
    if(file_read(patch_file, &read_data, 1) != 1){
        return -1;
    }
    ctrl_data = ctrl_data | read_data << 8;
    if(file_read(patch_file, &read_data, 1) != 1){
        return -1;
    }
    ctrl_data = ctrl_data | read_data << 16;
    if(file_read(patch_file, &read_data, 1) != 1){
        return -1;
    }
    ctrl_data = ctrl_data | read_data << 24;
    Handle_Data_Init(ctrl_data); // 设置新文件最大长度，和初始化其他变量

    if(file_read(patch_file, &read_data, 1) != 1){// 获取申请动态内存压缩大小 addr：8-8
        return -1;
    }
    
    /*
			   2 3 5 9 17 33 KB(动态申请内存+1)
		       1 2 4 8 16 32 KB(动态申请内存)
	zip_flag = 0 1 2 3 4  5 
	*/
    //根据选择申请对应内存
    zip_flag = read_data;
    databuff_len = 1 << (10 + zip_flag);            // 举例 ：zip_flag=0,则动态内存申请为：1k，内存开辟越大解压越大，则反之 
//    data_buff = new uint8_t[databuff_len];        // del isen  C++格式
    data_buff = (uint8_t *)IOT_Malloc(databuff_len);// add isen，动态内存申请api，注意在不同的嵌入式平台实现方式不一样 
    
#if BSPatch_DeBug
    printf(" === BsPatch() ctrl_data: %02x-%d-%d-%d \r\n", ctrl_data,ctrl_data, zip_flag,databuff_len);
#endif

    databuff_pos = 0;
    if(file_read(patch_file, &read_data, 1) != 1){// 获取diff压缩段1数据长度 addr：9-9
        return -1;
    }
    data_buff[databuff_pos++] = out_data = read_data;	//data_buff[0] = 25
    if(Unzipdata_Handle(out_data, old_file, new_file) == -1){// 获取数据索引、差分块和额外块的大小 addr：10-out_data
        return -1;
    }
    databuff_pos = databuff_pos % databuff_len;

    while(file_read(patch_file, &read_data, 1) == 1)
    {
/*----------------可以直接使用的数据----------------------------------------------------------*/
        ctrl_data = read_data;
        if(ctrl_data >> 4 == 0x0f)// 高4位为1111 -- 低四位为长度减一的值 -- 8位不匹配数据长度
        {
            Singledata_Len = (ctrl_data & 0x0f) + 1; // 举例：单个数据的长度: 0xfF4&0x0f+1 -> 5
            for(i = 0; i < Singledata_Len; i++)
            {
                if(file_read(patch_file, &read_data, 1) != 1)
                {
                    return -1;
                }
                data_buff[databuff_pos++] = out_data = read_data;
                if(Unzipdata_Handle(out_data, old_file, new_file) == -1)// 打补丁算法,解析解压后的数据
                {
                    return -1;
                }
                databuff_pos = databuff_pos % databuff_len;
            }
            continue;
        }
/*----------------需要做匹配处理的数据----------------------------------------------------------*/
	    else if(ctrl_data >> 7 == 0)// 高一位为0作为标识 -- 64索引,最大位2匹配 -- 8位索引加匹配数据
	    {
            right_len = 1;
            matchlen_compen = 2;
            Index_Mask = 0x3F << 1;
            Match_Mask = 0x1;
        }
        else if(ctrl_data >> 6 == 2)// 高两位为10作为标识 -- 16位索引加匹配数据	8
        {
            if(file_read(patch_file, &read_data, 1) != 1)
            {
                return -1;
            }
            ctrl_data = ((ctrl_data & 0x3f) << 8) | read_data;	//14位有效位
            right_len = 4 - (int)(zip_flag / 2);
            matchlen_compen = 3;
            Index_Mask = ((1 << (10 + (int)(zip_flag / 2))) - 1) << (4 - (int)(zip_flag / 2)); // 索引掩码
            Match_Mask = (1 << (4 - (int)(zip_flag / 2))) - 1;                                 // 匹配掩码
        }
        else if(ctrl_data >> 5 == 6)// 高三位为110作为标识 -- 24位索引加匹配数据	C
        {
            if(file_read(patch_file, &read_data, 1) != 1)
            {
                return -1;
            }
            ctrl_data = ((ctrl_data & 0x1f) << 16) | (read_data << 8);
            if(file_read(patch_file, &read_data, 1) != 1)
            {
                return -1;
            }
            ctrl_data = ctrl_data | read_data;
            right_len = 10 - zip_flag;
            matchlen_compen = 4;
            Index_Mask = ((1 << (10 + zip_flag)) - 1) << (10 - zip_flag);
            Match_Mask = (1 << (10 - zip_flag)) - 1;
        }
        else                        //高四位为1110作为标识 -- 32位索引加匹配数据 E
        {
            if(file_read(patch_file, &read_data, 1) != 1)
            {
                return -1;
            }
            ctrl_data = ((ctrl_data & 0xf) << 24) | (read_data << 16);
            if(file_read(patch_file, &read_data, 1) != 1)
            {
                return -1;
            }
            ctrl_data = ctrl_data | (read_data << 8);
            if(file_read(patch_file, &read_data, 1) != 1)
            {
                return -1;
            }
            ctrl_data = ctrl_data | read_data;
            right_len = 18 - zip_flag;
            matchlen_compen = 5;
            Index_Mask = ((1 << (10 + zip_flag)) - 1) << (18 - zip_flag);
            Match_Mask = (1 << (18 - zip_flag)) - 1;
        }

        index_pos = (ctrl_data & Index_Mask) >> right_len;
        match_len = (ctrl_data & Match_Mask) + matchlen_compen;
        index_pos = (databuff_pos + databuff_len - index_pos - 1) % databuff_len;
        for (i = 0; i < match_len; i++)
        {
            data_buff[databuff_pos++] = out_data = data_buff[index_pos++];
            if(Unzipdata_Handle(out_data, old_file, new_file) == -1)
            {
                return -1;
            }
            index_pos = index_pos % databuff_len;
            databuff_pos = databuff_pos % databuff_len;
        }
    }

    //delete []data_buff; // del isen  C++格式
    IOT_Free(data_buff);  // add isen
    data_buff = NULL; 
    
#if BSPatch_DeBug
    printf("BsPatch() END!\n");
#endif
    return 1;
}










