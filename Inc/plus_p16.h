#ifndef PLUS_P16_H_
#define PLUS_P16_H_

#include "bill_counter.h"

#define BC_P16_STX_1            0x7a
#define BC_P16_STX_2            0x7d
#define BC_P16_FRAME_SIZE_MIN   5
#define BC_P16_FRAME_SIZE_MAX   6
#define BC_P16_FRAME_SIZE_1     5
#define BC_P16_FRAME_SIZE_2     6

typedef struct
{
    const   BILL_COUNTER_INFO*  pInfo;
    uint32_t        xType;
    uint32_t        pCount[5];
}   BC_P16;

BILL_COUNTER*   BC_P16_create(const BILL_COUNTER_INFO* pInfo);
bool            BC_P16_input(BILL_COUNTER *pBC, SERIAL_HANDLE   hSerial);
uint32_t        BC_P16_getMessage(BILL_COUNTER* pBC, uint8_t* pBuffer, uint32_t ulBufferLength);

#endif