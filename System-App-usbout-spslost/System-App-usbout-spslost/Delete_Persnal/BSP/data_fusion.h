#ifndef __DATA_FUSION_H__
#define __DATA_FUSION_H__

#include "ads1285.h"
#include <stdint.h>
#include <stdbool.h>
#include "rtc.h"


/* �궨�� */
#define STARTUP_TIMEOUT_MS  (2 * 60 * 1000)  // 5����  �����궨�壬�������滻������ռ���ڴ�

// ======= ö�ٶ��� ===========

typedef enum {
    SYSTEM_USB_CONNECTED,
    SYSTEM_NORMAL_OPERATION
} SystemState;


typedef enum {
    STATE_IDLE,
    STATE_GNSS_RECEIVED,
    STATE_TIMESYNC,
    STATE_ACQUIRE,
} AppState;

extern AppState appState;  // ֻ������������
extern SystemState systemState;


#define SAMPLES_PER_BUFFER  8000
typedef struct {
    int32_t data[SAMPLES_PER_BUFFER];
    volatile bool isFull;       // ����������
    volatile bool isLocked;     // ��������ռ��
    uint32_t sampleCount;
} DataBuffer;

#define BUFFER_COUNT        3       // ���ػ���
typedef struct {
    DataBuffer buffers[BUFFER_COUNT];
    uint8_t activeIndex;       // ��ǰ�ɼ�����������
    uint8_t writeIndex;        // ��ǰд�뻺��������
    uint32_t totalSamples;
    uint32_t lostSamples;
    bool sdCardBusy;
} BufferSystem;




/* ���ͱ��� */
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;


/* �ⲿģ���� */
extern ADS1285_HandleTypeDef hads1285;



/* ���Ĺ��ܺ��� */
void CheckAndCreateNewFileIfNeeded(void);
void ProcessSubStateMachine(uint32_t currentTick);
void SetRTCFromConfigIfNeeded(void);
void BufferSystem_Init(void);
void FlushBufferToSD(uint8_t bufferIndex);
void ADS1285_Test(void);
void ProcessBuffers(void);
void MainLoop_StateMachine(void);
void CloseAllFilesIfAny(void);
bool SafeCloseCurrentFile(void);

/* ���ߺ��� */
void Soft_Reset(void);

#endif 
