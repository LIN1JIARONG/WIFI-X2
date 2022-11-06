/*
 * iot_gatts_table.c
 *
 *  Created on: 2022年1月25日
 *      Author: Administrator
 */

/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

/****************************************************************************
*
* This demo showcases creating a GATT database using a predefined attribute table.
* It acts as a GATT server and can send adv data, be connected by client.
* Run the gatt_client demo, the client demo will automatically connect to the gatt_server_service_table demo.
* Client demo will enable GATT server's notify after connection. The two devices will then exchange
* data.
*
****************************************************************************/


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"

#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_main.h"
//#include "gatts_table_creat_demo.h"
#include "esp_gatt_common_api.h"

#include "esp_blufi_api.h"
#include "esp_blufi.h"

#include "iot_gatts_table.h"
#include "iot_protocol.h"
#include "iot_inverter.h"
#include "iot_system.h"
#include "iot_station.h"

#include "esp_gatt_defs.h"

#include "iot_local_update.h"
#include "iot_Local_Protocol.h"

#define GATTS_TABLE_TAG "GATTS_TABLE_DEMO"

/* FreeRTOS event group to signal  */
static EventGroupHandle_t s_gatts_event_group;   		// FreeRTOS事件组信号
#define GATTS_WRITE_BIT 	BIT0   // 写事件   1
#define GATTS_READ_BIT      BIT1   // 读事件   2
#define GATTS_ON_BIT    	BIT2   // 开启事件  4
#define GATTS_OFF_BIT    	BIT3   // 关闭事件  8

//应用程序配置文件  索引 heart_rate_profile_tab
#define PROFILE_NUM                 1
#define PROFILE_APP_IDX             0
#define ESP_APP_ID                  0x55
#define SAMPLE_DEVICE_NAME          "BLE_GATTS_DEMO"
#define SVC_INST_ID                 0




/* The max length of characteristic value. When the GATT client performs a write or prepare write operation,
*  the data length must be less than GATTS_DEMO_CHAR_VAL_LEN_MAX.
*/

#define GATTS_DEMO_CHAR_VAL_LEN_MAX 500
#define PREPARE_BUF_MAX_SIZE        1024
#define CHAR_DECLARATION_SIZE       (sizeof(uint8_t))

#define ADV_CONFIG_FLAG             (1 << 0)
#define SCAN_RSP_CONFIG_FLAG        (1 << 1)

static uint8_t adv_config_done       = 0;

uint16_t heart_rate_handle_table[HRS_IDX_NB];

typedef struct {
    uint8_t                 *prepare_buf;
    int                     prepare_len;
} prepare_type_env_t;

#define  GATTS_RCV_VALUE_BUFLEN  1536
#define  GATTS_ACK_VALUE_BUFLEN  1024
uint8_t Gatts_valuebuf[GATTS_RCV_VALUE_BUFLEN];
uint8_t Gatts_ackbuf[GATTS_ACK_VALUE_BUFLEN];
uint16_t Gatts_acklen=0;
uint16_t Gatts_valuelen=0;
EMQTTREQ _g_EBLEREQType;       //BLE请求数据类型

static prepare_type_env_t prepare_write_env;


//#define CONFIG_SET_RAW_ADV_DATA
#ifdef CONFIG_SET_RAW_ADV_DATA
static uint8_t raw_adv_data[] = {
        /* flags */
        0x02, 0x01, 0x06,
        /* tx power*/
        0x02, 0x0a, 0xeb,
        /* service uuid */
        0x03, 0x03, 0xFF, 0x00,
        /* device name */
		0x0f, 0x09, 'E', 'S', 'P', '_', 'G', 'A', 'T', 'T', 'S', '_', '1','2', '3', '4'
	// 0x0f, 0x09, 'E', 'S', 'P', '_', 'G', 'A', 'T', 'T', 'S', '_', 'D','E', 'M', 'O'
	// 0x0f, 0x09, 'B', 'L', 'E', '_', 'G', 'A', 'T', 'T', 'S', '_', 'D','E', 'M', 'O'
};
static uint8_t raw_scan_rsp_data[] = {
        /* flags */
        0x02, 0x01, 0x06,
        /* tx power */
        0x02, 0x0a, 0xeb,
        /* service uuid */
        0x03, 0x03, 0xFF,0x00
};

#else
static uint8_t service_uuid[16] = {
    /* LSB <--------------------------------------------------------------------------------> MSB */
    //first uuid, 16bit, [12],[13] is the value
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
};

#define MANUFACLEN  14
//厂商信息:设备类型
#define MANUFACTURER_TYPE 4
uint8_t manufacturer_TYPEBUFF[4]={0x47,0x3a,0x33,0x33};//G:33
//uint8_t manufacturer_TYPEBUFF[4]={0x47,0x3a,0x33,0x33};//g:33

//厂商信息：mac地址
#define MANUFACTURER_MAC  8
uint8_t manufacturer_MACBUFF[8]={0x4d,0x3a,0x00,0x00,0x00,0x00,0x00,0x00 };//M:xx xx xx xx xx xx

char DEVICE_NAME_SN[30]={"THEISBLESN"};

/* The length of adv data must be less than 31 bytes */
static esp_ble_adv_data_t adv_data = {
    .set_scan_rsp        = false,    	/* !< 是否将此广告数据设置为扫描响应 */
    .include_name        = true, 		/* !< 广告数据是否包含设备名称     */
    .include_txpower     = true,		/* !< 广告数据包括发射功率        */
    .min_interval        = 0x0010, //slave connection min interval, Time = min_interval * 1.25 msec =20ms/* !< 广告数据显示从属首选连接最小间隔*/
    .max_interval        = 0x0020, //slave connection max interval, Time = max_interval * 1.25 msec = 20 ms/* !< 广告数据显示从属首选连接最大间隔*/
    .appearance          = 0x00,   /*!< 设备外观*/
    .manufacturer_len    = MANUFACTURER_TYPE,      //TEST_MANUFACTURER_DATA_LEN, /* !< 制造商数据长度*/
    .p_manufacturer_data = &manufacturer_TYPEBUFF[0],   //test_manufacturer,  /* !< 制造商数据点*/
    .service_data_len    = 0,      /* !< 服务数据长度*/
    .p_service_data      = NULL,   /* !< 服务数据点*/
    .service_uuid_len    = sizeof(service_uuid),  	/* !< 服务 uuid 长度*/
    .p_service_uuid      = service_uuid,    		/* !< 服务 uuid 数组点*/
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT), /* !< 发现模式的广告标志，详见 BLE_ADV_DATA_FLAG */
};

