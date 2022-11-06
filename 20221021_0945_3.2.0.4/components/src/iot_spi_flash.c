/*
 * iot_spi_flash.c
 *
 *  Created on: 2021年11月16日
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
	uint16_t W25QXX_TYPE = 0;	   //默认是W25Q128
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
	W25QXX_TYPE = W25QXX_ReadID(); //读取FLASH ID.
	IOT_Printf("IOT_SPI_FLASH_Init W25QXX_ReadID  = 0x%x \n", W25QXX_TYPE);


	W25QXX_CS_H;
	vTaskDelay(100 / portTICK_PERIOD_MS);

	W25QXX_TYPE = W25QXX_ReadID(); //读取FLASH ID.

	IOT_Printf("IOT_SPI_FLASH_Init W25QXX_ReadID  = 0x%x \n", W25QXX_TYPE);

	while((W25QXX_ReadSR() & 0X01) == 0X01)
	{
		if(++err_count > 2000)
		{
			IOT_Printf("IOT_W25Qxx_Init FLASH error \r\n"); /*FLASH 芯片损坏*/
			break;
		}
	}
 	W25QXX_Write_Enable(); //SPI_FLASH写使能
 	W25QXX_Write_SR(0);
 	W25QXX_Wait_Busy();
}


/*************************************************************
* 函数名: W25QXX_ReadSR
* 功  能: 读取FLASH状态寄存器
* 输  入: none
* 返回值: none
* 注  意: BIT7  6   5   4   3   2   1   0
	      SPR   RV  TB BP2 BP1 BP0 WEL BUSY
   SPR:默认为0,状态寄存器保护位配合WP使用
   TB,BP2,BP1,BP0:FLASH区域写保护设置
   WEL:写使能锁定
   BUSY:忙标记位: 1:忙; 0:空闲,默认
************************************** ***********************/
uint8_t W25QXX_ReadSR(void)
{
	uint8_t byte = 0;
	/*W25QXX_CS_L;                            //使能器件
	SPI_ReadWriteByte(W25X_ReadStatusReg);    //发送读取状态寄存器命令
	byte=SPI_ReadWriteByte(0Xff);             //读取一个字节
	W25QXX_CS_H;                              //取消片选
	*/
	W25QXX_CS_L;
	VprocHALWrite(W25X_ReadStatusReg);
	VprocHALRead(&byte);
	W25QXX_CS_H;

	return byte;
}

/*************************** **************************
* 函数名: W25QXX_Write_SR
* 功  能: 写SPI_FLASH状态寄存器
* 输  入: none
* 返回值: none
* 注  意: 只有SPR,TB,BP2,BP1,BP0(bit 7,5,4,3,2)可以写
**************************** *****************************/
void W25QXX_Write_SR(uint8_t sr)
{
	W25QXX_CS_L; //使能器件
	//SPI_ReadWriteByte(W25X_WriteStatusReg);   //发送写取状态寄存器命令
	//SPI_ReadWriteByte(sr);               //写入一个字节
	VprocHALWrite(W25X_WriteStatusReg);
	VprocHALWrite(sr);
	W25QXX_CS_H; //取消片选
}
/************* **************************************
* 函数名: W25QXX_Write_Enable
* 功  能: SPI_FLASH写使能
* 输  入: none
* 返回值: none
* 注  意: none//将WEL置位
************** *************************************/
void W25QXX_Write_Enable(void)
{
	W25QXX_CS_L; //使能器件
	//SPI_ReadWriteByte(W25X_WriteEnable);      //发送写使能
	VprocHALWrite(W25X_WriteEnable);
	W25QXX_CS_H; //取消片选
}
/************** **************************
* 函数名: W25QXX_Write_Disable
* 功  能: SPI_FLASH写使能
* 输  入: none
* 返回值: none
* 注  意: none//将WEL清零
*************** *********************/
void W25QXX_Write_Disable(void)
{
	W25QXX_CS_L; //使能器件
	//SPI_ReadWriteByte(W25X_WriteDisable);     //发送写禁止指令
	VprocHALWrite(W25X_WriteDisable);
	W25QXX_CS_H; //取消片选
}

/******************* ****************************************
* 函数名: W25Qxx_WaitBusy
* 功  能: 等待空闲
* 输  入: none
* 返回值: none
* 注  意: 只有SPR,TB,BP2,BP1,BP0(bit 7,5,4,3,2)可以写
********************* ****************************************/
void W25QXX_Wait_Busy(void)
{
	while ((W25QXX_ReadSR() & 0x01) == 0x01)
		; // 等待BUSY位清空
}

