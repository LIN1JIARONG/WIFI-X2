/*
 * iot_net.c
 *
 *  Created on: 2021��12��2��
 *      Author: Administrator
 */
/* HTTPS GET Example using plain mbedTLS sockets
 *
 * Contacts the howsmyssl.com API via TLS v1.2 and reads a JSON
 * response.
 *
 * Adapted from the ssl_client1 example in mbedtls.
 *
 * Original Copyright (C) 2006-2016, ARM Limited, All Rights Reserved, Apache 2.0 License.
 * Additions Copyright (C) Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD, Apache 2.0 License.
 *
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
//#include "protocol_examples_common.h"
#include "esp_netif.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "mbedtls/platform.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/esp_debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"
#include "esp_crt_bundle.h"

#include "iot_system.h"
#include "iot_mqtt.h"
#include "iot_net.h"
#include "iot_protocol.h"

#include "iot_inverter.h"
#include "iot_system.h"

#include "mbedtls/net_sockets.h"


/* Constants that aren't configurable in menuconfig */
//#define WEB_SERVER "charger-server.atesspower.com"
//#define WEB_PORT "8080"

//#define WEB_SERVER "ces.growatt.com"
//#define WEB_PORT "7006"

/*static const char *REQUEST = "GET" WEB_URL "HTTP/1.0\r\n"
    "Host: "WEB_SERVER"\r\n"
    "User-Agent: esp-idf/1.0 esp32\r\n"
    "\r\n";
*/

//#if test_wifi
#define  MBEDNET_Enable     0 // 1�ر�SSL/TLS 1.2    0  1 ��SSL/TLS 1.2  ���ӵ���CES.GROWATT.COM
//#endif

#define  TCP_Enable 		0
#define  MQTT_Enable 		0


mbedtls_entropy_context entropy;
mbedtls_ctr_drbg_context ctr_drbg;
mbedtls_ssl_context ssl;
mbedtls_x509_crt cacert;
mbedtls_ssl_config conf;
mbedtls_net_context server_fd;

unsigned char MQTT_TXbuf[1024] ={0};
unsigned char MQTT_RXbuf[512] ={0};

unsigned char MQTT_DATAbuf[1024] = {0};
uint16_t MQTT_DATALen=0;
static const char *Mbedtls_Log = "Mbedtls";

void IOT_Mbedtls_exit( mbedtls_net_context *ctx,int uret,unsigned char *ubuf )
{
	 if((System_state.Wifi_state == 0) || (1 == uSetserverflag))		//wifi�Ͽ���Ҫ��������ssl  //20220811 chen ת������Ҳ��Ҫ��������ssl
	 {
		 //mbedtls_ssl_config_free( &conf );
		ESP_LOGE(Mbedtls_Log, "free ssl");
		mbedtls_ssl_free( &ssl );
		 System_state.SSL_state  = 0;
	 }else									//tcp����������������ssl��ֻ��Ҫ����
	 {
		 if(System_state.SSL_state == 1)		//20220729 chen ����ssl�����ĳɹ�,����reset
		 {
			 ESP_LOGE(Mbedtls_Log, "reset ssl");
			 mbedtls_ssl_session_reset(&ssl);
		 }
	 }

	 // mbedtls_net_free(ctx);


	  System_state.Mbedtls_state=0;     //MQTT SSL ����
	  Mqtt_state.MQTT_connet=0;   		//MQTT ��¼�Ͽ�
		System_state.Server_Online   = 0;
	  ESP_LOGE(Mbedtls_Log, "\r\n****************IOT_Mbedtls_exit!!!!!!!!!!!!!!**********************\r\n");
	  if(uret != 0)
	  {
	      mbedtls_strerror(uret, (char *)ubuf, 100);
	      ESP_LOGE(Mbedtls_Log, "Last error was: -0x%x - %s", -uret, ubuf);
		  ESP_LOGE(Mbedtls_Log, "Last error was:-0x%x", -uret);
	  }

	  putchar('\n'); // JSON output doesn't have a newline at end

	  static int request_count;
	  ESP_LOGI(Mbedtls_Log, "Completed %d requests", ++request_count);
	   for(int countdown = 3; countdown >= 0; countdown--)
	   {
	       ESP_LOGI(Mbedtls_Log, "%d...", countdown);
	      vTaskDelay(1000 / portTICK_PERIOD_MS);
	   }
	   ESP_LOGI(Mbedtls_Log, "Starting again!");
}


