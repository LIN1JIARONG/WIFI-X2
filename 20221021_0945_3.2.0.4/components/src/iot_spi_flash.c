/*
 * iot_spi_flash.c
 *
 *  Created on: 2021��11��16��
 *      Author: Administrator
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "iot_spi_flash.h"
#include <sys/param.h>
#include "sdkconfig.h"

#include "iot_system.h"

#define Flash_HOST    SPI2_HOST
#define DMA_CHAN      Flash_HOST

#define PIN_NUM_CS   10
#define PIN_NUM_MISO 2
#define PIN_NUM_MOSI 7
#define PIN_NUM_CLK  6

#define W25QXX_CS_L gpio_set_level(PIN_NUM_CS, 0);
#define W25QXX_CS_H gpio_set_level(PIN_NUM_CS, 1);

static spi_device_handle_t g_spi;

#if CONFIG_IDF_TARGET_ESP32
#define SPI_HOST HSPI_HOST
#elif CONFIG_IDF_TARGET_ESP32S2
#define SPI_HOST SPI2_HOST
#elif defined CONFIG_IDF_TARGET_ESP32C3
#define SPI_HOST SPI2_HOST
#endif

int VprocHALInit(void)
{
	/*if the customer platform requires any init
    * then implement such init here.
    * Otherwise the implementation of this function is complete
    */
	esp_err_t ret = ESP_OK;

	spi_bus_config_t buscfg = {
		.miso_io_num = PIN_NUM_MISO,
		.mosi_io_num = PIN_NUM_MOSI,
		.sclk_io_num = PIN_NUM_CLK,
		.quadwp_io_num = -1,
		.quadhd_io_num = -1};

	spi_device_interface_config_t devcfg = {
		//.clock_speed_hz = 20 * 1000 * 1000, //Clock out at 20 MHz
		.clock_speed_hz = 4 * 1000 * 1000,  //Clock out at 4 MHz
		//.mode = 3,						//SPI mode 3     (CPOL, CPHA) - 3: (1, 1)
		.mode = 3,
		//.mode = 0,						//SPI mode 3     (CPOL, CPHA) - 0: (0, 0)
		.spics_io_num = -1,					//GPIO_NUM_15,             	   // CS pin
		.queue_size = 6,					//queue 7 transactions at a time
	};

	//Initialize the SPI bus
	if (g_spi)
	{
		return ret;
	}
	ret = spi_bus_initialize(SPI_HOST, &buscfg, 0);
	assert(ret == ESP_OK);
	ret = spi_bus_add_device(SPI_HOST, &devcfg, &g_spi);
	assert(ret == ESP_OK);
	gpio_set_pull_mode(PIN_NUM_CS, GPIO_FLOATING);
	return ret;
}

/*HAL clean up function - To close any open file connection
 * microsemi_spis_tw kernel char driver
 *
 * return: a positive integer value for success, a negative integer value for failure
 */

void VprocHALcleanup(void)
{
	/*if the customer platform requires any cleanup function
    * then implement such function here.
    * Otherwise the implementation of this function is complete
    */
	int ret = 0;
	ret = spi_bus_remove_device(g_spi);
	assert(ret == ESP_OK);
	ret = spi_bus_free(SPI_HOST);
	assert(ret == ESP_OK);

}

/* This is the platform dependent low level spi
 * function to write 16-bit data to the ZL380xx device
 */
int VprocHALWrite(uint8_t val)
{
	/*Note: Implement this as per your platform*/
	esp_err_t ret;
	spi_transaction_t t;
	// unsigned short data = 0;
	memset(&t, 0, sizeof(t));		 //Zero out the transaction
	t.length = sizeof(uint8_t) * 8; //Len is in bytes, transaction length is in bits.

	t.tx_buffer = &val;

	ret = spi_device_transmit(g_spi, &t); //Transmit
	assert(ret == ESP_OK);

	return 0;
}

