#include "iot_HTTP_update.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include <stdio.h>
#include <stdint.h>
#include "iot_ota.h"
#include "iot_bsdiff_api.h"
#include "iot_crc32.h"

#include "iot_spi_flash.h"
#include "esp_ota_ops.h"
#include "iot_uart.h"
#include "iot_fota.h"
#include "esp_spi_flash.h"

#include "iot_system.h"
#include "iot_mqtt.h"
#include "iot_timer.h"

#include "esp_crt_bundle.h"

#define HTTP_TAG "IOT_HTTP"
#define OTAReadBuffLen 1024
#define CONFIG_EXAMPLE_OTA_RECV_TIMEOUT 3000



esp_http_client_handle_t HTTPClienHandle = NULL;

//char CONFIG_FIRMWARE_UPG_URL[150] = {"http://cdn.growatt.com/update/device/New/shine4G-X2/OTA_TEST.patch"};
//uint8_t OTAReadBuff[OTAReadBuffLen + 1] = {0};



SOTAFileheaderINFO g_SOTAFileheader_t = { 0 };


extern FlASHTYPE IOT_OTA_Get_Save_Flash_Type_uc(void);

extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");




#define GET_OTA_TYPE(dest , src)   do{ \
		switch(src)\
		{ \
			case EFOTA_TYPR_DATALOGER:\
				dest = EUPDADTE_COLLACTOR; break; \
			case EFOTA_TYPR_DEVICE:  \
				dest = EUPDADTE_PORTABLE_POWER; break;\
			default: \
				dest = EUPDADTE_NULL; \
		}\
	}while(0);

#if 1
/******************************************************************************
 * FunctionName : IOT_OTA_HTTP_Init
 * Description  : OTA升级HTTP初始化
 * Parameters   : void
 * Returns      : 0 成功
 * 				 -1 失败
 * Notice       : none
*******************************************************************************/
int IOT_OTA_HTTP_Init_i(const char * URL)
{
	int err_t = ESP_FAIL;
	char cHTTPsURL[256] = {0};

	if(!((URL[4]  == 's') || (URL[4]  == 'S')))
	{
		sprintf(cHTTPsURL , "https:%s", &URL[5]);
	}else
	{
		memcpy(cHTTPsURL , URL , strlen((const char *)URL));
	}

	ESP_LOGE(HTTP_TAG,"cHTTPsURL = %s \r\n",cHTTPsURL);

	esp_http_client_config_t HTTPconfig = {0};

	HTTPconfig.url = cHTTPsURL;
	//HTTPconfig.cert_pem = (char *)server_cert_pem_start,
	HTTPconfig.crt_bundle_attach = esp_crt_bundle_attach;			//附着证书包
	HTTPconfig.timeout_ms = CONFIG_EXAMPLE_OTA_RECV_TIMEOUT;
	HTTPconfig.keep_alive_enable = true;
	//HTTPconfig.auth_type = HTTP_AUTH_TYPE_NONE;



	HTTPClienHandle =  esp_http_client_init(&HTTPconfig);


	if(HTTPClienHandle != NULL)
	{
		err_t = ESP_OK;
	}

	return err_t;
}

/******************************************************************************
 * FunctionName : IOT_HTTP_Set_Request_Header
 * Description  : 设置HTTP请求头
 * Parameters   : const char *Header_tem	HTTP请求头部字段名
 *				  const char *value_tem		请求的值
 * Returns      : ESP_OK 成功
 * 				  ESP_FAIL 失败
 * Notice       : none
*******************************************************************************/
int IOT_HTTP_Set_Request_Header_i(const char *Header_tem , const char *value_tem)
{
	int err_t = ESP_FAIL;

	err_t = esp_http_client_set_header(HTTPClienHandle ,Header_tem, value_tem);

	return err_t;
}
/******************************************************************************
 * FunctionName : IOT_HTTP_Client_Open
 * Description  : 打开HTTP连接,发送请求头
 * Parameters   : int32_t write_len	  HTTP Content length need to write to the server
 * Returns      : ESP_OK 成功
 * 				  ESP_FAIL 失败
 * Notice       : none
*******************************************************************************/
int IOT_HTTP_Client_Open_i(int32_t write_len)
{
	int err_t = ESP_FAIL;

	err_t = esp_http_client_open(HTTPClienHandle , write_len);

	return err_t;
}

