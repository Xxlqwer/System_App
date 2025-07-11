#ifndef ADS1285_H
#define ADS1285_H

#include "main.h"
#include "spi.h"


#define VREF_VALUE 4.096   // 表8-10   FIR 不管芯片连接的参考电压REF多少 转换时都用2.5v ,,SINC FILTER需要调整
// 计算用2.5  -2.54换算得到-1.975  
// 4.096 -2.54换算得到 -3.236

#define ADC_MAX 2147483647  // 2^31 - 1，32位最大值
#define ADC_MIN -2147483648 // 2^31，32位最小值
#define GAIN_DEFAULT 1             // 增益设置为默认值1
#define ADS1285_TIMEOUT 1000       // 超时设置
#define ADS1285_DATA_LENGTH 4      // 读取的数据字节长度


// 寄存器命令
#define ADS1285_CMD_WREG     0x40  // 写寄存器命令

#define ADS1285_CMD_RREG    0x20  // 读取寄存器命令
#define ADS1285_CMD_RDATA    0x12  // 读取转换数据
#define SYNC_REG_ADDR        0x00 //SYNC寄存器地址         
#define CONFIG0_REG_ADDR     0x01  // CONFIG0寄存器地址
#define CONFIG1_REG_ADDR     0x02  // CONFIG1寄存器地址

#define ADS1285_CMD_SYNC     0x04  // 同步命令
#define ADS1285_CMD_WAKEUP   0x00  // 唤醒命令

#define ADS1285_CMD_OFSCAL   0x60  // 写寄存器命令
#define ADS1285_CMD_GANCAL   0x61  // 写寄存器命令

// 输入通道
#define AIN1_CHANNEL 1
#define AIN2_CHANNEL 2
#define AIN_BOTH     3

// ADS1285超时时间定义
#define ADS1285_TIMEOUT 1000



// ADS1285句柄结构体定义
typedef struct {
    SPI_HandleTypeDef *hspi;    // SPI句柄，用于与ADS1285进行通信
    GPIO_TypeDef *cs_port;      // CS引脚端口，用于选择ADS1285设备
    uint16_t cs_pin;            // CS引脚，用于片选操作
    GPIO_TypeDef *drdy_port;    // DRDY引脚端口，用于指示数据准备好
    uint16_t drdy_pin;          // DRDY引脚，用于读取数据准备状态
	  GPIO_TypeDef *sync_port;
    uint16_t sync_pin;          // DRDY引脚，用于读取数据准备状态 	
} ADS1285_HandleTypeDef;

// 函数声明
HAL_StatusTypeDef ADS1285_Init(ADS1285_HandleTypeDef *hadc);
HAL_StatusTypeDef ADS1285_ReadRegister(ADS1285_HandleTypeDef *hadc, uint8_t reg_addr, uint8_t *reg_value);

HAL_StatusTypeDef ADS1285_ReadChannel(ADS1285_HandleTypeDef *hadc, uint8_t channel, float *data);

HAL_StatusTypeDef GetVoltage(float *voltage_ch1, float *voltage_ch2);

HAL_StatusTypeDef ADS1285_ResetHardware(ADS1285_HandleTypeDef *hadc);
HAL_StatusTypeDef ADS1285_Calibration(ADS1285_HandleTypeDef *hadc);
//HAL_StatusTypeDef ADS1285_WriteReg_Continuous(ADS1285_HandleTypeDef *hadc, uint8_t channel);
HAL_StatusTypeDef ADS1285_WriteReg_Continuous(ADS1285_HandleTypeDef *hadc);
HAL_StatusTypeDef ADS1285_ReadChannel_Once (ADS1285_HandleTypeDef *hadc, float *data);
HAL_StatusTypeDef ADS1285_ReadRawData(ADS1285_HandleTypeDef *hadc, int32_t *data);
void GetCurrentDateAsString(char *filename);
HAL_StatusTypeDef Write_MUX(uint8_t value);
HAL_StatusTypeDef Wait_DRDY(void);
HAL_StatusTypeDef Read_Data(int32_t *adc_value);
HAL_StatusTypeDef ADS1285_ResetAndCalibrate(ADS1285_HandleTypeDef *had);

HAL_StatusTypeDef ADS1285_StartupSequence(ADS1285_HandleTypeDef *hadc);

#endif