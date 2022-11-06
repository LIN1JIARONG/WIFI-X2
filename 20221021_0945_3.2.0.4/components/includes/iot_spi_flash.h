/*
 * iot_spi_flash.h
 *
 *  Created on: 2021��11��16��
 *      Author: Administrator
 */

#ifndef COMPONENTS_SPI_FLASH_IOT_SPI_FLASH_H_
#define COMPONENTS_SPI_FLASH_IOT_SPI_FLASH_H_

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "sdkconfig.h"


#define W25Q64

#ifdef W25Q80
              #define W25Q80_ID 	   0XEF13
			  #define W25X_Size        0x00100000   // 1M
#elif defined W25Q16
              #define W25Q16_ID 	   0XEF14
			  #define W25X_Size        0x00200000   // 2M
#elif defined W25Q32
              #define W25Q32_ID 	   0XEF15
			  #define W25X_Size        0x00400000   // 4M
#elif defined W25Q64
              #define W25Q64_ID 	   0XEF16
			  #define W25X_Size        0x00800000   // 8M
#endif

#define  W25X_FLASH_ErasePageSize      4096       // 4k
#define  W25X_FLASH_PerWritePageSize   256        // 256byte

#define PAGE_SIZE               256                     //ҳ��С
#define SECTOR_SIZE             4096                    //������С  4K
#define BLOCK_SIZE              0x10000                 //���С    64K


#define SPI_FLASH_SIZE 16 * 1024 * 1024

extern uint16_t W25QXX_TYPE; //����W25QXXоƬ�ͺ�
extern uint8_t W25QXX_BUFFER[4096];
 //W25Xϵ��/Qϵ��оƬ�б�
//W25Q80  ID  0XEF13
//W25Q16  ID  0XEF14
//W25Q32  ID  0XEF15
//W25Q64  ID  0XEF16
//W25Q128 ID  0XEF17
//W25Q256 ID  0XEF18

#define W25X_WriteEnable		       0x06
#define W25X_WriteDisable		       0x04
#define W25X_ReadStatusReg		       0x05
#define W25X_WriteStatusReg		       0x01
#define W25X_ReadData			       0x03
#define W25X_FastReadData		       0x0B
#define W25X_FastReadDual		       0x3B
#define W25X_PageProgram		       0x02
#define W25X_BlockErase			       0xD8
#define W25X_SectorErase		       0x20
#define W25X_ChipErase			       0xC7
#define W25X_PowerDown			       0xB9
#define W25X_ReleasePowerDown	       0xAB
#define W25X_DeviceID			       0xAB
#define W25X_ManufactDeviceID   	   0x90
#define W25X_JedecDeviceID		       0x9F



//ָ���
/*
#define W25X_WriteEnable 0x06
#define W25X_WriteDisable 0x04
#define W25X_ReadStatusReg 0x05
// #define W25X_ReadStatusReg2		0x35
// #define W25X_ReadStatusReg3		0x15
#define W25X_WriteStatusReg 0x01
// #define W25X_WriteStatusReg2    0x31
// #define W25X_WriteStatusReg3    0x11
#define W25X_ReadData 0x03
#define W25X_FastReadData 0x0B
#define W25X_FastReadDual 0x3B
#define W25X_PageProgram 0x02
#define W25X_BlockErase 0xD8
#define W25X_SectorErase 0x20
#define W25X_ChipErase 0xC7
#define W25X_PowerDown 0xB9
#define W25X_ReleasePowerDown 0xAB
#define W25X_DeviceID 0xAB
#define W25X_ManufactDeviceID 0x90
#define W25X_JedecDeviceID 0x9F
#define W25X_Enable4ByteAddr 0xB7
#define W25X_Exit4ByteAddr 0xE9
*/
void IOT_SPI_FLASH_Init(void);
uint16_t W25QXX_ReadID(void);       //��ȡFLASH ID
uint8_t W25QXX_ReadSR(void);        //��ȡ״̬�Ĵ���S
void W25QXX_Write_SR(uint8_t sr);  //д״̬�Ĵ���
void W25QXX_Write_Enable(void);   //дʹ��
void W25QXX_Write_Disable(void);  //д����


void W25QXX_Erase_Block(uint32_t BlockAddr);											   //�������
void W25QXX_Erase_Chip(void);                                                              //��Ƭ����
void W25QXX_Erase_Sector(uint32_t Dst_Addr);                                               //��������
void W25QXX_Wait_Busy(void);                                                               //�ȴ�����
void W25QXX_PowerDown(void);                                                               //�������ģʽ
void W25QXX_WAKEUP(void);                                                                  //����


void W25QXX_Write_NoCheck(uint8_t *pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite);
//void W25QXX_Read(uint8_t *pBuffer, uint32_t ReadAddr, uint16_t NumByteToRead);              //��ȡflash
void IOT_AppxFlashMutex (void);
void IOT_Mutex_SPI_FLASH_BufferRead(uint8_t* pBuffer,uint32_t ReadAddr,uint16_t NumByteToRead);
void IOT_Mutex_SPI_FLASH_WriteBuff(uint8_t* pWriteBuf,uint32_t WriteAddr,uint16_t WriteLen);
void IOT_Mutex_W25QXX_Erase_Sector(uint32_t Dst_Addr);

//void SPI_FLASH_WriteBuff(uint8_t* pWriteBuf,uint32_t WriteAddr,uint16_t WriteLen);


//uint16_t W25QXX_Read_Data(uint8_t *Temp_buff, uint32_t ReadAddr, uint16_t Size_Temp_buff);   //��ȡ�����е�һ������
//uint32_t Read_Post_Len(uint32_t Start_Addr, uint32_t End_Addr);                              //��ȡ��������ȷ�����ݵ��ܴ�С
//void TEST_W25qxx(void);

#endif /* COMPONENTS_SPI_FLASH_IOT_SPI_FLASH_H_ */
