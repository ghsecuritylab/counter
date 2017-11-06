#include "bill_counter.h"
#include "plus_p16.h"
#include "plus_p506.h"
#include "jisan_j305.h"
#include "nexbill_kl1850.h"
#include "bh1000.h"

const BILL_COUNTER_INFO _xP506 = 
{
    .xModel     =   BC_MODEL_P506,
    .pName      =   "P-506",
    .xSTX       =   BC_P506_STX,
    .ulFrameSize=   BC_P506_FRAME_SIZE,
    .xBaudrate  =   SERIAL_BAUDRATE_9600,
    .xParity    =   SERIAL_PARITY_NONE,
    .xDataBits  =   SERIAL_DATA_BITS_8,
    .xStopBits  =   SERIAL_STOP_BITS_1,
    .fCreate    =   BC_P506_create,
    .fInput     =   BC_P506_input,
    .fGetMessage=   BC_P506_getMessage
};
    
const BILL_COUNTER_INFO _xJ305 = 
{
    .xModel     =   BC_MODEL_J305,
    .pName      =   "J-305",
    .xSTX       =   BC_J305_STX,
    .ulFrameSize=   BC_J305_FRAME_SIZE,
    .xBaudrate  =   SERIAL_BAUDRATE_9600,
    .xParity    =   SERIAL_PARITY_NONE,
    .xDataBits  =   SERIAL_DATA_BITS_8,
    .xStopBits  =   SERIAL_STOP_BITS_1,
    .fCreate    =   BC_J305_create,
    .fInput     =   BC_J305_input,
    .fGetMessage=   BC_J305_getMessage
};

const BILL_COUNTER_INFO _xP16 = 
{
    .xModel     =   BC_MODEL_P16,
    .pName      =   "P16",
    .xSTX       =   BC_P16_STX_1,
    .ulFrameSize=   BC_P16_FRAME_SIZE_1,
    .xBaudrate  =   SERIAL_BAUDRATE_9600,
    .xParity    =   SERIAL_PARITY_ODD,
    .xDataBits  =   SERIAL_DATA_BITS_8,
    .xStopBits  =   SERIAL_STOP_BITS_1,
    .fCreate    =   BC_P16_create,
    .fInput     =   BC_P16_input,
    .fGetMessage=   BC_P16_getMessage
};
    
const BILL_COUNTER_INFO _xKL1850 = 
{
    .xModel     =   BC_MODEL_KL1850,
    .pName      =   "KL1850",
    .xSTX       =   BC_KL1850_STX,
    .ulFrameSize=   BC_KL1850_FRAME_SIZE,
    .xBaudrate  =   SERIAL_BAUDRATE_57600,
    .xParity    =   SERIAL_PARITY_NONE,
    .xDataBits  =   SERIAL_DATA_BITS_8,
    .xStopBits  =   SERIAL_STOP_BITS_1,
    .fCreate    =   BC_KL1850_create,
    .fInput     =   BC_KL1850_input,
    .fGetMessage=   BC_KL1850_getMessage
};
    
const BILL_COUNTER_INFO _xBH1000= 
{
    .xModel     =   BC_MODEL_BH1000,
    .pName      =   "BH1000",
    .xSTX       =   BC_BH1000_STX,
    .ulFrameSize=   BC_BH1000_FRAME_SIZE,
    .xBaudrate  =   SERIAL_BAUDRATE_115200,
    .xParity    =   SERIAL_PARITY_NONE,
    .xDataBits  =   SERIAL_DATA_BITS_8,
    .xStopBits  =   SERIAL_STOP_BITS_1,
    .fCreate    =   BC_BH1000_create,
    .fInput     =   BC_BH1000_input,
    .fGetMessage=   BC_BH1000_getMessage
};
    
const BILL_COUNTER_INFO*    pBillCounterInfo[] = 
{
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    &_xP506,
    NULL,
    &_xJ305,
    &_xP16,
    &_xKL1850,
    &_xBH1000,
};

BILL_COUNTER*   BC_getInstance(uint32_t    ulModel)
{
    if (ulModel < sizeof(pBillCounterInfo) / sizeof(BILL_COUNTER_INFO*))
    {
        if(pBillCounterInfo[ulModel] == NULL)
        {
            return  NULL;
        }
                            
        return  pBillCounterInfo[ulModel]->fCreate(pBillCounterInfo[ulModel]);;
    }
    
    return  NULL;
}

uint32_t    BC_GetModelList(BILL_COUNTER_INFO *pCounters[], uint32_t ulMaxCount)
{
    uint32_t    ulCount = 0;
    
    for(uint32_t i = 0 ; i < sizeof(pBillCounterInfo) / sizeof(BILL_COUNTER_INFO*) ; i++)
    {
        if (ulCount >= ulMaxCount)
        {
            break;
        }

        if (pBillCounterInfo[i] != NULL)
        {
            pCounters[ulCount++] = (BILL_COUNTER_INFO *)pBillCounterInfo[i];
        }
    }

    return  ulCount;
}
