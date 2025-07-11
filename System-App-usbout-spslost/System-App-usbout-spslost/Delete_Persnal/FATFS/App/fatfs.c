/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file   fatfs.c
  * @brief  ���� fatfs Ӧ�ó���Ĵ���
  ******************************************************************************
  * @attention
  *
  * ��Ȩ���� (c) 2025 STMicroelectronics��
  * ��������Ȩ����
  *
  * ���������������ṩ�������������������ĸ�Ŀ¼�е� LICENSE �ļ����ҵ���
  * ���δ������ṩ LICENSE �ļ�����"ԭ��"�ṩ��
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

uint8_t retUSER;    /* USER �ķ���ֵ */
char USERPath[4];   /* USER �߼�������·�� */
FATFS USERFatFS;    /* USER �߼����������ļ�ϵͳ���� */
FIL USERFile;       /* USER ���ļ����� */


ConfigSettings g_config = {
    .sample_rate = 2000,
    .space_threshold_mb = 3072,  // Ĭ�� 3GB
		.board_id =4 ,           
};


static char timestamp_filename[30]; // �洢��ʱ������ļ���
//static uint8_t lastRecordedHour =255;  // ��ʼ��Ϊ�Ƿ�Сʱ
// ���ȫ�ֱ������ٵ�ǰ�ļ���ʱ��
extern uint8_t currentFileHour;   // ��ʼ��Ϊ��Чֵ
extern uint8_t currentFileDay;    // ��ʼ��Ϊ��Чֵ
extern MMC_HandleTypeDef hmmc;	// eMMC���ƾ��


/* USER CODE END Variables */

// �Ƚ������ļ���ʱ�䣨DOS��ʽ��
bool IsFileOlder(FILINFO* f1, FILINFO* f2) 
{
    // �Ƚ����ڣ��ꡢ�¡��գ�
    if (f1->fdate != f2->fdate) 
    {
        return f1->fdate < f2->fdate;
    }
    // ������ͬʱ�Ƚ�ʱ�䣨ʱ���֡��룩
    return f1->ftime < f2->ftime;
}

// ��ȫ�ֱ����������
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

    // ��վ������������
    memset(&file, 0, sizeof(FIL));

    // ��ȡ��ǰʱ��
    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

    // ���µ�ǰ�ļ�ʱ��
    currentFileYear  = sDate.Year;
    currentFileMonth = sDate.Month;
    currentFileDay   = sDate.Date;
    currentFileHour  = sTime.Hours;

    // ����Ŀ¼·��
    snprintf(folder, sizeof(folder), "0:/Board%u", g_config.board_id);

    // ȷ��Ŀ¼����
    res = f_mkdir(folder);
    if (res != FR_OK && res != FR_EXIST) {
        printf("�޷�����Ŀ¼ %s������: %d\r\n", folder, res);
        return NULL;
    }

    // �����ļ�������ʱ�����
    snprintf(filename, sizeof(filename),
             "%s/%04d%02d%02d%02d%02d%02d.hex",
             folder,
             2000 + sDate.Year,
             sDate.Month,
             sDate.Date,
             sTime.Hours,
             sTime.Minutes,
             sTime.Seconds);

    // ���ļ������Ѵ�����׷�ӣ�
    res = f_open(&file, filename, FA_OPEN_ALWAYS | FA_WRITE);
    if (res != FR_OK) {
        printf("�޷��������ļ�: %s, ����: %d\r\n", filename, res);
        return NULL;
    }

    // �ƶ����ļ�ĩβ��ʵ��׷��д��
    res = f_lseek(&file, f_size(&file));
    if (res != FR_OK) {
        printf("�ļ���λʧ��: %d\r\n", res);
        f_close(&file);
        return NULL;
    }

    // ״̬���
    if (f_size(&file) == 0) {
        printf("�½�HEX�����ļ�: %s\r\n", filename);
    } else {
        printf("��дHEX�����ļ�: %s\r\n", filename);
    }

    return &file;

}


void MX_FATFS_Init(void)
{
  /*## FatFS������ USER ������ ###########################*/
  retUSER = FATFS_LinkDriver(&USER_Driver, USERPath);

  /* USER CODE BEGIN Init */
  /* ���ڳ�ʼ���ĸ����û����� */
  /* USER CODE END Init */
}

