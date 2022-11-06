#include "iot_ota.h"
#include "esp_log.h"
#include "string.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include <stdio.h>
#include <stdlib.h>

#include "iot_spi_flash.h"


#define DL_FEIL_TO_EXTERNAL_FLASH 	1
#ifdef	DL_FEIL_TO_EXTERNAL_FLASH
#define SAVE_FLIE_EXTERNAL_FLASH_ADDR  (uint32_t)(0x400000)   //下载文件保存到外部flash的地址
#endif

static const char *OTA_TAG = "ota";



#define OTA_assert(__e , re) do{\
	if(__e) (void)0 ;\
	else {ESP_LOGE(OTA_TAG,"%s Failed ,return ESP_FAIL!!", __func__);return (re);}\
	}while(0);



typedef struct{
	const esp_partition_t *Partition_boot;				//boot分区信息
	const esp_partition_t *Partition_running;				//正在的分区信息
	const esp_partition_t *Partition_update_partition;	//升级的分区信息
	const esp_partition_t *Partition_last_invalid_app;	//上一次升级未成功的分区信息

	esp_app_desc_t running_app_info;
	esp_app_desc_t new_app_info;
	esp_ota_handle_t update_handle ;

	FlASHTYPE OTAFlashType;			//Flash的类型
	uint32_t uiFileSaveAddr;		//下载文件的保存地址
	uint32_t uiFileSize;		//下载文件的大小
	uint32_t uiSavedOffset;		//已经保存数据的位置偏移

	UPDADTE_TYPE_t update_type;	//升级文件的类型


	bool image_header_is_need_to_check ;		//是否校验固件信息
	bool image_header_was_checked ;				//前200多个字节固件信息校验

}OTA_Parameter;

OTA_Parameter *GpOTAParam = NULL;

static uint8_t ucOTAInitFlag = 0;


