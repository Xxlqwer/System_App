#ifndef __BSP_EMMC_H_
#define __BSP_EMMC_H_

#include "main.h"
#include <stdio.h>
#define EMMC_TIMEOUT 0xffff

HAL_StatusTypeDef  EMMC_ReadBlock_DMA(uint8_t *pData, uint32_t BlockAdd ,uint32_t NumberOfBlocks);
HAL_StatusTypeDef EMMC_WriteBlock_DMA(uint8_t *pData,uint32_t BlockAdd,uint32_t NumberOfBlocks);
HAL_StatusTypeDef EMMC_Erase(uint32_t BlockAdd ,uint32_t NumberOfBlocks);
HAL_StatusTypeDef EMMC_GetInfo(HAL_MMC_CardInfoTypeDef *pData);

void EMMC_Getinfo_TEST();
void EMMC_TEST(void);

void EMMC_FATFS_TEST(void);

int8_t DMA_Write_Test(uint8_t *buff, uint32_t blk_addr, uint16_t blk_len);


// 声明 gps_data 为外部变量
//extern char gps_data[512];

#endif
