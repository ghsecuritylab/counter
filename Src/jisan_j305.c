#include "FreeRTOS.h"
#include "ret_value.h"
#include "assert.h"
#include "jisan_j305.h"
#include "trace.h"

static  BC_J305 _J305;

BILL_COUNTER*   BC_J305_create(const BILL_COUNTER_INFO* pInfo)
{
    _J305.pInfo = pInfo;
    
    return  (BILL_COUNTER*)&_J305;
}

bool    BC_J305_input(BILL_COUNTER *pBC, SERIAL_HANDLE   hSerial)
{
    ASSERT(pBC->pInfo->xModel == BC_MODEL_J305);

    uint32_t        i;
    BC_J305*        pJ305 = (BC_J305 *)pBC;
    uint32_t        ulReadLength = 0;
    static  uint8_t pFrame[BC_J305_FRAME_SIZE*2];
    int32_t         nStartIndex = -1;

    ulReadLength = SERIAL_gets(hSerial, (char *)&pFrame[ulReadLength], sizeof(pFrame), 10);
    if (ulReadLength != sizeof(pFrame))
    {
        return  false;
    }
    
    for(uint32_t i = 0 ; i < BC_J305_FRAME_SIZE ; i++)
    {
        if ((pFrame[i] == BC_J305_STX) && (pFrame[i + BC_J305_FRAME_SIZE - 1] != BC_J305_STX) && (pFrame[i + BC_J305_FRAME_SIZE] == BC_J305_STX))
        {
            nStartIndex = i;
            break;
        }
    }    

    if (nStartIndex < 0)
    {
        return  false;
    }
    
    TRACE_printf("Read[%2d] - ", BC_J305_FRAME_SIZE);
    TRACE_printDump((uint8_t *)&pFrame[nStartIndex], BC_J305_FRAME_SIZE, 0);
 
    pJ305->ulCount = 0;
    
    for(i = 0 ; i < 4 ; i++)
    {
        uint8_t nValue = 0;
        
        if (pFrame[i] < 8)
        {
            nValue = pFrame[i];
        }
        else if (pFrame[i] == 0x18)
        {
            nValue = 8;
        }
        else if (pFrame[i] == 0x19)
        {
            nValue = 9;
        }
        else if (pFrame[i] == 0x45)
        {
            nValue = 0;
        }
        else
        {
            return  false;
        }
        
        pJ305->ulCount = (pJ305->ulCount << 8) + nValue;
    }
    
    return  true;
}
                          

uint32_t        BC_J305_getMessage(BILL_COUNTER* pBC, uint8_t* pBuffer, uint32_t ulBufferLength)
{

    ASSERT(ulBufferLength >= 7);

    uint32_t    ulMessageLen = 0;
    BC_J305 * pJ305 = (BC_J305 *)pBC;
    
    pBuffer[ulMessageLen++] = 1;
    pBuffer[ulMessageLen++] = 0;
    pBuffer[ulMessageLen++] = 0;
    pBuffer[ulMessageLen++] = (pJ305->ulCount     ) & 0xFF;
    pBuffer[ulMessageLen++] = 0;
    pBuffer[ulMessageLen++] = 0;
    pBuffer[ulMessageLen++] = 0;
    
    return  ulMessageLen;
}
