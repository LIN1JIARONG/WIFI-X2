/*
 * iot_Local_Protocol.c
 *
 *  Created on: 2022年8月02日
 *      Author: grt-chenyan
 */


#include "iot_Local_Protocol.h"
#include "iot_protocol.h"
#include "iot_universal.h"

#include "mbedtls/aes.h"
#include "esp_log.h"
#include "iot_system.h"
#include "iot_inverter.h"
#include "iot_station.h"
#include "iot_fota.h"
#include "iot_local_update.h"
#include "iot_InvWave.h"


#define LOCAL_RTOTOCOL "LOCAL_RTOTOCOL"

#define LOCAL_RCV_BUFF_SIZE    1536

/*******************************************************************************
 * FunctionName : IOT_AES_CBC_encrypt_ui
 * Description  : AES_CBC 加密
 * Parameters   : uint8_t ucmode_tem	模式  1 加密 , 2 解密
 * 				  unsigned char *pucAesInputData_tem	输入数据
				  uint32_t InputDataLen					输入数据的长度
				  unsigned char *  pucAesOutputData		输出的数据
 * Returns      : iERR_t
 * Notice       : none
*******************************************************************************/
mbedtls_aes_context pSAesCtx;
mbedtls_aes_context pSAesCtx_1;

#if LOCAL_PROTOCOL_6_EN
#define uiAaeskeyLen  (16 + 1)
#define AES_KEY_BITS  128
const unsigned char _g_ucAesKeys[uiAaeskeyLen] = {AES_KEY_128BIT};
#else
#define uiAaeskeyLen  (32 + 1)
#define AES_KEY_BITS  256

const unsigned char _g_ucAesKeys[uiAaeskeyLen] = "1234567890#1234567890#1234567890\0";

#endif

uint32_t IOT_AES_CBC_encrypt_ui(uint8_t ucmode_tem ,unsigned char *pucAesInputData_tem , \
								uint32_t InputDataLen , unsigned char *pucAesOutputData)
{
	int iERR_t = 0;

#if LOCAL_PROTOCOL_6_EN
	unsigned char ucAesIvs[16 + 1]   = "growatt_aes16Ivs\0";  	//加密用 不能共用，调用完后会被修改值，需要重新赋值
	unsigned char ucAesIvs_d[16 + 1] = "growatt_aes16Ivs\0";	//解密用

#else
	unsigned char ucAesIvs[16 + 1] = "1234567890123456\0";  	//加密用 不能共用，调用完后会被修改值，需要重新赋值
	unsigned char ucAesIvs_d[16 + 1] = "1234567890123456\0";	//解密用
#endif

	if(1 == ucmode_tem)		//加密
	{
		mbedtls_aes_init(&pSAesCtx);
		mbedtls_aes_setkey_enc(&pSAesCtx, _g_ucAesKeys, AES_KEY_BITS);

		iERR_t = mbedtls_aes_crypt_cbc(&pSAesCtx,
											  MBEDTLS_AES_ENCRYPT,
											  InputDataLen,
											  ucAesIvs,
											  pucAesInputData_tem,
											  pucAesOutputData);
	}
	else if(2 == ucmode_tem)	//解密
	{
		mbedtls_aes_init(&pSAesCtx_1);
		mbedtls_aes_setkey_dec(&pSAesCtx_1, _g_ucAesKeys, AES_KEY_BITS);
		iERR_t = mbedtls_aes_crypt_cbc(&pSAesCtx_1,
											  MBEDTLS_AES_DECRYPT,
											  InputDataLen,
											  ucAesIvs_d,
											  pucAesInputData_tem,
											  pucAesOutputData);

	}

	return iERR_t;
}





