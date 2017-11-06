#ifndef __CNTIF_H__
#define __CNTIF_H__

#include "serial.h"
#include "cmsis_os.h"
#include "bill_counter.h"

RET_VALUE   CNTIF_start(BILL_COUNTER* pBillCounter);
RET_VALUE   CNTIF_stop(void);

#endif
