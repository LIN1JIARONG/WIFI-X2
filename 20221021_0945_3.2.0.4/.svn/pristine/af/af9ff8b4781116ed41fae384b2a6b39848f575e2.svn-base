/*
 * iot_bmsbdc.h
 *
 *  Created on: 2022年6月27日
 *      Author: Administrator
 */

#ifndef COMPONENTS_INCLUDES_IOT_BMSBDC_H_
#define COMPONENTS_INCLUDES_IOT_BMSBDC_H_

#include <stdbool.h>
#include <stdint.h>


#pragma pack(1)
typedef struct
{
	uint8_t BDClinknum_184;		    //BDC并网机数量  184寄存器
	uint8_t BMSpacknum_185;	 		//电池模块数量  185寄存器

}BDCdate;

#pragma pack(1)
typedef struct
{
	uint8_t  BDC03GroupNum;			//03寄存器组器需要读取的数目
	uint16_t BDC03Group[10][2];		//03寄存器组
	uint8_t  BDC04GroupNum;			//04寄存器组器需要读取的数目
	uint16_t BDC04Group[10][2];		//04寄存器组

	uint8_t  BMS03GroupNum;			//03寄存器组寄存器需要读取的数目
	uint16_t BMS03Group[10][2];		//03寄存器组
	uint8_t  BMS04GroupNum;			//04寄存器组寄存器需要读取的数目
	uint16_t BMS04Group[10][2];		//04寄存器组
}BDCGroup;


extern BDCGroup bdcgroup;
extern BDCdate bdcdate;


#endif /* COMPONENTS_INCLUDES_IOT_BMSBDC_H_ */
