/*
 * iot_https_ota.c
 *
 *  Created on: 2021年12月16日
 *      Author: Administrator
 */
/* OTA example
   This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_flash_partitions.h"
#include "esp_partition.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
//#include "protocol_examples_common.h"
#include "errno.h"

#include "esp_spi_flash.h"

//#include "iot_global.h"
#include "iot_params.h"
#include "iot_fota.h"
#include "iot_universal.h"
#include "iot_station.h"

#include "iot_HTTP_update.h"
#include "iot_system.h"

#include "iot_fota.h"

#if CONFIG_EXAMPLE_CONNECT_WIFI
#include "esp_wifi.h"
#endif

#if 1
static const char *HTTPS = "HTTPS";
#define NULLDATA 0

#define HASH_LEN 32 /* SHA-256 digest length */

/*an ota data write buffer ready to write to the flash*/
extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");


bool gbIsBsdiff  = false;		//是否为差分升

#if 0

#if 1
#define HTTPS_FOTA_URL "http://cdn.growatt.com/update/device/New/zktest/Power_test.patch"
#elif 2  // 升级路径测试用
#define HTTPS_FOTA_URL "https://cdn.growatt.com/update/device/New/zktest/WiFi_C3/iot_soc_ota.bin"
#else
#define HTTPS_FOTA_URL "https://cdn.growatt.com/update/device/New/zktest/WiFi_C3/native_ota.bin"
//#define HTTPS_FOTA_URL "https://cdn.growatt.com/update/device/datalog/ShineWiFi-X/3.1.0.2/user1.bin"
#endif



static void http_cleanup(esp_http_client_handle_t client)
{
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
}

static void print_sha256 (const uint8_t *image_hash, const char *label)
{
    char hash_print[HASH_LEN * 2 + 1];
    hash_print[HASH_LEN * 2] = 0;
    for (int i = 0; i < HASH_LEN; ++i) {
        sprintf(&hash_print[i * 2], "%02x", image_hash[i]);
    }
    ESP_LOGI(HTTPS, "%s: %s", label, hash_print);
}

