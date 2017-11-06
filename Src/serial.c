#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "queue.h"
#include "semphr.h"
#include "assert.h"
/* Library includes. */
#include "stm32f1xx_hal.h"

/* Demo application includes. */
#include "serial.h"
/*-----------------------------------------------------------*/

/* Misc defines. */
#define SERIAL_INVALID_QUEUE				( ( QueueHandle_t ) 0 )
#define SERIAL_NO_BLOCK						( ( TickType_t ) 0 )
#define SERIAL_TX_BLOCK_TIME				( 40 / portTICK_PERIOD_MS )

/*-----------------------------------------------------------*/

/* UART interrupt handler. */
void vUARTInterruptHandler( void );

/*-----------------------------------------------------------*/
static  SERIAL  _pSerials[SERIAL_PORT_MAX];

RET_VALUE       SERIAL_open(SERIAL_PORT xPort, SERIAL_BAUDRATE xBaudrate, SERIAL_PARITY xParity, SERIAL_STOP_BITS xStop, SERIAL_DATA_BITS xData, unsigned portBASE_TYPE uxBufferLength, SERIAL_HANDLE *phSerial)
{
    RET_VALUE   xRet = RET_OK;
    
    if (xPort >= SERIAL_PORT_MAX)
    {
        return  RET_ERROR;
    }
    
    switch(xPort)
    {
    case    SERIAL_PORT_1:    _pSerials[xPort].xUART.Instance = USART1; break;
//    case    SERIAL_PORT_2:    _pSerials[xPort].xUART.Instance = USART2; break;
//    case    SERIAL_PORT_3:    _pSerials[xPort].xUART.Instance = USART3; break;
    case    SERIAL_PORT_4:    _pSerials[xPort].xUART.Instance = UART4; break;
    default:
        return  RET_ERROR;
    }
    
    switch(xBaudrate)
    {
    case    SERIAL_BAUDRATE_50:		_pSerials[xPort].xUART.Init.BaudRate = 50;   break;
	case    SERIAL_BAUDRATE_75:		_pSerials[xPort].xUART.Init.BaudRate = 75;   break;
	case    SERIAL_BAUDRATE_110:	_pSerials[xPort].xUART.Init.BaudRate = 110;   break;
	case    SERIAL_BAUDRATE_134:	_pSerials[xPort].xUART.Init.BaudRate = 134;   break;
	case    SERIAL_BAUDRATE_150:	_pSerials[xPort].xUART.Init.BaudRate = 150;   break;
	case    SERIAL_BAUDRATE_200:	_pSerials[xPort].xUART.Init.BaudRate = 200;   break;
	case    SERIAL_BAUDRATE_300:	_pSerials[xPort].xUART.Init.BaudRate = 300;   break;
	case    SERIAL_BAUDRATE_600:	_pSerials[xPort].xUART.Init.BaudRate = 600;   break;
	case    SERIAL_BAUDRATE_1200:	_pSerials[xPort].xUART.Init.BaudRate = 1200;   break;
	case    SERIAL_BAUDRATE_1800:	_pSerials[xPort].xUART.Init.BaudRate = 1800;   break;
	case    SERIAL_BAUDRATE_2400:	_pSerials[xPort].xUART.Init.BaudRate = 2400;   break;
	case    SERIAL_BAUDRATE_4800:	_pSerials[xPort].xUART.Init.BaudRate = 4800;   break;
	case    SERIAL_BAUDRATE_9600:	_pSerials[xPort].xUART.Init.BaudRate = 9600;   break;
	case    SERIAL_BAUDRATE_19200:	_pSerials[xPort].xUART.Init.BaudRate = 19200;   break;
	case    SERIAL_BAUDRATE_38400:	_pSerials[xPort].xUART.Init.BaudRate = 38400;   break;
	case    SERIAL_BAUDRATE_57600:	_pSerials[xPort].xUART.Init.BaudRate = 57600;   break;
	case    SERIAL_BAUDRATE_115200:	_pSerials[xPort].xUART.Init.BaudRate = 115200;   break;
	default:	                    _pSerials[xPort].xUART.Init.BaudRate = 115200;   break;
    }

    switch(xParity)
    {
    case    SERIAL_PARITY_NONE: _pSerials[xPort].xUART.Init.Parity = UART_PARITY_NONE;  break;
	case    SERIAL_PARITY_ODD:  _pSerials[xPort].xUART.Init.Parity = UART_PARITY_ODD;  break;
	case    SERIAL_PARITY_EVEN: _pSerials[xPort].xUART.Init.Parity = UART_PARITY_EVEN;  break;
    default:                    _pSerials[xPort].xUART.Init.Parity = UART_PARITY_NONE;  break;
    }

    switch(xData)
    {
    case    SERIAL_DATA_BITS_8: _pSerials[xPort].xUART.Init.WordLength = UART_WORDLENGTH_8B;    break;
    default:                    _pSerials[xPort].xUART.Init.WordLength = UART_WORDLENGTH_8B;    break;
    }

    switch(xStop)
    {
    case    SERIAL_STOP_BITS_1: _pSerials[xPort].xUART.Init.StopBits = UART_STOPBITS_1;    break;
	case    SERIAL_STOP_BITS_2: _pSerials[xPort].xUART.Init.StopBits = UART_STOPBITS_2;    break;
    default:                    _pSerials[xPort].xUART.Init.StopBits = UART_STOPBITS_1;    break;
    }

    _pSerials[xPort].xUART.Init.Mode = UART_MODE_TX_RX;
    _pSerials[xPort].xUART.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    _pSerials[xPort].xUART.Init.OverSampling = UART_OVERSAMPLING_16;

	_pSerials[xPort].xSemaphore = xSemaphoreCreateMutex();
	if (_pSerials[xPort].xSemaphore == NULL)
    {
        xRet = RET_ERROR;
        goto finished;
    }

	_pSerials[xPort].xRxWait = xSemaphoreCreateBinary();
	if (_pSerials[xPort].xRxWait == NULL)
    {
        xRet = RET_ERROR;
        goto finished;
    }
	xSemaphoreTake(_pSerials[xPort].xRxWait, 1);

    _pSerials[xPort].pRxBuffer = (uint8_t *)pvPortMalloc(uxBufferLength);
    if (_pSerials[xPort].pRxBuffer == NULL)
    {
        xRet = RET_ERROR;
        goto finished;
    }
    _pSerials[xPort].ulRxBufferLength = uxBufferLength;
    _pSerials[xPort].ulRxIndex = 0;
    _pSerials[xPort].ulRxLength = 0;
    
    if (HAL_UART_Init(&_pSerials[xPort].xUART) != HAL_OK)
    {
        xRet = RET_ERROR;
        goto finished;
    }

    HAL_UART_Receive_IT(&_pSerials[xPort].xUART, _pSerials[xPort].pTmpBuffer, sizeof(_pSerials[xPort].pTmpBuffer));    

    switch(xPort)
    {
    case    SERIAL_PORT_1:    _pSerials[xPort].xIRQ = USART1_IRQn; break;
//    case    SERIAL_PORT_2:    _pSerials[xPort].xIRQ = USART2_IRQn; break;
//    case    SERIAL_PORT_3:    _pSerials[xPort].xIRQ = USART3_IRQn; break;
    case    SERIAL_PORT_4:    _pSerials[xPort].xIRQ = UART4_IRQn; break;
    default:
        xRet = RET_ERROR;
        goto finished;
    }
    
    HAL_NVIC_SetPriority(_pSerials[xPort].xIRQ, 5, 0);
    HAL_NVIC_EnableIRQ(_pSerials[xPort].xIRQ );
    
    *phSerial = &_pSerials[xPort];

finished:
    if (xRet != RET_OK)
    {
        if (_pSerials[xPort].xSemaphore != NULL)
        {
            vSemaphoreDelete(_pSerials[xPort].xSemaphore);
            _pSerials[xPort].xSemaphore = NULL;            
        }
        
        if (_pSerials[xPort].xRxWait != NULL)
        {
            vSemaphoreDelete(_pSerials[xPort].xRxWait);
            _pSerials[xPort].xRxWait = NULL;            
        }
        
        if (_pSerials[xPort].pRxBuffer != NULL)
        {
            vPortFree(_pSerials[xPort].pRxBuffer);
            _pSerials[xPort].pRxBuffer = NULL;
        }
    }
    
    return  xRet;
}

