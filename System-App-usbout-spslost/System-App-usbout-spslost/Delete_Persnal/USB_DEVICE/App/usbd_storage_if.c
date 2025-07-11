/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : usbd_storage_if.c
  * @version        : v1.0_Cube
  * @brief          : Memory management layer.
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
#include "usbd_storage_if.h"

/* USER CODE BEGIN INCLUDE */
#include "data_fusion.h"


/* USER CODE END INCLUDE */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

/* USER CODE END PV */

/** @addtogroup STM32_USB_OTG_DEVICE_LIBRARY
  * @brief Usb device.
  * @{
  */

/** @defgroup USBD_STORAGE
  * @brief Usb mass storage device module
  * @{
  */

/** @defgroup USBD_STORAGE_Private_TypesDefinitions
  * @brief Private types.
  * @{
  */

/* USER CODE BEGIN PRIVATE_TYPES */

/* USER CODE END PRIVATE_TYPES */

/**
  * @}
  */

/** @defgroup USBD_STORAGE_Private_Defines
  * @brief Private defines.
  * @{
  */

#define STORAGE_LUN_NBR                  1
#define STORAGE_BLK_NBR                  0x10000
#define STORAGE_BLK_SIZ                  0x200

/* USER CODE BEGIN PRIVATE_DEFINES */
extern MMC_HandleTypeDef hmmc;	// eMMC控制句柄


//逻辑单元号的数量 Logical Unit Number, LUN  1
//逻辑块数量 Logical Block Number, LBN   0x10000 ->十进制 65536 块
//逻辑块的大小 Logical Block Size, LBS   0x200 -> 十进制 512 字节
// 1MB=1024x1024字节
// 总容量=65536×512=33,554,432字节=32MB


/* USER CODE END PRIVATE_DEFINES */

/**
  * @}
  */

/** @defgroup USBD_STORAGE_Private_Macros
  * @brief Private macros.
  * @{
  */

/* USER CODE BEGIN PRIVATE_MACRO */

/* USER CODE END PRIVATE_MACRO */

/**
  * @}
  */

/** @defgroup USBD_STORAGE_Private_Variables
  * @brief Private variables.
  * @{
  */

/* USER CODE BEGIN INQUIRY_DATA_HS */
/** USB Mass storage Standard Inquiry Data. */
const int8_t STORAGE_Inquirydata_HS[] = {/* 36 */

  /* LUN 0 */
  0x00,
  0x80,
  0x02,
  0x02,
  (STANDARD_INQUIRY_DATA_LEN - 5),
  0x00,
  0x00,
  0x00,
  'S', 'T', 'M', ' ', ' ', ' ', ' ', ' ', /* Manufacturer : 8 bytes */
  'P', 'r', 'o', 'd', 'u', 'c', 't', ' ', /* Product      : 16 Bytes */
  ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
  '0', '.', '0' ,'1'                      /* Version      : 4 Bytes */
};
/* USER CODE END INQUIRY_DATA_HS */

/* USER CODE BEGIN PRIVATE_VARIABLES */

/* USER CODE END PRIVATE_VARIABLES */

/**
  * @}
  */

/** @defgroup USBD_STORAGE_Exported_Variables
  * @brief Public variables.
  * @{
  */

extern USBD_HandleTypeDef hUsbDeviceHS;

/* USER CODE BEGIN EXPORTED_VARIABLES */

/* USER CODE END EXPORTED_VARIABLES */

/**
  * @}
  */

/** @defgroup USBD_STORAGE_Private_FunctionPrototypes
  * @brief Private functions declaration.
  * @{
  */

static int8_t STORAGE_Init_HS(uint8_t lun);
static int8_t STORAGE_GetCapacity_HS(uint8_t lun, uint32_t *block_num, uint16_t *block_size);
static int8_t STORAGE_IsReady_HS(uint8_t lun);
static int8_t STORAGE_IsWriteProtected_HS(uint8_t lun);
static int8_t STORAGE_Read_HS(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len);
static int8_t STORAGE_Write_HS(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len);
static int8_t STORAGE_GetMaxLun_HS(void);

