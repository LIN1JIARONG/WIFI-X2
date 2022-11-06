/*
 * iot_station.c
 *
 *  Created on: 2021年11月22日
 *      Author: Administrator
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "iot_system.h"

#include "iot_scan.h"
#include "iot_station.h"
#include "iot_inverter.h"
#include "iot_gatts_table.h"
#include "iot_system.h"
#include "iot_protocol.h"
#include "esp_smartconfig.h"


#include <stdlib.h>
#include "nvs_flash.h"
#include "esp_netif.h"

#include "iot_SmartConfig.h"

/* The examples use WiFi configuration that you can set via project configuration menu
   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/

#define SMARTCONFIG_EN      1     //标准模式使能

#define EXAMPLE_ESP_WIFI_SSID      "zktest1"            //账户myssid
#define EXAMPLE_ESP_WIFI_PASS      "zktestzktest"		//密码mypassword
#define EXAMPLE_ESP_MAXIMUM_RETRY  10					//重连最大次数

#define WIFI_CONNECT_TIMEOUT     15    //连接超时时间   15s

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT 	BIT0   	//-连接到AP到IP
#define WIFI_FAIL_BIT      	BIT1   	//-连接失败后的最大重试次数
#define WIFI_SCAN_BIT      	BIT2   	//-wifi扫描
#define WIFI_CONNECT_BIT	BIT3 	//-wifi连接
#define WIFI_OTA_BIT		BIT4  	//-OAT 升级
#define WIFI_ON_BIT			BIT5  	//-
#define WIFI_OFF_BIT		BIT6  	//-
#define ESPTOUCH_START_BIT  BIT7
#define ESPTOUCH_DONE_BIT   BIT8
#define WIFI_DISCONNECT_BIT BIT9
#define WIFI_RECONNECT_BIT  BIT10


static bool bSmartConfigRestartFlag = false;
static bool bSmartConfigRunningFlag = false;

static bool bWIFIReConnectFlag = false;

volatile uint32_t ScanIntervalTime  = 60;		//WIFI 扫描时间间隔


char chostname[40] = {0};
static const char *WiFi = "WiFi";
static int s_retry_num = 0;

esp_netif_t *esp_netif = NULL;

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;   		//当我们连接时，发送FreeRTOS事件组信号

//注册事件回调函数
static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
	wifi_config_t  SWiFiConfig_Get;
	if(event_base == WIFI_EVENT)
		ESP_LOGI(WiFi, "event_handler wifi event_id=%d \r\n", event_id);
	if(event_base == IP_EVENT)
		ESP_LOGI(WiFi, "event_handler ip event_id=%d \r\n", event_id);
	//station启动时，esp_wifi_connect连接
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
#if SMARTCONFIG_EN		//标准配网
    	if(1 == System_state.SmartConfig_OnOff_Flag)
    	{
    		IOT_WIFIEven_StartSmartConfig();
    	}else
#endif
    	{
    		 xEventGroupSetBits(s_wifi_event_group, WIFI_RECONNECT_BIT);   //设置指定的事件标志位为1。
    	}
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
    	//错误的失败代码返回
    	//断开重连机制注意：
    	//1、若是主动断开,不应该调用 esp_wifi_connect
    	//2、当连接一个不存在的AP 会一直在连接状态，无法执行 AP扫描
    	wifi_event_sta_disconnected_t* wifi_disconnect_event = (wifi_event_sta_disconnected_t*) event_data;
    	ESP_LOGI(WiFi, "event_handler wifi_disconnect_event reason=%d \r\n", wifi_disconnect_event->reason);
		esp_wifi_get_config(ESP_IF_WIFI_STA, &SWiFiConfig_Get);
		ESP_LOGI(WiFi, "IOT_WiFi_Connect get2 ssid:%s password:%s\r\n",SWiFiConfig_Get.sta.ssid, SWiFiConfig_Get.sta.password);
    	//从AP失去连接时，，连接次数小于最大连接次数时，
    	System_state.Wifi_state = Offline;   		 // WIFI 掉线

       if(System_state.Wifi_ActiveReset & WIFI_DISCONNET)
       { // 配置保存密码重连 wifi 有手动调用esp_wifi_disconnect关闭事件
    	   System_state.Wifi_ActiveReset&= ~WIFI_DISCONNET;
    	   System_state.Wifi_ActiveReset|= WIFI_isDISCONNET ;
    	   ESP_LOGI(WiFi, "****event_handler  Wifi_ActiveReset ==WIFI_DISCONNET******");
        // xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);   //设置指定的事件标志位为1。
       }// 关闭连接的优先级大于 连接 的
       else if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY)
       {
    	   xEventGroupSetBits(s_wifi_event_group, WIFI_RECONNECT_BIT);   //设置指定的事件标志位为1。
        //	ESP_ERROR_CHECK(err);  有可能重发connect 失败异常
            s_retry_num++;
            ESP_LOGI(WiFi, "retry to connect to the AP num %d \r\n",s_retry_num);
        }
        else
        {
        	s_retry_num = 0;
           xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);   //设置指定的事件标志位为1。
        }
        ESP_LOGI(WiFi,"connect to the AP fail");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
    	//从连接的AP获得IP时，打印IP
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(WiFi, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);   //设置指定的事件标志位为1。
        // 若已经连接上获取过IP ，当IP 改变时，关闭所有已经创建的套接字并进行 重新创建
    }
    else if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_BEACON_TIMEOUT)
    {
    	// ESP32 station beacon timeout  信号检测超时事件

    	ScanIntervalTime = 20;    //扫描时间间隔 改成 20 s
    	IOT_WiFiScan_uc(ESP_WRITE , ScanIntervalTime); //20s 后开始扫描

        System_state.Wifi_state = Offline;   // WIFI 连接在线

    }

#if SMARTCONFIG_EN		//标准配网

	else if (event_base == SC_EVENT && event_id == SC_EVENT_SCAN_DONE)
	{
	   ESP_LOGI(WiFi_SC, "Scan done");
	}
	else if (event_base == SC_EVENT && event_id == SC_EVENT_FOUND_CHANNEL)
	{
	   ESP_LOGI(WiFi_SC, "Found channel");
	}
	else if (event_base == SC_EVENT && event_id == SC_EVENT_GOT_SSID_PSWD)
	{
	   ESP_LOGI(WiFi_SC, "Got SSID and password");

	   smartconfig_event_got_ssid_pswd_t *evt = (smartconfig_event_got_ssid_pswd_t *)event_data;

	   if(evt == NULL) return;

	   uint8_t ssid[33] = { 0 };
	   uint8_t password[65] = { 0 };
	   uint8_t rvd_data[33] = { 0 };
	   uint8_t SNbuf[31] = {0};
	   memcpy(SNbuf,&SysTAB_Parameter[Parameter_len[REG_SN]],MIN(Collector_Config_LenList[REG_SN] , sizeof(SNbuf)) );

	   memcpy(ssid, evt->ssid, sizeof(evt->ssid));
	   memcpy(password, evt->password, sizeof(evt->password));
	   ESP_LOGI(WiFi_SC, "SSID:%s", ssid);
	   ESP_LOGI(WiFi_SC, "PASSWORD:%s", password);
	   ESP_LOGI(WiFi_SC, "evt->type: %d",evt->type);
	  // if (evt->type == SC_TYPE_ESPTOUCH_V2)	//存在结构体不对齐 元素地址偏移问题
	  // {
		   ESP_ERROR_CHECK( esp_smartconfig_get_rvd_data(rvd_data, sizeof(rvd_data)) );
		   ESP_LOGI(WiFi_SC, "RVD_DATA:");
		   for (int i=0; i<33; i++)
		   {
			   IOT_Printf("%02x ", rvd_data[i]);
		   }
		   IOT_Printf("\n");


		   if(strcmp((const char *)SNbuf ,(const char *)rvd_data) == 0)
		   {
			   ESP_LOGI(WiFi_SC, "SNbuf strcmp successed");

			   IOT_SystemParameterSet(56,(char *)ssid , strlen((char *)ssid));		//保存wifi名字
			   IOT_SystemParameterSet(57,(char *)password , strlen((char *)password));		//保存wifi密码

			   IOT_WIFIOnlineFlag(255);				//配网标志 , 正在连接中
			   IOT_WIFIEvenSetCONNECTED();			//通知连接WIFI

		   }else
		   {
			   IOT_SmartConfig_Restart();   //重启标准配网
		   }
	  // }

		   IOT_WIFI_SmartConfig_Close_Delay(1 , SMARTCONFIG_CLOSE_DELAY_TIME);   //标准配网获取到密码 , 30s后自动关闭
	}
	else if (event_base == SC_EVENT && event_id == SC_EVENT_SEND_ACK_DONE)
	{
		IOT_WIFIEven_CloseSmartConfig();
	}

#endif

}
/******************************************************************************
 * FunctionName : IOT_SmartConfig_Restart
 * Description  : 设置标准配网功能重启
 * Parameters   : void
 * Returns      : 标准配网收到WIFI账号密码,但序列号不匹配时,需要重启SMConfig
*******************************************************************************/
void IOT_SmartConfig_Restart(void)
{
	ESP_LOGI(WiFi_SC, "SmartConfig_Restar");
	bSmartConfigRestartFlag = true;
	IOT_WIFIEven_CloseSmartConfig();			//关闭标准配网模式
}


