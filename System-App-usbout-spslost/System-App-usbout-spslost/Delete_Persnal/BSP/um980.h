#ifndef UM980_H
#define UM980_H

#include "usart.h"  // ���� USART ����ͷ�ļ�
#include "stdbool.h"
#ifdef __cplusplus
extern "C" {
#endif

// �������ڽṹ��
typedef struct {
    int year;
    int month;
    int day;
} Date;

#define RX_BUFFER_SIZE 512  //������ջ�������С������
// ������ʱ��ر���
#define SYNC_INTERVAL_HOURS 1  // ��ʱ�����Сʱ��
#define GNSS_BUFFER_SIZE 128

extern uint8_t rx_buffer[RX_BUFFER_SIZE];  // ���� rx_buffer ����
void Debug_Print(char *message);

void UM980_StatusCheck(void);

// ���� gps_data ����
extern char gps_data[RX_BUFFER_SIZE];  // �� um980����
extern volatile bool needTimeSync;


/**
 * @brief �����õ� ʹ�� USART6 ����������� UM980 ����Ӧ
 * �ú���ͨ�� USART6 �� UM980 ģ�鷢�Ͳ�ѯ״̬������������䷵�ص���Ӧ��
 * @note �ڵ��ô˺���ǰ��ȷ�� USART6 �Ѿ���ȷ��ʼ����
 */
void Set_RTC_From_GPGGA(char* time_string);
void Set_RTC_From_GPZDA_BIN(char* time_string);

void UM980_StatusCheck(void);
void UM980_DeInit(void);
bool PerformTimeSync(void);  // ������������

//int is_valid_GNGGA_data(uint8_t* data); //�ж�����֤�����Ƿ���Ч
int valid_GPZDA(uint8_t* data);

void extract_time(uint8_t* data);
int isLeapYear(int year);
void handleDateOverflow(Date *date);



#ifdef __cplusplus
}
#endif

#endif // UM980_H
