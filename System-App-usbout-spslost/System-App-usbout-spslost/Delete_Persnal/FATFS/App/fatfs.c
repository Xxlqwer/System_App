/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file   fatfs.c
  * @brief  用于 fatfs 应用程序的代码
  ******************************************************************************
  * @attention
  *
  * 版权所有 (c) 2025 STMicroelectronics。
  * 保留所有权利。
  *
  * 本软件按许可条款提供，许可条款可在软件组件的根目录中的 LICENSE 文件中找到。
  * 如果未随软件提供 LICENSE 文件，则按"原样"提供。
  *
  ******************************************************************************
  */
/* USER CODE END Header */
#include "fatfs.h"

/* USER CODE BEGIN Variables */
#include <stdio.h>
#include <string.h>
#include <stdbool.h> 
#include "rtc.h"
#include "ff.h"  

uint8_t retUSER;    /* USER 的返回值 */
char USERPath[4];   /* USER 逻辑驱动器路径 */
FATFS USERFatFS;    /* USER 逻辑驱动器的文件系统对象 */
FIL USERFile;       /* USER 的文件对象 */


ConfigSettings g_config = {
    .sample_rate = 2000,
    .space_threshold_mb = 3072,  // 默认 3GB
		.board_id =4 ,           
};


static char timestamp_filename[30]; // 存储带时间戳的文件名
//static uint8_t lastRecordedHour =255;  // 初始设为非法小时
// 添加全局变量跟踪当前文件的时间
extern uint8_t currentFileHour;   // 初始化为无效值
extern uint8_t currentFileDay;    // 初始化为无效值
extern MMC_HandleTypeDef hmmc;	// eMMC控制句柄


/* USER CODE END Variables */

// 比较两个文件的时间（DOS格式）
bool IsFileOlder(FILINFO* f1, FILINFO* f2) 
{
    // 比较日期（年、月、日）
    if (f1->fdate != f2->fdate) 
    {
        return f1->fdate < f2->fdate;
    }
    // 日期相同时比较时间（时、分、秒）
    return f1->ftime < f2->ftime;
}

// 在全局变量部分添加
uint8_t currentFileYear = 255;
uint8_t currentFileMonth = 255;
uint8_t currentFileDay = 255;
uint8_t currentFileHour = 255;

FIL* FATFS_CreateRawDataFileWithTimestamp(void)
{
   
    static FIL file;
    FRESULT res;
    char filename[64];
    char folder[32];
    RTC_TimeTypeDef sTime;
    RTC_DateTypeDef sDate;

    // 清空句柄，防锁残留
    memset(&file, 0, sizeof(FIL));

    // 获取当前时间
    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

    // 更新当前文件时间
    currentFileYear  = sDate.Year;
    currentFileMonth = sDate.Month;
    currentFileDay   = sDate.Date;
    currentFileHour  = sTime.Hours;

    // 构造目录路径
    snprintf(folder, sizeof(folder), "0:/Board%u", g_config.board_id);

    // 确保目录存在
    res = f_mkdir(folder);
    if (res != FR_OK && res != FR_EXIST) {
        printf("无法创建目录 %s，错误: %d\r\n", folder, res);
        return NULL;
    }

    // 构造文件名（带时间戳）
    snprintf(filename, sizeof(filename),
             "%s/%04d%02d%02d%02d%02d%02d.hex",
             folder,
             2000 + sDate.Year,
             sDate.Month,
             sDate.Date,
             sTime.Hours,
             sTime.Minutes,
             sTime.Seconds);

    // 打开文件（若已存在则追加）
    res = f_open(&file, filename, FA_OPEN_ALWAYS | FA_WRITE);
    if (res != FR_OK) {
        printf("无法打开数据文件: %s, 错误: %d\r\n", filename, res);
        return NULL;
    }

    // 移动到文件末尾以实现追加写入
    res = f_lseek(&file, f_size(&file));
    if (res != FR_OK) {
        printf("文件定位失败: %d\r\n", res);
        f_close(&file);
        return NULL;
    }

    // 状态输出
    if (f_size(&file) == 0) {
        printf("新建HEX数据文件: %s\r\n", filename);
    } else {
        printf("续写HEX数据文件: %s\r\n", filename);
    }

    return &file;

}