/******************************************************************************
 * FunctionName : IOT_IP4_Addr_CtoI
 * Description  : 将IP 地址 由字符串转换为整型
 * Parameters   : char *IP4AddrStr    ip地址字符串
 * 				  void * OutputBuff	  输出的整型buff
 * Returns      : 成功转换的个数
 * Notice		  例如	    192.168.1.1
 *                转换后结果  OutputBuff[0] == 192
 *                			 OutputBuff[1]  == 168
 *                			 OutputBuff[2]  == 1
 *                			 OutputBuff[3]  == 1
*******************************************************************************/
int IOT_IP4_Addr_CtoI(char *IP4AddrStr , void * OutputBuff)
{
	#define IP4_STR_FORMAT  "%d.%d.%d.%d"

	if((NULL == IP4AddrStr) || (NULL == OutputBuff))
	{
		return 0;
	}

	int *pIntBuff =(int *)OutputBuff;

	return sscanf((const char *)IP4AddrStr , IP4_STR_FORMAT , &pIntBuff[0] , &pIntBuff[1] , &pIntBuff[2] , &pIntBuff[3]);
}


/******************************************************************************
 * FunctionName : IOT_DHCP_Client_Stop_Config
 * Description  : 关闭 DHCP设备 功能 , 设置静态IP地址
 * Parameters   : void
 * Returns      : NONE
*******************************************************************************/
void IOT_DHCP_Client_Stop_Config(void)
{
	char sIPAddrBuff [30]  = {0};
	char sGatewayBuff[30]  = {0};
	char sNetmaskBuff[30]  = {0};

	int IPAddrIntBuff [4]  = {0};
	int GatewayIntBuff[4]  = {0};
	int NetmaskINTBuff[4]  = {0};

	memcpy(sIPAddrBuff  , &SysTAB_Parameter[Parameter_len[REG_ClientIP]] , MIN(sizeof(sIPAddrBuff) , Collector_Config_LenList[REG_ClientIP]));		//IP地址
	memcpy(sGatewayBuff , &SysTAB_Parameter[Parameter_len[REG_DG]]       , MIN(sizeof(sGatewayBuff), Collector_Config_LenList[REG_DG]) );		//网关
	memcpy(sNetmaskBuff , &SysTAB_Parameter[Parameter_len[REG_MASK]]     , MIN(sizeof(sNetmaskBuff), Collector_Config_LenList[REG_MASK] ));   //子网掩码


	IOT_IP4_Addr_CtoI(sIPAddrBuff   , IPAddrIntBuff);
	IOT_IP4_Addr_CtoI(sGatewayBuff  , GatewayIntBuff);
	IOT_IP4_Addr_CtoI(sNetmaskBuff  , NetmaskINTBuff);

#if 1
	IOT_Printf("IP addr : \"%d.%d.%d.%d\"\r\n" , IPAddrIntBuff[0]   , IPAddrIntBuff[1]   , IPAddrIntBuff[2]   , IPAddrIntBuff[3]);
	IOT_Printf("GW addr : \"%d.%d.%d.%d\"\r\n" , GatewayIntBuff[0]  , GatewayIntBuff[1]  , GatewayIntBuff[2]  , GatewayIntBuff[3]);
	IOT_Printf("NM addr : \"%d.%d.%d.%d\"\r\n" , NetmaskINTBuff[0]  , NetmaskINTBuff[1]  , NetmaskINTBuff[2]  , NetmaskINTBuff[3]);
#endif


	//    /* 以太网，静态ip连接配置 ip 网络 掩码 dns服务器*/
	esp_netif_dhcpc_stop(esp_netif); // add isen
	esp_netif_ip_info_t netinfo = {
					.ip.addr = esp_netif_ip4_makeu32     (IPAddrIntBuff[3]   , IPAddrIntBuff[2]   , IPAddrIntBuff[1]   , IPAddrIntBuff[0]),
					.gw.addr = esp_netif_ip4_makeu32     (GatewayIntBuff[3]  , GatewayIntBuff[2]  , GatewayIntBuff[1]  , GatewayIntBuff[0]),
					.netmask.addr = esp_netif_ip4_makeu32(NetmaskINTBuff[3]  , NetmaskINTBuff[2]  , NetmaskINTBuff[1]  , NetmaskINTBuff[0])
	};
	esp_netif_set_ip_info(esp_netif, &netinfo);// add isen

//	esp_netif_dns_info_t dnsip = {0};
//	dnsip.ip.type = ESP_IPADDR_TYPE_V4;
//	dnsip.ip.u_addr.ip4.addr = ESP_IP4TOADDR(0 , 255 , 255 , 255);
//	esp_netif_set_dns_info(esp_netif, ESP_NETIF_DNS_MAIN, &dnsip);// add isen
}

