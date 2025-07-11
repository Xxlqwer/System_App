#include "ads1285.h"
#include "stdio.h"
#include "rtc.h"
#include "fatfs.h"  // 为了访问 g_config.sample_rate
ADS1285_HandleTypeDef hads1285;// ADS1285 设备句柄

uint8_t ads1285_rx_buffer[ADS1285_DATA_LENGTH] = {0}; // 存储从 ADS1285 接收到的数据的缓冲区
extern ConfigSettings g_config;  // 告诉编译器变量定义在别处

/**
 * @brief 初始化 ADS1285 ADC 设备
 * @param hadc 指向 ADS1285 设备句柄的指针
 * @return HAL_OK 初始化成功
 * @return HAL_ERROR 初始化失败
 */
HAL_StatusTypeDef ADS1285_Init(ADS1285_HandleTypeDef *hadc) 
{    
    hadc->hspi = &hspi4;               // 选择 SPI4 进行通信
    hadc->cs_port = GPIOE;             // 片选（CS）GPIO 端口
    hadc->cs_pin = GPIO_PIN_11;        // 片选（CS）GPIO 引脚
    hadc->drdy_port = GPIOD;           // 数据就绪（DRDY）GPIO 端口
    hadc->drdy_pin = GPIO_PIN_12;      // 数据就绪（DRDY）GPIO 引脚
	hadc->sync_port = GPIOD;           // 同步GPIO 端口
	hadc->sync_pin  = GPIO_PIN_13;     // 同步GPIO 引脚

    // 初始化 RESET 引脚 (GPIOD 14)
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_14; 
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP; // 推挽输出模式
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    HAL_GPIO_WritePin(hadc->cs_port, hadc->cs_pin, GPIO_PIN_SET); // 初始化 CS 引脚，默认高电平（未选中）

	GPIO_InitStruct.Pin = hadc->sync_pin; 
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP; // 推挽输出模式
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(hadc->sync_port, &GPIO_InitStruct);

    HAL_GPIO_WritePin(hadc->sync_port, hadc->sync_pin, GPIO_PIN_RESET);
		
    // 修改这里：配置DRDY为外部中断，下降沿触发
    GPIO_InitStruct.Pin = hadc->drdy_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING; // 改为下降沿中断模式
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(hadc->drdy_port, &GPIO_InitStruct);
    
    // 启用DRDY对应的EXTI中断线
    // 假设DRDY在PD12，对应EXTI12
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
   
    return HAL_OK;
}

/*
*********************************************************************************************************
*   函 数 名: ADS1285_ResetHardware
*   功能说明: 硬件复位ADS1285
*   形    参: hadc - ADS1285设备句柄指针
*   返 回 值: HAL状态码
*********************************************************************************************************
*/
HAL_StatusTypeDef ADS1285_ResetHardware(ADS1285_HandleTypeDef *hadc)
{
    /* 复位时序: 低电平至少4个tCLK周期 */
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_RESET);
    HAL_Delay(1);  
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET);
    HAL_Delay(1); /* 等待芯片稳定 */
	
		return Wait_DRDY();
}

/**
 * @brief 对ADS1285进行内部偏移校准（Offset Calibration）和增益校准（Gain Calibration）
 * @param hadc ADS1285设备句柄
 * @retval HAL_OK 校准成功
 * @retval HAL_ERROR 校准失败
 */
