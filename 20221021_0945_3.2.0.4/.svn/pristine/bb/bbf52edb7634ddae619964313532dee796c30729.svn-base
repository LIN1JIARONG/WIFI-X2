/*
 * iot_InvWave.h
 *
 *  Created on: 2022年6月11日
 *      Author: Administrator
 */

#ifndef COMPONENTS_INCLUDES_IOT_INVWAVE_H_
#define COMPONENTS_INCLUDES_IOT_INVWAVE_H_

#include <stdbool.h>
#include <stdint.h>

typedef struct _WaveParam{
 uint16_t WaveType;          // 波形类型 （PV曲线、故障录波、实时波形、一键诊断、AC电压谐波、阻抗波形）
 uint16_t RegAddrStart;      // 读取波形的寄存器起始地址
 uint16_t RegAddrEnd;        // 读取波形的寄存器结束地址
 uint16_t Code0x10WaitTime;  // 读取波形数据前功能码0x10发送后等待时间（单位：S）
 uint8_t  WaveQuantity;      // 读取波形条数
}WaveParam;

enum {
	PVWaveCurve = 0,     // PV曲线
	FaultWave,           // 故障录波
	RealTimeWave,        // 实时波形
	OneKeyDiagnosis,     // 一键诊断
	ACWave,              // AC波形
	ACHarmonic,          // AC电压谐波
	Impedance,           // 阻抗波形
	PVISOCheck,          // ISO检测  LI 2019.01.18
	TotalWaveNums
};

enum {// LI 2018.04.04
	IOT_FREE = 0,
	IOT_READY,
	IOT_ING,
	IOT_BUSY
};

#define SingleRegReadNum_1 125 // 读取波形分段读，每次获取125个寄存器

uint8_t InverterWave_Judge(uint16_t Addr_Start, uint16_t Addr_End);
void InverterWave_GetAndSend(void);

uint8_t InverterWaveDataNum_Count(uint8_t mode);
uint8_t InverterWaveStatus_Judge(uint8_t mode, uint8_t status);
uint32_t InverterWaveWaitTime_Set(uint8_t mode, uint8_t time);
uint32_t InverterWaveDataSignleTCPAckTime_Set(uint8_t mode, uint8_t time);
void InverterWaveWaitTime_Count(void);
uint8_t IOT_InverterPVWaveNums_Handle_uc(uint8_t ucTpye_tem, uint8_t ucNums_tem);
uint8_t Replay_0x14(uint16_t start_addr, uint16_t end_addr, uint8_t *msg, uint16_t msg_len);

void IOT_INVAdditionalFunc_Handle(void);


#endif /* COMPONENTS_INCLUDES_IOT_INVWAVE_H_ */
