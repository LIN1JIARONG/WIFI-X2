/*
 * iot_bmsbdc.c
 *
 *  Created on: 2022��6��27��
 *      Author: Administrator
 */
#include "iot_inverter.h"
#include "iot_universal.h"
#include "iot_uart1.h"
#include "iot_system.h"
#include "iot_protocol.h"
#include "iot_rtc.h"
#include "iot_bmsbdc.h"

BDCdate bdcdate;

//���Ĵ�����
BDCGroup bdcgroup=
{
  1,
  {{5000, 5040}, {0, 0}, {0, 0}, {0, 0}, {0, 0},},

  1,
  {{0, 99}, {0, 0}, {0, 0}, {0, 0}, {0, 0},},
};

















