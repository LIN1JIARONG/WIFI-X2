/*
 * iot_scan.c
 *
 *  Created on: 2021年11月19日
 *      Author: Administrator
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"

#include "iot_system.h"
#include "iot_station.h"
#include "iot_station.h"

#include <string.h>
#include <stdlib.h>

#define DEFAULT_SCAN_LIST_SIZE   20

static const char *scan = "scan";
// wifi 扫描

static void print_auth_mode(int authmode)
{
    switch (authmode) {
    case WIFI_AUTH_OPEN:
        ESP_LOGI(scan, "Authmode \tWIFI_AUTH_OPEN");
        break;
    case WIFI_AUTH_WEP:
        ESP_LOGI(scan, "Authmode \tWIFI_AUTH_WEP");
        break;
    case WIFI_AUTH_WPA_PSK:
        ESP_LOGI(scan, "Authmode \tWIFI_AUTH_WPA_PSK");
        break;
    case WIFI_AUTH_WPA2_PSK:
        ESP_LOGI(scan, "Authmode \tWIFI_AUTH_WPA2_PSK");
        break;
    case WIFI_AUTH_WPA_WPA2_PSK:
        ESP_LOGI(scan, "Authmode \tWIFI_AUTH_WPA_WPA2_PSK");
        break;
    case WIFI_AUTH_WPA2_ENTERPRISE:
        ESP_LOGI(scan, "Authmode \tWIFI_AUTH_WPA2_ENTERPRISE");
        break;
    case WIFI_AUTH_WPA3_PSK:
        ESP_LOGI(scan, "Authmode \tWIFI_AUTH_WPA3_PSK");
        break;
    case WIFI_AUTH_WPA2_WPA3_PSK:
        ESP_LOGI(scan, "Authmode \tWIFI_AUTH_WPA2_WPA3_PSK");
        break;
    default:
        ESP_LOGI(scan, "Authmode \tWIFI_AUTH_UNKNOWN");
        break;
    }
}

static void print_cipher_type(int pairwise_cipher, int group_cipher)
{
    switch (pairwise_cipher) {
    case WIFI_CIPHER_TYPE_NONE:
        ESP_LOGI(scan, "Pairwise Cipher \tWIFI_CIPHER_TYPE_NONE");
        break;
    case WIFI_CIPHER_TYPE_WEP40:
        ESP_LOGI(scan, "Pairwise Cipher \tWIFI_CIPHER_TYPE_WEP40");
        break;
    case WIFI_CIPHER_TYPE_WEP104:
        ESP_LOGI(scan, "Pairwise Cipher \tWIFI_CIPHER_TYPE_WEP104");
        break;
    case WIFI_CIPHER_TYPE_TKIP:
        ESP_LOGI(scan, "Pairwise Cipher \tWIFI_CIPHER_TYPE_TKIP");
        break;
    case WIFI_CIPHER_TYPE_CCMP:
        ESP_LOGI(scan, "Pairwise Cipher \tWIFI_CIPHER_TYPE_CCMP");
        break;
    case WIFI_CIPHER_TYPE_TKIP_CCMP:
        ESP_LOGI(scan, "Pairwise Cipher \tWIFI_CIPHER_TYPE_TKIP_CCMP");
        break;
    default:
        ESP_LOGI(scan, "Pairwise Cipher \tWIFI_CIPHER_TYPE_UNKNOWN");
        break;
    }

    switch (group_cipher) {
    case WIFI_CIPHER_TYPE_NONE:
        ESP_LOGI(scan, "Group Cipher \tWIFI_CIPHER_TYPE_NONE");
        break;
    case WIFI_CIPHER_TYPE_WEP40:
        ESP_LOGI(scan, "Group Cipher \tWIFI_CIPHER_TYPE_WEP40");
        break;
    case WIFI_CIPHER_TYPE_WEP104:
        ESP_LOGI(scan, "Group Cipher \tWIFI_CIPHER_TYPE_WEP104");
        break;
    case WIFI_CIPHER_TYPE_TKIP:
        ESP_LOGI(scan, "Group Cipher \tWIFI_CIPHER_TYPE_TKIP");
        break;
    case WIFI_CIPHER_TYPE_CCMP:
        ESP_LOGI(scan, "Group Cipher \tWIFI_CIPHER_TYPE_CCMP");
        break;
    case WIFI_CIPHER_TYPE_TKIP_CCMP:
        ESP_LOGI(scan, "Group Cipher \tWIFI_CIPHER_TYPE_TKIP_CCMP");
        break;
    default:
        ESP_LOGI(scan, "Group Cipher \tWIFI_CIPHER_TYPE_UNKNOWN");
        break;
    }
}



/* Initialize Wi-Fi as sta and set scan method */
uint8_t wifi_scan(void)
{
//  ESP_ERROR_CHECK(esp_netif_init());  //初始化底层TCP/IP堆栈
//  ESP_ERROR_CHECK(esp_event_loop_create_default()); //创建默认事件循环。
//  esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();//使用默认WiFi Station配置创建esp_netif对象，将netif连接到WiFi并注册默认WiFi处理程序。
//  assert(sta_netif);
//  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
//  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    uint16_t number = DEFAULT_SCAN_LIST_SIZE;
    wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
    uint16_t ap_count = 0;
    memset(ap_info, 0, sizeof(ap_info));
    uint16_t uSSIDLen=0;
    uint16_t udatasumlen=0;

    esp_err_t ScanErr = 0;

    ScanErr = esp_wifi_scan_start(NULL, true);   //阻塞扫描  最长时间是1500ms  超时可能断开AP！！！

    if(ScanErr == ESP_ERR_WIFI_STATE)	//wifi正在连接
    {
    	return 2;
    }

    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
    ESP_LOGI(scan, "Total APs scanned = %u", ap_count);
    udatasumlen=0;

	memset(System_state.Get_TabDataBUF,0,300);

	if(ap_count > DEFAULT_SCAN_LIST_SIZE)
	{
		System_state.Get_TabDataBUF[udatasumlen++] = DEFAULT_SCAN_LIST_SIZE;
	}
	else
	{
		System_state.Get_TabDataBUF[udatasumlen++] = ap_count;	// 0  总WiFi数量
	}
    for (int i = 0; (i < DEFAULT_SCAN_LIST_SIZE) && (i < ap_count); i++)
    {
        ESP_LOGI(scan, "SSID \t\t%s", ap_info[i].ssid);
        ESP_LOGI(scan, "RSSI \t\t%d", ap_info[i].rssi);
        print_auth_mode(ap_info[i].authmode);
        if (ap_info[i].authmode != WIFI_AUTH_WEP) {
            print_cipher_type(ap_info[i].pairwise_cipher, ap_info[i].group_cipher);
        }
        ESP_LOGI(scan, "Channel \t\t%d\n", ap_info[i].primary);

        uSSIDLen = strlen(&ap_info[i].ssid);
        ESP_LOGI(scan, "uSSIDLen \t\t%d\n",uSSIDLen);  				// WIFI名称长度
        System_state.Get_TabDataBUF[udatasumlen++] = uSSIDLen;	    // 1
        if(uSSIDLen + udatasumlen >= 299)
        {
        	if(i>1)
     		System_state.Get_TabDataBUF[0]= i-1;
    		break;
        }
        else
        {
        	memcpy(&System_state.Get_TabDataBUF[udatasumlen] , ap_info[i].ssid , uSSIDLen ); //2+7 SSID 名称

        }
    	for(int j=0 ;j<uSSIDLen;j++ )
    	{
    		IOT_Printf("%02x ",ap_info[i].ssid[j]);
    	}
    	IOT_Printf("\r\n *********************ap_info[i].ssid**********************  \r\n" );

    	udatasumlen = udatasumlen + uSSIDLen;  								// 9
    	System_state.Get_TabDataBUF[udatasumlen++] = ap_info[i].rssi ;	// 信号
    	ESP_LOGI(scan, "udatasumlen =%d   System_state.rssi=%d  \r\n",udatasumlen,System_state.Get_TabDataBUF[udatasumlen]);

    	if( udatasumlen >= 280 )  // 数据太长
    	{

    		System_state.Get_TabDataBUF[0]= i;

    		ESP_LOGI(scan, "	if( udatasumlen >= 200 )   break System_state.Get_TabDataBUF[0]=%d \r\n",System_state.Get_TabDataBUF[0] );

    		break;
    	}

    }

    esp_wifi_scan_stop();

//	printf("\r\n *********************************** ********  \r\n" );
//for(int i=0 ;i<=udatasumlen;i++ )
//{
//	 printf("%02x ",System_state.Get_TabDataBUF[i]);
//}
    for (int i = 0; (i < DEFAULT_SCAN_LIST_SIZE) && (i < ap_count); i++)
    {
    	if(strncmp(&ap_info[i].ssid,&SysTAB_Parameter[Parameter_len[REG_WIFISSID]],Collector_Config_LenList[REG_WIFISSID] )==0 )
    	{
          ESP_LOGI(scan, "ap_info[i].ssid is ready =(  %s  ) \n",ap_info[i].ssid  );
       // IOT_EvenSrtCONNECTED();  //  发现设备 > 通知连接服务器
          return 1;
    	}
    }
    return 0;
}