uint8_t IOT_SSL_Init(void)
{
#if MBEDNET_Enable
	System_state.SSL_state=1;
	return OK;

#else
	 int ret=0;
	 char Server_URLbuf[50]={0};
	 char Server_PORTbuf[10]={0};

	 mbedtls_ssl_init(&ssl);   			 // ��ʼ��SSL������

	 mbedtls_x509_crt_init(&cacert);     // ʼ����֤����
	 mbedtls_ctr_drbg_init(&ctr_drbg);   // ��ʼ��CTR_DRBG������
	 ESP_LOGI(Mbedtls_Log, "Seeding the random number generator");
	 mbedtls_ssl_config_init(&conf);  	 // ��ʼ��SSL����������
	 mbedtls_entropy_init(&entropy);     // ��ʼ������
	 if((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
	                                    NULL, 0)) != 0)  //CTR_DRBG��������������  �����ʼ�����ܵ���
	 {
	    ESP_LOGE(Mbedtls_Log, "mbedtls_ctr_drbg_seed returned %d", ret);
	    abort();
	 }
	 ESP_LOGI(Mbedtls_Log, "Attaching the certificate bundle...");
	 ESP_LOGI(Mbedtls_Log, "Setting hostname for TLS session...");
	  if( IOT_SETConnect_uc(0xff) == 1)  //�޸�����
	  {
	    	bzero(Server_URLbuf, sizeof(Server_URLbuf));
	    	bzero(Server_PORTbuf, sizeof(Server_PORTbuf));
	     	memcpy(Server_URLbuf,_ucServerIPBuf,strlen(_ucServerIPBuf));
	    	memcpy(Server_PORTbuf,_ucServerPortBuf,strlen(_ucServerPortBuf));
	        ESP_LOGI(Mbedtls_Log, "Connecting IOT_SETConnect_uc ==1 \r\n");
	  }
	  else
	  {
		    printf("Jump here!\r\n");
	        bzero(Server_URLbuf, sizeof(Server_URLbuf));
	        bzero(Server_PORTbuf, sizeof(Server_PORTbuf));
	        for(int i = 0;i < Collector_Config_LenList[REG_ServerIP];i++)
	        {
	        	//printf("%c",SysTAB_Parameter[Parameter_len[REG_ServerIP + i]]);
	        	printf("%c",SysTAB_Parameter[Parameter_len[REG_ServerIP] + i]);
	        }
	        printf("\r\n");
	        for(int i = 0;i < Collector_Config_LenList[REG_ServerPORT];i++)
	        {
	        	printf("%c",SysTAB_Parameter[Parameter_len[REG_ServerPORT] + i]);
	        }
	        printf("\r\n");
	        //printf("%19s\r\n %s\r\n",&SysTAB_Parameter[Parameter_len[REG_ServerIP]],SysTAB_Parameter[Parameter_len[REG_ServerPORT]]);
	        memcpy(Server_URLbuf,&SysTAB_Parameter[Parameter_len[REG_ServerIP]],Collector_Config_LenList[REG_ServerIP]);
	        memcpy(Server_PORTbuf,&SysTAB_Parameter[Parameter_len[REG_ServerPORT]],Collector_Config_LenList[REG_ServerPORT]);
	        ESP_LOGI(Mbedtls_Log, "Connecting  REG_ServerIP \r\n");
	  }
	 ESP_LOGI(Mbedtls_Log, "Connecting to %s:%s...\r\n", Server_URLbuf, Server_PORTbuf);

	if((strncmp(Server_URLbuf,"20.6.1.65",9) ==0 ) ||(strncmp(Server_URLbuf,"20.60.5.104",11) ==0 )\
			||(strncmp(Server_URLbuf,"20.60.5.231",11) ==0 )||(strncmp(Server_URLbuf,"183.62.216.36",sizeof("183.62.216.36")) ==0 )\
			||(strncmp(Server_URLbuf,"10.5.5.63",sizeof("10.5.5.63")) ==0 )\
			/*||(strncmp(Server_URLbuf,"server-cn.growatt.com",sizeof("server-cn.growatt.com")) ==0 )*/)   //
	{
		printf("Not jump here!\r\n");
		//���ط�����������
		IOT_Printf(" MBEDTLS_SSL_VERIFY_NONE\r\n");
	}
	else
	{
		printf("Not jump here!\r\n");
		ret = esp_crt_bundle_attach(&conf);  // ���Ӳ�����һ��������֤����֤
		if(ret < 0)
		{
		  printf("Not jump here!\r\n");
		  ESP_LOGE(Mbedtls_Log, "esp_crt_bundle_attach returned -0x%x\n\n", -ret);
		  abort();
		}
	}

    /* Hostname set here should match CN in server certificate */  //�������õ�������Ӧ���������֤���е�CNƥ��
	if((ret = mbedtls_ssl_set_hostname(&ssl, Server_URLbuf)) != 0)
	{
	   printf("Not jump here!\r\n");
	   ESP_LOGE(Mbedtls_Log, "mbedtls_ssl_set_hostname returned -0x%x", -ret);
	   abort();
	}
	printf("Jump here!\r\n");
	ESP_LOGI(Mbedtls_Log, "Setting up the SSL/TLS structure...");
	if((ret = mbedtls_ssl_config_defaults(&conf,
	                                          MBEDTLS_SSL_IS_CLIENT,
	                                          MBEDTLS_SSL_TRANSPORT_STREAM,
	                                          MBEDTLS_SSL_PRESET_DEFAULT)) != 0)           //����SSL/TLS�ṹ  �ͻ���  TLS
	{
		    printf("Not jump here!\r\n");
	        ESP_LOGE(Mbedtls_Log, "mbedtls_ssl_config_defaults returned %d", ret);
	        goto exit;
	}
	/* MBEDTLS_SSL_VERIFY_OPTIONAL is bad for security, in this example it will print
	       a warning if CA verification fails but it will continue to connect.
	       You should consider using MBEDTLS_SSL_VERIFY_REQUIRED in your own code.
	*/
