#ifndef SERIAL_H_
#define SERIAL_H_

#include "ret_value.h"
#include "stm32f1xx_hal.h"
#include "semphr.h"

#define SERIAL_MAX_MUTEX_WAIT		pdMS_TO_TICKS( 300 )

typedef struct SERIAL_STRUCT
{
    UART_HandleTypeDef  xUART;
    IRQn_Type           xIRQ;
    SemaphoreHandle_t   xSemaphore;
    SemaphoreHandle_t   xRxWait;
    uint8_t *           pRxBuffer;
    uint32_t            ulRxBufferLength;
    uint32_t            ulRxIndex;
    uint32_t            ulRxLength;
    uint8_t             pTmpBuffer[8];
}   SERIAL, *SERIAL_HANDLE;

typedef enum
{ 
    SERIAL_PORT_1,
    SERIAL_PORT_4,
    SERIAL_PORT_MAX
} SERIAL_PORT;

typedef enum 
{ 
	SERIAL_PARITY_NONE, 
	SERIAL_PARITY_ODD, 
	SERIAL_PARITY_EVEN, 
	SERIAL_PARITY_MARK, 
	SERIAL_PARITY_SPACE
} SERIAL_PARITY;

typedef enum 
{ 
	SERIAL_STOP_BITS_1, 
	SERIAL_STOP_BITS_2 
} SERIAL_STOP_BITS;

typedef enum 
{ 
	SERIAL_DATA_BITS_5, 
	SERIAL_DATA_BITS_6, 
	SERIAL_DATA_BITS_7, 
	SERIAL_DATA_BITS_8 
} SERIAL_DATA_BITS;

typedef enum 
{ 
	SERIAL_BAUDRATE_50,		
	SERIAL_BAUDRATE_75,		
	SERIAL_BAUDRATE_110,		
	SERIAL_BAUDRATE_134,		
	SERIAL_BAUDRATE_150,    
	SERIAL_BAUDRATE_200,
	SERIAL_BAUDRATE_300,		
	SERIAL_BAUDRATE_600,		
	SERIAL_BAUDRATE_1200,	
	SERIAL_BAUDRATE_1800,	
	SERIAL_BAUDRATE_2400,   
	SERIAL_BAUDRATE_4800,
	SERIAL_BAUDRATE_9600,		
	SERIAL_BAUDRATE_19200,	
	SERIAL_BAUDRATE_38400,	
	SERIAL_BAUDRATE_57600,	
	SERIAL_BAUDRATE_115200
} SERIAL_BAUDRATE;


RET_VALUE       SERIAL_open(SERIAL_PORT xPort, SERIAL_BAUDRATE xBaudrate, SERIAL_PARITY xPartity, SERIAL_STOP_BITS xStop, SERIAL_DATA_BITS xData, unsigned portBASE_TYPE uxBufferLength, SERIAL_HANDLE *phSerial);
RET_VALUE       SERIAL_close(SERIAL_HANDLE hSerial);

unsigned long   SERIAL_gets(SERIAL_HANDLE hSerial, char *pValue, unsigned long ulSize, TickType_t xBlockTime);
RET_VALUE       SERIAL_puts(SERIAL_HANDLE hSerial, const char * const pcString, unsigned long ulLength, TickType_t xBlockTime);
char            SERIAL_getc(SERIAL_HANDLE hSerial, char *pValue, TickType_t xBlockTime);
char            SERIAL_putc(SERIAL_HANDLE hSerial, char cValue, TickType_t xBlockTime);
RET_VALUE       SERIAL_printf(SERIAL_HANDLE   hSerial, const char *pFormat, ... );

RET_VALUE       SERIAL_waitForSemaphore(SERIAL_HANDLE hSerial);

#endif

