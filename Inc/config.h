#ifndef CONFIG_H_
#define CONFIG_H_

#include "FreeRTOS.h"
#include "ethernet.h"
#include "ret_value.h"
#include "ip4_addr.h"

#define CONFIG_DEFAULT_PASSWORD         "posudo"
#define CONFIG_DEFAULT_SERIAL_NUMBER    "00000000"

#define CONFIG_SVRIF_STACK_SIZE   512
#define CONFIG_SVRIF_PRIORITY     osPriorityNormal

#define CONFIG_CNT_STACK_SIZE   256
#define CONFIG_CNT_PRIORITY     osPriorityNormal

#define CONFIG_SHELL_STACK_SIZE 128
#define CONFIG_SHELL_PRIORITY   osPriorityNormal

typedef struct
{
    uint8_t         pMAC[ETH_HWADDR_LEN];
    
    int             bStatic;
    ip4_addr_t      xIPAddr;
    ip4_addr_t      xNetmask;
    ip4_addr_t      xGatewayIPAddr;
}   NET_CONFIG;

typedef struct
{
    ip4_addr_t      xIPAddr;
    uint16_t        nPort;
}   SERVER_CONFIG;

#define CONFIG_SERIAL_NUMBER_LEN    32
#define CONFIG_PASSWORD_LEN         32

typedef struct
{
    uint16_t        nCRC;
    
    char            pPasswd[32];
    char            pSerialNumber[32];
    NET_CONFIG      xNet;
    SERVER_CONFIG   xServer;
}   CONFIG;


RET_VALUE   CONFIG_init(void);
RET_VALUE   CONFIG_setDefault(CONFIG* pConfig);
RET_VALUE   CONFIG_load(CONFIG* pConfig);
RET_VALUE   CONFIG_save(CONFIG* pConfig);

extern  const   char pFirmwareVersion[];

#endif