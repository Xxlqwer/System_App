/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    sdio.c
  * @brief   This file provides code for the configuration
  *          of the SDIO instances.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "sdio.h"
#include "stdio.h"
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

MMC_HandleTypeDef hmmc;
DMA_HandleTypeDef hdma_sdio_tx;
DMA_HandleTypeDef hdma_sdio_rx;

/* SDIO init function */

void MX_SDIO_MMC_Init(void)
{

  /* USER CODE BEGIN SDIO_Init 0 */

  /* USER CODE END SDIO_Init 0 */

  /* USER CODE BEGIN SDIO_Init 1 */

  /* USER CODE END SDIO_Init 1 */
  hmmc.Instance = SDIO;
  hmmc.Init.ClockEdge = SDIO_CLOCK_EDGE_RISING;
  hmmc.Init.ClockBypass = SDIO_CLOCK_BYPASS_DISABLE;
  hmmc.Init.ClockPowerSave = SDIO_CLOCK_POWER_SAVE_DISABLE;
  hmmc.Init.BusWide = SDIO_BUS_WIDE_4B;
  hmmc.Init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_DISABLE;
  hmmc.Init.ClockDiv = 0;
	//printf("0");
  if (HAL_MMC_Init(&hmmc) != HAL_OK)
  {
		printf("1");
    Error_Handler();
  }
  if (HAL_MMC_ConfigWideBusOperation(&hmmc, SDIO_BUS_WIDE_4B) != HAL_OK)
  {
		printf("2");
    Error_Handler();
  }
  /* USER CODE BEGIN SDIO_Init 2 */

  /* USER CODE END SDIO_Init 2 */

}