HAL_StatusTypeDef ADS1285_Calibration(ADS1285_HandleTypeDef *hadc)
{
    uint8_t ConfigData[5] = {0x00};

    // -------- 步骤1：配置输入短接（MUX设置为短接） --------
    ConfigData[0] = ADS1285_CMD_WREG | 0x02;    // WREG命令，起始地址0x02 (MUX寄存器)
    ConfigData[1] = 0x00;                       // 写入寄存器数量 - 1，即只写一个寄存器
    ConfigData[2] = 0x48;                       // 配置MUX=0x48，内部短路模式（PGA前的输入短接）

    HAL_GPIO_WritePin(hadc->cs_port, hadc->cs_pin, GPIO_PIN_RESET); // 拉低CS开始SPI通信
    if (HAL_SPI_Transmit(hadc->hspi, ConfigData, 3, ADS1285_TIMEOUT) != HAL_OK) 
    {
        HAL_GPIO_WritePin(hadc->cs_port, hadc->cs_pin, GPIO_PIN_SET); // 失败时拉高CS
        return HAL_ERROR;
    }
    HAL_GPIO_WritePin(hadc->cs_port, hadc->cs_pin, GPIO_PIN_SET); // 通信完成，拉高CS
    HAL_Delay(2); // 等待内部稳定

    // -------- 步骤2：启动偏移校准（Offset Self-Calibration） 对应文档8.4.6.1节 --------
    ConfigData[0] = ADS1285_CMD_OFSCAL;      // 偏移校准命令

    HAL_GPIO_WritePin(hadc->cs_port, hadc->cs_pin, GPIO_PIN_RESET);
    if (HAL_SPI_Transmit(hadc->hspi, ConfigData, 1, ADS1285_TIMEOUT) != HAL_OK) 
    {
        HAL_GPIO_WritePin(hadc->cs_port, hadc->cs_pin, GPIO_PIN_SET);
        return HAL_ERROR;
    }
    HAL_GPIO_WritePin(hadc->cs_port, hadc->cs_pin, GPIO_PIN_SET);
    // HAL_Delay(2); // 偏移校准需要短暂时间完成
    Wait_DRDY();

    // -------- 步骤3：再次配置输入短接（保证一致性） 文档8.4.6.3 --------
    ConfigData[0] = ADS1285_CMD_WREG | 2;
    ConfigData[1] = 0x00;
    ConfigData[2] = 0x48; // 再次设置MUX短接模式

    HAL_GPIO_WritePin(hadc->cs_port, hadc->cs_pin, GPIO_PIN_RESET);
    if (HAL_SPI_Transmit(hadc->hspi, ConfigData, 3, ADS1285_TIMEOUT) != HAL_OK) 
    {
        HAL_GPIO_WritePin(hadc->cs_port, hadc->cs_pin, GPIO_PIN_SET);
        return HAL_ERROR;
    }
    HAL_GPIO_WritePin(hadc->cs_port, hadc->cs_pin, GPIO_PIN_SET);
    HAL_Delay(2);

    // -------- 步骤4：启动增益校准（Gain Self-Calibration）  对应文档8.4.6.2节--------
    ConfigData[0] = ADS1285_CMD_GANCAL;      // 增益校准命令

    HAL_GPIO_WritePin(hadc->cs_port, hadc->cs_pin, GPIO_PIN_RESET);
    if (HAL_SPI_Transmit(hadc->hspi, ConfigData, 1, ADS1285_TIMEOUT) != HAL_OK) 
    {
        HAL_GPIO_WritePin(hadc->cs_port, hadc->cs_pin, GPIO_PIN_SET);
        return HAL_ERROR;
    }
    HAL_GPIO_WritePin(hadc->cs_port, hadc->cs_pin, GPIO_PIN_SET);
    Wait_DRDY();

    // -------- 校准完成 --------
    return ADS1285_WriteReg_Continuous(hadc);
}

// 启动封装流程：初始化 + RESET + 校准 + 配置
HAL_StatusTypeDef ADS1285_StartupSequence(ADS1285_HandleTypeDef *hadc) 
{
    if (ADS1285_Init(hadc) != HAL_OK) return HAL_ERROR;
    if (ADS1285_ResetHardware(hadc) != HAL_OK) return HAL_ERROR;
    if (ADS1285_Calibration(hadc) != HAL_OK) return HAL_ERROR;
    return HAL_OK;
}

HAL_StatusTypeDef ADS1285_ReadRawBytes(ADS1285_HandleTypeDef *hadc, uint8_t data[4]) 
{
		
		while (HAL_GPIO_ReadPin(hadc->drdy_port, hadc->drdy_pin) == GPIO_PIN_SET);
		HAL_GPIO_WritePin(hadc->cs_port, hadc->cs_pin, GPIO_PIN_RESET);
	
		if (HAL_SPI_Receive(hadc->hspi, data, 4, ADS1285_TIMEOUT) != HAL_OK) 
        {
			HAL_GPIO_WritePin(hadc->cs_port, hadc->cs_pin, GPIO_PIN_SET);
			return HAL_ERROR;
		}

		HAL_GPIO_WritePin(hadc->cs_port, hadc->cs_pin, GPIO_PIN_SET);
		return HAL_OK;
}

HAL_StatusTypeDef ADS1285_WriteReg_Continuous (ADS1285_HandleTypeDef *hadc)
{
	// 定义变量，用于存储寄存器的值
	uint8_t ConfigData[5] = {0x00};
     
    ConfigData[0] = ADS1285_CMD_WREG | 0; //起始地址0
	ConfigData[1] = 2;    //再写入2个
	ConfigData[2] = 0x00; //地址0x00			
	
	 //125SPS
	//ConfigData[3] = 0x02;	 // High	250 SPS	Linear	FIR	00000010	0X2
	
	//2000SPS
	//ConfigData[3] = 0x22;    //High	4000 SPS	Linear	FIR	00100010	0X22  实测2000SPS
	
	 //125SPS
	// ConfigData[3] = 0x02;	 // High	250 SPS	Linear	FIR	00000010	0X2
 
	//250SPS
 //ConfigData[3] = 0x0A;   //High	500 SPS	Linear	FIR	00001010	0XA


	//500SPS
 //ConfigData[3] = 0x02;	      //High	1000 SPS	Linear	FIR	00010010	0X12
	
	//1000SPS
	//ConfigData[3] = 0x1A;  //High	2000 SPS	Linear	FIR	00011010	0X1A 实测1000SPS
	
	//2000SPS
	//ConfigData[3] = 0x22;    //High	4000 SPS	Linear	FIR	00100010	0X22  实测2000SPS
	 // 根据采样率设置 ConfigData[3]
    switch (g_config.sample_rate)
    {
        case 125:
            ConfigData[3] = 0x02; // 实测125 SPS
            break;
        case 250:
            ConfigData[3] = 0x0A; // 实测250 SPS
            break;
        case 1000:
            ConfigData[3] = 0x1A; // 实测1000 SPS
            break;
        case 2000:
            ConfigData[3] = 0x22; // 实测2000 SPS
            break;
        default:
            ConfigData[3] = 0x22; // 默认 fallback：2000 SPS
            printf("警告: 不支持的采样率 %u，默认使用 2000 SPS\r\n", g_config.sample_rate);
            break;
    }
	
	
	ConfigData[4] = 0x0F; //地址0x02 000_01_111; 通道 1  参考电压 4.096V，BufferMode增益 1
    // 拉低片选信号，开始与 ADS1285 通信
    HAL_GPIO_WritePin(hadc->cs_port, hadc->cs_pin, GPIO_PIN_RESET);
	// 通过 SPI 发送写入命令，若发送失败则返回错误
    if (HAL_SPI_Transmit(hadc->hspi, ConfigData, 5, ADS1285_TIMEOUT) != HAL_OK) 
	{
        HAL_GPIO_WritePin(hadc->cs_port, hadc->cs_pin, GPIO_PIN_SET);
        return HAL_ERROR;
    }
    HAL_GPIO_WritePin(hadc->cs_port, hadc->cs_pin, GPIO_PIN_SET);

		return HAL_OK; 
}

