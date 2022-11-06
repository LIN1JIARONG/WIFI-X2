/*
 * iot_led.h
 *
 *  Created on: 2021Äê11ÔÂ16ÈÕ
 *      Author: Administrator
 */

#ifndef COMPONENTS_IOT_LED_H_
#define COMPONENTS_IOT_LED_H_


#include "iot_system.h"

#define LED1_PIN (GPIO_NUM_19)    //BLE     //GREE
#define LED2_PIN (GPIO_NUM_18)    //WIFI    //REG
#define LED3_PIN (GPIO_NUM_3)     //PV      //BLUE


#define LEDGREE_PIN  (GPIO_NUM_19)    //BLE     //GREE
#define LEDREG_PIN   (GPIO_NUM_18)    //WIFI    //REG
#define LEDBLUE_PIN  (GPIO_NUM_3)     //PV      //BLUE


#define KEY_PIN (GPIO_NUM_5)


void IOT_GPIO_Init(void);

void IOT_LED1_ON(void);
void IOT_LED1_OFF(void);
void IOT_LED2_ON(void);
void IOT_LED2_OFF(void);
void IOT_LED3_ON(void);
void IOT_LED3_OFF(void);
void IOT_LED_Task(void);
void IOT_LED_Polling(void);


void Key_init(void );



#endif /* COMPONENTS_IOT_LED_H_ */