/* USER CODE BEGIN PRIVATE_FUNCTIONS_DECLARATION */

/* USER CODE END PRIVATE_FUNCTIONS_DECLARATION */

/**
  * @}
  */

USBD_StorageTypeDef USBD_Storage_Interface_fops_HS =
{
  STORAGE_Init_HS,
  STORAGE_GetCapacity_HS,
  STORAGE_IsReady_HS,
  STORAGE_IsWriteProtected_HS,
  STORAGE_Read_HS,
  STORAGE_Write_HS,
  STORAGE_GetMaxLun_HS,
  (int8_t *)STORAGE_Inquirydata_HS
};

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Initializes the storage unit (medium).
  * @param  lun: Logical unit number.
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
int8_t STORAGE_Init_HS(uint8_t lun)
{
  /* USER CODE BEGIN 9 */
  UNUSED(lun);


  return (USBD_OK);
  /* USER CODE END 9 */
}

/**
  * @brief  Returns the medium capacity.
  * @param  lun: Logical unit number.
  * @param  block_num: Number of total block number.
  * @param  block_size: Block size.
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
int8_t STORAGE_GetCapacity_HS(uint8_t lun, uint32_t *block_num, uint16_t *block_size)
{
  /* USER CODE BEGIN 10 */
  //UNUSED(lun);

//  *block_num  = STORAGE_BLK_NBR;
//  *block_size = STORAGE_BLK_SIZ;

	//CloseAllFilesIfAny();      // 确保当前数据文件关闭
	//SystemState = STATE_USB;
	//Software_Reset();
//	*block_num  = 61079552-1; 
//  *block_size = 512; 
//	return USBD_OK;
	
	HAL_MMC_CardInfoTypeDef emmc_info;   //emmc 大小信息结构体
	if(HAL_MMC_GetCardState(&hmmc) == HAL_MMC_CARD_TRANSFER){
		HAL_MMC_GetCardInfo(&hmmc,&emmc_info);
		*block_num  = emmc_info.LogBlockNbr-1; //61079552   
    *block_size = emmc_info.LogBlockSize; //512
	
		return USBD_OK;
	}
	
	return USBD_FAIL;
	
	
//	int8_t ret = USBD_FAIL;
//	HAL_MMC_GetCardInfo(&hmmc,&emmc_info);
//	*block_num  = emmc_info.LogBlockNbr -1;
//  *block_size = emmc_info.LogBlockSize;
//	ret = USBD_OK;
//	
//	return ret;
	
//  return (USBD_OK);
  /* USER CODE END 10 */
}

/**
  * @brief   Checks whether the medium is ready.
  * @param  lun:  Logical unit number.
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
int8_t STORAGE_IsReady_HS(uint8_t lun)
{
  /* USER CODE BEGIN 11 */
	//printf("Isready\n\t ");
	uint8_t state = 0;
	state = HAL_MMC_GetCardState(&hmmc); //0x04  SD卡的例子也是0x04
	if(state != HAL_MMC_STATE_READY)
	{
		//return USBD_FAIL; //state=0x04   HAL_MMC_STATE_PROGRAMMING = 0x00000004U, 
		//	HAL_Delay(500);
	
	}
	
  return (USBD_OK);
  /* USER CODE END 11 */
}

/**
  * @brief  Checks whether the medium is write protected.
  * @param  lun: Logical unit number.
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
int8_t STORAGE_IsWriteProtected_HS(uint8_t lun)
{
  /* USER CODE BEGIN 12 */
  return (USBD_OK);
  /* USER CODE END 12 */
}