/******************************************************************************
 * FunctionName : IOT_DHCP_Client_Start
 * Description  : 开启DTCP功能
 * Parameters   : void
 * Returns      : NONE
*******************************************************************************/
void IOT_DHCP_Client_Start(void)
{
	esp_netif_dhcp_status_t status = {0};
	ESP_ERROR_CHECK( esp_netif_dhcpc_get_status(esp_netif, &status));

	if(ESP_NETIF_DHCP_STARTED != status)	//判断DHCP 是否已经关闭
	   ESP_ERROR_CHECK( esp_netif_dhcpc_start(esp_netif));
}
/******************************************************************************
 * FunctionName : wifi_init_sta
 * Description  : WIFI station 模式初始化
 * Parameters   : void
 * Returns      : NONE
*******************************************************************************/
void wifi_init_sta(void)
{
	char cDHCPOnoffFlag = 0;
	char SNbuf[30]={0};
	char *gethostname[40] ={0};
	uint8_t SmartConfigFlag = 0;

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif = esp_netif_create_default_wifi_sta();



    memcpy(&cDHCPOnoffFlag  , &SysTAB_Parameter[Parameter_len[REG_WIFIDHCP]]  ,  MIN(sizeof(cDHCPOnoffFlag),Collector_Config_LenList[REG_WIFIDHCP]));		//IP地址
    if('1' == cDHCPOnoffFlag)	//判断是否关闭DHCP
    {
    	ESP_LOGI(WiFi, "DHCPC  CLOSE !!");
    	IOT_DHCP_Client_Stop_Config();
    }else
    {
    	IOT_DHCP_Client_Start();
    	ESP_LOGI(WiFi, "DHCPC  START !!");
    }

    memcpy(SNbuf , &SysTAB_Parameter[Parameter_len[REG_SN]],MIN(sizeof(SNbuf), Collector_Config_LenList[REG_SN]) );

    memset(chostname , 0x00 , sizeof(chostname));
   	sprintf(chostname , "GRT-%s",SNbuf);

   	ESP_LOGI(WiFi, "chostname = %s",chostname);
   	ESP_ERROR_CHECK(esp_netif_set_hostname(esp_netif, (const char *)chostname));	//20220711 修改采集器在路由器中显示的名字
   	esp_netif_get_hostname(esp_netif, (const char **)gethostname);
   	ESP_LOGI(WiFi, "gethostname = %s",gethostname[0]);


    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));





    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