// scan response data   当超过31字节通过扫描响应应答设备发送
static esp_ble_adv_data_t scan_rsp_data = {
    .set_scan_rsp        = true,
    .include_name        = false,
    .include_txpower     = false,
    .min_interval        = false,
    .max_interval        = false,
    .appearance          = 0x00,
    .manufacturer_len    = MANUFACTURER_MAC, 		 //TEST_MANUFACTURER_DATA_LEN,
    .p_manufacturer_data = &manufacturer_MACBUFF[0], //&test_manufacturer[0],
    .service_data_len    = 0,
    .p_service_data      = NULL,
    .service_uuid_len    = 0,						 //sizeof(service_uuid),
    .p_service_uuid      = NULL,					 //service_uuid,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};
#endif /* CONFIG_SET_RAW_ADV_DATA */

static esp_ble_adv_params_t adv_params = {
    .adv_int_min         = 0x20,
    .adv_int_max         = 0x40,
    .adv_type            = ADV_TYPE_IND,
    .own_addr_type       = BLE_ADDR_TYPE_PUBLIC,
    .channel_map         = ADV_CHNL_ALL,
    .adv_filter_policy   = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    uint16_t char_handle;
    esp_bt_uuid_t char_uuid;
    esp_gatt_perm_t perm;
    esp_gatt_char_prop_t property;
    uint16_t descr_handle;
    esp_bt_uuid_t descr_uuid;
};

static void gatts_profile_event_handler(esp_gatts_cb_event_t event,
					esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

/* One gatt-based profile one app_id and one gatts_if, this array will store the gatts_if returned by ESP_GATTS_REG_EVT */
static struct gatts_profile_inst heart_rate_profile_tab[PROFILE_NUM] = {
    [PROFILE_APP_IDX] = {
        .gatts_cb = gatts_profile_event_handler,
        .gatts_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },
};

/* Service */
static const uint16_t GATTS_SERVICE_UUID_TEST      = 0x00FF;
static const uint16_t GATTS_CHAR_UUID_TEST_A       = 0xFF01;
static const uint16_t GATTS_CHAR_UUID_TEST_B       = 0xFF02;
static const uint16_t GATTS_CHAR_UUID_TEST_C       = 0xFF03;

static const uint16_t primary_service_uuid         = ESP_GATT_UUID_PRI_SERVICE;
static const uint16_t character_declaration_uuid   = ESP_GATT_UUID_CHAR_DECLARE;
static const uint16_t character_client_config_uuid = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
static const uint8_t char_prop_read                = ESP_GATT_CHAR_PROP_BIT_READ;

static const uint8_t char_prop_write_need_rsp      = ESP_GATT_CHAR_PROP_BIT_WRITE;		//允许客户端写操作请求应答
static const uint8_t char_prop_write_not_rsp       = ESP_GATT_CHAR_PROP_BIT_WRITE_NR;	//允许客户端写操作不用应答
static const uint8_t char_prop_write       	       = char_prop_write_need_rsp | char_prop_write_not_rsp;

static const uint8_t char_prop_read_write_notify   = ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
static const uint8_t heart_measurement_ccc[2]      = {0x00, 0x00};
static const uint8_t char_value[4]                 = {0x47, 0x52, 0x54, 0x33};

/* Full Database Description - Used to add attributes into the database */
static const esp_gatts_attr_db_t gatt_db[HRS_IDX_NB] =
{
    //Service Declaration
    [IDX_SVC]        =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ,
      sizeof(uint16_t), sizeof(GATTS_SERVICE_UUID_TEST), (uint8_t *)&GATTS_SERVICE_UUID_TEST}},

    /* Characteristic Declaration */
    [IDX_CHAR_A]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write_notify}},

    /* Characteristic Value */
    [IDX_CHAR_VAL_A] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_TEST_A, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(char_value), (uint8_t *)char_value}},

    /* Client Characteristic Configuration Descriptor */
    [IDX_CHAR_CFG_A]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      sizeof(uint16_t), sizeof(heart_measurement_ccc), (uint8_t *)heart_measurement_ccc}},

    /* Characteristic Declaration */
    [IDX_CHAR_B]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},

    /* Characteristic Value */
    [IDX_CHAR_VAL_B]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_TEST_B, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(char_value), (uint8_t *)char_value}},

    /* Characteristic Declaration */
    [IDX_CHAR_C]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_write}},

    /* Characteristic Value */
    [IDX_CHAR_VAL_C]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_TEST_C, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(char_value), (uint8_t *)char_value}},

};
//	注册回调函数  事件
//一旦设置了广告和扫描响应数据的配置，处理程序就可以使用这些事件中的任何一个来开始广告，
//这是使用以下esp_ble_gap_start_advertising()函数完成的

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
    #ifdef CONFIG_SET_RAW_ADV_DATA
        case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
            adv_config_done &= (~ADV_CONFIG_FLAG);
            if (adv_config_done == 0){
                esp_ble_gap_start_advertising(&adv_params);
            }
            break;
        case ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT:
            adv_config_done &= (~SCAN_RSP_CONFIG_FLAG);
            if (adv_config_done == 0){
                esp_ble_gap_start_advertising(&adv_params);
            }
            break;
    #else
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
            adv_config_done &= (~ADV_CONFIG_FLAG);
            if (adv_config_done == 0){   //当前广告只有一个只能轮训机制处理
                esp_ble_gap_start_advertising(&adv_params);
            }
            break;
        case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
            adv_config_done &= (~SCAN_RSP_CONFIG_FLAG);
            if (adv_config_done == 0){    //  扫描响应广告
                esp_ble_gap_start_advertising(&adv_params);
            }
            break;
    #endif
        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            /* advertising start complete event to indicate advertising start successfully or failed */
            if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(GATTS_TABLE_TAG, "advertising start failed");
            }else{
                ESP_LOGI(GATTS_TABLE_TAG, "advertising start successfully");
            }
            break;
        case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
            if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(GATTS_TABLE_TAG, "Advertising stop failed");
            }
            else {
                ESP_LOGI(GATTS_TABLE_TAG, "Stop adv successfully\n");
            }
            break;
        case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
                  param->update_conn_params.status,
                  param->update_conn_params.min_int,
                  param->update_conn_params.max_int,
                  param->update_conn_params.conn_int,
                  param->update_conn_params.latency,
                  param->update_conn_params.timeout);
            break;
        default:
            break;
    }
}


esp_gatt_rsp_t usGattRspBUff = {0};