#define CONFIG_EXAMPLE_SKIP_VERSION_CHECK 1




 static int IOT_Config_OTA_Partition(OTA_Parameter *OTA_Param_tem);
 static int IOT_OTA_Write_Data(OTA_Parameter *OTA_Param_tem , const uint8_t* data , size_t size);
 static int IOT_OTA_End(OTA_Parameter *OTA_Param_tem);


 /******************************************************************************
  * FunctionName : IOT_OTA_flash_erase_sector
  * Description  : 擦除一整个扇区
  * Parameters   : uint32_t addr 地址
  * Returns      : 0 成功
  * 				 -1 失败
  * Notice       : none
 *******************************************************************************/
 int IOT_OTA_flash_erase_sector(uint32_t addr)
 {
	 OTA_Parameter *pOTA_Param_t = GpOTAParam;

	 OTA_assert((pOTA_Param_t != NULL) , ESP_FAIL);

	 if(External_FLASH == pOTA_Param_t->OTAFlashType)
	 {
		 IOT_Mutex_W25QXX_Erase_Sector(addr);
		 return ESP_OK;
	 }
	 else
	 {
		 return spi_flash_erase_sector(addr);
	 }
 }

 /******************************************************************************
  * FunctionName : IOT_OTA_Flash_Write_uc
  * Description  : 写入数据到flash中
  * Parameters   : OTA_Parameter *OTA_Param_tem
  * Returns      : 0 成功
  * 				 -1 失败
  * Notice       : none
 *******************************************************************************/
 int IOT_OTA_Flash_Write_uc(uint32_t addr , uint8_t *src , uint32_t Len)
 {
	 OTA_Parameter *pOTA_Param_t = GpOTAParam;
	 OTA_assert((pOTA_Param_t != NULL) , ESP_FAIL);

	 if(External_FLASH == pOTA_Param_t->OTAFlashType)
	 {
		 IOT_Mutex_SPI_FLASH_WriteBuff(src , addr , Len);
		return ESP_OK;
	 }
	 else
	 {
		 return spi_flash_write(addr , src , Len);
	 }
 }

 /******************************************************************************
  * FunctionName : IOT_Config_OTA_Partition
  * Description  : 配置OTA分区参数
  * Parameters   : OTA_Parameter *OTA_Param_tem
  * 			   bool is_Bsdiff_tem
  * Returns      : 0 成功
  * 				 -1 失败
  * Notice       : none
 *******************************************************************************/
 int IOT_OTA_Init_uc(UPDADTE_TYPE_t OTA_TYPE_tem , bool is_Bsdiff_tem)
 {
	 int err_t = 0;

	 if((ucOTAInitFlag == 0) || (GpOTAParam == NULL))
	 {
		 GpOTAParam = (OTA_Parameter *)malloc(sizeof(OTA_Parameter));
	 }
	 OTA_Parameter *pOTA_Param_t = GpOTAParam;



	 if((OTA_TYPE_tem > EUPDADTE_NULL) && (OTA_TYPE_tem < EUPDADTE_TYPE_NUM) && (NULL != pOTA_Param_t))
	 {
		 memset(pOTA_Param_t, 0x00 , sizeof(OTA_Parameter));	//分配空间成功,将其初始化为0
		 pOTA_Param_t->update_type = OTA_TYPE_tem;				//升级类型
		 ESP_LOGI(OTA_TAG,"OTA UPDATE TYPE : %d",pOTA_Param_t->update_type);
	 }
	 else	//失败,退出
	 {
		 ESP_LOGE(OTA_TAG,"UPDATE TYPE SET FAIL , TYPE : %d", OTA_TYPE_tem);
		 ESP_LOGE(OTA_TAG,"GpOTAParam is NULL");

		 if(NULL != pOTA_Param_t)
		 {
			 free(GpOTAParam);
		 }else
		 {
			 ESP_LOGE(OTA_TAG,"GpOTAParam is NULL");
		 }

		 GpOTAParam = NULL;
		 return ESP_FAIL;
	 }

	 err_t = IOT_Config_OTA_Partition(pOTA_Param_t);	//初始化分区信息

	 if(ESP_FAIL != err_t)
	 {
		 ESP_LOGE(OTA_TAG,"IOT_OTA_Init_uc   is_Bsdiff_tem = %d\r\n",is_Bsdiff_tem);

		 ucOTAInitFlag = 1;  //初始化标志至1

		if(true == is_Bsdiff_tem)		//如果进行差分升级,就将下载文件保存到外部flash
		{
			GpOTAParam->OTAFlashType = External_FLASH;
			pOTA_Param_t->uiFileSaveAddr = SAVE_FLIE_EXTERNAL_FLASH_ADDR ; //下载文件保存到外部flash的地址;
		}else						//普通文件下载到内部flash
		{
			GpOTAParam->OTAFlashType = System_FLASH;
			pOTA_Param_t->uiFileSaveAddr = pOTA_Param_t->Partition_update_partition->address;
		}

	 }


	 return err_t;
 }




 /******************************************************************************
  * FunctionName : IOT_OTA_Set_Written_Offset_uc
  * Description  :设置保存数据的位置偏移
  * Parameters   : void
  * Returns      : size
  * Notice       : none
 *******************************************************************************/
void IOT_OTA_Set_Written_Offset_uc(uint32_t uiSavedOffset_tem)
 {
	 OTA_Parameter *pOTA_Param_t = GpOTAParam;

	 OTA_assert((pOTA_Param_t != NULL) , (void)0);

	 pOTA_Param_t->uiSavedOffset = uiSavedOffset_tem;

	 return (void)0;
 }


 /******************************************************************************
  * FunctionName : IOT_OTA_Get_Written_Size_uc
  * Description  : 获取已经保存数据的大小
  * Parameters   : void
  * Returns      : size
  * Notice       : none
 *******************************************************************************/
 uint32_t IOT_OTA_Get_Written_Size_uc(void)
 {
	 OTA_Parameter *pOTA_Param_t = GpOTAParam;
	 OTA_assert((pOTA_Param_t != NULL) , ESP_FAIL);

	 return  pOTA_Param_t->uiSavedOffset;
 }

 /******************************************************************************
   * FunctionName : IOT_OTA_Get_Running_app_addr_uc
   * Description  : 获取当前正在运行的app分区的地址
   * Parameters   : void
   * Returns      : addr
   * Notice       : none
  *******************************************************************************/
 uint32_t IOT_OTA_Get_Running_app_addr_uc(void)
{
	 OTA_Parameter *pOTA_Param_t = GpOTAParam;
	 OTA_assert((pOTA_Param_t != NULL) , ESP_FAIL);

	 return  pOTA_Param_t->Partition_running->address;
}

 /******************************************************************************
     * FunctionName : IOT_OTA_Get_Written_Size_uc
     * Description  : 获取当前正在升级的分区的地址
     * Parameters   : void
     * Returns      : addr
     * Notice       : none
    *******************************************************************************/
