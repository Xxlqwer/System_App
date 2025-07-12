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
//��ǿ���SDIO��λ���س�ʼ������
HAL_StatusTypeDef SDIO_Reinitialize(void) {
    uint8_t retry_count = 0;
    HAL_StatusTypeDef status;
    
    printf("��ʼSDIO��ȫ�س�ʼ��...\r\n");
    
    // ��ȫ����ʼ����ǰSDIO
    HAL_MMC_DeInit(&hmmc);
    
    // ����SDIO�ж�
    HAL_NVIC_DisableIRQ(SDIO_IRQn);
    
    // ����DMA�ж�
    HAL_NVIC_DisableIRQ(DMA2_Stream3_IRQn);
    HAL_NVIC_DisableIRQ(DMA2_Stream6_IRQn);
    
    // ����ʼ��DMA
    HAL_DMA_DeInit(&hdma_sdio_tx);
    HAL_DMA_DeInit(&hdma_sdio_rx);
    
    // ǿ�Ƹ�λSDIO
    __HAL_RCC_SDIO_FORCE_RESET();
    HAL_Delay(20);
    __HAL_RCC_SDIO_RELEASE_RESET();
    
    // ����SDIOʱ��
    __HAL_RCC_SDIO_CLK_DISABLE();
    HAL_Delay(100);  // ��eMMC�㹻��ʱ�临λ
    
    // ��������GPIO
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12);
    HAL_GPIO_DeInit(GPIOD, GPIO_PIN_2);
    
    // ����ѭ��
    while (retry_count < 3) {
        printf("SDIO��ʼ������ %d/3...\r\n", retry_count + 1);
        
        // ���³�ʼ��SDIO
        status = MX_SDIO_MMC_Init_Safe();
        
        if (status == HAL_OK) {
            printf("SDIO��ʼ���ɹ�!\r\n");
            return HAL_OK;
        }
        
        // ʧ�ܣ��ȴ�����ʱ�������
        HAL_Delay(200 + retry_count * 100);
        retry_count++;
    }
    
    printf("SDIO�س�ʼ��ʧ�ܣ��ѳ���%d��\r\n", retry_count);
    return HAL_ERROR;
}
//����һ�������������ϵİ�ȫ��ʼ��������
HAL_StatusTypeDef MX_SDIO_MMC_Init_Safe(void) 
{
    HAL_StatusTypeDef status;
    
    // ����ʱ��
    __HAL_RCC_SDIO_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    
    // ����GPIO
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF12_SDIO;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
    
    // ����SDIO
    hmmc.Instance = SDIO;
    hmmc.Init.ClockEdge = SDIO_CLOCK_EDGE_RISING;
    hmmc.Init.ClockBypass = SDIO_CLOCK_BYPASS_DISABLE;
    hmmc.Init.ClockPowerSave = SDIO_CLOCK_POWER_SAVE_DISABLE;
    hmmc.Init.BusWide = SDIO_BUS_WIDE_1B; // ����1λģʽ��ʼ��
    hmmc.Init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_DISABLE;
    hmmc.Init.ClockDiv = 2; // ���ͳ�ʼʱ��Ƶ��
    
    // ��ʼ��DMA
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
        printf("DMA TX��ʼ��ʧ��\r\n");
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
        printf("DMA RX��ʼ��ʧ��\r\n");
        return HAL_ERROR;
    }
    
    __HAL_LINKDMA(&hmmc, hdmarx, hdma_sdio_rx);
    
    // �����ж�
    HAL_NVIC_SetPriority(SDIO_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(SDIO_IRQn);
    
    // ��ʼ��MMC
    status = HAL_MMC_Init(&hmmc);
    if (status != HAL_OK) 
    {
        uint32_t error = HAL_MMC_GetError(&hmmc);
        printf("MMC��ʼ��ʧ�ܣ�������: 0x%08X\r\n", error);
        return status;
    }
    
    // ����Ϊ4λ����ģʽ
    status = HAL_MMC_ConfigWideBusOperation(&hmmc, SDIO_BUS_WIDE_4B);
    if (status != HAL_OK) 
    {
        printf("MMC����4λ����ʧ��\r\n");
        return status;
    }
    
    return HAL_OK;
}


/* USER CODE END 1 */