void example_prepare_write_event_env(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param)
{
    ESP_LOGI(GATTS_TABLE_TAG, "prepare write, handle = %d, value len = %d", param->write.handle, param->write.len);
    esp_gatt_status_t status = ESP_GATT_OK;

    if (prepare_write_env->prepare_buf == NULL) {
        //prepare_write_env->prepare_buf = (uint8_t *)malloc(PREPARE_BUF_MAX_SIZE * sizeof(uint8_t));
    	prepare_write_env->prepare_buf = (uint8_t *)Gatts_valuebuf; //指向接收buff
        prepare_write_env->prepare_len = 0;
        if (prepare_write_env->prepare_buf == NULL) {
            ESP_LOGE(GATTS_TABLE_TAG, "%s, Gatt_server prep no mem", __func__);
            status = ESP_GATT_NO_RESOURCES;
        }
    } else {
        if(param->write.offset > GATTS_RCV_VALUE_BUFLEN)		//接收的大小由默认的 PREPARE_BUF_MAX_SIZE  改为接收buff的大小 GATTS_RCV_VALUE_BUFLEN
        {
            status = ESP_GATT_INVALID_OFFSET;
        } else if ((param->write.offset + param->write.len) > GATTS_RCV_VALUE_BUFLEN)//接收的大小由默认的 PREPARE_BUF_MAX_SIZE  改为接收buff的大小 GATTS_RCV_VALUE_BUFLEN
        {
            status = ESP_GATT_INVALID_ATTR_LEN;
        }
    }
    /*send response when param->write.need_rsp is true */
    if (param->write.need_rsp){
       // esp_gatt_rsp_t *gatt_rsp = (esp_gatt_rsp_t *)malloc(sizeof(esp_gatt_rsp_t));
    	 esp_gatt_rsp_t *gatt_rsp = &usGattRspBUff;  //应答的buff改为静态

        if (gatt_rsp != NULL){
            gatt_rsp->attr_value.len = param->write.len;
            gatt_rsp->attr_value.handle = param->write.handle;
            gatt_rsp->attr_value.offset = param->write.offset;
            gatt_rsp->attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
            memcpy(gatt_rsp->attr_value.value, param->write.value, param->write.len);
            esp_err_t response_err = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, gatt_rsp);
            if (response_err != ESP_OK){
               ESP_LOGE(GATTS_TABLE_TAG, "Send response error");
            }
           // free(gatt_rsp); 屏蔽
        }else{
            ESP_LOGE(GATTS_TABLE_TAG, "%s, malloc failed", __func__);
        }
    }
    if (status != ESP_GATT_OK){
        return;
    }
    memcpy(prepare_write_env->prepare_buf + param->write.offset,
           param->write.value,
           param->write.len);
    prepare_write_env->prepare_len += param->write.len;

}

void example_exec_write_event_env(prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param){

    if (param->exec_write.exec_write_flag == ESP_GATT_PREP_WRITE_EXEC && prepare_write_env->prepare_buf){
        //esp_log_buffer_hex(GATTS_TABLE_TAG, prepare_write_env->prepare_buf, prepare_write_env->prepare_len);	//屏蔽打印
    }else{
        ESP_LOGI(GATTS_TABLE_TAG,"ESP_GATT_PREP_WRITE_CANCEL");
    }

    if (prepare_write_env->prepare_buf) {
        //free(prepare_write_env->prepare_buf);	//屏蔽
        prepare_write_env->prepare_buf = NULL;

        Gatts_valuelen = prepare_write_env->prepare_len ; 			//分包接收完成
		xEventGroupSetBits(s_gatts_event_group, GATTS_WRITE_BIT);	//触发写事件

    }

    prepare_write_env->prepare_len = 0;
}



static void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    switch (event) {
        case ESP_GATTS_REG_EVT:{  //当注册应用程序id时，事件来了
            esp_err_t set_dev_name_ret = esp_ble_gap_set_device_name(DEVICE_NAME_SN);  //设置广告的名称
            if(set_dev_name_ret){
                ESP_LOGE(GATTS_TABLE_TAG, "set device name failed, error code = %x", set_dev_name_ret);
            }
#ifdef CONFIG_SET_RAW_ADV_DATA
            esp_err_t raw_adv_ret = esp_ble_gap_config_adv_data_raw(raw_adv_data, sizeof(raw_adv_data));  //配置标准广告数据 esp_ble_adv_data_t
            if (raw_adv_ret){
                ESP_LOGE(GATTS_TABLE_TAG, "config raw adv data failed, error code = %x ", raw_adv_ret);
            }
            adv_config_done |= ADV_CONFIG_FLAG;
            esp_err_t raw_scan_ret = esp_ble_gap_config_scan_rsp_data_raw(raw_scan_rsp_data, sizeof(raw_scan_rsp_data));
            if (raw_scan_ret){
                ESP_LOGE(GATTS_TABLE_TAG, "config raw scan rsp data failed, error code = %x", raw_scan_ret);
            }
            adv_config_done |= SCAN_RSP_CONFIG_FLAG;
#else
            //config adv data
            esp_err_t ret = esp_ble_gap_config_adv_data(&adv_data);
            if (ret){
                ESP_LOGE(GATTS_TABLE_TAG, "config adv data failed, error code = %x", ret);
            }
            adv_config_done |= ADV_CONFIG_FLAG;
            //config scan response data
            ret = esp_ble_gap_config_adv_data(&scan_rsp_data);     // 如果数据过长31字节 ，通常较长的参数存储在扫描响应中
            if (ret){
                ESP_LOGE(GATTS_TABLE_TAG, "config scan response data failed, error code = %x", ret);
            }
            adv_config_done |= SCAN_RSP_CONFIG_FLAG;
    #endif
            esp_err_t create_attr_ret = esp_ble_gatts_create_attr_tab(gatt_db, gatts_if, HRS_IDX_NB, SVC_INST_ID);
            if (create_attr_ret){
                ESP_LOGE(GATTS_TABLE_TAG, "create attr table failed, error code = %x", create_attr_ret);
            }
        }
       	    break;
        case ESP_GATTS_READ_EVT:   // APP 读操作  //当gatt客户端请求读操作时，事件发生
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_READ_EVT");
            xEventGroupSetBits(s_gatts_event_group, GATTS_READ_BIT);
       	    break;
        case ESP_GATTS_WRITE_EVT:       ///当gatt客户端请求写操作时，事件发生
            if (!param->write.is_prep)  // 小于 分包长度
            {
                //the data length of gattc write  must be less than GATTS_DEMO_CHAR_VAL_LEN_MAX.
//              ESP_LOGI(GATTS_TABLE_TAG, "GATT_WRITE_EVT, handle = %d, value len = %d, value :", param->write.handle, param->write.len);
//              esp_log_buffer_hex(GATTS_TABLE_TAG, param->write.value, param->write.len);
//               memcpy(Gatts_valuebuf,  param->write.value, param->write.len);
//                Gatts_valuelen=param->write.len;
//              esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, heart_rate_handle_table[IDX_CHAR_VAL_A],
//                		Gatts_valuelen, Gatts_valuebuf, false);
                if (heart_rate_handle_table[IDX_CHAR_CFG_A] == param->write.handle && param->write.len == 2){
                    uint16_t descr_value = param->write.value[1]<<8 | param->write.value[0];
                    if (descr_value == 0x0001){
                        ESP_LOGI(GATTS_TABLE_TAG, "notify enable");
                        uint8_t notify_data[15];
                        for (int i = 0; i < sizeof(notify_data); ++i)
                        {
                            notify_data[i] = i % 0xff;
                        }
                        //the size of notify_data[] need less than MTU size
                        esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, heart_rate_handle_table[IDX_CHAR_VAL_A],
                                                sizeof(notify_data), notify_data, false);
                    }else if (descr_value == 0x0002){
                        ESP_LOGI(GATTS_TABLE_TAG, "indicate enable");
                        uint8_t indicate_data[15];
                        for (int i = 0; i < sizeof(indicate_data); ++i)
                        {
                            indicate_data[i] = i % 0xff;
                        }
                        //the size of indicate_data[] need less than MTU size
                        esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, heart_rate_handle_table[IDX_CHAR_VAL_A],
                                            sizeof(indicate_data), indicate_data, true);
                    }
                    else if (descr_value == 0x0000){
                        ESP_LOGI(GATTS_TABLE_TAG, "notify/indicate disable ");
                    }else{
                        ESP_LOGE(GATTS_TABLE_TAG, "unknown descr value");
                        esp_log_buffer_hex(GATTS_TABLE_TAG, param->write.value, param->write.len);
                    }
                }
                /* send response when param->write.need_rsp is true */
                if (param->write.need_rsp){
                    esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
                }
