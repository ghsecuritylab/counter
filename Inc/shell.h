#ifndef __SHELL_H__
#define __SHELL_H__

#include "cmsis_os.h"
#include "serial.h"
#include "bill_counter.h"

RET_VALUE   SHELL_start(BILL_COUNTER* pBC);
RET_VALUE   SHELL_stop(void);

int         SHELL_getLine(char* pLine, unsigned int ulMaxLength, int bSecure );
RET_VALUE   SHELL_print(char* pString);
RET_VALUE   SHELL_dump(uint8_t *pBuffer, uint32_t ulLen);
#endif