uint32_t IOT_OTA_Get_Update_Partition_addr_uc(void)
{
	 OTA_Parameter *pOTA_Param_t = GpOTAParam;
	 OTA_assert((pOTA_Param_t != NULL) , ESP_FAIL);
	 return  (uint32_t)(pOTA_Param_t->Partition_update_partition->address);
}

/******************************************************************************
    * FunctionName : IOT_OTA_Get_Save_File_addr_uc
    * Description  : 获取下载流程中保存文件的地址
    * Parameters   : void
    * Returns      : 保存文件的地址
    * Notice       : none
   *******************************************************************************/
uint32_t IOT_OTA_Get_Save_File_addr_uc(void)
{
	OTA_Parameter *pOTA_Param_t = GpOTAParam;
	OTA_assert((pOTA_Param_t != NULL) , ESP_FAIL);

	return  (uint32_t)(pOTA_Param_t->uiFileSaveAddr);
}

/******************************************************************************
    * FunctionName : IOT_OTA_Get_Save_Flash_Type_uc
    * Description  : 获取下载流程中保存文件的flash 类型
    * Parameters   : void
    * Returns      : 	External_FLASH  外置Flash
						System_FLASH	系统内置Flash
    * Notice       : none
   *******************************************************************************/
FlASHTYPE IOT_OTA_Get_Save_Flash_Type_uc(void)
{
	OTA_Parameter *pOTA_Param_t = GpOTAParam;
	OTA_assert((pOTA_Param_t != NULL) , NULL_FLASH);
	return  (pOTA_Param_t->OTAFlashType);
}

 /******************************************************************************
  * FunctionName : IOT_OTA_Write_uc
  * Description  : 写入数据到OTA分区
  * Parameters   : const uint8_t* data  升级数据
  * 			   size_t size			数据大小
  * Returns      : 0 成功
  * 				 -1 失败
  * Notice       : none
 *******************************************************************************/
 int IOT_OTA_Write_uc(const uint8_t* data , size_t size)
 {
	 OTA_Parameter *pOTA_Param_t = GpOTAParam;
	 OTA_assert((pOTA_Param_t != NULL) , ESP_FAIL);
	 return  IOT_OTA_Write_Data(pOTA_Param_t , data , size);
 }


 /******************************************************************************
  * FunctionName : IOT_OTA_Finish_uc
  * Description  : 升级结束,释放升级相关参数占用的内存
  * Parameters   : void
  * Returns      : 0 成功
  * 				 -1 失败
  * Notice       : none
 *******************************************************************************/
 int IOT_OTA_Finish_uc(void)
 {
	int err_t = ESP_FAIL;
	//uint32_t CRC32_t = 0;

	OTA_Parameter *OTA_Param_t = GpOTAParam;


	//CRC32_t = FLASH_GetCRC32(OTA_Param_t->Partition_update_partition->address , OTA_Param_t->uiSavedOffset);

	//ESP_LOGI(OTA_TAG,"update type = %d , CRC32 = 0x%x",OTA_Param_t->update_type,CRC32_t);

	if(NULL != OTA_Param_t)
	{
		err_t = IOT_OTA_End(OTA_Param_t);
		ucOTAInitFlag = 0;		//初始化标志 清0
		free(OTA_Param_t);
		GpOTAParam = NULL;
	}

	return err_t;
 }