#if BLE_Readmode

                memcpy(&Gatts_valuebuf[Gatts_valuelen], param->write.value, param->write.len);
                Gatts_valuelen+=param->write.len;

                System_state.BLEread_waitTime = GetSystemTick_ms();  //每次帧数据 80MS  传输距离有关!!!
                System_state.BLE_ReadDone = 1;

                if(param->write.len == ( System_state.blemtu_value-3 ))
                {
                	 System_state.BLE_ReadDone = 2;
                }
                ESP_LOGE(GATTS_TABLE_TAG, "Gatts_valuelen=%d  param->write.len=%d  System_state.BLEread_waitTime=%d" ,Gatts_valuelen,param->write.len,System_state.BLEread_waitTime);
#else
                memcpy(Gatts_valuebuf,  param->write.value, param->write.len);
                Gatts_valuelen=param->write.len;
                xEventGroupSetBits(s_gatts_event_group, GATTS_WRITE_BIT);
#endif

            }
            else
            {
            	/* handle prepare write */
                example_prepare_write_event_env(gatts_if, &prepare_write_env, param);
            }

            IOT_BLEofftime_uc(ESP_WRITE,BLE_CLOSE_TIME_CONNECTED_WHEN_RECV_DATA);		//收到数据刷新关闭时间

      	    break;
        case ESP_GATTS_EXEC_WRITE_EVT: //当gatt客户端请求执行写时，出现事件
            // the length of gattc prepare write data must be less than GATTS_DEMO_CHAR_VAL_LEN_MAX.
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_EXEC_WRITE_EVT");
            example_exec_write_event_env(&prepare_write_env, param);
            break;
        case ESP_GATTS_MTU_EVT:  //当设置mtu完成时，事件来
        	System_state.blemtu_value = param->mtu.mtu;
        	ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);

            break;
        case ESP_GATTS_CONF_EVT: //
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_CONF_EVT, status = %d, attr_handle %d", param->conf.status, param->conf.handle);
            break;
        case ESP_GATTS_START_EVT: //当启动服务完成时，出现事件
            ESP_LOGI(GATTS_TABLE_TAG, "SERVICE_START_EVT, status %d, service_handle %d", param->start.status, param->start.service_handle);
            break;
        case ESP_GATTS_CONNECT_EVT:  // 当gatt客户端连接时，事件来
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_CONNECT_EVT, conn_id = %d", param->connect.conn_id);
            esp_log_buffer_hex(GATTS_TABLE_TAG, param->connect.remote_bda, 6);
            esp_ble_conn_update_params_t conn_params = {0};
            memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
            /* For the iOS system, please refer to Apple official documents about the BLE connection parameters restrictions. */
            conn_params.latency = 0;
            conn_params.max_int = 0x20;    // max_int = 0x20*1.25ms = 40ms
            conn_params.min_int = 0x10;    // min_int = 0x10*1.25ms = 20ms
            conn_params.timeout = 400;    // timeout = 400*10ms = 4000ms
            //start sent the update connection parameters to the peer device.
            esp_ble_gap_update_conn_params(&conn_params);

            System_state.Ble_state=Online ;
			System_state.WIFI_BLEdisplay |=Displayflag_BLECON;  //写便携式显示状态
			IOT_KEYSET0X06(56,System_state.WIFI_BLEdisplay );

            break;
        case ESP_GATTS_DISCONNECT_EVT:  //当gatt客户端断开连接时，事件来
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_DISCONNECT_EVT, reason = 0x%x", param->disconnect.reason);
            esp_ble_gap_start_advertising(&adv_params);
            System_state.Ble_state = Offline ;
            System_state.BLEKeysflag = 0;

			System_state.WIFI_BLEdisplay &= ~Displayflag_BLECON;  //写便携式显示状态
			IOT_KEYSET0X06(56,System_state.WIFI_BLEdisplay );
			IOT_InCMDTime_uc(ESP_WRITE,3);

			IOT_BLEofftime_uc(ESP_WRITE, BLE_CLOSE_TIME_DISCONNECT );  // 10s 后关闭
            break;
        case ESP_GATTS_CREAT_ATTR_TAB_EVT:{
            if (param->add_attr_tab.status != ESP_GATT_OK){
                ESP_LOGE(GATTS_TABLE_TAG, "create attribute table failed, error code=0x%x", param->add_attr_tab.status);
            }
            else if (param->add_attr_tab.num_handle != HRS_IDX_NB){
                ESP_LOGE(GATTS_TABLE_TAG, "create attribute table abnormally, num_handle (%d) \
                        doesn't equal to HRS_IDX_NB(%d)", param->add_attr_tab.num_handle, HRS_IDX_NB);
            }
            else {
                ESP_LOGI(GATTS_TABLE_TAG, "create attribute table successfully, the number handle = %d\n",param->add_attr_tab.num_handle);
                memcpy(heart_rate_handle_table, param->add_attr_tab.handles, sizeof(heart_rate_handle_table));
                esp_ble_gatts_start_service(heart_rate_handle_table[IDX_SVC]);
            }
            break;
        }
        case ESP_GATTS_STOP_EVT:
        case ESP_GATTS_OPEN_EVT:
        case ESP_GATTS_CANCEL_OPEN_EVT:
        case ESP_GATTS_CLOSE_EVT:
        case ESP_GATTS_LISTEN_EVT:
        case ESP_GATTS_CONGEST_EVT:
        case ESP_GATTS_UNREG_EVT:
        case ESP_GATTS_DELETE_EVT:
        default:
            break;
    }
}
// 注册回调函数 注册事件
static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    /* If event is register event, store the gatts_if for each profile */
    if (event == ESP_GATTS_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            heart_rate_profile_tab[PROFILE_APP_IDX].gatts_if = gatts_if;
        } else {
            ESP_LOGE(GATTS_TABLE_TAG, "reg app failed, app_id %04x, status %d",
                    param->reg.app_id,
                    param->reg.status);
            return;
        }
    }
    do {
        int idx;
        for (idx = 0; idx < PROFILE_NUM; idx++) {
            /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
            if (gatts_if == ESP_GATT_IF_NONE || gatts_if == heart_rate_profile_tab[idx].gatts_if) {
                if (heart_rate_profile_tab[idx].gatts_cb) {
                    heart_rate_profile_tab[idx].gatts_cb(event, gatts_if, param);
                }
            }
        }
    } while (0);
}
void IOT_GattsTable_Init(void)
{
	esp_err_t ret;
	uint8_t ble_mac[6]={0};

   memcpy(DEVICE_NAME_SN , &SysTAB_Parameter[Parameter_len[REG_SN]] , Collector_Config_LenList[REG_SN] );
	/* Initialize NVS. */
//	ret = nvs_flash_init();
//	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
//	        ESP_ERROR_CHECK(nvs_flash_erase());
//	        ret = nvs_flash_init();
//	    }
//	    ESP_ERROR_CHECK( ret );

	ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));  //在初始化时释放未使用的BT经典内存。ESP_BT_MODE_CLASSIC_BT
	ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));  //在初始化时释放未使用的BT经典内存。ESP_BT_MODE_CLASSIC_BT
	ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BTDM));  //在初始化时释放未使用的BT经典内存。ESP_BT_MODE_CLASSIC_BT
	esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT(); //
	ret = esp_bt_controller_init(&bt_cfg);
	if (ret) {
		ESP_LOGE(GATTS_TABLE_TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
		return;
	}
	ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
	if (ret) {
		ESP_LOGE(GATTS_TABLE_TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
		return;
	}
	ret = esp_bluedroid_init();
	if (ret) {
	   	ESP_LOGE(GATTS_TABLE_TAG, "%s init bluetooth failed: %s"  , __func__, esp_err_to_name(ret));
	        return;
	    }
	ret = esp_bluedroid_enable();
	if (ret) {
		ESP_LOGE(GATTS_TABLE_TAG, "%s enable bluetooth failed: %s", __func__, esp_err_to_name(ret));
	        return;
	    }

	esp_read_mac(ble_mac, ESP_MAC_BT);
	ESP_LOGI(GATTS_TABLE_TAG, " ***1*******read BLE_mac = (%02x:%02x:%02x:%02x:%02x:%02x) \r\n",ble_mac[0],ble_mac[1],ble_mac[2],ble_mac[3],ble_mac[4],ble_mac[5] );
//	for(uint8_t i=0 ,j=2;i<6;i++)
//	{
//		manufacturer_MACBUFF[j++]=ble_mac[i];
//		manufacturer_MACBUFF[j++]=0x3a;
//	}

	manufacturer_MACBUFF[0]=0x4d;
	manufacturer_MACBUFF[1]=0x3a;
	manufacturer_MACBUFF[2]=ble_mac[0];
	manufacturer_MACBUFF[3]=ble_mac[1];
	manufacturer_MACBUFF[4]=ble_mac[2];
	manufacturer_MACBUFF[5]=ble_mac[3];
	manufacturer_MACBUFF[6]=ble_mac[4];
	manufacturer_MACBUFF[7]=ble_mac[5];

	if(System_state.BLEBindingGg== 1)
	{
	  manufacturer_TYPEBUFF[0]=0x47;  //{0x47,0x3a,0x33,0x33};//G:33  已绑定设备
	}
	else
	{
      manufacturer_TYPEBUFF[0]=0x67;  //{0x47,0x3a,0x33,0x33};//g:33  未绑定设备
	}
	manufacturer_TYPEBUFF[1]=0x3a;
	manufacturer_TYPEBUFF[2]=0x33;

#if test_wifi
	manufacturer_TYPEBUFF[3]=0x34;
#else
	manufacturer_TYPEBUFF[3]=0x33;
#endif

	ret = esp_ble_gatts_register_callback(gatts_event_handler);  // 注册回调函数 注册事件
	if (ret){
	    ESP_LOGE(GATTS_TABLE_TAG, "gatts register error, error code = %x", ret);
	        return;
	    }
	ret = esp_ble_gap_register_callback(gap_event_handler);		//	注册回调函数  事件
	if (ret){
	    ESP_LOGE(GATTS_TABLE_TAG, "gap register error, error code = %x", ret);
	        return;
	    }
	ret = esp_ble_gatts_app_register(ESP_APP_ID);				// 注册应用程序标识符
	if (ret){
	    ESP_LOGE(GATTS_TABLE_TAG, "gatts app register error, error code = %x", ret);
	        return;
	    }
	esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500);  // MTU 收发数据长度  木桶效应  GATTS_DEMO_CHAR_VAL_LEN_MAX
	if (local_mtu_ret)
	{
	    ESP_LOGE(GATTS_TABLE_TAG, "set local  MTU failed, error code = %x", local_mtu_ret);
	}
	//System_state.ble_onoff=1;
}