//读取芯片ID
//返回值如下:
//0XEF13,表示芯片型号为W25Q80
//0XEF14,表示芯片型号为W25Q16
//0XEF15,表示芯片型号为W25Q32
//0XEF16,表示芯片型号为W25Q64
//0XEF17,表示芯片型号为W25Q128
uint16_t W25QXX_ReadID(void)
{
	uint8_t Temp = 0;
	uint16_t out_Temp = 0;
	/*
	SPI_ReadWriteByte(0x90);//发送读取ID命令
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
* Function Name  : W25QXX_Erase_Block  块擦除函数
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
    VprocHALWrite(W25X_BlockErase);       //发送块擦除命令
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
* 函数名: W25QXX_Erase_Chip
* 功  能: 擦除整个芯片
* 输  入: none
* 返回值: none
* 注  意: 整片擦除时间
					W25X16:25s
					W25X32:40s
					W25X64:40s
********** *********************************/
void W25QXX_Erase_Chip(void)
{
	IOT_Printf("\r\n W25QXX_Erase_Chip..ing \r\n");
	W25QXX_Write_Enable(); //SET WEL
	W25QXX_Wait_Busy();
	W25QXX_CS_L; //使能器件
	//SPI_ReadWriteByte(W25X_ChipErase);        //发送片擦除命令
	VprocHALWrite(W25X_ChipErase);
	W25QXX_CS_H;		//取消片选
	W25QXX_Wait_Busy(); //等待芯片擦除结束
	IOT_Printf("\r\n W25QXX_Erase_Chip...OK \r\n");
}

/*******************************************************************************
* 函数名: W25QXX_Erase_Sector
* 功  能: 擦除一个扇区
* 输  入: none
* 返回值: none
* 注  意: Dst_Addr:盲区地址 0~511 for w25x16
          擦除一个扇区最少时间为:150ms
*******************************************************************************/
void W25QXX_Erase_Sector(uint32_t Dst_Addr)
{
	//监视falsh擦除情况,测试用
	IOT_Printf("\r\n Erase_Sector SECTOR_value =%d ,SECTOR_Addr: 0x%x\r\n",(Dst_Addr/W25X_FLASH_ErasePageSize), Dst_Addr);
//	Dst_Addr *= 4096;
	W25QXX_Write_Enable(); 						  //SET WEL
	W25QXX_Wait_Busy();
	W25QXX_CS_L; 								  //使能器件
	/*SPI_ReadWriteByte(W25X_SectorErase);        //发送扇区擦除指令
	SPI_ReadWriteByte((uint8_t)((Dst_Addr)>>16)); //发送24bit地址
	SPI_ReadWriteByte((uint8_t)((Dst_Addr)>>8));
	SPI_ReadWriteByte((uint8_t)Dst_Addr);*/
	VprocHALWrite(W25X_SectorErase);			  //发送扇区擦除指令
	VprocHALWrite((uint8_t)((Dst_Addr) >> 16));   //发送24bit地址
	VprocHALWrite((uint8_t)((Dst_Addr) >> 8));
	VprocHALWrite((uint8_t)Dst_Addr);
	W25QXX_CS_H;								  //取消片选
	W25QXX_Wait_Busy();                           //等待擦除完成
}

/****************  **************************************
* 函数名: W25QXX_PowerDown
* 功  能: 进入掉电模式
* 输  入: none
* 返回值: none
* 注  意: 只有SPR,TB,BP2,BP1,BP0(bit 7,5,4,3,2)可以写
******************* *********************************/
void W25QXX_PowerDown(void)
{
	W25QXX_CS_L; //使能器件
	//SPI_ReadWriteByte(W25X_PowerDown);        //发送掉电命令
	VprocHALWrite(W25X_PowerDown);
	W25QXX_CS_H; //取消片选
}
/********************* *****************************
* 函数名: W25QXX_WAKEUP
* 功  能: 写SPI_FLASH 唤醒
* 输  入: none
* 返回值: none
* 注  意: 只有SPR,TB,BP2,BP1,BP0(bit 7,5,4,3,2)可以写
********************** *****************************/
void W25QXX_WAKEUP(void)
{
	W25QXX_CS_L; //使能器件
	//SPI_ReadWriteByte(W25X_ReleasePowerDown);   //  send W25X_PowerDown command 0xAB
	VprocHALWrite(W25X_ReleasePowerDown);
	W25QXX_CS_H; //取消片选
}



/*******************************************************************************
* 函数名: W25Qxx_WritePage
* 功  能: SPI在一页(0~65535)内写入小于256个字节的数据
* 输  入: pBuffer:数据缓存
		 WriteAddr:开始写入的起始地址(24bit)
	     NumByteToWrite:要写入的数据个数最大(256),另外该个数不能超过当前页的剩余字节数.
* 返回值: none
* 注  意: 要指定地址开始写入最大256字节的数据
*******************************************************************************/
void W25QXX_Write_Page(uint8_t *pWriteBuf, uint32_t WriteAddr, uint16_t NumByteToWrite)
{
	uint16_t i=0;
	W25QXX_Write_Enable(); //SET WEL
	W25QXX_CS_L;		   //使能器件
	//SPI_ReadWriteByte(W25X_PageProgram);      //发送写页命令
	VprocHALWrite(W25X_PageProgram);
	//SPI_ReadWriteByte((uint8_t)((WriteAddr)>>16)); //发送24bit地址
	VprocHALWrite((uint8_t)((WriteAddr) >> 16));
	//SPI_ReadWriteByte((uint8_t)((WriteAddr)>>8));
	VprocHALWrite((uint8_t)((WriteAddr) >> 8));
	//SPI_ReadWriteByte((uint8_t)WriteAddr);
	VprocHALWrite((uint8_t)(WriteAddr));
	for (i = 0; i < NumByteToWrite; i++)
	{
		//SPI_ReadWriteByte(pBuffer[i]);//循环写数
		VprocHALWrite(pWriteBuf[i]);
	}
	W25QXX_CS_H;		//取消片选
	W25QXX_Wait_Busy(); //等待写入结束
}

//W25Qxx_WriteSectors
//无检验写SPI FLASH
//必须确保所写的地址范围内的数据全部为0XFF,否则在非0XFF处写入的数据将失败!
//具有自动换页功能
//在指定地址开始写入指定长度的数据,但是要确保地址不越界!
//pBuffer:数据存储区
//WriteAddr:开始写入的地址(24bit)
//NumByteToWrite:要写入的字节数(最大65535)
//CHECK OK
void W25QXX_Write_NoCheck(uint8_t *pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite)
{
	uint16_t pageremain;
	pageremain = 256 - WriteAddr % 256; //单页剩余的字节数
	if (NumByteToWrite <= pageremain)
		pageremain = NumByteToWrite; //不大于256个字节

	while (1)
	{
		W25QXX_Write_Page(pBuffer, WriteAddr, pageremain);
		if (NumByteToWrite == pageremain)
			break; //写入结束了
		else	   //NumByteToWrite>pageremain
		{
			pBuffer += pageremain;
			WriteAddr += pageremain;

			NumByteToWrite -= pageremain; //减去已经写入了的字节数
			if (NumByteToWrite > 256)
				pageremain = 256; //一次可以写入256个字节
			else
				pageremain = NumByteToWrite; //不够256个字节了
		}
	};
}
//扇区写
signed char W25Qxx_WriteSectors(uint32_t WriteAdd, uint8_t* pWriteBuf, uint16_t WriteLen)
{
	signed char  Result = 0;
	int16_t i16Pages = 0;
	uint8_t* pBuf;

	i16Pages = (int16_t)(WriteLen /PAGE_SIZE);
	pBuf = pWriteBuf;
	#if 0
	IOT_Printf("\r\n 扇区起始地址:%d \r\n",WriteAdd);
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
//读取SPI FLASH
//在指定地址开始读取指定长度的数据
//pBuffer:数据存储区
//ReadAddr:开始读取的地址(24bit)
//NumByteToRead:要读取的字节数(最大65535)
void W25QXX_Read(uint8_t *pBuffer, uint32_t ReadAddr, uint16_t NumByteToRead)
{
	//ESP_LOGI("read_date", "ReadAddr=%d,NumByteToRead=%d", ReadAddr, NumByteToRead);
	uint32_t i;
	uint8_t Temp = 0;
	W25QXX_CS_L;								//使能器件
/*SPI_ReadWriteByte(W25X_ReadData);         //发送读取命令
    SPI_ReadWriteByte((uint8_t)((ReadAddr)>>16));  //发送24bit地址
    SPI_ReadWriteByte((uint8_t)((ReadAddr)>>8));
    SPI_ReadWriteByte((uint8_t)ReadAddr);
    for(i=0;i<NumByteToRead;i++)
	{
        pBuffer[i]=SPI_ReadWriteByte(0XFF);   //循环读数
    }
*/
	VprocHALWrite(W25X_ReadData);				//发送读取命令
	VprocHALWrite((uint8_t)((ReadAddr) >> 16)); //发送24bit地址
	VprocHALWrite((uint8_t)((ReadAddr) >> 8));
	VprocHALWrite((uint8_t)ReadAddr);
	for (i = 0; i < NumByteToRead; i++)
	{
		VprocHALRead(&Temp);
		pBuffer[i] = Temp; //循环读数
	}
	W25QXX_CS_H;
}

/************************************************/
//写SPI FLASH
//在指定的地址写入指定长度的数据
//该函数带擦除功能
//pBuffer:数据缓存
//WriteAddr:数据的起始地址(24bit)
//NumByteToWrite:要写入的数据的个数(最大65535)

void SPI_FLASH_WriteBuff(uint8_t* pWriteBuf,uint32_t WriteAddr,uint16_t WriteLen)
{

	uint16_t secoff = 0;          //WriteAddrrn在当前扇区的地址
	uint16_t Free = 0;            //当前扇区剩余空间大小
	uint8_t *pBuf;

	pBuf = pWriteBuf;

	//判断起始地址是否为扇区起始地址
	while( ((WriteAddr %SECTOR_SIZE) > 0) && (WriteLen > 0))
	{
		if ( (WriteAddr %PAGE_SIZE) > 0)
		{
			secoff = WriteAddr %PAGE_SIZE;       //写地址在当前页的偏移量
			Free = PAGE_SIZE -secoff;           //当前页的空闲空间
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
	//写入地址为当前扇区起始地址以及数据个数大于扇区大小
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
			secoff = WriteAddr %PAGE_SIZE;       //写地址在当前页的偏移量
			Free = PAGE_SIZE -secoff;           //当前页的空闲空间
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


//写SPI FLASH    写一整扇
//在指定地址开始写入指定长度的数据
//该函数带擦除操作!
//pBuffer:数据存储区
//WriteAddr:开始写入的地址(24bit)
//NumByteToWrite:要写入的字节数(最大65535)
uint8_t W25QXX_BUFFER[4096];   // 读写函数指定BUFFER   其他读写不得调用
uint8_t SPIFlashOperate = ENABLE;

//uint8_t FLASH_BUF[SECTOR_SIZE];


/**
  * 函数功能: 线程安全的printf方式
  * 输入参数: 无
  * 返 回 值: 无
  * 说    明: 无
  */
SemaphoreHandle_t  xFlashMutex = NULL;
 void IOT_AppxFlashMutex (void)
{
    /* 创建互斥信号量 */
	 xFlashMutex = xSemaphoreCreateMutex();

     if(xFlashMutex == NULL)
    {
    	 xFlashMutex = xSemaphoreCreateMutex();
        /* 没有创建成功，用户可以在这里加入创建失败的处理机制 */
    }
}
void IOT_Mutex_SPI_FLASH_BufferRead(uint8_t* pBuffer,uint32_t ReadAddr,uint16_t NumByteToRead)
{
	 /* 互斥信号量 */
	xSemaphoreTake(xFlashMutex, portMAX_DELAY);

	//SPI_FLASH_BufferRead(pBuffer,ReadAddr,NumByteToRead);
	W25QXX_Read(pBuffer,  ReadAddr,  NumByteToRead);

	xSemaphoreGive(xFlashMutex);
}


void IOT_Mutex_SPI_FLASH_WriteBuff(uint8_t* pWriteBuf,uint32_t WriteAddr,uint16_t WriteLen)
{
	 /* 互斥信号量 */
	xSemaphoreTake(xFlashMutex, portMAX_DELAY);

    SPI_FLASH_WriteBuff(pWriteBuf,WriteAddr,WriteLen);

	xSemaphoreGive(xFlashMutex);
}


void IOT_Mutex_W25QXX_Erase_Sector(uint32_t Dst_Addr)
{
	/* 互斥信号量 */
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
//	W25QXX_Erase_Chip();				   // 全片擦写 -- 会导致新参数区被擦除
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
//   	 	uint16_t FLASH_SIZE = 8 * 1024 * 1024; //FLASH 大小为8M字节
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