uint16_t IOT_Repack_ProtocolData_V5ToV6_us(uint8_t *pucData_tem , uint16_t usDataLen_tem , uint8_t *pucOut_tem)
{
	uint16_t usTotalLenght = 0;
	uint16_t usProtocolVersion = ProtocolVS06;
	uint16_t usCRC_t  = 0;

	uint16_t encrypt_len = 0;
	uint16_t usDataLen = 0;
	uint8_t  DeviceAddr = 0;
	uint8_t  FuncionCode = 0;

	uint8_t *pbuff = NULL;
	uint8_t *pAESOutPutbuff = NULL;

	usDataLen = pucData_tem[4];
	usDataLen = (usDataLen << 8 ) + pucData_tem[5];

	#if 1
		ESP_LOGI(LOCAL_RTOTOCOL , "IOT_Repack_ProtocolData_V5ToV6_us len = %d ,pucData_tem :" ,usDataLen_tem);
		for(int i = 0; i < usDataLen_tem ; i++ )
		{
			IOT_Printf(" %02x ",pucData_tem[i]);
		}
		IOT_Printf("\r\n");
	#endif

	if((usDataLen < 8 ) ||((usDataLen + 6) != usDataLen_tem))
	{
		ESP_LOGI(LOCAL_RTOTOCOL , "IOT_Repack_ProtocolData_V5ToV6_us usDataLen = %d ,usDataLen_tem = %d" ,usDataLen,usDataLen_tem);
		return 0;
	}

	pbuff = (uint8_t *)malloc(usDataLen_tem  + 16 );  //最多需要补15个零

	if(NULL == pbuff)
	{
		usTotalLenght =  0;
		goto END;
	}

	bzero(pbuff , usDataLen_tem  + 16);
	memcpy(pbuff , pucData_tem , usDataLen_tem);

	usTotalLenght = usDataLen + 6;			//加上总长度和 协议版本以及实际数据长度字段所占用的长度
	encrypt_len = usTotalLenght - 8;		//得到加密的数据长度

	while((encrypt_len % 16 )!= 0)	//加密的数据长度不能整除16 , 需要补零
	{
		pbuff[usTotalLenght++] = 0x00;
		encrypt_len++;

		if(usTotalLenght >= (usDataLen_tem  + 16))
		{
			break;
		}
	}

	pAESOutPutbuff = (uint8_t *)malloc(encrypt_len);

	if(NULL == pAESOutPutbuff)
	{
		usTotalLenght =  0;
		goto END;
	}
	bzero(pAESOutPutbuff , encrypt_len);

	IOT_AES_CBC_encrypt_ui(1 , &pbuff[8] ,encrypt_len, pAESOutPutbuff);


	memcpy(&pbuff[8] , pAESOutPutbuff , encrypt_len );
	memcpy(pucOut_tem ,pbuff , usTotalLenght );


	pucOut_tem[0] = HIGH(usTotalLenght);
	pucOut_tem[1] = LOW(usTotalLenght);

	pucOut_tem[2] = HIGH(usProtocolVersion);
	pucOut_tem[3] = LOW(usProtocolVersion);

	usCRC_t = Modbus_Caluation_CRC16(pucOut_tem, usTotalLenght);
	pucOut_tem[usTotalLenght]     = HIGH(usCRC_t);
	pucOut_tem[usTotalLenght + 1] = LOW(usCRC_t);


	usTotalLenght += 2;

#if 1
		ESP_LOGI(LOCAL_RTOTOCOL , "after encrypt:" ,usDataLen_tem);
		for(int i = 0; i < usTotalLenght ; i++ )
		{
			IOT_Printf(" %02x ",pucOut_tem[i]);
		}
		IOT_Printf("\r\n");
	#endif

END:
	if(NULL != pbuff)
		free(pbuff);
	if(NULL != pAESOutPutbuff)
		free(pAESOutPutbuff);

	return usTotalLenght;	//返回的长度不包括 总长度字段 , 包括CRC16校验码字段的长度

}


/* 本地通讯协议数据解密
 *
 * */

