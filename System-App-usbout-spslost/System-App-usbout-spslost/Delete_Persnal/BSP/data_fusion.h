#ifndef __DATA_FUSION_H__
#define __DATA_FUSION_H__

#include "ads1285.h"
#include <stdint.h>
#include <stdbool.h>
#include "rtc.h"


/* 宏定义 */
#define STARTUP_TIMEOUT_MS  (2 * 60 * 1000)  // 5分钟  常量宏定义，编译期替换，不会占用内存

// ======= 枚举定义 ===========

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

extern AppState appState;  // 只声明，不定义
extern SystemState systemState;


#define SAMPLES_PER_BUFFER  8000
typedef struct {
    int32_t data[SAMPLES_PER_BUFFER];
    volatile bool isFull;       // 缓冲区已满
    volatile bool isLocked;     // 缓冲区被占用
    uint32_t sampleCount;
} DataBuffer;

#define BUFFER_COUNT        3       // 三重缓冲
typedef struct {
    DataBuffer buffers[BUFFER_COUNT];
    uint8_t activeIndex;       // 当前采集缓冲区索引
    uint8_t writeIndex;        // 当前写入缓冲区索引
    uint32_t totalSamples;
    uint32_t lostSamples;
    bool sdCardBusy;
} BufferSystem;




/* 类型别名 */
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;


/* 外部模块句柄 */
extern ADS1285_HandleTypeDef hads1285;



/* 核心功能函数 */
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

/* 工具函数 */
void Soft_Reset(void);

#endif 
