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
// ������ʱ
uint32_t startupTick = 0;
uint32_t lastSyncTime = 0;
uint32_t syncIntervalMs = 60*60*1000;  // ÿСʱ ms
static bool fileCreated = false;
// ���ȫ�ֱ������ٵ�ǰ�ļ���ʱ��
// uint8_t currentFileHour = 0xFF;   // ��ʼ��Ϊ��Чֵ
// uint8_t currentFileDay = 0xFF;    // ��ʼ��Ϊ��Чֵ


// ���Լ�����
uint32_t bufferFullCount = 0;
uint32_t writeCompleteCount = 0;
uint32_t bufferOverrunCount = 0;

volatile BufferSystem g_bufferSystem = {0};
volatile bool needTimeSync = false;
FIL rawDataFile;  // �����������ļ����

AppState appState = STATE_IDLE;
SystemState systemState = SYSTEM_NORMAL_OPERATION;


extern uint8_t gnssBuffer[GNSS_BUFFER_SIZE];
extern USBD_HandleTypeDef hUsbDeviceHS;  //���USB״̬

void MainLoop_StateMachine(void)
{
    uint32_t currentTick = HAL_GetTick();
	static uint32_t usbDisconnectTick = 0; /* ��̬����ֻ�ڱ�������ά�� USB�Ͽ��ָ����� */

    /* ����״̬�ж� */
    if (hUsbDeviceHS.dev_state == USBD_STATE_CONFIGURED)
    {
        if (systemState != SYSTEM_USB_CONNECTED)
        {
            printf("USB���ӣ���ͣ�ɼ�\r\n");
            systemState = SYSTEM_USB_CONNECTED;
					
			// �ر� �򿪵��ļ�������У�
            if (fileCreated)
            {
                f_close(&rawDataFile);
                fileCreated = false;
                printf("�����ļ��ѹرգ�USB���룩\r\n");
            }
        }

        return;  // USBģʽ�²�����״̬����
    }
    else
    {
		// 2. USBδ����״̬����κ��δ�ӣ�
		if (usbDisconnectTick == 0)
		{
			usbDisconnectTick = HAL_GetTick();  // ��һ�μ��Ͽ�
		}
		
		if (HAL_GetTick() - usbDisconnectTick > 1000)  // ��ʱ1000msȷ�϶Ͽ�
		{
			if (systemState != SYSTEM_NORMAL_OPERATION)
            {
				printf("USB�ѶϿ����ָ��ɼ�\r\n");
					
				// step1: ж��/���ô洢�ӿڣ���ֹFR_LOCKED
				f_mount(NULL, "", 1);
				__HAL_RCC_SDIO_FORCE_RESET();
				__HAL_RCC_SDIO_RELEASE_RESET();
				__HAL_RCC_SDIO_CLK_DISABLE();
				HAL_Delay(50);  // ������ʱ
					
				// step2: �س�ʼ��SDIO��FatFs
                if (SDIO_Reinitialize() != HAL_OK) 
                {
                    printf("SDIO�س�ʼ��ʧ�ܣ�������λϵͳ...\r\n");
                    HAL_Delay(1000);
                    NVIC_SystemReset(); // ���SDIO�޷��ָ���ִ��ϵͳ��λ
                    return;
                }

                // �����ļ�ϵͳ
                printf("SDIO��ʼ���ɹ���׼�������ļ�ϵͳ...\r\n");
                if (FATFS_Mount() != 0)
                {
                    printf("�ļ�ϵͳ����ʧ�ܣ����Ը�ʽ��...\r\n");
                    Format_EMMC();  // ���Ը�ʽ��eMMC
                    if (FATFS_Mount() != 0) 
                    {
                        printf("��ʽ���������ʧ�ܣ�ִ��ϵͳ��λ\r\n");
                        HAL_Delay(1000);
                        NVIC_SystemReset();
                        return;
                    }
                }
        
                printf("�ļ�ϵͳ���سɹ�\r\n");

				// step3: �������ã���ѡ��
				printf("[DEBUG] ׼����ȡ config.txt...\r\n");
				LoadConfigFromFile();
				// SetRTCFromConfigIfNeeded();

				// step4: ADC �ָ�����
				if (ADS1285_WriteReg_Continuous(&hads1285) != HAL_OK)
				{
					printf("ADC���ûָ�ʧ��!\r\n");
				}

				// step5: �ָ��ж���״̬
				HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
				appState = STATE_ACQUIRE;
				systemState = SYSTEM_NORMAL_OPERATION;
				usbDisconnectTick = 0;

				printf("[INFO] �ɼ�ϵͳ�ѻָ�����\r\n");
		
			}					
		}
    }
    /* ��״̬�жϣ�ֻ���� SYSTEM_NORMAL_OPERATION �²Ŵ��� */
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
                printf("GNSS��ʱδ��Ӧ����ʼ�ɼ�\r\n");
				
                // ʹ�� RTC ʱ���ʼ����ǰ�ļ�ʱ��
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
			HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, GPIO_PIN_SET); // ��ʱ���ָʾ

			memset(gnssBuffer, 0, sizeof(gnssBuffer));
			// ���õ�ǰ�ļ�ʱ��
			RTC_TimeTypeDef sTime;// ��ȡ��ǰʱ��
			RTC_DateTypeDef sDate;// ��ȡ��ǰ����
			HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);// ��ȡ��ǰʱ��
			HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);// ��ȡ��ǰ����
			currentFileHour = sTime.Hours;// ���µ�ǰ�ļ�ʱ��
			currentFileDay = sDate.Date;// ���µ�ǰ�ļ�ʱ��
			HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);
			HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
			appState = STATE_ACQUIRE;  // ����ɼ�ģʽ

			// ������ USB ����ʱд FatFs
		    if (hUsbDeviceHS.dev_state == USBD_STATE_CONFIGURED) 
            {
				printf("��ǰΪUSB MSCģʽ������GNSS�����ļ�������\r\n");					
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
                    //printf("��ʱ�ɹ����´���ʱ��%dСʱ��\r\n", SYNC_INTERVAL_HOURS);
//                    CreateNewFileWithTimestamp();
                    // �����ļ�ʱ��
					RTC_TimeTypeDef sTime;
					RTC_DateTypeDef sDate;
					HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
					HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
					currentFileHour = sTime.Hours;
					currentFileDay = sDate.Date;
					memset(gnssBuffer, 0, sizeof(gnssBuffer));  // ��ղ�������
										
                } 
                else 
                {
                    printf("��ʱʧ�ܣ�����10���Ӻ�����\r\n");
                    lastSyncTime = currentTick - (syncIntervalMs - 10 * 60 * 1000);
                }

                needTimeSync = false;
				// ��������DRDY�ж�
                HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
                appState = STATE_ACQUIRE;
            }
            break;
        }
