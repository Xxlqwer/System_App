#include "ads1285.h"
#include "stdio.h"
#include "rtc.h"
#include "fatfs.h"  // Ϊ�˷��� g_config.sample_rate
ADS1285_HandleTypeDef hads1285;// ADS1285 �豸���

uint8_t ads1285_rx_buffer[ADS1285_DATA_LENGTH] = {0}; // �洢�� ADS1285 ���յ������ݵĻ�����
extern ConfigSettings g_config;  // ���߱��������������ڱ�

/**
 * @brief ��ʼ�� ADS1285 ADC �豸
 * @param hadc ָ�� ADS1285 �豸�����ָ��
 * @return HAL_OK ��ʼ���ɹ�
 * @return HAL_ERROR ��ʼ��ʧ��
 */
HAL_StatusTypeDef ADS1285_Init(ADS1285_HandleTypeDef *hadc) 
{    
    hadc->hspi = &hspi4;               // ѡ�� SPI4 ����ͨ��
    hadc->cs_port = GPIOE;             // Ƭѡ��CS��GPIO �˿�
    hadc->cs_pin = GPIO_PIN_11;        // Ƭѡ��CS��GPIO ����
    hadc->drdy_port = GPIOD;           // ���ݾ�����DRDY��GPIO �˿�
    hadc->drdy_pin = GPIO_PIN_12;      // ���ݾ�����DRDY��GPIO ����
	hadc->sync_port = GPIOD;           // ͬ��GPIO �˿�
	hadc->sync_pin  = GPIO_PIN_13;     // ͬ��GPIO ����

    // ��ʼ�� RESET ���� (GPIOD 14)
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_14; 
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP; // �������ģʽ
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    HAL_GPIO_WritePin(hadc->cs_port, hadc->cs_pin, GPIO_PIN_SET); // ��ʼ�� CS ���ţ�Ĭ�ϸߵ�ƽ��δѡ�У�

	GPIO_InitStruct.Pin = hadc->sync_pin; 
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP; // �������ģʽ
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(hadc->sync_port, &GPIO_InitStruct);

    HAL_GPIO_WritePin(hadc->sync_port, hadc->sync_pin, GPIO_PIN_RESET);
		
    // �޸��������DRDYΪ�ⲿ�жϣ��½��ش���
    GPIO_InitStruct.Pin = hadc->drdy_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING; // ��Ϊ�½����ж�ģʽ
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(hadc->drdy_port, &GPIO_InitStruct);
    
    // ����DRDY��Ӧ��EXTI�ж���
    // ����DRDY��PD12����ӦEXTI12
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
   
    return HAL_OK;
}

/*
*********************************************************************************************************
*   �� �� ��: ADS1285_ResetHardware
*   ����˵��: Ӳ����λADS1285
*   ��    ��: hadc - ADS1285�豸���ָ��
*   �� �� ֵ: HAL״̬��
*********************************************************************************************************
*/
HAL_StatusTypeDef ADS1285_ResetHardware(ADS1285_HandleTypeDef *hadc)
{
    /* ��λʱ��: �͵�ƽ����4��tCLK���� */
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_RESET);
    HAL_Delay(1);  
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET);
    HAL_Delay(1); /* �ȴ�оƬ�ȶ� */
	
		return Wait_DRDY();
}

/**
 * @brief ��ADS1285�����ڲ�ƫ��У׼��Offset Calibration��������У׼��Gain Calibration��
 * @param hadc ADS1285�豸���
 * @retval HAL_OK У׼�ɹ�
 * @retval HAL_ERROR У׼ʧ��
 */
