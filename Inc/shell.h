#ifndef __SHELL_H__
#define __SHELL_H__

#include "serial.h"
#include "cmsis_os.h"

typedef struct
{
    SERIAL      xSerial;
    osThreadId  xThread;
}   SHELL;

RET_VALUE   SHELL_create(SHELL** ppShell);
RET_VALUE   SHELL_destroy(SHELL** ppShell);

RET_VALUE   SHELL_start( SHELL*    pShell, uint16_t usStackSize, UBaseType_t uxPriority );
RET_VALUE   SHELL_stop( SHELL*    pShell);

#endif
