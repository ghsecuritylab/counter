#ifndef __NET_H__
#define __NET_H__

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "cmsis_os.h"
#include "lwip/opt.h"
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "netif/etharp.h"
#include "lwip/dhcp.h"
#include "lwip/netif.h"
#include "lwip/timeouts.h"     
#include "lwip/tcpip.h"
     
#include "ret_value.h"
#include "config.h"

typedef struct
{
    uint8_t nLength;
    uint8_t pData[];
}   MESSAGE;

RET_VALUE   SVRIF_start(void);
RET_VALUE   SVRIF_stop(void);

void    SVRIF_setFlags(u8_t xFlag);
void    SVRIF_clearFlags(u8_t xFlag);

RET_VALUE   SVRIF_send(uint8_t* pData, uint8_t  nDataLength);

err_t   SVRIF_input( struct pbuf *p);
#endif