HAL_StatusTypeDef ADS1285_Calibration(ADS1285_HandleTypeDef *hadc)
{
    uint8_t ConfigData[5] = {0x00};

    // -------- ����1����������̽ӣ�MUX����Ϊ�̽ӣ� --------
    ConfigData[0] = ADS1285_CMD_WREG | 0x02;    // WREG�����ʼ��ַ0x02 (MUX�Ĵ���)
    ConfigData[1] = 0x00;                       // д��Ĵ������� - 1����ֻдһ���Ĵ���
    ConfigData[2] = 0x48;                       // ����MUX=0x48���ڲ���·ģʽ��PGAǰ������̽ӣ�

    HAL_GPIO_WritePin(hadc->cs_port, hadc->cs_pin, GPIO_PIN_RESET); // ����CS��ʼSPIͨ��
    if (HAL_SPI_Transmit(hadc->hspi, ConfigData, 3, ADS1285_TIMEOUT) != HAL_OK) 
    {
        HAL_GPIO_WritePin(hadc->cs_port, hadc->cs_pin, GPIO_PIN_SET); // ʧ��ʱ����CS
        return HAL_ERROR;
    }
    HAL_GPIO_WritePin(hadc->cs_port, hadc->cs_pin, GPIO_PIN_SET); // ͨ����ɣ�����CS
    HAL_Delay(2); // �ȴ��ڲ��ȶ�

    // -------- ����2������ƫ��У׼��Offset Self-Calibration�� ��Ӧ�ĵ�8.4.6.1�� --------
    ConfigData[0] = ADS1285_CMD_OFSCAL;      // ƫ��У׼����

    HAL_GPIO_WritePin(hadc->cs_port, hadc->cs_pin, GPIO_PIN_RESET);
    if (HAL_SPI_Transmit(hadc->hspi, ConfigData, 1, ADS1285_TIMEOUT) != HAL_OK) 
    {
        HAL_GPIO_WritePin(hadc->cs_port, hadc->cs_pin, GPIO_PIN_SET);
        return HAL_ERROR;
    }
    HAL_GPIO_WritePin(hadc->cs_port, hadc->cs_pin, GPIO_PIN_SET);
    // HAL_Delay(2); // ƫ��У׼��Ҫ����ʱ�����
    Wait_DRDY();

    // -------- ����3���ٴ���������̽ӣ���֤һ���ԣ� �ĵ�8.4.6.3 --------
    ConfigData[0] = ADS1285_CMD_WREG | 2;
    ConfigData[1] = 0x00;
    ConfigData[2] = 0x48; // �ٴ�����MUX�̽�ģʽ

    HAL_GPIO_WritePin(hadc->cs_port, hadc->cs_pin, GPIO_PIN_RESET);
    if (HAL_SPI_Transmit(hadc->hspi, ConfigData, 3, ADS1285_TIMEOUT) != HAL_OK) 
    {
        HAL_GPIO_WritePin(hadc->cs_port, hadc->cs_pin, GPIO_PIN_SET);
        return HAL_ERROR;
    }
    HAL_GPIO_WritePin(hadc->cs_port, hadc->cs_pin, GPIO_PIN_SET);
    HAL_Delay(2);

    // -------- ����4����������У׼��Gain Self-Calibration��  ��Ӧ�ĵ�8.4.6.2��--------
    ConfigData[0] = ADS1285_CMD_GANCAL;      // ����У׼����

    HAL_GPIO_WritePin(hadc->cs_port, hadc->cs_pin, GPIO_PIN_RESET);
    if (HAL_SPI_Transmit(hadc->hspi, ConfigData, 1, ADS1285_TIMEOUT) != HAL_OK) 
    {
        HAL_GPIO_WritePin(hadc->cs_port, hadc->cs_pin, GPIO_PIN_SET);
        return HAL_ERROR;
    }
    HAL_GPIO_WritePin(hadc->cs_port, hadc->cs_pin, GPIO_PIN_SET);
    Wait_DRDY();

    // -------- У׼��� --------
    return ADS1285_WriteReg_Continuous(hadc);
}