uint16_t IOT_Repack_LocalProtocolData_Decrypt_us(uint8_t *pucData_tem , uint16_t usDataLen_tem , uint8_t *pucOut_tem)
{
	uint16_t usRtLen = 0;
	uint16_t usTotalLenght = 0;
	uint16_t encrypt_len = 0;

	uint8_t *pAESOutPutbuff = NULL;

#if DEBUG_BLE
	ESP_LOGI(LOCAL_RTOTOCOL , "usDataLen_tem = %d ,pucData_tem :" ,usDataLen_tem);
	for(int i = 0; i < usDataLen_tem ; i++ )
	{
		IOT_Printf(" %02x ",pucData_tem[i]);
	}
	IOT_Printf("\r\n");
#endif

	if((NULL == pucData_tem) || (NULL == pucOut_tem) || (0 == usDataLen_tem))
	{
		return 0;
	}

	uint8_t *pbuff = (uint8_t *)malloc(usDataLen_tem);
	if(NULL == pbuff)
	{
		usRtLen =  0;
		goto END;
	}
	bzero(pbuff , usDataLen_tem);

	memcpy(pbuff , pucData_tem , usDataLen_tem);


	ESP_LOGI(LOCAL_RTOTOCOL , "LOCAL_PROTOCOL = 0x%x " ,LOCAL_PROTOCOL);

	if((LOCAL_PROTOCOL & 0x00ff)  == ProtocolVS05)		// 5.0 协议全部数据都加密
	{
		if((usDataLen_tem % 16) != 0)
		{
			usRtLen =  0;
			goto END;
		}

		pAESOutPutbuff = (uint8_t *)malloc(usDataLen_tem);
		if(pAESOutPutbuff == NULL)
		{
			usRtLen =  0;
			goto END;
		}
		bzero(pAESOutPutbuff , encrypt_len);
		IOT_AES_CBC_encrypt_ui(2 ,pbuff,usDataLen_tem ,pAESOutPutbuff);	//AES 解密
		memcpy(pucOut_tem , pAESOutPutbuff , usDataLen_tem);
		usRtLen = usDataLen_tem;

	}else
	{
		usTotalLenght = pbuff[0];
		usTotalLenght = (usTotalLenght << 8 ) + pbuff[1];

		if(Modbus_Caluation_CRC16(pbuff, usTotalLenght) != MERGE(pbuff[usTotalLenght], pbuff[usTotalLenght+1]))
		{
			ESP_LOGI(LOCAL_RTOTOCOL , "IOT_Repack_LocalProtocolData_Decrypt_us CRC check Failed" );
			usRtLen =  0;
			goto END;
		}

		ESP_LOGI(LOCAL_RTOTOCOL , "usTotalLenght = %d " ,usTotalLenght);

		encrypt_len = usTotalLenght - 6 - 2 ;  // 减去协议头和CRC16字段的长度
		if((encrypt_len % 16) != 0)
		{
			usRtLen =  0;
			goto END;
		}

		pAESOutPutbuff = (uint8_t *)malloc(encrypt_len);
		if(pAESOutPutbuff == NULL)
		{
			usRtLen =  0;
			goto END;
		}
		bzero(pAESOutPutbuff , encrypt_len);

		IOT_AES_CBC_encrypt_ui(2 ,&pbuff[8],encrypt_len ,pAESOutPutbuff);	//AES 解密

		memcpy(&pbuff[8] , pAESOutPutbuff , encrypt_len);

		memcpy(pucOut_tem , pbuff , usDataLen_tem);

		usRtLen = usDataLen_tem;
	}


#if DEBUG_BLE
	ESP_LOGI(LOCAL_RTOTOCOL , "usRtLen = %d ,pucOUTPUTData_tem :" ,usRtLen);
	for(int i = 0; i < usDataLen_tem ; i++ )
	{
		IOT_Printf(" %02x ",pucOut_tem[i]);
	}
	IOT_Printf("\r\n");
#endif

END:
	if(NULL != pAESOutPutbuff)
	{
		free(pAESOutPutbuff);
	}
	if(NULL != pbuff)
	{
		free(pbuff);
	}

	return usRtLen;

}




/**********************************
 * IOT_ServerReceiveHandle
 * 服务器publish 接收处理
 * return  _g_EMQTTREQType
 **********************************/