/* This is the platform dependent low level spi
 * function to read 16-bit data from the ZL380xx device
 */
int VprocHALRead(uint8_t *pVal)
{
	/*Note: Implement this as per your platform*/
	esp_err_t ret;
	spi_transaction_t t;
	// unsigned short data = 0xFFFF;
	uint8_t data1 = 0xFF;

	memset(&t, 0, sizeof(t)); //Zero out the transaction
	/*t.length = sizeof(unsigned short) * 8;
    t.rxlength = sizeof(unsigned short) * 8; //The unit of len is byte, and the unit of length is bit.
    t.rx_buffer = &data;*/
	t.length = sizeof(uint8_t) * 8;
	t.rxlength = sizeof(uint8_t) * 8; 		//The unit of len is byte, and the unit of length is bit.
	t.rx_buffer = &data1;
	ret = spi_device_transmit(g_spi, &t); //Transmit!
	*pVal = data1;
	//*pVal = ntohs(data);
	assert(ret == ESP_OK);
	return 0;
}

void IOT_SPI_FLASH_Init(void)
{
	uint16_t err_count = 0;
	uint16_t W25QXX_TYPE = 0;	   //Ĭ����W25Q128
	gpio_config_t io_conf;
	//disable interrupt
	io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
	//set as output mode
	io_conf.mode = GPIO_MODE_OUTPUT;
	//bit mask of the pins that you want to set,e.g.GPIO18/19
	io_conf.pin_bit_mask = (1ULL << PIN_NUM_CS);
	//disable pull-down mode
	io_conf.pull_down_en = 0;
	//disable pull-up mode
	io_conf.pull_up_en = 0;
	//configure GPIO with the given settings
	gpio_config(&io_conf);

	VprocHALInit();


	vTaskDelay(100 / portTICK_PERIOD_MS);

	W25QXX_CS_H;
	vTaskDelay(100 / portTICK_PERIOD_MS);
	W25QXX_TYPE = W25QXX_ReadID(); //��ȡFLASH ID.
	IOT_Printf("IOT_SPI_FLASH_Init W25QXX_ReadID  = 0x%x \n", W25QXX_TYPE);


	W25QXX_CS_H;
	vTaskDelay(100 / portTICK_PERIOD_MS);

	W25QXX_TYPE = W25QXX_ReadID(); //��ȡFLASH ID.

	IOT_Printf("IOT_SPI_FLASH_Init W25QXX_ReadID  = 0x%x \n", W25QXX_TYPE);

	while((W25QXX_ReadSR() & 0X01) == 0X01)
	{
		if(++err_count > 2000)
		{
			IOT_Printf("IOT_W25Qxx_Init FLASH error \r\n"); /*FLASH оƬ��*/
			break;
		}
	}
 	W25QXX_Write_Enable(); //SPI_FLASHдʹ��
 	W25QXX_Write_SR(0);
 	W25QXX_Wait_Busy();
}


/*************************************************************
* ������: W25QXX_ReadSR
* ��  ��: ��ȡFLASH״̬�Ĵ���
* ��  ��: none
* ����ֵ: none
* ע  ��: BIT7  6   5   4   3   2   1   0
	      SPR   RV  TB BP2 BP1 BP0 WEL BUSY
   SPR:Ĭ��Ϊ0,״̬�Ĵ�������λ���WPʹ��
   TB,BP2,BP1,BP0:FLASH����д��������
   WEL:дʹ������
   BUSY:æ���λ: 1:æ; 0:����,Ĭ��
************************************** ***********************/
uint8_t W25QXX_ReadSR(void)
{
	uint8_t byte = 0;
	/*W25QXX_CS_L;                            //ʹ������
	SPI_ReadWriteByte(W25X_ReadStatusReg);    //���Ͷ�ȡ״̬�Ĵ�������
	byte=SPI_ReadWriteByte(0Xff);             //��ȡһ���ֽ�
	W25QXX_CS_H;                              //ȡ��Ƭѡ
	*/
	W25QXX_CS_L;
	VprocHALWrite(W25X_ReadStatusReg);
	VprocHALRead(&byte);
	W25QXX_CS_H;

	return byte;
}

