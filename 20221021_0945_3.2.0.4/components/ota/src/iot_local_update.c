#include "iot_local_update.h"
#include "iot_universal.h"
#include "iot_ota.h"
#include "esp_err.h"
#include "iot_fota.h"
#include "iot_system.h"

#include "iot_protocol.h"
#include "esp_log.h"
#include "iot_timer.h"
#include "iot_inverter_fota.h"
#include "iot_Local_Protocol.h"

#if 0
������ʼ����ɱ�־��Ҫ�Ż�����ֹ����������ͻ
#endif

#define LOG_LOCAL_UPDATE "LOG_LOCAL_UPDATE"


extern uint8_t encrypt(uint8_t *pdata, uint16_t length);
extern uint8_t IOT_SystemParameterSet(uint16_t ParamValue ,char * data,uint8_t len);
/******************************************************************************
 * FunctionName : IOT_WiFiConnect_Handle_uc
 * Description  : WiFi����״̬�ж�
 * Parameters   : u8 ucMode_tem: ģʽ
 *               u16 usiVal_tem: ֵ
 * Returns      : 0-�ɹ�   / 1-ʧ��
 * Notice       : none
*******************************************************************************/
uint8_t IOT_LocalFota_Handle_uc(uint8_t ucMode_tem, uint8_t *pucSrc_tem, uint16_t usiVal_tem)
{
	int err_t = 1;
	char cFotaType = EFOTA_TYPR_FULL;
	int OTA_TYPE =	EUPDADTE_NULL;

	if(ucMode_tem == 0) // ���յ���0������
	{

		cFotaType = IOT_FotaType_Manage_c(0xff, 0);

		if(cFotaType == EFOTA_TYPR_DATALOGER)
		{
			OTA_TYPE = EUPDADTE_COLLACTOR;
		}else
		{
			OTA_TYPE = EUPDADTE_INERTER;
		}

		if(ESP_OK == IOT_OTA_Init_uc(OTA_TYPE,g_FotaParams.ucIsbsdiffFlag))	//��ʼ������
		{
			err_t = 0;
		}

	}
	else if(ucMode_tem == 1) // ���յ��̼�����������
	{
		if(ESP_OK == IOT_OTA_Write_uc(pucSrc_tem , usiVal_tem))
		{
			err_t = 0;
		}
	}

	return err_t;
}


/******************************************************************************
 * FunctionName : LocalUpdate_ZeroPackage_judge_b
 * Description  : �жϱ���������0�������Ƿ��ȡ
 * Parameters   : u8 ucMode_tem: ģʽ	1 ����  0 ��λ  0xff ��ѯ
 *               bool bFlags_tem: ֵ
 * Returns      : true-�ѻ�ȡ   / false-δ��ȡ
 * Notice       : none
*******************************************************************************/
bool LocalUpdate_ZeroPackage_judge_b(uint8_t ucMode_tem , bool bFlags_tem)
{
	static bool bGetZeroPackageFlag = false;		//�Ƿ��Ѿ���ȡ����0�����ݱ��

	if(1 == ucMode_tem)	//����flag
	{
		bGetZeroPackageFlag = bFlags_tem;
	}
	else if(0 == ucMode_tem)
	{
		bGetZeroPackageFlag = false;
	}
	else if(0xff == ucMode_tem)
	{
		return bGetZeroPackageFlag;
	}

	return false;
}


