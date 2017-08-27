#include "FreeRTOS.h"
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

RET_VALUE       SERIAL_open(SERIAL* pSerial, SERIAL_PORT xPort, SERIAL_BAUDRATE xBaudrate, SERIAL_PARITY xParity, SERIAL_STOP_BITS xStop, SERIAL_DATA_BITS xData, unsigned portBASE_TYPE uxBufferLength)
{
    assert(pSerial != NULL);

    switch(xPort)
    {
    case    SERIAL_PORT_1:    pSerial->xUART.Instance = USART1; break;
    case    SERIAL_PORT_2:    pSerial->xUART.Instance = USART2; break;
    case    SERIAL_PORT_3:    pSerial->xUART.Instance = USART3; break;
    case    SERIAL_PORT_4:    pSerial->xUART.Instance = UART4; break;
    default:
        return  RET_ERROR;
    }
    
    switch(xBaudrate)
    {
    case    SERIAL_BAUDRATE_50:		pSerial->xUART.Init.BaudRate = 50;   break;
	case    SERIAL_BAUDRATE_75:		pSerial->xUART.Init.BaudRate = 75;   break;
	case    SERIAL_BAUDRATE_110:	pSerial->xUART.Init.BaudRate = 110;   break;
	case    SERIAL_BAUDRATE_134:	pSerial->xUART.Init.BaudRate = 134;   break;
	case    SERIAL_BAUDRATE_150:	pSerial->xUART.Init.BaudRate = 150;   break;
	case    SERIAL_BAUDRATE_200:	pSerial->xUART.Init.BaudRate = 200;   break;
	case    SERIAL_BAUDRATE_300:	pSerial->xUART.Init.BaudRate = 300;   break;
	case    SERIAL_BAUDRATE_600:	pSerial->xUART.Init.BaudRate = 600;   break;
	case    SERIAL_BAUDRATE_1200:	pSerial->xUART.Init.BaudRate = 1200;   break;
	case    SERIAL_BAUDRATE_1800:	pSerial->xUART.Init.BaudRate = 1800;   break;
	case    SERIAL_BAUDRATE_2400:	pSerial->xUART.Init.BaudRate = 2400;   break;
	case    SERIAL_BAUDRATE_4800:	pSerial->xUART.Init.BaudRate = 4800;   break;
	case    SERIAL_BAUDRATE_9600:	pSerial->xUART.Init.BaudRate = 9600;   break;
	case    SERIAL_BAUDRATE_19200:	pSerial->xUART.Init.BaudRate = 19200;   break;
	case    SERIAL_BAUDRATE_38400:	pSerial->xUART.Init.BaudRate = 38400;   break;
	case    SERIAL_BAUDRATE_57600:	pSerial->xUART.Init.BaudRate = 57600;   break;
	case    SERIAL_BAUDRATE_115200:	pSerial->xUART.Init.BaudRate = 115200;   break;
	default:	pSerial->xUART.Init.BaudRate = 115200;   break;
    }

    switch(xParity)
    {
    case    SERIAL_PARITY_NONE: pSerial->xUART.Init.Parity = UART_PARITY_NONE;  break;
	case    SERIAL_PARITY_ODD:  pSerial->xUART.Init.Parity = UART_PARITY_ODD;  break;
	case    SERIAL_PARITY_EVEN: pSerial->xUART.Init.Parity = UART_PARITY_EVEN;  break;
    default:                    pSerial->xUART.Init.Parity = UART_PARITY_NONE;  break;
    }

    switch(xData)
    {
    case    SERIAL_DATA_BITS_8: pSerial->xUART.Init.WordLength = UART_WORDLENGTH_8B;    break;
    default:                    pSerial->xUART.Init.WordLength = UART_WORDLENGTH_8B;    break;
    }

    switch(xStop)
    {
    case    SERIAL_STOP_BITS_1: pSerial->xUART.Init.StopBits = UART_STOPBITS_1;    break;
	case    SERIAL_STOP_BITS_2: pSerial->xUART.Init.StopBits = UART_STOPBITS_2;    break;
    default:                    pSerial->xUART.Init.StopBits = UART_STOPBITS_1;    break;
    }

    pSerial->xUART.Init.Mode = UART_MODE_TX_RX;
    pSerial->xUART.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    pSerial->xUART.Init.OverSampling = UART_OVERSAMPLING_16;

	pSerial->xSemaphore = xSemaphoreCreateMutex();
	if (pSerial->xSemaphore == NULL)
    {
        return  RET_ERROR;
    }

    if (HAL_UART_Init(&pSerial->xUART) != HAL_OK)
    {
        if (pSerial->xSemaphore != NULL)
        {
            vSemaphoreDelete(pSerial->xSemaphore);
            pSerial->xSemaphore = NULL;
        }
        
        return  RET_ERROR;
    }
    
    return  RET_OK;
}

/*-----------------------------------------------------------*/

RET_VALUE       SERIAL_close(SERIAL* pSerial)
{
    assert(pSerial != NULL);

	if (pSerial->xSemaphore != NULL)
    {
        vSemaphoreDelete(pSerial->xSemaphore);
        pSerial->xSemaphore = NULL;
    }
    
    if (HAL_UART_DeInit(&pSerial->xUART) != HAL_OK)
    {
        return  RET_ERROR;
    }
    
    return  RET_OK;        
}

/*-----------------------------------------------------------*/

RET_VALUE   SERIAL_puts(SERIAL* pSerial, const char * const pData, unsigned long ulLength, TickType_t xBlockTime)
{
    assert(pSerial != NULL);

    RET_VALUE   xRet = RET_OK;
    
    if( xSemaphoreTake( pSerial->xSemaphore, xBlockTime) != pdPASS )
    {
        xRet = RET_ERROR;
    }
    else
    {        
        if (HAL_UART_Transmit(&pSerial->xUART, (uint8_t *)pData, ulLength, xBlockTime) != HAL_OK)
        {
            xRet = RET_ERROR;
        }

        xSemaphoreGive( pSerial->xSemaphore);
    }
    
    return  xRet;
}

/*-----------------------------------------------------------*/

char            SERIAL_getc(SERIAL* pSerial, char *pValue, TickType_t xBlockTime)
{
    assert(pSerial != NULL);
    
    if (HAL_UART_Receive(&pSerial->xUART, (uint8_t *)pValue, 1, xBlockTime) != HAL_OK)
    {
        return  RET_ERROR;
    }
    
    return  RET_OK;
}

/*-----------------------------------------------------------*/

char            SERIAL_putc(SERIAL* pSerial, char cValue, TickType_t xBlockTime)
{
    assert(pSerial != NULL);

    RET_VALUE   xRet = RET_OK;
    
    if( xSemaphoreTake( pSerial->xSemaphore, xBlockTime) != pdPASS )
    {
        xRet = RET_ERROR;
    }
    else
    {        
        if (HAL_UART_Transmit(&pSerial->xUART, (uint8_t *)&cValue, 1, xBlockTime) != HAL_OK)
        {
            xRet = RET_ERROR;
        }
        
        xSemaphoreGive( pSerial->xSemaphore);
    }
    
    return  xRet;
}