/******************************************************************************
 * FunctionName : IOT_Config_OTA_Partition
 * Description  : 配置OTA分区参数
 * Parameters   : OTA_Parameter *OTA_Param_tem
 * Returns      : 0 成功
 * 				 -1 失败
 * Notice       : none
*******************************************************************************/
static int IOT_Config_OTA_Partition(OTA_Parameter *pOTA_Param_t)
{
	int err_t = 0;

	OTA_assert((pOTA_Param_t != NULL) , ESP_FAIL);

	pOTA_Param_t->Partition_boot = esp_ota_get_boot_partition();			//获取boot 分区信息
	pOTA_Param_t->Partition_running = esp_ota_get_running_partition();	//获取当前正在运行固件所在的分区信息
	pOTA_Param_t->Partition_last_invalid_app = esp_ota_get_last_invalid_partition(); //获取上次OTA升级后无效的分区信息
	pOTA_Param_t->Partition_update_partition = esp_ota_get_next_update_partition(NULL);

	ESP_LOGI(OTA_TAG, "boot_Addr = 0x%x",pOTA_Param_t->Partition_boot->address);
	ESP_LOGI(OTA_TAG, "running_Addr = 0x%x",pOTA_Param_t->Partition_running->address);

	if(pOTA_Param_t->Partition_last_invalid_app != NULL)
	ESP_LOGI(OTA_TAG, "last_invalid_Addr = 0x%x",pOTA_Param_t->Partition_last_invalid_app->address);
	ESP_LOGI(OTA_TAG, "update_partition_Addr = 0x%x",pOTA_Param_t->Partition_update_partition->address);


	esp_ota_get_partition_description(pOTA_Param_t->Partition_running, &pOTA_Param_t->running_app_info);

	err_t = esp_ota_begin(pOTA_Param_t->Partition_update_partition, OTA_WITH_SEQUENTIAL_WRITES, &(pOTA_Param_t->update_handle));
    if (err_t != ESP_OK) {
        ESP_LOGE(OTA_TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err_t));
        esp_ota_abort(pOTA_Param_t->update_handle);

    } else ESP_LOGI(OTA_TAG, "esp_ota_begin succeeded");

	return err_t;
}

/******************************************************************************
 * FunctionName : IOT_OTA_Write
 * Description  : 写入数据到OTA分区中
 * Parameters   : OTA_Parameter *OTA_Param_tem
 * Returns      : 0 成功
 * 				 -1 失败
 * Notice       : none
*******************************************************************************/

static int IOT_OTA_Write_Data(OTA_Parameter *OTA_Param_tem , const uint8_t* data , size_t size)
{
	int err_t = ESP_FAIL;
	uint32_t *uiFotaWriteOffset= &OTA_Param_tem->uiSavedOffset;
	const uint32_t uiSocUpdatePartitionAddr = OTA_Param_tem->uiFileSaveAddr;


	if((NULL == OTA_Param_tem) || (size <= 0))
	{
		ESP_LOGE(OTA_TAG, "OTA Config fail: OTA_Param_tem is NULL or size = %d" ,size);
		err_t = ESP_FAIL;
		return err_t;
	}

#define IMAGE_HEADER_CHECK 0	//检查固件信息

#if IMAGE_HEADER_CHECK
	else if((OTA_Param_tem->image_header_was_checked != true) && (OTA_Param_tem->image_header_is_need_to_check == true))
	{
		if (size > sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t))
		{
			// check current version with downloading
			memcpy(&OTA_Param_tem->new_app_info, &data[sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t)], sizeof(esp_app_desc_t));
			ESP_LOGI(OTA_TAG, "New firmware version: %s", OTA_Param_tem->new_app_info.version);

			esp_app_desc_t running_app_info;
			if (esp_ota_get_partition_description(OTA_Param_tem->Partition_running, &running_app_info) == ESP_OK) {
				ESP_LOGI(OTA_TAG, "Running firmware version: %s", running_app_info.version);
			}

			esp_app_desc_t invalid_app_info;
			if (esp_ota_get_partition_description(OTA_Param_tem->Partition_last_invalid_app, &invalid_app_info) == ESP_OK) {
				ESP_LOGI(OTA_TAG, "Last invalid firmware version: %s", invalid_app_info.version);
			}

			// check current version with last invalid partition
			if (OTA_Param_tem->Partition_last_invalid_app != NULL) {
				if (memcmp(invalid_app_info.version, OTA_Param_tem->new_app_info.version, sizeof(OTA_Param_tem->new_app_info.version)) == 0)
				{
					ESP_LOGW(OTA_TAG, "New version is the same as invalid version.");
					ESP_LOGW(OTA_TAG, "Previously, there was an attempt to launch the firmware with %s version, but it failed.", invalid_app_info.version);
					ESP_LOGW(OTA_TAG, "The firmware has been rolled back to the previous version.");
				}
			}
		#ifdef CONFIG_EXAMPLE_SKIP_VERSION_CHECK
			if (memcmp(OTA_Param_tem->new_app_info.version, running_app_info.version, sizeof(OTA_Param_tem->new_app_info.version)) == 0)
			{
				ESP_LOGW(OTA_TAG, "Current running version is the same as a new. We will not continue the update.");
			}
		#endif

			OTA_Param_tem->image_header_was_checked = true;
		}else
		{
			return ESP_FAIL;
		}
	}
