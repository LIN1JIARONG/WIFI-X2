/*
 * iot_gatts_table.h
 *
 *  Created on: 2022Äê1ÔÂ25ÈÕ
 *      Author: Administrator
 */

#ifndef COMPONENTS_INCLUDES_IOT_GATTS_TABLE_H_
#define COMPONENTS_INCLUDES_IOT_GATTS_TABLE_H_

/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BLE_Readmode	0


/* Attributes State Machine */
enum
{
    IDX_SVC,
    IDX_CHAR_A,
    IDX_CHAR_VAL_A,
    IDX_CHAR_CFG_A,

    IDX_CHAR_B,
    IDX_CHAR_VAL_B,

    IDX_CHAR_C,
    IDX_CHAR_VAL_C,

    HRS_IDX_NB,
};



void IOT_GattsTable_Init(void);
void IOT_GattsTable_task(void *arg);

void IOT_GattsEvenSetR(void);

void IOT_GattsEvenONBLE(void);
void IOT_GattsEvenOFFBLE(void);
void IOT_GattsEvenGATTS_WRITE(void);


#endif /* COMPONENTS_INCLUDES_IOT_GATTS_TABLE_H_ */