/*************************** **************************
* ������: W25QXX_Write_SR
* ��  ��: дSPI_FLASH״̬�Ĵ���
* ��  ��: none
* ����ֵ: none
* ע  ��: ֻ��SPR,TB,BP2,BP1,BP0(bit 7,5,4,3,2)����д
**************************** *****************************/
void W25QXX_Write_SR(uint8_t sr)
{
	W25QXX_CS_L; //ʹ������
	//SPI_ReadWriteByte(W25X_WriteStatusReg);   //����дȡ״̬�Ĵ�������
	//SPI_ReadWriteByte(sr);               //д��һ���ֽ�
	VprocHALWrite(W25X_WriteStatusReg);
	VprocHALWrite(sr);
	W25QXX_CS_H; //ȡ��Ƭѡ
}
/************* **************************************
* ������: W25QXX_Write_Enable
* ��  ��: SPI_FLASHдʹ��
* ��  ��: none
* ����ֵ: none
* ע  ��: none//��WEL��λ
************** *************************************/
void W25QXX_Write_Enable(void)
{
	W25QXX_CS_L; //ʹ������
	//SPI_ReadWriteByte(W25X_WriteEnable);      //����дʹ��
	VprocHALWrite(W25X_WriteEnable);
	W25QXX_CS_H; //ȡ��Ƭѡ
}
/************** **************************
* ������: W25QXX_Write_Disable
* ��  ��: SPI_FLASHдʹ��
* ��  ��: none
* ����ֵ: none
* ע  ��: none//��WEL����
*************** *********************/
void W25QXX_Write_Disable(void)
{
	W25QXX_CS_L; //ʹ������
	//SPI_ReadWriteByte(W25X_WriteDisable);     //����д��ָֹ��
	VprocHALWrite(W25X_WriteDisable);
	W25QXX_CS_H; //ȡ��Ƭѡ
}

/******************* ****************************************
* ������: W25Qxx_WaitBusy
* ��  ��: �ȴ�����
* ��  ��: none
* ����ֵ: none
* ע  ��: ֻ��SPR,TB,BP2,BP1,BP0(bit 7,5,4,3,2)����д
********************* ****************************************/
void W25QXX_Wait_Busy(void)
{
	while ((W25QXX_ReadSR() & 0x01) == 0x01)
		; // �ȴ�BUSYλ���
}

//��ȡоƬID
//����ֵ����:
//0XEF13,��ʾоƬ�ͺ�ΪW25Q80
//0XEF14,��ʾоƬ�ͺ�ΪW25Q16
//0XEF15,��ʾоƬ�ͺ�ΪW25Q32
//0XEF16,��ʾоƬ�ͺ�ΪW25Q64
//0XEF17,��ʾоƬ�ͺ�ΪW25Q128
uint16_t W25QXX_ReadID(void)
{
	uint8_t Temp = 0;
	uint16_t out_Temp = 0;
	/*
	SPI_ReadWriteByte(0x90);//���Ͷ�ȡID����
	SPI_ReadWriteByte(0x00);
	SPI_ReadWriteByte(0x00);
	SPI_ReadWriteByte(0x00);
	Temp|=SPI_ReadWriteByte(0xFF)<<8;
	Temp|=SPI_ReadWriteByte(0xFF);*/
	W25QXX_CS_L;
	VprocHALWrite(0x90);
	VprocHALWrite(0);
	VprocHALWrite(0);
	VprocHALWrite(0);
	VprocHALRead(&Temp);
	out_Temp = (uint16_t)Temp << 8;
	VprocHALRead(&Temp);
	out_Temp |= Temp;
	W25QXX_CS_H;
	return out_Temp;
}