//  mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_REQUIRED);   //����֤����֤��ʽ Ĭ��:��������ΪNONE���ͻ���ΪREQUIRED


	if((strncmp(Server_URLbuf,"20.6.1.65",9) ==0 ) ||(strncmp(Server_URLbuf,"20.60.5.104",11) ==0 )\
			||(strncmp(Server_URLbuf,"20.60.5.231",11) ==0 )||(strncmp(Server_URLbuf,"183.62.216.36",sizeof("183.62.216.36")) ==0 )\
			||(strncmp(Server_URLbuf,"10.5.5.63",sizeof("10.5.5.63")) ==0 )\
			/*||(strncmp(Server_URLbuf,"server-cn.growatt.com",sizeof("server-cn.growatt.com")) ==0 )*/)   //
	{
		printf("Not jump here!\r\n");
		mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_NONE);
	    ESP_LOGE(Mbedtls_Log, "mbedtls_ssl_conf_authmode******************** MBEDTLS_SSL_VERIFY_NONE*************\n\n");
	}
	else
	{
		printf("Jump here!\r\n");
		mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_REQUIRED);
		ESP_LOGE(Mbedtls_Log, "mbedtls_ssl_conf_authmode******************** MBEDTLS_SSL_VERIFY_REQUIRED*************\n\n");
	}

	mbedtls_ssl_conf_ca_chain(&conf, &cacert, NULL);  				 //������֤�Զ�֤�����������
	mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg); //����������������ص�

#ifdef CONFIG_MBEDTLS_DEBUG
	mbedtls_esp_enable_debug_log(&conf, CONFIG_MBEDTLS_DEBUG_LEVEL);
#endif

	if ((ret = mbedtls_ssl_setup(&ssl, &conf)) != 0)   				 //����ʹ�õ�SSL������
	{
		printf("Not jump here!\r\n");
	    ESP_LOGE(Mbedtls_Log, "mbedtls_ssl_setup returned -0x%x\n\n", -ret);
	    goto exit;
	}
	System_state.SSL_state=1;  							    //SSL ��ʼ���������
	return OK;
	exit:
//	IOT_Mbedtls_exit(&server_fd,ret,MQTT_RXbuf);
	System_state.SSL_state=0;  							    //SSL ��ʼ���������
	return FAIL;
#endif

}

#if MBEDNET_Enable

#if 1
#define WEB_SERVER "20.60.5.231"       //183.62.216.35 // 127.0.0.1  //20.6.1.65   //20.60.5.104 //20.60.5.231
#define WEB_PORT "7006" 			 //7006

#else
#define WEB_SERVER "20.60.5.220"
#define WEB_PORT "8080"

#endif

#define WEB_PATH "/"

static const char *HTTP = "example";

static const char *REQUEST = "GET " WEB_PATH " HTTP/1.0\r\n"
    "Host: "WEB_SERVER":"WEB_PORT"\r\n"
    "User-Agent: esp-idf/1.0 esp32\r\n"
    "\r\n";
int s ;
#endif



void IOT_Connet_Close(void)
{

#if	MBEDNET_Enable
	close(s);
#else
	mbedtls_ssl_close_notify( &ssl );		//֪ͨ����������׼���Ͽ� , �������� ��֪ͨ����

	if( server_fd.fd == -1 )
	return;

	close(server_fd.fd );			//�ر�����

	server_fd.fd = -1;

	//mbedtls_net_free( &server_fd );
	//mbedtls_ssl_free( &ssl );
	//mbedtls_ssl_config_free( &conf );
#endif
}