#if SMARTCONFIG_EN
    ESP_ERROR_CHECK( esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL) ); //20220721 add chen smartConfig 标准配网
#endif

	char wifi_ssid[50]={0};					// flash存储的wifi账户
	char wifi_password[50]={0};				// flash存储的wifi密码
	wifi_config_t wifi_config = {0};

#if SMARTCONFIG_EN

	SmartConfigFlag = 1;	 //默认开启标准配网

	ESP_ERROR_CHECK(esp_wifi_get_config(WIFI_IF_STA, &wifi_config));

	for(int i = 0; i < sizeof(wifi_config.sta.ssid);i++)
	{
		if(wifi_config.sta.ssid[i] != 0x00)
		{
			IOT_Printf("wifi_config.sta.ssid = %s \r\n",wifi_config.sta.ssid);
			SmartConfigFlag = 0; // 检测到WIFI已经被配置过,不开启标准配网
			ESP_LOGI(WiFi, "wifi has been config , SmartConfig  not open!!");
			break;
		}
	}

//	if(0 == SmartConfigFlag)	//需要再次判断参数表中密码是否已经恢复出厂设置 , 有时 reset wifi config 不成功
//	{
//		IOT_Printf(" Collector_Config_LenList[REG_WIFIPASSWORD] = %d \r\n",Collector_Config_LenList[REG_WIFIPASSWORD]);
//		IOT_Printf(" SysTAB_Parameter[Parameter_len[REG_WIFIPASSWORD]] = %x \r\n",SysTAB_Parameter[Parameter_len[REG_WIFIPASSWORD]]);
//
//		if((1 == Collector_Config_LenList[REG_WIFIPASSWORD] ) && ('0' == SysTAB_Parameter[Parameter_len[REG_WIFIPASSWORD]]))
//		{
//			ESP_ERROR_CHECK( esp_wifi_restore());	//清除已配置的wifi参数
//			ESP_LOGI(WiFi, "SmartConfig  open!!");
//			SmartConfigFlag = 1;  //开启 标准配网
//		}
//	}

	if((1 == Collector_Config_LenList[REG_WIFIPASSWORD] ) && ('0' == SysTAB_Parameter[Parameter_len[REG_WIFIPASSWORD]])
			&& (strncmp((const char *)&SysTAB_Parameter[Parameter_len[REG_WIFISSID]]  , "12345678" , 8) == 0))
	{
		ESP_ERROR_CHECK( esp_wifi_restore());	//清除已配置的wifi参数
		ESP_LOGI(WiFi, "SmartConfig  open!!");
		SmartConfigFlag = 1;  //开启 标准配网
	}else
	{
		SmartConfigFlag = 0;  //关闭 标准配网
	}



#endif
	if(0 == SmartConfigFlag)	//配置WIFI参数
	{
		memset(&wifi_config,0,sizeof(wifi_config));
		bzero(wifi_ssid, sizeof(wifi_ssid));
		bzero(wifi_password, sizeof(wifi_password));

		memcpy(wifi_ssid ,&SysTAB_Parameter[Parameter_len[REG_WIFISSID]],Collector_Config_LenList[REG_WIFISSID] );
		memcpy(wifi_password ,&SysTAB_Parameter[Parameter_len[REG_WIFIPASSWORD]],Collector_Config_LenList[REG_WIFIPASSWORD] );

		memcpy(wifi_config.sta.ssid, wifi_ssid, strlen( wifi_ssid));
		memcpy(wifi_config.sta.password, wifi_password, strlen( wifi_password));

		wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
		wifi_config.sta.pmf_cfg.capable = true;
		wifi_config.sta.pmf_cfg.required = false;

		//设置 ESP32 STA 或 AP 的配置。本程序设置为STA，并包含WiFi SSID和密码等信息
		ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
	}

    System_state.SmartConfig_OnOff_Flag = SmartConfigFlag;

    //设置WiFi工作模式为station、soft-AP或station+soft-AP，默认模式为soft-AP模式。本程序设置为station
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    //esp_wifi_start，根据配置，启动WiFi
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(WiFi, "wifi_init_sta finished");

    ESP_ERROR_CHECK( esp_wifi_set_inactive_time(WIFI_IF_STA, 6));  // 设置bcn  超时时间 为 6 s

}