/******************************************************************************
 * FunctionName : IOT_client_fetch_headers
 * Description  : 过滤多余的接收头信息
 * Parameters   : void
 * Returns      :
 * 				  - (0) if stream doesn't contain content-length header, or chunked encoding (checked by `esp_http_client_is_chunked` response)
 *    			  - (-1: ESP_FAIL) if any errors
 *     			  - Download data length defined by content-length header
 * Notice       : none
*******************************************************************************/
static int IOT_client_fetch_headers_i(void)
{
    return esp_http_client_fetch_headers(HTTPClienHandle);
}


/******************************************************************************
 * FunctionName : IOT_HTTP_Rang_Get
 * Description  : 打开HTTP连接,发送请求头
 * Parameters   : int32_t write_len	  HTTP Content length need to write to the server
 * Returns      : ESP_OK 成功
 * 				  ESP_FAIL 失败
 * Notice       : none
*******************************************************************************/
int IOT_HTTP_Set_Range_GET_i(uint32_t GetStartAddr_tem,uint32_t GetLen_tem)
{
	int err_t = ESP_FAIL;
	int HTTPErrCode_t = 0;

	int content_length = 0;
	char cRequestValue[10] = {0};

	if(0 == GetLen_tem)
	{
		sprintf(cRequestValue,"bytes=%d-",GetStartAddr_tem);
	}
	else
	{
		sprintf(cRequestValue,"bytes=%d-%d",GetStartAddr_tem , GetStartAddr_tem + GetLen_tem -1);
	}

	ESP_LOGE(HTTP_TAG,"GET Range : %s \r\n",cRequestValue);

	err_t =  IOT_HTTP_Set_Request_Header_i("Range", (const char *)cRequestValue);
	ESP_LOGE(HTTP_TAG,"IOT_HTTP_Set_Request_Header ERR = %d \r\n",err_t);

	err_t = esp_http_client_open(HTTPClienHandle , 0);

	if(err_t == -1)
	{
		ESP_LOGE(HTTP_TAG,"esp_http_client_open ERR = %d \r\n",err_t);
		return err_t;
	}

	content_length = IOT_client_fetch_headers_i();
	HTTPErrCode_t = esp_http_client_get_status_code(HTTPClienHandle);


	ESP_LOGE(HTTP_TAG,"HTTP status_code  %d \r\n",HTTPErrCode_t);
	if(content_length <= 0)
	{
		ESP_LOGE(HTTP_TAG,"content_length  = %d \r\n",content_length);
		return ESP_FAIL;
	}

	err_t =  content_length;
	ESP_LOGI(HTTP_TAG,"content_length = %d\r\n",content_length);

	return err_t;
}


/******************************************************************************
 * FunctionName : IOT_HTTP_Client_Open
 * Description  : 打开HTTP连接,发送请求头
 * Parameters   : int32_t write_len	  HTTP Content length need to write to the server
 * Returns      :  - (-1) if any errors
 *     			   - Length of data was read
 * Notice       : none
*******************************************************************************/
int IOT_HTTP_Client_Read_i(uint8_t *Recvbuffer_tem, int BuffLen_tem)
{
	int read_len = ESP_FAIL;

	read_len =  esp_http_client_read(HTTPClienHandle, (char *)Recvbuffer_tem, BuffLen_tem);

	return read_len;
}


/******************************************************************************
 * FunctionName : IOT_http_cleanup
 * Description  : 关闭HTTP连接
 * Parameters   : void
 * Returns      : void
 * Notice       : none
*******************************************************************************/
static void IOT_http_cleanup(void)
{
    esp_http_client_close(HTTPClienHandle);
    esp_http_client_cleanup(HTTPClienHandle);

    HTTPClienHandle = NULL;	//指向空,防止野指针
}

#else
#include "netdb.h"

char gcHeaderKey[200] = {0};
char gcReqHeader[200] = {0};

#define WEB_SERVER "cdn.growatt.com"
#define WEB_PORT "80"
#define WEB_PATH "http://cdn.growatt.com/update/device/datalog/ShineWiFi-X2/3.2.0.1/3200.bin "

static const char *TAG = "example";