uint8_t IOT_Mbedtls_Init(void)
{
#if MBEDNET_Enable
	const struct addrinfo hints = {
	        .ai_family = AF_INET,
	        .ai_socktype = SOCK_STREAM,
	    };
	    struct addrinfo *res;
	    struct in_addr *addr;
	    int   r;
	    char recv_buf[64];
    int err = getaddrinfo(WEB_SERVER, WEB_PORT, &hints, &res);

      if(err != 0 || res == NULL) {
          ESP_LOGE(HTTP, "DNS lookup failed err=%d res=%p", err, res);
//          vTaskDelay(1000 / portTICK_PERIOD_MS);
//          continue;
      }

      /* Code to print the resolved IP.

         Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
      addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
      ESP_LOGI(HTTP, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

      s = socket(res->ai_family, res->ai_socktype, 0);
      if(s < 0) {
          ESP_LOGE(HTTP, "... Failed to allocate socket.");
          freeaddrinfo(res);
//          vTaskDelay(1000 / portTICK_PERIOD_MS);
//          continue;

          System_state.Mbedtls_state = 0;
          return FAIL;

      }
      ESP_LOGI(HTTP, "... allocated socket");

      if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
          ESP_LOGE(HTTP, "... socket connect failed errno=%d", errno);
          close(s);
          freeaddrinfo(res);
//          vTaskDelay(4000 / portTICK_PERIOD_MS);
//          continue;
        System_state.Mbedtls_state=0;

       	return FAIL;
      }

      System_state.Mbedtls_state=1;

	return OK;
#else

    char buf[512];
	int ret=0;
	int flags=0 ;
    mbedtls_net_init(&server_fd);  //��ʼ��һ��������

    char Server_URLbuf[50]={0};
    char Server_PORTbuf[10]={0};

    if(	IOT_SETConnect_uc(0xff) == 1)  //�޸�����
    {
    	bzero(Server_URLbuf, sizeof(Server_URLbuf));
    	bzero(Server_PORTbuf, sizeof(Server_PORTbuf));
     	memcpy(Server_URLbuf,_ucServerIPBuf,strlen(_ucServerIPBuf));
    	memcpy(Server_PORTbuf,_ucServerPortBuf,strlen(_ucServerPortBuf));
        ESP_LOGI(Mbedtls_Log, "Connecting IOT_SETConnect_uc ==1 \r\n");
    }
    else
    {
    	printf("Jump here!\r\n");
        bzero(Server_URLbuf, sizeof(Server_URLbuf));
        bzero(Server_PORTbuf, sizeof(Server_PORTbuf));
        memcpy(Server_URLbuf,&SysTAB_Parameter[Parameter_len[REG_ServerIP]],Collector_Config_LenList[REG_ServerIP]);
        memcpy(Server_PORTbuf,&SysTAB_Parameter[Parameter_len[REG_ServerPORT]],Collector_Config_LenList[REG_ServerPORT]);
        ESP_LOGI(Mbedtls_Log, "Connecting  REG_ServerIP \r\n");
    }

    ESP_LOGI(Mbedtls_Log, "Connecting to %s:%s...\r\n", Server_URLbuf, Server_PORTbuf);

    if ((ret = mbedtls_net_connect(&server_fd, Server_URLbuf,
    		Server_PORTbuf, MBEDTLS_NET_PROTO_TCP)) != 0)  		  // �ڸ�����Э����������host:port������
    {
       ESP_LOGE(Mbedtls_Log, "mbedtls_net_connect returned -%x", -ret);
       goto exit;
    }
    ESP_LOGI(Mbedtls_Log, "Connected.");
    mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, NULL);  // Ϊд���������õײ�BIO�ص�
    ESP_LOGI(Mbedtls_Log, "Performing the SSL/TLS handshake...");
    while ((ret = mbedtls_ssl_handshake(&ssl)) != 0)  //ִ��SSL/TLS����
    {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
        {
            ESP_LOGE(Mbedtls_Log, "mbedtls_ssl_handshake returned -0x%x", -ret);
            goto exit;
        }
    }
    ESP_LOGI(Mbedtls_Log, "Verifying peer X.509 certificate...");
    if ((flags = mbedtls_ssl_get_verify_result(&ssl)) != 0)  //��֤�Զ�X.509֤��
    {
        /* In real life, we probably want to close connection if ret != 0 */
        ESP_LOGW(Mbedtls_Log, "Failed to verify peer certificate!");
        bzero(buf, sizeof(buf));
        mbedtls_x509_crt_verify_info( buf, sizeof(buf), "  ! ", flags);
        ESP_LOGW(Mbedtls_Log, "verification info: %s", buf);
    }
    else
    {
        ESP_LOGI(Mbedtls_Log, "Certificate verified.");
    }
    ESP_LOGI(Mbedtls_Log, "Cipher suite is %s", mbedtls_ssl_get_ciphersuite(&ssl));  		// ���ص�ǰ�����׼�������
    ESP_LOGI(Mbedtls_Log, "OKheap_size = %d !!!\r\n", esp_get_minimum_free_heap_size());	//
    ESP_LOGI(Mbedtls_Log, "Writing HTTP request...");
    System_state.Mbedtls_state=1;

// 	IOT_PINGRESPSet_uc(ESP_WRITE,30);   // �ϱ�����
// 	IOT_TCPDelaytimeSet_uc(ESP_WRITE,Mqtt_DATATime);

    return OK;
	exit:
	return FAIL;

#endif
}

uint8_t IOT_Mbdetls_senddata(unsigned char *pucSendDatabuf ,uint32_t ucSend_len)
{
	int ret;
    do{
#if MBEDNET_Enable
    	ret = write(s, (const unsigned char *) pucSendDatabuf, ucSend_len);
#else
    	 ESP_LOGI(Mbedtls_Log, "IOT_Mbdetls_senddata Len = %d",ucSend_len);

        ret = mbedtls_ssl_write(&ssl,
                                    (const unsigned char *)pucSendDatabuf,
									ucSend_len);
        ESP_LOGI(Mbedtls_Log, "IOT_Mbdetls_senddata ret = %d",ret);
#endif

        if (ret >= 0) {
                ESP_LOGI(Mbedtls_Log, "%d bytes written", ret);
                ucSend_len += ret;
        } else if (ret != MBEDTLS_ERR_SSL_WANT_WRITE && ret != MBEDTLS_ERR_SSL_WANT_READ) {
            ESP_LOGE(Mbedtls_Log, "mbedtls_ssl_write returned -0x%x", -ret);
            goto exit;
         }
     }while(ucSend_len < strlen((char *)pucSendDatabuf));
     return OK;
     exit:

//     for(int countdown = 2; countdown >= 0; countdown--)
//     {
//    	 IOT_Mbedtls_exit(&server_fd,ret,MQTT_RXbuf);
//         ESP_LOGI(Mbedtls_Log, "%d...", countdown);
//         vTaskDelay(1000 / portTICK_PERIOD_MS);
//     }

	 IOT_Connet_Close();

 	 return FAIL;

}

