/*
 * iot_params.h
 *
 *  Created on: 2021年12月20日
 *      Author: Administrator
 */

#ifndef COMPONENTS_INCLUDES_IOT_PARAMS_H_
#define COMPONENTS_INCLUDES_IOT_PARAMS_H_


#ifndef IOT_PARAMS_GLOBAL
#define IOT_PARAMS_EXTERN extern
#else
#define IOT_PARAMS_EXTERN
#endif


/****************FLASH(SOC) 地址定义区*************************************************/
/*
	数据保存格式:[各参数长度   ][参数数据   ][数据区写标志位     ][参数区crc16值  ]
             [  256b    ][  3834b ][      4b      ][      2b     ]
	     举例：[  0-255   ][256-4089][   4090-4093  ][  4094-4095  ]
*/
/* 系统参数flash地址定义区 */
#define PARAMS_ZONE_WRITED_SIGN     "sign"      // 参数区写标志位
#define PARAMS_ZONE_PAGE_SIZE       4096        // 参数区页数据大小：4k
#define PARAMS_LEN_OFFSET           256         // 保存参数有效数据长度值，字节数：256字节
#define PARAMS_SIGN_OFFSET          4090        // 保存参数区写入标志位，字节数前：4字节
#define PARAMS_CRC_OFFSET           4094        // 保存参数crc校验值，字节数：2字节

/* 升级参数flash地址定义区 */
#define FOTA_PARAMS_ADDR            0x003F1000  // 4032K~4036K 升级升级参数区  总共：4k

/****************FLASH(SOC) 地址定义区*************************************************/

IOT_PARAMS_EXTERN void IOT_AddParam_Init(void);
IOT_PARAMS_EXTERN void IOT_AddParam_Write(unsigned char *pucSrc_tem, unsigned char ucNum_tem, unsigned char ucLen_tem);
IOT_PARAMS_EXTERN void IOT_AddParam_Read(unsigned char *pucSrc_tem, unsigned char ucNum_tem);



#endif /* COMPONENTS_INCLUDES_IOT_PARAMS_H_ */
