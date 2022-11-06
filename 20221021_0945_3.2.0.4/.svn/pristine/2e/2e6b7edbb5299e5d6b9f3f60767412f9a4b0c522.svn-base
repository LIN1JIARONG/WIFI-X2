/*
 * iot_SmartConfig.c
 *
 *  Created on: 2022年7月22日
 *      Author: grt-chenyan
 */

#include "iot_SmartConfig.h"
#include <stdlib.h>
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

#include "iot_system.h"
#include "iot_protocol.h"
#include "esp_smartconfig.h"
#include "iot_timer.h"
#include "iot_station.h"

#define SMARTCONFIG_V2_KEY   AES_KEY_128BIT



/******************************************************************************
 * FunctionName : IOT_WIFI_SmartConfig_Start
 * Description  : 开启标准配置模式
 * Parameters   : none
 * Returns      : V2 模式  需要 16位 AES密钥
*******************************************************************************/
char V2_KEY[17] = {SMARTCONFIG_V2_KEY};

const smartconfig_start_config_t cfg = {
		.enable_log = false,
		.esp_touch_v2_enable_crypt = true,
		.esp_touch_v2_key = V2_KEY,
	};
void IOT_WIFI_SmartConfig_Start(void)
{
	//smartconfig_start_config_t Smcfg = {0};

	//memcpy(V2_KEY , SMARTCONFIG_V2_KEY , strlen(SMARTCONFIG_V2_KEY));
	ESP_ERROR_CHECK( esp_smartconfig_set_type(SC_TYPE_ESPTOUCH_V2) );


//	cfg.esp_touch_v2_enable_crypt = true;
//	cfg.esp_touch_v2_key = V2_KEY;

	//printf("esp_touch_v2_key addr %x ,  %d \r\n", cfg.esp_touch_v2_key , sizeof(smartconfig_start_config_t));
	//printf("esp_touch_v2_key = %s \r\n", cfg.esp_touch_v2_key);
	//smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();

	ESP_LOGI(WiFi_SC, "smartconfig start");
	ESP_ERROR_CHECK( esp_smartconfig_start(&cfg) );

}

void IOT_WIFI_SmartConfig_Close(void)
{
	ESP_LOGI(WiFi_SC, "smartconfig stop");
	esp_smartconfig_stop();
}



/******************************************************************************
 * FunctionName : IOT_WIFI_SmartConfig_Close_Delay
 * Description  : SmartConfig 延时关闭
 * Parameters   : ucMode_tem  模式
 * 					1  设置关闭的时间			单位 s(秒)
 * 					2  检测是否可以关闭 SmartConfig
 * Returns      : none
*******************************************************************************/
void IOT_WIFI_SmartConfig_Close_Delay(uint8_t ucMode_tem , uint32_t uiDelay_Time_s_tem)
{
	static long long   llTime = 0;
	static long long   uiDelayTime = 0;

	static uint8_t ucSmartConfingDelayCloseFlag =0;

	if(1 == ucMode_tem)
	{
		llTime = IOT_GetTick();
		uiDelayTime = uiDelay_Time_s_tem * 1000;

		ucSmartConfingDelayCloseFlag = 1;
	}
	else if(1 == ucSmartConfingDelayCloseFlag)
	{
		if((IOT_GetTick() - llTime) > uiDelayTime)
		{
			ucSmartConfingDelayCloseFlag = 0;

			if(1 == System_state.SmartConfig_OnOff_Flag)
			{
				IOT_WIFIEven_CloseSmartConfig();
			}

		}
	}

}

