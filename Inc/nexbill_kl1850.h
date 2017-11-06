#ifndef NEXBILL_KL1850_H_
#define NEXBILL_KL1850_H_

#include "bill_counter.h"

#define BC_KL1850_STX         0x3e
#define BC_KL1850_FRAME_SIZE  11

typedef struct
{
    const   BILL_COUNTER_INFO*  pInfo;
    uint32_t        ulCount;
}   BC_KL1850;

BILL_COUNTER*   BC_KL1850_create(const BILL_COUNTER_INFO* pInfo);
bool            BC_KL1850_input(BILL_COUNTER *pBC, SERIAL_HANDLE   hSerial);
uint32_t        BC_KL1850_getMessage(BILL_COUNTER* pBC, uint8_t* pBuffer, uint32_t ulBufferLength);

#endif