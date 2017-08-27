#ifndef SERIAL_H_
#define SERIAL_H_

#include "ret_value.h"
#include "stm32f1xx_hal.h"

#define SERIAL_MAX_MUTEX_WAIT		pdMS_TO_TICKS( 300 )

typedef struct
{
    UART_HandleTypeDef  xUART;
    SemaphoreHandle_t   xSemaphore;
}   SERIAL;

typedef enum
{ 
    SERIAL_PORT_0,
    SERIAL_PORT_1,
    SERIAL_PORT_2,
    SERIAL_PORT_3,
    SERIAL_PORT_4,
    SERIAL_PORT_5,
    SERIAL_PORT_6
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


RET_VALUE       SERIAL_open(SERIAL* pSerial, SERIAL_PORT xPort, SERIAL_BAUDRATE xBaudrate, SERIAL_PARITY xPartity, SERIAL_STOP_BITS xStop, SERIAL_DATA_BITS xData, unsigned portBASE_TYPE uxBufferLength);
RET_VALUE       SERIAL_close(SERIAL* pSerial);

RET_VALUE       SERIAL_puts(SERIAL* pSerial, const char * const pcString, unsigned long ulLength, TickType_t xBlockTime);
char            SERIAL_getc(SERIAL* pSerial, char *pValue, TickType_t xBlockTime);
char            SERIAL_putc(SERIAL* pSerial, char cValue, TickType_t xBlockTime);

RET_VALUE       SERIAL_waitForSemaphore(SERIAL* pSerial);

#endif