static void infinite_loop(void)
{
    int i = 0;
    ESP_LOGI(HTTPS, "When a new firmware is available on the server, press the reset button to download it");
    while(1)
    {
        ESP_LOGI(HTTPS, "Waiting for a new firmware ... %d", ++i);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

static void __attribute__((noreturn)) task_fatal_error(void)
{
    ESP_LOGE(HTTPS, "Exiting task due to fatal error...");
//    (void)vTaskDelete(NULL);
//    while (1) {
//        ;
//    }
}

/******************************************************************************
 * FunctionName : IOT_FotaSoc_Init()
 * Description  : soc升级内部信息初始化
 * Parameters   : none
 * Returns      : none
 * Notice       : 1、获取新的ota 文件存放位置
*******************************************************************************/
void IOT_FotaSoc_Init(void)
{
	const esp_partition_t *update_partition = NULL;
	const esp_partition_t *configured = esp_ota_get_boot_partition();
	const esp_partition_t *running = esp_ota_get_running_partition();
	if (configured != running)  //获取地址
	{
		ESP_LOGE(HTTPS, "Configured OTA boot partition at offset 0x%08x, but running from offset 0x%08x \r\n",
			  configured->address, running->address);
		ESP_LOGE(HTTPS, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)\r\n");
	}


	ESP_LOGE(HTTPS, "Running partition type %d subtype %d (offset 0x%08x)\r\n",
	running->type, running->subtype, running->address);

	update_partition = esp_ota_get_next_update_partition(NULL);
	ESP_LOGE(HTTPS, "Writing to partition subtype %d at offset 0x%x \r\n",
				update_partition->subtype, update_partition->address);
	g_FotaParams.uiFotaFileStartAddr = update_partition->address; // 得到存放升级文件soc-flash起始地址
	assert(update_partition != NULL);

//	ESP_LOGE(HTTPS, "\r\n\r\n******fota test success, version is : 1.0.0.x ******\r\n\r\n");
//  while(1)
//	{
//		vTaskDelay(5000 / portTICK_RATE_MS);
//	}

}
#endif
/******************************************************************************
 * FunctionName : IOT_FotaHttpFile_Get_uc()
 * Description  : 升级文件读取
 * Parameters   : none
 * Returns      : none
 * Notice       : 1、获取新的ota 文件存放位置
*******************************************************************************/
void IOT_FotaHttpFile_Get_uc(void *pvParameter)
{
	esp_err_t err;

	//const char *cURL= (const char *)SysTAB_Parameter[Parameter_len[80]];	//HTTP 下载URL


	uint8_t ucHTTPDownloadCtrlFlag = 1;
	uint8_t ucHTTPDownloadSuccessedFlag = 0;
	uint8_t ucHTTPDownloadTimes = 0;
	uint32_t uiHTTPErrCode = HTTP_DL_ERROR;
	char cOTAType =0;

	while(1)
	{
		if((System_state.Wifi_state == Online) && (EHTTP_UPDATE == g_FotaParams.uiWayOfUpdate) && (1 == ucHTTPDownloadCtrlFlag))
		{
//			ucHTTPDownloadCtrlFlag = 0;  // 失败也 只下载 1次   屏蔽会下载 3次

			char ucURLBUFF[200] = {0};

			ucHTTPDownloadSuccessedFlag = 0;
			memcpy(ucURLBUFF ,&SysTAB_Parameter[Parameter_len[REG_ServerURL2]] , Collector_Config_LenList[REG_ServerURL2]);
			cOTAType	= IOT_FotaType_Manage_c(0xff, NULLDATA);	//获取升级类型

			err = IOT_HTTP_OTA_i((const char *)ucURLBUFF,cOTAType ,g_FotaParams.ucIsbsdiffFlag);


			switch(err)
			{
				/*HTTP 下载成功*/
				case HTTP_SUCCESSED:
					ESP_LOGE(HTTPS,"HTTP DOWNLOAD SUCCESSED");
					//后续需要延时重启,等待进度上传完成
					ucHTTPDownloadCtrlFlag = 0;
					ucHTTPDownloadSuccessedFlag = 1; //升级成功
					break;
				/*HTTP 初始化失败*/
				case HTTP_ERR_INIT_FAILED:
					ESP_LOGE(HTTPS,"HTTP_ERR_INIT_FAILED");
					break;
				/*HTTP 获取文件头失败*/
				case HTTP_ERR_GET_HEADER_FAILED:
					ESP_LOGE(HTTPS,"HTTP_ERR_GET_HEADER_FAILED");
					break;
				/*HTTP GET请求失败*/
				case HTTP_ERR_GET_FAILED:
					ESP_LOGE(HTTPS,"HTTP_ERR_GET_FAILED");
					break;
				/*HTTP READ请求失败*/
				case HTTP_ERR_READ_FAILED:
					ESP_LOGE(HTTPS,"HTTP_ERR_READ_FAILED");
					break;
				/*HTTP 下载超时*/
				case HTTP_ERR_TIMEOUT:
					ESP_LOGE(HTTPS,"HTTP_ERR_TIMEOUT");
					break;
				/*下载的采集器固件设备类型不符合*/
				case HTTP_ERR_DEVICE_TYPE_FAILED:
					ucHTTPDownloadCtrlFlag = 0;		//退出下载
					uiHTTPErrCode = HTTP_CHECK_ERROR;
					ESP_LOGE(HTTPS,"HTTP_ERR_DEVICE_TYPE_FAILED");
					break;
				case HTTP_ERR_DL_FILE_CRC32_FAILED:
					uiHTTPErrCode = HTTP_CHECK_ERROR;
					ESP_LOGE(HTTPS,"HTTP_ERR_DL_FILE_CRC32_FAILED");
					break;
				default:
					ESP_LOGE(HTTPS,"Other errors");
					break;
			}

			if(++ucHTTPDownloadTimes >= 3)	//下载超过3次,退出下载
			{
				ucHTTPDownloadCtrlFlag = 0; //退出下载
			}

		}

		if((1 == ucHTTPDownloadSuccessedFlag) && (1 != iot_Bsdiff_Update_Judge_uc(0xff)))		//判断是否需要进行差分升级
		{
			//成功
			IOT_UpdateProgress_record(HTTP_PROGRESS , 100);         //保存进度 100
			IOT_MasterFotaStatus_SetAndGet_ui(1 , EFOTA_FINISHING);	//结束升级
		}else
		{
			if(0 == ucHTTPDownloadCtrlFlag)		//下载结束,退出
			{
				IOT_SystemParameterSet(10,"0", 1);  //保存数据
				IOT_SystemParameterSet(36,"0", 1);  //保存数据
				IOT_SystemParameterSet(44,"0", 1);  //保存数据

				IOT_UpdateProgress_record(HTTP_PROGRESS, uiHTTPErrCode);	//错误码
				IOT_MasterFotaStatus_SetAndGet_ui(1 , EFOTA_FINISHING);	    //结束升级
			}
		}

		vTaskDelay(100 / portTICK_PERIOD_MS);
	}
	//IOT_WaitOTAback();
# if 0
	const esp_partition_t *update_partition = NULL;
	const esp_partition_t *configured = esp_ota_get_boot_partition();
	const esp_partition_t *running = esp_ota_get_running_partition();

	esp_ota_handle_t update_handle = 0 ; 							 // ！！！升级完成后必须释放该开辟的内存空间
    esp_http_client_handle_t client;

    #define HTTP_READ_SINGLE_LEN_MAX 1024
    char *pcHttpFotaReadDataBuf = malloc(HTTP_READ_SINGLE_LEN_MAX+1);// 创建http 升级文件字节读取缓存

    unsigned int uiSocUpdatePartitionAddr = 0; 						 // 升级文件soc-flash起始地址
    unsigned int uiHttpReadDataLen = 0;
    ESP_LOGE(HTTPS, "Starting IOT_FotaHttpFile_Get_uc()");
	if (configured != running)  //获取地址
	{
		ESP_LOGE(HTTPS, "Configured OTA boot partition at offset 0x%08x, but running from offset 0x%08x \r\n",
			  configured->address, running->address);
		ESP_LOGE(HTTPS, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)\r\n");
	}
	ESP_LOGE(HTTPS, "Running partition type %d subtype %d (offset 0x%08x)\r\n",
	running->type, running->subtype, running->address);

	update_partition = esp_ota_get_next_update_partition(NULL);
	ESP_LOGE(HTTPS, "Writing to partition subtype %d at offset 0x%x \r\n",
				update_partition->subtype, update_partition->address);
	g_FotaParams.uiFotaFileStartAddr = update_partition->address; // 得到存放升级文件soc-flash起始地址
	assert(update_partition != NULL);

    uiSocUpdatePartitionAddr = g_FotaParams.uiFotaFileStartAddr; 	// 得到升级起始地址

    update_partition = esp_ota_get_next_update_partition(NULL);  	// 获取soc 升级flash地址信息
		/* 初始化http参数  url / 证书 / 超时等 */
		esp_http_client_config_t config ={
				.url = "https://cdn.growatt.com",          // 动态服务器下发设置
				.cert_pem = (char *)server_cert_pem_start, // 固定放到编译文件中保存，也可以做到动态服务器下发设置
				.timeout_ms = 5000,                        // 网络超时时间 1000 = 1s
				.keep_alive_enable = true,                 // http 保持在线使能
		};

		ESP_LOGE(HTTPS, "sys heap_size = %d !!!", esp_get_minimum_free_heap_size());

		client = esp_http_client_init(&config); 		   // 初始化http服务器 -- close关闭连接前只需调用一次
		if (client == NULL)
		{
			ESP_LOGE(HTTPS, "Failed to initialise HTTP connection");
			task_fatal_error();							   // http初始化失败，直接退出升级
		}

		err = esp_http_client_set_url(client, HTTPS_FOTA_URL); // 设置http升级文件请求路径
		if (err != ESP_OK)
		{
			ESP_LOGE(HTTPS, "esp_http_client_set_url Failed %s", esp_err_to_name(err));
			esp_http_client_cleanup(client);
			task_fatal_error();
		}

		#define HTTP_READ_OVER_TIME_MAX 10  // 读取溢出次数
		unsigned char ucHttpReadDataOverTimes = 0;
		char *pcHttpHeader = NULL; // 请求头
	    #define HEADER_STRING "bytes=0-19\r\n"  // 设置下载前20字节

	while(1)
    {
		if(1)
		{
			/* 下载文件前，首先先下载升级文件前20字节头，用来校验文件和断点续传校验用 */
			err = esp_http_client_set_header(client, "Range", (char*)HEADER_STRING);
			ESP_LOGE(HTTPS, "set_header: %d", err);
			err = esp_http_client_get_header(client, "Range", &pcHttpHeader);
			ESP_LOGE(HTTPS, "esp_http_client_get_header: %d - %s", err, pcHttpHeader);

			err = esp_http_client_open(client, 0); // 开启http请求
			if (err != ESP_OK)
			{
				ESP_LOGE(HTTPS, "Failed to open HTTP connection: %s", esp_err_to_name(err));
				esp_http_client_cleanup(client);
				task_fatal_error();
			}
			esp_http_client_fetch_headers(client);// http请求头过滤设置

			ESP_LOGE(HTTPS, "https status = %d , content_length: %d", esp_http_client_get_status_code(client),
													  esp_http_client_get_content_length(client));
			while(1)
			{
				uiHttpReadDataLen += esp_http_client_read(client, pcHttpFotaReadDataBuf, 20); // 读升级包前20字节
				ESP_LOGE(HTTPS, "esp_http_client_read: %d,", uiHttpReadDataLen);
				if(uiHttpReadDataLen == 20)
				{
					IOT_Fota_Init();  // 升级初始化
					IOT_FotaFile_Check_c(0, (unsigned char*)pcHttpFotaReadDataBuf);
					uiHttpReadDataLen = 0;
					ucHttpReadDataOverTimes = 0;
					break;
				}
				else if((uiHttpReadDataLen > 5) || (ucHttpReadDataOverTimes > HTTP_READ_OVER_TIME_MAX))
				{
					http_cleanup(client);
					task_fatal_error(); // http初始化失败，直接退出升级
				}
				ucHttpReadDataOverTimes++;
			}
			esp_http_client_close(client);
			esp_http_client_delete_header(client, "Range");
		}
		ucHttpReadDataOverTimes = 0;
		if(1)
		{
			/*下载升级文件body*/
		 	pcHttpHeader = NULL;
			char cFotaBodyRangeStr[30] = {0};
			unsigned int ucBodyRangeStartOffset = 20 + g_FotaParams.uiPointSize;// 读取升级文件数据断点值偏移起始地址  --和断点保存值对应关联

			sprintf(cFotaBodyRangeStr, "%s%d%s", "bytes=", ucBodyRangeStartOffset, "-\r\n");
			err = esp_http_client_set_header(client, "Range", cFotaBodyRangeStr);
		 	ESP_LOGE(HTTPS, "esp_http_client_set_header: %d", err);
			err = esp_http_client_get_header(client, "Range", &pcHttpHeader);
		 	ESP_LOGE(HTTPS, "esp_http_client_get_header: %d - %s", err, pcHttpHeader);
			err = esp_http_client_open(client, 0);
			if (err != ESP_OK)
			{
		 		ESP_LOGE(HTTPS, "Failed to open HTTP connection: %s", esp_err_to_name(err));
				esp_http_client_cleanup(client);
				task_fatal_error();
			}
			esp_http_client_fetch_headers(client);// http请求头过滤
		 	ESP_LOGE(HTTPS, "https status = %d , content_length: %d", esp_http_client_get_status_code(client),															  esp_http_client_get_content_length(client));
		}

		int iHttpFotaSingleRevLen = 0;                           // http升级数据单次接收到的字节长度
		int iHttpFotaReadingSingleLen = HTTP_READ_SINGLE_LEN_MAX;// http升级数据单次读取目标字节长度(最大字节数：1024，可能动态变化)
		int iHttpFotaReadSingleMaxLen = HTTP_READ_SINGLE_LEN_MAX;// http升级数据单次读取最大目标字节长度(默认字节数：1024)
		int iHttpFotaReadedSingleLen = 0;                        // http升级数据单次已读取目标字节长度(最大字节数：1024)
		int iHttpFotaReadedSumLen = g_FotaParams.uiPointSize;    // http升级数据总已读取目标字节长度(最大字节数：升级文件总长度)
		int iHttpFotaFileSumLen = 0;                             // http升级文件字节总长度(最大字节数：升级文件总长度)
		unsigned int uiHttpFotaDownLoadingSumLen = 0;            // 升级文件当前下载文件总长度，每read一次累加一次
		bool bFotaFileImageHeaderCheckedStatus = false;          // 升级文件头检验状态
		char cHttpFotaReadOverTimes = 0; // http数据读取溢出次数

		iHttpFotaFileSumLen = esp_http_client_get_content_length(client); // 得到升级文件总长度
		while (1)
		{
			iHttpFotaSingleRevLen = esp_http_client_read(client, pcHttpFotaReadDataBuf, iHttpFotaReadingSingleLen);
			ESP_LOGE(HTTPS, "esp_http_client_read: %d, ota_write_data=", iHttpFotaSingleRevLen);
//	        for(int i=0; i< BUFFSIZE; i++) // BUFFSIZE  // 调试用
//	        	IOT_Printf( "%02X", ota_write_data[i]);
//			while(1)
//			{
//				ESP_LOGE(HTTPS, "sys heap_size = %d !!!", esp_get_minimum_free_heap_size());
//				vTaskDelay(5000 / portTICK_RATE_MS);
//			}
			cHttpFotaReadOverTimes++;
			if (0|| iHttpFotaSingleRevLen < 0) // http数据读取失败， tcp异常
			{
				if(cHttpFotaReadOverTimes == 3) // 最大允许出错请求 3次
				{
					ESP_LOGE(HTTPS, "Error: SSL data read error = %d", iHttpFotaSingleRevLen);
					http_cleanup(client);
					break;
					task_fatal_error();
				}
			}
			else if (iHttpFotaSingleRevLen > 0) // http数据读取成功
			{
				cHttpFotaReadOverTimes = 0;
				if(IOT_FotaType_Manage_c(0xFF, 0) == EFOTA_TYPR_DATALOGER)// 采集器升级
				{
					if((bFotaFileImageHeaderCheckedStatus == false) && (!g_FotaParams.uiPointSize)) // 是否是采集器soc升级文件第一包字节数据-用来校验版本等信息
					{
						esp_app_desc_t new_app_info;
						if(iHttpFotaSingleRevLen > sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t))
						{
							/* 得到当前下载固件版本号 */
							memcpy(&new_app_info, &pcHttpFotaReadDataBuf[sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t)], sizeof(esp_app_desc_t));
							ESP_LOGE(HTTPS, "New firmware version: %s", new_app_info.version);
							/* 得到当前soc运行版本号 */
							const esp_partition_t *running = esp_ota_get_running_partition();
							esp_app_desc_t running_app_info;
							if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK)
							{
								ESP_LOGE(HTTPS, "Running firmware version: %s", running_app_info.version);
							}
							/* 得到当前有效soc运行版本号 */
							const esp_partition_t* last_invalid_app = esp_ota_get_last_invalid_partition();
							esp_app_desc_t invalid_app_info;
							if (esp_ota_get_partition_description(last_invalid_app, &invalid_app_info) == ESP_OK)
							{
								ESP_LOGE(HTTPS, "Last invalid firmware version: %s", invalid_app_info.version);
							}
							// check current version with last invalid partition
							if (last_invalid_app != NULL)
							{
								if (memcmp(invalid_app_info.version, new_app_info.version, sizeof(new_app_info.version)) == 0)
								{
									ESP_LOGW(HTTPS, "New version is the same as invalid version.");
									ESP_LOGW(HTTPS, "Previously, there was an attempt to launch the firmware with %s version, but it failed.", invalid_app_info.version);
									ESP_LOGW(HTTPS, "The firmware has been rolled back to the previous version.");
									http_cleanup(client);
									infinite_loop();
								}
							}

							/* 比较下载固件和当前升级固件版本号是否一致 */
							if(memcmp(new_app_info.version, running_app_info.version, sizeof(new_app_info.version)) == 0)
							{
								ESP_LOGW(HTTPS, "Current running version is the same as a new.");
							}

							err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);
							if(err != ESP_OK)
							{
								ESP_LOGE(HTTPS, "esp_ota_begin failed (%s)", esp_err_to_name(err));
								http_cleanup(client);
								esp_ota_abort(update_handle);
								task_fatal_error();
							}
							bFotaFileImageHeaderCheckedStatus = true; // 采集器soc升级文件头检验通过
							ESP_LOGI(HTTPS, "esp_ota_begin succeeded, datalog soc fota file downloading ");
						}
						else // 采集器soc 升级文件头长度下载不足，退出http文件下载
						{
							ESP_LOGE(HTTPS, "received package is not fit len");
							http_cleanup(client);
							esp_ota_abort(update_handle);
							task_fatal_error();
						}
					}
				}
				else if(IOT_FotaType_Manage_c(0xFF, 0) == EFOTA_TYPR_DEVICE)// 设备升级
				{
					ESP_LOGI(HTTPS, "esp_ota_begin succeeded, device fota file downloading ");
					// 设备升级操作
				}

				iHttpFotaReadedSingleLen += iHttpFotaSingleRevLen;   // http单次已读取字节数累加
				uiHttpFotaDownLoadingSumLen += iHttpFotaSingleRevLen;// http总在读取字节数累加

				ESP_LOGE(HTTPS, "iHttpFotaSingleRevLen       = %d", iHttpFotaSingleRevLen);
				ESP_LOGE(HTTPS, "iHttpFotaReadedSingleLen    = %d", iHttpFotaReadedSingleLen);
				ESP_LOGE(HTTPS, "uiHttpFotaDownLoadingSumLen = %d-%d", uiHttpFotaDownLoadingSumLen, iHttpFotaReadedSumLen);

				if((iHttpFotaReadedSingleLen == iHttpFotaReadSingleMaxLen) || \
				   (uiHttpFotaDownLoadingSumLen == iHttpFotaFileSumLen))// 单次http读取完成 1024byte / 总http字节读取完成
				{
					if((uiSocUpdatePartitionAddr+iHttpFotaReadedSumLen)%4096 == 0) // 整页数计算，并执行整页擦除  soc flash-4096byte/sector
					{
						err = spi_flash_erase_sector((uiSocUpdatePartitionAddr+iHttpFotaReadedSumLen)/4096); // flash-4096byte/sector
						ESP_LOGE(HTTPS, "spi_flash_erase_sector %d-%d", err, (uiSocUpdatePartitionAddr+iHttpFotaReadedSumLen)/4096);
					}
					err = spi_flash_write(uiSocUpdatePartitionAddr+iHttpFotaReadedSumLen, \
										  (const void *)pcHttpFotaReadDataBuf, \
										  iHttpFotaReadedSingleLen); 	// 升级文件数据写入 soc-flash
//		            err = esp_ota_write( update_handle, (const void *)ota_write_data, data_read);
//		            err = spi_flash_erase_sector((update_partition->address+binary_file_length)/4096);
//		            ESP_LOGI(HTTPS, "spi_flash_erase_sector %d-%d", err, (update_partition->address+binary_file_length)/4096);
//		            err = esp_ota_write_with_offset( update_handle, (const void *)ota_write_data, data_read,
//		            		                           update_partition->address+binary_file_length);
					if (err != ESP_OK) // soc-flash 升级数据写入错误，http关闭
					{
						http_cleanup(client);
						esp_ota_abort(update_handle);
						task_fatal_error();
					}

					g_FotaParams.uiPointSize += iHttpFotaReadedSingleLen;  // 断点数据下载累加
					if((g_FotaParams.uiPointSize) && (g_FotaParams.uiPointSize%(1024*4*10)) == 0) // 断点下载长度 每 40K 保存一次
					{
					//	ESP_LOGE(HTTPS, "*******GucIOTESPHTTPBreakPointSaveSize = %d",g_FotaParams.uiPointSize);
						IOT_FotaParams_Write_c(4, g_FotaParams.uiPointSize); // 记录HTTP断点下载保存数据长度：编号=4
					}

					iHttpFotaReadedSumLen += iHttpFotaReadedSingleLen;     // http总已读取字节数累加
					iHttpFotaReadingSingleLen = iHttpFotaReadSingleMaxLen; // 重新装载单次读取长度值 1024byte
					iHttpFotaReadedSingleLen = 0;                          // 单次读取完成长度值--清空

				//	ESP_LOGE(HTTPS, " iHttpFotaReadedSumLen = %d", iHttpFotaReadedSumLen);
				//	ESP_LOGE(HTTPS, " http fota file download : %d ",
				//					  ((iHttpFotaReadedSumLen*100)/(iHttpFotaFileSumLen+g_FotaParams.uiPointSize)));

					if(uiHttpFotaDownLoadingSumLen == iHttpFotaFileSumLen)// http fota文件下载完成
					{
						IOT_FotaParams_Write_c(4, 0); // 记录HTTP断点下载保存数据长度：编号=4
				//		ESP_LOGE(HTTPS, " http fota file download success, File sum size = %d", uiHttpFotaDownLoadingSumLen);
				//		ESP_LOGE(HTTPS, "Total read fota file sum length: %d", iHttpFotaReadedSumLen);
						goto GOTO_HTTP_READ_END;
					}

//					if(uiHttpFotaDownLoadingSumLen%(1024*5*10) == 0)esp_restart(); // 调试用-测试断点续传，没下载50k重启一次
				}
				else // 单次数据读取小于1024 byte, 继续请求直到请求满1024byte
				{
					iHttpFotaReadingSingleLen = iHttpFotaReadSingleMaxLen - iHttpFotaSingleRevLen;
				}
			}
			else if (iHttpFotaSingleRevLen == 0) // http read 数据字节个数为0
			{
			   /*
				* As esp_http_client_read never returns negative error code, we rely on
				* `errno` to check for underlying transport connectivity closure if any
				*/
				if (errno == ECONNRESET || errno == ENOTCONN)
				{
					ESP_LOGE(HTTPS, "Connection closed, errno = %d", errno);
					break;
				}
				if (esp_http_client_is_complete_data_received(client) == true) // http接收完成
				{
				//	ESP_LOGE(HTTPS, "Connection closed");
					break;
				}
			}
		}
    }

