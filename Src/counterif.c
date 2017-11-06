/* Standard includes. */
#include "string.h"
#include "stdio.h"

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "task.h"
#include "semphr.h"

/* Example includes. */
#include "FreeRTOS_CLI.h"
        
/* Demo application includes. */
#include "stm32f1xx_hal.h"
#include "trace.h"
#include "counterif.h"
#include "serverif.h"

#define CNT_MAX_FRAME_SIZE		50

static  osThreadId      xThread = NULL;
static uint8_t  pBuffer[128];
static uint8_t  pPrevBuffer[128];
static SERIAL_HANDLE   hSerial = NULL;

/*-----------------------------------------------------------*/

/*
 * The task that implements the command console processing.
 */
void CNTIF_main( void const *pvParameters );

/*-----------------------------------------------------------*/
RET_VALUE   CNTIF_start(BILL_COUNTER* pBillCounter)
{
    RET_VALUE   xRet = RET_OK;
    
    if (xThread == NULL)
    {
        osThreadDef(CntIFTask, CNTIF_main, CONFIG_CNT_PRIORITY, 0, CONFIG_CNT_STACK_SIZE);
        xThread = osThreadCreate(osThread(CntIFTask), pBillCounter);
        if (xThread == NULL)
        {
            xRet = RET_ERROR;
        }
    }

    return  xRet;
}

/*-----------------------------------------------------------*/
RET_VALUE   CNTIF_stop(void)
{
    if (xThread != 0)
    {
        if (hSerial != NULL)
        {
            SERIAL_close(hSerial);
            hSerial = NULL;
        }
        
        osThreadTerminate(xThread);
        xThread = NULL;
    }
         
    return RET_OK;
}

/*-----------------------------------------------------------*/
void CNTIF_main( void const *pParams )
{
    RET_VALUE       xRet;
    BILL_COUNTER*   pBC = (BILL_COUNTER*)pParams;
    unsigned long   ulTime = 0;
    unsigned long   ulPrevTime = 0;
    unsigned long   ulCount = 0;
    unsigned long   ulPrevCount = 0;
    
    memset(pBuffer, 0, sizeof(pBuffer));
    memset(pPrevBuffer, 0, sizeof(pPrevBuffer));
    
	/* Initialise the UART. */
    xRet = SERIAL_open(SERIAL_PORT_1, pBC->pInfo->xBaudrate, pBC->pInfo->xParity, pBC->pInfo->xStopBits, pBC->pInfo->xDataBits, 256, &hSerial);
    if (xRet != RET_OK)
    {
        return ;
    }
    
	for( ;; )
	{
        if (pBC->pInfo->fInput(pBC, hSerial) == false)
        {
            vTaskDelay(10);
            continue;
        }
        
        ulCount = pBC->pInfo->fGetMessage(pBC, pBuffer, sizeof(pBuffer));
        
        ulTime = xTaskGetTickCount() / configTICK_RATE_HZ;
        if ((ulTime != ulPrevTime) || (ulCount != ulPrevCount) || (memcmp(pBuffer, pPrevBuffer, ulCount) != 0))
        {
            SVRIF_send((uint8_t *)pBuffer, ulCount);
            memcpy(pPrevBuffer, pBuffer, ulCount);
            ulPrevCount = ulCount;
            ulPrevTime  = ulTime;
        }
        vTaskDelay(1);
	}
}