void IOT_MbedtlsWrite_task(void *pvParameters)
{
   uint32_t written_bytes;
#if DEBUG_HEAPSIZE
	static	uint8_t timer_logoh=0;
#endif
   Write_init:
	 System_state.Server_Online   = 0;
	 Mqtt_state.MQTT_connet = 0;
   do
   {
 		vTaskDelay(5000 / portTICK_PERIOD_MS);
   }
   while( !System_state.Mbedtls_state) ; 	// �ȴ�WiFi ����·��������

   while(1)
   {
	if(System_state.Mbedtls_state==1) 	 	// SSL ��������
	{
		if(Mqtt_state.MQTT_connet==1)  		// MQTT��¼   iOT_MQTT.h  ״̬
		{
			printf("Jump here:IOT_ReadyForData().\r\n");
			if(IOT_ReadyForData()==ESP_READY )    								 // ���ݲ�ѯ�Ƿ����
			{
				ESP_LOGE(Mbedtls_Log, "*************_g_EMSGMQTTSendTCPType %d******\r\n",_g_EMSGMQTTSendTCPType);
				if(_g_EMSGMQTTSendTCPType == EMSG_INVPARAMS_POST)				// �����ϱ������ 0x03
				{
					MQTT_DATALen=IOT_ESPSend_0x03(MQTT_DATAbuf ,REMOTE_SERVER_PROTOCOL );
					ESP_LOGI(Mbedtls_Log, "ESP_READY  IOT_ESPSend_0x03  MQTT_DATALen=%d \r\n"  ,MQTT_DATALen);
				}
				else if(_g_EMSGMQTTSendTCPType == EMSG_ONLINE_POST )		 	// �����ϱ������ 0x04
				{
					MQTT_DATALen=IOT_ESPSend_0x04(MQTT_DATAbuf,REMOTE_SERVER_PROTOCOL );
					ESP_LOGI(Mbedtls_Log, "ESP_READY  IOT_ESPSend_0x04 MQTT_DATALen=%d \r\n", MQTT_DATALen );
				}
				else if(_g_EMSGMQTTSendTCPType == EMSG_SYSTEM_POST	)   	 	// �Զ��ϱ��ɼ��� 0x19
				{
					MQTT_DATALen=IOT_UploadParamSelf(MQTT_DATAbuf,REMOTE_SERVER_PROTOCOL );
					ESP_LOGI(Mbedtls_Log, "ESP_READY  IOT_UploadParamSelf MQTT_DATALen=%d \r\n", MQTT_DATALen );
				}
				else if(  _g_EMSGMQTTSendTCPType == EMSG_GET_INVINFO_RESPONSE)  // �����������ȡ 0x05��ѯ����
				{
					if(System_state.Host_setflag & CMD_0X05 )
					{
					//	if(!(InvCommCtrl.CMDFlagGrop & CMD_0X05 ))  // ���������Ӧ����λ �ȴ� �����߳�������ݴ���
						{
						MQTT_DATALen=IOT_ESPSend_0x05(MQTT_DATAbuf,REMOTE_SERVER_PROTOCOL );
						ESP_LOGI(Mbedtls_Log, "ESP_READY  IOT_ESPSend_0x05 MQTT_DATALen=%d \r\n", MQTT_DATALen );
					    }
					}
				}
				else if( _g_EMSGMQTTSendTCPType == EMSG_GET_INVREG_RESPONSE)  	// ��������� �������� Ӧ��  06 10����Ӧ��---�ɹ�ACK  ʧ��publish
				{	// 06 10���� ʧ��publish
					if(System_state.Host_setflag & CMD_0X06 )
					{
					//	if(!(InvCommCtrl.CMDFlagGrop & CMD_0X06 ))  // ���������Ӧ����λ �ȴ� �����߳�������ݴ���
						{
							MQTT_DATALen=IOT_ESPSend_0x06(MQTT_DATAbuf,REMOTE_SERVER_PROTOCOL );
							ESP_LOGI(Mbedtls_Log, "ESP_READY  IOT_ESPSend_0x06 MQTT_DATALen=%d \r\n", MQTT_DATALen );
						}
					}
					else if(System_state.Host_setflag & CMD_0X10 )
					{
						//if(!(InvCommCtrl.CMDFlagGrop & CMD_0X10 ))  // ���������Ӧ����λ �ȴ� �����߳�������ݴ���
						{
							MQTT_DATALen=IOT_ESPSend_0x10(MQTT_DATAbuf,REMOTE_SERVER_PROTOCOL );
							ESP_LOGI(Mbedtls_Log, "ESP_READY  IOT_ESPSend_0x10 MQTT_DATALen=%d \r\n", MQTT_DATALen );
						}
					}

				}
				else if(_g_EMSGMQTTSendTCPType == EMSG_GET_DLINFO_RESPONSE )   	// ��������ѯ�ɼ���������ָ�� 0x19
				{
				//	if(!(InvCommCtrl.CMDFlagGrop & CMD_0X19 ))  // ���������Ӧ����λ �ȴ� �����߳�������ݴ���
					{
						MQTT_DATALen=IOT_ESPSend_0x19(MQTT_DATAbuf,REMOTE_SERVER_PROTOCOL );
						ESP_LOGI(Mbedtls_Log, "ESP_READY  IOT_ESPSend_0x19 MQTT_DATALen=%d \r\n", MQTT_DATALen );
					}
				}
				else if(_g_EMSGMQTTSendTCPType == EMSG_GET_DLIREG_RESPONSE ) 	// ���������òɼ���������ָ�� 0x18
				{
					MQTT_DATALen=IOT_ESPSend_0x18(MQTT_DATAbuf,REMOTE_SERVER_PROTOCOL );
					ESP_LOGI(Mbedtls_Log, "ESP_READY  IOT_ESPSend_0x18 MQTT_DATALen=%d \r\n", MQTT_DATALen );
				}
				else if(  _g_EMSGMQTTSendTCPType ==  EMSG_OFFLINE_POST)  		// ��ʷ���� �������� 0x50
				{
					MQTT_DATALen = IOT_ESPSend_0x50(MQTT_DATAbuf,REMOTE_SERVER_PROTOCOL );
					if(MQTT_DATALen ==0 )
					{
						IOT_MqttSendStep_Set(1,0);
						_g_EMSGMQTTSendTCPType = EMSG_NULL;
					}
				 ESP_LOGI(Mbedtls_Log, "ESP_READY  IOT_ESPSend_0x50 MQTT_DATALen=%d \r\n", MQTT_DATALen );

				}
				else if( _g_EMSGMQTTSendTCPType ==  EMSG_AMMTER_POST)			// ���ʵ�� 0x20 У�鳤�� ��ֹ��ʷ�����쳣
				{
					MQTT_DATALen=IOT_ESPSend_0x20(MQTT_DATAbuf,REMOTE_SERVER_PROTOCOL );
					if(MQTT_DATALen ==0 )
					{
						IOT_MqttSendStep_Set(1,0);
						_g_EMSGMQTTSendTCPType = EMSG_NULL;
					}
					ESP_LOGI(Mbedtls_Log, "ESP_READY  IOT_ESPSend_0x20 MQTT_DATALen=%d \r\n", MQTT_DATALen );
				}
				else if( _g_EMSGMQTTSendTCPType ==  EMSG_HISTORY_AMMTER_POST)   // ��ʷ������� 0x22
				{
					MQTT_DATALen=IOT_ESPSend_0x22(MQTT_DATAbuf,REMOTE_SERVER_PROTOCOL );
					if(MQTT_DATALen ==0 )
					{
						IOT_MqttSendStep_Set(1,0);
						_g_EMSGMQTTSendTCPType = EMSG_NULL;
					}
					ESP_LOGI(Mbedtls_Log, "ESP_READY  IOT_ESPSend_0x22 MQTT_DATALen=%d \r\n", MQTT_DATALen );
				}
				else if( Mqtt_state.MqttSend_step == 4 )   						// ������ACKӦ��
				{
				    ESP_LOGI(Mbedtls_Log, "******ACK******* MqttSend_step  =%d  *************\r\n ", Mqtt_state.MqttSend_step  );
				}
				else if(_g_EMSGMQTTSendTCPType ==EMSG_PROGRESS_POST)			// ���������ϴ� 0x25
				{
					MQTT_DATALen=IOT_ESPSend_0x25(MQTT_DATAbuf , REMOTE_SERVER_PROTOCOL);	// �ϴ����� 7.0 Э��
					ESP_LOGI(Mbedtls_Log, "ESP_READY  IOT_ESPSend_0x25 MQTT_DATALen=%d \r\n", MQTT_DATALen );
				}
				else if(_g_EMSGMQTTSendTCPType ==EMSG_INVWAVE_POST)				// һ����������ϴ� 0x14
				{
						MQTT_DATALen=IOT_ESPSend_0x14(MQTT_DATAbuf , REMOTE_SERVER_PROTOCOL);	//
						ESP_LOGI(Mbedtls_Log, "ESP_READY  IOT_ESPSend_0x14 MQTT_DATALen=%d \r\n", MQTT_DATALen );
				}
				else if(_g_EMSGMQTTSendTCPType ==EMSG_BDC03_POST)
				{
						MQTT_DATALen=IOT_ESPSend_0x37(MQTT_DATAbuf , REMOTE_SERVER_PROTOCOL);	//
						ESP_LOGI(Mbedtls_Log, "ESP_READY  IOT_ESPSend_0x37 MQTT_DATALen=%d \r\n", MQTT_DATALen );
				}
				else if(_g_EMSGMQTTSendTCPType ==EMSG_BDC04_POST)
				{
						MQTT_DATALen=IOT_ESPSend_0x38(MQTT_DATAbuf , REMOTE_SERVER_PROTOCOL);	//
						ESP_LOGI(Mbedtls_Log, "ESP_READY  IOT_ESPSend_0x38 MQTT_DATALen=%d \r\n", MQTT_DATALen );
				}
				else 	 													   // �ϱ��������ݹ��� AKC/����
				{

				}

			}
//			else if( Mqtt_state.MqttSend_step == 4 )   // ������ACKӦ��
//			{
//		     	 ESP_LOGI(Mbedtls_Log, "******ACK******* MqttSend_step  =%d  *************\r\n ", Mqtt_state.MqttSend_step  );
//			}
			else
			{
			// ESP_LOGI(Mbedtls_Log, "************* MqttSend_step  =%d  *****_g_EMSGMQTTSendTCPType=%d*****Mqtt_state.AgainTime=%d***\r\n ", Mqtt_state.MqttSend_step, _g_EMSGMQTTSendTCPType,Mqtt_state.AgainTime );
			}
		}
		else
		{
		 //	ESP_LOGI(Mbedtls_Log, "************* Mqtt_state.MQTT_connet =%d  **************\r\n ", Mqtt_state.MQTT_connet );
		}
	}
	else
	{
		goto Write_init;
	}

    written_bytes = IOT_MqttDataPack_ui(Mqtt_state.MqttSend_step , MQTT_TXbuf,MQTT_DATAbuf,MQTT_DATALen); //  MQTT ���ݴ��   ���� ��¼��Ϣ
    if(written_bytes>0 &&  (System_state.Wifi_state==1) && (System_state.Mbedtls_state==1 ))
    {
//    	ESP_LOGI(Mbedtls_Log, "*************MqttSend_step  =%d written_bytes=%d*************\r\n", Mqtt_state.MqttSend_step,written_bytes );
      ESP_LOGI(Mbedtls_Log, " MqttDataPack written_bytes=%d  ", written_bytes);
    	for(int j=0; j<written_bytes; j++)
    	{
    		IOT_Printf("%02x ",MQTT_TXbuf[j]);
    	}
    	ESP_LOGI(Mbedtls_Log, "\r\n*************MqttSend_step  =%d written_bytes=%d*************\r\n", Mqtt_state.MqttSend_step,written_bytes );
    	if(IOT_Mbdetls_senddata(MQTT_TXbuf,written_bytes)==OK)   //����MQTT ������
    	{
    		MQTT_DATALen=0;
    		memset(MQTT_TXbuf,0, written_bytes);
    		written_bytes=0;

    		IOT_MqttSendStep_Set(1,0);    //��ǰ������0  �Ƿ���Ҫ�Ż���
       		ESP_LOGI(Mbedtls_Log, "*************IOT_Mbdetls_senddat OK  add  MqttSend_step =%d*************\r\n",Mqtt_state.MqttSend_step);
       		//������0 �����ûӦ�𣬾���BUG ����Ҫ�ط�
    	}
    	else
    	{
       		ESP_LOGI(Mbedtls_Log, "*************IOT_Mbdetls_senddat ERR  add  MqttSend_step =%d*************\r\n",Mqtt_state.MqttSend_step);
    	}
    }
    else if(System_state.Wifi_state==0) //WIFI�Ͽ�
    {
    	Mqtt_state.MqttSend_step = 0 ;
    	System_state.Mbedtls_state = 0 ;
		System_state.Server_Online = 0;
		Mqtt_state.MQTT_connet = 0;
    }
#if DEBUG_HEAPSIZE

    if(timer_logoh++ > 10 )
	{ timer_logoh=0;
		ESP_LOGI(Mbedtls_Log, " IOT_MbedtlsWrite_task uxTaskGetStackHighWaterMark= %d !!!\r\n", uxTaskGetStackHighWaterMark(NULL));
	}
#endif
	vTaskDelay(300 / portTICK_PERIOD_MS);
  }
}