void MX_FATFS_Init(void)
{
  /*## FatFS：链接 USER 驱动器 ###########################*/
  retUSER = FATFS_LinkDriver(&USER_Driver, USERPath);

  /* USER CODE BEGIN Init */
  /* 用于初始化的附加用户代码 */
  /* USER CODE END Init */
}

/**
  * @brief  从 RTC 获取时间
  * @param  无
  * @retval 时间 (DWORD 格式)
  */
DWORD get_fattime(void)
{
  /* USER CODE BEGIN get_fattime */
  //return 0;
	
	RTC_DateTypeDef date;
    RTC_TimeTypeDef time;
    HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);
    
    return ((DWORD)(date.Year + 20) << 25) |
           ((DWORD)date.Month << 21) |
           ((DWORD)date.Date << 16) |
           ((DWORD)time.Hours << 11) |
           ((DWORD)time.Minutes << 5) |
           ((DWORD)time.Seconds >> 1);
  /* USER CODE END get_fattime */
}

/* USER CODE BEGIN Application */
#include "ff.h"
#include "diskio.h"
#include <rtc.h>

FATFS fs; // 文件系统对象
FRESULT res;
BYTE work[_MAX_SS];

int FATFS_Mount(void)
{
    /** f_mount 函数用于挂载指定的文件系统，使其可用于后续文件操作（如读/写文件、创建目录等）。
     * &fs    - 指向 FATFS 结构的指针。FATFS 结构在 FatFs 库中用于存储文件系统的信息。
     * ""     - 要挂载的驱动器名称或路径。"" 表示默认驱动器，即 FatFs 将尝试挂载第一个可用驱动器。若要挂载特定驱动器，请提供其名称，例如 "0:"、"1:" 等。
     * 1      - 挂载选项。值为 1 表示将文件系统设置为读/写模式。此选项也可设置为 0，表示只读模式。
     */
    // 先卸载已挂载的文件系统
    f_mount(NULL, "", 0);
    
    res = f_mount(&fs, "", 1); /* 挂载选项。值为 1 表示将文件系统设置为读/写模式。此选项也可设置为 0，表示只读模式。 */
    if (res == FR_OK)
    {
        printf("FatFs 挂载成功。\r\n");
        return 0;
    }
    else if (res == FR_NO_FILESYSTEM)
    {
        printf("未找到文件系统。正在格式化...\r\n");

        /** f_mkfs 函数格式化指定的存储设备（本例中为 0: 驱动器）并在其上创建新的文件系统（FAT32）。调用此函数将擦除目标设备上的所有数据，因为它会完全重新格式化设备并创建新的文件系统结构。
         * 0:           - 目标驱动器名称。"0:" 表示第一个驱动器。FatFs 允许通过驱动器名称指定目标设备，通常为 0:、1: 等。
         * FS_FAT32     - 文件系统类型。FS_FAT32 表示创建 FAT32 文件系统。根据存储需求和支持的格式，也可选择 FS_FAT12、FS_FAT16 或 FS_EXFAT。
         * 0            - 文件系统选项。通常设为 0 表示默认配置，除非有特殊要求。
         * work         - 工作缓冲区（缓存区），用于在文件系统创建过程中存储临时数据。FatFs 在格式化期间需要足够大的内存块作为工作区。
         * sizeof(work) - 工作区大小（即 work 数组的大小）。告知 f_mkfs 函数可用的工作空间大小。FatFs 使用此内存完成格式化期间的各种操作。
         */
        res = f_mkfs("0:", FS_FAT32, 0, work, sizeof(work));
        if (res == FR_OK)
        {
            printf("格式化成功。正在重新挂载...\r\n");
            res = f_mount(&fs, "", 1);
            if (res == FR_OK)
            {
                printf("格式化后 FatFs 挂载成功。\r\n");
                return 0;
            }
            else
            {
                printf("格式化后挂载失败。错误：%d\r\n", res);
                return -1;
            }
        }
        else
        {
            printf("格式化失败。错误：%d\r\n", res);
            return -1;
        }
    }
    else
    {
        printf("FatFs 挂载失败，错误：%d\r\n", res);
        return -1;
    }
}

