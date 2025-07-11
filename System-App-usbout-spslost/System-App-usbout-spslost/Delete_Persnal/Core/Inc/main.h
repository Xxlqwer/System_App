/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */


/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */


/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LED0_Pin GPIO_PIN_0
#define LED0_GPIO_Port GPIOF
#define LED1_Pin GPIO_PIN_1
#define LED1_GPIO_Port GPIOF
#define LED2_Pin GPIO_PIN_2
#define LED2_GPIO_Port GPIOF
#define USB_OTG_HS_RST_Pin GPIO_PIN_1
#define USB_OTG_HS_RST_GPIO_Port GPIOC
#define GNSS_1PPS_OUT_Pin GPIO_PIN_0
#define GNSS_1PPS_OUT_GPIO_Port GPIOG
#define GNSS_RST_Pin GPIO_PIN_1
#define GNSS_RST_GPIO_Port GPIOG
#define AD_CS__Pin GPIO_PIN_11
#define AD_CS__GPIO_Port GPIOE
#define DRDY__Pin GPIO_PIN_12
#define DRDY__GPIO_Port GPIOD
#define SYNC_Pin GPIO_PIN_13
#define SYNC_GPIO_Port GPIOD
#define AD_RST__Pin GPIO_PIN_14
#define AD_RST__GPIO_Port GPIOD
#define PWDN__Pin GPIO_PIN_15
#define PWDN__GPIO_Port GPIOD
#define GNSS_PWR_CTL_Pin GPIO_PIN_2
#define GNSS_PWR_CTL_GPIO_Port GPIOG

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