/**
  * @brief  �� RTC ��ȡʱ��
  * @param  ��
  * @retval ʱ�� (DWORD ��ʽ)
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

FATFS fs; // �ļ�ϵͳ����
FRESULT res;
BYTE work[_MAX_SS];

int FATFS_Mount(void)
{
    /** f_mount �������ڹ���ָ�����ļ�ϵͳ��ʹ������ں����ļ����������/д�ļ�������Ŀ¼�ȣ���
     * &fs    - ָ�� FATFS �ṹ��ָ�롣FATFS �ṹ�� FatFs �������ڴ洢�ļ�ϵͳ����Ϣ��
     * ""     - Ҫ���ص����������ƻ�·����"" ��ʾĬ������������ FatFs �����Թ��ص�һ����������������Ҫ�����ض������������ṩ�����ƣ����� "0:"��"1:" �ȡ�
     * 1      - ����ѡ�ֵΪ 1 ��ʾ���ļ�ϵͳ����Ϊ��/дģʽ����ѡ��Ҳ������Ϊ 0����ʾֻ��ģʽ��
     */
    // ��ж���ѹ��ص��ļ�ϵͳ
    f_mount(NULL, "", 0);
    
    res = f_mount(&fs, "", 1); /* ����ѡ�ֵΪ 1 ��ʾ���ļ�ϵͳ����Ϊ��/дģʽ����ѡ��Ҳ������Ϊ 0����ʾֻ��ģʽ�� */
    if (res == FR_OK)
    {
        printf("FatFs ���سɹ���\r\n");
        return 0;
    }
    else if (res == FR_NO_FILESYSTEM)
    {
        printf("δ�ҵ��ļ�ϵͳ�����ڸ�ʽ��...\r\n");

        /** f_mkfs ������ʽ��ָ���Ĵ洢�豸��������Ϊ 0: ���������������ϴ����µ��ļ�ϵͳ��FAT32�������ô˺���������Ŀ���豸�ϵ��������ݣ���Ϊ������ȫ���¸�ʽ���豸�������µ��ļ�ϵͳ�ṹ��
         * 0:           - Ŀ�����������ơ�"0:" ��ʾ��һ����������FatFs ����ͨ������������ָ��Ŀ���豸��ͨ��Ϊ 0:��1: �ȡ�
         * FS_FAT32     - �ļ�ϵͳ���͡�FS_FAT32 ��ʾ���� FAT32 �ļ�ϵͳ�����ݴ洢�����֧�ֵĸ�ʽ��Ҳ��ѡ�� FS_FAT12��FS_FAT16 �� FS_EXFAT��
         * 0            - �ļ�ϵͳѡ�ͨ����Ϊ 0 ��ʾĬ�����ã�����������Ҫ��
         * work         - �����������������������������ļ�ϵͳ���������д洢��ʱ���ݡ�FatFs �ڸ�ʽ���ڼ���Ҫ�㹻����ڴ����Ϊ��������
         * sizeof(work) - ��������С���� work ����Ĵ�С������֪ f_mkfs �������õĹ����ռ��С��FatFs ʹ�ô��ڴ���ɸ�ʽ���ڼ�ĸ��ֲ�����
         */
        res = f_mkfs("0:", FS_FAT32, 0, work, sizeof(work));
        if (res == FR_OK)
        {
            printf("��ʽ���ɹ����������¹���...\r\n");
            res = f_mount(&fs, "", 1);
            if (res == FR_OK)
            {
                printf("��ʽ���� FatFs ���سɹ���\r\n");
                return 0;
            }
            else
            {
                printf("��ʽ�������ʧ�ܡ�����%d\r\n", res);
                return -1;
            }
        }
        else
        {
            printf("��ʽ��ʧ�ܡ�����%d\r\n", res);
            return -1;
        }
    }
    else
    {
        printf("FatFs ����ʧ�ܣ�����%d\r\n", res);
        return -1;
    }
}

/**
 * @brief ���������������ļ�
 * @return �ļ����ָ�룬ʧ�ܷ���NULL
 */
