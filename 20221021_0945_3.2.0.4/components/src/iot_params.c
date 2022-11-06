/*
 * iot_params.c
 *
 *  Created on: 2021��12��20��
 *      Author: Administrator
 */
/*
 * iot_params.c
 *
 *  Created on: 2021��12��8��
 *      Author: Administrator
 */
#define  IOT_PARAMS_GLOBAL
#include "iot_params.h"
#include "iot_universal.h"

#include <string.h>
#include "esp_spi_flash.h"
#include "esp_system.h"
#include "iot_system.h"

#define MAX_PARAMS_LEN  50	// ÿ��������󳤶ȣ�Ԥ��50byte

IOT_PARAMS_EXTERN unsigned char g_ucFlashBuf[PARAMS_ZONE_PAGE_SIZE];


/******************************************************************************
 * FunctionName : IOT_AddParam_Init
 * Description  : ���Ӳ�����ʼ���ⲿflash
 * Parameters   : none
 * Returns      : none
 * Notice       : �����Ӳ���flash��ַ���䣺
 *							0x00007000 - 0x00007FFF : 4K���ݿռ�
 * 				  			0x00007000 - 0x000070FF ����Ų���������ַ��Ԥ��80��������ÿ������Ԥ������Ϊ50�ֽڣ�0-255 == 256�ֽ�
 * 				  			0x00007100 - 0x00007FF9 ����Ų�������        256-4089 -->80*50 == (4096 - 256 - 6)�ֽ�
 * 				  			0x00007FFA - 0x00007FFD ���������У��λ       4090-4093 == 4�ֽ�
 * 				  			0x00007FFE - 0x00007FFF �����crcУ��ֵ       4094-4095 == 2�ֽ�
 * 				            |�������ݳ��ȵ�ַ--|--����ֵ--|--���λ--|--����crcֵ--| = 4096�ֽ�
 *
 * 				      �������num���䣺
 * 				      NO   : ƫ�Ƶ�ַ 256�ֽ� -- ע�⣺ÿ������Ԥ�������50�ֽڳ���
 *					  NO.0 �� ���������� ����4���ֽ�
 *                    NO.1 : �����ļ�CRC32 ����1���ֽ�
 *                    NO.2 : �����ļ�Ԥ�� ����1���ֽ�
 *                    NO.3 : �����ļ�Ԥ�� ����1���ֽ�
 *                    NO.4 : �����ļ��ϵ��¼���س���ֵ
 *                    NO.5 : ����״̬��¼ֵ
 *                    NO.6 : �����ļ������Flash����
*******************************************************************************/
void IOT_AddParam_Init(void)
{
	unsigned char *pucParamBuf = g_ucFlashBuf;// ���������� |�������ݳ��ȵ�ַ--|--����ֵ--|--���λ--|--����crcֵ--| = 4096�ֽ�
	unsigned int uiParamsOffsetAddr = PARAMS_LEN_OFFSET;
	unsigned int uiParamsSignOffsetAddr = PARAMS_SIGN_OFFSET;
	unsigned int uiParamsCrcOffsetAddr = PARAMS_CRC_OFFSET;
    unsigned int uiParamsPageSize = PARAMS_ZONE_PAGE_SIZE;
	unsigned int uiSaveParamsAddr = FOTA_PARAMS_ADDR; // ���������ַ
	unsigned short int usiCRC16Val = 0;

	spi_flash_read(uiSaveParamsAddr, pucParamBuf, uiParamsPageSize);	  // һ���Խ�ԭ�����ݶ����õ�����
	if(memcmp(&pucParamBuf[uiParamsSignOffsetAddr],PARAMS_ZONE_WRITED_SIGN,4))// ����Ƿ�д���FLASH
	{
		memset(pucParamBuf,0,uiParamsPageSize);
		memset(pucParamBuf,1,uiParamsOffsetAddr);// ����Ĭ����1�� ����Ĭ���ǣ�0
		memcpy(&pucParamBuf[uiParamsSignOffsetAddr],PARAMS_ZONE_WRITED_SIGN,4); // д����������־λ

		/* CRC16У�� */
		usiCRC16Val = Modbus_Caluation_CRC16(pucParamBuf, uiParamsPageSize-2);
		pucParamBuf[uiParamsCrcOffsetAddr] = HIGH(usiCRC16Val);
		pucParamBuf[uiParamsCrcOffsetAddr+1] = LOW(usiCRC16Val);

		spi_flash_erase_sector(FOTA_PARAMS_ADDR/PARAMS_ZONE_PAGE_SIZE);     // д�����ǰ��Ҫ�Ȳ���ҳ
		spi_flash_write(uiSaveParamsAddr, pucParamBuf, uiParamsPageSize);   // һ���Խ�д�����

		IOT_Printf("\r\n IOT_AddParam_Init()  usiCRC16Val = %d \r\n",usiCRC16Val);
	}
}

