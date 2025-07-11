#include "bsp_emmc.h"
#include <string.h>
#include <stdio.h>

extern MMC_HandleTypeDef hmmc;

static HAL_MMC_CardInfoTypeDef emmc_info;

static HAL_StatusTypeDef EMMC_Wait_Ready(void)
{
    uint16_t count = EMMC_TIMEOUT;
    while (count--)
    {
        if (HAL_MMC_GetCardState(&hmmc) == HAL_MMC_CARD_TRANSFER)
        {
            return HAL_OK;
        }
        HAL_Delay(1);
    }

    return HAL_ERROR;
}

HAL_StatusTypeDef EMMC_ReadBlock_DMA(uint8_t *pData, uint32_t BlockAdd, uint32_t NumberOfBlocks)
{
    if (HAL_MMC_ReadBlocks_DMA(&hmmc, pData, BlockAdd, NumberOfBlocks) != HAL_OK)
    {
        return HAL_ERROR;
    }
    while (EMMC_Wait_Ready() != HAL_OK);

    return HAL_OK;
}

HAL_StatusTypeDef EMMC_WriteBlock_DMA(uint8_t *pData, uint32_t BlockAdd, uint32_t NumberOfBlocks)
{
    if (HAL_MMC_WriteBlocks_DMA(&hmmc, pData, BlockAdd, NumberOfBlocks) != HAL_OK)
    {
        return HAL_ERROR;
    }
    while (EMMC_Wait_Ready() != HAL_OK);

    return HAL_OK;
}

HAL_StatusTypeDef EMMC_Erase(uint32_t BlockAdd, uint32_t NumberOfBlocks)
{
    if (HAL_MMC_Erase(&hmmc, BlockAdd, BlockAdd + NumberOfBlocks - 1) != HAL_OK)
    {
        return HAL_ERROR;
    }
    while (EMMC_Wait_Ready() != HAL_OK);

    return HAL_OK;
}

HAL_StatusTypeDef EMMC_GetInfo(HAL_MMC_CardInfoTypeDef *pData)
{
    return HAL_MMC_GetCardInfo(&hmmc, pData);
}

void EMMC_Getinfo_TEST(void)
{
    HAL_StatusTypeDef ret = HAL_OK;
    printf("\r\nEMMC_Getinfo_Test\r\n");
    ret = EMMC_GetInfo(&emmc_info);
    if (ret == HAL_OK)
    {
        printf("EMMC_Info: \r\n");
        printf("CardType: %d\r\n", emmc_info.CardType);
        printf("Class: %d\r\n", emmc_info.Class);
        printf("RelCardAdd: %d\r\n", emmc_info.RelCardAdd);
        printf("BlockNbr: %d\r\n", emmc_info.BlockNbr);
        printf("BlockSize: %d\r\n", emmc_info.BlockSize);
        printf("LogBlockNbr: %d\r\n", emmc_info.LogBlockNbr);
        printf("LogoBlockSize: %d\r\n", emmc_info.LogBlockSize);
    }
    else
    {
        printf("\r\nEMMC_GetInfo_ERROR(%d) \r\n", ret);
    }
}

static uint8_t write_buffer[512] __attribute__((aligned(4)));
static uint8_t read_buffer[512] __attribute__((aligned(4)));

int8_t DMA_Read_Write_Test(void)
{
    int8_t ret = HAL_ERROR;
    HAL_StatusTypeDef status;

    memset(write_buffer, 0x5a, 512);
    memset(read_buffer, 0xff, 512);

    status = EMMC_WriteBlock_DMA(write_buffer, 0, 1);
    if (status != HAL_OK)
    {
        printf("Write operation failed. HAL status:%d\n", status);
        ret = HAL_ERROR;
    }

    status = EMMC_ReadBlock_DMA(read_buffer, 0, 1);
    if (status != HAL_OK)
    {
        printf("Read operation failed. HAL status:%d\n", status);
        ret = HAL_ERROR;
    }

    for (unsigned int i = 0; i < 512; i++)
    {
        if (write_buffer[i] != read_buffer[i])
        {
            printf("emmc test failed, index = %d write data is 0x%x, but read data is 0x%x\r\n", i, write_buffer[i], read_buffer[i]);
            ret = HAL_ERROR;
            return ret;
        }
    }

    printf("emmc write read test successful\r\n");

    return ret;
}

/** 效果：
emmc demo

EMMC_Getinfo_Test
EMMC_Info: 
CardType: 1
Class: 245
RelCardAdd: 2
BlockNbr: 61079552
BlockSize: 512
LogBlockNbr: 61079552
LogoBlockSize: 512
emmc write read test successful
*
*/
