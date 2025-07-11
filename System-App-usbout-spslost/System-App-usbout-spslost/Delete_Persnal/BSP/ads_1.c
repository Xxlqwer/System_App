#include "ads_1.h"
//#include "MCT8316.h"
#include "spi.h"
#include "math.h"

uint8_t readData[4] ={0x00};

uint8_t data[20] = {0x00};

uint32_t readDataFinal = 0x0000000;

uint32_t offsetFinal = 0x0000000,gainFinal = 0x0000000;

uint8_t readData1,readData2,readData3,readData4;

// 直接读取数据
void Direct_Read_Data(uint8_t * Data)
{
	int i = 0;
	uint8_t command;
	uint8_t data_temp;

	while(DRDY);
	
	ADC_X_CS_LOW;  // 拉低CS引脚
	
	for(i=0;i<4;i++)
	{
		HAL_SPI_Receive(&hspi4,(uint8_t *)&Data[i],1,50);// 读取数据
	}
	readDataFinal = (Data[0]<<24)|(Data[1]<<16)|(Data[2]<<8)|(Data[3]);
	
	ADC_X_CS_HIGH;  // 拉低CS引脚
	HAL_Delay(1);
}

// 读取数据，指令--0x12
void ReadConversionData(uint8_t * Data,uint8_t command)
{
	int i = 0;
	ADC_X_CS_LOW;  // 拉低CS引脚
	
	HAL_SPI_Transmit(&hspi4,(uint8_t *)&command,1,10);

	for(i=0;i<4;i++)
	{
		HAL_SPI_Receive(&hspi4,(uint8_t *)&Data[i],1,10);// 读取数据
	}
	readDataFinal = (Data[0]<<24)|(Data[1]<<16)|(Data[2]<<8)|(Data[3]);
	
	ADC_X_CS_HIGH;  // 拉低CS引脚	
	
}
// 写寄存器
uint8_t  writeData = 0;
void WriteReg(uint8_t *writeRegData, uint8_t number,uint8_t first)
{
	uint8_t  i=0;// 要写入的数据
	uint8_t subNumber = number - 1;
	
	writeRegData[0] = first;
	writeRegData[1] = subNumber;  // 将代码个数写入
	
	ADC_X_CS_LOW;  // 拉低CS引脚
	
	for(i=0;i<number+2;i++){
		writeData = writeRegData[i];
		HAL_SPI_Transmit(&hspi4,(uint8_t *)&writeData,1,10);
	}
	ADC_X_CS_HIGH;  // 拉高CS引脚
}
// 读寄存器的值
void ReadReg(uint8_t const startAddr, uint8_t *regData, uint8_t number)
{
	uint8_t  writeReadData = 0,i,readDataTemp = 0;
	
	ADC_X_CS_LOW;  // 拉低CS引脚
	
	writeReadData = (0x02<<4)|startAddr;
	HAL_SPI_Transmit(&hspi4,(uint8_t *)&writeReadData,1,10);
	
	writeReadData = (0x00<<4)|(number - 1);
	HAL_SPI_Transmit(&hspi4,(uint8_t *)&writeReadData,1,10);
	
	for(i=0;i<number;i++){
		HAL_SPI_Receive(&hspi4,(uint8_t *)&readDataTemp,1,10);// 读取数据
		regData[i] = readDataTemp;
	}
	offsetFinal = (regData[2]<<16) | (regData[1]<<8) | (regData[0]);
	gainFinal   = (regData[5]<<16) | (regData[4]<<8) | (regData[3]);
	ADC_X_CS_HIGH;  // 拉高CS引脚
}
//  初始化：
void ADS1285_Init_1(uint8_t const startAddr,uint8_t number)
{
	data[0] = (0x04<<4)|startAddr;// 地址
  data[1] = number - 1;           // 写入的寄存器个数
  data[2] = (0x00<<7)|PULSEMODE;  // (0x00<<7)|CONTINUOUSMODE
	data[3] = (MIDPOWER<<6)|(SPS250<<3)|(MINI_PHS<<2)|FILTER_RES ; // 00011011 (HIGHPOWER<<6)|(SPS1000<<3)|(LINEAR_PHS<<2)|FIR  // 0100 0011
	data[4] = (INPUT_1<<5) | (REF_4_096V<<3) | GAIN_1;  // 00001000
//	data[5] = ;
//	data[6] = ;
//	data[7] = ;
//	data[8] = ; 
 	WriteReg(data,number,data[0]);
}
// 写入单个寄存器
void SingleByteCommand(uint8_t command)
{
	ADC_X_CS_LOW;   // 拉低CS引脚
	HAL_SPI_Transmit(&hspi4,(uint8_t *)&command,1,10);
	ADC_X_CS_HIGH;  // 拉高CS引脚
}

// 判断数据是否为负数
int isNegative(int32_t data) {
    return (data < 0);
}


// 换算为电压值
float convertToVoltage(int32_t data) {
    uint32_t N = 0xFFFFFFFF + 1;  // 2^32
    if (isNegative(data)) {
        return -((float)(N - 1 - (uint32_t)data) / (N - 1)) * FULL_SCALE_VOLTAGE;
    } else {
        return ((float)data / (N - 1)) * FULL_SCALE_VOLTAGE;
    }
}