/******************************************************************************
 * FunctionName : IOT_AddParam_Write
 * Description  : ���Ӳ���д���ⲿflash
 * Parameters   : none
 * Returns      : none
 * Notice       : �����Ӳ���flash��ַ���䣺
 *							0x00007000 - 0x00007FFF : 4K���ݿռ�
 * 				  			0x00007000 - 0x000070FF ����Ų���������ַ��Ԥ��80��������ÿ������Ԥ������Ϊ50�ֽڣ�0-255 == 256�ֽ�
 * 				  			0x00007100 - 0x00007FF9 ����Ų�������         256-4089 -->80*50 == (4096 - 256 - 6)�ֽ�
 * 				  			0x00007FFA - 0x00007FFD ���������У��λ       4090-4093 == 4�ֽ�
 * 				  			0x00007FFE - 0x00007FFF �����crcУ��ֵ        4094-4095 == 2�ֽ�
 * 				           |�������ݳ��ȵ�ַ--|--����ֵ--|--���λ--|--����crcֵ--| = 4096�ֽ�
 *
 * 				      �������num���䣺
 * 				      NO   : ƫ�Ƶ�ַ 256�ֽ� -- ע�⣺ÿ������Ԥ�������50�ֽڳ���
 *					  NO.0 �� ���������� ����4���ֽ�
 *                    NO.1 : �����ļ�CRC32 ����1���ֽ�
 *                    NO.2 : �����ļ�Ԥ�� ����1���ֽ�
 *                    NO.3 : �����ļ�Ԥ�� ����1���ֽ�
 *                    NO.4 : �����ļ��ϵ��¼���س���ֵ
 *                    NO.5 : ����״̬��¼ֵ
 *                    NO.6 : �����ļ������Flash����
 *                    NO.7 : �����ļ��������ʼ��ַ
*******************************************************************************/
void IOT_AddParam_Write(unsigned char *pucSrc_tem, unsigned char ucNum_tem, unsigned char ucLen_tem)
{
	unsigned char *pucParamBuf = g_ucFlashBuf;// ���������� |�������ݳ��ȵ�ַ--|--����ֵ--|--���λ--|--����crcֵ--| = 4096�ֽ�
	unsigned int uiParamsOffsetAddr = PARAMS_LEN_OFFSET;
//	unsigned int uiParamsSignOffsetAddr = PARAMS_SIGN_OFFSET;
	unsigned int uiParamsCrcOffsetAddr = PARAMS_CRC_OFFSET;
    unsigned int uiParamsPageSize = PARAMS_ZONE_PAGE_SIZE;
	unsigned int uiSaveParamsAddr = FOTA_PARAMS_ADDR; // ���������ַ
	char cSingleParamLen  = MAX_PARAMS_LEN;           // �����������ݳ���
	unsigned short int usiCRC16Val = 0;

	spi_flash_read(uiSaveParamsAddr, pucParamBuf, uiParamsPageSize);// һ���Խ�ԭ�����ݶ����õ�����
	usiCRC16Val = Modbus_Caluation_CRC16(pucParamBuf, uiParamsPageSize-2);           // ��ȡԭ�в��� (���� + ����)crcУ��

	/* ���鱣������crcֵ  */
	if((pucParamBuf[uiParamsCrcOffsetAddr] == HIGH(usiCRC16Val)) &&\
 	   (pucParamBuf[uiParamsCrcOffsetAddr+1] == LOW(usiCRC16Val)))
	{
		pucParamBuf[ucNum_tem] = ucLen_tem;
		memcpy(&pucParamBuf[(uiParamsOffsetAddr+(ucNum_tem*cSingleParamLen))],pucSrc_tem,ucLen_tem); // ����д�����ֵ
		usiCRC16Val = Modbus_Caluation_CRC16(pucParamBuf, uiParamsPageSize-2);                                    // ��д����� (���� + ����)crcУ��
		pucParamBuf[uiParamsCrcOffsetAddr] = HIGH(usiCRC16Val);
		pucParamBuf[uiParamsCrcOffsetAddr+1] = LOW(usiCRC16Val);
		spi_flash_erase_sector(FOTA_PARAMS_ADDR/PARAMS_ZONE_PAGE_SIZE);     // д�����ǰ��Ҫ�Ȳ���ҳ
		spi_flash_write(uiSaveParamsAddr, pucParamBuf, uiParamsPageSize);   // һ���Խ�д�����

		IOT_Printf("\r\n IOT_AddParam_Write() num = %d, len = %d CRC = %x src = ", ucNum_tem,ucLen_tem,usiCRC16Val);
		{for(int i=0; i<ucLen_tem; i++)IOT_Printf(" %02x", pucSrc_tem[i]); IOT_Printf("\r\n");}
	}
	else
	{
		IOT_Printf("\r\n IOT_AddParam_Write() saved crc  = %x%x \r\n",pucParamBuf[uiParamsCrcOffsetAddr], pucParamBuf[uiParamsCrcOffsetAddr]+1);
		IOT_Printf("\r\n IOT_AddParam_Write() Read crc fail, rewrite = %x \r\n",usiCRC16Val);

		pucParamBuf[uiParamsCrcOffsetAddr] = HIGH(usiCRC16Val);
		pucParamBuf[uiParamsCrcOffsetAddr+1] = LOW(usiCRC16Val);
		spi_flash_erase_sector(FOTA_PARAMS_ADDR/PARAMS_ZONE_PAGE_SIZE);     // д�����ǰ��Ҫ�Ȳ���ҳ
		spi_flash_write(uiSaveParamsAddr, pucParamBuf, uiParamsPageSize);   // һ���Խ�д�����
	}
}