/**
  * @brief  Reads data from the medium.
  * @param  lun: Logical unit number.
  * @param  buf: data buffer.
  * @param  blk_addr: Logical block address.
  * @param  blk_len: Blocks number.
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
int8_t STORAGE_Read_HS(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len)
{
  /* USER CODE BEGIN 13 */
//  UNUSED(lun);
//  UNUSED(buf);
//  UNUSED(blk_addr);
//  UNUSED(blk_len);
	
//	return (USBD_OK);

	 int8_t ret = USBD_FAIL;
	 HAL_StatusTypeDef status;
	
	// 调用HAL函数读取eMMC块
	//if(HAL_MMC_ReadBlocks(&hmmc, buf, blk_addr,  blk_len, HAL_MAX_DELAY) == HAL_OK)
	  status = HAL_MMC_ReadBlocks_DMA(&hmmc, buf, blk_addr, blk_len);
		if(status == HAL_OK)
		{
			ret = USBD_OK;  // 如果读取成功，则等待eMMC状态转换为READY
		

			//等待MMC空闲
		while(HAL_MMC_GetState(&hmmc) == HAL_MMC_STATE_BUSY){
		 //printf("Waiting for MMC to be ready...\n");
		};
		while(HAL_MMC_GetCardState(&hmmc) != HAL_MMC_CARD_TRANSFER){
		//printf("Waiting for card to reach TRANSFER state...\n");
		};
		
		//printf("Read ok\n"); //连接usb后可以执行到这一步
		
	}
		else
		{
		
		  // 打印错误信息
        //printf("Read operation failed. HAL status: %d\n", status);
        //printf("MMC error code: 0x%08X\n", hmmc.ErrorCode);
	
		}
	
	  return ret;
	
  /* USER CODE END 13 */
}

/**
  * @brief  Writes data into the medium.
  * @param  lun: Logical unit number.
  * @param  buf: data buffer.
  * @param  blk_addr: Logical block address.
  * @param  blk_len: Blocks number.
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
int8_t STORAGE_Write_HS(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len)
{
  /* USER CODE BEGIN 14 */
//  UNUSED(lun);
//  UNUSED(buf);
//  UNUSED(blk_addr);
//  UNUSED(blk_len);
	
		int8_t ret = USBD_FAIL;
	HAL_StatusTypeDef status;
	// 调用HAL函数向eMMC块写入数据
	//if(HAL_MMC_WriteBlocks(&hmmc, buf, blk_addr,  blk_len, HAL_MAX_DELAY) == HAL_OK){
	 status = HAL_MMC_WriteBlocks_DMA(&hmmc, buf, blk_addr, blk_len);
	if(status == HAL_OK)
	{
		 // 如果写入成功，则等待eMMC状态转换为READY
		ret = USBD_OK;
		//等待MMC空闲
		while(HAL_MMC_GetState(&hmmc) == HAL_MMC_STATE_BUSY){
			 //printf("Waiting for MMC to be ready...\n");
		};
		
		while(HAL_MMC_GetCardState(&hmmc) != HAL_MMC_CARD_TRANSFER){
		//printf("Waiting for card to reach TRANSFER state...\n");
		};
	
		//printf("Write operation completed successfully.\n");
	}
	else
	{
		//printf("Write operation failed. HAL status:%d\n",status);
    //printf("MMC error code: 0x%08X\n", hmmc.ErrorCode);
	}
	
	return ret;
	
  /* USER CODE END 14 */
}

/**
  * @brief  Returns the Max Supported LUNs.
  * @param  None
  * @retval Lun(s) number.
  */
int8_t STORAGE_GetMaxLun_HS(void)
{
  /* USER CODE BEGIN 15 */
  return (STORAGE_LUN_NBR - 1);
  /* USER CODE END 15 */
}

/* USER CODE BEGIN PRIVATE_FUNCTIONS_IMPLEMENTATION */

/* USER CODE END PRIVATE_FUNCTIONS_IMPLEMENTATION */

/**
  * @}
  */

/**
  * @}
  */