//esp_bt.h
//* In such cases, after receiving the WiFi configuration, you can disable/deinit bluetooth and release its memory.
//* Below is the sequence of APIs to be called for such scenarios:
//*
//*      esp_bluedroid_disable();
//*      esp_bluedroid_deinit();
//*      esp_bt_controller_disable();
//*      esp_bt_controller_deinit();
//*      esp_bt_mem_release(ESP_BT_MODE_BTDM);



void IOT_Gattstable_release(void)
{
	esp_err_t ret;

	esp_ble_gatts_stop_service(heart_rate_handle_table[IDX_SVC]);
	esp_ble_gap_stop_scanning();
	esp_ble_gap_stop_advertising();		//停止广播
	esp_ble_gap_disconnect(adv_params.peer_addr);


	//blufi_security_deinit();
	ret = esp_bluedroid_disable();
	if (ret){
	ESP_LOGE(GATTS_TABLE_TAG, "esp_bluedroid_disable error, error code = %x", ret);
 return ;
	}
	ret = esp_bluedroid_deinit();

	if (ret){
	ESP_LOGE(GATTS_TABLE_TAG, "esp_bluedroid_deinit error, error code = %x", ret);
return ;
	}

#if 1 //(1 == test_wifi)
	ret = esp_bt_controller_disable();
	if (ret){
	ESP_LOGE(GATTS_TABLE_TAG, "esp_bt_controller_disable error, error code = %x", ret);
 return ;
	}
	ret = esp_bt_controller_deinit();
	if (ret){
	ESP_LOGE(GATTS_TABLE_TAG, "esp_bt_controller_deinit error, error code = %x", ret);
	 return ;
	}

	ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));
	ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
	ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BTDM));
