#ifndef ADS1285_H
#define ADS1285_H

#include "main.h"
#include "spi.h"


#define VREF_VALUE 4.096   // ��8-10   FIR ����оƬ���ӵĲο���ѹREF���� ת��ʱ����2.5v ,,SINC FILTER��Ҫ����
// ������2.5  -2.54����õ�-1.975  
// 4.096 -2.54����õ� -3.236

#define ADC_MAX 2147483647  // 2^31 - 1��32λ���ֵ
#define ADC_MIN -2147483648 // 2^31��32λ��Сֵ
#define GAIN_DEFAULT 1             // ��������ΪĬ��ֵ1
#define ADS1285_TIMEOUT 1000       // ��ʱ����
#define ADS1285_DATA_LENGTH 4      // ��ȡ�������ֽڳ���


// �Ĵ�������
#define ADS1285_CMD_WREG     0x40  // д�Ĵ�������

#define ADS1285_CMD_RREG    0x20  // ��ȡ�Ĵ�������
#define ADS1285_CMD_RDATA    0x12  // ��ȡת������
#define SYNC_REG_ADDR        0x00 //SYNC�Ĵ�����ַ         
#define CONFIG0_REG_ADDR     0x01  // CONFIG0�Ĵ�����ַ
#define CONFIG1_REG_ADDR     0x02  // CONFIG1�Ĵ�����ַ

#define ADS1285_CMD_SYNC     0x04  // ͬ������
#define ADS1285_CMD_WAKEUP   0x00  // ��������

#define ADS1285_CMD_OFSCAL   0x60  // д�Ĵ�������
#define ADS1285_CMD_GANCAL   0x61  // д�Ĵ�������

// ����ͨ��
#define AIN1_CHANNEL 1
#define AIN2_CHANNEL 2
#define AIN_BOTH     3

// ADS1285��ʱʱ�䶨��
#define ADS1285_TIMEOUT 1000



// ADS1285����ṹ�嶨��
typedef struct {
    SPI_HandleTypeDef *hspi;    // SPI�����������ADS1285����ͨ��
    GPIO_TypeDef *cs_port;      // CS���Ŷ˿ڣ�����ѡ��ADS1285�豸
    uint16_t cs_pin;            // CS���ţ�����Ƭѡ����
    GPIO_TypeDef *drdy_port;    // DRDY���Ŷ˿ڣ�����ָʾ����׼����
    uint16_t drdy_pin;          // DRDY���ţ����ڶ�ȡ����׼��״̬
	  GPIO_TypeDef *sync_port;
    uint16_t sync_pin;          // DRDY���ţ����ڶ�ȡ����׼��״̬ 	
} ADS1285_HandleTypeDef;

// ��������
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