/*-----------------------------------------------------------*/

RET_VALUE       SERIAL_close(SERIAL_HANDLE hSerial)
{
    ASSERT(hSerial != NULL);

    HAL_NVIC_DisableIRQ(hSerial->xIRQ);

    if (HAL_UART_DeInit(&hSerial->xUART) != HAL_OK)
    {
        return  RET_ERROR;
    }
    
	if (hSerial->xSemaphore != NULL)
    {
        vSemaphoreDelete(hSerial->xSemaphore);
        hSerial->xSemaphore = NULL;
    }
    
	if (hSerial->xRxWait != NULL)
    {
        vSemaphoreDelete(hSerial->xRxWait);
        hSerial->xRxWait = NULL;
    }
    
    if (hSerial->pRxBuffer != NULL)
    {
        vPortFree(hSerial->pRxBuffer);
        hSerial->pRxBuffer = NULL;
    }
    
    return  RET_OK;        
}

unsigned long SERIAL_gets(SERIAL_HANDLE hSerial, char *pValue, unsigned long ulSize, TickType_t xBlockTime)
{
    ASSERT(hSerial != NULL);
    unsigned long   ulCount = 0;
    char            xCH;
    
    TickType_t  xStartTick = xTaskGetTickCount();
    
    while(ulCount < ulSize)
    {
        if (SERIAL_getc(hSerial, &xCH, 1) == RET_OK)
        {
           pValue[ulCount++] = xCH;
        }
        else
        {
            osDelay(1);
        }

        if (xTaskGetTickCount() - xStartTick > xBlockTime)
        {
            break;
        }
    }

    return  ulCount;
}

