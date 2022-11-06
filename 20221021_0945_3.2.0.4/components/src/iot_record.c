/*
 * iot_record.c
 *
 *  Created on: 2022��7��15��
 *      Author: grt-chenyan
 */

#include "iot_record.h"
#include "iot_system.h"
#include "iot_spi_flash.h"
#include "iot_universal.h"
#include <string.h>
#include "esp_log.h"
#include "freertos/task.h"

#define  RECORD_PRINTF_EN  0

#define  RECORD_DEBUG     ( (DEBUG_EN)  &&  (RECORD_PRINTF_EN))

#define  ESP_RECORD  		 "OFFLINE_DATA_RECORD"
#define  RESET_SIGN          "Record_REST_Sign"


#define  RECORD_SECTORS_NUM             (1000)						//����1000����������
#define  FLASH_ErasePageSize  			(W25X_FLASH_ErasePageSize)	//������С
#define  OFFLINE_DATA_RECORD_ADDR   	(DATA_ADDR)					//�������ݱ������ʼ��ַ
#define  OFFLINE_DATA_RECORD_END_ADDR	(DATA_ADDR + FLASH_ErasePageSize * RECORD_SECTORS_NUM)		//0x3EC00

#if LOAD_FORMAT_PARAM
#define RECORD_PARAMETER_ADDR   		(OFFLINE_DATA_RECORD_END_ADDR + FLASH_ErasePageSize * 5)	//������������ ������ƫ��5��������
#define RECORD_PARAMETER_BACKUP_ADDR    (RECORD_PARAMETER_ADDR + FLASH_ErasePageSize)
#else
uint8_t  ucPacketSizeResetFlag  =  0 ;  //1  ��ʾ�洢��ʽ������
#endif

#define  DATA_SPACE_SET  			0x01
#define  DATA_SPACE_RESET   		0xff
#define  DATA_SPACE_RESET_RECORD  	0xCE

#define  OFFLINE_DATA_PACKET_SIZE_1K   0x01
#define  OFFLINE_DATA_PACKET_SIZE_2K   0x02
#define  OFFLINE_DATA_PACKET_SIZE_4K   0x04

uint32_t GuiInverterFlashData_W_Addr = 0;    //����������д��ַ
uint32_t GuiInverterFlashData_R_Addr = 0;	 //���������Ѷ���ַ
bool rw_addr_gotflag = false;
bool bRecordClearingFlag = false;

extern const char* IOT_ESP_strstr(const unsigned char *FoundBuf, const unsigned char *FindStr,uint16_t sumnum,uint8_t findnum);


#if LOAD_FORMAT_PARAM
const record_storage_format_param_t RecordFormatParamDefault = {
	.uiPacketSize = OFFLINE_DATA_PACKET_SIZE_1K,
	.WriteAddr = OFFLINE_DATA_RECORD_ADDR,
	.ReadAddr =  OFFLINE_DATA_RECORD_ADDR,
	.reserve = 0,
	.ucIOTESPInverterID = {"INVERSNERR"}
};
#endif

record_storage_format_param_t GformatParam = {0};


