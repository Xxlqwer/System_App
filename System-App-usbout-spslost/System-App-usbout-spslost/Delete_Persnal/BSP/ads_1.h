#ifndef ADS_1_H
#define ADS_1_H
#include "stm32f4xx_hal.h"

// 寄存器地址
#define	ID_SYNC_REG				0x00
#define	CONFIG0_REG 			0x01
#define	CONFIG1_REG 			0x02
#define	HPF0_REG				0x03
#define	HPF1_REG				0x04
#define	OFFSET0_REG				0x05
#define	OFFSET1_REG				0x06
#define	OFFSET2_REG				0x07
#define	GAIN0_REG				0x08
#define	GAIN1_REG				0x09
#define	GAIN2_REG				0x0A
#define	GPIO_REG				0x0B
#define	SRC0_REG				0x0C
#define	SRC1_REG				0x0D

// 命令：
#define	WAKEUP				      0x00
#define	STANDBY 				  0x02
#define	SYNC 				      0x04
#define	ADS1285_RESET			  0x06
#define	RDATA				      0x12
#define	OFSCAL				      0x60
#define	GANCAL				      0x61


//  个寄存器位：
#define  PULSEMODE          0
#define  CONTINUOUSMODE     1

#define  HIGHPOWER          0
#define  MIDPOWER           1
#define  LOWPOWER           2
#define  DePOWER            3

#define  SPS250             0
#define  SPS500             1
#define  SPS1000            2
#define  SPS2000            3
#define  SPS4000            4

#define  LINEAR_PHS         0
#define  MINI_PHS           1

#define  FILTER_RES         0
#define  SINC               1
#define  FIR                2
#define  FIR_IIR            3

#define  INPUT_1  			0
#define  INPUT_2  			1
#define  INTERNAL_400  		2
#define  INPUT_1andINPUT_2  3
#define  RESERVED_4  		4
#define  INTERNAL_0  		5
#define  RESERVED_6  		6
#define  RESERVED_7  		7

#define  REF_5V  			0
#define  REF_4_096V  		1
#define  REF_2_5V  			2
#define  RESERVED  			3

#define  GAIN_1  			0
#define  GAIN_2  			1
#define  GAIN_4  			2
#define  GAIN_8  			3
#define  GAIN_16  			4
#define  GAIN_32  			5
#define  GAIN_64  			6
#define  BUFFER  			7

#define ADS_X_DRDY_GPIO_Port GPIOD
#define ADS_X_DRDY_Pin  GPIO_PIN_12
#define ADC_X_CS_GPIO_Port  GPIOE
#define ADC_X_CS_Pin  GPIO_PIN_11

// 假设量程为 +/- 4.096
#define FULL_SCALE_VOLTAGE 4.096

// ADS1285 DRDY引脚，若是该引脚为低电平，则表示数据转换完成
#define  DRDY  HAL_GPIO_ReadPin(ADS_X_DRDY_GPIO_Port, ADS_X_DRDY_Pin) // GPIOD , GPIO_PIN_12
// CS引脚
#define  ADC_X_CS_LOW  HAL_GPIO_WritePin(ADC_X_CS_GPIO_Port, ADC_X_CS_Pin,0)  // GPIOE  GPIO_PIN_11
#define  ADC_X_CS_HIGH HAL_GPIO_WritePin(ADC_X_CS_GPIO_Port, ADC_X_CS_Pin,1)

extern uint8_t readData[4];

// 写入单个字节
void SingleByteCommand(uint8_t command);
// 直接读取数据，DRDY引脚拉点之后，将CS引脚拉低，然后读取数据
void Direct_Read_Data(uint8_t * Data);
// 读取数据
void ReadConversionData(uint8_t * Data,uint8_t command);
// 读取寄存器,穿入起始地址和读取寄存器的个数
void ReadReg(uint8_t startAddr, uint8_t *regData, uint8_t number);
// 写入寄存器,穿入起始地址和读取寄存器的个数
void WriteReg(uint8_t *writeRegData, uint8_t number,uint8_t first);
// 初始化
void ADS1285_Init_1(uint8_t const startAddr,uint8_t number);

int isNegative(int32_t data);
float convertToVoltage(int32_t data);

#endif