#endif
	ret = esp_bt_mem_release(ESP_BT_MODE_BLE);
	if (ret){
	ESP_LOGE(GATTS_TABLE_TAG, "esp_bt_mem_release ESP_BT_MODE_BLE error, error code = %x", ret);
	 return ;
	}
	ret = esp_bt_mem_release(ESP_BT_MODE_CLASSIC_BT);
	if (ret){
	ESP_LOGE(GATTS_TABLE_TAG, "esp_bt_mem_release ESP_BT_MODE_CLASSIC_BT error, error code = %x", ret);
	 return ;
	}
	ret = esp_bt_mem_release(ESP_BT_MODE_BTDM);
	if (ret){
	ESP_LOGE(GATTS_TABLE_TAG, "esp_bt_mem_release ESP_BT_MODE_BTDM error, error code = %x", ret);
	 return ;
	}

}




/********************************************  ***** *************************************/


void IOT_GetGattsBLE_config()
{
//	esp_ble_tx_power_get();
}


void IOT_GattsSendData(uint8_t *Sendbuf ,uint16_t Sendlen)
{
	uint32_t break_count = 0;
	uint32_t uiMaxSendLen = 0;
	uint16_t usLen = 0;

	ESP_LOGE(GATTS_TABLE_TAG, "IOT_GattsSendData Sendlen = %d", Sendlen);

	for(int i = 0 ; i < Sendlen ; i++)
	{
		IOT_Printf("%02x ",Sendbuf[i]);
	}
	IOT_Printf("\r\n");
	//拆包发送 MTU-3
	if(System_state.blemtu_value > 512)
	{
		uiMaxSendLen = 500 - 3;
	}else
	{
		uiMaxSendLen = System_state.blemtu_value - 3;
	}

	while((Sendlen > 0) && (break_count++ < 0x0000ffff ))
	{
		if(Sendlen > uiMaxSendLen)
		{
			usLen = uiMaxSendLen;
		}else
		{
			usLen = Sendlen;
		}

		esp_ble_gatts_send_indicate(   heart_rate_profile_tab[0].gatts_if, 0, heart_rate_handle_table[IDX_CHAR_VAL_A],
				usLen,  Sendbuf, false);

		Sendbuf += usLen;
		Sendlen -= usLen;
	}
}

void IOT_GattsReceiveHandle(void)
{
	_g_EBLEREQType = IOT_ServerReceiveHandle(Gatts_valuebuf,Gatts_valuelen );  // 解析数据 获取响应数据类型
}

