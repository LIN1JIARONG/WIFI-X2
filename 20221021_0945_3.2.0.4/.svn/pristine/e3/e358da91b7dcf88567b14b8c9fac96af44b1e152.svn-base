/*
 * iot_uart1.c
 *
 *  Created on: 2021年11月16日
 *      Author: Administrator
 */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"

#include "iot_uart1.h"
#include "iot_inverter.h"
#include "iot_mqtt.h"
#include "iot_system.h"
#include "iot_inverter.h"
#include "iot_inverter_fota.h"
#include "iot_InvWave.h"
static const int RX_BUF_SIZE = 1024 ;

uint8_t RX_BUF[1024]={0};
s_RxBuffer Uart_Rxtype={0};
static const char *Modbus_log = "uart_TASK";

void IOT_UART0_Init(void)
{
     const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    //We won't use a buffer for sending data.
    uart_driver_install(UART_NUM_0, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_0, &uart_config);
    uart_set_pin(UART_NUM_0, TXD0_PIN, RXD0_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

}

void IOT_UART1_Init(void)
{
#if test_wifi
    const uart_config_t uart_config = {
        .baud_rate = 115200,  //9600  //19200 //115200
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
#else

    const uart_config_t uart_config = {
        .baud_rate = 9600,  //9600  //19200 //115200
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
#endif
    //We won't use a buffer for sending data.
    uart_driver_install(UART_NUM_1, RX_BUF_SIZE , 0, 0, NULL, 0);
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, TXD1_PIN, RXD1_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    Uart_Rxtype.ReadDone=0;
    Uart_Rxtype.Read_waitTime=0;
    Uart_Rxtype.Size=0;
    Uart_Rxtype.Read_count=0;

}

//放弃修改  目前 便携式修改波特率， 串口无应答死机
uint8_t IOT_SetUartbaud( uart_port_t uart_num, int Setbaud_rate)   //
{
	IOT_Printf("\r\n");
	IOT_Printf("\r\n");
	IOT_Printf("\r\n");
	IOT_Printf("\r\n");
	IOT_Printf("SetUartbaud %d !!!!! \r\n",Setbaud_rate);
	IOT_Printf("\r\n");
	IOT_Printf("\r\n");
	IOT_Printf("\r\n");
	IOT_Printf("\r\n");

	ESP_LOGI(Modbus_log ," \r\nIOT_SetUartbaud!!!!!!!!!!************\r\n" );
	// if (uart_set_baudrate(uart_num, Setbaud_rate) != ESP_OK)
	{
		return ESP_FAIL1;
	}
	ESP_LOGI(Modbus_log ,"  \r\n****************Setbaud_rate=%d****\r\n",Setbaud_rate );
	return ESP_OK1;

}



uint16_t IOT_UART1SendData(uint8_t *uSendDATABuf, uint16_t uSendLen )
{
   /*Print response directly to stdout as it is read */
   const int txBytes = uart_write_bytes(UART_NUM_1, uSendDATABuf, uSendLen);

#if DEBUG_UART
   ESP_LOGI(Modbus_log, "\r\n**************IOT_UART1SendData****CMD=%d*****txBytes=%d*****\r\n" ,uSendDATABuf[1],txBytes);
   for(int i = 0; i < uSendLen; i++)
   {
	   IOT_Printf("%02x ",uSendDATABuf[i]);
   }
   ESP_LOGI(Modbus_log, " *******************\r\n");
#endif

   return txBytes;

}

void IOT_ModbusRXTask(void *arg)
{
 //	esp_log_level_set(Modbus_log, ESP_LOG_INFO);

    uint8_t* data = (uint8_t*) malloc(256+1);
#if DEBUG_HEAPSIZE
	  static uint8_t timer_logod=0;
#endif

    ESP_LOGI(Modbus_log,"IOT_ModbusRXTask \n");
    while (1)
    {
       const int rxBytes = uart_read_bytes(UART_NUM_1 , data , 256, 50/ portTICK_RATE_MS);  // 看应用实际调整 接收超时时间 150 ms 一帧
       // 接收超过120满返回  +  接收超时返回    或者优化 portTICK_RATE_MS 时间  , 可以通过串口测试出底层接收超时时间
       //FIFO 分三个串口共用，可以更改120 但是占用其他串口 ,实际测试超120
       Uart_Rxtype.Read_waitTime ++ ;
       if(rxBytes > 0)
       {
    	   Uart_Rxtype.Read_waitTime=0;
    	   memcpy(&RX_BUF[Uart_Rxtype.Size],data,rxBytes );
    	   Uart_Rxtype.Size +=rxBytes;
    	   memset(data,0,256);
    	   Uart_Rxtype.Read_count = 1;
    	   if( Uart_Rxtype.Size  >=RX_BUF_SIZE )
    	   Uart_Rxtype.Size =0;

//         data[rxBytes] = 0;   					// 在串口接收的数据增加结束符
//         IOT_ModbusReceiveHandle(data,rxBytes);
//         ESP_LOGI(Modbus_log, "Read %d ", rxBytes);
//         memset(data,0,RX_BUF_SIZE);
//		   ModbusReceive(data,rxBytes);
//         ESP_LOGI(Modbus_log, "Read %d bytes: '%s'", rxBytes, data);
//         ESP_LOG_BUFFER_HEXDUMP(Modbus_log, data, rxBytes, ESP_LOG_INFO);
       }
       if(( Uart_Rxtype.Read_waitTime  >= 2 ) && Uart_Rxtype.Size > 0)  // (50)*2 ms  接收超时  时间可以优化。
       {
         Uart_Rxtype.Read_waitTime=0;
         RX_BUF[Uart_Rxtype.Size] = 0;   								// 在串口接收的数据增加结束符
#if DEBUG_UART
        ESP_LOGI(Modbus_log, "Read %d ", Uart_Rxtype.Size);
        for(int i = 0; i < Uart_Rxtype.Size; i++)
        {
        	IOT_Printf("%02x ",RX_BUF[i]);
        }
        ESP_LOGI(Modbus_log, "*******\r\n" );
#endif

   	     if(IOT_GetInvUpdtStat()==1)  //没有升级
         {
   	    	 IOT_ModbusReceiveHandle(RX_BUF,Uart_Rxtype.Size);
         }
   	     else
   	     {
      	     memcpy(IOTESPUsart0RcvBuf,RX_BUF,Uart_Rxtype.Size );
      	     IOTESPUsart0RcvLen = Uart_Rxtype.Size;
   	     }
//       处理完成数据清零
         //   处理完成数据清零
           	if((InverterWaveStatus_Judge(1,0) == IOT_ING) || \
           	  (InverterWaveStatus_Judge(1,0) == IOT_BUSY))  //  是否有波形在请求
           	{}
           	else
           	{
				 Uart_Rxtype.Read_count=0 ; // 数据解析完成 清零，下次轮训
				 Uart_Rxtype.Size=0;		  // 数据长度清0
				 memset(RX_BUF,0,RX_BUF_SIZE);
           	}
       }

#if DEBUG_HEAPSIZE

       	if(timer_logod++>200)
   		{
       		timer_logod=0;
       	   ESP_LOGI(Modbus_log, "is IOT_ModbusRXTask uxTaskGetStackHighWaterMark= %d !!!\r\n", uxTaskGetStackHighWaterMark(NULL));
   		}
#endif
     //  vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    free(data);
}

void IOT_ModbusTXTask(void *arg)
{

 	static uint8_t  HoldingSentone_flag =0 ;   // 主动上报 03 数据标记 只上报一次
 	static uint32_t TX_Timeout=0;
#if DEBUG_HEAPSIZE
	static	uint8_t timer_logoe=0;
#endif
 	ESP_LOGI(Modbus_log,"IOT_ModbusTXTask \r\n");
	//IOT_INVPollingEN();
 	InvCommCtrl.CMDFlagGrop=0;
	IOT_InCMDTime_uc(ESP_WRITE,30);

    while (1)
    {
    	if(Uart_Rxtype.Read_count==1)	// 接收数据中，等待处理完成  接收超时时间可以优化提高响应速度
    	{   							// 此处加接收超时机制  当命令码异常  一直查询数据，没有应答时候，取消命令码 需要优化	Read_count位置
    		if(TX_Timeout++>10 )  		// 300ms
    		{
    			Uart_Rxtype.Read_count =0;
    			TX_Timeout =0 ;
    		}
    	}
    	else //接收完成  下比数据的 发送
    	{
    		TX_Timeout = 0;
   //   	ESP_LOGI(Modbus_log, " 0 is IOT_ModbusTXTask %x _GucInvPollingStatus= %d !!!\r\n", InvCommCtrl.CMDFlagGrop ,_GucInvPollingStatus);
    	    if(IOT_GetInvUpdtStat() ==1)  // 升级逆变器
    	    {
    			IOT_ModbusSendHandle();
    		}
    		else
    		{
    			IOT_ESP_NEW_UpdataPV();
    		}
  //        ESP_LOGI(Modbus_log, "1 is IOT_ModbusTXTask %x _GucInvPollingStatus= %d !!!\r\n", InvCommCtrl.CMDFlagGrop ,_GucInvPollingStatus);
    	}
    	if( IOT_InCMDTime_uc(ESP_READ,0) ==ESP_OK1 )  //定时轮训03/04 数据 置为InvCommCtrl.CMDFlagGrop |= CMD_0X03;
    	{
    	   //接收到03/04 数据会赋值 ，否则接收超时也会10秒
    	   //ESP_LOGI(Modbus_log , " InvCommCtrl.CMDFlagGrop=%x  ,_GucInvPolling0304ACK =%d",InvCommCtrl.CMDFlagGrop ,_GucInvPolling0304ACK);
    	   IOT_InCMDTime_uc(ESP_WRITE,10);  	     //轮训时间数据间隔   更改到 PollingSwitch
    	   //InvCommCtrl.CMDFlagGrop |= CMD_0X03;  //定时获取03/04数据
    	   IOT_INVPollingEN();
    	}
    	if((IOT_INVgetSNtime_uc(ESP_READ ,0 ) == ESP_OK1)&& ( memcmp(InvCommCtrl.SN,"INVERSNERR",10)  ==0 ))
    	{
    		IOT_INVgetSNtime_uc(ESP_WRITE ,180 );
    		IOT_ESP_InverterSN_Get();
    	}

    	IOT_INVAdditionalFunc_Handle();  //一键诊断


    	if(InvCommCtrl.DTC != 0 && System_state.Pv_state == 1 &&  HoldingSentone_flag==0 )  // 在线标志位 需要轮训两次以上，避免03数据获取不全
    	{
    		HoldingSentone_flag=1;
    		InvCommCtrl.HoldingSentFlag = 1 ;        //主动上报 03 数据标记 只上报一次

    	    ESP_LOGI(Modbus_log , " HoldingSentone_flag=%d  ,_GucInvPolling0304ACK =%d",HoldingSentone_flag ,_GucInvPolling0304ACK);

    		ESP_LOGI(Modbus_log , "InvCommCtrl.DTC =%d",InvCommCtrl.DTC);
    		ESP_LOGI(Modbus_log , "InvCommCtrl.SN =%s",InvCommCtrl.SN);
    	//	IOT_ESP_InverterType_Get();  //范围值
    		IOT_DTCInit(InvCommCtrl.DTC);
    	}

#if DEBUG_HEAPSIZE

    	if(timer_logoe++>100)
    	{
    		timer_logoe=0;
    	   ESP_LOGI(Modbus_log, "is IOT_ModbusTXTask uxTaskGetStackHighWaterMark= %d !!!\r\n", uxTaskGetStackHighWaterMark(NULL));
    	}
#endif

    	vTaskDelay(100 / portTICK_PERIOD_MS); //100 ms  逆变器经常不回数据
    }
}


#if UART1_EVENTS

// USB 底层分包32 字节 触发事件

#define EX_UART_NUM UART_NUM_1
static QueueHandle_t uart1_queue;
#define PATTERN_CHR_NUM    (3)         /*!< Set the number of consecutive and identical characters received by receiver which defines a UART pattern*/

void IOT_UART1_events_init(void )
{
	  /* Configure parameters of an UART driver,
	     * communication pins and install the driver */
#if test_wifi
	    uart_config_t uart_config = {
	        .baud_rate = 115200,
	        .data_bits = UART_DATA_8_BITS,
	        .parity = UART_PARITY_DISABLE,
	        .stop_bits = UART_STOP_BITS_1,
	        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
	        .source_clk = UART_SCLK_APB,

	    };
#else
	    uart_config_t uart_config = {
	    	        .baud_rate = 9600,
	    	        .data_bits = UART_DATA_8_BITS,
	    	        .parity = UART_PARITY_DISABLE,
	    	        .stop_bits = UART_STOP_BITS_1,
	    	        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
	    	        .source_clk = UART_SCLK_APB,

	    	    };
#endif
	    //Install UART driver, and get the queue.
	    uart_driver_install(EX_UART_NUM, RX_BUF_SIZE * 2, RX_BUF_SIZE * 2, 20, &uart1_queue, 0);
	    uart_param_config(EX_UART_NUM, &uart_config);

	  //Set UART log level
	  //esp_log_level_set(Modbus_log, ESP_LOG_INFO);
	  //Set UART pins (using UART0 default pins ie no changes.)
	  // uart_set_pin(UART_NUM_1, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
	   uart_set_pin(EX_UART_NUM, TXD1_PIN, RXD1_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
	   uart_set_rx_timeout(EX_UART_NUM,120);
	  //Set uart pattern detect function.
	  // uart_enable_pattern_det_baud_intr(EX_UART_NUM, '+', PATTERN_CHR_NUM, 9, 0, 0);
	  //Reset the pattern queue length to record at most 20 pattern positions.
	   uart_pattern_queue_reset(EX_UART_NUM, 20);
}

void IOT_ModbusRXeventsTask(void *arg)
{
	 uart_event_t event;
	 size_t buffered_size;
     int rxBytes ;
	 uint8_t* dtmp = (uint8_t*) malloc(RX_BUF_SIZE);
	    for(;;)
	    {
	        //Waiting for UART event.
	        if(xQueueReceive(uart1_queue, (void * )&event, (portTickType)(150 / portTICK_PERIOD_MS) )) //portMAX_DELAY 接收超时时间 50ms  read_bytes 也占用一定的时间
	        {//9600 波特率接收时间 120字节 120ms   与115200  波特率 50ms  接收时间不一致
	            bzero(dtmp, RX_BUF_SIZE);

	            switch(event.type)
	            {
	             /*We'd better handler data event fast, there would be much more data events than
	              other types of events. If we take too much time on data event, the queue might
	              be full.*/
	             case UART_DATA:
	        //  ESP_LOGI(Modbus_log, "[UART DATA]: %d", event.size);
	             rxBytes = uart_read_bytes(EX_UART_NUM, dtmp, event.size, portMAX_DELAY);
				 memcpy(&RX_BUF[Uart_Rxtype.Size],dtmp,rxBytes );
				 Uart_Rxtype.Size +=rxBytes;
				 Uart_Rxtype.Read_count = 1;
				//   ESP_LOGI(Modbus_log, "  if [UART DATA]: %d    Uart_Rxtype.Size=%d    GetSystemTick_ms=%d", event.size ,Uart_Rxtype.Size  ,GetSystemTick_ms());
	          //     ESP_LOGI(Modbus_log, "[DATA EVT]:");
	         //      uart_write_bytes(EX_UART_NUM, (const char*) dtmp, event.size);
	                 break;
	                //Event of HW FIFO overflow detected
	                case UART_FIFO_OVF:
	                    ESP_LOGI(Modbus_log, "hw fifo overflow");
	                    // If fifo overflow happened, you should consider adding flow control for your application.
	                    // The ISR has already reset the rx FIFO,
	                    // As an example, we directly flush the rx buffer here in order to read more data.
	                    uart_flush_input(EX_UART_NUM);
	                    xQueueReset(uart1_queue);
	                    break;
	                //Event of UART ring buffer full
	                case UART_BUFFER_FULL:
	                    ESP_LOGI(Modbus_log, "ring buffer full");
	                    // If buffer full happened, you should consider encreasing your buffer size
	                    // As an example, we directly flush the rx buffer here in order to read more data.
	                    uart_flush_input(EX_UART_NUM);
	                    xQueueReset(uart1_queue);
	                    break;
	                //Event of UART RX break detected
	                case UART_BREAK:
	                    ESP_LOGI(Modbus_log, "uart rx break");
	                    break;
	                //Event of UART parity check error
	                case UART_PARITY_ERR:
	                    ESP_LOGI(Modbus_log, "uart parity error");
	                    break;
	                //Event of UART frame error
	                case UART_FRAME_ERR:
	                    ESP_LOGI(Modbus_log, "uart frame error");
	                    break;
	                //UART_PATTERN_DET
	                case UART_PATTERN_DET:
	                    uart_get_buffered_data_len(EX_UART_NUM, &buffered_size);
	                    int pos = uart_pattern_pop_pos(EX_UART_NUM);
	                    ESP_LOGI(Modbus_log, "[UART PATTERN DETECTED] pos: %d, buffered size: %d", pos, buffered_size);
	                    if (pos == -1){
	                        // There used to be a UART_PATTERN_DET event, but the pattern position queue is full so that it can not
	                        // record the position. We should set a larger queue size.
	                        // As an example, we directly flush the rx buffer here.
	                        uart_flush_input(EX_UART_NUM);
	                    } else {
	                        uart_read_bytes(EX_UART_NUM, dtmp, pos, 100 / portTICK_PERIOD_MS);
	                        uint8_t pat[PATTERN_CHR_NUM + 1];
	                        memset(pat, 0, sizeof(pat));
	                        uart_read_bytes(EX_UART_NUM, pat, PATTERN_CHR_NUM, 100 / portTICK_PERIOD_MS);
	                        ESP_LOGI(Modbus_log, "read data: %s", dtmp);
	                        ESP_LOGI(Modbus_log, "read pat : %s", pat);
	                    }
	                    break;
	                //Others
	                default:
	                    ESP_LOGI(Modbus_log, "uart event type: %d", event.type);
	                    break;
	            }
	        //    ESP_LOGI(Modbus_log, "uart[%d] event:  event.timeout_flag %d  event.size=%d", EX_UART_NUM,event.timeout_flag,event.size);
	          //  event.type=0;
	        //    event.timeout_flag=0;
	          //  event.size=0;
	        }
	        else
	        {
	        //	ESP_LOGI(Modbus_log, "  else [UART DATA]: %d    Uart_Rxtype.Size=%d    GetSystemTick_ms=%d", event.size ,Uart_Rxtype.Size  ,GetSystemTick_ms());

	 	 	 	 if(Uart_Rxtype.Read_count == 1  )
	 	 	 	 {
	 	 	 		Uart_Rxtype.Read_count =0;
	 	 	 		Uart_Rxtype.ReadDone=1;
                    RX_BUF[Uart_Rxtype.Size] = 0;   					// 在串口接收的数据增加结束符
	              	if(IOT_GetInvUpdtStat()==1)  //没有升级
	                {
	              	  IOT_ModbusReceiveHandle(RX_BUF,Uart_Rxtype.Size);	//处理接收到的 数据
	                }
	              	else
	                {
	              	  memcpy(IOTESPUsart0RcvBuf,RX_BUF,Uart_Rxtype.Size);
	              	  IOTESPUsart0RcvLen = Uart_Rxtype.Size;
	              	}
#if DEBUG_UART
	                ESP_LOGI(Modbus_log, " UART_DATA Read %d ", Uart_Rxtype.Size);
	                for(int i = 0; i < Uart_Rxtype.Size; i++)
	                {
	                	IOT_Printf("%02x ",RX_BUF[i]);
	                }
	                ESP_LOGI(Modbus_log, "*******\r\n" );
#endif

	           //   处理完成数据清零
	             	if((InverterWaveStatus_Judge(1,0) == IOT_ING) || \
	             	  (InverterWaveStatus_Judge(1,0) == IOT_BUSY))  //  是否有波形在请求
	             	{}
	             	else
	             	{
	             		Uart_Rxtype.Read_count=0 ; // 数据解析完成 清零，下次轮训
	             		Uart_Rxtype.Size=0;		   // 数据长度清0
	                	memset(RX_BUF,0,RX_BUF_SIZE);
	             	}
	 	 	 	 }
	        }
	   //     ESP_LOGI(Modbus_log, "1uart[%d] event:  event.timeout_flag %d  event.size=%d", EX_UART_NUM,event.timeout_flag,event.size);
	    }
	    free(dtmp);
	    dtmp = NULL;
	    vTaskDelete(NULL);
}

#endif