FIL* FATFS_CreateRawDataFile(void)
{
    static FIL file;
    FRESULT res;
    
    // ʹ�õ�ǰʱ�������ļ��� (���⸲�Ǿ��ļ�)
    RTC_TimeTypeDef sTime;
    RTC_DateTypeDef sDate;
    char filename[32];
    
    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
    
    snprintf(filename, sizeof(filename), "data_%02d%02d%02d.bin", 
             sTime.Hours, sTime.Minutes, sTime.Seconds);

    // �����������ļ�
    res = f_open(&file, filename, FA_CREATE_ALWAYS | FA_WRITE);
    if (res != FR_OK) 
    {
        printf("���������ļ�ʧ��: %d\r\n", res);
        return NULL;
    }
    
    printf("�Ѵ��������������ļ�: %s\r\n", filename);
    return &file;
}
int FATFS_RW_Test(void)
{
    FIL file;
    FRESULT res;
    UINT bw, br;
    char write_data[]  = "Hello, this is EMMC DMA FatFs test.";
    char read_data[64] = { 0 };

    // 1. �����ļ���д������
    res = f_open(&file, "test.csv", FA_CREATE_ALWAYS | FA_WRITE);
    if (res != FR_OK)
    {
        printf("f_open (д��) ʧ�ܣ�%d\r\n", res);
        return -1;
    }

    res = f_write(&file, write_data, strlen(write_data), &bw);
    if (res != FR_OK || bw != strlen(write_data))
    {
        printf("f_write ʧ�ܣ�%d\r\n", res);
        f_close(&file);
        return -1;
    }

    f_close(&file);
    printf("�ļ�д��ɹ���\r\n");

    // 2. ���ļ�����ȡ����
    res = f_open(&file, "test.csv", FA_READ);
    if (res != FR_OK)
    {
        printf("f_open (��ȡ) ʧ�ܣ�%d\r\n", res);
        return -1;
    }

    res = f_read(&file, read_data, sizeof(read_data) - 1, &br);
    if (res != FR_OK)
    {
        printf("f_read ʧ�ܣ�%d\r\n", res);
        f_close(&file);
        return -1;
    }

    f_close(&file);
    printf("���ļ���ȡ��%s\r\n", read_data);

    // 3. ��֤����
    if (strcmp(write_data, read_data) == 0)
    {
        printf("FatFs EMMC ��/д����ͨ����\r\n");
        return 0;
    }
    else
    {
        printf("FatFs EMMC ��/д����ʧ�ܣ�\r\n");
        return -1;
    }
}

// FATFS_RW_Test1/2/3 �����Ʒ���
// [...] (Ϊ������ʡ�ԣ�����ģʽ��ͬ)

/**
 * @brief FATFS ��������д����ԣ��޶�ȡ��֤��
 * @return 0 �ɹ�, -1 ʧ��
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
        // ���ڴ���Ӹ�����...
    };
    int line_count = sizeof(write_data) / sizeof(write_data[0]);

    // 1. �����ļ�������ģʽ��
    res = f_open(&file, "multiline_test.csv", FA_CREATE_ALWAYS | FA_WRITE);
    if (res != FR_OK) {
        printf("[����] �����ļ�ʧ�ܣ�%d\r\n", res);
        return -1;
    }

    // 2. ����д������
    for (int i = 0; i < line_count; i++) {
        // д��������
        res = f_write(&file, write_data[i], strlen(write_data[i]), &bw);
        if (res != FR_OK || bw != strlen(write_data[i])) {
            printf("[����] д��� %d ��ʧ�ܣ�%d\r\n", i + 1, res);
            f_close(&file);
            return -1;
        }

        // д�뻻�з���Windows��\r\n��Linux��\n��
        res = f_write(&file, "\n", 1, &bw);
        if (res != FR_OK || bw != 1) {
            printf("[����] �ڵ� %d ��д�뻻�з�ʧ�ܣ�%d\r\n", i + 1, res);
            f_close(&file);
            return -1;
        }
    }

    // 3. ˢ�²��ر��ļ�
    res = f_close(&file);
    if (res != FR_OK) {
        printf("[����] �ر��ļ�ʧ�ܣ�%d\r\n", res);
        return -1;
    }

    printf("[�ɹ�] ���ļ�д�� %d �С�\r\n", line_count);
    return 0;
}


/**
  * @brief  ɾ������������ļ�
  * @retval FRESULT �������
  */
