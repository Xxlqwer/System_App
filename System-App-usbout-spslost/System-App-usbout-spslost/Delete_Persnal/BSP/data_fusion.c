#include "ads1285.h"
#include "rtc.h"
#include "stdio.h"
#include "string.h"
#include "data_fusion.h"
#include "fatfs.h"
#include "stm32f4xx_hal.h"
#include "um980.h"
#include "usb_device.h"
#include "sdio.h"
// 启动计时
uint32_t startupTick = 0;
uint32_t lastSyncTime = 0;
uint32_t syncIntervalMs = 60*60*1000;  // 每小时 ms
static bool fileCreated = false;
// 添加全局变量跟踪当前文件的时间
// uint8_t currentFileHour = 0xFF;   // 初始化为无效值
// uint8_t currentFileDay = 0xFF;    // 初始化为无效值


// 调试计数器
uint32_t bufferFullCount = 0;
uint32_t writeCompleteCount = 0;
uint32_t bufferOverrunCount = 0;

volatile BufferSystem g_bufferSystem = {0};
volatile bool needTimeSync = false;
FIL rawDataFile;  // 二进制数据文件句柄

AppState appState = STATE_IDLE;
SystemState systemState = SYSTEM_NORMAL_OPERATION;


extern uint8_t gnssBuffer[GNSS_BUFFER_SIZE];
extern USBD_HandleTypeDef hUsbDeviceHS;  //检测USB状态

void MainLoop_StateMachine(void)
{
    uint32_t currentTick = HAL_GetTick();
	static uint32_t usbDisconnectTick = 0; /* 静态变量只在本函数内维护 USB断开恢复流程 */

    /* 顶层状态判断 */
    if (hUsbDeviceHS.dev_state == USBD_STATE_CONFIGURED)
    {
        if (systemState != SYSTEM_USB_CONNECTED)
        {
            printf("USB连接，暂停采集\r\n");
            systemState = SYSTEM_USB_CONNECTED;
					
			// 关闭 打开的文件（如果有）
            if (fileCreated)
            {
                f_close(&rawDataFile);
                fileCreated = false;
                printf("数据文件已关闭（USB插入）\r\n");
            }
        }

        return;  // USB模式下不做子状态处理
    }
    else
    {
		// 2. USB未连接状态（插拔后或未接）
		if (usbDisconnectTick == 0)
		{
			usbDisconnectTick = HAL_GetTick();  // 第一次检测断开
		}
		
		if (HAL_GetTick() - usbDisconnectTick > 1000)  // 延时1000ms确认断开
		{
			if (systemState != SYSTEM_NORMAL_OPERATION)
            {
				printf("USB已断开，恢复采集\r\n");
					
				// step1: 卸载/重置存储接口，防止FR_LOCKED
				f_mount(NULL, "", 1);
				__HAL_RCC_SDIO_FORCE_RESET();
				__HAL_RCC_SDIO_RELEASE_RESET();
				__HAL_RCC_SDIO_CLK_DISABLE();
				HAL_Delay(50);  // 稍作延时
					
				// step2: 重初始化SDIO和FatFs
                if (SDIO_Reinitialize() != HAL_OK) 
                {
                    printf("SDIO重初始化失败，尝试软复位系统...\r\n");
                    HAL_Delay(1000);
                    NVIC_SystemReset(); // 如果SDIO无法恢复，执行系统复位
                    return;
                }

                // 挂载文件系统
                printf("SDIO初始化成功，准备挂载文件系统...\r\n");
                if (FATFS_Mount() != 0)
                {
                    printf("文件系统挂载失败，尝试格式化...\r\n");
                    Format_EMMC();  // 尝试格式化eMMC
                    if (FATFS_Mount() != 0) 
                    {
                        printf("格式化后挂载仍失败，执行系统复位\r\n");
                        HAL_Delay(1000);
                        NVIC_SystemReset();
                        return;
                    }
                }
        
                printf("文件系统挂载成功\r\n");

				// step3: 加载配置（可选）
				printf("[DEBUG] 准备读取 config.txt...\r\n");
				LoadConfigFromFile();
				// SetRTCFromConfigIfNeeded();

				// step4: ADC 恢复配置
				if (ADS1285_WriteReg_Continuous(&hads1285) != HAL_OK)
				{
					printf("ADC配置恢复失败!\r\n");
				}

				// step5: 恢复中断与状态
				HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
				appState = STATE_ACQUIRE;
				systemState = SYSTEM_NORMAL_OPERATION;
				usbDisconnectTick = 0;

				printf("[INFO] 采集系统已恢复运行\r\n");
		
			}					
		}
    }
    /* 子状态判断（只有在 SYSTEM_NORMAL_OPERATION 下才处理） */
    ProcessSubStateMachine(currentTick);
}