/**
 * @brief 读取ADS1285原始的32位数据，不进行电压换算
 * @param hadc 传感器的ADC句柄
 * @param data 存储原始数据的指针
 * @return HAL_OK: 成功，HAL_ERROR: 失败
 */
HAL_StatusTypeDef ADS1285_ReadRawData(ADS1285_HandleTypeDef *hadc, int32_t *data)
{
    if (Wait_DRDY() != HAL_OK) return HAL_ERROR;

    uint8_t rx[4] = {0};
    HAL_GPIO_WritePin(hadc->cs_port, hadc->cs_pin, GPIO_PIN_RESET);
    if (HAL_SPI_Receive(hadc->hspi, rx, 4, ADS1285_TIMEOUT) != HAL_OK) 
    {
        HAL_GPIO_WritePin(hadc->cs_port, hadc->cs_pin, GPIO_PIN_SET);
        return HAL_ERROR;
    }
    HAL_GPIO_WritePin(hadc->cs_port, hadc->cs_pin, GPIO_PIN_SET);

    *data = (rx[0] << 24) | (rx[1] << 16) | (rx[2] << 8) | rx[3];
    return HAL_OK;
}


/**
 * @brief 等待 DRDY 引脚变低，表示数据就绪
 * @retval HAL_OK 等待成功
 * @retval HAL_ERROR 等待超时
 */
HAL_StatusTypeDef Wait_DRDY(void) 
{
    uint32_t timeout = 1000000; // 设置超时计数

    while (HAL_GPIO_ReadPin(hads1285.drdy_port, hads1285.drdy_pin) != GPIO_PIN_RESET) 
    {
        if (--timeout == 0) 
        {
            return HAL_ERROR;
        }
    }

    return HAL_OK;
}

/**
 * @brief 读取 ADC 转换结果
 * @param adc_value 存储转换后的 32 位 ADC 值
 * @retval HAL_OK 读取成功
 * @retval HAL_ERROR 读取失败
 */
HAL_StatusTypeDef Read_Data(int32_t *adc_value) 
{
    uint8_t cmd = ADS1285_CMD_RDATA; // 读取 ADC 转换结果命令
    uint8_t rxBuffer[4] = {0};

    HAL_GPIO_WritePin(hads1285.cs_port, hads1285.cs_pin, GPIO_PIN_RESET);
    if (HAL_SPI_Transmit(hads1285.hspi, &cmd, 1, ADS1285_TIMEOUT) != HAL_OK) 
    {
        HAL_GPIO_WritePin(hads1285.cs_port, hads1285.cs_pin, GPIO_PIN_SET);
        return HAL_ERROR;
    }

    if (HAL_SPI_Receive(hads1285.hspi, rxBuffer, 4, ADS1285_TIMEOUT) != HAL_OK) 
    {
        HAL_GPIO_WritePin(hads1285.cs_port, hads1285.cs_pin, GPIO_PIN_SET);
        return HAL_ERROR;
    }

    HAL_GPIO_WritePin(hads1285.cs_port, hads1285.cs_pin, GPIO_PIN_SET);

    // 组合 4 字节数据为 32 位有符号整数
    *adc_value = (rxBuffer[0] << 24) | (rxBuffer[1] << 16) | (rxBuffer[2] << 8) | rxBuffer[3];

    return HAL_OK;
}

// 重置硬件和校准函数
HAL_StatusTypeDef ADS1285_ResetAndCalibrate(ADS1285_HandleTypeDef *had)
{
    if (ADS1285_ResetHardware(had) != HAL_OK) 
    {
        return HAL_ERROR;
    }
    if (ADS1285_Calibration(had) != HAL_OK) 
    {
        return HAL_ERROR;
    }
    return HAL_OK;
}