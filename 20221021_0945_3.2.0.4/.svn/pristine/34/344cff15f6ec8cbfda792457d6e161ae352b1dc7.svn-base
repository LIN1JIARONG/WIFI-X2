set(srcs "src/iot_led.c"
         "src/iot_spi_flash.c"
         "src/iot_uart1.c"
         "src/iot_timer.c"
         "src/iot_scan.c"
         "src/iot_station.c"
       
         "src/iot_universal.c"
         "src/iot_system.c"
         "src/mqtt.c"
         "src/iot_net.c"
         "src/iot_rtc.c"
         "src/iot_mqtt.c"
         "src/iot_protocol.c"
         "src/iot_inverter.c"
         "src/iot_fota.c"
         "src/iot_params.c"
         "src/iot_https_ota.c"
         "src/iot_gatts_table.c"
     	 "src/iot_inverter_fota.c"
     	 "src/iot_ammeter.c"
         "src/iot_bmsbdc.c"
         "src/iot_InvWave.c"
         "src/iot_record.c"
         "src/iot_SmartConfig.c"
         "src/iot_Local_Protocol.c"
         
             #差分升级
    	 "ota/src/iot_ota.c"
		 "ota/src/iot_HTTP_update.c"
		 "ota/src/iot_crc32.c"
		 "ota/src/iot_bsdiff_api.c"
		 "ota/src/iot_bsfile_handle.c"
		 "ota/src/iot_bspatch.c"
    	 #本地升级
    	 "ota/src/iot_local_update.c"
    
          )
 
set(include_dirs "${project_dir}/components/ota/include")
 
idf_component_register(SRCS "${srcs}"  
          			 INCLUDE_DIRS includes
          			 INCLUDE_DIRS "${include_dirs}" 
                     PRIV_REQUIRES nvs_flash driver bt  
                     PRIV_REQUIRES esp_http_client app_update
                     PRIV_REQUIRES esp_wifi mbedtls          
                     EMBED_TXTFILES ${project_dir}/server_certs/ca_cert.pem         
                      )
 