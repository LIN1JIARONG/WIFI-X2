/*
 * iot_params.h
 *
 *  Created on: 2021��12��20��
 *      Author: Administrator
 */

#ifndef COMPONENTS_INCLUDES_IOT_PARAMS_H_
#define COMPONENTS_INCLUDES_IOT_PARAMS_H_


#ifndef IOT_PARAMS_GLOBAL
#define IOT_PARAMS_EXTERN extern
#else
#define IOT_PARAMS_EXTERN
#endif


/****************FLASH(SOC) ��ַ������*************************************************/
/*
	���ݱ����ʽ:[����������   ][��������   ][������д��־λ     ][������crc16ֵ  ]
             [  256b    ][  3834b ][      4b      ][      2b     ]
	     ������[  0-255   ][256-4089][   4090-4093  ][  4094-4095  ]
*/
/* ϵͳ����flash��ַ������ */
#define PARAMS_ZONE_WRITED_SIGN     "sign"      // ������д��־λ
#define PARAMS_ZONE_PAGE_SIZE       4096        // ������ҳ���ݴ�С��4k
#define PARAMS_LEN_OFFSET           256         // ���������Ч���ݳ���ֵ���ֽ�����256�ֽ�
#define PARAMS_SIGN_OFFSET          4090        // ���������д���־λ���ֽ���ǰ��4�ֽ�
#define PARAMS_CRC_OFFSET           4094        // �������crcУ��ֵ���ֽ�����2�ֽ�

/* ��������flash��ַ������ */
#define FOTA_PARAMS_ADDR            0x003F1000  // 4032K~4036K ��������������  �ܹ���4k

/****************FLASH(SOC) ��ַ������*************************************************/

IOT_PARAMS_EXTERN void IOT_AddParam_Init(void);
IOT_PARAMS_EXTERN void IOT_AddParam_Write(unsigned char *pucSrc_tem, unsigned char ucNum_tem, unsigned char ucLen_tem);
IOT_PARAMS_EXTERN void IOT_AddParam_Read(unsigned char *pucSrc_tem, unsigned char ucNum_tem);



#endif /* COMPONENTS_INCLUDES_IOT_PARAMS_H_ */
