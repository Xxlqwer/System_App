#include <string.h>
#include <stdio.h>
#include <stdint.h>  // 添加标准整数类型定义
#include "usart.h"   
#include "main.h"     
#include "rtc.h"
#include <stdbool.h>
#include "um980.h"
#include <ctype.h>  // for isdigit
#include <stdlib.h> // for atof
#include "data_fusion.h"


#define UART_BUFFER_SIZE 256  //定义接收缓冲区大小和索引
uint8_t rx_buffer[RX_BUFFER_SIZE] = {0}; // 存储接收到的数据
//uint8_t tx_buffer[] = "GPGGA 1\r\n"; // UM980 的状态查询命令  MODE    获取位置信息 CONFIG GPGGA bestpos
uint8_t tx_buffer[] = "GPZDA 1\r\n";


uint8_t gnssByte = 0;                          // 单字节接收变量
uint8_t gnssBuffer[GNSS_BUFFER_SIZE] = {0};    // GNSS 拼接缓冲区
uint8_t gnssIndex = 0;                         // 当前接收索引

// 将 rx 数组定义到全局范围
uint8_t rx_it[100];
uint8_t data_index = 0;
extern uint32_t lastSyncTime;

// 用于存储接收到的 GNGGA 数据
char gps_data[RX_BUFFER_SIZE] = {0};  // 确保 gps_data 是静态变量，避免跨函数调用时丢失

RTC_TimeTypeDef g_baseRtcTime;
RTC_DateTypeDef g_baseRtcDate;
uint32_t g_baseTick = 0;

volatile bool gnssReady = false;
// USART6 中断处理函数  USART6_IRQHandler

/**
 * @brief 使用 USART6 发送命令并在中断接收 UM980 的响应
 */
void UM980_StatusCheck(void) {
    // 清空接收缓冲区
    memset(rx_buffer, 0, sizeof(rx_buffer));

    // 发送查询命令
    if (HAL_UART_Transmit(&huart6, tx_buffer, sizeof(tx_buffer) - 1, 1000) != HAL_OK) {
        printf("Failed to send command to UM980\r\n");
        return ;
    }
		
		// 使用非阻塞方式接收数据
		//HAL_UART_Receive_IT(&huart6, rx_buffer, sizeof(rx_buffer));
		
}

// 验证 GPRMC数据是否有效
int valid_GPZDA(uint8_t* data) {
	
    // 将数据转换为字符串
    char str[RX_BUFFER_SIZE + 1];
    strncpy(str, (char*)data, RX_BUFFER_SIZE);
    str[RX_BUFFER_SIZE] = '\0';
	
		// 判断是否为 GPRMC 或 GNRMC 报文
	if (strncmp(str, "$GNZDA", 6) != 0 ) {
			return 0;  // 非 $GNZDA,095107.00,22,06,2025,,*71
	}
    // 查找逗号分隔符
    char* token = strtok(str, ",");
    int index = 0;
		int hour = 0, min = 0, sec = 0;
		int day = 0, month = 0, year = 0;
    while (token!= NULL) {
         // 检查第一个逗号后的内容是否符合时间格式 hhmmss.ss
        if (index == 1) {
								// 提取时间字段
								float sec_decimal = 0;
								sscanf(token, "%2d%2d%2d.%f", &hour, &min, &sec, &sec_decimal);
						} else if (index == 9) {
								// 提取日期字段
								sscanf(token, "%2d%2d%2d", &day, &month, &year);
								//year += 2000;  // 补充年份前缀
						}
						token = strtok(NULL, ",");
						index++;
    }
		// 设置 STM32 RTC
//RTC_TimeTypeDef sTime = {0};
//sTime.Hours   = hour;
//sTime.Minutes = min;
//sTime.Seconds = sec;
//HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);

//RTC_DateTypeDef sDate = {0};
//sDate.Date  = day;
//sDate.Month = month;
//sDate.Year  = year;
//HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
    return 1;  // 有效数据
}



// 从 GPZDA 数据中提取时间信息
void extract_time(uint8_t* data) {
    // 将数据转换为字符串
    char str[RX_BUFFER_SIZE + 1];
    strncpy(str, (char*)data, RX_BUFFER_SIZE);
    str[RX_BUFFER_SIZE] = '\0';
	// 判断报文类型   GPZDA,011651.00,03,07,2025,,*65
	if (strncmp(str, "$GNZDA", 6) != 0 && strncmp(str, "$GPZDA", 6) != 0)
			return ;
	
    // 查找逗号分隔符
    char* token = strtok(str, ",");
    int index = 0;
		
		int day = 0, month = 0, year = 0;
		char time_string[10] = {0};  // 用于传给时间设置函数
    while (token!= NULL) {
         switch (index) {
        case 1: 
					 strncpy(time_string, token, 8);
            time_string[8] = '\0';  // 终止字符串
            break;
        case 2: day   = atoi(token); break;
        case 3: month = atoi(token); break;
        case 4: year  = atoi(token); break;
    }
    token = strtok(NULL, ",");
    index++;
    }
		// 校验时间数据合理性
    if (day < 1 || day > 31 || month < 1 || month > 12 || year < 2000 || year > 2099)
        return;
		
		// 设置日期
		RTC_DateTypeDef sDate = {0};
		sDate.Date  = day;
		sDate.Month = month;
		sDate.Year  = (uint8_t)(year - 2000);  // STM32 RTC年份为2000年起
		if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK) {
				Error_Handler();
		}
		
		// 设置时间
		Set_RTC_From_GPZDA_BIN(time_string); 
		
}

