#ifndef _TRACE_H
#define _TRACE_H

#include <stdint.h>
#include "ret_value.h"
#include "stdbool.h"

void        TRACE_setEnable(bool bEnable);
bool        TRACE_getEnable(void);
void        TRACE_print(char *pString);
void        TRACE_printUINT8(uint8_t xValue);
void        TRACE_printDump(uint8_t* pValue, uint32_t ulCount, uint32_t ulColumnLength);
RET_VALUE   TRACE_printf(const char *pFormat, ... );


#endif