#endif

	#if 0
	err_t = esp_ota_write(OTA_Param_tem->update_handle, data, size);
    if (err_t != ESP_OK) {
        esp_ota_abort(OTA_Param_tem->update_handle);
    }
	#else

	if(( uiSocUpdatePartitionAddr + *uiFotaWriteOffset) % 4096 == 0) // 整页数计算，并执行整页擦除  soc flash-4096byte/sector
	{
		err_t = IOT_OTA_flash_erase_sector((uiSocUpdatePartitionAddr + *uiFotaWriteOffset)/4096); // flash-4096byte/sector
		ESP_LOGE(OTA_TAG, "spi_flash_erase_sector %d-%d", err_t, (uiSocUpdatePartitionAddr + *uiFotaWriteOffset)/4096);
	}

	err_t = IOT_OTA_Flash_Write_uc(uiSocUpdatePartitionAddr + *uiFotaWriteOffset , (uint8_t *)data , size); 	// 升级文件数据写入 soc-flash
	#endif


	*uiFotaWriteOffset += size;

	return err_t;
}



/******************************************************************************
 * FunctionName : IOT_OTA_Set_Boot
 * Description  : 设置启动分区
 * Parameters   : void
 * Returns      : 0 成功
 * 				 -1 失败
 * Notice       : none
*******************************************************************************/
int IOT_OTA_Set_Boot(void)
{
	int err_t = ESP_FAIL;

	OTA_Parameter *pOTA_Param_t = GpOTAParam;
	OTA_assert((pOTA_Param_t != NULL) , ESP_FAIL);

	err_t = esp_ota_set_boot_partition(pOTA_Param_t->Partition_update_partition);
	if (err_t != ESP_OK)
	{
		ESP_LOGE(OTA_TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err_t));

	}

	return err_t;
}



/******************************************************************************
 * FunctionName : IOT_OTA_End
 * Description  : 完成OTA升级
 * Parameters   : OTA_Parameter *OTA_Param_tem
 * Returns      : 0 成功
 * 				 -1 失败
 * Notice       : none
*******************************************************************************/
static int IOT_OTA_End(OTA_Parameter *pOTA_Param_t)
{
	int err_t = ESP_FAIL;

	OTA_assert((pOTA_Param_t != NULL) , ESP_FAIL);
	err_t = esp_ota_end(pOTA_Param_t->update_handle);
    if (err_t != ESP_OK)
    {
        if (err_t == ESP_ERR_OTA_VALIDATE_FAILED) {
            ESP_LOGE(OTA_TAG, "Image validation failed, image is corrupted");
        }
        ESP_LOGE(OTA_TAG, "esp_ota_end failed (%s)!", esp_err_to_name(err_t));
    }


//    if(OTA_Param_tem->update_type == EUPDADTE_COLLACTOR)		//采集器升级
//    {
//		err_t = esp_ota_set_boot_partition(OTA_Param_tem->Partition_update_partition);
//		if (err_t != ESP_OK)
//		{
//			ESP_LOGE(OTA_TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err_t));
//
//		}
//		ESP_LOGI(OTA_TAG, "Prepare to restart system!");
//		esp_restart();
//    }
//
//    ESP_LOGI(OTA_TAG,"OTA Finish err_t = %d ",err_t);

	return err_t;
}