FRESULT DeleteOldestFile(void) 
{
    FRESULT res;
    DIR dir;
    FILINFO fno;
    FILINFO oldest_fno = {0};
    char path[64];
    char oldest_path[128] = {0};
		
		 // ��װ��ǰ���Ŀ¼·��
    snprintf(path, sizeof(path), "0:/Board%u", g_config.board_id);
    
    res = f_opendir(&dir, "0:/");
    if (res != FR_OK) return res;
    
    // ������ɵ��ļ�
    while (1) 
    {
        res = f_readdir(&dir, &fno);
        if (res != FR_OK || fno.fname[0] == 0) break;
        
       // ֻ��������������ļ�
        if (strstr(fno.fname, ".hex") || strstr(fno.fname, "DATA_"))
        {
            // ��һ���ļ����ҵ��޸�ʱ�������ļ�
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
    
    // ɾ���ҵ�������ļ�
    if (oldest_path[0] != 0) 
    {
        printf("ɾ���ļ�: %s\r\n", oldest_path);
        return f_unlink(oldest_path);
    }
    
    return FR_NO_FILE;
}


/**
  * @brief  ���洢�ռ䲢ɾ�����ļ�
  * @retval None
  */
// �޸� CheckStorageSpace ����
void CheckStorageSpace(void) 
{
    FATFS* fs;
    DWORD free_clusters, total_clusters;
    FRESULT res;
    uint64_t total_space, free_space;
    
    res = f_getfree("0:", &free_clusters, &fs);
    if (res != FR_OK) 
    {
        printf("��ȡ�洢�ռ�ʧ��: %d\r\n", res);
        return;
    }
		// �����ļ�ϵͳ�ܿռ�
    total_clusters = fs->n_fatent - 2;  // �ܴ���
    uint32_t sectors_per_cluster = fs->csize;  // ÿ��������
    total_space = (uint64_t)total_clusters * sectors_per_cluster * 512;
    free_space = (uint64_t)free_clusters * sectors_per_cluster * 512;
			 // ����ļ�ϵͳ�洢�ռ�
    printf("�ļ�ϵͳ�洢�ռ�: �ܹ� %llu MB, ���� %llu MB\r\n", 
           total_space / (1024 * 1024), free_space / (1024 * 1024));
    
		
		 // ��ȡ eMMC ʵ�ʴ洢��Ϣ
    HAL_MMC_CardInfoTypeDef emmc_info;  // eMMC ��Ϣ�ṹ��
    if(HAL_MMC_GetCardState(&hmmc) == HAL_MMC_CARD_TRANSFER) {
        // ��ȡ eMMC ����Ϣ
        HAL_MMC_GetCardInfo(&hmmc, &emmc_info);
        
        // ��ȡeMMC�ܿռ�
        uint64_t emmc_total_space = (uint64_t)emmc_info.LogBlockNbr * emmc_info.LogBlockSize;
        
        // ����eMMC���ÿռ�
        uint64_t emmc_free_space = (uint64_t)free_clusters * fs->csize * 512;  // ���ÿռ�
        
        // ��� eMMC �洢�ռ���Ϣ
        printf("eMMC �洢�ռ�: �ܹ� %llu MB, ���� %llu MB\r\n", 
               emmc_total_space / (1024 * 1024), emmc_free_space / (1024 * 1024));
    }
		
	

   // ����Ƿ������ֵ
   if (free_space < (uint64_t)g_config.space_threshold_mb * 1024 * 1024) 
    {
        printf("�洢�ռ䲻�㣬ɾ�����ļ�...\r\n");
        while (free_space < (uint64_t)g_config.space_threshold_mb * 1024 * 1024)
        {
            res = DeleteOldestFile();
            if (res != FR_OK) 
            {
                if (res == FR_NO_FILE) printf("û�и����ļ���ɾ��\r\n");
                else printf("ɾ���ļ�ʧ��: %d\r\n", res);
                break;
            }
            
            // ���¼�����ÿռ�
            res = f_getfree("0:", &free_clusters, &fs);
            if (res != FR_OK) break;
            
            free_space = (uint64_t)free_clusters * sectors_per_cluster * 512;
            printf("ɾ������ÿռ�: %llu MB\r\n", free_space / (1024 * 1024));
        }
    }
}

/**
 * @brief �� config.txt ��ȡ���ò����� g_config����λ��MB��
 */
void FATFS_ReadConfigFromUSB(void)
{
    FIL file;
    FRESULT res;
    char line[64];
    UINT br;

    res = f_open(&file, "0:/config.txt", FA_READ);
    if (res != FR_OK) {
        printf("δ�ҵ������ļ� config.txt��ʹ��Ĭ������\r\n");
        return;
    }

    while (f_gets(line, sizeof(line), &file)) {
        char key[32], value[32];
        if (sscanf(line, "%31[^=]=%31s", key, value) == 2) {
            if (strcmp(key, "sample_rate") == 0) {
                g_config.sample_rate = (uint16_t)atoi(value);
                printf("����: ������ = %u SPS\r\n", g_config.sample_rate);
            } else if (strcmp(key, "space_threshold_mb") == 0) {
                g_config.space_threshold_mb = (uint32_t)atoi(value);
                printf("����: �洢�ռ���ֵ = %u MB\r\n", g_config.space_threshold_mb);
            }
        }
    }

    f_close(&file);
}

void Check_CreateConfig(void)
{
    FIL file;
    FRESULT res;

    // 1. ������ֻ��ģʽ�� config.txt
    res = f_open(&file, CONFIG_FILE_PATH, FA_READ);
    if (res == FR_OK)
    {
        // �ļ��Ѵ��ڣ��رռ���
        f_close(&file);
        printf("�����ļ��Ѵ��ڣ�����������\r\n");
        return;
    }

    // 2. �ļ������ڣ�������д��Ĭ������
    printf("�����ļ������ڣ����ڴ���Ĭ������...\r\n");
    res = f_open(&file, CONFIG_FILE_PATH, FA_CREATE_ALWAYS | FA_WRITE);
    if (res != FR_OK)
    {
        printf("���������ļ�ʧ�ܣ�������: %d\r\n", res);
        return;
    }

    // 3. д��Ĭ������
   const char *defaultConfig =
"sample_rate=2000\r\n"
"space_threshold_mb=3072\r\n"
"board_id=4\r\n"
"use_time_from_config=1\r\n"
"date_time=2025-07-09 12:00:00\r\n"
"\r\n"
"# ����˵����\r\n"
"# sample_rate: ֧�ֵĲ�����ֵ����λSPS����125, 250, 500, 1000, 2000\r\n"
"# space_threshold_mb: ���ʣ��ռ䣨MB��������ʱ�Զ�ɾ��������\r\n"
"# board_id: ��ǰ���ţ������ļ��� BoardX ���ƣ�\r\n"
"# use_time_from_config: 0=��ʹ������ʱ�䣻1=ʹ�� date_time ���� RTC\r\n"
"# date_time: ����ʱ�䣬��ʽΪ yyyy-mm-dd hh:mm:ss������ use_time_from_config=1 ʱ��Ч\r\n";

    UINT bytesWritten = 0;
    res = f_write(&file, defaultConfig, strlen(defaultConfig), &bytesWritten);
    if (res == FR_OK && bytesWritten == strlen(defaultConfig))
    {
        printf("Ĭ�������ļ�д��ɹ���\r\n");
    }
    else
    {
        printf("д�������ļ�ʧ�ܣ�������: %d\r\n", res);
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
        printf("��ȡ�����ļ�ʧ�ܣ�������: %d\r\n", res);
        return;
    }
		memset(line, 0, sizeof(line));
		memset(g_config.config_time_string, 0, sizeof(g_config.config_time_string));

		while (f_gets(line, sizeof(line), &file))
    {
        // �����ĩ���з�
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

    printf("���ö�ȡ���: sample_rate=%hu, space_threshold_mb=%u MB, board_id=%u, use_time_from_config=%u, config_time_string=%s\r\n",
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

        // ����RTCʱ��
        if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK ||
            HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
        {
            printf("RTC ����ʧ��\r\n");
        }
        else
        {
            printf("RTC �Ѹ��������ļ�����Ϊ: %s\r\n", g_config.config_time_string);
            
            // �������ñ�־��ͬ�����ļ�
            UpdateConfigUseTimeFlag(0);
        }
    }
    else
    {
        printf("ʱ���ַ�������ʧ��: %s\r\n", g_config.config_time_string);
    }
}

void UpdateConfigUseTimeFlag(uint8_t newValue)
{
    FIL file;
    FRESULT res;
    char configContent[512] = {0};  // �洢���������ļ�����
    char tempPath[32] = "0:/config_temp.txt";  // ��ʱ�ļ���
    char line[128];
    UINT br, bw;
    bool updated = false;

    // 1. ��ԭʼ�����ļ�
    res = f_open(&file, CONFIG_FILE_PATH, FA_READ);
    if (res != FR_OK) 
    {
        printf("�޷��������ļ����и��£�������: %d\r\n", res);
        return;
    }

    // 2. ��ȡ���������ļ�
    while (f_gets(line, sizeof(line), &file)) 
    {
        // ����Ƿ�����Ҫ���µ���
        if (strncmp(line, "use_time_from_config=", strlen("use_time_from_config=")) == 0) 
        {
            // �滻Ϊ�µ�����ֵ
            snprintf(line, sizeof(line), "use_time_from_config=%u\r\n", newValue);
            updated = true;
        }
        
        // ��ӵ��������ݻ�����
        strncat(configContent, line, sizeof(configContent) - strlen(configContent) - 1);
    }
    f_close(&file);

    // 3. ���û���ҵ�������������
    if (!updated) 
    {
        char newLine[32];
        snprintf(newLine, sizeof(newLine), "use_time_from_config=%u\r\n", newValue);
        strncat(configContent, newLine, sizeof(configContent) - strlen(configContent) - 1);
    }

    // 4. ������ʱ�ļ���д��������
    res = f_open(&file, tempPath, FA_CREATE_ALWAYS | FA_WRITE);
    if (res != FR_OK) 
    {
        printf("�޷�������ʱ�����ļ���������: %d\r\n", res);
        return;
    }
    
    res = f_write(&file, configContent, strlen(configContent), &bw);
    if (res != FR_OK || bw != strlen(configContent)) 
    {
        printf("д����ʱ�����ļ�ʧ��: %d\r\n", res);
        f_close(&file);
        f_unlink(tempPath);  // ɾ����ʱ�ļ�
        return;
    }
    f_close(&file);
    
    // 5. ����ʱ�ļ��滻ԭʼ�����ļ�
    f_unlink(CONFIG_FILE_PATH);  // ɾ�����ļ�
    if (f_rename(tempPath, CONFIG_FILE_PATH) != FR_OK) 
    {
        printf("��������ʱ�ļ�ʧ�ܣ����ֶ��ָ�����\r\n");
        f_unlink(tempPath);  // ������ʱ�ļ�
        return;
    }
    
    // 6. �����ڴ��е�����ֵ
    g_config.use_time_from_config = newValue;
    printf("�����ļ���־λ�Ѹ���: use_time_from_config=%d\r\n", newValue);
}

/**
  * @brief  ��ʽ�� eMMC
  * @retval 0 ����ɹ�������ֵ��ʾʧ��
  */
int Format_EMMC(void)
{
    FRESULT res;
    BYTE work[_MAX_SS];  // ������
    // ��ʽ�� eMMC��0: �̷���FS_FAT32 ���ͣ�Ĭ�ϴش�С����������
    res = f_mkfs("0:", FS_FAT32, 0, work, sizeof(work));
    if (res != FR_OK)
    {
        printf("��ʽ��ʧ�ܣ�������: %d\r\n", res);
        return -1;  // ���ش�����
    }

    printf("��ʽ����ɣ��������¹���...\r\n");

    // ��ʽ���ɹ������¹���
    res = f_mount(&fs, "0:", 1);  // �����ļ�ϵͳ
    if (res != FR_OK)
    {
        printf("���¹���ʧ�ܣ�������: %d\r\n", res);
        return -2;  // ����ʧ�ܷ��ش�����
    }

    printf("eMMC ��ʽ�������سɹ�\r\n");
    return 0;  // �ɹ�
}




/** ���
emmc ��ʾ

EMMC_Getinfo_Test
EMMC_Info: 
CardType: 1
Class: 245
RelCardAdd: 2
BlockNbr: 61079552
BlockSize: 512
LogBlockNbr: 61079552
LogoBlockSize: 512
emmc д���ȡ���Գɹ�
δ�ҵ��ļ�ϵͳ�����ڸ�ʽ��...
��ʽ���ɹ����������¹���...
��ʽ���� FatFs ���سɹ���
�ļ�д��ɹ���
���ļ���ȡ��Hello, this is EMMC DMA FatFs test.
FatFs EMMC ��/д����ͨ����
*/

/* USER CODE END Application */