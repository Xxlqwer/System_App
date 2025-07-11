/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file   fatfs.h
  * @brief  Header for fatfs applications
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
#ifndef __fatfs_H
#define __fatfs_H
#ifdef __cplusplus
 extern "C" {
#endif

#include "ff.h"
#include "ff_gen_drv.h"
#include "user_diskio.h" /* defines USER_Driver as external */

/* USER CODE BEGIN Includes */
// 添加存储空间阈值定义 (100MB)
//#define STORAGE_FULL_THRESHOLD (100 * 1024 * 1024)  // 100MB
//#define STORAGE_FULL_THRESHOLD  (26ULL * 1024ULL * 1024ULL * 1024ULL)  // 26GB（字节）
//#define STORAGE_FULL_THRESHOLD  (3ULL * 1024ULL * 1024ULL * 1024ULL) //0706
#define MAX_FILES_TO_DELETE 10  // 最大删除文件数，限制删除5个文件

// 配置文件路径
#define CONFIG_FILE_PATH "0:/config.txt"

#define BOARD_FOLDER "Board3"

/* USER CODE END Includes */

extern uint8_t retUSER; /* Return value for USER */
extern char USERPath[4]; /* USER logical drive path */
extern FATFS USERFatFS; /* File system object for USER logical drive */
extern FIL USERFile; /* File object for USER */

void MX_FATFS_Init(void);
void CheckStorageSpace(void);
FIL* FATFS_CreateRawDataFile(void);
FIL* FATFS_CreateRawDataFileWithTimestamp(void);
void Check_CreateConfig(void);
void LoadConfigFromFile(void);
void UpdateConfigUseTimeFlag(uint8_t newValue);
int Format_EMMC(void);
void RTC_SetTimeFromConfig(void);
int FATFS_Mount(void);

/* USER CODE BEGIN Prototypes */

//typedef struct {
//    uint16_t sample_rate;
//    uint32_t space_threshold_mb;  // 存储空间阈值（单位：MB）
//		uint8_t board_id;  
//} ConfigSettings;
typedef struct {
    uint16_t sample_rate;
    uint32_t space_threshold_mb;
    uint8_t board_id;
    uint8_t use_time_from_config;
    char config_time_string[32];  // 格式: "2025-07-09 08:30:00"
} ConfigSettings;

extern ConfigSettings g_config;  // 全局配置

// 添加全局变量声明
extern uint8_t currentFileYear;
extern uint8_t currentFileMonth;
extern uint8_t currentFileDay;
extern uint8_t currentFileHour;

/* USER CODE END Prototypes */
#ifdef __cplusplus
}
#endif
#endif /*__fatfs_H */
