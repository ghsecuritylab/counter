#ifndef PLUS_H_
#define PLUS_H_

#include "bill_counter.h"

#define BC_P506_STX         0x80
#define BC_P506_FRAME_SIZE  27

typedef struct
{
    const   BILL_COUNTER_INFO*  pInfo;
    uint32_t        xType;
    char            pDisplay[5];
    char            pSubdisplay[4];
    uint32_t        pCount[4];
}   BC_P506;

BILL_COUNTER*   BC_P506_create(const BILL_COUNTER_INFO* pInfo);
bool            BC_P506_input(BILL_COUNTER *pBC, SERIAL_HANDLE   hSerial);
uint32_t        BC_P506_getMessage(BILL_COUNTER* pBC, uint8_t* pBuffer, uint32_t ulBufferLength);

#endif