/**
 * @brief 创建二进制数据文件
 * @return 文件句柄指针，失败返回NULL
 */
FIL* FATFS_CreateRawDataFile(void)
{
    static FIL file;
    FRESULT res;
    
    // 使用当前时间生成文件名 (避免覆盖旧文件)
    RTC_TimeTypeDef sTime;
    RTC_DateTypeDef sDate;
    char filename[32];
    
    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
    
    snprintf(filename, sizeof(filename), "data_%02d%02d%02d.bin", 
             sTime.Hours, sTime.Minutes, sTime.Seconds);

    // 创建二进制文件
    res = f_open(&file, filename, FA_CREATE_ALWAYS | FA_WRITE);
    if (res != FR_OK) 
    {
        printf("创建数据文件失败: %d\r\n", res);
        return NULL;
    }
    
    printf("已创建二进制数据文件: %s\r\n", filename);
    return &file;
}
int FATFS_RW_Test(void)
{
    FIL file;
    FRESULT res;
    UINT bw, br;
    char write_data[]  = "Hello, this is EMMC DMA FatFs test.";
    char read_data[64] = { 0 };

    // 1. 创建文件并写入内容
    res = f_open(&file, "test.csv", FA_CREATE_ALWAYS | FA_WRITE);
    if (res != FR_OK)
    {
        printf("f_open (写入) 失败：%d\r\n", res);
        return -1;
    }

    res = f_write(&file, write_data, strlen(write_data), &bw);
    if (res != FR_OK || bw != strlen(write_data))
    {
        printf("f_write 失败：%d\r\n", res);
        f_close(&file);
        return -1;
    }

    f_close(&file);
    printf("文件写入成功。\r\n");

    // 2. 打开文件并读取内容
    res = f_open(&file, "test.csv", FA_READ);
    if (res != FR_OK)
    {
        printf("f_open (读取) 失败：%d\r\n", res);
        return -1;
    }

    res = f_read(&file, read_data, sizeof(read_data) - 1, &br);
    if (res != FR_OK)
    {
        printf("f_read 失败：%d\r\n", res);
        f_close(&file);
        return -1;
    }

    f_close(&file);
    printf("从文件读取：%s\r\n", read_data);

    // 3. 验证内容
    if (strcmp(write_data, read_data) == 0)
    {
        printf("FatFs EMMC 读/写测试通过！\r\n");
        return 0;
    }
    else
    {
        printf("FatFs EMMC 读/写测试失败！\r\n");
        return -1;
    }
}

// FATFS_RW_Test1/2/3 的类似翻译
// [...] (为简洁起见省略，翻译模式相同)

/**
 * @brief FATFS 多行数据写入测试（无读取验证）
 * @return 0 成功, -1 失败
 */