GOTO_HTTP_READ_END: // http 数据读取结束
    if (esp_http_client_is_complete_data_received(client) != true)
    {
     // ESP_LOGE(HTTPS, "Error in receiving complete file");
        http_cleanup(client);
        esp_ota_abort(update_handle);
        task_fatal_error();
    }

    free(pcHttpFotaReadDataBuf); // 释放http数据保存缓冲区

     /* 暂时不用 esp ota api接口函数写flash数据，这里end就会报错，但是不影响升级固件包校验完整性（原因待澄清），仍然可以执行boot跳转 */
//    err = esp_ota_end(update_handle);
//    if (err != ESP_OK) {
//        if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
//            ESP_LOGE(HTTPS, "Image validation failed, image is corrupted");
//        }
//        ESP_LOGE(HTTPS, "esp_ota_end failed (%s)!", esp_err_to_name(err));
//        http_cleanup(client);
//        task_fatal_error();
//    }

    if(IOT_FotaType_Manage_c(0xFF, 0) == EFOTA_TYPR_DATALOGER)// 采集器升级
    {
		err = esp_ota_set_boot_partition(update_partition);// 设置采集器soc升级新固件执行地址
		if (err != ESP_OK)
		{
			ESP_LOGE(HTTPS, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
			http_cleanup(client);
			task_fatal_error();
		}
		ESP_LOGE(HTTPS, "Prepare to restart system!");
		esp_restart();
    }
    else // 设备升级
    {
    	// 设备升级 -- 触发进行下一步升级数据下发到设备端操作
    }

    return;
#endif
}