void ProcessSubStateMachine(uint32_t currentTick)
{
    static uint32_t lastSpaceCheck = 0;
    static uint32_t lastSampleTime = 0;
    static uint32_t lastSampleCount = 0;
    static uint32_t lastReportTick = 0;

    switch (appState)
    {
        case STATE_IDLE:
        {
            if (startupTick == 0) 
            {
                startupTick = currentTick;
            };

            if (currentTick - startupTick >= STARTUP_TIMEOUT_MS) 
            {
                printf("GNSS超时未响应，开始采集\r\n");
				
                // 使用 RTC 时间初始化当前文件时间
				RTC_TimeTypeDef sTime;
				RTC_DateTypeDef sDate;
				HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
				HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
				currentFileHour = sTime.Hours;
				currentFileDay = sDate.Date;
							
                appState = STATE_ACQUIRE;
            }
            break;
        }
//--------------------------------------------------
        case STATE_GNSS_RECEIVED:
		{
			printf("GNSS: %s", gnssBuffer);
			extract_time(gnssBuffer);
			HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, GPIO_PIN_SET); // 授时完成指示

			memset(gnssBuffer, 0, sizeof(gnssBuffer));
			// 设置当前文件时间
			RTC_TimeTypeDef sTime;// 获取当前时间
			RTC_DateTypeDef sDate;// 获取当前日期
			HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);// 获取当前时间
			HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);// 获取当前日期
			currentFileHour = sTime.Hours;// 更新当前文件时间
			currentFileDay = sDate.Date;// 更新当前文件时间
			HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);
			HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
			appState = STATE_ACQUIRE;  // 进入采集模式

			// 避免在 USB 连接时写 FatFs
		    if (hUsbDeviceHS.dev_state == USBD_STATE_CONFIGURED) 
            {
				printf("当前为USB MSC模式，跳过GNSS数据文件创建。\r\n");					
			}
            else
            {}
			break;
		}
//----------------------------------------------------------
        case STATE_TIMESYNC:
        {
            if (needTimeSync && systemState == SYSTEM_NORMAL_OPERATION) 
            {
                //HAL_NVIC_DisableIRQ(EXTI15_10_IRQn);

                if (PerformTimeSync()) 
                {
                    //printf("授时成功，下次授时在%d小时后\r\n", SYNC_INTERVAL_HOURS);
//                    CreateNewFileWithTimestamp();
                    // 更新文件时间
					RTC_TimeTypeDef sTime;
					RTC_DateTypeDef sDate;
					HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
					HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
					currentFileHour = sTime.Hours;
					currentFileDay = sDate.Date;
					memset(gnssBuffer, 0, sizeof(gnssBuffer));  // 清空残余数据
										
                } 
                else 
                {
                    printf("授时失败，将在10分钟后重试\r\n");
                    lastSyncTime = currentTick - (syncIntervalMs - 10 * 60 * 1000);
                }

                needTimeSync = false;
				// 重新启用DRDY中断
                HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
                appState = STATE_ACQUIRE;
            }
            break;
        }
//--------------------------------------------------
        case STATE_ACQUIRE:
		{
	
			/* -------- 1. 每分钟状态日志：始终先打 -------- */
            if (currentTick - lastReportTick >= 60000) 
            {
                lastReportTick = currentTick;
                printf("[STATUS] Samples: %u, Lost: %u, Buffers: F:%u W:%u O:%u\n",
                       g_bufferSystem.totalSamples,
                       g_bufferSystem.lostSamples,
                       bufferFullCount,
                       writeCompleteCount,
                       bufferOverrunCount);
                bufferFullCount = 0;
                writeCompleteCount = 0;
                bufferOverrunCount = 0;
            }
			/* -------- 2. 定时 GNSS 授时 -------- */
            if (currentTick - lastSyncTime >= syncIntervalMs) 
            {
                needTimeSync = true;
                appState = STATE_TIMESYNC; //进入授时
							 
                break;
            }
				
			/* -------- 3. 12 h 存储空间检查 -------- */ //uint32_t 是无符号整型  不会因溢出而报错
            if (currentTick  - lastSpaceCheck > 12 * 60 * 60 * 1000) 
            {
              CheckStorageSpace();
              lastSpaceCheck = currentTick ;
            }

			/* -------- 4. 文件轮换检测 -------- */
            CheckAndCreateNewFileIfNeeded();

			/* -------- 5. 若文件尚不存在则尝试创建 -------- */
            if (!fileCreated) 
            {
				FIL* filePtr = FATFS_CreateRawDataFileWithTimestamp();
				if(filePtr != NULL) 
				{
						rawDataFile = *filePtr;
						fileCreated = true;
				}
                else
                {
					/* 创建失败也不要 Error_Handler —— 留给下一轮重试 */
					printf("创建数据文件失败，等待下一轮重试\r\n");
					break;         
                }

				/* -------- 6. 写缓冲区 -------- */
                ProcessBuffers();

//            if (currentTick - lastSampleTime >= 1000) 
//            {
//                uint32_t samplesPerSecond = g_bufferSystem.totalSamples - lastSampleCount;
//                lastSampleCount = g_bufferSystem.totalSamples;
//                lastSampleTime = currentTick;

//                printf("实时采样率: %lu SPS" ,samplesPerSecond);
//                PrintBufferStatus();
//            }
            
           
            }
						
			break;
		}
