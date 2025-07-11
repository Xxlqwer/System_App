#include "ads_1.h"
//#include "MCT8316.h"
#include "spi.h"
#include "math.h"

uint8_t readData[4] ={0x00};

uint8_t data[20] = {0x00};

uint32_t readDataFinal = 0x0000000;

uint32_t offsetFinal = 0x0000000,gainFinal = 0x0000000;

uint8_t readData1,readData2,readData3,readData4;

// ֱ�Ӷ�ȡ����
void Direct_Read_Data(uint8_t * Data)
{
	int i = 0;
	uint8_t command;
	uint8_t data_temp;

	while(DRDY);
	
	ADC_X_CS_LOW;  // ����CS����
	
	for(i=0;i<4;i++)
	{
		HAL_SPI_Receive(&hspi4,(uint8_t *)&Data[i],1,50);// ��ȡ����
	}
	readDataFinal = (Data[0]<<24)|(Data[1]<<16)|(Data[2]<<8)|(Data[3]);
	
	ADC_X_CS_HIGH;  // ����CS����
	HAL_Delay(1);
}

// ��ȡ���ݣ�ָ��--0x12
void ReadConversionData(uint8_t * Data,uint8_t command)
{
	int i = 0;
	ADC_X_CS_LOW;  // ����CS����
	
	HAL_SPI_Transmit(&hspi4,(uint8_t *)&command,1,10);

	for(i=0;i<4;i++)
	{
		HAL_SPI_Receive(&hspi4,(uint8_t *)&Data[i],1,10);// ��ȡ����
	}
	readDataFinal = (Data[0]<<24)|(Data[1]<<16)|(Data[2]<<8)|(Data[3]);
	
	ADC_X_CS_HIGH;  // ����CS����	
	
}
// д�Ĵ���
uint8_t  writeData = 0;
void WriteReg(uint8_t *writeRegData, uint8_t number,uint8_t first)
{
	uint8_t  i=0;// Ҫд�������
	uint8_t subNumber = number - 1;
	
	writeRegData[0] = first;
	writeRegData[1] = subNumber;  // ���������д��
	
	ADC_X_CS_LOW;  // ����CS����
	
	for(i=0;i<number+2;i++){
		writeData = writeRegData[i];
		HAL_SPI_Transmit(&hspi4,(uint8_t *)&writeData,1,10);
	}
	ADC_X_CS_HIGH;  // ����CS����
}
// ���Ĵ�����ֵ
void ReadReg(uint8_t const startAddr, uint8_t *regData, uint8_t number)
{
	uint8_t  writeReadData = 0,i,readDataTemp = 0;
	
	ADC_X_CS_LOW;  // ����CS����
	
	writeReadData = (0x02<<4)|startAddr;
	HAL_SPI_Transmit(&hspi4,(uint8_t *)&writeReadData,1,10);
	
	writeReadData = (0x00<<4)|(number - 1);
	HAL_SPI_Transmit(&hspi4,(uint8_t *)&writeReadData,1,10);
	
	for(i=0;i<number;i++){
		HAL_SPI_Receive(&hspi4,(uint8_t *)&readDataTemp,1,10);// ��ȡ����
		regData[i] = readDataTemp;
	}
	offsetFinal = (regData[2]<<16) | (regData[1]<<8) | (regData[0]);
	gainFinal   = (regData[5]<<16) | (regData[4]<<8) | (regData[3]);
	ADC_X_CS_HIGH;  // ����CS����
}
//  ��ʼ����
void ADS1285_Init_1(uint8_t const startAddr,uint8_t number)
{
	data[0] = (0x04<<4)|startAddr;// ��ַ
  data[1] = number - 1;           // д��ļĴ�������
  data[2] = (0x00<<7)|PULSEMODE;  // (0x00<<7)|CONTINUOUSMODE
	data[3] = (MIDPOWER<<6)|(SPS250<<3)|(MINI_PHS<<2)|FILTER_RES ; // 00011011 (HIGHPOWER<<6)|(SPS1000<<3)|(LINEAR_PHS<<2)|FIR  // 0100 0011
	data[4] = (INPUT_1<<5) | (REF_4_096V<<3) | GAIN_1;  // 00001000
//	data[5] = ;
//	data[6] = ;
//	data[7] = ;
//	data[8] = ; 
 	WriteReg(data,number,data[0]);
}
// д�뵥���Ĵ���
void SingleByteCommand(uint8_t command)
{
	ADC_X_CS_LOW;   // ����CS����
	HAL_SPI_Transmit(&hspi4,(uint8_t *)&command,1,10);
	ADC_X_CS_HIGH;  // ����CS����
}

// �ж������Ƿ�Ϊ����
int isNegative(int32_t data) {
    return (data < 0);
}


// ����Ϊ��ѹֵ
float convertToVoltage(int32_t data) {
    uint32_t N = 0xFFFFFFFF + 1;  // 2^32
    if (isNegative(data)) {
        return -((float)(N - 1 - (uint32_t)data) / (N - 1)) * FULL_SCALE_VOLTAGE;
    } else {
        return ((float)data / (N - 1)) * FULL_SCALE_VOLTAGE;
    }
}