/*-----------------------------------------------------------*/
/*-----------------------------------------------------------*/

RET_VALUE   SERIAL_puts(SERIAL_HANDLE hSerial, const char * const pData, unsigned long ulLength, TickType_t xBlockTime)
{
    ASSERT(hSerial != NULL);

    RET_VALUE   xRet = RET_OK;
    
    if( xSemaphoreTake( hSerial->xSemaphore, xBlockTime) != pdPASS )
    {
        xRet = RET_ERROR;
    }
    else
    {        
        if (HAL_UART_Transmit(&hSerial->xUART, (uint8_t *)pData, ulLength, xBlockTime) != HAL_OK)
        {
            xRet = RET_ERROR;
        }

        xSemaphoreGive( hSerial->xSemaphore);
    }
    
    return  xRet;
}

/*-----------------------------------------------------------*/

char            SERIAL_getc(SERIAL_HANDLE hSerial, char *pValue, TickType_t xBlockTime)
{ 
    ASSERT(hSerial != NULL);
    
    if( xSemaphoreTake( hSerial->xSemaphore, xBlockTime) != pdPASS )
    {
        return  RET_ERROR;
    }

    if (hSerial->ulRxLength == 0)
    {
        if (xSemaphoreTake( hSerial->xRxWait, xBlockTime) != pdPASS )
        {
            xSemaphoreGive( hSerial->xSemaphore);

            return  RET_ERROR;
        }
    }

    if (hSerial->ulRxLength == 0)
    {
        xSemaphoreGive( hSerial->xSemaphore);
        return  RET_ERROR;
    }

    __HAL_UART_DISABLE_IT(&hSerial->xUART, UART_IT_RXNE);
    
    *pValue = (char)hSerial->pRxBuffer[hSerial->ulRxIndex];
    hSerial->ulRxIndex = (hSerial->ulRxIndex + 1) % hSerial->ulRxBufferLength ;
    --hSerial->ulRxLength;
    
    __HAL_UART_ENABLE_IT(&hSerial->xUART, UART_IT_RXNE);
    
    xSemaphoreGive( hSerial->xSemaphore);
    
    return  RET_OK;
}

/*-----------------------------------------------------------*/

char            SERIAL_putc(SERIAL_HANDLE hSerial, char cValue, TickType_t xBlockTime)
{
    ASSERT(hSerial != NULL);

    RET_VALUE   xRet = RET_OK;
    
    if( xSemaphoreTake( hSerial->xSemaphore, xBlockTime) != pdPASS )
    {
        xRet = RET_ERROR;
    }
    else
    {        
        if (HAL_UART_Transmit(&hSerial->xUART, (uint8_t *)&cValue, 1, xBlockTime) != HAL_OK)
        {
            xRet = RET_ERROR;
        }
        
        xSemaphoreGive( hSerial->xSemaphore);
    }
    
    return  xRet;
}