// ������װ���̣���ʼ�� + RESET + У׼ + ����
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
	// ������������ڴ洢�Ĵ�����ֵ
	uint8_t ConfigData[5] = {0x00};
     
    ConfigData[0] = ADS1285_CMD_WREG | 0; //��ʼ��ַ0
	ConfigData[1] = 2;    //��д��2��
	ConfigData[2] = 0x00; //��ַ0x00			
	
	 //125SPS
	//ConfigData[3] = 0x02;	 // High	250 SPS	Linear	FIR	00000010	0X2
	
	//2000SPS
	//ConfigData[3] = 0x22;    //High	4000 SPS	Linear	FIR	00100010	0X22  ʵ��2000SPS
	
	 //125SPS
	// ConfigData[3] = 0x02;	 // High	250 SPS	Linear	FIR	00000010	0X2
 
	//250SPS
 //ConfigData[3] = 0x0A;   //High	500 SPS	Linear	FIR	00001010	0XA


	//500SPS
 //ConfigData[3] = 0x02;	      //High	1000 SPS	Linear	FIR	00010010	0X12
	
	//1000SPS
	//ConfigData[3] = 0x1A;  //High	2000 SPS	Linear	FIR	00011010	0X1A ʵ��1000SPS
	
	//2000SPS
	//ConfigData[3] = 0x22;    //High	4000 SPS	Linear	FIR	00100010	0X22  ʵ��2000SPS
	 // ���ݲ��������� ConfigData[3]
    switch (g_config.sample_rate)
    {
        case 125:
            ConfigData[3] = 0x02; // ʵ��125 SPS
            break;
        case 250:
            ConfigData[3] = 0x0A; // ʵ��250 SPS
            break;
        case 1000:
            ConfigData[3] = 0x1A; // ʵ��1000 SPS
            break;
        case 2000:
            ConfigData[3] = 0x22; // ʵ��2000 SPS
            break;
        default:
            ConfigData[3] = 0x22; // Ĭ�� fallback��2000 SPS
            printf("����: ��֧�ֵĲ����� %u��Ĭ��ʹ�� 2000 SPS\r\n", g_config.sample_rate);
            break;
    }
	
	
	ConfigData[4] = 0x0F; //��ַ0x02 000_01_111; ͨ�� 1  �ο���ѹ 4.096V��BufferMode���� 1
    // ����Ƭѡ�źţ���ʼ�� ADS1285 ͨ��
    HAL_GPIO_WritePin(hadc->cs_port, hadc->cs_pin, GPIO_PIN_RESET);
	// ͨ�� SPI ����д�����������ʧ���򷵻ش���
    if (HAL_SPI_Transmit(hadc->hspi, ConfigData, 5, ADS1285_TIMEOUT) != HAL_OK) 
	{
        HAL_GPIO_WritePin(hadc->cs_port, hadc->cs_pin, GPIO_PIN_SET);
        return HAL_ERROR;
    }
    HAL_GPIO_WritePin(hadc->cs_port, hadc->cs_pin, GPIO_PIN_SET);

		return HAL_OK; 
}

/**
 * @brief ��ȡADS1285ԭʼ��32λ���ݣ������е�ѹ����
 * @param hadc ��������ADC���
 * @param data �洢ԭʼ���ݵ�ָ��
 * @return HAL_OK: �ɹ���HAL_ERROR: ʧ��
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
 * @brief �ȴ� DRDY ���ű�ͣ���ʾ���ݾ���
 * @retval HAL_OK �ȴ��ɹ�
 * @retval HAL_ERROR �ȴ���ʱ
 */
HAL_StatusTypeDef Wait_DRDY(void) 
{
    uint32_t timeout = 1000000; // ���ó�ʱ����

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
 * @brief ��ȡ ADC ת�����
 * @param adc_value �洢ת����� 32 λ ADC ֵ
 * @retval HAL_OK ��ȡ�ɹ�
 * @retval HAL_ERROR ��ȡʧ��
 */
HAL_StatusTypeDef Read_Data(int32_t *adc_value) 
{
    uint8_t cmd = ADS1285_CMD_RDATA; // ��ȡ ADC ת���������
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

    // ��� 4 �ֽ�����Ϊ 32 λ�з�������
    *adc_value = (rxBuffer[0] << 24) | (rxBuffer[1] << 16) | (rxBuffer[2] << 8) | rxBuffer[3];

    return HAL_OK;
}

// ����Ӳ����У׼����
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