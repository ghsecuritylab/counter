#include <string.h>
#include "FreeRTOS.h"
#include "ret_value.h"
#include "assert.h"
#include "nexbill_kl1850.h"
#include "trace.h"

static  BC_KL1850 _KL1850;

BILL_COUNTER*   BC_KL1850_create(const BILL_COUNTER_INFO* pInfo)
{
    _KL1850.pInfo = pInfo;
    
    return  (BILL_COUNTER*)&_KL1850;
}

bool    BC_KL1850_input(BILL_COUNTER *pBC, SERIAL_HANDLE   hSerial)
{
    ASSERT(pBC->pInfo->xModel == BC_MODEL_KL1850);

    BC_KL1850*        pKL1850 = (BC_KL1850 *)pBC;
    uint32_t        ulReadLength = 0;
    static  uint8_t pFrame[BC_KL1850_FRAME_SIZE];
    int32_t         nStartIndex = -1;

    while(1)
    {
        if (SERIAL_getc(hSerial, (char *)&pFrame[0], 1000) == RET_OK)
        {
            TRACE_printf("%c ", pFrame[0]);
        }
    }
    if (SERIAL_getc(hSerial, (char *)&pFrame[0], 10) != RET_OK)
    {
        return  false;
    }
    ulReadLength = 1;
    


    if (pFrame[0] != BC_KL1850_STX)
    {
        TRACE_printf("%02x ", pFrame[0]);
        return  false;
    }

    
    ulReadLength += SERIAL_gets(hSerial, (char *)&pFrame[ulReadLength], 4, 10);
    if ((ulReadLength != 5) || (strncmp((char *)pFrame, ">1SV4", 5) != 0))
    {
        return  false;
    }
    
    ulReadLength += SERIAL_gets(hSerial, (char *)&pFrame[ulReadLength], 6, 10);
    if (ulReadLength != BC_KL1850_FRAME_SIZE)
    {
        return  false;
    }
    
    TRACE_printf("Read[%2d] - ", BC_KL1850_FRAME_SIZE);
    TRACE_printDump((uint8_t *)&pFrame[nStartIndex], BC_KL1850_FRAME_SIZE, 0);
 
    pKL1850->ulCount = pFrame[5] | ((uint32_t)pFrame[6] << 8) | ((uint32_t)pFrame[7] << 16) | ((uint32_t)pFrame[8] << 24);
    
    return  true;
}
                          

uint32_t        BC_KL1850_getMessage(BILL_COUNTER* pBC, uint8_t* pBuffer, uint32_t ulBufferLength)
{

    ASSERT(ulBufferLength >= 7);

    uint32_t    ulMessageLen = 0;
    BC_KL1850 * pKL1850 = (BC_KL1850 *)pBC;
    
    pBuffer[ulMessageLen++] = 1;
    pBuffer[ulMessageLen++] = (pKL1850->ulCount >> 16) & 0xFF;
    pBuffer[ulMessageLen++] = (pKL1850->ulCount >> 8) & 0xFF;
    pBuffer[ulMessageLen++] = (pKL1850->ulCount     ) & 0xFF;
    pBuffer[ulMessageLen++] = 0;
    pBuffer[ulMessageLen++] = 0;
    pBuffer[ulMessageLen++] = 0;
    
    return  ulMessageLen;
}