/******************************************************************************
 * FunctionName : IOT_LocalTCPAck0x26_Package
 * Description  : ��������0x26��������Ӧ���ݴ��
 * Parameters   : u8 ucMode_tem: ģʽ	1 �������    0xff ��ȡ��Ӧ���ݰ�
 *               uint8_t *TargetBUF ��Ӧ����ŵ�buff
 *                uint8_t AckData_tem[5] Ӧ������
 * Returns      : ���ݰ��ĳ���
 * Notice       : none
*******************************************************************************/
uint32_t IOT_LocalTCPAck0x26_Package(uint8_t ucMode_tem , uint8_t *TargetBUF, uint8_t AckData_tem[5])
{
	#ifndef _VERSION_		//Э��汾
	#define	_VERSION_ 		0x0005
	#endif

	static uint8_t 	ACK_DATA[30] = {0};
	static uint16_t ACK_DATA_Len = 0;
	uint16_t len = 0;
	uint32_t uiCRC16 = 0;

	if(1 == ucMode_tem)	//�������
	{
		ACK_DATA[len++] = 0x00;	//ͨ�ű��
		ACK_DATA[len++] = 0x01;
		ACK_DATA[len++] = (_VERSION_ >> 8) & 0xff;	//Э��汾
		ACK_DATA[len++] = _VERSION_ & 0xff;
		ACK_DATA[len++]	= 0x00;
		ACK_DATA[len++] = (2 + 10 + 5);
		ACK_DATA[len++] = 0x01;  //�豸��ַ
		ACK_DATA[len++] = 0x26;  //������

		memcpy(&ACK_DATA[len], &SysTAB_Parameter[Parameter_len[REG_SN]], 10);		 //�ɼ���SN��
		len += 10;
		memcpy(&ACK_DATA[18], AckData_tem, 5);
		len += 5;

		ACK_DATA_Len = len;
	}
	else if(0xff == ucMode_tem)
	{
		if(TargetBUF == NULL)
		{
			return 0;
		}


		if((LOCAL_PROTOCOL & 0x000000ff) == (ProtocolVS06  & 0x000000ff))	//����ͨѶ6.0Э��
		{
			ACK_DATA_Len = IOT_Repack_ProtocolData_V5ToV6_us(ACK_DATA, ACK_DATA_Len , TargetBUF);
		}else
		{
			memcpy(TargetBUF, ACK_DATA, ACK_DATA_Len);
			encrypt(&TargetBUF[8], (ACK_DATA_Len - 8));		//������
			IOT_Printf("\r\n***********************encrypt**************************  \r\n",ACK_DATA_Len);

			uiCRC16 = Modbus_Caluation_CRC16(TargetBUF, ACK_DATA_Len);
			TargetBUF[ACK_DATA_Len] = HIGH(uiCRC16);
			TargetBUF[ACK_DATA_Len+1] = LOW(uiCRC16);
			ACK_DATA_Len = ACK_DATA_Len + 2;

		}

		for(int i = 0 ; i < ACK_DATA_Len ; i++)
		{
			IOT_Printf("%02x",TargetBUF[i]);
		}
	}

	return ACK_DATA_Len;
}

