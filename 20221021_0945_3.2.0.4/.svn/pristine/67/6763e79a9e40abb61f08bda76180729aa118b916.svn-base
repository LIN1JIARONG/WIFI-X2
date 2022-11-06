/*
 * iot_inverter_fota.h
 *
 *  Created on: 2022年3月21日
 *      Author: Administrator
 */

#ifndef COMPONENTS_INCLUDES_IOT_INVERTER_FOTA_H_
#define COMPONENTS_INCLUDES_IOT_INVERTER_FOTA_H_



/*
升级命令及应答格式定义：
"逆变器地址 命令码 寄存器 长度 数据 校验位 H 校验位 L"
命令码：0x06
		寄存器 0x1f：上位机下发烧录文件
命令码：0x17
		寄存器 0x02：烧录文件长度
		寄存器 0x03：烧录文件 CRC 校验值
		寄存器 0x04：擦除 Flash
		寄存器 0x05：发送烧录数据
		寄存器 0x06：烧录文件发送结束
命令码：0x03
		寄存器 0x1f：查询烧录进度
*/

#define FotaSendTimes     5       // 升级命令最多发送次数
#define FotaSendinterval  3000    // 升级命令发送时间间隔 单位：1000 = 1s

/* 升级文件类型定义 */
enum _FotaFileType{
	file_bin = 0x0001,
	file_hex = 0x0010
};

/* 命令码发送状态定义 */
enum _FotaCodeStatus{
	code_wait = 0,  // 发送等待
	code_ing,       // 发送中
	code_end        // 发送结束
};

/* 数服协议命令码参数定义 */
enum _GPFotaCode{
	GPFotaCode_06 = 0,
	GPFotaCode_17,
	GPFotaCode_03,
	GPFotaCodeNum
}GPFotaCode;

/* 数服协议命令码-0x06对应寄存器结构体定义 */
enum _GPFotaCode_06_register{
	Code_06_register_02 = 0,   // 向 22（0x16）号保持寄存器写入 0x00，不保存波特率
	Code_06_register_16,       // 向向 22（0x16） 号保持寄存器写入 0x01，设置波特率为 38400
	Code_06_register_1F,       // 上位机下发烧录文件
	Code_06_register_num       // 0x06寄存器个数
};

/* 数服协议命令码-0x17对应寄存器结构体定义 */
enum _GPFotaCode_17_register{
	Code_17_register_02 = 0,  // 烧录文件长度
	Code_17_register_03,      // 烧录文件 CRC 校验值
	Code_17_register_04,      // 擦除 Flash
	Code_17_register_05,      // 发送烧录数据
	Code_17_register_06,      // 烧录文件发送结束
	Code_17_register_num      // 0x17寄存器个数
};

/* 数服协议命令码-0x03对应寄存器结构体定义 */
enum _GPFotaCode_03_register{
	Code_03_register_1F = 0,  // 查询烧录进度
	Code_03_register_num      // 0x03寄存器个数
};

/* 数服协议命令码的寄存器结构体定义 */
typedef struct _GPFotaCode_register{
	unsigned char Code_06_register[Code_06_register_num];
	unsigned char Code_17_register[Code_17_register_num];
	unsigned char Code_03_register[Code_03_register_num];
}GPFotaCode_register;

/* 数服协议参数定义 */
typedef struct _GPFotaParam{
	unsigned char code;                // 命令码
	unsigned char coderegister;        // 寄存器
	unsigned char sendtimes;           // 发送次数
	unsigned int  sendinterval;        // 发送间隔
	unsigned char status;              // 命令码发送状态
}GPFotaParam;


#pragma pack(4)
typedef struct {
	volatile uint8_t InvUpdateFlag;			//逆变器开始升级标志位
	uint32_t InvUpdtOuttime;		    	 //逆变器升级超时时间(不包括下载时间)
	uint32_t DLOuttime;						//升级逆变器，单笔数据之间的超时时间

} INVFota_date;
extern INVFota_date INVFOAT;

extern GPFotaCode_register GPFotaCode_registers;
extern GPFotaParam GPFotaParams[GPFotaCodeNum];

extern unsigned char GucCode_06_register[Code_06_register_num];
extern unsigned char GucCode_17_register[Code_17_register_num];
extern unsigned char GucCode_03_register[Code_03_register_num];
#define UsartDataMaxLen  36
extern unsigned char IOTESPUsart0RcvBuf[UsartDataMaxLen];
extern unsigned char IOTESPUsart0RcvLen ;

uint8_t IOT_GetInvUpdtStat(void);



void IOT_ESP_NEW_InvUpdataInit(void);
uint8_t IOT_ESP_NEW_UpdataPV(void);



#endif /* COMPONENTS_INCLUDES_IOT_INVERTER_FOTA_H_ */
