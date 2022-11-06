/*
 * iot_SmartConfig.h
 *
 *  Created on: 2022Äê7ÔÂ22ÈÕ
 *      Author: grt-chenyan
 */


#include <stdint.h>

#ifndef COMPONENTS_INCLUDES_IOT_SMARTCONFIG_H_
#define COMPONENTS_INCLUDES_IOT_SMARTCONFIG_H_


#define WiFi_SC  "WiFi_SmartConfig"

void IOT_WIFI_SmartConfig_Start(void);
void IOT_WIFI_SmartConfig_Close(void);

void IOT_WIFI_SmartConfig_Close_Delay(uint8_t ucMode_tem , uint32_t uiDelay_Time_s_tem);
#endif /* COMPONENTS_INCLUDES_IOT_SMARTCONFIG_H_ */