//--------------------------------------------------
        default:
            break;
    }
}

void Soft_Reset(void) 
{
    HAL_NVIC_SystemReset();
}


void BufferSystem_Init(void)
{
  memset((void*)&g_bufferSystem, 0, sizeof(g_bufferSystem));
  for (int i = 0; i < BUFFER_COUNT; i++)
  {
    g_bufferSystem.buffers[i].isFull = false;      // 修复这里
    g_bufferSystem.buffers[i].isLocked = false;    // 修复这里
    g_bufferSystem.buffers[i].sampleCount = 0;
  }
    
  g_bufferSystem.activeIndex = 0;
  g_bufferSystem.writeIndex = 0;
}


void CheckAndCreateNewFileIfNeeded(void)
{
    RTC_TimeTypeDef sTime;
    RTC_DateTypeDef sDate;
    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
    
    // 检查是否需要创建新文件
    if ((currentFileHour != sTime.Hours) || (currentFileDay != sDate.Date))
    {
		printf("时间变化，准备轮换文件\r\n");
		/* ① 先安全关闭当前文件；失败则本轮放弃轮换，等待下一分钟再试 */
		if (!SafeCloseCurrentFile()) 
        {
			printf("关闭文件失败，本轮不创建新文件，避免 FR_LOCKED\r\n");
			return;
		}
			
		/* ② 尝试创建新文件 */
        FIL *filePtr = FATFS_CreateRawDataFileWithTimestamp();
        if (filePtr == NULL) 
        {
            printf("新建文件失败，本轮写入停用\r\n");
            return;             /* 不再调用 Error_Handler，避免死循环 */
        }

        rawDataFile   = *filePtr;
        fileCreated   = true;
        currentFileHour = sTime.Hours;
        currentFileDay  = sDate.Date;
	
		
		}
}
bool SafeCloseCurrentFile(void)
{
    if (!fileCreated)
        return true;

    FRESULT res;

    res = f_sync(&rawDataFile);
    if (res != FR_OK) 
    {
        printf("f_sync 失败: %d\r\n", res);
        return false;
    }

    res = f_close(&rawDataFile);
    if (res != FR_OK) 
    {
        printf("f_close 失败: %d\r\n", res);  // ?? 你这里没判断！
        return false;
    }

    fileCreated = false;
    return true;
}

void SetRTCFromConfigIfNeeded(void) 
{
    if (g_config.use_time_from_config == 1 && strlen(g_config.config_time_string) >= 19) 
    {
        RTC_TimeTypeDef sTime = {0};
        RTC_DateTypeDef sDate = {0};

        int year, month, day, hour, min, sec;
        if (sscanf(g_config.config_time_string, "%d-%d-%d %d:%d:%d", 
                   &year, &month, &day, &hour, &min, &sec) == 6) 
        {

            sDate.Year = year - 2000;
            sDate.Month = month;
            sDate.Date = day;
            sDate.WeekDay = 1;  // 可忽略或手动设置

            sTime.Hours = hour;
            sTime.Minutes = min;
            sTime.Seconds = sec;

            HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
            HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);

            printf("已根据配置文件设置RTC: %s\r\n", g_config.config_time_string);
										 
						// 修改标志
            g_config.use_time_from_config = 0;

            // 同步写回 config.txt
            UpdateConfigUseTimeFlag(0);
										 
        } 
        else 
        {
            printf("配置文件中的 date_time 格式错误: %s\r\n", g_config.config_time_string);
        }
    }
}


void PrintBufferStatus(void) 
{
    uint32_t free_count = 0, writing_count = 0, full_count = 0;
    
    for (int i = 0; i < BUFFER_COUNT; i++) 
    {
        if (!g_bufferSystem.buffers[i].isFull && 
            !g_bufferSystem.buffers[i].isLocked) 
        {
            free_count++;
        }
        else if (!g_bufferSystem.buffers[i].isFull && 
                 g_bufferSystem.buffers[i].isLocked) 
        {
            writing_count++;
        }
        else if (g_bufferSystem.buffers[i].isFull) 
        {
            full_count++;
        }
    }
    
    printf("缓冲区状态: 空闲:%u 写入:%u 已满:%u\r\n", 
           free_count, writing_count, full_count);
}

