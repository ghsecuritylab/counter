#ifndef JISAN_J305_H_
#define JISAN_J305_H_

#include "bill_counter.h"

#define BC_J305_STX         0x45
#define BC_J305_FRAME_SIZE  13

typedef struct
{
    const   BILL_COUNTER_INFO*  pInfo;
    uint32_t        ulCount;
}   BC_J305;

BILL_COUNTER*   BC_J305_create(const BILL_COUNTER_INFO* pInfo);
bool            BC_J305_input(BILL_COUNTER *pBC, SERIAL_HANDLE   hSerial);
RET_VALUE       BC_J305_parser(BILL_COUNTER *pBC, uint8_t *pFrame, uint32_t ulFrameLength);
uint32_t        BC_J305_getMessage(BILL_COUNTER* pBC, uint8_t* pBuffer, uint32_t ulBufferLength);

#endif