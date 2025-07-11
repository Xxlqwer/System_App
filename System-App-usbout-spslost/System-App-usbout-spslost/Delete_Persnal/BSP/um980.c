#include <string.h>
#include <stdio.h>
#include <stdint.h>  // ��ӱ�׼�������Ͷ���
#include "usart.h"   
#include "main.h"     
#include "rtc.h"
#include <stdbool.h>
#include "um980.h"
#include <ctype.h>  // for isdigit
#include <stdlib.h> // for atof
#include "data_fusion.h"


#define UART_BUFFER_SIZE 256  //������ջ�������С������
uint8_t rx_buffer[RX_BUFFER_SIZE] = {0}; // �洢���յ�������
//uint8_t tx_buffer[] = "GPGGA 1\r\n"; // UM980 ��״̬��ѯ����  MODE    ��ȡλ����Ϣ CONFIG GPGGA bestpos
uint8_t tx_buffer[] = "GPZDA 1\r\n";


uint8_t gnssByte = 0;                          // ���ֽڽ��ձ���
uint8_t gnssBuffer[GNSS_BUFFER_SIZE] = {0};    // GNSS ƴ�ӻ�����
uint8_t gnssIndex = 0;                         // ��ǰ��������

// �� rx ���鶨�嵽ȫ�ַ�Χ
uint8_t rx_it[100];
uint8_t data_index = 0;
extern uint32_t lastSyncTime;

// ���ڴ洢���յ��� GNGGA ����
char gps_data[RX_BUFFER_SIZE] = {0};  // ȷ�� gps_data �Ǿ�̬����������纯������ʱ��ʧ

RTC_TimeTypeDef g_baseRtcTime;
RTC_DateTypeDef g_baseRtcDate;
uint32_t g_baseTick = 0;

volatile bool gnssReady = false;
// USART6 �жϴ�����  USART6_IRQHandler

/**
 * @brief ʹ�� USART6 ����������жϽ��� UM980 ����Ӧ
 */
void UM980_StatusCheck(void) {
    // ��ս��ջ�����
    memset(rx_buffer, 0, sizeof(rx_buffer));

    // ���Ͳ�ѯ����
    if (HAL_UART_Transmit(&huart6, tx_buffer, sizeof(tx_buffer) - 1, 1000) != HAL_OK) {
        printf("Failed to send command to UM980\r\n");
        return ;
    }
		
		// ʹ�÷�������ʽ��������
		//HAL_UART_Receive_IT(&huart6, rx_buffer, sizeof(rx_buffer));
		
}

// ��֤ GPRMC�����Ƿ���Ч
int valid_GPZDA(uint8_t* data) {
	
    // ������ת��Ϊ�ַ���
    char str[RX_BUFFER_SIZE + 1];
    strncpy(str, (char*)data, RX_BUFFER_SIZE);
    str[RX_BUFFER_SIZE] = '\0';
	
		// �ж��Ƿ�Ϊ GPRMC �� GNRMC ����
	if (strncmp(str, "$GNZDA", 6) != 0 ) {
			return 0;  // �� $GNZDA,095107.00,22,06,2025,,*71
	}
    // ���Ҷ��ŷָ���
    char* token = strtok(str, ",");
    int index = 0;
		int hour = 0, min = 0, sec = 0;
		int day = 0, month = 0, year = 0;
    while (token!= NULL) {
         // ����һ�����ź�������Ƿ����ʱ���ʽ hhmmss.ss
        if (index == 1) {
								// ��ȡʱ���ֶ�
								float sec_decimal = 0;
								sscanf(token, "%2d%2d%2d.%f", &hour, &min, &sec, &sec_decimal);
						} else if (index == 9) {
								// ��ȡ�����ֶ�
								sscanf(token, "%2d%2d%2d", &day, &month, &year);
								//year += 2000;  // �������ǰ׺
						}
						token = strtok(NULL, ",");
						index++;
    }
		// ���� STM32 RTC
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
    return 1;  // ��Ч����
}



// �� GPZDA ��������ȡʱ����Ϣ
void extract_time(uint8_t* data) {
    // ������ת��Ϊ�ַ���
    char str[RX_BUFFER_SIZE + 1];
    strncpy(str, (char*)data, RX_BUFFER_SIZE);
    str[RX_BUFFER_SIZE] = '\0';
	// �жϱ�������   GPZDA,011651.00,03,07,2025,,*65
	if (strncmp(str, "$GNZDA", 6) != 0 && strncmp(str, "$GPZDA", 6) != 0)
			return ;
	
    // ���Ҷ��ŷָ���
    char* token = strtok(str, ",");
    int index = 0;
		
		int day = 0, month = 0, year = 0;
		char time_string[10] = {0};  // ���ڴ���ʱ�����ú���
    while (token!= NULL) {
         switch (index) {
        case 1: 
					 strncpy(time_string, token, 8);
            time_string[8] = '\0';  // ��ֹ�ַ���
            break;
        case 2: day   = atoi(token); break;
        case 3: month = atoi(token); break;
        case 4: year  = atoi(token); break;
    }
    token = strtok(NULL, ",");
    index++;
    }
		// У��ʱ�����ݺ�����
    if (day < 1 || day > 31 || month < 1 || month > 12 || year < 2000 || year > 2099)
        return;
		
		// ��������
		RTC_DateTypeDef sDate = {0};
		sDate.Date  = day;
		sDate.Month = month;
		sDate.Year  = (uint8_t)(year - 2000);  // STM32 RTC���Ϊ2000����
		if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK) {
				Error_Handler();
		}
		
		// ����ʱ��
		Set_RTC_From_GPZDA_BIN(time_string); 
		
}