RET_VALUE    SERIAL_printf
(
    SERIAL_HANDLE   hSerial,
    const char *pFormat, 
    ... 
)
{
   va_list  ap;
   uint32_t ulLen = 0;
   static char  pBuff[128];
   
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

    SERIAL_puts(hSerial, pBuff, strlen(pBuff), HAL_MAX_DELAY );

    return  RET_OK;
}


void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(&_pSerials[SERIAL_PORT_1].xUART);
    
    if (_pSerials[SERIAL_PORT_1].xUART.RxXferSize != _pSerials[SERIAL_PORT_1].xUART.RxXferCount)
    {
        if (_pSerials[SERIAL_PORT_1].ulRxBufferLength == _pSerials[SERIAL_PORT_1].ulRxLength)
        {
            _pSerials[SERIAL_PORT_1].ulRxIndex = (_pSerials[SERIAL_PORT_1].ulRxIndex + 1) % _pSerials[SERIAL_PORT_1].ulRxBufferLength ;
            --_pSerials[SERIAL_PORT_1].ulRxLength;
        }
        
        uint32_t    ulPosition = (_pSerials[SERIAL_PORT_1].ulRxIndex +  _pSerials[SERIAL_PORT_1].ulRxLength ) % _pSerials[SERIAL_PORT_1].ulRxBufferLength;
        _pSerials[SERIAL_PORT_1].xUART.pRxBuffPtr--;
        _pSerials[SERIAL_PORT_1].xUART.RxXferCount++;
        _pSerials[SERIAL_PORT_1].pRxBuffer[ulPosition] = *_pSerials[SERIAL_PORT_1].xUART.pRxBuffPtr;
        _pSerials[SERIAL_PORT_1].ulRxLength++;        

        if (_pSerials[SERIAL_PORT_1].ulRxLength == 1)
        {
            BaseType_t xHigherPriorityTaskWoken;

            xSemaphoreGiveFromISR(_pSerials[SERIAL_PORT_1].xRxWait, &xHigherPriorityTaskWoken);
        }
    }    
}

void UART4_IRQHandler(void)
{
    HAL_UART_IRQHandler(&_pSerials[SERIAL_PORT_4].xUART);
    
    if (_pSerials[SERIAL_PORT_4].xUART.RxXferSize != _pSerials[SERIAL_PORT_4].xUART.RxXferCount)
    {
        if (_pSerials[SERIAL_PORT_4].ulRxBufferLength == _pSerials[SERIAL_PORT_4].ulRxLength)
        {
            _pSerials[SERIAL_PORT_4].ulRxIndex = (_pSerials[SERIAL_PORT_4].ulRxIndex + 1) % _pSerials[SERIAL_PORT_4].ulRxBufferLength ;
            --_pSerials[SERIAL_PORT_4].ulRxLength;
        }

        if (_pSerials[SERIAL_PORT_4].ulRxLength == 0)
        {
            BaseType_t xHigherPriorityTaskWoken;

            xSemaphoreGiveFromISR(_pSerials[SERIAL_PORT_4].xRxWait, &xHigherPriorityTaskWoken);
        }
        
        uint32_t    ulPosition = (_pSerials[SERIAL_PORT_4].ulRxIndex +  _pSerials[SERIAL_PORT_4].ulRxLength) % _pSerials[SERIAL_PORT_4].ulRxBufferLength;
        _pSerials[SERIAL_PORT_4].xUART.pRxBuffPtr--;
        _pSerials[SERIAL_PORT_4].xUART.RxXferCount++;
        _pSerials[SERIAL_PORT_4].pRxBuffer[ulPosition] = *_pSerials[SERIAL_PORT_4].xUART.pRxBuffPtr;
        _pSerials[SERIAL_PORT_4].ulRxLength++;        
    }    
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    assert_param(huart != NULL);
    
    huart->pRxBuffPtr -= huart->RxXferSize;
    huart->RxXferCount = huart->RxXferSize;
}

void    HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
}