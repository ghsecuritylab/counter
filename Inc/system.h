#ifndef SYS_H_
#define SYS_H_

#include "FreeRTOS.h"

void        SYS_init(void);
void        SYS_reset(void);
uint8_t     SYS_getModelID(void);
const char* SYS_getModelName(uint8_t xModel);

#endif