int FATFS_MultiLineWriteTest(void)
{
    FIL file;
    FRESULT res;
    UINT bw;
    const char *write_data[] = {
        "Line 1: Hello, this is EMMC DMA FatFs test.",
        "Line 2: Writing multiple lines to a file.",
        "Line 3: Last line of the test data.",
        // 可在此添加更多行...
    };
    int line_count = sizeof(write_data) / sizeof(write_data[0]);

    // 1. 创建文件（覆盖模式）
    res = f_open(&file, "multiline_test.csv", FA_CREATE_ALWAYS | FA_WRITE);
    if (res != FR_OK) {
        printf("[错误] 创建文件失败：%d\r\n", res);
        return -1;
    }

    // 2. 逐行写入数据
    for (int i = 0; i < line_count; i++) {
        // 写入数据行
        res = f_write(&file, write_data[i], strlen(write_data[i]), &bw);
        if (res != FR_OK || bw != strlen(write_data[i])) {
            printf("[错误] 写入第 %d 行失败：%d\r\n", i + 1, res);
            f_close(&file);
            return -1;
        }

        // 写入换行符（Windows：\r\n，Linux：\n）
        res = f_write(&file, "\n", 1, &bw);
        if (res != FR_OK || bw != 1) {
            printf("[错误] 在第 %d 行写入换行符失败：%d\r\n", i + 1, res);
            f_close(&file);
            return -1;
        }
    }

    // 3. 刷新并关闭文件
    res = f_close(&file);
    if (res != FR_OK) {
        printf("[错误] 关闭文件失败：%d\r\n", res);
        return -1;
    }

    printf("[成功] 向文件写入 %d 行。\r\n", line_count);
    return 0;
}


/**
  * @brief  删除最早的数据文件
  * @retval FRESULT 操作结果
  */
FRESULT DeleteOldestFile(void) 
{
    FRESULT res;
    DIR dir;
    FILINFO fno;
    FILINFO oldest_fno = {0};
    char path[64];
    char oldest_path[128] = {0};
		
		 // 组装当前板号目录路径
    snprintf(path, sizeof(path), "0:/Board%u", g_config.board_id);
    
    res = f_opendir(&dir, "0:/");
    if (res != FR_OK) return res;
    
    // 查找最旧的文件
    while (1) 
    {
        res = f_readdir(&dir, &fno);
        if (res != FR_OK || fno.fname[0] == 0) break;
        
       // 只处理符合条件的文件
        if (strstr(fno.fname, ".hex") || strstr(fno.fname, "DATA_"))
        {
            // 第一次文件或找到修改时间更早的文件
            if (oldest_fno.fname[0] == 0 || 
                fno.fdate < oldest_fno.fdate || 
                (fno.fdate == oldest_fno.fdate && fno.ftime < oldest_fno.ftime) || 
                (fno.fdate == oldest_fno.fdate && fno.ftime == oldest_fno.ftime && strcmp(fno.fname, oldest_fno.fname) < 0)) 
            {
                oldest_fno = fno;
                snprintf(oldest_path, sizeof(oldest_path), "0:/%s", fno.fname);
            }
        }
    }
    f_closedir(&dir);
    
    // 删除找到的最旧文件
    if (oldest_path[0] != 0) 
    {
        printf("删除文件: %s\r\n", oldest_path);
        return f_unlink(oldest_path);
    }
    
    return FR_NO_FILE;
}


/**
  * @brief  检查存储空间并删除旧文件
  * @retval None
  */