void IOT_GattsSendHandle(void)
{
	ESP_LOGI(GATTS_TABLE_TAG, "*********** _g_EBLEREQType=%d ******* \r\n",_g_EBLEREQType);
  	switch(_g_EBLEREQType)
	{
		case MQTTREQ_SET_TIME:
			break;
		case MQTTREQ_CMD_0x05 :
			if(System_state.Host_setflag & CMD_0X05 )   	// 服务器查询置位
			{
				if(!(InvCommCtrl.CMDFlagGrop & CMD_0X05 ))  // 接收逆变器应答清位  等待 其他线程完成数据处理
				{
					Gatts_acklen=IOT_ESPSend_0x05(Gatts_ackbuf , LOCAL_PROTOCOL );  // 逆变器参数获取 0x05查询数据
				    ESP_LOGI(GATTS_TABLE_TAG, "ESP_READY  LOCAL_PROTOCOL MQTT_DATALen=%d \r\n", Gatts_acklen );
					_g_EBLEREQType = MQTTREQ_NULL;			//
				}
			}
			break;
		case MQTTREQ_CMD_0x06 :
			if(System_state.Host_setflag & CMD_0X06 )   	// 服务器设置置位
			{
				if(!(InvCommCtrl.CMDFlagGrop & CMD_0X06 ))  // 接收逆变器应答清位 等待 其他线程完成数据处理
				{
					Gatts_acklen=IOT_ESPSend_0x06(Gatts_ackbuf ,LOCAL_PROTOCOL );
					ESP_LOGI(GATTS_TABLE_TAG, "ESP_READY  IOT_ESPSend_0x06 MQTT_DATALen=%d \r\n", Gatts_acklen );
					_g_EBLEREQType = MQTTREQ_NULL;
				}
			}
			break;
		case MQTTREQ_CMD_0x10 :
			if(System_state.Host_setflag & CMD_0X10   )   	// 服务器设置置位
			{
				if(!(InvCommCtrl.CMDFlagGrop & CMD_0X10 ))  // 接收逆变器应答清位 等待 其他线程完成数据处理
				{
					Gatts_acklen=IOT_ESPSend_0x10(Gatts_ackbuf ,LOCAL_PROTOCOL );
					ESP_LOGI(GATTS_TABLE_TAG, "ESP_READY  IOT_ESPSend_0x10 MQTT_DATALen=%d \r\n", Gatts_acklen );
					_g_EBLEREQType = MQTTREQ_NULL;
				}
			}
			break;
		case MQTTREQ_CMD_0x18 :
			if(System_state.Host_setflag & CMD_0X18)   		// 服务器查询置位
			{
				if(!(InvCommCtrl.CMDFlagGrop & CMD_0X18))   // 接收逆变器应答清位 等待 其他线程完成数据处理
				{
					Gatts_acklen=IOT_ESPSend_0x18(Gatts_ackbuf ,LOCAL_PROTOCOL );
					_g_EBLEREQType = MQTTREQ_NULL;
					ESP_LOGI(GATTS_TABLE_TAG, "!!!!!!!! IOT_ESPSend_0x18 MQTT_DATALen=%d ,System_state.ServerGET_REGAddrStart =%d\r\n", Gatts_acklen ,System_state.ServerGET_REGAddrStart);
				}
			}
			break;
		case MQTTREQ_CMD_0x19 :
			if(System_state.Host_setflag & CMD_0X19 )   	// 服务器查询置位
			{
			 	if(!(InvCommCtrl.CMDFlagGrop & CMD_0X19 ))  // 接收逆变器应答清位 等待 其他线程完成数据处理
				{
					Gatts_acklen=IOT_ESPSend_0x19(Gatts_ackbuf ,LOCAL_PROTOCOL );
					_g_EBLEREQType = MQTTREQ_NULL;
					ESP_LOGI(GATTS_TABLE_TAG, "ESP_READY  IOT_ESPSend_0x19 MQTT_DATALen=%d \r\n", Gatts_acklen );
				}
			}
			ESP_LOGI(GATTS_TABLE_TAG, "System_state.Host_setflag=%x  InvCommCtrl.CMDFlagGrop=%x \r\n", System_state.Host_setflag,InvCommCtrl.CMDFlagGrop );
			break;
		case MQTTREQ_CMD_0x17 :
			if(System_state.Host_setflag & CMD_0X17   )   	// 服务器设置置位
			{
				if(!(InvCommCtrl.CMDFlagGrop & CMD_0X17 ))  // 接收逆变器应答清位 等待 其他线程完成数据处理
				{
					System_state.Host_setflag &= ~CMD_0X17;
					Gatts_acklen=IOT_ESPSend_0x17(Gatts_ackbuf ,LOCAL_PROTOCOL );
					_g_EBLEREQType = MQTTREQ_NULL;
				//	ESP_LOGI(GATTS_TABLE_TAG, "ESP_READY IOT_ESPSend_0x17 MQTT_DATALen=%d \r\n", Gatts_acklen );
				}
			}
			break;
		case MQTTREQ_CMD_0x26 :
			if(System_state.Host_setflag & CMD_0X26   )   	// 服务器设置置位
			{
				if(!(InvCommCtrl.CMDFlagGrop & CMD_0X26 ))  // 接收逆变器应答清位 等待 其他线程完成数据处理
				{
					System_state.Host_setflag &= ~CMD_0X26;

					Gatts_acklen = IOT_LocalTCPAck0x26_Package(0xff , Gatts_ackbuf , NULL);

					ESP_LOGI(GATTS_TABLE_TAG, "ESP_READY  IOT_ESPSend_0x26 MQTT_DATALen=%d \r\n", Gatts_acklen );
					for(int i = 0; i < Gatts_acklen ; i++)
					{
						IOT_Printf("%x",Gatts_ackbuf[i]);
					}
					ESP_LOGI(GATTS_TABLE_TAG, "\r\n ****************************  \r\n");
					_g_EBLEREQType = MQTTREQ_NULL;
				}
			}
			break;
		default:
			break;
		}
}

void IOT_GattsEvenSetR(void)
{
	if(_g_EBLEREQType != MQTTREQ_NULL )
	xEventGroupSetBits(s_gatts_event_group, GATTS_READ_BIT);
}
void IOT_GattsEvenONBLE(void)
{
	xEventGroupSetBits(s_gatts_event_group, GATTS_ON_BIT);
}
void IOT_GattsEvenOFFBLE(void)
{
	xEventGroupSetBits(s_gatts_event_group, GATTS_OFF_BIT);
}
void IOT_GattsEvenGATTS_WRITE(void)
{
	xEventGroupSetBits(s_gatts_event_group, GATTS_WRITE_BIT);

}


EMQTTREQ IOT_DataReceiveHandle(uint8_t* pucBuf_tem, uint16_t suiLen_tem)
{
	if((LOCAL_PROTOCOL & 0x000000ff) == (ProtocolVS06  & 0x000000ff))
	{
		return IOT_LocalReceiveHandle(pucBuf_tem , suiLen_tem);
	}else
	{
		return IOT_ServerReceiveHandle(pucBuf_tem , suiLen_tem);
	}
}