static const char *REQUEST = "GET " WEB_PATH " HTTP/1.0\r\n"
    "Host: "WEB_SERVER":"WEB_PORT"\r\n"
    "User-Agent: esp-idf/1.0 esp32\r\n"
	"Range: bytes=0-20\r\n"
    "\r\n";


const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };
struct addrinfo *res;
struct in_addr *addr;
int giSocketFd = 0;


/******************************************************************************
 * FunctionName : IOT_OTA_HTTP_Init
 * Description  : OTA升级HTTP初始化
 * Parameters   : void
 * Returns      : 0 成功
 * 				 -1 失败
 * Notice       : none
*******************************************************************************/
int IOT_OTA_HTTP_Init_i(const char * URL)
{
	int err_t = ESP_FAIL;

	err_t = getaddrinfo(WEB_SERVER, WEB_PORT, &hints, &res);

	if(err_t != 0 || res == NULL)
	{
		ESP_LOGE(TAG, "DNS lookup failed err=%d res=%p", err_t, res);
	}


	addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
	ESP_LOGI(TAG, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

	giSocketFd = socket(res->ai_family, res->ai_socktype, 0);
	if(giSocketFd < 0)
	{
		ESP_LOGE(TAG, "... Failed to allocate socket.");
		freeaddrinfo(res);

		return ESP_FAIL;
	}
	ESP_LOGI(TAG, "... allocated socket");

	if(connect(giSocketFd, res->ai_addr, res->ai_addrlen) != 0)
	{
		ESP_LOGE(TAG, "... socket connect failed errno=%d", errno);
		close(giSocketFd);
		freeaddrinfo(res);
		return ESP_FAIL;
	}

	ESP_LOGI(TAG, "... connected");
	freeaddrinfo(res);


	bzero(gcHeaderKey , sizeof(gcHeaderKey));
	sprintf(gcHeaderKey ,"GET  %s  HTTP/1.0\r\nHost: %s:%s\r\nUser-Agent: esp-idf/1.0 esp32\r\n",URL , "cdn.growatt.com" , "80");

	ESP_LOGI(TAG, "gcHeaderKey = %s",gcHeaderKey);
	return err_t;
}

/******************************************************************************
 * FunctionName : IOT_HTTP_Set_Request_Header
 * Description  : 设置HTTP请求头
 * Parameters   : const char *Header_tem	HTTP请求头部字段名
 *				  const char *value_tem		请求的值
 * Returns      : ESP_OK 成功
 * 				  ESP_FAIL 失败
 * Notice       : none
*******************************************************************************/
int IOT_HTTP_Set_Request_Header_i(const char *Header_tem , const char *value_tem)
{
	int err_t = ESP_OK;
	char header[20] = {0};

	sprintf(header , "%s: %s\r\n",Header_tem,value_tem);
	bzero(gcReqHeader , sizeof(gcReqHeader));
	sprintf(gcReqHeader , "%s%s\r\n",gcHeaderKey ,header);

	ESP_LOGI(TAG, "gcReqHeader = %s",gcReqHeader);
	return err_t;
}
/******************************************************************************
 * FunctionName : IOT_HTTP_Client_Open
 * Description  : 打开HTTP连接,发送请求头
 * Parameters   : int32_t write_len	  HTTP Content length need to write to the server
 * Returns      : ESP_OK 成功
 * 				  ESP_FAIL 失败
 * Notice       : none
*******************************************************************************/
int IOT_HTTP_Client_Open_i(char *ReqHeader,int32_t write_len)
{
	int err_t = ESP_FAIL;

	if (write(giSocketFd, ReqHeader, write_len) < 0)
	{
		ESP_LOGE(TAG, "... socket send failed");
		return err_t;
	}
	ESP_LOGI(TAG, "... socket send success");;

	struct timeval receiving_timeout;
	receiving_timeout.tv_sec = 5;
	receiving_timeout.tv_usec = 0;
	if (setsockopt(giSocketFd, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
			sizeof(receiving_timeout)) < 0)
	{
		ESP_LOGE(TAG, "... failed to set socket receiving timeout");
		return err_t;
	}


	err_t = ESP_OK;
	return err_t;
}

/******************************************************************************
 * FunctionName : IOT_client_fetch_headers
 * Description  : 过滤多余的接收头信息
 * Parameters   : void
 * Returns      :
 * 				  - (0) if stream doesn't contain content-length header, or chunked encoding (checked by `esp_http_client_is_chunked` response)
 *    			  - (-1: ESP_FAIL) if any errors
 *     			  - Download data length defined by content-length header
 * Notice       : none
*******************************************************************************/
static int IOT_client_fetch_headers_i(void)
{
	uint32_t ContentLength = 0;

	return ContentLength;
}


/******************************************************************************
 * FunctionName : IOT_HTTP_Rang_Get
 * Description  : 打开HTTP连接,发送请求头
 * Parameters   : int32_t write_len	  HTTP Content length need to write to the server
 * Returns      : ESP_OK 成功
 * 				  ESP_FAIL 失败
 * Notice       : none
*******************************************************************************/
int IOT_HTTP_Set_Range_GET_i(uint32_t GetStartAddr_tem,uint32_t GetLen_tem)
{
	int err_t = ESP_FAIL;
	//int HTTPErrCode_t = 0;

	int content_length = 0;
	char cRequestValue[10] = {0};

	if(0 == GetLen_tem)
	{
		sprintf(cRequestValue,"bytes=%d-",GetStartAddr_tem);
	}
	else
	{
		sprintf(cRequestValue,"bytes=%d-%d",GetStartAddr_tem , GetStartAddr_tem + GetLen_tem -1);
	}

	ESP_LOGE(HTTP_TAG,"GET Range : %s \r\n",cRequestValue);

	err_t =  IOT_HTTP_Set_Request_Header_i("Range", (const char *)cRequestValue);
	ESP_LOGE(HTTP_TAG,"IOT_HTTP_Set_Request_Header ERR = %d \r\n",err_t);

	err_t = IOT_HTTP_Client_Open_i(REQUEST , strlen(REQUEST));

	if(err_t == -1)
	{
		ESP_LOGE(HTTP_TAG,"esp_http_client_open ERR = %d \r\n",err_t);
		return err_t;
	}

	content_length = IOT_client_fetch_headers_i();


	//ESP_LOGE(HTTP_TAG,"HTTP status_code  %d \r\n",HTTPErrCode_t);
	if(content_length <= 0)
	{
		ESP_LOGE(HTTP_TAG,"content_length  = %d \r\n",content_length);
		return ESP_FAIL;
	}

	err_t =  content_length;
	ESP_LOGI(HTTP_TAG,"content_length = %d\r\n",content_length);

	return err_t;
}


/******************************************************************************
 * FunctionName : IOT_HTTP_Client_Open
 * Description  : 打开HTTP连接,发送请求头
 * Parameters   : int32_t write_len	  HTTP Content length need to write to the server
 * Returns      :  - (-1) if any errors
 *     			   - Length of data was read
 * Notice       : none
*******************************************************************************/
int IOT_HTTP_Client_Read_i(uint8_t *Recvbuffer_tem, int BuffLen_tem)
{
	int read_len = ESP_FAIL;

	read_len =   read(giSocketFd, Recvbuffer_tem, BuffLen_tem);

	ESP_LOGI(HTTP_TAG,"read len= %d\r\n",read_len);

	for(int i = 0 ;i <  BuffLen_tem;  i++)
	{
		IOT_Printf("%02x",Recvbuffer_tem[i]);
	}

	return read_len;
}


/******************************************************************************
 * FunctionName : IOT_http_cleanup
 * Description  : 关闭HTTP连接
 * Parameters   : void
 * Returns      : void
 * Notice       : none
*******************************************************************************/
static void IOT_http_cleanup(void)
{
	close(giSocketFd);
}
#endif
/******************************************************************************
 * FunctionName : IOT_http_Set_BreakPoint
 * Description  : 设置HTTP请求断点位置
 * Parameters   : uint32_t BreakPointAddr_tem 断点位置
 * Returns      : void
 * Notice       : none
*******************************************************************************/

uint32_t BreakPointAddr = 0;

static void IOT_http_Set_BreakPoint(uint32_t BreakPointAddr_tem)
{
	BreakPointAddr = BreakPointAddr_tem;

}
/******************************************************************************
 * FunctionName : IOT_http_Get_BreakPoint
 * Description  : 获取HTTP请求断点位置
 * Parameters   : uint32_t BreakPointAddr_tem 断点位置
 * Returns      : void
 * Notice       : none
*******************************************************************************/
static uint32_t IOT_http_Get_BreakPoint_ui(void)
{
	return BreakPointAddr;
}



/******************************************************************************
 * FunctionName : IOT_HTTP_Dowmload_Timeout_Judge_uc
 * Description  : http下载超时判断
 * Parameters   : uint16_t uiUpdateProgress 进度
 * Returns      : void
 * Notice       : none
*******************************************************************************/
uint8_t IOT_HTTP_Dowmload_Timeout_Judge_uc(uint8_t ucMode_tem , uint32_t uiThresholdValue_tem)
{
	static uint32_t uiTimeThreshold = 0;
	static long long llRecTime = 0;

	uint8_t err = 1;

	if(1 == ucMode_tem)
	{
		uiTimeThreshold = uiThresholdValue_tem;
		llRecTime = IOT_GetTick();

	}else if(0xff == ucMode_tem)
	{
		if((IOT_GetTick() - llRecTime) >= uiTimeThreshold)
		{
			err = 0;
		}
	}

	return err;
}


/******************************************************************************
 * FunctionName : IOT_HTTP_OTA
 * Description  : HTTP下载
 * Parameters   : const char *cURL		//下载文件的URL网址
 * 				  char cOTAType_tem		//升级的类型
 * 				  bool is_Bsdiff_tem	//是否进行差分升级
 *
 * Returns      :   HTTP_SUCCESSED						HTTP下载成功
					HTTP_ERR_INIT_FAILED 				初始化失败
					HTTP_ERR_GET_HEADER_FAILED  		获取文件头失败
					HTTP_ERR_GET_FAILED  				HTTP GET请求失败
					HTTP_ERR_READ_FAILED  				HTTP READ请求失败
					HTTP_ERR_TIMEOUT 				  	下载超时
					HTTP_ERR_DL_FILE_CRC32_FAILED		下载文件CRC校验错误
					HTTP_ERR_BSDIFF_CRC32_FAILED		差分文件CRC校验错误 （无效）
					HTTP_ERR_DEVICE_TYPE_FAILED 		设备类型校验错误

 * Notice       : none
*******************************************************************************/
int IOT_HTTP_OTA_i(const char *cURL,char cOTAType_tem , bool is_Bsdiff_tem)
{
	char *HTTP_OTA = "HTTP_OTA";
	int iOTAERR_t = HTTP_SUCCESSED;
	static int s_iERR_TIMES = 0;

	static bool s_bInitflag = false;
	static bool s_bReadHeaderflag = false;
	//static bool bBreakPoint	= false;

	uint32_t uiFileLen = 0;
	uint32_t uiFileCRC = 0;
	uint32_t uiGETLen = 0;
	uint32_t uiGETStartAddr = 0;
	uint32_t iREADLen = 0;
	uint32_t uiReadAddr = 0;
	uint32_t uiBreakPointAddr = 0;	//断点位置

	UPDADTE_TYPE_t OTA_TYPE = EUPDADTE_NULL;
	uint8_t *pucReadBuf = NULL;
	uint8_t ucReDownloadFlag = 0;	//为 1 时,不需要重新下载

	#define DOWMLOAD_TIMEOUT_VALUE (10 * 60 * 1000)		//下载超时时间 10分钟

	IOT_HTTP_Dowmload_Timeout_Judge_uc(1 , DOWMLOAD_TIMEOUT_VALUE);	//设置超时时间

	/* 初始化 */
	if(ESP_OK ==  IOT_OTA_HTTP_Init_i(cURL))		//初始化HTTP
	{
		GET_OTA_TYPE(OTA_TYPE,cOTAType_tem);		//获取升级的类型
		if(ESP_OK ==  IOT_OTA_Init_uc(OTA_TYPE , is_Bsdiff_tem))    //初始化OTA分区信息
		{
			s_bInitflag = true;
			pucReadBuf = (uint8_t *)malloc(HTTP_READ_MAX_SIZE);	//
		}
	}
	if((true != s_bInitflag) || (NULL == pucReadBuf))
	{
		iOTAERR_t = HTTP_ERR_INIT_FAILED;
		goto INIT_FAIL;
	}

	/* 获取文件头 */
	uiGETLen = IOT_HTTP_Set_Range_GET_i(0,20);	//设置第一次GET请求的范围,获取文件前20字节文件头
	ESP_LOGI(HTTP_OTA,"GET HEADER LEN  = %d",uiGETLen);
	if(uiGETLen == 20)
	{
		iREADLen = IOT_HTTP_Client_Read_i(pucReadBuf, 20);	//读取HTTP 数据
		if(iREADLen == 20)
		{
			if(0 == IOT_FotaFile_HEAD_Check_c(pucReadBuf))	//检查文件头
			{
				//flash中保存有完整的文件,无需重新下载
				ucReDownloadFlag = 1;
				goto CHECK_File;
			}
			s_bReadHeaderflag = true;			//标记为已获得前20字节文件头
		}
	}
	if(true != s_bReadHeaderflag)		//未能获取文件头，下载失败
	{
		iOTAERR_t = HTTP_ERR_GET_HEADER_FAILED;
		goto GET_FIlE_HEADER_FAIL;
	}

	/* 获取剩下的固件数据 */

	/* 这里可添加下载断点获取比较函数段 */

	/***********************************/

	uiBreakPointAddr = IOT_Get_OTA_breakpoint_ui();		//获取下载的断点
	uiFileLen = IOT_Get_OTA_FileSize_ui();				//获取本次下载文件的长度
	uiFileCRC = IOT_Get_OTA_FileCRC32Val_ui();			//获取本次下载的文件的CRC32校验码
	uiGETStartAddr =  uiBreakPointAddr + 20;			//设置GET请求的起始地址，跳过20字节文件头


	if(  (uiGETStartAddr >= uiFileLen + 20))	//断点超过文件大小,重新下载
	{
		uiGETStartAddr = 20;
		uiBreakPointAddr = 0;  //清断点

	}

	ESP_LOGI(HTTP_OTA,"uiBreakPointAddr  = %d",uiBreakPointAddr);

	uiGETLen = IOT_HTTP_Set_Range_GET_i(uiGETStartAddr , 0);	//从断点处开始获取剩余的固件数据

	ESP_LOGI(HTTP_OTA,"GET FLIE LEN  = %d",uiGETLen);

	if(uiFileLen != (uiBreakPointAddr + uiGETLen))		//获取的数据长度有误，退出下载
	{
		iOTAERR_t = HTTP_ERR_GET_FAILED;
		goto GET_FAIL;
	}

	uiReadAddr = uiBreakPointAddr ;					 //得到读取数据的起始地址
	IOT_OTA_Set_Written_Offset_uc(uiBreakPointAddr); //设置保存数据的位置偏移


	bool ReadLoop = true;		//循环读取 true
	do
	{
		iREADLen = IOT_HTTP_Client_Read_i(pucReadBuf, HTTP_READ_MAX_SIZE);

		ESP_LOGI(HTTP_OTA,"READ LEN = %d",iREADLen);

		if(iREADLen != HTTP_READ_MAX_SIZE)	//校验读取的数据长度
		{
			if(uiFileLen != (uiReadAddr + iREADLen)) //判断是否读取完整个文件
			{
				iOTAERR_t = HTTP_ERR_READ_FAILED;
				goto READ_FAIL;
			}
		}


		ESP_LOGI(HTTP_OTA,"uiReadAddr = %d",uiReadAddr);

		if(ESP_OK != IOT_OTA_Write_uc((const uint8_t*)pucReadBuf , iREADLen))	//保存下载的数据
		{
			iOTAERR_t = HTTP_ERR_READ_FAILED;
			goto READ_FAIL;
		}


		if(uiFileLen == (uiReadAddr + iREADLen))
		{
			ReadLoop = false;		//下载完成,退出
			uiBreakPointAddr = uiReadAddr + iREADLen;
		}
		else
		{
			uiReadAddr += iREADLen;
			uiBreakPointAddr = uiReadAddr;  //断点位置更新
		}



		if((uiBreakPointAddr != 0) && (uiBreakPointAddr % (40 * 1024) == 0))		//每40K保存一次断点
		{
			IOT_Set_OTA_breakpoint_ui(uiBreakPointAddr); //保存断点位置

			//更新升级进度
			uint8_t ucDLProgress = (uiBreakPointAddr * 100) / g_FotaParams.uiFileSize;
			if(ucDLProgress >= 100)
			{
				ucDLProgress = 99;   //最后1% 需要校验完成在更新
			}

			IOT_UpdateProgress_record(HTTP_PROGRESS , ucDLProgress);  //保存进度
		}



		if(0 == IOT_HTTP_Dowmload_Timeout_Judge_uc(0xff , 0))  //判断下载是否超时
		{
			iOTAERR_t = HTTP_ERR_TIMEOUT;
			goto READ_FAIL;
		}

		vTaskDelay(10/portTICK_PERIOD_MS);			//增加延时，切换任务，避免触发看门狗超时
	}while(ReadLoop);


CHECK_File :
	if((1 == ucReDownloadFlag) || (true == IOT_FotaFile_Integrality_Check_b(1)))	//校验下载后的文件
	{
		if(EFOTA_TYPR_DATALOGER == cOTAType_tem)	//判断是否为采集器的文件
		{
			if(true == is_Bsdiff_tem)	//判断是否为差分升级的文件
			{
				iot_Bsdiff_Update_Judge_uc(1);		//设置稍后进行解压补丁包
			}else
			{
				if(true == IOT_FotaFile_DeviceType_Check_b())	//校验升级文件的设备类型
				{
					IOT_OTA_Set_Boot();		//设置下载文件的分区为启动分区
				}else
				{
					iOTAERR_t = HTTP_ERR_DEVICE_TYPE_FAILED;
					goto CHECK_FILE_FAILED;
				}
			}

		}

		if(1 != ucReDownloadFlag)
		{
			IOT_Set_OTA_breakpoint_ui(uiBreakPointAddr);  // 保存断点
		}

	}
	else	//校验失败
	{
		IOT_Set_OTA_breakpoint_ui(0); 	//断点位置 清0
		iOTAERR_t = HTTP_ERR_DL_FILE_CRC32_FAILED;
		goto CHECK_FILE_FAILED;
	}



#if 0	//搬运文件到外部flash测试
	uint8_t temp[1024] = {0};
	uint32_t len = 0;

	IOT_Printf("flash test \r\n");
	SPI_FLASH_WriteBuff((uint8_t*)"OTA_TAST", 0x20000 ,9);
	W25QXX_Read(temp,0x20000,9);
	ESP_LOGI(HTTP_OTA,"flash test  = %s",temp);

	//uint32_t fileSize = uiPatchFlieSize;
	uint32_t fileSize = 837984;

	for(int i = 0;i < fileSize ;)
	{
		if((i + 1024) > fileSize)
		{
			len = fileSize - i;
		}else len = 1024;

		spi_flash_read(uiOldFlieAddr + i, temp, len);
		SPI_FLASH_WriteBuff(temp, 0x20000 + i ,len);
		i += len;
	}

	CRC32_t = FLASH_GetCRC32(0x20000 , fileSize,1);
	IOT_Printf("\r\n  ***************************************~ \r\n");
	ESP_LOGI(HTTP_OTA,"OTA CRC32 NN = 0x%x",CRC32_t);

#endif




#if 0  //通过串口发送固件数据测试
	uint8_t temp2[1024] = {0};
	uint32_t len = 0;

	uint32_t fileSize = uiNewFlieSize;

	for(int i = 0;i < fileSize ;)
	{
		if((i + 1024) > fileSize)
		{
			len = fileSize - i;
		}else len = 1024;

		spi_flash_read( uiNewFlieAddr + i , temp2 ,len);
		IOT_UART1SendData(temp2,len);

		i += len;
	}

#endif


	iOTAERR_t = HTTP_SUCCESSED;		//HTTP下载文件成功

CHECK_FILE_FAILED:
READ_FAIL:
GET_FAIL:
	s_bReadHeaderflag = false;	//清除获取头标志
GET_FIlE_HEADER_FAIL:
	s_bInitflag = false;		//清除HTTP初始化标志
INIT_FAIL:

	if(NULL != pucReadBuf)
	{
		free(pucReadBuf);
	}
	IOT_http_cleanup();		//关闭HTTP连接
	IOT_OTA_Finish_uc();	//OTA结束

	ESP_LOGI(HTTP_OTA,"~************** HTTP DOWNLOAD ERR= %d ***************-",iOTAERR_t);
	return iOTAERR_t;
}