// 修改 CheckStorageSpace 函数
void CheckStorageSpace(void) 
{
    FATFS* fs;
    DWORD free_clusters, total_clusters;
    FRESULT res;
    uint64_t total_space, free_space;
    
    res = f_getfree("0:", &free_clusters, &fs);
    if (res != FR_OK) 
    {
        printf("获取存储空间失败: %d\r\n", res);
        return;
    }
		// 计算文件系统总空间
    total_clusters = fs->n_fatent - 2;  // 总簇数
    uint32_t sectors_per_cluster = fs->csize;  // 每簇扇区数
    total_space = (uint64_t)total_clusters * sectors_per_cluster * 512;
    free_space = (uint64_t)free_clusters * sectors_per_cluster * 512;
			 // 输出文件系统存储空间
    printf("文件系统存储空间: 总共 %llu MB, 可用 %llu MB\r\n", 
           total_space / (1024 * 1024), free_space / (1024 * 1024));
    
		
		 // 获取 eMMC 实际存储信息
    HAL_MMC_CardInfoTypeDef emmc_info;  // eMMC 信息结构体
    if(HAL_MMC_GetCardState(&hmmc) == HAL_MMC_CARD_TRANSFER) {
        // 获取 eMMC 卡信息
        HAL_MMC_GetCardInfo(&hmmc, &emmc_info);
        
        // 获取eMMC总空间
        uint64_t emmc_total_space = (uint64_t)emmc_info.LogBlockNbr * emmc_info.LogBlockSize;
        
        // 计算eMMC可用空间
        uint64_t emmc_free_space = (uint64_t)free_clusters * fs->csize * 512;  // 可用空间
        
        // 输出 eMMC 存储空间信息
        printf("eMMC 存储空间: 总共 %llu MB, 可用 %llu MB\r\n", 
               emmc_total_space / (1024 * 1024), emmc_free_space / (1024 * 1024));
    }
		
	

   // 检查是否低于阈值
   if (free_space < (uint64_t)g_config.space_threshold_mb * 1024 * 1024) 
    {
        printf("存储空间不足，删除旧文件...\r\n");
        while (free_space < (uint64_t)g_config.space_threshold_mb * 1024 * 1024)
        {
            res = DeleteOldestFile();
            if (res != FR_OK) 
            {
                if (res == FR_NO_FILE) printf("没有更多文件可删除\r\n");
                else printf("删除文件失败: %d\r\n", res);
                break;
            }
            
            // 重新计算可用空间
            res = f_getfree("0:", &free_clusters, &fs);
            if (res != FR_OK) break;
            
            free_space = (uint64_t)free_clusters * sectors_per_cluster * 512;
            printf("删除后可用空间: %llu MB\r\n", free_space / (1024 * 1024));
        }
    }
}

/**
 * @brief 从 config.txt 读取配置并更新 g_config（单位：MB）
 */
void FATFS_ReadConfigFromUSB(void)
{
    FIL file;
    FRESULT res;
    char line[64];
    UINT br;

    res = f_open(&file, "0:/config.txt", FA_READ);
    if (res != FR_OK) {
        printf("未找到配置文件 config.txt，使用默认配置\r\n");
        return;
    }

    while (f_gets(line, sizeof(line), &file)) {
        char key[32], value[32];
        if (sscanf(line, "%31[^=]=%31s", key, value) == 2) {
            if (strcmp(key, "sample_rate") == 0) {
                g_config.sample_rate = (uint16_t)atoi(value);
                printf("配置: 采样率 = %u SPS\r\n", g_config.sample_rate);
            } else if (strcmp(key, "space_threshold_mb") == 0) {
                g_config.space_threshold_mb = (uint32_t)atoi(value);
                printf("配置: 存储空间阈值 = %u MB\r\n", g_config.space_threshold_mb);
            }
        }
    }

    f_close(&file);
}

void Check_CreateConfig(void)
{
    FIL file;
    FRESULT res;

    // 1. 尝试以只读模式打开 config.txt
    res = f_open(&file, CONFIG_FILE_PATH, FA_READ);
    if (res == FR_OK)
    {
        // 文件已存在，关闭即可
        f_close(&file);
        printf("配置文件已存在，跳过创建。\r\n");
        return;
    }

    // 2. 文件不存在，创建并写入默认配置
    printf("配置文件不存在，正在创建默认配置...\r\n");
    res = f_open(&file, CONFIG_FILE_PATH, FA_CREATE_ALWAYS | FA_WRITE);
    if (res != FR_OK)
    {
        printf("创建配置文件失败，错误码: %d\r\n", res);
        return;
    }

    // 3. 写入默认内容
   const char *defaultConfig =
"sample_rate=2000\r\n"
"space_threshold_mb=3072\r\n"
"board_id=4\r\n"
"use_time_from_config=1\r\n"
"date_time=2025-07-09 12:00:00\r\n"
"\r\n"
"# 配置说明：\r\n"
"# sample_rate: 支持的采样率值（单位SPS）：125, 250, 500, 1000, 2000\r\n"
"# space_threshold_mb: 最低剩余空间（MB），不足时自动删除旧数据\r\n"
"# board_id: 当前板编号（决定文件夹 BoardX 名称）\r\n"
"# use_time_from_config: 0=不使用配置时间；1=使用 date_time 设置 RTC\r\n"
"# date_time: 配置时间，格式为 yyyy-mm-dd hh:mm:ss，仅在 use_time_from_config=1 时生效\r\n";

    UINT bytesWritten = 0;
    res = f_write(&file, defaultConfig, strlen(defaultConfig), &bytesWritten);
    if (res == FR_OK && bytesWritten == strlen(defaultConfig))
    {
        printf("默认配置文件写入成功。\r\n");
    }
    else
    {
        printf("写入配置文件失败，错误码: %d\r\n", res);
    }

		f_sync(&file);
    f_close(&file);
}

