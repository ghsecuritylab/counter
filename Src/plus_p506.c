#include "FreeRTOS.h"
#include "ret_value.h"
#include "plus_p506.h"
#include "assert.h"
#include "trace.h"

static  BC_P506 _P506;

BILL_COUNTER*   BC_P506_create(const BILL_COUNTER_INFO* pInfo)
{
    _P506.pInfo = pInfo;
    
    return  (BILL_COUNTER*)&_P506;
}

bool    BC_P506_input(BILL_COUNTER *pBC, SERIAL_HANDLE   hSerial)
{
    uint32_t        ulReadLength = 0;
    static  uint8_t pFrame[BC_P506_FRAME_SIZE];
    
    if (pBC->pInfo->xModel != BC_MODEL_P506)
    {
        return  RET_INVALID_ARGUMENT;
    }

    BC_P506 * pP506 = (BC_P506 *)pBC;
    
    if ((SERIAL_getc(hSerial, (char *)&pFrame[0], 10) != RET_OK) || (pFrame[0] != BC_P506_STX))
    {
        return  false;
    }
    ulReadLength = 1;
    
    ulReadLength += SERIAL_gets(hSerial, (char *)&pFrame[1], sizeof(pFrame) - 1, 100);
    if (ulReadLength != sizeof(pFrame))
    {
        return  false;
    }
    
    TRACE_printf("Read[%2d] - ", ulReadLength);
    TRACE_printDump((uint8_t *)pFrame, ulReadLength, 0);
 
    uint32_t    i;
    
    pP506->xType = pFrame[3];
    
    for(i = 0 ; i < 4 ; i++)
    {
        if (pFrame[6+i] == 0x25)
        {
            pP506->pDisplay[i] = ' ';
        }
        else if (pFrame[6+i] < 10)
        {
            pP506->pDisplay[i] = '0' + pFrame[6+i];
        }
        else
        {
            pP506->pDisplay[i] = '?';
        }
    }
    pP506->pDisplay[4] = '\0';
    
    for(i = 0 ; i < 3 ; i++)
    {
        if (pFrame[10+i] == 0x25)
        {
            pP506->pDisplay[i] = ' ';
        }
        else if (pFrame[10+i] < 10)
        {
            pP506->pDisplay[i] = '0' + pFrame[10+i];
        }
        else
        {
            pP506->pDisplay[i] = '?';
        }
    }
    pP506->pDisplay[3] = '\0';
    
    for(i = 0 ; i < 4 ; i++)
    {
        pP506->pCount[i] = pFrame[19 + (i*2)];
    }

    return  true;
}
                          
uint32_t        BC_P506_getMessage(BILL_COUNTER* pBC, uint8_t* pBuffer, uint32_t ulBufferLength)
{
    ASSERT(ulBufferLength >= 15);

    uint32_t    ulMessageLen = 0;
    uint32_t    ulTotal = 0;
    BC_P506 * pP506 = (BC_P506 *)pBC;
        
    pBuffer[ulMessageLen++] = 1;
    for(uint32_t i = 0 ; i < 4 ; i++)
    { 
        ulTotal += pP506->pCount[i];
    }

    pBuffer[ulMessageLen++] = (ulTotal >> 8) & 0xFF;
    pBuffer[ulMessageLen++] = (ulTotal     ) & 0xFF;
    
    for(uint32_t i = 0 ; i < 4 ; i++)
    {
        pBuffer[ulMessageLen++] = i+1;
        pBuffer[ulMessageLen++] = (pP506->pCount[i] >> 8) & 0xFF;
        pBuffer[ulMessageLen++] = (pP506->pCount[i]     ) & 0xFF;
    }
    
    return  ulMessageLen;
}