/*******************************************************************************
* Function Name  : W25QXX_Erase_Block  ���������
* Description    : Erases the specified FLASH sector.
* Input          : SectorAddr: address of the sector to erase.
* Output         : None
* Return         : None
*******************************************************************************/
void W25QXX_Erase_Block(uint32_t BlockAddr)
{
	IOT_Printf("\r\n W25QXX_Erase_Block BLOCK_value =%d ,BlockAddr: 0x%x\r\n",(BlockAddr/BLOCK_SIZE), BlockAddr);

    /* Send write enable instruction */
	W25QXX_Write_Enable();                 //SET WEL
	W25QXX_Wait_Busy();
    /* Sector Erase */

    /* Select the FLASH: Chip Select low */
	W25QXX_CS_L;
    /* Send Sector Erase instruction */
    VprocHALWrite(W25X_BlockErase);       //���Ϳ��������
    /* Send SectorAddr high nibble address byte */
    VprocHALWrite((uint8_t)((BlockAddr & 0xFF0000) >> 16));
    /* Send SectorAddr medium nibble address byte */
    VprocHALWrite((uint8_t)((BlockAddr & 0xFF00) >> 8));
    /* Send SectorAddr low nibble address byte */
    VprocHALWrite((uint8_t)(BlockAddr & 0xFF));
    /* Deselect the FLASH: Chip Select high */
    W25QXX_CS_H;

    /* Wait the end of Flash writing */
    W25QXX_Wait_Busy();

}
/******** ***************************************
* ������: W25QXX_Erase_Chip
* ��  ��: ��������оƬ
* ��  ��: none
* ����ֵ: none
* ע  ��: ��Ƭ����ʱ��
					W25X16:25s
					W25X32:40s
					W25X64:40s
********** *********************************/
void W25QXX_Erase_Chip(void)
{
	IOT_Printf("\r\n W25QXX_Erase_Chip..ing \r\n");
	W25QXX_Write_Enable(); //SET WEL
	W25QXX_Wait_Busy();
	W25QXX_CS_L; //ʹ������
	//SPI_ReadWriteByte(W25X_ChipErase);        //����Ƭ��������
	VprocHALWrite(W25X_ChipErase);
	W25QXX_CS_H;		//ȡ��Ƭѡ
	W25QXX_Wait_Busy(); //�ȴ�оƬ��������
	IOT_Printf("\r\n W25QXX_Erase_Chip...OK \r\n");
}

/*******************************************************************************
* ������: W25QXX_Erase_Sector
* ��  ��: ����һ������
* ��  ��: none
* ����ֵ: none
* ע  ��: Dst_Addr:ä����ַ 0~511 for w25x16
          ����һ����������ʱ��Ϊ:150ms
*******************************************************************************/
void W25QXX_Erase_Sector(uint32_t Dst_Addr)
{
	//����falsh�������,������
	IOT_Printf("\r\n Erase_Sector SECTOR_value =%d ,SECTOR_Addr: 0x%x\r\n",(Dst_Addr/W25X_FLASH_ErasePageSize), Dst_Addr);
//	Dst_Addr *= 4096;
	W25QXX_Write_Enable(); 						  //SET WEL
	W25QXX_Wait_Busy();
	W25QXX_CS_L; 								  //ʹ������
	/*SPI_ReadWriteByte(W25X_SectorErase);        //������������ָ��
	SPI_ReadWriteByte((uint8_t)((Dst_Addr)>>16)); //����24bit��ַ
	SPI_ReadWriteByte((uint8_t)((Dst_Addr)>>8));
	SPI_ReadWriteByte((uint8_t)Dst_Addr);*/
	VprocHALWrite(W25X_SectorErase);			  //������������ָ��
	VprocHALWrite((uint8_t)((Dst_Addr) >> 16));   //����24bit��ַ
	VprocHALWrite((uint8_t)((Dst_Addr) >> 8));
	VprocHALWrite((uint8_t)Dst_Addr);
	W25QXX_CS_H;								  //ȡ��Ƭѡ
	W25QXX_Wait_Busy();                           //�ȴ��������
}