void LoadConfigFromFile(void)
{
    FIL file;
    char line[64];
    FRESULT res;

    res = f_open(&file, CONFIG_FILE_PATH, FA_READ);
    if (res != FR_OK)
    {
        printf("读取配置文件失败，错误码: %d\r\n", res);
        return;
    }
		memset(line, 0, sizeof(line));
		memset(g_config.config_time_string, 0, sizeof(g_config.config_time_string));

		while (f_gets(line, sizeof(line), &file))
    {
        // 清除行末换行符
        line[strcspn(line, "\r\n")] = '\0';

        if (strncmp(line, "sample_rate=", strlen("sample_rate=")) == 0)
        {
            sscanf(line + strlen("sample_rate="), "%hu", &g_config.sample_rate);
        }
        else if (strncmp(line, "space_threshold_mb=", strlen("space_threshold_mb=")) == 0)
        {
            sscanf(line + strlen("space_threshold_mb="), "%u", &g_config.space_threshold_mb);
        }
        else if (strncmp(line, "board_id=", strlen("board_id=")) == 0)
        {
            g_config.board_id = (uint8_t)atoi(line + strlen("board_id="));
        }
        else if (strncmp(line, "use_time_from_config=", strlen("use_time_from_config=")) == 0)
        {
            g_config.use_time_from_config = (uint8_t)atoi(line + strlen("use_time_from_config="));
        }
        else if (strncmp(line, "date_time=", strlen("date_time=")) == 0)
        {
            strncpy(g_config.config_time_string, line + strlen("date_time="), sizeof(g_config.config_time_string) - 1);
            g_config.config_time_string[sizeof(g_config.config_time_string) - 1] = '\0';
        }
    }

    f_close(&file);

    printf("配置读取完成: sample_rate=%hu, space_threshold_mb=%u MB, board_id=%u, use_time_from_config=%u, config_time_string=%s\r\n",
           g_config.sample_rate,
           g_config.space_threshold_mb,
           g_config.board_id,
           g_config.use_time_from_config,
           g_config.config_time_string);
}

void RTC_SetTimeFromConfig(void)
{
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};
    int year, month, day, hour, minute, second;

    if (sscanf(g_config.config_time_string, "%d-%d-%d %d:%d:%d",
               &year, &month, &day, &hour, &minute, &second) == 6)
    {
        sTime.Hours = hour;
        sTime.Minutes = minute;
        sTime.Seconds = second;

        sDate.Year = year - 2000;
        sDate.Month = month;
        sDate.Date = day;

        // 设置RTC时间
        if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK ||
            HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
        {
            printf("RTC 设置失败\r\n");
        }
        else
        {
            printf("RTC 已根据配置文件设置为: %s\r\n", g_config.config_time_string);
            
            // 更新配置标志并同步到文件
            UpdateConfigUseTimeFlag(0);
        }
    }
    else
    {
        printf("时间字符串解析失败: %s\r\n", g_config.config_time_string);
    }
}