//--------------------------------------------------
        case STATE_ACQUIRE:
		{
	
			/* -------- 1. ÿ����״̬��־��ʼ���ȴ� -------- */
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
			/* -------- 2. ��ʱ GNSS ��ʱ -------- */
            if (currentTick - lastSyncTime >= syncIntervalMs) 
            {
                needTimeSync = true;
                appState = STATE_TIMESYNC; //������ʱ
							 
                break;
            }
				
			/* -------- 3. 12 h �洢�ռ��� -------- */ //uint32_t ���޷�������  ���������������
            if (currentTick  - lastSpaceCheck > 12 * 60 * 60 * 1000) 
            {
              CheckStorageSpace();
              lastSpaceCheck = currentTick ;
            }

			/* -------- 4. �ļ��ֻ���� -------- */
            CheckAndCreateNewFileIfNeeded();

			/* -------- 5. ���ļ��в��������Դ��� -------- */
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
					/* ����ʧ��Ҳ��Ҫ Error_Handler ���� ������һ������ */
					printf("���������ļ�ʧ�ܣ��ȴ���һ������\r\n");
					break;         
                }

				/* -------- 6. д������ -------- */
                ProcessBuffers();

//            if (currentTick - lastSampleTime >= 1000) 
//            {
//                uint32_t samplesPerSecond = g_bufferSystem.totalSamples - lastSampleCount;
//                lastSampleCount = g_bufferSystem.totalSamples;
//                lastSampleTime = currentTick;

//                printf("ʵʱ������: %lu SPS" ,samplesPerSecond);
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
    g_bufferSystem.buffers[i].isFull = false;      // �޸�����
    g_bufferSystem.buffers[i].isLocked = false;    // �޸�����
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
    
    // ����Ƿ���Ҫ�������ļ�
    if ((currentFileHour != sTime.Hours) || (currentFileDay != sDate.Date))
    {
		printf("ʱ��仯��׼���ֻ��ļ�\r\n");
		/* �� �Ȱ�ȫ�رյ�ǰ�ļ���ʧ�����ַ����ֻ����ȴ���һ�������� */
		if (!SafeCloseCurrentFile()) 
        {
			printf("�ر��ļ�ʧ�ܣ����ֲ��������ļ������� FR_LOCKED\r\n");
			return;
		}
			
		/* �� ���Դ������ļ� */
        FIL *filePtr = FATFS_CreateRawDataFileWithTimestamp();
        if (filePtr == NULL) 
        {
            printf("�½��ļ�ʧ�ܣ�����д��ͣ��\r\n");
            return;             /* ���ٵ��� Error_Handler��������ѭ�� */
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
        printf("f_sync ʧ��: %d\r\n", res);
        return false;
    }

    res = f_close(&rawDataFile);
    if (res != FR_OK) 
    {
        printf("f_close ʧ��: %d\r\n", res);  // ?? ������û�жϣ�
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
            sDate.WeekDay = 1;  // �ɺ��Ի��ֶ�����

            sTime.Hours = hour;
            sTime.Minutes = min;
            sTime.Seconds = sec;

            HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
            HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);

            printf("�Ѹ��������ļ�����RTC: %s\r\n", g_config.config_time_string);
										 
						// �޸ı�־
            g_config.use_time_from_config = 0;

            // ͬ��д�� config.txt
            UpdateConfigUseTimeFlag(0);
										 
        } 
        else 
        {
            printf("�����ļ��е� date_time ��ʽ����: %s\r\n", g_config.config_time_string);
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
    
    printf("������״̬: ����:%u д��:%u ����:%u\r\n", 
           free_count, writing_count, full_count);
}