/****************  **************************************
* ������: W25QXX_PowerDown
* ��  ��: �������ģʽ
* ��  ��: none
* ����ֵ: none
* ע  ��: ֻ��SPR,TB,BP2,BP1,BP0(bit 7,5,4,3,2)����д
******************* *********************************/
void W25QXX_PowerDown(void)
{
	W25QXX_CS_L; //ʹ������
	//SPI_ReadWriteByte(W25X_PowerDown);        //���͵�������
	VprocHALWrite(W25X_PowerDown);
	W25QXX_CS_H; //ȡ��Ƭѡ
}
/********************* *****************************
* ������: W25QXX_WAKEUP
* ��  ��: дSPI_FLASH ����
* ��  ��: none
* ����ֵ: none
* ע  ��: ֻ��SPR,TB,BP2,BP1,BP0(bit 7,5,4,3,2)����д
********************** *****************************/
void W25QXX_WAKEUP(void)
{
	W25QXX_CS_L; //ʹ������
	//SPI_ReadWriteByte(W25X_ReleasePowerDown);   //  send W25X_PowerDown command 0xAB
	VprocHALWrite(W25X_ReleasePowerDown);
	W25QXX_CS_H; //ȡ��Ƭѡ
}



/*******************************************************************************
* ������: W25Qxx_WritePage
* ��  ��: SPI��һҳ(0~65535)��д��С��256���ֽڵ�����
* ��  ��: pBuffer:���ݻ���
		 WriteAddr:��ʼд�����ʼ��ַ(24bit)
	     NumByteToWrite:Ҫд������ݸ������(256),����ø������ܳ�����ǰҳ��ʣ���ֽ���.
* ����ֵ: none
* ע  ��: Ҫָ����ַ��ʼд�����256�ֽڵ�����
*******************************************************************************/
void W25QXX_Write_Page(uint8_t *pWriteBuf, uint32_t WriteAddr, uint16_t NumByteToWrite)
{
	uint16_t i=0;
	W25QXX_Write_Enable(); //SET WEL
	W25QXX_CS_L;		   //ʹ������
	//SPI_ReadWriteByte(W25X_PageProgram);      //����дҳ����
	VprocHALWrite(W25X_PageProgram);
	//SPI_ReadWriteByte((uint8_t)((WriteAddr)>>16)); //����24bit��ַ
	VprocHALWrite((uint8_t)((WriteAddr) >> 16));
	//SPI_ReadWriteByte((uint8_t)((WriteAddr)>>8));
	VprocHALWrite((uint8_t)((WriteAddr) >> 8));
	//SPI_ReadWriteByte((uint8_t)WriteAddr);
	VprocHALWrite((uint8_t)(WriteAddr));
	for (i = 0; i < NumByteToWrite; i++)
	{
		//SPI_ReadWriteByte(pBuffer[i]);//ѭ��д��
		VprocHALWrite(pWriteBuf[i]);
	}
	W25QXX_CS_H;		//ȡ��Ƭѡ
	W25QXX_Wait_Busy(); //�ȴ�д�����
}

