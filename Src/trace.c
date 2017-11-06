#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "ret_value.h"
#include "trace.h"
#include "shell.h"

static  bool    bTrace = false;
static  char    pBuff[128];

void        TRACE_setEnable(bool bEnable)
{
    bTrace = bEnable;
}

bool    TRACE_getEnable(void)
{
    return  bTrace;
}

void    TRACE_print(char *pString)
{
    if (bTrace)
    {
        SHELL_print(pString);
    }
}

void    TRACE_printUINT8(uint8_t xValue)
{
    if (bTrace)
    {
        char    pBuff[16];

        sprintf(pBuff, "%02x ", xValue);
        SHELL_print(pBuff);
    }
}

void    TRACE_printDump(uint8_t* pValue, uint32_t ulCount, uint32_t ulColumnLength)
{
    if (bTrace)
    {    
        while(ulCount > 0)
        {
            uint32_t    i;
            uint32_t    ulLen = 0;
            
            for(i = 0 ; i < ulCount && ((ulColumnLength == 0) || (i < ulColumnLength)) ; i++)
            {
                ulLen += sprintf(&pBuff[ulLen], "%02x ", pValue[i]);
            }
            ulLen += sprintf(&pBuff[ulLen], "\n");
            SHELL_print(pBuff);
            
            pValue += i;
            ulCount -= i;
        }
    }
}

RET_VALUE    TRACE_printf
(
    const char *pFormat, 
    ... 
)
{
    if (bTrace)
    {
       va_list  ap;
       uint32_t ulLen = 0;
       static char  pBuff[256];
       
    #if 0
        TIME_STRUCT  xTime;
       FTE_CHAR    pTimeBuff[64];
       
        _time_get(&xTime);
        FTE_TIME_toStr(&xTime, pTimeBuff, sizeof(pTimeBuff));   
        ulLen = sprintf(&_pBuff[ulLen], "[%s] ", pTimeBuff);
    #endif   
        va_start(ap, pFormat);
        vsnprintf(&pBuff[ulLen], sizeof(pBuff) - ulLen,  (char *)pFormat, ap );
        va_end(ap);
       
        SHELL_print(pBuff);
    }
    
    return  RET_OK;
}