/* 4. ��Ӳ�������֤���ܵ�ProcessBuffers���� */
void ProcessBuffers(void) 
{
	
	  // ����ǰû����Ч�ļ����Ͳ������κλ�����
    if (!fileCreated)
    {
        // ���Դ�ӡһ����ʾ
        printf("[WARN] �������ļ�������������д��\n");
        return;
    }
    for (int i = 0; i < BUFFER_COUNT; i++) 
    {
        // ʹ��ѭ�����������ǹ̶�����
        uint8_t bufferIndex = (g_bufferSystem.writeIndex + i) % BUFFER_COUNT;
        volatile DataBuffer* buffer = &g_bufferSystem.buffers[bufferIndex];
        
        if (buffer->isFull && !buffer->isLocked) 
        {
            buffer->isLocked = true; // ������ֹ��������
            
            // ��¼��ʼʱ�䣨�������ܷ�����
            uint32_t startTime = HAL_GetTick();
            
            // д��SD��
            FlushBufferToSD(bufferIndex);
            
            // �������ܼ�����
            writeCompleteCount++;
            
            // ��ӡд���ʱ
//            printf("д�� %u ������ʱ %lu ms\n", 
//                   buffer->sampleCount, 
//                   HAL_GetTick() - startTime);
            
            // ���û�����״̬
            buffer->isFull = false;
            buffer->isLocked = false;
            buffer->sampleCount = 0;
            
            // ����д������
            g_bufferSystem.writeIndex = (bufferIndex + 1) % BUFFER_COUNT;
            
            break; // ÿ��ֻ����һ��������
        }
    }
}

void FlushBufferToSD(uint8_t bufferIndex)
{
    volatile DataBuffer* buffer = &g_bufferSystem.buffers[bufferIndex];
    
    // ֱ��д��ԭʼ����������
    UINT bytesWritten;
    uint32_t bytesToWrite = buffer->sampleCount * sizeof(int32_t);
    
    // д�����ݿ�
    FRESULT res = f_write(&rawDataFile, 
                         (const void*)buffer->data, 
                         bytesToWrite, 
                         &bytesWritten);
    
    if (res != FR_OK || bytesWritten != bytesToWrite) 
    {
        printf("д�����: Ԥ�� %u �ֽ�, ʵ�� %u �ֽ�, ���� %d\n", 
               bytesToWrite, bytesWritten, res);
    }
    
    // ��ÿ10��д��ͬ��һ�Σ����������ܣ�
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
    
    // 1. ��ȡADC����
    if (ADS1285_ReadRawData(&hads1285, &raw_data) != HAL_OK) 
    {
        return;
    }
    
    // 2. ��ȡ��ǰ�������
    volatile DataBuffer* activeBuffer = &g_bufferSystem.buffers[g_bufferSystem.activeIndex];
    
    // 3. ��黺�����Ƿ�����
    if (activeBuffer->sampleCount >= SAMPLES_PER_BUFFER) 
    {
        // 4. ��ǵ�ǰ����������
        activeBuffer->isFull = true;
        bufferFullCount++;
        
        // 5. ������һ�����л�����
        uint8_t nextIndex = (g_bufferSystem.activeIndex + 1) % BUFFER_COUNT;
        uint8_t startIndex = nextIndex;
        uint8_t found = 0;
        
        do 
        {
            // ��黺�����Ƿ���ã�δ����δ������
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
            // ���л�������æ����������������
            g_bufferSystem.lostSamples++;
            bufferOverrunCount++;
            return;
        }
        
        // 6. �л����»�����
        g_bufferSystem.activeIndex = nextIndex;
        activeBuffer = &g_bufferSystem.buffers[nextIndex];
        activeBuffer->sampleCount = 0;  // ���ü�����
    }
    
    // 7. �洢����
    activeBuffer->data[activeBuffer->sampleCount] = raw_data;
    activeBuffer->sampleCount++;
    g_bufferSystem.totalSamples++;
}


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    // ����Ƿ���DRDY���ŵ��ж�
    if (GPIO_Pin == hads1285.drdy_pin)
    {
        // ֻ��case 2״̬�²ɼ�����
        if (appState  == STATE_ACQUIRE)
        {
            ADS1285_Test(); // �ɼ����ݵ����ػ���
        }
    }
}