//W25Qxx_WriteSectors
//�޼���дSPI FLASH
//����ȷ����д�ĵ�ַ��Χ�ڵ�����ȫ��Ϊ0XFF,�����ڷ�0XFF��д������ݽ�ʧ��!
//�����Զ���ҳ����
//��ָ����ַ��ʼд��ָ�����ȵ�����,����Ҫȷ����ַ��Խ��!
//pBuffer:���ݴ洢��
//WriteAddr:��ʼд��ĵ�ַ(24bit)
//NumByteToWrite:Ҫд����ֽ���(���65535)
//CHECK OK
void W25QXX_Write_NoCheck(uint8_t *pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite)
{
	uint16_t pageremain;
	pageremain = 256 - WriteAddr % 256; //��ҳʣ����ֽ���
	if (NumByteToWrite <= pageremain)
		pageremain = NumByteToWrite; //������256���ֽ�

	while (1)
	{
		W25QXX_Write_Page(pBuffer, WriteAddr, pageremain);
		if (NumByteToWrite == pageremain)
			break; //д�������
		else	   //NumByteToWrite>pageremain
		{
			pBuffer += pageremain;
			WriteAddr += pageremain;

			NumByteToWrite -= pageremain; //��ȥ�Ѿ�д���˵��ֽ���
			if (NumByteToWrite > 256)
				pageremain = 256; //һ�ο���д��256���ֽ�
			else
				pageremain = NumByteToWrite; //����256���ֽ���
		}
	};
}
//����д
signed char W25Qxx_WriteSectors(uint32_t WriteAdd, uint8_t* pWriteBuf, uint16_t WriteLen)
{
	signed char  Result = 0;
	int16_t i16Pages = 0;
	uint8_t* pBuf;

	i16Pages = (int16_t)(WriteLen /PAGE_SIZE);
	pBuf = pWriteBuf;
	#if 0
	IOT_Printf("\r\n ������ʼ��ַ:%d \r\n",WriteAdd);
	#endif
	do
	{

		W25QXX_Write_Page(pBuf, WriteAdd, PAGE_SIZE);
		pBuf += PAGE_SIZE;
		WriteAdd += PAGE_SIZE;
		i16Pages--;
	}
	while(i16Pages > 0);

	return Result;
}
//��ȡSPI FLASH
//��ָ����ַ��ʼ��ȡָ�����ȵ�����
//pBuffer:���ݴ洢��
//ReadAddr:��ʼ��ȡ�ĵ�ַ(24bit)
//NumByteToRead:Ҫ��ȡ���ֽ���(���65535)
void W25QXX_Read(uint8_t *pBuffer, uint32_t ReadAddr, uint16_t NumByteToRead)
{
	//ESP_LOGI("read_date", "ReadAddr=%d,NumByteToRead=%d", ReadAddr, NumByteToRead);
	uint32_t i;
	uint8_t Temp = 0;
	W25QXX_CS_L;								//ʹ������
/*SPI_ReadWriteByte(W25X_ReadData);         //���Ͷ�ȡ����
    SPI_ReadWriteByte((uint8_t)((ReadAddr)>>16));  //����24bit��ַ
    SPI_ReadWriteByte((uint8_t)((ReadAddr)>>8));
    SPI_ReadWriteByte((uint8_t)ReadAddr);
    for(i=0;i<NumByteToRead;i++)
	{
        pBuffer[i]=SPI_ReadWriteByte(0XFF);   //ѭ������
    }
*/
	VprocHALWrite(W25X_ReadData);				//���Ͷ�ȡ����
	VprocHALWrite((uint8_t)((ReadAddr) >> 16)); //����24bit��ַ
	VprocHALWrite((uint8_t)((ReadAddr) >> 8));
	VprocHALWrite((uint8_t)ReadAddr);
	for (i = 0; i < NumByteToRead; i++)
	{
		VprocHALRead(&Temp);
		pBuffer[i] = Temp; //ѭ������
	}
	W25QXX_CS_H;
}

/************************************************/
//дSPI FLASH
//��ָ���ĵ�ַд��ָ�����ȵ�����
//�ú�������������
//pBuffer:���ݻ���
//WriteAddr:���ݵ���ʼ��ַ(24bit)
//NumByteToWrite:Ҫд������ݵĸ���(���65535)