void UpdateConfigUseTimeFlag(uint8_t newValue)
{
    FIL file;
    FRESULT res;
    char configContent[512] = {0};  // 存储整个配置文件内容
    char tempPath[32] = "0:/config_temp.txt";  // 临时文件名
    char line[128];
    UINT br, bw;
    bool updated = false;

    // 1. 打开原始配置文件
    res = f_open(&file, CONFIG_FILE_PATH, FA_READ);
    if (res != FR_OK) 
    {
        printf("无法打开配置文件进行更新，错误码: %d\r\n", res);
        return;
    }

    // 2. 读取整个配置文件
    while (f_gets(line, sizeof(line), &file)) 
    {
        // 检查是否是需要更新的行
        if (strncmp(line, "use_time_from_config=", strlen("use_time_from_config=")) == 0) 
        {
            // 替换为新的配置值
            snprintf(line, sizeof(line), "use_time_from_config=%u\r\n", newValue);
            updated = true;
        }
        
        // 添加到配置内容缓冲区
        strncat(configContent, line, sizeof(configContent) - strlen(configContent) - 1);
    }
    f_close(&file);

    // 3. 如果没有找到配置项，添加新行
    if (!updated) 
    {
        char newLine[32];
        snprintf(newLine, sizeof(newLine), "use_time_from_config=%u\r\n", newValue);
        strncat(configContent, newLine, sizeof(configContent) - strlen(configContent) - 1);
    }

    // 4. 创建临时文件并写入新内容
    res = f_open(&file, tempPath, FA_CREATE_ALWAYS | FA_WRITE);
    if (res != FR_OK) 
    {
        printf("无法创建临时配置文件，错误码: %d\r\n", res);
        return;
    }
    
    res = f_write(&file, configContent, strlen(configContent), &bw);
    if (res != FR_OK || bw != strlen(configContent)) 
    {
        printf("写入临时配置文件失败: %d\r\n", res);
        f_close(&file);
        f_unlink(tempPath);  // 删除临时文件
        return;
    }
    f_close(&file);
    
    // 5. 用临时文件替换原始配置文件
    f_unlink(CONFIG_FILE_PATH);  // 删除旧文件
    if (f_rename(tempPath, CONFIG_FILE_PATH) != FR_OK) 
    {
        printf("重命名临时文件失败，请手动恢复配置\r\n");
        f_unlink(tempPath);  // 清理临时文件
        return;
    }
    
    // 6. 更新内存中的配置值
    g_config.use_time_from_config = newValue;
    printf("配置文件标志位已更新: use_time_from_config=%d\r\n", newValue);
}

/**
  * @brief  格式化 eMMC
  * @retval 0 如果成功，其他值表示失败
  */
int Format_EMMC(void)
{
    FRESULT res;
    BYTE work[_MAX_SS];  // 工作区
    // 格式化 eMMC（0: 盘符，FS_FAT32 类型，默认簇大小，工作区）
    res = f_mkfs("0:", FS_FAT32, 0, work, sizeof(work));
    if (res != FR_OK)
    {
        printf("格式化失败！错误码: %d\r\n", res);
        return -1;  // 返回错误码
    }

    printf("格式化完成，正在重新挂载...\r\n");

    // 格式化成功，重新挂载
    res = f_mount(&fs, "0:", 1);  // 挂载文件系统
    if (res != FR_OK)
    {
        printf("重新挂载失败！错误码: %d\r\n", res);
        return -2;  // 挂载失败返回错误码
    }

    printf("eMMC 格式化并挂载成功\r\n");
    return 0;  // 成功
}




/** 结果
emmc 演示

EMMC_Getinfo_Test
EMMC_Info: 
CardType: 1
Class: 245
RelCardAdd: 2
BlockNbr: 61079552
BlockSize: 512
LogBlockNbr: 61079552
LogoBlockSize: 512
emmc 写入读取测试成功
未找到文件系统。正在格式化...
格式化成功。正在重新挂载...
格式化后 FatFs 挂载成功。
文件写入成功。
从文件读取：Hello, this is EMMC DMA FatFs test.
FatFs EMMC 读/写测试通过！
*/

/* USER CODE END Application */