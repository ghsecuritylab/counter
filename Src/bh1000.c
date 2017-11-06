#include "FreeRTOS.h"
#include "ret_value.h"
#include "assert.h"
#include "bh1000.h"
#include "trace.h"

static  BC_BH1000 _BH1000;

BILL_COUNTER*   BC_BH1000_create(const BILL_COUNTER_INFO* pInfo)
{
    _BH1000.pInfo = pInfo;
    
    return  (BILL_COUNTER*)&_BH1000;
}

bool    BC_BH1000_input(BILL_COUNTER *pBC, SERIAL_HANDLE   hSerial)
{
    ASSERT(pBC->pInfo->xModel == BC_MODEL_BH1000);

    uint32_t        i;
    BC_BH1000*        pBH1000 = (BC_BH1000 *)pBC;
    uint32_t        ulReadLength = 0;
    static  uint8_t pFrame[BC_BH1000_FRAME_SIZE*2];

    if (SERIAL_getc(hSerial, (char *)&pFrame[0], 10) != RET_OK)
    {
        return  false;
    }
    ulReadLength = 1;

    if (pFrame[0] != BC_BH1000_STX)
    {
        return  false;
    }
    
    ulReadLength += SERIAL_gets(hSerial, (char *)&pFrame[1], BC_BH1000_FRAME_SIZE - 1, 100);
    if (ulReadLength != BC_BH1000_FRAME_SIZE)
    {
        return  false;
    }

    TRACE_printf("Read[%2d] - ", BC_BH1000_FRAME_SIZE);
    TRACE_printDump((uint8_t *)pFrame, BC_BH1000_FRAME_SIZE, 0);
 
    pBH1000->ulCount = 0;    
    for(i = 4 ; i < 8 ; i++)
    {
        if ((pFrame[i] < 0x30) || (0x39 < pFrame[i]))
        {
            break;
        }

        pBH1000->ulCount = (pBH1000->ulCount * 10) + pFrame[i] - 0x30;
    }
    
    pBH1000->ulSum = 0;    
    for(i = 8 ; i < 16 ; i++)
    {
        if ((pFrame[i] < 0x30) || (0x39 < pFrame[i]))
        {
            break;
        }

        pBH1000->ulSum = (pBH1000->ulSum * 10) + pFrame[i] - 0x30;
    }
    
    return  true;
}
                          

uint32_t        BC_BH1000_getMessage(BILL_COUNTER* pBC, uint8_t* pBuffer, uint32_t ulBufferLength)
{

    ASSERT(ulBufferLength >= 8);

    uint32_t    ulMessageLen = 0;
    BC_BH1000 * pBH1000 = (BC_BH1000 *)pBC;
    
    pBuffer[ulMessageLen++] = 1;
    pBuffer[ulMessageLen++] = 0;
    pBuffer[ulMessageLen++] = 0;
    pBuffer[ulMessageLen++] = (pBH1000->ulCount     ) & 0xFF;
    pBuffer[ulMessageLen++] = (pBH1000->ulSum >> 24 ) & 0xFF;
    pBuffer[ulMessageLen++] = (pBH1000->ulSum >> 16 ) & 0xFF;
    pBuffer[ulMessageLen++] = (pBH1000->ulSum >> 8  ) & 0xFF;
    pBuffer[ulMessageLen++] = (pBH1000->ulSum       ) & 0xFF;
    
    return  ulMessageLen;
}