void Set_RTC_From_GPZDA_BIN(char* time_string) {
    // �� GNGGA ʱ���ֶν��� UTC ʱ�䣨hhmmss ��ʽ��
    int hh = (time_string[0] - '0') * 10 + (time_string[1] - '0');
    int mm = (time_string[2] - '0') * 10 + (time_string[3] - '0');
    int ss = (time_string[4] - '0') * 10 + (time_string[5] - '0');
    // תΪ����ʱ�䣨+8 Сʱ��
    hh += 8;
    if (hh >= 24) {
        hh -= 24;

        // ��ȡ��ǰ���ڣ�ʹ�� BIN��
        RTC_DateTypeDef sDate;
        HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

        // ����һ�첢�������
        Date tempDate = {sDate.Year, sDate.Month, sDate.Date + 1};
        handleDateOverflow(&tempDate); 

        // ��������
        sDate.Year  = tempDate.year;
        sDate.Month = tempDate.month;
        sDate.Date  = tempDate.day;

        if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK) {
            Error_Handler();
        }
    }

    // ����ʱ��ṹ��
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


 
// �ж��Ƿ�Ϊ����
int isLeapYear(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}


// �����������
void handleDateOverflow(Date *date) {
    int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}; //12���·ݵ�����
    if (isLeapYear(date->year)){
        daysInMonth[1] = 29;  // ����2����29��
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
		gnssReady = false;  // ����ɱ�־
		HAL_NVIC_EnableIRQ(USART6_IRQn);  // �����ж�
	    HAL_Delay(200);  // �ȴ�GNSS��Ӧ
		UM980_StatusCheck();//���������um980
    
    // 2. �ȴ�������ʱ����Ϣ
    uint32_t start = HAL_GetTick();
    while ((HAL_GetTick() - start <10000))
		{
			if (gnssReady)
        {
            break;
        }
		}
		
		HAL_NVIC_DisableIRQ(USART6_IRQn); // ��ʱ��ر�
		if (!gnssReady)
    {
        printf("��ʱʧ�ܣ�δ�յ���Ч GNSS ����\r\n");
        return false;
    }
		extract_time(gnssBuffer);  // ����ʱ����Ϣ
    // �ؼ��޸ģ�����ʱ��ɺ���������ʱ���
    lastSyncTime = HAL_GetTick();  // ʹ����ʱ���ʱ�̵�ʱ���
    printf("��ʱ�ɹ����´���ʱ��%uСʱ�� (��ǰʱ���: %u)\r\n", 
           SYNC_INTERVAL_HOURS, lastSyncTime);
    
    return true;
}


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART6)
    {
        if (gnssIndex < GNSS_BUFFER_SIZE - 1) {
            gnssBuffer[gnssIndex++] = gnssByte;
					 // 2. ���Դ�ӡ�������ݣ���������
//            if (gnssByte >= 32 && gnssByte <= 126) {
//                printf("%c", gnssByte);  // ��ӡ�ɼ��ַ�
//            } else if (gnssByte == '\r') {
//                printf("<CR>");
//            } else if (gnssByte == '\n') {
//                printf("<LF>\n");
//            }

            if (gnssByte == '\n')  // ������
            {
                gnssBuffer[gnssIndex] = '\0';  // ��ֹ�ַ���
                if (valid_GPZDA(gnssBuffer)) {
										 if (systemState == SYSTEM_NORMAL_OPERATION) 
											{
													appState = STATE_GNSS_RECEIVED;
											} 
											else 
											{
												printf("USB����״̬������ GNSS ���ݣ�%s", gnssBuffer);
												if (needTimeSync) 
												{
														printf("������ǰ��ʱ�����ӳ��´�ͬ����\r\n");
														needTimeSync = false;			
												}
											}
										gnssIndex = 0;  // �������������۳ɹ����
            }
        } else {
            // ��ֹԽ��
            gnssIndex = 0;
								}

        // ����������һ���ֽ�
        HAL_UART_Receive_IT(&huart6, &gnssByte, 1);
    }
	}
}

int valid_RxCplt(const char* data) {
    // 1. ���ֽ�����ת��Ϊ�ַ���
    char str[RX_BUFFER_SIZE + 1];
    strncpy(str, (const char*)data, RX_BUFFER_SIZE);
    str[RX_BUFFER_SIZE] = '\0';

    // 2. ������ $GNZDA �� $GPZDA ��ͷ
    if (strncmp(str, "$GNZDA", 6) != 0 && strncmp(str, "$GPZDA", 6) != 0) {
        return 0;
    }

    // 3. ����ʱ��������ֶ�
    int hour, min, sec;
    int day, month, year;

    int matched = sscanf(str, "$%*2sZDA,%2d%2d%2d.00,%d,%d,%d",
                         &hour, &min, &sec, &day, &month, &year);

    if (matched != 6) return 0;

    // 4. ��У��ʱ���Ƿ�Ϸ�
    if (hour > 23 || min > 59 || sec > 59) return 0;
    if (month < 1 || month > 12 || day < 1 || day > 31) return 0;
    if (year < 2024 || year > 2099) return 0;

    return 1;  // �Ϸ��� GNZDA ����
}


 // �ر� UM980��Vcc
void UM980_DeInit(void) {
		// �� PG2 ����Ϊ�͵�ƽ ,um980�ر�
   HAL_GPIO_WritePin(GPIOG, GPIO_PIN_2, GPIO_PIN_RESET);
}
 

// USART6�� stm32��um980֮���շ���Ϣ�ģ�printf��Ҫ���͵����Դ��� USART1
void Debug_Print(char *message) {
    HAL_UART_Transmit(&huart1, (uint8_t*)message, strlen(message), HAL_MAX_DELAY);
}