void HAL_MMC_MspInit(MMC_HandleTypeDef* mmcHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(mmcHandle->Instance==SDIO)
  {
  /* USER CODE BEGIN SDIO_MspInit 0 */

  /* USER CODE END SDIO_MspInit 0 */
    /* SDIO clock enable */
    __HAL_RCC_SDIO_CLK_ENABLE();

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    /**SDIO GPIO Configuration
    PC8     ------> SDIO_D0
    PC9     ------> SDIO_D1
    PC10     ------> SDIO_D2
    PC11     ------> SDIO_D3
    PC12     ------> SDIO_CK
    PD2     ------> SDIO_CMD
    */
    GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11
                          |GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF12_SDIO;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF12_SDIO;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /* SDIO DMA Init */
    /* SDIO_TX Init */
    hdma_sdio_tx.Instance = DMA2_Stream3;
    hdma_sdio_tx.Init.Channel = DMA_CHANNEL_4;
    hdma_sdio_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_sdio_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_sdio_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_sdio_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    hdma_sdio_tx.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    hdma_sdio_tx.Init.Mode = DMA_PFCTRL;
    hdma_sdio_tx.Init.Priority = DMA_PRIORITY_HIGH;
    hdma_sdio_tx.Init.FIFOMode = DMA_FIFOMODE_ENABLE;
    hdma_sdio_tx.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
    hdma_sdio_tx.Init.MemBurst = DMA_MBURST_INC4;
    hdma_sdio_tx.Init.PeriphBurst = DMA_PBURST_INC4;
    if (HAL_DMA_Init(&hdma_sdio_tx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(mmcHandle,hdmatx,hdma_sdio_tx);

    /* SDIO_RX Init */
    hdma_sdio_rx.Instance = DMA2_Stream6;
    hdma_sdio_rx.Init.Channel = DMA_CHANNEL_4;
    hdma_sdio_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_sdio_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_sdio_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_sdio_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    hdma_sdio_rx.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    hdma_sdio_rx.Init.Mode = DMA_PFCTRL;
    hdma_sdio_rx.Init.Priority = DMA_PRIORITY_HIGH;
    hdma_sdio_rx.Init.FIFOMode = DMA_FIFOMODE_ENABLE;
    hdma_sdio_rx.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
    hdma_sdio_rx.Init.MemBurst = DMA_MBURST_INC4;
    hdma_sdio_rx.Init.PeriphBurst = DMA_PBURST_INC4;
    if (HAL_DMA_Init(&hdma_sdio_rx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(mmcHandle,hdmarx,hdma_sdio_rx);

    /* SDIO interrupt Init */
    HAL_NVIC_SetPriority(SDIO_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(SDIO_IRQn);
  /* USER CODE BEGIN SDIO_MspInit 1 */

  /* USER CODE END SDIO_MspInit 1 */
  }
}

void HAL_MMC_MspDeInit(MMC_HandleTypeDef* mmcHandle)
{

  if(mmcHandle->Instance==SDIO)
  {
  /* USER CODE BEGIN SDIO_MspDeInit 0 */

  /* USER CODE END SDIO_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_SDIO_CLK_DISABLE();

    /**SDIO GPIO Configuration
    PC8     ------> SDIO_D0
    PC9     ------> SDIO_D1
    PC10     ------> SDIO_D2
    PC11     ------> SDIO_D3
    PC12     ------> SDIO_CK
    PD2     ------> SDIO_CMD
    */
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11
                          |GPIO_PIN_12);

    HAL_GPIO_DeInit(GPIOD, GPIO_PIN_2);

    /* SDIO DMA DeInit */
    HAL_DMA_DeInit(mmcHandle->hdmatx);
    HAL_DMA_DeInit(mmcHandle->hdmarx);

    /* SDIO interrupt Deinit */
    HAL_NVIC_DisableIRQ(SDIO_IRQn);
  /* USER CODE BEGIN SDIO_MspDeInit 1 */

  /* USER CODE END SDIO_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */
//更强大的SDIO复位和重初始化函数
HAL_StatusTypeDef SDIO_Reinitialize(void) {
    uint8_t retry_count = 0;
    HAL_StatusTypeDef status;
    
    printf("开始SDIO完全重初始化...\r\n");
    
    // 完全反初始化当前SDIO
    HAL_MMC_DeInit(&hmmc);
    
    // 禁用SDIO中断
    HAL_NVIC_DisableIRQ(SDIO_IRQn);
    
    // 禁用DMA中断
    HAL_NVIC_DisableIRQ(DMA2_Stream3_IRQn);
    HAL_NVIC_DisableIRQ(DMA2_Stream6_IRQn);
    
    // 反初始化DMA
    HAL_DMA_DeInit(&hdma_sdio_tx);
    HAL_DMA_DeInit(&hdma_sdio_rx);
    
    // 强制复位SDIO
    __HAL_RCC_SDIO_FORCE_RESET();
    HAL_Delay(20);
    __HAL_RCC_SDIO_RELEASE_RESET();
    
    // 禁用SDIO时钟
    __HAL_RCC_SDIO_CLK_DISABLE();
    HAL_Delay(100);  // 给eMMC足够的时间复位
    
    // 重新配置GPIO
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12);
    HAL_GPIO_DeInit(GPIOD, GPIO_PIN_2);
    
    // 重试循环
    while (retry_count < 3) {
        printf("SDIO初始化尝试 %d/3...\r\n", retry_count + 1);
        
        // 重新初始化SDIO
        status = MX_SDIO_MMC_Init_Safe();
        
        if (status == HAL_OK) {
            printf("SDIO初始化成功!\r\n");
            return HAL_OK;
        }
        
        // 失败，等待更长时间后重试
        HAL_Delay(200 + retry_count * 100);
        retry_count++;
    }
    
    printf("SDIO重初始化失败，已尝试%d次\r\n", retry_count);
    return HAL_ERROR;
}
//创建一个带错误处理和诊断的安全初始化函数：
HAL_StatusTypeDef MX_SDIO_MMC_Init_Safe(void) 
{
    HAL_StatusTypeDef status;
    
    // 启用时钟
    __HAL_RCC_SDIO_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    
    // 配置GPIO
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF12_SDIO;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
    
    // 配置SDIO
    hmmc.Instance = SDIO;
    hmmc.Init.ClockEdge = SDIO_CLOCK_EDGE_RISING;
    hmmc.Init.ClockBypass = SDIO_CLOCK_BYPASS_DISABLE;
    hmmc.Init.ClockPowerSave = SDIO_CLOCK_POWER_SAVE_DISABLE;
    hmmc.Init.BusWide = SDIO_BUS_WIDE_1B; // 先用1位模式初始化
    hmmc.Init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_DISABLE;
    hmmc.Init.ClockDiv = 2; // 降低初始时钟频率
    
    // 初始化DMA
    // SDIO_TX Init
    hdma_sdio_tx.Instance = DMA2_Stream3;
    hdma_sdio_tx.Init.Channel = DMA_CHANNEL_4;
    hdma_sdio_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_sdio_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_sdio_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_sdio_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    hdma_sdio_tx.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    hdma_sdio_tx.Init.Mode = DMA_PFCTRL;
    hdma_sdio_tx.Init.Priority = DMA_PRIORITY_HIGH;
    hdma_sdio_tx.Init.FIFOMode = DMA_FIFOMODE_ENABLE;
    hdma_sdio_tx.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
    hdma_sdio_tx.Init.MemBurst = DMA_MBURST_INC4;
    hdma_sdio_tx.Init.PeriphBurst = DMA_PBURST_INC4;
    
    if (HAL_DMA_Init(&hdma_sdio_tx) != HAL_OK) 
    {
        printf("DMA TX初始化失败\r\n");
        return HAL_ERROR;
    }
    
    __HAL_LINKDMA(&hmmc, hdmatx, hdma_sdio_tx);
    
    // SDIO_RX Init
    hdma_sdio_rx.Instance = DMA2_Stream6;
    hdma_sdio_rx.Init.Channel = DMA_CHANNEL_4;
    hdma_sdio_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_sdio_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_sdio_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_sdio_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    hdma_sdio_rx.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    hdma_sdio_rx.Init.Mode = DMA_PFCTRL;
    hdma_sdio_rx.Init.Priority = DMA_PRIORITY_HIGH;
    hdma_sdio_rx.Init.FIFOMode = DMA_FIFOMODE_ENABLE;
    hdma_sdio_rx.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
    hdma_sdio_rx.Init.MemBurst = DMA_MBURST_INC4;
    hdma_sdio_rx.Init.PeriphBurst = DMA_PBURST_INC4;
    
    if (HAL_DMA_Init(&hdma_sdio_rx) != HAL_OK) 
    {
        printf("DMA RX初始化失败\r\n");
        return HAL_ERROR;
    }
    
    __HAL_LINKDMA(&hmmc, hdmarx, hdma_sdio_rx);
    
    // 启用中断
    HAL_NVIC_SetPriority(SDIO_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(SDIO_IRQn);
    
    // 初始化MMC
    status = HAL_MMC_Init(&hmmc);
    if (status != HAL_OK) 
    {
        uint32_t error = HAL_MMC_GetError(&hmmc);
        printf("MMC初始化失败，错误码: 0x%08X\r\n", error);
        return status;
    }
    
    // 配置为4位总线模式
    status = HAL_MMC_ConfigWideBusOperation(&hmmc, SDIO_BUS_WIDE_4B);
    if (status != HAL_OK) 
    {
        printf("MMC配置4位总线失败\r\n");
        return status;
    }
    
    return HAL_OK;
}


/* USER CODE END 1 */
