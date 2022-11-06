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

/* �򲹶��Ĵ���(���ݾ��ļ��Ͳ����ļ���ԭ���ļ�) */
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
            write_data = read_data + handle_data.patchdata[i]; // ͨ�����ļ����ݿ�ʼ��װ���ļ�
            if(file_write(new_file, &write_data, 1) != 1)
                return -1;
        }
    }
    if(handle_data.patchctrl_data.patchdata2_len) // ��ԭ���ļ�����ʼд�����ļ����ݵ�flash
    {
        if(file_write(new_file, &(handle_data.patchdata[handle_data.patchctrl_data.patchdata1_len]),  \
                      handle_data.patchctrl_data.patchdata2_len) != handle_data.patchctrl_data.patchdata2_len)
            return -1;
    }
    return 0;
}

/* ��ʼ��ѹ��ԭ�ļ�ǰ���������ļ���󳤶Ȳ���ʼ���������� */
void Handle_Data_Init(off_t data)
{
    unzip_data_pos = 0;
    data_counter = 0;
    unzip_data_maxlen = data; // �õ����ļ�����С����ֵ
    memset(&handle_data, 0, sizeof(structpatch_data));
    memset(&Patch_Unzip_Ctrl_Data, 0, sizeof(Unzip_Ctrl_Data));
    handle_data_status = Handle_Ctrl_Ctrl_Byte;// ��λ-������ƿ�-����������ݿ��С(���������� ��ֿ顢������С)
}

/*
��ѹ������ݴ���
��ԭ�������
*/
int Unzipdata_Handle(uint8_t data, FILE_ID *old_file, FILE_ID *new_file)
{
    static int ctrl_size = 0;
    static int diff_size = 0;
    static int extra_size = 0;
    switch(handle_data_status){
    case Handle_Ctrl_Ctrl_Byte:          // ������ƿ�-����������ݿ��С
        ctrl_size = (data >> 6) + 1;     // ����������С1
        diff_size = (data >> 3) & 0x07;  // ��ֿ��С2
        extra_size = data & 0x07;        // ������С1
        handle_data_status = Handle_Ctrl_Index_Byte;
        data_counter = 0;
        break;
    case Handle_Ctrl_Index_Byte:         // ��ȡ�����ݵ�����
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
    case Handle_Ctrl_DiffLen_Byte:       // ���������ݿ�
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
    case Handle_Ctrl_ExtraLen_Byte:      // ����������ݿ�
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
    case Handle_Unzip_Data:            // �����ѹ����
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



/*��ѹ�����ļ�,ͨ���ص�����(�����)����ֽ�������
����:���ļ������������ļ��ļ�������
���:-1ʧ��,1�ɹ�
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

    file_lseek(patch_file, 4); // diff�ļ��׵�ַ��ʼ-ƫ��4����ַ addr��0-3
    if(file_read(patch_file, &read_data, 1) != 1){// ��ȡnewfile�ļ����ܳ��� addr��4-7
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
    Handle_Data_Init(ctrl_data); // �������ļ���󳤶ȣ��ͳ�ʼ����������

    if(file_read(patch_file, &read_data, 1) != 1){// ��ȡ���붯̬�ڴ�ѹ����С addr��8-8
        return -1;
    }
    
    /*
			   2 3 5 9 17 33 KB(��̬�����ڴ�+1)
		       1 2 4 8 16 32 KB(��̬�����ڴ�)
	zip_flag = 0 1 2 3 4  5 
	*/
    //����ѡ�������Ӧ�ڴ�
    zip_flag = read_data;
    databuff_len = 1 << (10 + zip_flag);            // ���� ��zip_flag=0,��̬�ڴ�����Ϊ��1k���ڴ濪��Խ���ѹԽ����֮ 
//    data_buff = new uint8_t[databuff_len];        // del isen  C++��ʽ
    data_buff = (uint8_t *)IOT_Malloc(databuff_len);// add isen����̬�ڴ�����api��ע���ڲ�ͬ��Ƕ��ʽƽ̨ʵ�ַ�ʽ��һ�� 
    
#if BSPatch_DeBug
    printf(" === BsPatch() ctrl_data: %02x-%d-%d-%d \r\n", ctrl_data,ctrl_data, zip_flag,databuff_len);
#endif

    databuff_pos = 0;
    if(file_read(patch_file, &read_data, 1) != 1){// ��ȡdiffѹ����1���ݳ��� addr��9-9
        return -1;
    }
    data_buff[databuff_pos++] = out_data = read_data;	//data_buff[0] = 25
    if(Unzipdata_Handle(out_data, old_file, new_file) == -1){// ��ȡ������������ֿ�Ͷ����Ĵ�С addr��10-out_data
        return -1;
    }
    databuff_pos = databuff_pos % databuff_len;

    while(file_read(patch_file, &read_data, 1) == 1)
    {
/*----------------����ֱ��ʹ�õ�����----------------------------------------------------------*/
        ctrl_data = read_data;
        if(ctrl_data >> 4 == 0x0f)// ��4λΪ1111 -- ����λΪ���ȼ�һ��ֵ -- 8λ��ƥ�����ݳ���
        {
            Singledata_Len = (ctrl_data & 0x0f) + 1; // �������������ݵĳ���: 0xfF4&0x0f+1 -> 5
            for(i = 0; i < Singledata_Len; i++)
            {
                if(file_read(patch_file, &read_data, 1) != 1)
                {
                    return -1;
                }
                data_buff[databuff_pos++] = out_data = read_data;
                if(Unzipdata_Handle(out_data, old_file, new_file) == -1)// �򲹶��㷨,������ѹ�������
                {
                    return -1;
                }
                databuff_pos = databuff_pos % databuff_len;
            }
            continue;
        }
/*----------------��Ҫ��ƥ�䴦�������----------------------------------------------------------*/
	    else if(ctrl_data >> 7 == 0)// ��һλΪ0��Ϊ��ʶ -- 64����,���λ2ƥ�� -- 8λ������ƥ������
	    {
            right_len = 1;
            matchlen_compen = 2;
            Index_Mask = 0x3F << 1;
            Match_Mask = 0x1;
        }
        else if(ctrl_data >> 6 == 2)// ����λΪ10��Ϊ��ʶ -- 16λ������ƥ������	8
        {
            if(file_read(patch_file, &read_data, 1) != 1)
            {
                return -1;
            }
            ctrl_data = ((ctrl_data & 0x3f) << 8) | read_data;	//14λ��Чλ
            right_len = 4 - (int)(zip_flag / 2);
            matchlen_compen = 3;
            Index_Mask = ((1 << (10 + (int)(zip_flag / 2))) - 1) << (4 - (int)(zip_flag / 2)); // ��������
            Match_Mask = (1 << (4 - (int)(zip_flag / 2))) - 1;                                 // ƥ������
        }
        else if(ctrl_data >> 5 == 6)// ����λΪ110��Ϊ��ʶ -- 24λ������ƥ������	C
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
        else                        //����λΪ1110��Ϊ��ʶ -- 32λ������ƥ������ E
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

    //delete []data_buff; // del isen  C++��ʽ
    IOT_Free(data_buff);  // add isen
    data_buff = NULL; 
    
#if BSPatch_DeBug
    printf("BsPatch() END!\n");
#endif
    return 1;
}