void IOT_GattsTable_task(void  *arg)
{
	EventBits_t gatts_eventbits ;

	s_gatts_event_group = xEventGroupCreate();      //创建事件标志组
	System_state.BLEKeysflag=0;   					//蓝牙密钥校验
#if DEBUG_HEAPSIZE
	static uint8_t timer_logoa=0;
#endif
	ESP_LOGI(GATTS_TABLE_TAG, "IOT_GattsTable_task  ing ...  \r\n");

	while(1)
	{
		gatts_eventbits = xEventGroupWaitBits(s_gatts_event_group,
				GATTS_WRITE_BIT|GATTS_READ_BIT|GATTS_OFF_BIT|GATTS_ON_BIT,
				pdTRUE,
	          pdFALSE,
	          portMAX_DELAY);
		ESP_LOGI(GATTS_TABLE_TAG,"*********** s_gatts_event_group =%x***********\r\n",gatts_eventbits);
	    if( ( gatts_eventbits & GATTS_WRITE_BIT ) != 0 ) //写事件   --- APP 写数据，接收解析
	    {
#if test_wifi

	    	uint8_t *pucAesOutputBuff = (uint8_t *)malloc(Gatts_valuelen);	//定义临时buff用于保存解密后的数据
			if(NULL != pucAesOutputBuff)
			{
				memset(pucAesOutputBuff ,0x00 ,Gatts_acklen);  //bzero();
				if(IOT_Repack_LocalProtocolData_Decrypt_us(Gatts_valuebuf, Gatts_valuelen, pucAesOutputBuff) >  0)
				{
					_g_EBLEREQType = IOT_DataReceiveHandle(pucAesOutputBuff , Gatts_valuelen);  // 解析数据 获取响应数据类型 串行 可以优化并行（容错率高）
				}else
				{
					_g_EBLEREQType = IOT_DataReceiveHandle(Gatts_valuebuf,Gatts_valuelen);  // 解析数据 获取响应数据类型 串行 可以优化并行（容错率高）
				}
				free(pucAesOutputBuff);
			}else
			{
				_g_EBLEREQType = IOT_DataReceiveHandle(Gatts_valuebuf,Gatts_valuelen);  // 解析数据 获取响应数据类型 串行 可以优化并行（容错率高）
			}
#else
			_g_EBLEREQType = IOT_ServerReceiveHandle(Gatts_valuebuf,Gatts_valuelen);  // 解析数据 获取响应数据类型 串行 可以优化并行（容错率高）
#endif
			memset(Gatts_valuebuf,0, Gatts_valuelen);
			Gatts_valuelen=0;
	    }
	    if(( gatts_eventbits & GATTS_READ_BIT ) != 0 )      //读事件   --- 由APP 写事件,解析功能码触发
	    {
	    	IOT_GattsSendHandle(); //数据打包
		  	if(Gatts_acklen>5)
		  	{
#if test_wifi
	#if !LOCAL_PROTOCOL_6_EN   //本地通讯6.0协议
		  		while(0 != (Gatts_acklen % 16))	//发送的数据长度需要为16的整数倍
		  		{
		  			if(Gatts_acklen >= GATTS_ACK_VALUE_BUFLEN)	//防止数据长度超过缓冲长度
		  			{
		  				Gatts_acklen = GATTS_ACK_VALUE_BUFLEN;
		  				break;
		  			}
		  			Gatts_ackbuf[Gatts_acklen++] = 0x00;	//补零
		  		}
				#if 1
		  		IOT_Printf("** Gatts_ackbuf **\r\n");
		  		for(int i = 0; i < Gatts_acklen ; i++)
				{
		  			IOT_Printf("%02x",Gatts_ackbuf[i]);
				}
		  		IOT_Printf("\r\n");
				#endif
		  		uint8_t *pucAesOutputBuff = (uint8_t *)malloc(Gatts_acklen); //定义临时buff用于保存加密后的数据
		  		if(NULL != pucAesOutputBuff)
		  		{
		  			memset(pucAesOutputBuff ,0x00 ,Gatts_acklen);  //bzero();
		  			IOT_AES_CBC_encrypt_ui(1 ,Gatts_ackbuf ,Gatts_acklen,pucAesOutputBuff);	//AES-CBC 加密
		  			IOT_GattsSendData(pucAesOutputBuff,Gatts_acklen);   //数据发送    MTU大小优化    APP MTU 小限制长度
		  		}
				#if 0
		  		IOT_Printf("** pucAesOutputBuff **\r\n");
		  		for(int i = 0; i < Gatts_acklen ; i++)
				{
		  			IOT_Printf("%02x",pucAesOutputBuff[i]);
				}
		  		IOT_Printf("\r\n");

				#endif

		  		free(pucAesOutputBuff);
		  		pucAesOutputBuff = NULL;

	#else
		  		IOT_GattsSendData(Gatts_ackbuf,Gatts_acklen);   //数据发送    MTU大小优化    APP MTU 小限制长度
	#endif
#else
				IOT_GattsSendData(Gatts_ackbuf,Gatts_acklen);   //数据发送    MTU大小优化    APP MTU 小限制长度
#endif
		  		}
		    ESP_LOGI(GATTS_TABLE_TAG, "*********** GATTS_READ_BIT callback****Gatts_acklen=%d******* \r\n",Gatts_acklen);

		  	memset(Gatts_ackbuf,0, Gatts_acklen);
		  	Gatts_acklen=0;
		  	if(System_state.ble_onoff == 2)   //重启
		  	{
		  		IOT_GattsEvenOFFBLE();
		  	}
	    }
	    if(( gatts_eventbits & GATTS_OFF_BIT ) != 0 )      //关闭蓝牙事件
	    {
		   ESP_LOGI(GATTS_TABLE_TAG, "*********** GATTS_OFF_BIT callback*********** \r\n");
		   IOT_Gattstable_release();
	       if(System_state.ble_onoff==1)   //主动关闭
		   {
		      System_state.ble_onoff=0;
		   }
		   else if(System_state.ble_onoff == 2)   //重启
		   {
			  IOT_GattsEvenONBLE();
		   }
	      // goto Gatts_init;
	    }
		if( ( gatts_eventbits & GATTS_ON_BIT ) != 0 ) //写事件   --- APP 写数据，接收解析
		{
			IOT_BLEkeytokencmp();

			IOT_GattsTable_Init(); 		//IOT_Gatts_Serve_Init();  		 //更换TABle
			System_state.ble_onoff=1;
		    ESP_LOGI(GATTS_TABLE_TAG, "*********** GATTS_ON_BIT callback********** \r\n");
		}
// vTaskDelay(110 / portTICK_PERIOD_MS);

#if DEBUG_HEAPSIZE
		if(timer_logoa++ > 5)
		{
			timer_logoa =0;
			ESP_LOGI(GATTS_TABLE_TAG, "is IOT_Gatts_server_task  uxTaskGetStackHighWaterMark= %d !!!\r\n", uxTaskGetStackHighWaterMark(NULL));
		}
#endif

	}
}

//void app_main(void)
//{
//    esp_err_t ret;
//    /* Initialize NVS. */
//    ret = nvs_flash_init();
//    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
//        ESP_ERROR_CHECK(nvs_flash_erase());
//        ret = nvs_flash_init();
//    }
//    ESP_ERROR_CHECK( ret );
//    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
//    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
//    ret = esp_bt_controller_init(&bt_cfg);
//    if (ret) {
//        ESP_LOGE(GATTS_TABLE_TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
//        return;
//    }
//    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
//    if (ret) {
//        ESP_LOGE(GATTS_TABLE_TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
//        return;
//    }
//    ret = esp_bluedroid_init();
//    if (ret) {
//        ESP_LOGE(GATTS_TABLE_TAG, "%s init bluetooth failed: %s", __func__, esp_err_to_name(ret));
//        return;
//    }
//    ret = esp_bluedroid_enable();
//    if (ret) {
//        ESP_LOGE(GATTS_TABLE_TAG, "%s enable bluetooth failed: %s", __func__, esp_err_to_name(ret));
//        return;
//    }
//    ret = esp_ble_gatts_register_callback(gatts_event_handler);
//    if (ret){
//        ESP_LOGE(GATTS_TABLE_TAG, "gatts register error, error code = %x", ret);
//        return;
//    }
//    ret = esp_ble_gap_register_callback(gap_event_handler);
//    if (ret){
//        ESP_LOGE(GATTS_TABLE_TAG, "gap register error, error code = %x", ret);
//        return;
//    }
//    ret = esp_ble_gatts_app_register(ESP_APP_ID);
//    if (ret){
//        ESP_LOGE(GATTS_TABLE_TAG, "gatts app register error, error code = %x", ret);
//        return;
//    }
//    esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500);
//    if (local_mtu_ret){
//        ESP_LOGE(GATTS_TABLE_TAG, "set local  MTU failed, error code = %x", local_mtu_ret);
//    }
//}
//
//
//
//