void IOT_MbedtlsRead_task(void *pvParameters)
{
    int ret, len;
    ret=0;
    len=0;

#if DEBUG_HEAPSIZE
	static	uint8_t timer_logog=0;
#endif

  //  esp_log_level_set(Mbedtls_Log, ESP_LOG_INFO);
    InitSSL:
	 System_state.Server_Online = 0;
	 Mqtt_state.MQTT_connet = 0;
    do
    {
      vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
    while(!System_state.Wifi_state); 		//�ȴ�WiFi ����·��������

    if(System_state.Wifi_state==1)
    if(  IOT_SSL_Init() == FAIL   ) 				//System_state.SSL_state=1;
    {	//System_state.SSL_state=0;
    	ESP_LOGI(Mbedtls_Log,"IOT_SSL_Init FAIL \r\n");
      	goto exit;
    }
    //��ʼ��¼ MQTT ������
    ESP_LOGI(Mbedtls_Log,"System_state.SSL_state OK =%d",System_state.SSL_state);
    ESP_LOGI(Mbedtls_Log, "Writing HTTP request...");
    ESP_LOGI(Mbedtls_Log, "mbedtls_ssl version= %d-%d", ssl.major_ver, ssl.minor_ver);
    IOT_MqttSendStep_Set(1,1);  //��¼

    while(1)
	{
#if DEBUG_HEAPSIZE
    	if( timer_logog ++ >10)
        {	timer_logog =0;
        	ESP_LOGI(Mbedtls_Log, "is IOT_MbedtlsRead_task uxTaskGetStackHighWaterMark= %d !!!\r\n", uxTaskGetStackHighWaterMark(NULL));
        }
#endif
     	if((System_state.Wifi_state==1 )  && ( System_state.Mbedtls_state==0) )
     	{	// ���ӷ�����
     		printf("Jump here!\r\n");
     		if(IOT_Mbedtls_Init()== FAIL)	//  System_state.Mbedtls_state=1;
     		{
     			goto exit;
     		}
     	    IOT_MqttSendStep_Set(1,1);  // ���µ�¼ MQTT��Ϊ
     	}
    	ESP_LOGI(Mbedtls_Log, "Reading HTTP response...\r\n");
    	// ����ʽ��������
    	do
    	{
    	    len = sizeof(MQTT_RXbuf) - 1;  //len = 511
    	    bzero(MQTT_RXbuf, sizeof(MQTT_RXbuf));
#if MBEDNET_Enable
    	    ret = read(s,(unsigned char *)MQTT_RXbuf, len);
#else
    	    ret = mbedtls_ssl_read(&ssl, (unsigned char *)MQTT_RXbuf, len);
#endif
    	    printf("Jump here!\r\n");
    	    ESP_LOGI(Mbedtls_Log, "-----------mbedtls_ssl_read OK-----------ret=%d\r\n",ret);

    	    printf("MQTT_RXbuf Data = \r\n");
    	    for(int i = 0;i < ret;i++)
    	    	printf("%02x ",MQTT_RXbuf[i]);
    	    printf("\r\n");
    	    if(ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE)
    	    {
    	          ESP_LOGI(Mbedtls_Log, "sll read err!!!\n");
    	          continue;
    	    }
    	    else if(ret>0)   					//�Ǹ���������յ����ݣ���������Ϊ�쳣
    		{
    	      printf("ret = %d.\r\n",ret); //ʮ���Ʊ�ʾ�Ƚ����
    	      if(IOT_MqttRev_ui(MQTT_RXbuf,ret))//���յ� MQTT ���ݰ�  ���н���
    	      {
    	    	  MQTT_DATALen=0;  				//�������ݳ��ȸ�ֵ����
//    	      	for(int j=0; j<ret; j++)
//    	      	{
//    	      		IOT_Printf("%x ",MQTT_RXbuf[j]);
//    	      	}
    	    	  printf("Jump here!\r\n");
				  ESP_LOGI(Mbedtls_Log, "\r\n-----------IOT_MqttRev_ui MqttSend_step=%d!!!------ret=%d-----\r\n", Mqtt_state.MqttSend_step,ret);
    	      }
    		}
    	    if(ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY)
    	    {

    	    	printf("Jump here!(if error)\r\n");
    	    	ESP_LOGI(Mbedtls_Log, "The peer notified us that the connection is going to be closed.\r\n");
    	        ret = 0;
    	        break;
    	    }
    	    if(ret < 0)
    	    {
    	        ESP_LOGE(Mbedtls_Log, "mbedtls_ssl_read returned -0x%x", -ret);
    	        break;
    	    }
    	    if(ret == 0)
    	    {
    	        ESP_LOGI(Mbedtls_Log, "connection closed");
    	        break;
    	    }
    	    printf("Jump here!\r\n");
    	    ESP_LOGI(Mbedtls_Log, "-----------mbedtls_ssl_read =%d-----------\r\n", ret);
//    	    for(int k = 0; k < ret; k++)
//    	    {
//    	        ESP_LOGE(Mbedtls_Log,"%02x ",MQTT_RXbuf[k]);
//    	    }
    	    ESP_LOGI(Mbedtls_Log,"-----------do while(1)read 1-----------\r\n" );
    	 }
    	 while(1);
    	 ESP_LOGE(Mbedtls_Log,"\r\n--------------------closed  0----------------------*\r\n" );
#if MBEDNET_Enable
    	 exit:
		  System_state.Mbedtls_state=0;     //MQTT SSL ����
	 	  Mqtt_state.MQTT_connet=0;   		//MQTT ��¼�Ͽ�

	 	 System_state.SSL_state = 0;

	 	  if((System_state.SSL_state==0 ) || ( System_state.Wifi_state==0))
	 	     goto	InitSSL;
#else
    	  mbedtls_ssl_close_notify(&ssl);   //֪ͨ�Զ��������ڹر�
    	  mbedtls_net_free(&server_fd);		//�ͷ��ڴ�
    	  exit:

		  System_state.Mbedtls_state = 0;     //MQTT SSL ����
		  Mqtt_state.MQTT_connet=0;   		//MQTT ��¼�Ͽ�
		 System_state.Server_Online   = 0;
// ��֪��ʲôBUG  ����ʱ���޷��ͷ�
//          mbedtls_ssl_session_reset(&ssl);
//          mbedtls_net_free(&server_fd);
//          char buf[50];
//          if(ret != 0)
//          {
//              mbedtls_strerror(ret,  buf, 100);
//              ESP_LOGE(Mbedtls_Log, "Last error was: -0x%x - %s", -ret, buf);
//          }
//          putchar('\n'); // JSON output doesn't have a newline at end

          static int request_count;

          request_count++;
          ESP_LOGI(Mbedtls_Log, "Completed %d requests", request_count);

          for(int countdown = 0; countdown >= 0; countdown--)	//2 �ĳ� 0
          {
        	  IOT_Mbedtls_exit(&server_fd,ret,MQTT_RXbuf);
              ESP_LOGI(Mbedtls_Log, "%d...", countdown);
              vTaskDelay(1000 / portTICK_PERIOD_MS);
          }
//        System_state.Mbedtls_state=0;    //���������Ͽ�����
          ESP_LOGI(Mbedtls_Log, "Starting again!");


          if(0 != System_state.ble_onoff)	//�����������򲻴�������
          {
        	  request_count = 0;
          }

          if(request_count > 100) //��������  ��Ҫ�Ż� ���򵥲��� WiFi�źŲ���ӦBUG
          {
        	  request_count = 0;
        	  //ESP_LOGI(Mbedtls_Log, "Mbedtls RESTART\r\n");
        	 // IOT_SYSRESTART();		//����
          }

          if((System_state.SSL_state==0 ) || ( System_state.Wifi_state==0))
        	  goto	InitSSL;
#endif
	}
}