void SPI_FLASH_WriteBuff(uint8_t* pWriteBuf,uint32_t WriteAddr,uint16_t WriteLen)
{

	uint16_t secoff = 0;          //WriteAddrrn�ڵ�ǰ�����ĵ�ַ
	uint16_t Free = 0;            //��ǰ����ʣ��ռ��С
	uint8_t *pBuf;

	pBuf = pWriteBuf;

	//�ж���ʼ��ַ�Ƿ�Ϊ������ʼ��ַ
	while( ((WriteAddr %SECTOR_SIZE) > 0) && (WriteLen > 0))
	{
		if ( (WriteAddr %PAGE_SIZE) > 0)
		{
			secoff = WriteAddr %PAGE_SIZE;       //д��ַ�ڵ�ǰҳ��ƫ����
			Free = PAGE_SIZE -secoff;           //��ǰҳ�Ŀ��пռ�
			if (WriteLen > Free)
			{
				W25QXX_Write_Page(pBuf, WriteAddr, Free);
				pBuf += Free;
				WriteAddr += Free;
				WriteLen -= Free;
			}
			else
			{
				W25QXX_Write_Page(pBuf, WriteAddr, WriteLen);
				pBuf += WriteLen;
				WriteAddr += WriteLen;
				WriteLen = 0;
			}
		}
		else
		{
			if (WriteLen > PAGE_SIZE)
			{
				W25QXX_Write_Page(pBuf, WriteAddr, PAGE_SIZE);
				pBuf += PAGE_SIZE;
				WriteAddr += PAGE_SIZE;
				WriteLen -= PAGE_SIZE;
			}
			else
			{
				W25QXX_Write_Page(pBuf, WriteAddr, WriteLen);
				pBuf += WriteLen;
				WriteAddr += WriteLen;
				WriteLen = 0;
			}
		}
	}
	//д���ַΪ��ǰ������ʼ��ַ�Լ����ݸ�������������С
	while( (0 == (WriteAddr %SECTOR_SIZE) ) && (WriteLen >= SECTOR_SIZE))
	{
		W25QXX_Erase_Sector(WriteAddr);
		W25Qxx_WriteSectors(WriteAddr, pBuf, SECTOR_SIZE);
		pBuf += SECTOR_SIZE;
		WriteAddr += SECTOR_SIZE;
		WriteLen -= SECTOR_SIZE;
	}

	while( WriteLen > 0)
	{
		if (0 == (WriteAddr %SECTOR_SIZE))
		{
			W25QXX_Erase_Sector(WriteAddr);
		}

		if ( (WriteAddr %PAGE_SIZE) > 0)
		{
			secoff = WriteAddr %PAGE_SIZE;       //д��ַ�ڵ�ǰҳ��ƫ����
			Free = PAGE_SIZE -secoff;           //��ǰҳ�Ŀ��пռ�
			if (WriteLen > Free)
			{
				W25QXX_Write_Page(pBuf, WriteAddr, Free);
				pBuf += Free;
				WriteAddr += Free;
				WriteLen -= Free;
			}
			else
			{
				W25QXX_Write_Page(pBuf, WriteAddr, WriteLen);
				pBuf += WriteLen;
				WriteAddr += WriteLen;
				WriteLen = 0;
			}
		}
		else
		{
			if ( WriteLen > PAGE_SIZE)
			{
				W25QXX_Write_Page(pBuf, WriteAddr, PAGE_SIZE);
				pBuf += PAGE_SIZE;
				WriteAddr += PAGE_SIZE;
				WriteLen -= PAGE_SIZE;
			}
			else
			{
				W25QXX_Write_Page(pBuf, WriteAddr, WriteLen);
				pBuf += WriteLen;
				WriteAddr += WriteLen;
				WriteLen = 0;
			}
		}
	}
}