/******************************************************************************
 * FunctionName : IOT_GetRecord_task
 * Description  : ��ȡ�������ݼ�¼����
 * Parameters   : none
 * Returns      : none
 * Notice       :
*******************************************************************************/
void IOT_GetRecord_task(void  *arg)
{
	while(1)
	{

		if(rw_addr_gotflag == false)
		{
			IOT_ESP_InverterFlashDataRWAddr_Get();
		}
		else
		{
			vTaskDelete(NULL);	//ɾ����ǰ����
		}

		 vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}

/******************************************************************************
 * FunctionName : IOT_ClearRecord_task
 * Description  : ����������ݼ�¼����
 * Parameters   : none
 * Returns      : none
 * Notice       :
*******************************************************************************/
void IOT_ClearRecord_task(void  *arg)
{
	int i = 0;
	ESP_LOGE(ESP_RECORD , " IOT_ClearRecord_task ");

	bRecordClearingFlag = true;

	while(1)
	{

		if(rw_addr_gotflag == true)
		{
			IOT_Mutex_W25QXX_Erase_Sector(OFFLINE_DATA_RECORD_ADDR + i * SECTOR_SIZE);
			i++;
		}

		if(i >= RECORD_SECTORS_NUM)
		{
			bRecordClearingFlag = false;

			#if !LOAD_FORMAT_PARAM
			if(0 == GformatParam.uiPacketSize)
			{
				ucPacketSizeResetFlag = 1;	//��  GformatParam.uiPacketSize δ ��ֵ  �� ��λ��־��1 �� ��ֹд������ʱ�ظ���ջ�����
			}
			#endif

			vTaskDelete(NULL);	//ɾ����ǰ����
			break;
		}

		 vTaskDelay(10 / portTICK_PERIOD_MS);
	}
}


void IOT_ReadRecord_Init(void)
{
#if LOAD_FORMAT_PARAM
    iot_Record_storageFormat_init();
#endif
    xTaskCreate(IOT_GetRecord_task,"IOT_GetRecord_task", 1024 * 4, NULL, configMAX_PRIORITIES - 11, NULL);
}


/******************************************************************************
 * FunctionName : IOT_Get_Record_Num
 * Description  : ��ȡ��������ʣ����Ŀ (��λ 1k byte)
 * Parameters   : none
 * Returns      : none
 * Notice       :
*******************************************************************************/
uint16_t IOT_Get_Record_Num(void)
{
	uint32_t usRecord_sector_num = 0;
	uint32_t uiPacketSizeType = GformatParam.uiPacketSize;

	ESP_LOGE(ESP_RECORD , " IOT_Get_Record_Num  GuiInverterFlashData_W_Addr = 0x%x , GuiInverterFlashData_R_Addr = 0x%x",\
			GuiInverterFlashData_W_Addr , GuiInverterFlashData_R_Addr);

	if(GuiInverterFlashData_W_Addr == GuiInverterFlashData_R_Addr)
	{
		return 0;
	}
	else if(GuiInverterFlashData_W_Addr > GuiInverterFlashData_R_Addr)  //��д�����Ѷ�
	{
		usRecord_sector_num = GuiInverterFlashData_W_Addr - GuiInverterFlashData_R_Addr;
	}else
	{
		usRecord_sector_num = (OFFLINE_DATA_RECORD_END_ADDR - GuiInverterFlashData_R_Addr) + (GuiInverterFlashData_W_Addr - OFFLINE_DATA_RECORD_ADDR);
	}


	if((uiPacketSizeType != OFFLINE_DATA_PACKET_SIZE_1K)
		&& (uiPacketSizeType != OFFLINE_DATA_PACKET_SIZE_2K)
		&& (uiPacketSizeType != OFFLINE_DATA_PACKET_SIZE_4K))
	{
		uiPacketSizeType = OFFLINE_DATA_PACKET_SIZE_1K;
	}


	usRecord_sector_num = usRecord_sector_num / (uiPacketSizeType * 1024);

	IOT_Printf(" IOT_Get_Record_Num() = %d\r\n",usRecord_sector_num);
	return usRecord_sector_num;
}

/******************************************************************************
 * FunctionName : iot_Record_storageFormat_Write
 * Description  : ��λ Record ����
 * Parameters   : none
 * Returns      : none
 * Notice       :
*******************************************************************************/
void iot_Reset_Record(void)
{
	int i = 0;

	ESP_LOGE(ESP_RECORD , " iot_Reset_Record ");

	for(i = 0;i < RECORD_SECTORS_NUM; i++ )
	{
		IOT_Mutex_W25QXX_Erase_Sector(OFFLINE_DATA_RECORD_ADDR + i * SECTOR_SIZE);
	}
}

/******************************************************************************
 * FunctionName : iot_Reset_Record_Task
 * Description  : ������������������ݷ���
 * Parameters   : none
 * Returns      : none
 * Notice       :
*******************************************************************************/
void iot_Reset_Record_Task(void)
{
	if(true == bRecordClearingFlag)
	{
		return;
	}
    xTaskCreate(IOT_ClearRecord_task,"IOT_ClearRecord_task", 1024 * 4, NULL, configMAX_PRIORITIES - 12, NULL);
}


#if LOAD_FORMAT_PARAM
/******************************************************************************
 * FunctionName : iot_Record_storageFormat_Write
 * Description  : ���� Record ������ʽ��Ϣ
 * Parameters   : none
 * Returns      : none
 * Notice       :
*******************************************************************************/
void iot_Record_storageFormat_Write(record_storage_format_param_t  * FormatParam_tem )
{
	uint8_t *pbuf = NULL;
	uint16_t CRC16;
	record_storage_format_param_t  * pFormatParam_tem = FormatParam_tem;

	pbuf = (uint8_t *)malloc(FLASH_ErasePageSize);

	if(NULL == pbuf)
	{
		return;
	}

	bzero(pbuf , FLASH_ErasePageSize);

	memcpy(pbuf , pFormatParam_tem , sizeof(record_storage_format_param_t));

	CRC16 = GetCRC16(pbuf,FLASH_ErasePageSize - 2);
	pbuf[FLASH_ErasePageSize - 2] = (CRC16 >> 8) & 0xff;
	pbuf[FLASH_ErasePageSize - 1] = CRC16  & 0xff;

	//д����������
	IOT_Mutex_SPI_FLASH_WriteBuff(pbuf , RECORD_PARAMETER_ADDR , FLASH_ErasePageSize);

	//д�뱸����
	IOT_Mutex_SPI_FLASH_WriteBuff(pbuf , RECORD_PARAMETER_BACKUP_ADDR , FLASH_ErasePageSize);

	free(pbuf);
}


/******************************************************************************
 * FunctionName : iot_Record_storageFormat_init
 * Description  : ��ʼ��Record ������ʽ��Ϣ
 * Parameters   : none
 * Returns      : none
 * Notice       :
*******************************************************************************/
void iot_Record_storageFormat_init(void)
{
	uint8_t *pbuf = NULL;
	uint16_t CRC16 =0;

	pbuf =(uint8_t *)malloc(FLASH_ErasePageSize);

	if(NULL == pbuf)
	{
		return;
	}

	bzero(pbuf , FLASH_ErasePageSize);


	IOT_Mutex_SPI_FLASH_BufferRead(pbuf,RECORD_PARAMETER_ADDR , FLASH_ErasePageSize);

	CRC16 = GetCRC16(pbuf,FLASH_ErasePageSize - 2);
	if((pbuf[FLASH_ErasePageSize - 2] == (uint8_t)((CRC16 >> 8) & 0xff) ) && (pbuf[FLASH_ErasePageSize - 1] == (uint8_t)(CRC16 & 0xff)))
	{
		memcpy((uint8_t *)&GformatParam , pbuf , sizeof(record_storage_format_param_t));

		#if RECORD_DEBUG
		IOT_Printf("GformatParam.uiPacketSize = 0x%x" , GformatParam.uiPacketSize);
		IOT_Printf("GformatParam.WriteAddr = 0x%x" , GformatParam.WriteAddr);
		IOT_Printf("GformatParam.ReadAddr = 0x%x" , GformatParam.ReadAddr);
		//printf("GformatParam.reserve = 0x%x" , GformatParam.reserve);
		IOT_Printf("GformatParam.ReadAddr = %s" , GformatParam.ucIOTESPInverterID);
		#endif
	}
	else
	{
		bzero(pbuf , FLASH_ErasePageSize);
		IOT_Mutex_SPI_FLASH_BufferRead(pbuf,RECORD_PARAMETER_BACKUP_ADDR , FLASH_ErasePageSize);	//�ӱ������ж�ȡ
		CRC16 = GetCRC16(pbuf,FLASH_ErasePageSize - 2);

		if((pbuf[FLASH_ErasePageSize - 2] == (uint8_t)((CRC16 >> 8) & 0xff) ) && (pbuf[FLASH_ErasePageSize - 1] == (uint8_t)(CRC16 & 0xff)))
		{
			memcpy((uint8_t *)&GformatParam , pbuf , sizeof(record_storage_format_param_t));
			IOT_Mutex_SPI_FLASH_WriteBuff(pbuf , RECORD_PARAMETER_ADDR , FLASH_ErasePageSize);	//д������������

			#if RECORD_DEBUG
			IOT_Printf("GformatParam.uiPacketSize = 0x%x" , GformatParam.uiPacketSize);
			IOT_Printf("GformatParam.WriteAddr = 0x%x" , GformatParam.WriteAddr);
			IOT_Printf("GformatParam.ReadAddr = 0x%x" , GformatParam.ReadAddr);
			//IOT_Printf("GformatParam.reserve = 0x%x" , GformatParam.reserve);
			IOT_Printf("GformatParam.ReadAddr = %s" , GformatParam.ucIOTESPInverterID);
			#endif

		}else
		{
			ESP_LOGE(ESP_RECORD , " iot_Record_storageFormat_init  read backup check failed");
			iot_Reset_Record();  //�����������
			iot_Record_storageFormat_Write(&RecordFormatParamDefault);	 //д��Ĭ�ϵĲ���
			memcpy((uint8_t *)&GformatParam , (uint8_t *)&RecordFormatParamDefault , sizeof(record_storage_format_param_t));

			#if RECORD_DEBUG
			IOT_Printf("GformatParam.uiPacketSize = 0x%x" , GformatParam.uiPacketSize);
			IOT_Printf("GformatParam.WriteAddr = 0x%x" , GformatParam.WriteAddr);
			IOT_Printf("GformatParam.ReadAddr = 0x%x" , GformatParam.ReadAddr);
			//IOT_Printf("GformatParam.reserve = 0x%x" , GformatParam.reserve);
			IOT_Printf("GformatParam.ReadAddr = %s" , GformatParam.ucIOTESPInverterID);
			#endif
		}

	}

	free(pbuf);
}

#endif


/******************************************************************************
 * FunctionName : IOT_ESP_InverterFlashDataRWAddr_Get
 * Description  : esp �����������д��ַ��ȡ
 * Parameters   : none
 * Returns      : 0 - �ڶ�  1- ��ȡ�ɹ�
 * Notice       : 1��W-R ��д��־λ������������� �� 0xFF-0xFF : δд�Ѳ�    - �ɽ���д��ֹ��    - 0xFF^0xFF = 0x00 ס��������� *ע��*
 *                                        0x01-0x01 : ��д�Ѷ�  - �ɽ���д��ֹ��    - 0x01^0x01 = 0x00
 *                                        0x01-0xFF : ��дδ��  - �ɽ��ж���ֹд    - 0x01^0xFF = 0xFE
 *                2��flash�������ݱ����ʽ :  (1byte)+д(1byte)+������Чλ(1byte)+���ȶ�Ӧҳ��(1byte)+����(2byte)+crc(2byte)+�����id(10byte)+time(6byte)+data(������)
 *                  (�� -д ע�ͣ�0x01-��ʾ�Ѷ���д   ��0xFF-��ʾδ��δд)
*******************************************************************************/
bool IOT_ESP_InverterFlashDataRWAddr_Get(void)
{
	static uint32_t Addr = OFFLINE_DATA_RECORD_ADDR;
	static uint8_t  rw_valflag_1 = 0;
	static uint8_t  rw_valflag_2 = 0;
	static uint8_t  rw_endflag = 0;

	uint8_t ucErr_t = 0;
	uint8_t rwbytebuf[4] = {0};           // ��дλ��ַ

	uint8_t ucPacketType = OFFLINE_DATA_PACKET_SIZE_1K;
	uint32_t Single_Addr = 0;// �����ϵ絥�β�ѯflash��д��ַ

	if(rw_addr_gotflag == true) return 1;  // �ϵ��ѻ�ö�д��ַ

	/* �ϵ��һ�β�ѯ�ش��������ݶ�д��ַ */
	if(Addr == OFFLINE_DATA_RECORD_ADDR )
	{
		GuiInverterFlashData_W_Addr = OFFLINE_DATA_RECORD_ADDR;   // ����������ش�ʧ�ܱ�������дflash��ַ
		GuiInverterFlashData_R_Addr = OFFLINE_DATA_RECORD_ADDR;   // ����������ش�ʧ���������ݶ�flash��ַ

		/* �����ݶ�д��ַ */
		IOT_Mutex_SPI_FLASH_BufferRead(rwbytebuf , Addr , 4); // ����1��2��3��4������ͷ��־λ
		if((rwbytebuf[0]==0xFF && rwbytebuf[1]==0xFF) || \
		   (rwbytebuf[0]==0x01 && rwbytebuf[1]==0xFF) || \
		   (rwbytebuf[0]==0x01 && rwbytebuf[1]==0x01))
		{
			rw_valflag_1 = rwbytebuf[0] ^ rwbytebuf[1];
		}
		else
		{
			rw_valflag_1 = 0x00;
		}
//		ESP_printf(" ** rwbytebuf[x] == %x - %x - %x - %x**\n",rwbytebuf[0],rwbytebuf[1],rwbytebuf[2],rwbytebuf[3]);
		ucPacketType = rwbytebuf[3];       // !!!ע�⣺�µ�flashû�������ݱ��棬��һ���ϵ� rwbytebuf[3] = 0/0xFF,��������ʱ�� rwbytebuf[3]>4;

	#if LOAD_FORMAT_PARAM
		if(ucPacketType != (GformatParam.uiPacketSize & 0xff))
		{
			if((GformatParam.uiPacketSize != OFFLINE_DATA_PACKET_SIZE_1K)
				|| (GformatParam.uiPacketSize != OFFLINE_DATA_PACKET_SIZE_2K)
				||((GformatParam.uiPacketSize != OFFLINE_DATA_PACKET_SIZE_4K)))
			{
				#if RECORD_DEBUG
				ESP_LOGE(ESP_RECORD , " ERROR �� GformatParam.uiPacketSize = %d",GformatParam.uiPacketSize);
				#endif
				GformatParam.uiPacketSize = OFFLINE_DATA_PACKET_SIZE_1K;
			}

			rw_valflag_1 = 0x00;
			ucPacketType = GformatParam.uiPacketSize & 0xff;
		}
	#else
		if((ucPacketType != LOW(OFFLINE_DATA_PACKET_SIZE_1K))
			  && (ucPacketType != LOW(OFFLINE_DATA_PACKET_SIZE_2K))
			  && (ucPacketType != LOW(OFFLINE_DATA_PACKET_SIZE_4K))
		  )
		{
			rw_valflag_1 = 0x00;	//���Ϊ��д�Ѷ�  �� δдδ��

			#if RECORD_DEBUG
			ESP_LOGE(ESP_RECORD , " ERROR �� ucPacketType = %d",ucPacketType);
			#endif
			ucPacketType = OFFLINE_DATA_PACKET_SIZE_1K;
		}
		else if((rwbytebuf[2] == LOW(DATA_SPACE_SET))  && (ucPacketType != (GformatParam.uiPacketSize & 0xff)))		//��⵽�����ֲ�ͬ�ı����ʽ
		{
			#if RECORD_DEBUG
			ESP_LOGE(ESP_RECORD , "1 IOT_ESP_InverterFlashDataRWAddr_Get ucPacketType != GformatParam.uiPacketSize");
			#endif
			if(0 == GformatParam.uiPacketSize)
			{
				GformatParam.uiPacketSize = ucPacketType;
			}
			else	//�������ֲ�ͬ�Ĵ洢��ʽ,���ȫ����������
			{
				iot_Reset_Record();
				GuiInverterFlashData_W_Addr = OFFLINE_DATA_RECORD_ADDR;
				GuiInverterFlashData_R_Addr = OFFLINE_DATA_RECORD_ADDR;
				ucPacketSizeResetFlag = 1;
				GformatParam.uiPacketSize = 0;
				ucErr_t =  0;
				goto END;
			}

		}
	#endif


		if((rwbytebuf[2] == LOW(DATA_SPACE_RESET_RECORD)))	//�ж���һ�������Ƿ�Ϊ ��ռ�¼����־
		{
			uint8_t *pbuf = (uint8_t *)malloc(256);

			bzero(pbuf , 256);
			IOT_Mutex_SPI_FLASH_BufferRead(pbuf , Addr , 256);
			if(true == IOT_ESP_Record_ResetSign_Check_b(pbuf))
			{
				#if RECORD_DEBUG
				IOT_Printf("\r\n");
				ESP_LOGE(ESP_RECORD , " true == IOT_ESP_Record_ResetSign_Check_b  Reset Record!!!");
				IOT_Printf("\r\n");
				#endif

				iot_Reset_Record();
				GuiInverterFlashData_W_Addr = OFFLINE_DATA_RECORD_ADDR;
				GuiInverterFlashData_R_Addr = OFFLINE_DATA_RECORD_ADDR;
				ucPacketSizeResetFlag = 1;
				GformatParam.uiPacketSize = 0;
				free(pbuf);

				ucErr_t =  0;
				goto END;
			}
			free(pbuf);
		}


		Addr &= (~(uint32_t)(ucPacketType * 1024 - 1));  //ǿ�Ƶ�ַ�� 1k , 2k , 4k ����
		Single_Addr  &= (~(uint32_t)(ucPacketType * 1024 - 1));  //ǿ�Ƶ�ַ�� 1k , 2k , 4k ����

//		if((ucPacketType == 0x00) || (ucPacketType == 0xFF) || (ucPacketType > 4))
//		{
//			ucPacketType = 1;
//			rw_valflag_1 = 0x00;
//		}

		Addr += (ucPacketType * 1024); // ÿ������  (ucPacketType * 1024)byte

		Single_Addr += (ucPacketType * 1024); // ÿ������  (ucPacketType * 1024)byte
	}

	/* �ϵ��ѯ�ش��������ݶ�д��ַ  (һ��ѭ����ѯ4K)*/
	//while( ((!Single_Addr || (Single_Addr % FLASH_ErasePageSize )) && (Addr <= OFFLINE_DATA_RECORD_END_ADDR)))
	while( Addr < OFFLINE_DATA_RECORD_END_ADDR )
	{
		IOT_Mutex_SPI_FLASH_BufferRead(rwbytebuf,Addr,4);
		if((rwbytebuf[0]==0xFF && rwbytebuf[1]==0xFF) || \
		   (rwbytebuf[0]==0x01 && rwbytebuf[1]==0xFF) || \
		   (rwbytebuf[0]==0x01 && rwbytebuf[1]==0x01))
		{
			rw_valflag_2 = rwbytebuf[0] ^ rwbytebuf[1];
		}
		else
		{
			rw_valflag_2 = 0x00;
		}
//		ESP_printf(" ** rwbytebuf[x] == %x - %x - %x - %x**\n",rwbytebuf[0],rwbytebuf[1],rwbytebuf[2],rwbytebuf[3]);
		ucPacketType = rwbytebuf[3];       // !!!ע�⣺�µ�flashû�������ݱ��棬��һ���ϵ� rwbytebuf[3] = 0/0xFF,��������ʱ�� rwbytebuf[3]>4;

	#if LOAD_FORMAT_PARAM
		if(ucPacketType != (GformatParam.uiPacketSize & 0xff))
		{
			if((GformatParam.uiPacketSize != OFFLINE_DATA_PACKET_SIZE_1K)
				|| (GformatParam.uiPacketSize != OFFLINE_DATA_PACKET_SIZE_2K)
				||((GformatParam.uiPacketSize != OFFLINE_DATA_PACKET_SIZE_4K)))
			{
				#if RECORD_DEBUG
				ESP_LOGE(ESP_RECORD , "IOT_ESP_InverterFlashDataRWAddr_Get  failed GformatParam.uiPacketSize = 0x%x",GformatParam.uiPacketSize);
				#endif
				GformatParam.uiPacketSize = OFFLINE_DATA_PACKET_SIZE_1K;
			}

			rw_valflag_2 = 0x00;
			ucPacketType = GformatParam.uiPacketSize & 0xff;
		}
	#else
		if(
			(ucPacketType != LOW(OFFLINE_DATA_PACKET_SIZE_1K))
			&& (ucPacketType != LOW(OFFLINE_DATA_PACKET_SIZE_2K))
			&& (ucPacketType != LOW(OFFLINE_DATA_PACKET_SIZE_4K))
		  )
		{
			rw_valflag_2 = 0x00;	//���Ϊ��д�Ѷ�  �� δдδ��
			#if RECORD_DEBUG
			ESP_LOGE(ESP_RECORD , " ERROR �� ucPacketType = %d",ucPacketType);
			#endif
			ucPacketType = OFFLINE_DATA_PACKET_SIZE_1K;
		}
		else if((rwbytebuf[2] == LOW(DATA_SPACE_SET))  && (ucPacketType != (GformatParam.uiPacketSize & 0xff)))
		{
			if(0 == GformatParam.uiPacketSize)
			{
				GformatParam.uiPacketSize = ucPacketType;
			}
			else	//�������ֲ�ͬ�Ĵ洢��ʽ,���ȫ����������
			{
				iot_Reset_Record();
				ucPacketSizeResetFlag = 1;
				GformatParam.uiPacketSize = 0;
				GuiInverterFlashData_W_Addr = OFFLINE_DATA_RECORD_ADDR;
				GuiInverterFlashData_R_Addr = OFFLINE_DATA_RECORD_ADDR;

				ucErr_t = 0;
				goto END;
			}

		}
	#endif

		Addr &= (~(uint32_t)(ucPacketType * 1024 - 1));  //ǿ�Ƶ�ַ�� 1k , 2k , 4k ����
		Single_Addr  &= (~(uint32_t)(ucPacketType * 1024 - 1));  //ǿ�Ƶ�ַ�� 1k , 2k , 4k ����

//		if((ucPacketType == 0x00) || (ucPacketType == 0xFF) || (ucPacketType > 4))
//		{
//			ucPacketType = 1;
//			rw_valflag_2 = 0x00;
//		}

		if((rw_valflag_1 == 0xFE) && (rw_valflag_2 == 0x00))      // ������д��ַ ����һ������Ϊδ������һ������Ϊ�Ѷ���
		{
			rw_endflag |= 0x10;
			GuiInverterFlashData_W_Addr = Addr;

			#if RECORD_DEBUG
			IOT_Printf(" **IOT_ESP_InverterFlashDataRWAddr_Get w_addr == %x **\n",GuiInverterFlashData_W_Addr);
			#endif
		}
		else if((rw_valflag_1 == 0x00) && (rw_valflag_2 == 0xFE)) // �����ݶ���ַ ����һ������Ϊ�Ѷ�����һ������Ϊδ����
		{
			rw_endflag |= 0x01;
			GuiInverterFlashData_R_Addr = Addr;

			#if RECORD_DEBUG
			IOT_Printf(" **IOT_ESP_InverterFlashDataRWAddr_Get r_addr == %x **\n",GuiInverterFlashData_R_Addr);
			#endif
		}

		if((rw_endflag & 0x11) == 0x11)break;   // ��ǰ��ѯ����д��ַ��������ѯ

		rw_valflag_1 = rw_valflag_2;            // ������һ�α�־λ(0x11\0x1F\0xFF)
		Addr += (ucPacketType * 1024); 			// ÿ������  ���ɸ�����

		Single_Addr += (ucPacketType * 1024); 	//ÿ������  ���ɸ�����
		#if RECORD_DEBUG
		IOT_Printf(" **IOT_ESP_InverterFlashDataRWAddr_Get  loop addr = %x**\n",Addr );
		#endif

		 vTaskDelay(20 / portTICK_PERIOD_MS);
	}

	/*�ϵ��ѯ�ش��������ݶ�д��ַ���� */
	if((Addr >= OFFLINE_DATA_RECORD_END_ADDR) || ((rw_endflag & 0x11) == 0x11))
	{
		#if RECORD_DEBUG
		IOT_Printf("\r\n");
		IOT_Printf("\r\n");
		IOT_Printf(" **IOT_ESP_InverterFlashDataRWAddr_Get successed == r %x - w %x**\n",GuiInverterFlashData_R_Addr , GuiInverterFlashData_W_Addr);
		IOT_Printf("\r\n");
		IOT_Printf("\r\n");
		#endif
		ucErr_t = 1;
		goto END;
	}

//	ESP_printf(" ** Single_Addr rw_addr == %x - %x - %x**\n",Single_Addr,IOTESPInverterFlashData_R_Addr,IOTESPInverterFlashData_W_Addr);
END:
	rw_addr_gotflag = true;
	return ucErr_t;

}

/******************************************************************************
 * FunctionName : IOT_ESP_SPI_FLASH_ReUploadData_Write
 * Description  : ESP �ⲿflash �ش����ݱ���
 * Parameters   : u8 *src : д���ݻ����� ==  �����id(10byte)+time(6byte)+data(������)
 *                u8 len  : д���ݳ���
 * Returns      : none
 * Notice       : 1��flash�������ݱ����ʽ :  д(1byte)+��(1byte)+������Чλ(1byte)+���ȶ�Ӧҳ��(1byte)+����(2byte)+crc(2byte)+�����id(10byte)+time(6byte)+data(������)
 *                  (�� -д ע�ͣ�0x01-��ʾ�Ѷ���д   ��0xFF-��ʾδ��δд)
 *                2��ÿ�ζ�д��ַ�ı�󣬱���Ҫ���ٽ��ַ�жϣ�����
*******************************************************************************/
void IOT_ESP_SPI_FLASH_ReUploadData_Write(uint8_t *src,uint16_t len)
{
	#define dataheadlen 8
	uint8_t ucPacketType = 0;
	uint16_t usDataLen	= 0;
	uint16_t crcval = 0;

	if((rw_addr_gotflag == false) || (true == bRecordClearingFlag))	//δ��ʼ����� ������������˳�
	{
		return;
	}

	usDataLen = len + dataheadlen;

	uint8_t *pStr = (uint8_t *)malloc(usDataLen);
	if(NULL == pStr)
	{
		return;
	}

	bzero(pStr , usDataLen);


	if((usDataLen > (OFFLINE_DATA_PACKET_SIZE_4K * 1024)) || (0 == usDataLen))
	{
		#if RECORD_DEBUG
		ESP_LOGE(ESP_RECORD , "IOT_ESP_SPI_FLASH_ReUploadData_Write usDataLen = %d",usDataLen);
		#endif
		return;
	}
	else if(usDataLen > (OFFLINE_DATA_PACKET_SIZE_2K  * 1024))
	{
		ucPacketType = OFFLINE_DATA_PACKET_SIZE_4K & 0xff;
	}
	else if(usDataLen > (OFFLINE_DATA_PACKET_SIZE_1K  * 1024))
	{
		ucPacketType = OFFLINE_DATA_PACKET_SIZE_2K & 0xff;
	}else
	{
		ucPacketType = OFFLINE_DATA_PACKET_SIZE_1K & 0xff;
	}

#if LOAD_FORMAT_PARAM
	if(ucPacketType != (GformatParam.uiPacketSize & 0xff))
	{
		GformatParam.uiPacketSize = ucPacketType;
		iot_Record_storageFormat_Write(&GformatParam );

		GuiInverterFlashData_R_Addr &= (~(uint32_t)(ucPacketType * 1024 - 1));  //ǿ�Ƶ�ַ�� 1k , 2k , 4k ����
		GuiInverterFlashData_W_Addr &= (~(uint32_t)(ucPacketType * 1024 - 1));  //ǿ�Ƶ�ַ�� 1k , 2k , 4k ����
	}
#else
	if(ucPacketType != (GformatParam.uiPacketSize & 0xff))
	{
		#if RECORD_DEBUG
		ESP_LOGI(ESP_RECORD ,"ucPacketType = %d , GformatParam.uiPacketSize = %d\r\n",ucPacketType , GformatParam.uiPacketSize);
		ESP_LOGI(ESP_RECORD , "IOT_ESP_SPI_FLASH_ReUploadData_Write ucPacketType != GformatParam.uiPacketSize ");
		#endif

		if(ucPacketSizeResetFlag == 1)	//�жϴ洢���Ƿ񱻸�λ , GformatParam.uiPacketSize�Ƿ�reset
		{
			ucPacketSizeResetFlag = 0;
		}else
		{
			#if RECORD_DEBUG
			ESP_LOGI(ESP_RECORD , "Reset Record");
			#endif
			//iot_Reset_Record();
			iot_Reset_Record_Task();		//���� ɾ�������¼����
			GuiInverterFlashData_W_Addr = OFFLINE_DATA_RECORD_ADDR;
			GuiInverterFlashData_R_Addr = OFFLINE_DATA_RECORD_ADDR;
			GformatParam.uiPacketSize = ucPacketType;
			goto END;
		}
		GformatParam.uiPacketSize = ucPacketType;
	}

	GuiInverterFlashData_R_Addr &= (~(uint32_t)(ucPacketType * 1024 - 1));  //ǿ�Ƶ�ַ�� 1k , 2k , 4k ����
	GuiInverterFlashData_W_Addr &= (~(uint32_t)(ucPacketType * 1024 - 1));  //ǿ�Ƶ�ַ�� 1k , 2k , 4k ����

#endif
	#if RECORD_DEBUG
	ESP_LOGI(ESP_RECORD , "IOT_ESP_SPI_FLASH_ReUploadData_Write ucPacketType = %d , GformatParam.uiPacketSize = %d",ucPacketType , GformatParam.uiPacketSize);
	#endif

	pStr[0] = 0x01;				//��д��־
	pStr[1] = 0xff;				//δ����־
	pStr[2] = DATA_SPACE_SET & 0xff;	//���ݱ����λ���Ƿ���Ч
	pStr[3] = ucPacketType;    //��������  1k 2k  4K

	pStr[4] = (len >> 8) & 0xff;
	pStr[5] =  len & 0xff;


	memcpy(&pStr[8] , src , len);
	crcval = GetCRC16(&pStr[8] , len);
	pStr[6] = (crcval >> 8) & 0XFF;
	pStr[7] = crcval & 0XFF;


	IOT_Mutex_SPI_FLASH_WriteBuff(pStr , GuiInverterFlashData_W_Addr , usDataLen);

	GuiInverterFlashData_W_Addr = (GuiInverterFlashData_W_Addr + ucPacketType * 1024);

	#if RECORD_DEBUG
	ESP_LOGI(ESP_RECORD ," GuiInverterFlashData_W_Addr = 0x%x", GuiInverterFlashData_W_Addr);
	#endif

	GuiInverterFlashData_W_Addr &= (~(uint32_t)(ucPacketType * 1024 - 1));  //ǿ�Ƶ�ַ�� 1k , 2k , 4k ����


	if(GuiInverterFlashData_W_Addr >= OFFLINE_DATA_RECORD_END_ADDR)		//����,��ͷ��ʼ��
	{
		#if RECORD_DEBUG
		ESP_LOGI(ESP_RECORD , "record full !!");
		#endif

		GuiInverterFlashData_W_Addr = OFFLINE_DATA_RECORD_ADDR;
	}

	if(GuiInverterFlashData_W_Addr == GuiInverterFlashData_R_Addr) //д��ַ ׷�� ����ַ
	{
		#if RECORD_DEBUG
		ESP_LOGI(ESP_RECORD , "д��ַ׷�϶���ַ");
		#endif

		GuiInverterFlashData_R_Addr = GuiInverterFlashData_W_Addr + (ucPacketType * 1024);

		if(GuiInverterFlashData_R_Addr >= OFFLINE_DATA_RECORD_END_ADDR)
		{
			GuiInverterFlashData_R_Addr = OFFLINE_DATA_RECORD_ADDR;
		}

		#if RECORD_DEBUG
		ESP_LOGI(ESP_RECORD , "GuiInverterFlashData_R_Addr = 0x%x , GuiInverterFlashData_w_Addr = 0x%x",GuiInverterFlashData_R_Addr,GuiInverterFlashData_W_Addr );
		#endif

		GuiInverterFlashData_R_Addr &= (~(uint32_t)(ucPacketType * 1024 - 1));  //ǿ�Ƶ�ַ�� 1k , 2k , 4k ����
	}

	#if RECORD_DEBUG
	ESP_LOGI(ESP_RECORD , "sava success GuiInverterFlashData_R_Addr = 0x%x , GuiInverterFlashData_w_Addr = 0x%x",GuiInverterFlashData_R_Addr,GuiInverterFlashData_W_Addr );
	#endif

END:
	free(pStr);
}


/******************************************************************************
 * FunctionName : IOT_ESP_SPI_FLASH_ReUploadData_Read_us
 * Description  : ESP �ⲿflash �ش����ݱ���
 * Parameters   : u8 *src : д���ݻ����� ==  �����id(10byte)+time(6byte)+data(������)
 *                u8 len  : д���ݳ���
 * Returns      : ���ݵĳ��� , ����ȡ���������򷵻�0
 * Notice       : 1��flash�������ݱ����ʽ :  д(1byte)+��(1byte)+������Чλ(1byte)+���ȶ�Ӧҳ��(1byte)+����(2byte)+crc(2byte)+�����id(10byte)+time(6byte)+data(������)
 *                  (�� -д ע�ͣ�0x01-��ʾ�Ѷ���д   ��0xFF-��ʾδ��δд)
 *                2��ÿ�ζ�д��ַ�ı�󣬱���Ҫ���ٽ��ַ�жϣ�����
*******************************************************************************/
uint16_t IOT_ESP_SPI_FLASH_ReUploadData_Read_us(uint8_t *output,uint16_t BUffLen)
{
	#define dataheadlen 8
	uint8_t ucHeadDataBuf[8] = {0};
	uint8_t ucWriteBuf[2] = {0};
	uint8_t ucPacketType = OFFLINE_DATA_PACKET_SIZE_1K;
	uint16_t crcval = 0;
	uint16_t usDataLen	= 0;
	uint8_t * pbuff = NULL;

	if((rw_addr_gotflag == false) || (true == bRecordClearingFlag))
	{
		return 0;
	}

	ucWriteBuf[1] = (DATA_SPACE_SET & 0xff);

	#if RECORD_DEBUG
	ESP_LOGI(ESP_RECORD , "IOT_ESP_SPI_FLASH_ReUploadData_Read_us GuiInverterFlashData_R_Addr = 0x%x",GuiInverterFlashData_R_Addr);
	#endif

	IOT_Mutex_SPI_FLASH_BufferRead(ucHeadDataBuf , GuiInverterFlashData_R_Addr , 8);

	if((ucHeadDataBuf[0] == 0x01) && (ucHeadDataBuf[1] == 0xff))	//�ж������Ƿ�Ϊδ������
	{
		if(ucHeadDataBuf[2] != (DATA_SPACE_SET & 0xff))		//�ж������Ƿ���Ч
		{
			#if RECORD_DEBUG
			ESP_LOGI(ESP_RECORD , "ucHeadDataBuf[2]  != DATA_SPACE_SET");
			#endif

			usDataLen = 0;
			ucWriteBuf[1] = (DATA_SPACE_RESET & 0xff);		//�������Ϊ��Ч
			goto ERR;
		}
		ucPacketType = ucHeadDataBuf[3];	//��ȡPacket ����

		if((ucPacketType != (GformatParam.uiPacketSize & 0xff)) && (0 != GformatParam.uiPacketSize))
		{
			#if RECORD_DEBUG
			ESP_LOGI(ESP_RECORD , "ucPacketType != GformatParam.uiPacketSize");
			#endif
			ucPacketType = (GformatParam.uiPacketSize & 0xff);
			usDataLen = 0;
			ucWriteBuf[1] = (DATA_SPACE_RESET & 0xff);		//�������Ϊ��Ч

			goto ERR;
		}

		pbuff = (uint8_t *)malloc(ucPacketType  * 1024);
		if(NULL == pbuff)
		{
			#if RECORD_DEBUG
			ESP_LOGI(ESP_RECORD , "malloc failed pbuff == NULL");
			#endif
			return 0;
		}
		bzero(pbuff , ucPacketType  * 1024);

		IOT_Mutex_SPI_FLASH_BufferRead(pbuff , GuiInverterFlashData_R_Addr + dataheadlen, (ucPacketType * 1024 - dataheadlen));

		usDataLen = ucHeadDataBuf[4];
		usDataLen = (usDataLen << 8)  | ucHeadDataBuf[5];

		crcval = GetCRC16(pbuff , usDataLen);

		if((ucHeadDataBuf[6] == (uint8_t)((crcval>>8) & 0x00FF)) && (ucHeadDataBuf[7] == (uint8_t)(crcval & 0x00FF)))// ��λ  ��λ crc �ж�
		{
			//if(IOT_ESP_strstr((const uint8_t *)pbuff,(const uint8_t *)"1234567890",10,10))
			if(1)		//��������кŲ����ж�
			{
				if(usDataLen > BUffLen)
				{
					usDataLen = BUffLen;
				}

				memcpy(output ,pbuff ,  usDataLen);
			}else	//��������кŴ���
			{
				#if RECORD_DEBUG
				ESP_LOGI(ESP_RECORD , "ReUploadData_Read SN Check failed SN : %s",pbuff);
				#endif
				usDataLen = 0;
				ucWriteBuf[1] = (DATA_SPACE_RESET & 0xff);		//�������Ϊ��Ч
			}
		}
		else
		{
			usDataLen = 0;
			uint32_t data_crc = ucHeadDataBuf[6];
			data_crc = data_crc << 8 | ucHeadDataBuf[7];
			#if RECORD_DEBUG
			IOT_Printf("IOT_ESP_S_ReUploadData_Read crc fail crcval = %x , data_crc =%x, read_addr = %x!\r\n",crcval , data_crc ,GuiInverterFlashData_R_Addr);
			#endif
		}
		free(pbuff);
ERR:
		ucWriteBuf[0] = 0x01;		//�Ѷ�
		IOT_Mutex_SPI_FLASH_WriteBuff(&ucWriteBuf[0] , GuiInverterFlashData_R_Addr + 1, 2);	//д���Ѷ���־��������Чλ
	}

	GuiInverterFlashData_R_Addr +=  ucPacketType * 1024;
	GuiInverterFlashData_R_Addr &= (~(uint32_t)(ucPacketType * 1024 - 1));  //ǿ�Ƶ�ַ�� 1k , 2k , 4k ����


	if(GuiInverterFlashData_R_Addr >= OFFLINE_DATA_RECORD_END_ADDR)		//����ַ Ϊ�洢��ĩ��ַ
	{
		#if RECORD_DEBUG
		ESP_LOGI(ESP_RECORD , " IOT_ESP_SPI_FLASH_ReUploadData_Read_us Read end !!");
		#endif
		GuiInverterFlashData_R_Addr = OFFLINE_DATA_RECORD_ADDR;		//��ͷ��ʼ��
	}
	#if RECORD_DEBUG
	ESP_LOGI(ESP_RECORD , "IOT_ESP_SPI_FLASH_ReUploadData_Read_us NEW GuiInverterFlashData_R_Addr = 0x%x",GuiInverterFlashData_R_Addr);
	#endif
	return usDataLen;
}


/******************************************************************************
 * FunctionName : IOT_ESP_Record_CleanSign_Write
 * Description  : д���¼ ��λ��־
 * Parameters   :
 * Returns      : none
 * Notice       :
*******************************************************************************/
void IOT_ESP_Record_ResetSign_Write(void)
{
	uint8_t  pStrBuf[128] = {0};
	uint16_t usLen = 0;
	uint16_t crcval = 0;

	pStrBuf[0] = 0x01;								//��д��־
	pStrBuf[1] = 0xff;								//δ����־
	pStrBuf[2] = DATA_SPACE_RESET_RECORD & 0xff;	//��ռ�¼
	pStrBuf[3] = 0x01;    //��������  1k 2k  4K

	usLen =	strlen(RESET_SIGN);
	pStrBuf[4] = (usLen >> 8) & 0xff;
	pStrBuf[5] =  usLen & 0xff;


	memcpy(&pStrBuf[8] , RESET_SIGN , usLen);
	crcval = GetCRC16(&pStrBuf[8] , usLen);
	pStrBuf[6] = (crcval >> 8) & 0XFF;
	pStrBuf[7] = crcval & 0XFF;


	IOT_Mutex_SPI_FLASH_WriteBuff(pStrBuf , OFFLINE_DATA_RECORD_ADDR , usLen + 8);

	GuiInverterFlashData_W_Addr = OFFLINE_DATA_RECORD_ADDR + 4096;
	GuiInverterFlashData_R_Addr =GuiInverterFlashData_W_Addr + 4096;

}


/******************************************************************************
 * FunctionName : IOT_ESP_Record_CleanSign_Write
 * Description  : �ж������Ƿ�Ϊ�����������־
 * Parameters   : pucData_tem
 * Returns      : none
 * Notice       :
*******************************************************************************/
bool IOT_ESP_Record_ResetSign_Check_b(uint8_t *pucData_tem)
{
	uint8_t *pStr = pucData_tem;
	uint16_t usLen = sizeof(RESET_SIGN);
	uint16_t usDataLen = 0;
	uint16_t crcval = 0;

	char *pSignbuff = (char *)malloc(usLen);

	if((NULL == pSignbuff) || (NULL == pucData_tem))
	{
		goto END;
	}

	bzero(pSignbuff , usLen);



	if((pStr[2] == (DATA_SPACE_RESET_RECORD & 0x00ff)) && (pStr[1] == 0xff))
	{
		usDataLen = pStr[4];
		usDataLen = (usDataLen << 8) |( pStr[5] & 0xff);
		memcpy(pSignbuff , &pStr[8] , MIN(usDataLen , usLen));

		if(strcmp((const char*)pSignbuff , (const char*)RESET_SIGN ) == 0)
		{
			free(pSignbuff);
			return true;
		}
	}
END:
	if(NULL != pSignbuff)
		free(pSignbuff);

	return false;
}