/******************************************************************************
 * FunctionName : IOT_WiFi_Connect
 * Description  : 连接WiFi路由器
 * Parameters   : ap_ssid: wifi name   ap_password: wifi password
 * Returns      : none
*******************************************************************************/
void IOT_WiFi_Connect( void )
{
	char cDHCPOnoffFlag = 0;
	wifi_config_t SWiFiConfig ;

	memcpy(&cDHCPOnoffFlag  , &SysTAB_Parameter[Parameter_len[71]]  ,  MIN(sizeof(cDHCPOnoffFlag),Collector_Config_LenList[71]));		//IP地址
	if('1' == cDHCPOnoffFlag)	//判断是否关闭DHCP
	{
		ESP_LOGI(WiFi, "IOT_WiFi_Connect DHCPC  CLOSE !!");
		IOT_DHCP_Client_Stop_Config();
	}else
	{
		IOT_DHCP_Client_Start();
		ESP_LOGI(WiFi, "IOT_WiFi_Connect DHCPC  START !!");
	}

	memset(&SWiFiConfig,0,sizeof(SWiFiConfig));

	char wifi_ssid[50]={0};			// flash存储的wifi账户
	char wifi_password[50]={0};		// flash存储的wifi密码

	bzero(wifi_ssid, sizeof(wifi_ssid));
	bzero(wifi_password, sizeof(wifi_password));

	memcpy(wifi_ssid ,&SysTAB_Parameter[Parameter_len[REG_WIFISSID]],Collector_Config_LenList[REG_WIFISSID] );
	memcpy(wifi_password ,&SysTAB_Parameter[Parameter_len[REG_WIFIPASSWORD]],Collector_Config_LenList[REG_WIFIPASSWORD] );

	ESP_LOGI(WiFi, " **********wifi_ssid**%s\r\n",wifi_ssid);
	ESP_LOGI(WiFi, " **********wifi_password**%s\r\n",wifi_password);


	ESP_LOGI(WiFi, "************IOT_WiFi_Connect esp_wifi_disconnecting   delay........ !\r\n");

	memcpy(SWiFiConfig.sta.ssid, wifi_ssid, strlen( wifi_ssid));
	memcpy(SWiFiConfig.sta.password, wifi_password, strlen( wifi_password));
	SWiFiConfig.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
	SWiFiConfig.sta.pmf_cfg.capable = true;
	SWiFiConfig.sta.pmf_cfg.required = false;
	ESP_LOGI(WiFi, "wifi_set_config ssid:%s password:%s  \r\n",SWiFiConfig.sta.ssid, SWiFiConfig.sta.password);
//	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &SWiFiConfig));
	System_state.Wifi_ActiveReset |= WIFI_CONNETING;
    esp_wifi_connect();

}

void IOT_WiFistationInit(void)
{

	wifi_init_sta();  // 默认 sta 开始在事件里触发连接
	System_state.wifi_onoff=1;
//  esp_wifi_set_ps(WIFI_PS_NONE);   /测试天线关闭休眠
//	IOT_WiFi_Connect();
//	IOT_WiFi_Connect(wifi_ssid,wifi_password);
//	IOT_EvenSetSCAN();

}