#if 0
void app_main(void)
{
    uint8_t sha_256[HASH_LEN] = { 0 };
    esp_partition_t partition;

    // get sha256 digest for the partition table
    partition.address   = ESP_PARTITION_TABLE_OFFSET;
    partition.size      = ESP_PARTITION_TABLE_MAX_LEN;
    partition.type      = ESP_PARTITION_TYPE_DATA;
    esp_partition_get_sha256(&partition, sha_256);
    print_sha256(sha_256, "SHA-256 for the partition table: ");

    // get sha256 digest for bootloader
    partition.address   = ESP_BOOTLOADER_OFFSET;
    partition.size      = ESP_PARTITION_TABLE_OFFSET;
    partition.type      = ESP_PARTITION_TYPE_APP;
    esp_partition_get_sha256(&partition, sha_256);
    print_sha256(sha_256, "SHA-256 for bootloader: ");

    // get sha256 digest for running partition
    esp_partition_get_sha256(esp_ota_get_running_partition(), sha_256);
    print_sha256(sha_256, "SHA-256 for current firmware: ");

    // Initialize NVS.
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // OTA app partition table has a smaller NVS partition size than the non-OTA
        // partition table. This size mismatch may cause NVS initialization to fail.
        // If this happens, we erase NVS partition and initialize NVS again.
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
  //ESP_ERROR_CHECK(example_connect());
#if CONFIG_EXAMPLE_CONNECT_WIFI
    /* Ensure to disable any WiFi power save mode, this allows best throughput
     * and hence timings for overall OTA operation.
     */
    esp_wifi_set_ps(WIFI_PS_NONE);
#endif // CONFIG_EXAMPLE_CONNECT_WIFI
    xTaskCreate(&IOT_FotaHttpFile_Get_uc, "IOT_FotaHttpFile_Get_uc", 8192, NULL, 5, NULL);
}
#endif
#endif
