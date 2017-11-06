/* Standard includes. */
#include "string.h"
#include "stdio.h"

/* FreeRTOS includes. */
#include <lwip/sockets.h> 
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "task.h"
#include "semphr.h"
#include "trace.h"
#include "assert.h"
#include "ethernetif.h"
#include "netif_ex.h"
#include "shell.h"

/* Demo application includes. */
#include "stm32f1xx_hal.h"
#include "serverif.h"

extern  CONFIG  xConfig;    
extern  ETH_HandleTypeDef xETH;

/* Variables Initialization */
static  QueueHandle_t   xMessageQ;
static  struct netif    xIF;
static  osThreadId      xThread;


/*
 * The task that implements the command console processing.
 */
void SVRIF_taskMain( void const *pvParameters );

RET_VALUE   SVRIF_client();

/*-----------------------------------------------------------*/

/*-----------------------------------------------------------*/
RET_VALUE   SVRIF_start(void)
{
    xMessageQ = xQueueCreate( 10, sizeof( MESSAGE * ) );
    
    osThreadDef(NetTask, SVRIF_taskMain, CONFIG_SVRIF_PRIORITY, 0, CONFIG_SVRIF_STACK_SIZE);
    xThread = osThreadCreate(osThread(NetTask), 0);
    if (xThread == NULL)
    {
        vQueueDelete(xMessageQ);
        xMessageQ = 0;
        return  RET_ERROR;
    }

    return  RET_OK;
}

/*-----------------------------------------------------------*/
RET_VALUE   SVRIF_stop( void )
{
    if (xThread != 0)
    {
        MESSAGE*    pMsg;
        
        ETH_stop();

        osThreadTerminate(xThread);
        xThread = 0;
        
        while( xQueueReceive( xMessageQ, &pMsg, 0 ) == pdPASS )
        {
            vPortFree(pMsg);
        }
        
        vQueueDelete(xMessageQ);
        xMessageQ = 0;
    }
         
    return RET_OK;
}


/*-----------------------------------------------------------*/
RET_VALUE   SVRIF_send(uint8_t* pData, uint8_t  nDataLength)
{
    MESSAGE* pMsg = (MESSAGE*)pvPortMalloc(sizeof(MESSAGE) + nDataLength + 10);
    if (pMsg == NULL)
    {
        return  RET_ERROR;
    }
    
    pMsg->nLength = 0;
    memcpy(&pMsg->pData[pMsg->nLength], pData, nDataLength);
    pMsg->nLength += nDataLength;
    
    if( xQueueSend( xMessageQ, ( void * ) &pMsg, 0 ) != pdPASS )
    {
        
        vPortFree(pMsg);
        return  RET_ERROR;
    }
        
    return  RET_OK;
}

void    SVRIF_setFlags(u8_t xFlag)
{
    xIF.flags |= xFlag;
}

void    SVRIF_clearFlags(u8_t xFlag)
{
    xIF.flags &= ~xFlag;
}

err_t   SVRIF_input( struct pbuf *p)
{
    return  xIF.input( p, &xIF );
}


/*-----------------------------------------------------------*/
  
void SVRIF_taskMain( void const *pParams )
{
    /* init code for LWIP */
    tcpip_init( NULL, NULL );

    /* add the network interface (IPv4/IPv6) with RTOS */
    if (netif_add(&xIF, &xConfig.xNet.xIPAddr, &xConfig.xNet.xNetmask, &xConfig.xNet.xGatewayIPAddr, NULL, &NETIF_initCB, &tcpip_input) == NULL)
    {
        return;
    }

    /* set MAC hardware address */
    xIF.hwaddr_len = ETH_HWADDR_LEN;
    memcpy(xIF.hwaddr, xConfig.xNet.pMAC, ETH_HWADDR_LEN);

    ETH_lowLevelInit(&xETH);

    /* Registers the default network interface */
    netif_set_default(&xIF);    
    
   /* Start DHCP negotiation for a network interface (IPv4) */
   // if (!xConfig.bStatic)
    {
        dhcp_start(&xIF);
    }

    for(;;)
    {
        if (!(xIF.flags & NETIF_FLAG_UP))
        {
            if (netif_is_link_up(&xIF))
            {
                /* When the netif is fully configured this function must be called */
                netif_set_up(&xIF);
            
                SVRIF_client();
            }

        } 
        else
        {
            if (!netif_is_link_up(&xIF))
            {
                /* When the netif link is down this function must be called */
                netif_set_down(&xIF);
            }
            else
            {
                SVRIF_client();
            }
        }

        osDelay(1000);
    }
}

static char    pBuffer[128];
RET_VALUE   SVRIF_client(void)
{
    struct sockaddr_in  addr;
    int sock;
    int nRet;
    
    memset(&addr, 0, sizeof(addr));
    addr.sin_len = sizeof(addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(xConfig.xServer.nPort);
    addr.sin_addr.s_addr = xConfig.xServer.xIPAddr.addr;

    sock = lwip_socket(AF_INET, SOCK_STREAM, 0);
    if ( sock < 0)
    {
        return  RET_ERROR;
    }
    
    nRet = lwip_connect(sock, (struct sockaddr*)&addr, sizeof(addr));
    if (nRet != 0)
    {
        return  RET_ERROR;
    }
    
    while(1)
    {
        MESSAGE* pMsg = NULL;
        
        if( xQueueReceive( xMessageQ, &pMsg, 10 ) == pdPASS )
        {
            int32_t nLen = 0;
            
            pBuffer[nLen++] = 0x58;
            pBuffer[nLen++] = 0x00;
            pBuffer[nLen++] =  6 + pMsg->nLength;
            pBuffer[nLen++] =  xConfig.xNet.pMAC[5];
            pBuffer[nLen++] =  xConfig.xNet.pMAC[4];
            pBuffer[nLen++] =  xConfig.xNet.pMAC[3];
            pBuffer[nLen++] =  xConfig.xNet.pMAC[2];
            pBuffer[nLen++] =  xConfig.xNet.pMAC[1];
            pBuffer[nLen++] =  xConfig.xNet.pMAC[0];
            memcpy(&pBuffer[nLen], pMsg->pData, pMsg->nLength);
            nLen += pMsg->nLength;
            pBuffer[nLen++] = 0x64;
            
            int32_t    nSentLength;
            nSentLength = lwip_write(sock, pBuffer, nLen);
            if (nSentLength < 0)
            {
                vPortFree(pMsg);
                TRACE_printf("connect failed \n");
                break;
            }
            
            if (nSentLength != nLen)
            {
                TRACE_printf("Send failed \n");
            }
            
            TRACE_printf("Send[%d] - ", nSentLength);
            TRACE_printDump((uint8_t*)pBuffer, nLen, 0);

            vPortFree(pMsg);
            
            osDelay(100);
        }
        else
        {
            osDelay(100);
        }
    }

    lwip_close(sock);
    
    return  RET_OK;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == GPIO_PIN_14)
  {
//    SVRIF_setLink(&gnetif);
  }
  else if (GPIO_Pin == GPIO_PIN_15)
  {
    /*connect to tcp server */ 
//    tcp_echoclient_connect();
  }
}

