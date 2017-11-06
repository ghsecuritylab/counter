#ifndef JISAN_BH1000_H_
#define JISAN_BH1000_H_

#include "bill_counter.h"

#define BC_BH1000_STX         0x02
#define BC_BH1000_FRAME_SIZE  17

typedef struct
{
    const   BILL_COUNTER_INFO*  pInfo;
    uint32_t        ulCount;
    uint32_t        ulSum;
}   BC_BH1000;

BILL_COUNTER*   BC_BH1000_create(const BILL_COUNTER_INFO* pInfo);
bool            BC_BH1000_input(BILL_COUNTER *pBC, SERIAL_HANDLE   hSerial);
RET_VALUE       BC_BH1000_parser(BILL_COUNTER *pBC, uint8_t *pFrame, uint32_t ulFrameLength);
uint32_t        BC_BH1000_getMessage(BILL_COUNTER* pBC, uint8_t* pBuffer, uint32_t ulBufferLength);

#endif