/******************************************************************************
 * FunctionName : IOT_LocalTCPCode0x26_Handle() add LI 20210115
 * Description  : ���ձ��� �������ݰ�
 * Parameters   : unsigned char *pucSrc_tem : �������ݰ�
 * Returns      : none
 * Notice       : 0x19:
 * 						����0001 0005 0014 01 26 30303030303030303030 [0006 00FF 0001 0001 2*crc] - �·��ܰ� 255/��һ���� /���� 0001
 * 						Ӧ��0001 0005 0013 01 26 30303030303030303030 [00FF 0001 00 2*crc] - �·��ܰ� 255/��һ���� /״̬ 00
*******************************************************************************/
void IOT_LocalTCPCode0x26_Handle(unsigned char *pucSrc_tem)
{
	#define SINGLE_PACKAGE_SIZE  512

	unsigned char ucAckBuf[5] = {0};
	unsigned char ucAcklen = 0;
	unsigned char ucDataOffset = 6;         // ����ƫ�Ƶ�ַ��6-��ȥ���ݳ���2b / �ܰ���2b / ��ǰ����2b
	unsigned char i = 0;
	unsigned int  uiPackageLen = 0;         // ����������
	unsigned int  uiSumPackageNums = 0;       // ����������
	unsigned int  uiPointPackageNums = 0;     // �ϵ�İ���
	unsigned int  uiCurrentPackageNums = 0; // ��ǰ���������
	uint8_t  ucReDownloadFlag = 0;

	#if 0
	�ֶν���

	�������ݸ�ʽ
	pucSrc_tem[00 ~ 01]	����������
	pucSrc_tem[02 ~ 03]	�ļ����ݷְ�������
	pucSrc_tem[04 ~ 05]	��ǰ���ݰ����
	pucSrc_tem[06 ~ xx]	����������

	��Ӧ�����ݸ�ʽ

	ucAckBuf[00 ~ 01]	�ļ����ݷְ�������
	ucAckBuf[02 ~ 03]	��ǰ���ݰ����
	ucAckBuf[04]		����״̬��
	#endif


	ucAckBuf[0] = pucSrc_tem[2];
	ucAckBuf[1] = pucSrc_tem[3]; // �ܰ�����
	ucAckBuf[2] = pucSrc_tem[4];
	ucAckBuf[3] = pucSrc_tem[5]; // ��ǰ����
	ucAckBuf[4] = 0x00;          // Ӧ��״̬ -- �ɹ�
	ucAcklen = 7;                // Ӧ�𳤶�

	uiPackageLen = IOT_htoi(&pucSrc_tem[0],2);
	uiSumPackageNums = IOT_htoi(&pucSrc_tem[2],2);
	uiCurrentPackageNums = IOT_htoi(&pucSrc_tem[4],2);

	IOT_Printf(" \r\nIOT_LocalTCPCode0x26_Handle \r\n");
	IOT_Printf(" uiPackageLen = %d\r\n",uiPackageLen);
	IOT_Printf(" uiSumPackageNums = %d\r\n",uiSumPackageNums);
	IOT_Printf(" uiCurrentPackageNums = %d\r\n",uiCurrentPackageNums);
//	IOT_ESP_Char_printf("IOT_LocalTCPCode0x26_Handle()_1", &pucSrc_tem[ucDataOffset], uiPackageLen-4);

	if((uiPackageLen > (SINGLE_PACKAGE_SIZE + 4)) || \
	   (uiSumPackageNums > ((2048 * 1024) / SINGLE_PACKAGE_SIZE)))     // �����������ݰ����ȴ��� 1024�ֽ� / ���������ݰ���С> 2M
	{
		ucAckBuf[4] = 0x02;          // Ӧ��״̬ -- ����������ʧ��ʧ��
		goto ACK_FAIL_0X26;
	}

	if(uiCurrentPackageNums == 0x00) // ��һ������  ��ʼ��ȡ�ϵ����ݰ����  ��ʼ������׼��
	{

		if(IOT_LocalFota_Handle_uc(0, &pucSrc_tem[ucDataOffset], (uiPackageLen-4))) // д���һ������, ������ʼ��
		{
			ucAckBuf[4] = 0x01;      // Ӧ��״̬ -- ���� 1
			goto ACK_FAIL_0X26;
		}

		IOT_MasterFotaStatus_SetAndGet_ui(1 , EFOTA_LOCAL_UPDATE);  //����״̬Ϊ��������
		//У���ļ�ͷ,���ϵ�λ��
		if(0 == IOT_FotaFile_HEAD_Check_c(&pucSrc_tem[ucDataOffset]))
		{
			//Flash�ѱ�������ͬ���ļ�,�����ظ�����
			ucReDownloadFlag = 1;	//add chen 20220712  �ѱ�������ͬ���ļ� ����Ҫ�ظ�У��
			ESP_LOGI(LOG_LOCAL_UPDATE,"The Flash has saved the same file!!");
			uiPointPackageNums = uiSumPackageNums;
			ucAckBuf[2] = (uiPointPackageNums >> 8 ) & 0xff; //��λ
			ucAckBuf[3] = uiPointPackageNums  & 0xff ; 		// ��λ


		}else
		{
			#if 0	//���Դ���
			g_FotaParams.uiPointSize = 0 ;
			IOT_FotaParams_Write_c(4, g_FotaParams.uiPointSize);
			#endif

			//Ӧ��Ķϵ�λ��
			uiPointPackageNums = g_FotaParams.uiPointSize / SINGLE_PACKAGE_SIZE;
			g_FotaParams.uiPointSize = (uiPointPackageNums * SINGLE_PACKAGE_SIZE); //�ϵ���ʼ��ַ ��ҪΪ SINGLE_PACKAGE_SIZE �ı���

			ucAckBuf[2] = (uiPointPackageNums >> 8 ) & 0xff; //��λ
			ucAckBuf[3] = uiPointPackageNums  & 0xff ; // ��λ

			ESP_LOGI(LOG_LOCAL_UPDATE,"uiPointSize = %d, PointPackageNums = %d",g_FotaParams.uiPointSize,uiPointPackageNums);
		}

		IOT_OTA_Set_Written_Offset_uc(g_FotaParams.uiPointSize); //���������������ݵ�λ��ƫ��
		LocalUpdate_ZeroPackage_judge_b(1 , true);		//����Ѿ���ȡ���0������

	}
	else // ��ʼ���������ļ���
	{
		if(LocalUpdate_ZeroPackage_judge_b( 0xff , 0) != true)	//δ��ȡ����0������,���»�ȡ
		{
			ucAckBuf[2] = 0x00;	//����Ӧ����Ϊ0
			ucAckBuf[3] = 0x00;
			ucAckBuf[4] = 0x03;      // Ӧ��״̬ -- 03 ��������
			goto ACK_FAIL_0X26;
		}
		if(IOT_LocalFota_Handle_uc(1, &pucSrc_tem[ucDataOffset], (uiPackageLen-4))) // д������������
		{
			ucAckBuf[4] = 0x01;      // Ӧ��״̬ -- ����������ʧ��ʧ��
			goto ACK_FAIL_0X26;
		}
		g_FotaParams.uiPointSize += (uiPackageLen - 4);		//�ϵ�λ�ø���
		if((uiCurrentPackageNums != 0) && (uiCurrentPackageNums % 40 == 0))		//ÿ40������һ�ζϵ�
		{
			IOT_FotaParams_Write_c(4, g_FotaParams.uiPointSize); // ��¼HTTP�ϵ����ر������ݳ��ȣ����=4

			uint8_t ucDLProgress = (uiCurrentPackageNums * 100) / uiSumPackageNums;

			char sProgress[3] = {0};

			if(ucDLProgress >= 100)
			{
				ucDLProgress = 99;   //���1% ��ҪУ������ڸ���
			}

			sProgress[0] ='0' + ((ucDLProgress / 100) % 10);
			sProgress[1] ='0' + ((ucDLProgress / 10) % 10) ;
			sProgress[2] ='0' + (ucDLProgress % 10) ;

			IOT_SystemParameterSet(101 ,(char *)sProgress , 3);  //�������
		}
	}
	if((uiSumPackageNums == uiCurrentPackageNums) || (1 == ucReDownloadFlag)) // �����ļ����·���ɣ���ʼ��������
	{
		if((1 == ucReDownloadFlag) || (true == IOT_FotaFile_Integrality_Check_b(1)))	//У��ͨ��
		{
			char cOTAType = IOT_FotaType_Manage_c(0xff, 0);//��ȡ����������
			ucAckBuf[4] = 0x00;          // Ӧ��״̬ -- �ɹ�
			LocalUpdate_ZeroPackage_judge_b(1 , false);	//��ջ�ȡ�ļ�ͷ��־
			if(cOTAType == EFOTA_TYPR_DATALOGER)
			{
				if(true == g_FotaParams.ucIsbsdiffFlag)	//�ж��Ƿ���в������
				{
					iot_Bsdiff_Update_Judge_uc(1);
				}
				else
				{
					IOT_MasterFotaStatus_SetAndGet_ui(1 , EFOTA_FINISH);

					if(true == IOT_FotaFile_DeviceType_Check_b())	//У�������ļ����豸����
					{
						IOT_SystemParameterSet(101 ,(char *)"100" , 3);  //�������
						IOT_OTA_Set_Boot();		//������������
					}else
					{
						IOT_SystemParameterSet(101 ,(char *)"134" , 3);  //ʧ��
					}
					IOT_OTA_Finish_uc();	//OTA����
					IOT_SYSRESTTime_uc(ESP_WRITE, 30 );  //�ȴ�30s ������
					System_state.SYS_RestartFlag = 1;
				}
			}else
			{
				INVFOAT.InvUpdateFlag = 1;		//�������������
				//IOT_MasterFotaStatus_SetAndGet_ui(1 , EFOTA_UPDATE_DEVICE);
				IOT_MasterFotaStatus_SetAndGet_ui(1 ,EFOTA_LOCAL_UPDATE_DEVICE);
			}

			if(0 == ucReDownloadFlag)
			{
				IOT_FotaParams_Write_c(4, g_FotaParams.uiPointSize); //����ϵ�
			}

		}else	//�����ļ�У�鲻ͨ��
		{
			g_FotaParams.uiPointSize = 0;  				//�ϵ���0	,�´����ش�0��ʼ
			IOT_FotaParams_Write_c(4, g_FotaParams.uiPointSize); //����ϵ�
			LocalUpdate_ZeroPackage_judge_b(0 , 0);	 		//��ȡ�ļ�ͷ��־��0

			IOT_SystemParameterSet(101 ,(char *)"134" , 3);  //�������,�̼��Ƿ�

			IOT_MasterFotaStatus_SetAndGet_ui(1 , EFOTA_FINISH);
			ucAckBuf[4] = 0x02;      // Ӧ��״̬ -- ����У�����
			goto ACK_FAIL_0X26;
		}
	}

ACK_FAIL_0X26:

IOT_LocalTCPAck0x26_Package(1 ,NULL ,ucAckBuf);	//���0x26��������Ӧ��

	//IOT_LocalFotaStatus_check_uc(); // ����״̬���

}


