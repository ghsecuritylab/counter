#ifndef BILL_COUNTER_H_
#define BILL_COUNTER_H_

#include "FreeRTOS.h"
#include "serial.h"
#include "stdbool.h"

#define BC_MODEL_106DD      1
#define BC_MODEL_329DD      1
#define BC_MODEL_P30        2
#define BC_MODEL_P624       4
#define BC_MODEL_P506       6
#define BC_MODEL_J305       8
#define BC_MODEL_P16        9
#define BC_MODEL_KL1850     10
#define BC_MODEL_BH1000     11


struct  _BILL_COUNTER_STRUCT;

typedef struct _BILL_COUNTER_INFO_STRUCT
{
    uint32_t            xModel;
    const char*         pName;
    uint8_t             xSTX;
    uint32_t            ulFrameSize;    
    SERIAL_BAUDRATE     xBaudrate;
    SERIAL_PARITY       xParity;
    SERIAL_DATA_BITS    xDataBits;
    SERIAL_STOP_BITS    xStopBits;
    
    struct _BILL_COUNTER_STRUCT*      (*fCreate)(const struct _BILL_COUNTER_INFO_STRUCT* pInfo);
    bool                (*fInput)(struct _BILL_COUNTER_STRUCT * pBC, SERIAL_HANDLE   hSerial);
    uint32_t            (*fGetMessage)(struct _BILL_COUNTER_STRUCT* pBC, uint8_t* pBuffer, uint32_t ulBufferLength);

}   BILL_COUNTER_INFO;

typedef struct  _BILL_COUNTER_STRUCT
{
    const   BILL_COUNTER_INFO*  pInfo;
}   BILL_COUNTER;

BILL_COUNTER*   BC_getInstance(uint32_t    ulModel);

uint32_t    BC_GetModelList(BILL_COUNTER_INFO *pCounters[], uint32_t ulMaxCount);

#endif