EMQTTREQ IOT_LocalReceiveHandle(uint8_t* pucBuf_tem, uint16_t suiLen_tem)
{
	uint8_t ParamOffset = 0; // SN参数偏移量
	uint8_t DataOffset =0;
	uint8_t *p=NULL;
	uint16_t tempCID = 0;
	uint16_t tempLen = 0;
	EMQTTREQ EMQTTREQTypeVal = MQTTREQ_NULL;

	int i = 0 ,j = 0;

	ParamOffset = 0;// 在原来基础上加了20个采集器序列号字符  //本地通讯只有

	p =pucBuf_tem ;
	/* 通讯编号校验 */
	tempCID = p[0];
	tempCID = (tempCID<<8) | p[1];
	tempLen = p[4];
	tempLen = (tempLen << 8) | p[5];
	tempLen += 6;		//本次数据长度,这个长度是没有添加CRC校验的2byte

	IOT_Printf("\r\n*******IOT_ServerReceiveHandle --a********ReceiveHandle***suiLen_tem=%d****************\r\n ",suiLen_tem);
   	/* Print response directly to stdout as it is read */

#if DEBUG_BLE
   	for(int i = 0; i < suiLen_tem; i++) {
   		IOT_Printf("%02x ",pucBuf_tem[i]);
   	}
#endif
	if(tempLen < LOCAL_RCV_BUFF_SIZE)
	{
//		if(Modbus_Caluation_CRC16(p, tempLen) != MERGE(p[tempLen], p[tempLen+1]))
//		{
//			printf("\r\n************server publish CRC error*************\r\n");
//		 	return  EMQTTREQTypeVal;
//		}
	}
	else
	{
		return EMQTTREQTypeVal;
	}

	if((System_state.BLEKeysflag==0 ) &&(p[7] != 0x18) )  //没有发送密钥
	{
		IOT_Printf("\r\n**********System_state.BLEKeysflag==0****p[7] != 0x18***********\r\n ");
		return EMQTTREQTypeVal;
	}


#if DEBUG_BLE
	IOT_Printf("\r\n**********IOT_LocalReceiveHandle********suiLen_tem=%d***************\r\n ",suiLen_tem);
   	/* Print response directly to stdout as it is read */
   	for(int i = 0; i < suiLen_tem; i++) {
   		IOT_Printf("%02x ",pucBuf_tem[i]);
   	}
#endif
	if(p[7] == 0x19) 	  					// 服务器 查询 采集器参数的指令
	{
		EMQTTREQTypeVal=MQTTREQ_CMD_0x19;
		System_state.Host_setflag |= CMD_0X19;
		InvCommCtrl.CMDFlagGrop |= CMD_0X19;
		System_state.ServerGET_REGNUM = ((unsigned int)p[18 + ParamOffset]<<8) + p[19 + ParamOffset];    		  // 寄存器编号个数
		IOT_Printf("\r\n******************System_state.ServerGET_REGNUM =%d***************\r\n ",System_state.ServerGET_REGNUM );

		if(	System_state.ServerGET_REGNUM ==1)
		{
			System_state.ServerGET_REGAddrStart = ((unsigned int)p[20 + ParamOffset]<<8) + p[21 + ParamOffset];
			System_state.ServerGET_REGAddrEnd = ((unsigned int)p[20 + ParamOffset]<<8) + p[21+ ParamOffset];
		}
		else
		{
			 for(i=0 , j=0 ;i < System_state.ServerGET_REGNUM  ;i++,j++)
			 {
				System_state.ServerGET_REGAddrStart = ((unsigned int)p[20+i+j + ParamOffset]<<8) + p[21+i+j + ParamOffset];       //终止参数编号
				System_state.ServerGET_REGAddrBUFF[i]=System_state.ServerGET_REGAddrStart ;

			 }
		}
		IOT_Printf("\r\n******************System_state.ServerGET_REGAddrStart=%d***************\r\n ",System_state.ServerGET_REGAddrStart);
		IOT_Printf("\r\n******************System_state.ServerGET_REGAddrEnd =%d***************\r\n ",System_state.ServerGET_REGAddrEnd );

 		if(System_state.ServerGET_REGAddrStart == 75)
		{
		   IOT_WIFIEvenSetSCAN();
		}
	}
	else if(p[7] == 0x18) // 服务器 设置 采集器参数的指令
	{
		EMQTTREQTypeVal = MQTTREQ_CMD_0x18;
		System_state.Host_setflag |= CMD_0X18;
	 	InvCommCtrl.CMDFlagGrop |= CMD_0X18;
		System_state.ServerSET_REGNUM = ((p[18 + ParamOffset]<<8) + p[19 + ParamOffset] );
		System_state.ServerSET_REGAddrLen = 0;
		for(i=0; i<System_state.ServerSET_REGNUM ;i++)
		{
			System_state.ServerSET_REGAddrStart = ((unsigned int)p[22  + ParamOffset +DataOffset]<<8) + p[23+ ParamOffset+DataOffset]; //起始参数编号
			System_state.ServerSET_REGAddrLen =  ((unsigned int)p[24 + ParamOffset+DataOffset]<<8) + p[25 + ParamOffset+DataOffset];
			IOT_Printf("\r\n******************ServerSET_REGAddrStart=%d******ServerSET_REGAddrLen=%d*********\r\n ",System_state.ServerSET_REGAddrStart,System_state.ServerSET_REGAddrLen);

			if((System_state.ble_onoff==1 )&&(System_state.BLEKeysflag==0 ) &&(System_state.ServerSET_REGAddrStart != 54 ) )  //没有发送密钥
			{
				IOT_Printf("\r\n**********System_state.BLEKeysflag==0****System_state.ServerSET_REGAddrStart!= 54***********\r\n ");
				System_state.Host_setflag  &= ~CMD_0X18;
				InvCommCtrl.CMDFlagGrop &= ~CMD_0X18;
				return EMQTTREQTypeVal;
			}


			  if(System_state.ServerSET_REGAddrLen > 0)//APP高级设置     17 会清空 19    // 长度是 0//APP高级设置     19 会清空 17    // 长度是 0
			  {
				 if((System_state.ServerSET_REGAddrStart == 17 )||(System_state.ServerSET_REGAddrStart==19))
				 {
					 IOT_SystemParameterSet(17 ,(char *) &p[26 + ParamOffset+DataOffset],System_state.ServerSET_REGAddrLen );  //保存数据  同时保存
					 IOT_SystemParameterSet(19 ,(char *) &p[26 + ParamOffset+DataOffset],System_state.ServerSET_REGAddrLen );  //保存数据
				 }
				 else if(System_state.ServerSET_REGAddrStart==18)
				 {
					 IOT_SystemParameterSet(18 ,(char *) &p[26 + ParamOffset+DataOffset],System_state.ServerSET_REGAddrLen );  //保存数据
				 }
				 else
				 {
					 IOT_ServerFunc_0x18(System_state.ServerSET_REGAddrStart,System_state.ServerSET_REGAddrLen,&p[26 + ParamOffset+DataOffset]);
				 }
			  }

			DataOffset = System_state.ServerSET_REGAddrLen + 4;

			if(80 == System_state.ServerSET_REGAddrStart)	//如果为80号寄存器,需要确认升级的方式
			{
				g_FotaParams.uiWayOfUpdate = ELOCAL_UPDATE;  //本地升级
			}

		}//ACK?
	}
	else if(p[7] == 0x17) // 透传命令
	{
		EMQTTREQTypeVal=MQTTREQ_CMD_0x17;
		System_state.Host_setflag |= CMD_0X17;
		InvCommCtrl.CMDFlagGrop |= CMD_0X17;
		InvCommCtrl.SetParamDataLen=  ((unsigned int)p[18 + ParamOffset]<<8) + p[19 + ParamOffset]; // 透传区数据长度
	    memcpy( InvCommCtrl.SetParamData, &p[20 + ParamOffset], InvCommCtrl.SetParamDataLen );
	}
	else if(p[7] == 0x26) // 升级
	{
		IOT_stoppolling();
		IOT_LocalTCPCode0x26_Handle(&p[18 + ParamOffset]);	//本地升级 数据处理
		EMQTTREQTypeVal=MQTTREQ_CMD_0x26;
		System_state.Host_setflag |= CMD_0X26;
		InvCommCtrl.CMDFlagGrop |= CMD_0X26;
		//IOT_GattsEvenSetR();    				// 触发应答APP 18命令
	}
	else if(p[7] == 0x05)
	{
		IOT_stoppolling();
		EMQTTREQTypeVal=MQTTREQ_CMD_0x05;
		System_state.Host_setflag |= CMD_0X05;
		InvCommCtrl.CMDFlagGrop |= CMD_0X05;
		InvCommCtrl.SetParamStart = MERGE(p[18 + ParamOffset], p[19 + ParamOffset]);
		InvCommCtrl.SetParamEnd = MERGE(p[20 + ParamOffset], p[21 + ParamOffset]);
		IOT_Printf("\r\n*******CMD_0x05***********InvCommCtrl.SetParamStart=%d***************\r\n ",InvCommCtrl.SetParamStart);
		IOT_Printf("\r\n******************InvCommCtrl.SetParamEnd =%d***************\r\n ",InvCommCtrl.SetParamEnd  );
	}
	else if(p[7] == 0x06)
	{
		IOT_stoppolling();
		EMQTTREQTypeVal=MQTTREQ_CMD_0x06;
		System_state.Host_setflag |= CMD_0X06;
		InvCommCtrl.CMDFlagGrop |= CMD_0X06;

		InvCommCtrl.Set_InvParameterNum = MERGE(p[18 + ParamOffset], p[19 + ParamOffset]);
		InvCommCtrl.Set_InvParameterData = MERGE(p[20 + ParamOffset], p[21 + ParamOffset]);
		IOT_Printf("\r\n******CMD_0x06************InvCommCtrl.Set_InvParameterNum=%d***************\r\n ",InvCommCtrl.Set_InvParameterNum);
		IOT_Printf("\r\n******************InvCommCtrl.Set_InvParameterData =%d***************\r\n ",InvCommCtrl.Set_InvParameterData  );

	}
	else if(p[7] == 0x10)
	{
		IOT_stoppolling();
		EMQTTREQTypeVal=MQTTREQ_CMD_0x10;

		InvCommCtrl.SetParamStart = MERGE(p[18 + ParamOffset], p[19 + ParamOffset]);
		InvCommCtrl.SetParamEnd = MERGE(p[20 + ParamOffset], p[21 + ParamOffset]);
		//1500V 多机指令兼容
		if(p[22+ParamOffset] == '#')
		{
			InvCommCtrl.SetParamDataLen = (InvCommCtrl.SetParamEnd - InvCommCtrl.SetParamStart + 1) * 2 ;
			p[22+ParamOffset] = 0x00;
			p[23+ParamOffset] = 0x01;
			memcpy(InvCommCtrl.SetParamData, &p[22+ParamOffset], InvCommCtrl.SetParamDataLen);
		}
		else
		{
			InvCommCtrl.SetParamDataLen = (InvCommCtrl.SetParamEnd - InvCommCtrl.SetParamStart + 1) * 2;
			memcpy(InvCommCtrl.SetParamData, &p[22+ParamOffset], InvCommCtrl.SetParamDataLen);
		}
		/* 判断获取波形  */
		if(InverterWave_Judge(InvCommCtrl.SetParamStart,InvCommCtrl.SetParamEnd))
		{
			IOT_Printf("\r\n\r\n\r\n\r\n*******InverterWave_Judge***************\r\n\r\n\r\n\r\n\r\n");
			IOT_PINGRESPSet_uc(ESP_WRITE,Mqtt_PINGTime);   // 重新装载心跳时间// 如果获取波形请求时，延时心跳包发送，直到波形请求结束。。。
			IOT_TCPDelaytimeSet_uc(ESP_WRITE,System_state.Up04dateTime+1);// 延时04数据上传
		}

		System_state.Host_setflag|= CMD_0X10;
		InvCommCtrl.CMDFlagGrop |= CMD_0X10;

		IOT_Printf("\r\n*******CMD_0x10***********InvCommCtrl.SetParamStart=%d***************\r\n ",InvCommCtrl.SetParamStart);
		IOT_Printf("\r\n******************InvCommCtrl.SetParamEnd =%d***************\r\n ",InvCommCtrl.SetParamEnd  );
		IOT_Printf("\r\n******************InvCommCtrl.SetParamDataLen =%d***************\r\n ",InvCommCtrl.SetParamDataLen  );
	}
	else if(p[7] == 0x14)
	{
		IOT_ServerACK_0x14();
	}


	p = NULL;
	return EMQTTREQTypeVal;
}