//дSPI FLASH    дһ����
//��ָ����ַ��ʼд��ָ�����ȵ�����
//�ú�������������!
//pBuffer:���ݴ洢��
//WriteAddr:��ʼд��ĵ�ַ(24bit)
//NumByteToWrite:Ҫд����ֽ���(���65535)
uint8_t W25QXX_BUFFER[4096];   // ��д����ָ��BUFFER   ������д���õ���
uint8_t SPIFlashOperate = ENABLE;

//uint8_t FLASH_BUF[SECTOR_SIZE];


/**
  * ��������: �̰߳�ȫ��printf��ʽ
  * �������: ��
  * �� �� ֵ: ��
  * ˵    ��: ��
  */
SemaphoreHandle_t  xFlashMutex = NULL;
 void IOT_AppxFlashMutex (void)
{
    /* ���������ź��� */
	 xFlashMutex = xSemaphoreCreateMutex();

     if(xFlashMutex == NULL)
    {
    	 xFlashMutex = xSemaphoreCreateMutex();
        /* û�д����ɹ����û�������������봴��ʧ�ܵĴ������ */
    }
}
void IOT_Mutex_SPI_FLASH_BufferRead(uint8_t* pBuffer,uint32_t ReadAddr,uint16_t NumByteToRead)
{
	 /* �����ź��� */
	xSemaphoreTake(xFlashMutex, portMAX_DELAY);

	//SPI_FLASH_BufferRead(pBuffer,ReadAddr,NumByteToRead);
	W25QXX_Read(pBuffer,  ReadAddr,  NumByteToRead);

	xSemaphoreGive(xFlashMutex);
}


void IOT_Mutex_SPI_FLASH_WriteBuff(uint8_t* pWriteBuf,uint32_t WriteAddr,uint16_t WriteLen)
{
	 /* �����ź��� */
	xSemaphoreTake(xFlashMutex, portMAX_DELAY);

    SPI_FLASH_WriteBuff(pWriteBuf,WriteAddr,WriteLen);

	xSemaphoreGive(xFlashMutex);
}


void IOT_Mutex_W25QXX_Erase_Sector(uint32_t Dst_Addr)
{
	/* �����ź��� */
	xSemaphoreTake(xFlashMutex , portMAX_DELAY);

	W25QXX_Erase_Sector(Dst_Addr);

	xSemaphoreGive(xFlashMutex);
}


//
//
//
//static const char *TAG = "W25QXX";
//  uint8_t TEXT_Buffer[] = {"I am aithinker xuhongv"};
//#define SIZE sizeof(TEXT_Buffer)
//void TEST_W25qxx(void)
//{
//	uint8_t Readw25qxxbuf[512]={"11111111111111111111111111111111111"};
//	uint8_t Writew25qxxbuf[512]={0};
//	W25QXX_Erase_Chip();				   // ȫƬ��д -- �ᵼ���²�����������
//
//
//	SPI_FLASH_WriteBuff(Readw25qxxbuf,100,10);
//
//	W25QXX_Read(Writew25qxxbuf, 100, 10);
//
//	IOT_Printf("\r\n******************TEST_W25qxx ***************\r\n " );
//   	        /* Print response directly to stdout as it is read */
//	 	ESP_LOGI(TAG, "Writew25qxxbuf:%s \n", Writew25qxxbuf);
//
//   	 	uint8_t datatemp[256] = {0};
//
//   	 	uint16_t FLASH_SIZE = 8 * 1024 * 1024; //FLASH ��СΪ8M�ֽ�
//
//   	 	ESP_LOGI(TAG, "Write mySaveBuff length:%d", SIZE);
//   	 	ESP_LOGI(TAG, "Write mySaveBuff:%s\n", TEXT_Buffer);
//
//   	 SPI_FLASH_WriteBuff(TEXT_Buffer, 200, SIZE);
//   	 	W25QXX_Read(datatemp, 200, SIZE);  //FLASH_SIZE - 100
//
//   	 	ESP_LOGI(TAG, "Get mySaveBuff:%s \n", datatemp);
//}
//
