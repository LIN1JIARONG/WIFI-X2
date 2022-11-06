/*
 * iot_InvWave.h
 *
 *  Created on: 2022��6��11��
 *      Author: Administrator
 */

#ifndef COMPONENTS_INCLUDES_IOT_INVWAVE_H_
#define COMPONENTS_INCLUDES_IOT_INVWAVE_H_

#include <stdbool.h>
#include <stdint.h>

typedef struct _WaveParam{
 uint16_t WaveType;          // �������� ��PV���ߡ�����¼����ʵʱ���Ρ�һ����ϡ�AC��ѹг�����迹���Σ�
 uint16_t RegAddrStart;      // ��ȡ���εļĴ�����ʼ��ַ
 uint16_t RegAddrEnd;        // ��ȡ���εļĴ���������ַ
 uint16_t Code0x10WaitTime;  // ��ȡ��������ǰ������0x10���ͺ�ȴ�ʱ�䣨��λ��S��
 uint8_t  WaveQuantity;      // ��ȡ��������
}WaveParam;

enum {
	PVWaveCurve = 0,     // PV����
	FaultWave,           // ����¼��
	RealTimeWave,        // ʵʱ����
	OneKeyDiagnosis,     // һ�����
	ACWave,              // AC����
	ACHarmonic,          // AC��ѹг��
	Impedance,           // �迹����
	PVISOCheck,          // ISO���  LI 2019.01.18
	TotalWaveNums
};

enum {// LI 2018.04.04
	IOT_FREE = 0,
	IOT_READY,
	IOT_ING,
	IOT_BUSY
};

#define SingleRegReadNum_1 125 // ��ȡ���ηֶζ���ÿ�λ�ȡ125���Ĵ���

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