void IOT_GETMAC(void)
{
	uint8_t ble_mac[6]={0};
	uint8_t read_mac[6]={0};
	char new_wifimac[20]={0};
	char flash_wifmac[20]={0};

	bzero(new_wifimac, sizeof(new_wifimac));
	bzero(flash_wifmac, sizeof(flash_wifmac));

	//esp_wifi_get_mac(WIFI_IF_STA,read_mac);
	esp_read_mac(read_mac, ESP_MAC_WIFI_STA);
	ESP_LOGI(WiFi, " **********read WIFI_mac = (%02x:%02x:%02x:%02x:%02x:%02x) \r\n",read_mac[0],read_mac[1],read_mac[2],read_mac[3],read_mac[4],read_mac[5]);
	memcpy(flash_wifmac,&SysTAB_Parameter[Parameter_len[REG_WIFIMAC]],Collector_Config_LenList[REG_WIFIMAC] );
	sprintf(new_wifimac,"%02x:%02x:%02x:%02x:%02x:%02x" ,read_mac[0],read_mac[1],read_mac[2],read_mac[3],read_mac[4],read_mac[5]);

	//esp_bt_get_mac();esp_read_mac(ble_mac, ESP_MAC_WIFI_STA);
	esp_read_mac(ble_mac, ESP_MAC_BT);
	ESP_LOGI(WiFi, " **********read BLE_mac = (%02x:%02x:%02x:%02x:%02x:%02x) \r\n",ble_mac[0],ble_mac[1],ble_mac[2],ble_mac[3],ble_mac[4],ble_mac[5] );
	ESP_LOGI(WiFi, " **********read flash =%s  len=%d\r\n",flash_wifmac,Collector_Config_LenList[REG_WIFIMAC] );
	if(strcmp(new_wifimac,flash_wifmac)!=0 )// 当前的mac不一样，保存MAC到flash
	{
	//	ESP_LOGI(WiFi, " **********??????????????****************\r\n",flash_wifmac,Collector_Config_LenList[REG_WIFIMAC] );
		IOT_SystemParameterSet(REG_WIFIMAC,new_wifimac,17);
		bzero(new_wifimac, sizeof(new_wifimac));
		sprintf(new_wifimac,"%02x:%02x:%02x:%02x:%02x:%02x" ,ble_mac[0],ble_mac[1],ble_mac[2],ble_mac[3],ble_mac[4],ble_mac[5]);
		IOT_SystemParameterSet(REG_BLEMAC,new_wifimac,17);
	}
}

void IOT_WIFIEven_StartSmartConfig(void)    	  // 通知扫描wifi AP
{
	xEventGroupSetBits(s_wifi_event_group, ESPTOUCH_START_BIT);
}

void IOT_WIFIEven_CloseSmartConfig(void)    	  // 通知扫描wifi AP
{
	xEventGroupSetBits(s_wifi_event_group, ESPTOUCH_DONE_BIT);
}

void IOT_WIFIEvenSetSCAN(void)    	  // 通知扫描wifi AP
{
	xEventGroupSetBits(s_wifi_event_group, WIFI_SCAN_BIT);
}

void IOT_WIFIEvenSetCONNECTED(void)   // 通知连接
{
	xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECT_BIT);
	//IOT_WiFi_Connect();
}

void IOT_WIFIEvenSetDISCONNECTED(void)   // 通知关闭
{
	xEventGroupSetBits(s_wifi_event_group, WIFI_DISCONNECT_BIT);
	//IOT_WiFi_Connect();
}


void IOT_WIFIEvenRECONNECTED(void)   // 通知重新连接
{
	bWIFIReConnectFlag = true;
	IOT_WIFIEvenSetDISCONNECTED();	//关闭WIFI
	//IOT_WiFi_Connect();
}


void IOT_WIFIEvenSetOAT(void)   	 // 通知升级
{
	xEventGroupSetBits(s_wifi_event_group, WIFI_OTA_BIT);
}
void IOT_WIFIEvenSetON(void)   	 // 通知升级
{
	xEventGroupSetBits(s_wifi_event_group, WIFI_ON_BIT);
}
void IOT_WIFIEvenSetOFF(void)   	 // 通知升级
{
	xEventGroupSetBits(s_wifi_event_group, WIFI_OFF_BIT);
}

void IOT_WaitOTAback( void )
{
	EventBits_t bits ;
    ESP_LOGI(WiFi, "IOT_WaitOTAback  waitin ...  \r\n");
    bits = xEventGroupWaitBits(s_wifi_event_group,
    		WIFI_OTA_BIT,
			pdTRUE,
          pdFALSE,
          portMAX_DELAY);
    if( ( bits & WIFI_OTA_BIT ) != 0 )
    {
  	    ESP_LOGI(WiFi, "***********IOT_WaitOTAback callback*********** \r\n");
    }
}
void IOT_WIFIOnlineFlag(uint8_t uWififlag)
{
	 char data[10]={0};
	 char REG_WIFISTATUSBUF[5];
	 memset(data,0,10);
//#if test_wifi==0
	 memset(REG_WIFISTATUSBUF,0,5);
	 memcpy(REG_WIFISTATUSBUF ,&SysTAB_Parameter[Parameter_len[REG_WIFISTATUS]],Collector_Config_LenList[REG_WIFISTATUS] );
	 if(uWififlag == 1 )  // wifi 配网成功。在线
	 {
		 if(  strcmp(REG_WIFISTATUSBUF,"0")!=0  )
		 {
		     memcpy( data,"0", 1 );
			 IOT_SystemParameterSet(REG_WIFISTATUS ,data,1 );
	 	 }
	 }
	 else  if(uWififlag == 2)  // wifi 配网失败。密码错误
	 {
		 if( strcmp(REG_WIFISTATUSBUF,"204")!=0 )
		 {
		     memcpy( data,"204", 3 );
			 IOT_SystemParameterSet(REG_WIFISTATUS ,data,3 );
	 	 }
	 }
	 else  if(uWififlag == 3)  // wifi 配网失败。wifi 名称错误
	 {
		 if( strcmp(REG_WIFISTATUSBUF,"201")!=0 )
		 {
		     memcpy( data,"201", 3 );
			 IOT_SystemParameterSet(REG_WIFISTATUS ,data,3 );
	 	 }
	 }
	 else   				  // 正在连接中
	 {
		 if( strcmp(REG_WIFISTATUSBUF,"255")!=0  )
		 {
		     memcpy( data,"255", 3);
			 IOT_SystemParameterSet(REG_WIFISTATUS,data,3);
	 	 }
	 }

}



