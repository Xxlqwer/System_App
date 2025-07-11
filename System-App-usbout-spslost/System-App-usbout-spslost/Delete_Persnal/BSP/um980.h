#ifndef UM980_H
#define UM980_H

#include "usart.h"  // 包含 USART 配置头文件
#include "stdbool.h"
#ifdef __cplusplus
extern "C" {
#endif

// 定义日期结构体
typedef struct {
    int year;
    int month;
    int day;
} Date;

#define RX_BUFFER_SIZE 512  //定义接收缓冲区大小和索引
// 新增授时相关变量
#define SYNC_INTERVAL_HOURS 1  // 授时间隔（小时）
#define GNSS_BUFFER_SIZE 128

extern uint8_t rx_buffer[RX_BUFFER_SIZE];  // 声明 rx_buffer 变量
void Debug_Print(char *message);

void UM980_StatusCheck(void);

// 定义 gps_data 变量
extern char gps_data[RX_BUFFER_SIZE];  // 在 um980定义
extern volatile bool needTimeSync;


/**
 * @brief 测试用的 使用 USART6 发送命令并接收 UM980 的响应
 * 该函数通过 USART6 向 UM980 模块发送查询状态的命令，并接收其返回的响应。
 * @note 在调用此函数前，确保 USART6 已经正确初始化。
 */
void Set_RTC_From_GPGGA(char* time_string);
void Set_RTC_From_GPZDA_BIN(char* time_string);

void UM980_StatusCheck(void);
void UM980_DeInit(void);
bool PerformTimeSync(void);  // 新增函数声明

//int is_valid_GNGGA_data(uint8_t* data); //中断里验证数据是否有效
int valid_GPZDA(uint8_t* data);

void extract_time(uint8_t* data);
int isLeapYear(int year);
void handleDateOverflow(Date *date);



#ifdef __cplusplus
}
#endif

#endif // UM980_H