/******************************************************************************
 * FunctionName : IOT_AddParam_Read
 * Description  : ���Ӳ�����ȡ�ⲿflash
 * Parameters   : none
 * Returns      : none
 * Notice       : �����Ӳ���flash��ַ���䣺
 *							0x00007000 - 0x00007FFF : 4K���ݿռ�
 * 				  			0x00007000 - 0x000070FF ����Ų���������ַ��Ԥ��80��������ÿ������Ԥ������Ϊ50�ֽڣ�0-255 == 256�ֽ�
 * 				  			0x00007100 - 0x00007FF9 ����Ų�������         256-4089 -->80*50 == (4096 - 256 - 6)�ֽ�
 * 				  			0x00007FFA - 0x00007FFD ���������У��λ       4090-4093 == 4�ֽ�
 * 				  			0x00007FFE - 0x00007FFF �����crcУ��ֵ        4094-4095 == 2�ֽ�
 * 				            |�������ݳ��ȵ�ַ--|--����ֵ--|--���λ--|--����crcֵ--| = 4096�ֽ�
 *
 * 				      �������num���䣺
 * 				      NO   : ƫ�Ƶ�ַ 256�ֽ� -- ע�⣺ÿ������Ԥ�������50�ֽڳ���
 *					  NO.0 �� ���������� ����4���ֽ�
 *                    NO.1 : �����ļ�CRC32 ����4���ֽ�
 *                    NO.2 : �����ļ�Ԥ�� ����1���ֽ�
 *                    NO.3 : �����ļ�Ԥ�� ����1���ֽ�
 *                    NO.4 : �����ļ��ϵ��¼���س���ֵ
 *                    NO.5 : ����״̬��¼ֵ
 *                    NO.6 : �����ļ������Flash����	����4���ֽ�
*******************************************************************************/
void IOT_AddParam_Read(unsigned char *pucSrc_tem, unsigned char ucNum_tem)
{
    unsigned char *pucParamBuf = g_ucFlashBuf;// ���������� |�������ݳ��ȵ�ַ--|--����ֵ--|--���λ--|--����crcֵ--| = 4096�ֽ�
	unsigned int uiParamsOffsetAddr = PARAMS_LEN_OFFSET;
	//	unsigned int uiParamsSignOffsetAddr = PARAMS_SIGN_OFFSET;
	unsigned int uiParamsCrcOffsetAddr = PARAMS_CRC_OFFSET;
	unsigned int uiParamsPageSize = PARAMS_ZONE_PAGE_SIZE;
	unsigned int uiSaveParamsAddr = FOTA_PARAMS_ADDR; // ���������ַ
	char cSingleParamLen  = MAX_PARAMS_LEN;           // �����������ݳ���
	unsigned short int usiCRC16Val = 0;

	spi_flash_read(uiSaveParamsAddr, pucParamBuf, uiParamsPageSize);	   // һ���Խ�ԭ�����ݶ����õ�����
	usiCRC16Val = Modbus_Caluation_CRC16(pucParamBuf, uiParamsPageSize-2); // ��ȡԭ�в��� (���� + ����)crcУ��

	/* ���鱣������crcֵ  */
	if((pucParamBuf[uiParamsCrcOffsetAddr] == HIGH(usiCRC16Val)) &&\
 		 (pucParamBuf[uiParamsCrcOffsetAddr+1] == LOW(usiCRC16Val)))
	{
		memcpy(pucSrc_tem,&pucParamBuf[(uiParamsOffsetAddr+(ucNum_tem*cSingleParamLen))], pucParamBuf[ucNum_tem]); // �õ���ȡֵ


		IOT_Printf("\r\n IOT_AddParam_Read() num = %d, len = %d src = ", ucNum_tem,pucParamBuf[ucNum_tem]);
		{for(int i=0; i<pucParamBuf[ucNum_tem]; i++)IOT_Printf(" %02x", pucSrc_tem[i]); IOT_Printf("\r\n");}
	}
	else // crc fail
	{
		spi_flash_erase_sector(FOTA_PARAMS_ADDR/PARAMS_ZONE_PAGE_SIZE); // !!! crc�������³�ʼ�����в���
		IOT_Printf("IOT_AddParam_Read() num = %d CRC fail = %x , rewrite addparams again! \r\n", ucNum_tem, usiCRC16Val);
	}
}







