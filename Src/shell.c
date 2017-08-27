/* Standard includes. */
#include "string.h"
#include "stdio.h"

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* Example includes. */
#include "FreeRTOS_CLI.h"
#define configCOMMAND_INT_MAX_OUTPUT_SIZE   512
        
/* Demo application includes. */
#include "stm32f1xx_hal.h"
#include "shell.h"

/* Dimensions the buffer into which input characters are placed. */
#define cmdMAX_INPUT_SIZE		50

/* Dimentions a buffer to be used by the UART driver, if the UART driver uses a
buffer at all. */
#define cmdQUEUE_LENGTH			25

/* DEL acts as a backspace. */
#define cmdASCII_DEL		( 0x7F )

/* The maximum time to wait for the mutex that guards the UART to become
available. */
#define cmdMAX_MUTEX_WAIT		pdMS_TO_TICKS( 300 )

#ifndef configCLI_BAUD_RATE
	#define configCLI_BAUD_RATE	115200
#endif

/*-----------------------------------------------------------*/

/*
 * The task that implements the command console processing.
 */
void SHELL_main( void const *pvParameters );

/*-----------------------------------------------------------*/

/* Const messages output by the command console. */
static const char * const pcWelcomeMessage = "FreeRTOS command server.\r\nType Help to view a list of registered commands.\r\n\r\n>";
static const char * const pcEndOfOutputMessage = "\r\n[Press ENTER to execute the previous command again]\r\n>";
static const char * const pcNewLine = "\r\n";

RET_VALUE   SHELL_create(SHELL** ppShell)
{
    SHELL*  pShell;
    
    pShell = (SHELL *)pvPortMalloc(sizeof(SHELL));
    if (pShell == NULL)
    {
        return  RET_NOT_ENOUGH_MEMORY;
    }
    
    memset(pShell, 0, sizeof(SHELL));
    
    *ppShell = pShell;
    
    return  RET_OK;
}

RET_VALUE   SHELL_destroy(SHELL** ppShell)
{
    if (*ppShell != NULL)
    {
        SHELL_stop(*ppShell);
   
        vPortFree(*ppShell);
        
        *ppShell = NULL;
    }
    
    return  RET_OK;
}

/*-----------------------------------------------------------*/
RET_VALUE   SHELL_start( SHELL*    pShell, uint16_t usStackSize, UBaseType_t uxPriority )
{
    RET_VALUE   xRet = RET_OK;
    
    osThreadDef(shellTask, SHELL_main, osPriorityNormal, 1, usStackSize);
    pShell->xThread = osThreadCreate(osThread(shellTask), pShell);
    if (pShell->xThread == NULL)
    {
        xRet = RET_ERROR;
    }

    return  xRet;
}

/*-----------------------------------------------------------*/
RET_VALUE   SHELL_stop( SHELL*    pShell)
{
    if (pShell->xThread != 0)
    {
        osThreadTerminate(pShell->xThread);
        pShell->xThread = 0;
    }
         
    return RET_OK;
}

/*-----------------------------------------------------------*/

void SHELL_main( void const *pParams )
{
//    assert(pParams != NULL);
    SHELL*    pShell = (SHELL*)pParams;
    
    RET_VALUE   xRet;
    char    cRxedChar;
    uint8_t ucInputIndex = 0;
    char *pcOutputString;
    static char cInputString[ cmdMAX_INPUT_SIZE ];
    static char cLastInputString[ cmdMAX_INPUT_SIZE ];

	/* Obtain the address of the output buffer.  Note there is no mutual
	exclusion on this buffer as it is assumed only one command console interface
	will be used at any one time. */
	pcOutputString = FreeRTOS_CLIGetOutputBuffer();

	/* Initialise the UART. */
    xRet = SERIAL_open(&pShell->xSerial, SERIAL_PORT_4, SERIAL_BAUDRATE_115200, SERIAL_PARITY_NONE, SERIAL_STOP_BITS_1, SERIAL_DATA_BITS_8, 256);
    if (xRet != RET_OK)
    {
        return ;
    }

	/* Send the welcome message. */
    SERIAL_puts(&pShell->xSerial, pcWelcomeMessage, strlen( pcWelcomeMessage ), HAL_MAX_DELAY );

	for( ;; )
	{
		/* Wait for the next character.  The while loop is used in case
		INCLUDE_vTaskSuspend is not set to 1 - in which case portMAX_DELAY will
		be a genuine block time rather than an infinite block time. */
		while( SERIAL_getc( &pShell->xSerial, &cRxedChar, portMAX_DELAY ) != RET_OK);

        /* Echo the character back. */
        SERIAL_putc(&pShell->xSerial, cRxedChar, portMAX_DELAY );

        /* Was it the end of the line? */
        if( cRxedChar == '\n' || cRxedChar == '\r' )
        {
            /* Just to space the output from the input. */
            SERIAL_puts(&pShell->xSerial, pcNewLine, strlen( pcNewLine ), HAL_MAX_DELAY );

            /* See if the command is empty, indicating that the last command
            is to be executed again. */
            if( ucInputIndex == 0 )
            {
                /* Copy the last command back into the input string. */
                strcpy( cInputString, cLastInputString );
            }

            /* Pass the received command to the command interpreter.  The
            command interpreter is called repeatedly until it returns
            pdFALSE	(indicating there is no more output) as it might
            generate more than one string. */
            BaseType_t  xReturned;
            do
            {
                /* Get the next output string from the command interpreter. */
                xReturned = FreeRTOS_CLIProcessCommand( cInputString, pcOutputString, configCOMMAND_INT_MAX_OUTPUT_SIZE );

                /* Write the generated string to the UART. */
                SERIAL_puts( &pShell->xSerial, pcOutputString, strlen( pcOutputString ), HAL_MAX_DELAY );

            } while( xReturned != pdFALSE );

            /* All the strings generated by the input command have been
            sent.  Clear the input string ready to receive the next command.
            Remember the command that was just processed first in case it is
            to be processed again. */
            strcpy( cLastInputString, cInputString );
            ucInputIndex = 0;
            memset( cInputString, 0x00, cmdMAX_INPUT_SIZE );

            SERIAL_puts( &pShell->xSerial, pcEndOfOutputMessage, strlen( pcEndOfOutputMessage ), HAL_MAX_DELAY );
        }
        else
        {
            if( cRxedChar == '\r' )
            {
                /* Ignore the character. */
            }
            else if( ( cRxedChar == '\b' ) || ( cRxedChar == cmdASCII_DEL ) )
            {
                /* Backspace was pressed.  Erase the last character in the
                string - if any. */
                if( ucInputIndex > 0 )
                {
                    ucInputIndex--;
                    cInputString[ ucInputIndex ] = '\0';
                }
            }
            else
            {
                /* A character was entered.  Add it to the string entered so
                far.  When a \n is entered the complete	string will be
                passed to the command interpreter. */
                if( ( cRxedChar >= ' ' ) && ( cRxedChar <= '~' ) )
                {
                    if( ucInputIndex < cmdMAX_INPUT_SIZE )
                    {
                        cInputString[ ucInputIndex ] = cRxedChar;
                        ucInputIndex++;
                    }
                }
            }
        }
	}
}
