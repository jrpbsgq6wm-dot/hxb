#ifndef __SPI__H__
#define __SPI__H__

#include "sys.h"
#include "stm32f4xx.h"

extern SPI_HandleTypeDef SPI1_Config;  //SPI2¾ä±ú
extern SPI_HandleTypeDef SPI2_Config;  //SPI2¾ä±ú
extern SPI_HandleTypeDef SPI3_Config;  //SPI2¾ä±ú

extern void SPI2_Init(void);
extern void SPI1_Init(void);
extern void SPI3_Init(void);
extern void SPI_SetSpeed(SPI_HandleTypeDef *hspi,u8 SPI_BaudRatePrescaler);

extern u8 SPI_ReadWriteByte(SPI_HandleTypeDef *hspi,u8 TxData);

extern void ADS7124_Init(void);
extern uint8_t AD7124_Recive8Bit(void);
extern void AD7124_Send8Bit(uint8_t _data);

#endif