/* 4. 添加采样率验证功能到ProcessBuffers函数 */
void ProcessBuffers(void) 
{
	
	  // 若当前没有有效文件，就不处理任何缓冲区
    if (!fileCreated)
    {
        // 可以打印一次提示
        printf("[WARN] 无数据文件，缓冲区跳过写入\n");
        return;
    }
    for (int i = 0; i < BUFFER_COUNT; i++) 
    {
        // 使用循环索引而不是固定索引
        uint8_t bufferIndex = (g_bufferSystem.writeIndex + i) % BUFFER_COUNT;
        volatile DataBuffer* buffer = &g_bufferSystem.buffers[bufferIndex];
        
        if (buffer->isFull && !buffer->isLocked) 
        {
            buffer->isLocked = true; // 加锁防止并发访问
            
            // 记录开始时间（用于性能分析）
            uint32_t startTime = HAL_GetTick();
            
            // 写入SD卡
            FlushBufferToSD(bufferIndex);
            
            // 更新性能计数器
            writeCompleteCount++;
            
            // 打印写入耗时
//            printf("写入 %u 样本耗时 %lu ms\n", 
//                   buffer->sampleCount, 
//                   HAL_GetTick() - startTime);
            
            // 重置缓冲区状态
            buffer->isFull = false;
            buffer->isLocked = false;
            buffer->sampleCount = 0;
            
            // 更新写入索引
            g_bufferSystem.writeIndex = (bufferIndex + 1) % BUFFER_COUNT;
            
            break; // 每次只处理一个缓冲区
        }
    }
}

void FlushBufferToSD(uint8_t bufferIndex)
{
    volatile DataBuffer* buffer = &g_bufferSystem.buffers[bufferIndex];
    
    // 直接写入原始二进制数据
    UINT bytesWritten;
    uint32_t bytesToWrite = buffer->sampleCount * sizeof(int32_t);
    
    // 写入数据块
    FRESULT res = f_write(&rawDataFile, 
                         (const void*)buffer->data, 
                         bytesToWrite, 
                         &bytesWritten);
    
    if (res != FR_OK || bytesWritten != bytesToWrite) 
    {
        printf("写入错误: 预期 %u 字节, 实际 %u 字节, 错误 %d\n", 
               bytesToWrite, bytesWritten, res);
    }
    
    // 仅每10次写入同步一次（大幅提高性能）
    static uint8_t writeCount = 0;
    if (++writeCount >= 10) 
    {
        f_sync(&rawDataFile);
        writeCount = 0;
    }
}


void ADS1285_Test(void) 
{
    int32_t raw_data = 0;
    
    // 1. 读取ADC数据
    if (ADS1285_ReadRawData(&hads1285, &raw_data) != HAL_OK) 
    {
        return;
    }
    
    // 2. 获取当前活动缓冲区
    volatile DataBuffer* activeBuffer = &g_bufferSystem.buffers[g_bufferSystem.activeIndex];
    
    // 3. 检查缓冲区是否已满
    if (activeBuffer->sampleCount >= SAMPLES_PER_BUFFER) 
    {
        // 4. 标记当前缓冲区已满
        activeBuffer->isFull = true;
        bufferFullCount++;
        
        // 5. 查找下一个空闲缓冲区
        uint8_t nextIndex = (g_bufferSystem.activeIndex + 1) % BUFFER_COUNT;
        uint8_t startIndex = nextIndex;
        uint8_t found = 0;
        
        do 
        {
            // 检查缓冲区是否可用（未满且未锁定）
            if (!g_bufferSystem.buffers[nextIndex].isFull && 
                !g_bufferSystem.buffers[nextIndex].isLocked) 
            {
                found = 1;
                break;
            }
            nextIndex = (nextIndex + 1) % BUFFER_COUNT;
        } while (nextIndex != startIndex);
        
        if (!found) 
        {
            // 所有缓冲区都忙，丢弃样本并计数
            g_bufferSystem.lostSamples++;
            bufferOverrunCount++;
            return;
        }
        
        // 6. 切换到新缓冲区
        g_bufferSystem.activeIndex = nextIndex;
        activeBuffer = &g_bufferSystem.buffers[nextIndex];
        activeBuffer->sampleCount = 0;  // 重置计数器
    }
    
    // 7. 存储数据
    activeBuffer->data[activeBuffer->sampleCount] = raw_data;
    activeBuffer->sampleCount++;
    g_bufferSystem.totalSamples++;
}


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    // 检查是否是DRDY引脚的中断
    if (GPIO_Pin == hads1285.drdy_pin)
    {
        // 只在case 2状态下采集数据
        if (appState  == STATE_ACQUIRE)
        {
            ADS1285_Test(); // 采集数据到三重缓冲
        }
    }
}