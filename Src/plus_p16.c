#include "FreeRTOS.h"
#include "ret_value.h"
#include "assert.h"
#include "plus_p16.h"
#include "trace.h"
#include "assert.h"

static  BC_P16 _P16;

BILL_COUNTER*   BC_P16_create(const BILL_COUNTER_INFO* pInfo)
{
    _P16.pInfo = pInfo;
    
    return  (BILL_COUNTER*)&_P16;
}

bool    BC_P16_input(BILL_COUNTER *pBC, SERIAL_HANDLE   hSerial)
{
    uint32_t        ulReadLength = 0;
    static  uint8_t pFrame[BC_P16_FRAME_SIZE_MAX];
    BC_P16*         pP16 =  (BC_P16*)pBC;
    
    if (pBC->pInfo->xModel != BC_MODEL_P16)
    {
        return  RET_INVALID_ARGUMENT;
    }
    
    if (SERIAL_getc(hSerial, (char *)&pFrame[0], 10) != RET_OK)
    {
        return  false;
    }
    ulReadLength = 1;
    
    if (pFrame[0] == BC_P16_STX_1)
    {
        ulReadLength += SERIAL_gets(hSerial, (char *)&pFrame[1], BC_P16_FRAME_SIZE_1 - 1, 100);
        if (ulReadLength != BC_P16_FRAME_SIZE_1)
        {
            return  false;
        }
    }
    else if (pFrame[0] == BC_P16_STX_2)
    {
        ulReadLength += SERIAL_gets(hSerial, (char *)&pFrame[1], BC_P16_FRAME_SIZE_2 - 1, 100);
        if (ulReadLength != BC_P16_FRAME_SIZE_2)
        {
            return  false;
        }
    }
    else
    {
        return  false;
    }
    
    uint8_t nCRC = 0;
    uint32_t    i;
    
    for(i = 1 ; i < ulReadLength - 1; i++)
    {
        nCRC += pFrame[i];
    }
    
    if ((nCRC & 0x7F) != pFrame[ulReadLength - 1])
    {
        return  false;
    }

    if (pFrame[0] == BC_P16_STX_1)
    {
        switch(pFrame[1])
        {
        case    0x00:
        case    0x01:
        case    0x02:
        case    0x03:
        case    0x04:
            pP16->pCount[pFrame[1]] = ((uint16_t)pFrame[2] << 8) | pFrame[3];
            break;
        case    0x71:
            for(i = 0 ; i < 5 ; i++)
            {
                pP16->pCount[i] = 0;
            }
            break;
        }
        
    }
    else if (pFrame[0] == BC_P16_STX_2)
    {
        TRACE_printf("%3d %3d %3d %3d %3d\n", pP16->pCount[0], pP16->pCount[1], pP16->pCount[2], pP16->pCount[3], pP16->pCount[4]);
        return  true;
    }
    
    return  false;
}

uint32_t        BC_P16_getMessage(BILL_COUNTER* pBC, uint8_t* pBuffer, uint32_t ulBufferLength)
{
    ASSERT(ulBufferLength >= 15);
    
    uint32_t    ulMessageLen = 0;
    BC_P16 * pP16 = (BC_P16 *)pBC;

    pBuffer[ulMessageLen++] = 1;
    pBuffer[ulMessageLen++] = (pP16->pCount[0] >> 8) & 0xFF;
    pBuffer[ulMessageLen++] = (pP16->pCount[0]     ) & 0xFF;
    for(uint32_t i = 1 ; i < 5 ; i++)
    {
        pBuffer[ulMessageLen++] = i;
        pBuffer[ulMessageLen++] = (pP16->pCount[i] >> 8) & 0xFF;
        pBuffer[ulMessageLen++] = (pP16->pCount[i]     ) & 0xFF;
    }
    
    return  ulMessageLen;
}