void IOT_WIFIstation_Task(void *arg)
{

	  EventBits_t wifi_bits ;
	  s_wifi_event_group = xEventGroupCreate();

	  uint8_t stationCAN=0;
#if DEBUG_HEAPSIZE
	  static uint8_t timer_logob=0;
#endif

    while(1)
    {
    //	IOT_WIFIhandlercallback(s_wifi_event_group);

    	wifi_bits = xEventGroupWaitBits(s_wifi_event_group,
              WIFI_CONNECTED_BIT | WIFI_FAIL_BIT|WIFI_SCAN_BIT| WIFI_CONNECT_BIT|WIFI_ON_BIT|WIFI_OFF_BIT|ESPTOUCH_DONE_BIT|\
			  ESPTOUCH_START_BIT|WIFI_DISCONNECT_BIT | WIFI_RECONNECT_BIT,
  			  pdTRUE,
              pdFALSE,
              portMAX_DELAY);
    	ESP_LOGI(WiFi, "xEventGroupWaitBits  in  bits=0x%x \r\n",wifi_bits);


        if( ( wifi_bits & WIFI_CONNECTED_BIT ) != 0 )
        {
        	wifi_ap_record_t ap_info = {0};
        	char cRssi[10] = {0};

        	ScanIntervalTime = 60;   //连接成功 , WIFI扫描时间间隔修改为60s
        	IOT_WIFIOnlineFlag(1);
        	System_state.Wifi_ActiveReset &= ~WIFI_CONNETING;
        	ESP_LOGI(WiFi, "***********WIFI_CONNECTED_BIT callback*********** \r\n");

			System_state.WIFI_BLEdisplay |=Displayflag_WIFICON;  //写便携式显示状态
			IOT_KEYSET0X06(56,System_state.WIFI_BLEdisplay );

			esp_wifi_sta_get_ap_info(&ap_info);

			if(ap_info.rssi < 0) ap_info.rssi = -ap_info.rssi;

			sprintf(cRssi,"-%03d",ap_info.rssi);
			ESP_LOGI(WiFi, "***********s ap_info.rssi = %s*********** \r\n" , cRssi);

			IOT_SystemParameterSet(76 , cRssi , strlen(cRssi) );  //保存信号值

			System_state.Wifi_state = Online;   // WIFI 连接在线
        }
        else if(( wifi_bits & WIFI_FAIL_BIT ) != 0 )
        {
        	System_state.Wifi_state = Offline;
    	    System_state.Wifi_ActiveReset &= ~WIFI_CONNETING;

    	    IOT_WiFiScan_uc(ESP_WRITE , ScanIntervalTime);  //3s 后触发扫描

    	    IOT_WIFIOnlineFlag(2);
      	    ESP_LOGI(WiFi, " ************WIFI_FAIL_BIT callback************ \r\n");
			System_state.WIFI_BLEdisplay &= ~Displayflag_WIFICON;  //写便携式显示状态
			IOT_KEYSET0X06(56,System_state.WIFI_BLEdisplay );

        }
        else if( ( wifi_bits & WIFI_DISCONNECT_BIT ) != 0)
		{

        	System_state.Wifi_ActiveReset &= ~WIFI_isDISCONNET;
        	System_state.Wifi_ActiveReset |= WIFI_DISCONNET;
			ESP_ERROR_CHECK(esp_wifi_disconnect());

			if(true == bWIFIReConnectFlag)	//判断是否需要重新连接
			{
				IOT_WIFIEvenSetCONNECTED();
			}

		}
        else if(( wifi_bits & WIFI_SCAN_BIT ) != 0 )   //定时逻辑与 人为操作 冲突 需要优化
        {
        	if((System_state.Wifi_ActiveReset & WIFI_CONNETING) == WIFI_CONNETING)
        	{
        		ESP_LOGI(WiFi, "IOT_WIFIstation_Task -- WIFI is conneting , scan not shart\r\n");
        	}else
        	{
        		System_state.Wifi_ActiveReset |= WIFI_SCAN;

        		stationCAN = wifi_scan();

        		System_state.Wifi_ActiveReset &= ~WIFI_SCAN;

				if((InvCommCtrl.CMDFlagGrop & CMD_0X19))
				{
					InvCommCtrl.CMDFlagGrop &= ~CMD_0X19;
					ESP_LOGI(WiFi, "----------------IOT_GattsEvenSetR-- GET_REGNUM=%d------- GET_REGAddrStart=%d---------\r\n",System_state.ServerGET_REGNUM,System_state.ServerGET_REGAddrStart);
					IOT_GattsEvenSetR();
				}

				if(2 == stationCAN)		//WIFI 正在连接 ,关闭 防卡死
				{
					IOT_WiFiScan_uc(ESP_WRITE , ScanIntervalTime);  //正在连接,自动扫描延后执行
					IOT_WIFIEvenSetDISCONNECTED();
				}
				else if(stationCAN==1)  //发现已经保存WiFi
				{
					if(System_state.Wifi_state == Offline)
					{
						//IOT_WiFi_Connect();
						IOT_WIFIEvenSetCONNECTED();
					}
				}
				ESP_LOGI(WiFi, " ************WIFI_SCAN_BIT callback*** .Wifi_state=%d********* \r\n",System_state.Wifi_state);
        	}
        }

        else if( ( wifi_bits & WIFI_CONNECT_BIT ) != 0 )
        {
			if(SysTAB_Parameter[Parameter_len[REG_WIFISSID]]=='0' && Collector_Config_LenList[REG_WIFISSID]==1 )
			{
			 ESP_LOGI(WiFi, "IOT_WIFIstation is No parameters\r\n");
			}
			else
			{
				IOT_WiFiConnectTimeout_uc(ESP_WRITE , WIFI_CONNECT_TIMEOUT );		//设置连接超时时间 15s
				System_state.Wifi_ActiveReset |= WIFI_CONNETING;
				ESP_LOGI(WiFi, "**********IOT_WiFi_Connect **********\r\n");

				IOT_WiFiScan_uc(ESP_WRITE , ScanIntervalTime);  //正在连接,自动扫描延后执行

				IOT_WiFi_Connect(); // 设置wifi账户密码 连接wifi  账户需要保存在flash /更新，  可以优化 参数传入方式
				ESP_LOGI(WiFi, "**********IOT_WiFi_Connect **********\r\n");
			}
			ESP_LOGI(WiFi, "IOT_WIFIhandlercallback   WIFI_CONNECT_BIT \r\n ");
        }
        else if( ( wifi_bits & WIFI_RECONNECT_BIT ) != 0 )
		{
        	esp_err_t err;

        	IOT_WiFiConnectTimeout_uc(ESP_WRITE , WIFI_CONNECT_TIMEOUT );		//设置连接超时时间 15s

        	IOT_WiFiScan_uc(ESP_WRITE , ScanIntervalTime);  //正在连接,自动扫描延后执行
        	System_state.Wifi_ActiveReset |= WIFI_CONNETING;
        	err = esp_wifi_connect();
			if(err == ESP_OK)
			{
				ESP_LOGI(WiFi, "ESP_OK ");
			}
			else if(err == ESP_ERR_WIFI_NOT_INIT)		// 没有初始化
			{
				ESP_LOGI(WiFi, "ESP_ERR_WIFI_NOT_INIT");
			}
			else if(err == ESP_ERR_WIFI_NOT_STARTED)	// 没有started启动
			{
				ESP_LOGI(WiFi, "ESP_ERR_WIFI_NOT_STARTED");
			}
			else if(err == ESP_ERR_WIFI_CONN)  			// wifi 内部错误
			{
				ESP_LOGI(WiFi, "ESP_ERR_WIFI_CONN");
			}
			else if(err == ESP_ERR_WIFI_SSID)			// SSID 无效
			{
				ESP_LOGI(WiFi, "ESP_ERR_WIFI_SSID\n");
			}
			else
			{

			}
		}
        else if( ( wifi_bits & WIFI_OFF_BIT ) != 0 )
        {
        	System_state.wifi_onoff = 0;
        }
        else if( ( wifi_bits & WIFI_ON_BIT ) != 0 ) //写事件   --- APP 写数据，接收解析
	    {
	    	ESP_LOGI(WiFi, "ESP_WIFI_MODE_STA  System_state.wifi_onoff =%d \r\n",System_state.wifi_onoff);
	    	if(System_state.wifi_onoff==0)
	    	IOT_WiFistationInit();
	        System_state.wifi_onoff=1;
	    }


#if SMARTCONFIG_EN		//标准配网
		else if( ( wifi_bits & ESPTOUCH_START_BIT ) != 0 )
		{
			IOT_WIFI_SmartConfig_Start();  //开启标准模式
		}
        else if( ( wifi_bits & ESPTOUCH_DONE_BIT ) != 0 )
        {
        	IOT_WIFI_SmartConfig_Close();

        	if(true == bSmartConfigRestartFlag)	//如果需要重启标准配网 , 在关闭后重新开启
        	{
        		IOT_WIFIEven_StartSmartConfig();
        		bSmartConfigRestartFlag = false;
        	}else
        	{
        		System_state.SmartConfig_OnOff_Flag = 0;
        	}
        }
#endif

        ESP_LOGI(WiFi, "xEventGroupWaitBits  out 0x%x \r\n",wifi_bits);
#if DEBUG_HEAPSIZE

		if(timer_logob++ > 5)
		{
			timer_logob =0;
			ESP_LOGI(WiFi, "is IOT_WIFIstation_Task  uxTaskGetStackHighWaterMark= %d !!!\r\n", uxTaskGetStackHighWaterMark(NULL));
		}
#endif

    //	vTaskDelay(100/ portTICK_PERIOD_MS);
    }
}