void Set_RTC_From_GPZDA_BIN(char* time_string) {
    // 从 GNGGA 时间字段解析 UTC 时间（hhmmss 格式）
    int hh = (time_string[0] - '0') * 10 + (time_string[1] - '0');
    int mm = (time_string[2] - '0') * 10 + (time_string[3] - '0');
    int ss = (time_string[4] - '0') * 10 + (time_string[5] - '0');
    // 转为北京时间（+8 小时）
    hh += 8;
    if (hh >= 24) {
        hh -= 24;

        // 获取当前日期（使用 BIN）
        RTC_DateTypeDef sDate;
        HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

        // 增加一天并处理溢出
        Date tempDate = {sDate.Year, sDate.Month, sDate.Date + 1};
        handleDateOverflow(&tempDate); 

        // 更新日期
        sDate.Year  = tempDate.year;
        sDate.Month = tempDate.month;
        sDate.Date  = tempDate.day;

        if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK) {
            Error_Handler();
        }
    }

    // 设置时间结构体
    RTC_TimeTypeDef sTime = {0};
    sTime.Hours   = hh;
    sTime.Minutes = mm;
    sTime.Seconds = ss;
    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sTime.StoreOperation = RTC_STOREOPERATION_RESET;

    printf("GPS_Time(BIN): %02d:%02d:%02d\n", hh, mm, ss);

    if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK) {
        Error_Handler();
    }
		
}


 
// 判断是否为闰年
int isLeapYear(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}


// 处理日期溢出
void handleDateOverflow(Date *date) {
    int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}; //12个月份的天数
    if (isLeapYear(date->year)){
        daysInMonth[1] = 29;  // 闰年2月有29天
    }

    if (date->day > daysInMonth[date->month - 1]){
        date->day = 1;
        date->month++;
        if (date->month > 12) {
            date->month = 1;
            date->year++;
        }
    }
}

bool PerformTimeSync(void) 
{
		gnssReady = false;  // 清除旧标志
		HAL_NVIC_EnableIRQ(USART6_IRQn);  // 开启中断
	    HAL_Delay(200);  // 等待GNSS响应
		UM980_StatusCheck();//发送命令给um980
    
    // 2. 等待并解析时间信息
    uint32_t start = HAL_GetTick();
    while ((HAL_GetTick() - start <10000))
		{
			if (gnssReady)
        {
            break;
        }
		}
		
		HAL_NVIC_DisableIRQ(USART6_IRQn); // 授时后关闭
		if (!gnssReady)
    {
        printf("授时失败：未收到有效 GNSS 数据\r\n");
        return false;
    }
		extract_time(gnssBuffer);  // 解析时间信息
    // 关键修改：在授时完成后立即更新时间戳
    lastSyncTime = HAL_GetTick();  // 使用授时完成时刻的时间戳
    printf("授时成功，下次授时在%u小时后 (当前时间戳: %u)\r\n", 
           SYNC_INTERVAL_HOURS, lastSyncTime);
    
    return true;
}


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART6)
    {
        if (gnssIndex < GNSS_BUFFER_SIZE - 1) {
            gnssBuffer[gnssIndex++] = gnssByte;
					 // 2. 调试打印接收内容（建议放这里）
//            if (gnssByte >= 32 && gnssByte <= 126) {
//                printf("%c", gnssByte);  // 打印可见字符
//            } else if (gnssByte == '\r') {
//                printf("<CR>");
//            } else if (gnssByte == '\n') {
//                printf("<LF>\n");
//            }

            if (gnssByte == '\n')  // 结束符
            {
                gnssBuffer[gnssIndex] = '\0';  // 终止字符串
                if (valid_GPZDA(gnssBuffer)) {
										 if (systemState == SYSTEM_NORMAL_OPERATION) 
											{
													appState = STATE_GNSS_RECEIVED;
											} 
											else 
											{
												printf("USB连接状态，忽略 GNSS 数据：%s", gnssBuffer);
												if (needTimeSync) 
												{
														printf("跳过当前授时请求，延迟下次同步。\r\n");
														needTimeSync = false;			
												}
											}
										gnssIndex = 0;  // 重置索引，无论成功与否
            }
        } else {
            // 防止越界
            gnssIndex = 0;
								}

        // 继续接收下一个字节
        HAL_UART_Receive_IT(&huart6, &gnssByte, 1);
    }
	}
}

int valid_RxCplt(const char* data) {
    // 1. 将字节数组转换为字符串
    char str[RX_BUFFER_SIZE + 1];
    strncpy(str, (const char*)data, RX_BUFFER_SIZE);
    str[RX_BUFFER_SIZE] = '\0';

    // 2. 必须以 $GNZDA 或 $GPZDA 开头
    if (strncmp(str, "$GNZDA", 6) != 0 && strncmp(str, "$GPZDA", 6) != 0) {
        return 0;
    }

    // 3. 解析时间和日期字段
    int hour, min, sec;
    int day, month, year;

    int matched = sscanf(str, "$%*2sZDA,%2d%2d%2d.00,%d,%d,%d",
                         &hour, &min, &sec, &day, &month, &year);

    if (matched != 6) return 0;

    // 4. 简单校验时间是否合法
    if (hour > 23 || min > 59 || sec > 59) return 0;
    if (month < 1 || month > 12 || day < 1 || day > 31) return 0;
    if (year < 2024 || year > 2099) return 0;

    return 1;  // 合法的 GNZDA 报文
}


 // 关闭 UM980的Vcc
void UM980_DeInit(void) {
		// 将 PG2 配置为低电平 ,um980关闭
   HAL_GPIO_WritePin(GPIOG, GPIO_PIN_2, GPIO_PIN_RESET);
}
 

// USART6是 stm32和um980之间收发消息的，printf需要发送到调试串口 USART1
void Debug_Print(char *message) {
    HAL_UART_Transmit(&huart1, (uint8_t*)message, strlen(message), HAL_MAX_DELAY);
}

