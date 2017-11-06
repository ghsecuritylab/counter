/* Standard includes. */
#include "string.h"
#include "stdio.h"
#include "assert.h"
#include "ip4_addr.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "FreeRTOS_CLI.h"
#include "stm32f1xx_hal.h"
#include "shell.h"
#include "config.h"
#include "sys.h"
#include "bill_counter.h"
#include "system.h"
#include "counterif.h"
#include "serverif.h"
#include "image.h"

#define cmdMAX_INPUT_SIZE		    50
#define cmdQUEUE_LENGTH			    25
#define cmdASCII_DEL		        0x7F

#define cmdMAX_MUTEX_WAIT		pdMS_TO_TICKS( 300 )

#ifndef configCLI_BAUD_RATE
	#define configCLI_BAUD_RATE	SERIAL_BAUDRATE_115200
#endif

/*-----------------------------------------------------------*/
extern  const BILL_COUNTER*   pBC;

static  SERIAL_PORT         xPort;
static  SERIAL_BAUDRATE     xBaudrate;
static  SERIAL_PARITY       xParity;
static  SERIAL_STOP_BITS    xStopBits;
static  SERIAL_DATA_BITS    xDataBits;

static  SERIAL_HANDLE       hSerial;
static  osThreadId          xThread;

/*
 * The task that implements the command console processing.
 */
void SHELL_main( void const *pvParameters );
RET_VALUE   SHELL_getIP(char* pPrompt, ip4_addr_t* pOldIP, ip4_addr_t* pNewIP);

/*-----------------------------------------------------------*/
extern   CONFIG xConfig;

/* Const messages output by the command console. */
static const char * const pInputPassword = "Input Password : ";
static const char * const pPrompt1 = "Counter>> ";
static const char * const pPrompt2 = 
"============ Main Menu ============\r\n"
"\r\n"
" 1. Set DHCP or Static IP .........\r\n"
"\r\n"
" 2. Set MAC Address ...............\r\n"
"\r\n"
" 3. Set Server IP Address .........\r\n"
"\r\n"
" 4. Set Server Port No ............\r\n"
"\r\n"
" 5. Select Counter Keeper .........\r\n"
"\r\n"
" d. Display Env....................\r\n"
"\r\n"
" f. New firmware Download..........\r\n"
"\r\n"
" p. Password Change ...............\r\n"
"\r\n"
" s. Serial Number Setting..........\r\n"
"\r\n"
" R. System Reset and Restart!......\r\n"
"\r\n"
" q. Test Quit and Out!.............\r\n"
"\r\n"
"===================================\r\n"
"\r\n"
"CVB_Test>> : ";
static const char * const pNewLine = "\r\n";

/*-----------------------------------------------------------*/
RET_VALUE   SHELL_start(BILL_COUNTER* pBC)
{
    RET_VALUE   xRet = RET_OK;

    if (xThread == NULL)
    {
        xPort       = SERIAL_PORT_4;
        xBaudrate   = configCLI_BAUD_RATE;
        xDataBits   = SERIAL_DATA_BITS_8;
        xParity     = SERIAL_PARITY_NONE;
        xStopBits   = SERIAL_STOP_BITS_1;
    
        osThreadDef(shellTask, SHELL_main, CONFIG_SHELL_PRIORITY, 0, CONFIG_SHELL_STACK_SIZE);
        xThread = osThreadCreate(osThread(shellTask), pBC);
        if (xThread == NULL)
        {
            xRet = RET_ERROR;
        }
    }

    return  xRet;
}

/*-----------------------------------------------------------*/
RET_VALUE   SHELL_stop(void)
{
    if (xThread != 0)
    {
        osThreadTerminate(xThread);
        xThread = 0;
    }
         
    return RET_OK;
}

/*-----------------------------------------------------------*/

void SHELL_main( void const *pParams )
{
    RET_VALUE   xRet;
    BILL_COUNTER*   pBC = (BILL_COUNTER*)pParams;
    int nLineLength = 0;
    static char pInputLine[ cmdMAX_INPUT_SIZE ];
    
	/* Initialise the UART. */
    xRet = SERIAL_open(xPort, xBaudrate, xParity, xStopBits, xDataBits, 256, &hSerial);
    if (xRet != RET_OK)
    {
        return ;
    }

	for( ;; )
	{
        memset(pInputLine, 0, sizeof(pInputLine));
        
        SERIAL_puts(hSerial, pPrompt1, strlen(pPrompt1), HAL_MAX_DELAY );
        nLineLength = SHELL_getLine(pInputLine, sizeof(pInputLine), 0);

        if (nLineLength == 0)
        {
            continue;
        }

        if ((nLineLength == 1) && (pInputLine[0] == '?'))
        {
            /* Send the welcome message. */
            SERIAL_printf(hSerial, "==============================================================\r\n");
            SERIAL_printf(hSerial, "=           (C) COPYRIGHT 2010 FutureTek,Inc., Ltd           =\r\n");
            SERIAL_printf(hSerial, "=                                                            =\r\n");
            SERIAL_printf(hSerial, "=    CV H/W Diagnostic Test Program (Version %s)    =\r\n", pFirmwareVersion);
            SERIAL_printf(hSerial, "=                                                            =\r\n");
            SERIAL_printf(hSerial, "==============================================================\r\n");

            SERIAL_puts(hSerial, pInputPassword, strlen(pInputPassword), HAL_MAX_DELAY );
            nLineLength = SHELL_getLine(pInputLine, sizeof(pInputLine), 1);
            if (strcmp(pInputLine, xConfig.pPasswd) == 0)
            {
                bool    bStop = false;
                
                while( !bStop )
                {
                    int bUpdatedConfig = 0;
                    SERIAL_puts(hSerial, pPrompt2, strlen(pPrompt2), HAL_MAX_DELAY);
                    nLineLength = SHELL_getLine(pInputLine, sizeof(pInputLine), 0);

                    if (nLineLength == 0)
                    {
                        continue;
                    }
                    
                    if (nLineLength != 1)
                    {
                        continue;
                    }
                    
                    switch(pInputLine[0])
                    {
                    case '1':
                        {
                            SERIAL_printf(hSerial, "Set IP Mode (DHCP[0]/STATIC[1]) : ");
                            nLineLength = SHELL_getLine(pInputLine, sizeof(pInputLine), 0);
                            if ((nLineLength == 1) && (pInputLine[0] == '0'))
                            {
                                xConfig.xNet.bStatic = 0;
                                bUpdatedConfig = 1;
                                SERIAL_printf(hSerial, "IP Mode is DHCP\n");
                            }
                            else if ((nLineLength == 1) && (pInputLine[0] == '1'))
                            {
                                ip4_addr_t  xIPAddr;
                                ip4_addr_t  xNetmask;
                                ip4_addr_t  xGatewayIPAddr;
                                
                                if (SHELL_getIP("IP Address", &xConfig.xNet.xIPAddr, &xIPAddr) != RET_OK)
                                {
                                    break;
                                }
                                
                                if (SHELL_getIP("Subnet Mask", &xConfig.xNet.xNetmask, &xNetmask) != RET_OK)
                                {
                                    break;
                                }
                                
                                if (SHELL_getIP("Gateway IP Address", &xConfig.xNet.xGatewayIPAddr, &xGatewayIPAddr) != RET_OK)
                                {
                                    break;
                                }
                                                                
                                SERIAL_printf(hSerial, "%16s : %s\n", "Mode",       "STATIC");
                                SERIAL_printf(hSerial, "%16s : %s\n", "IP Address", ip4addr_ntoa(&xIPAddr));
                                SERIAL_printf(hSerial, "%16s : %s\n", "Netmask",    ip4addr_ntoa(&xNetmask));
                                SERIAL_printf(hSerial, "%16s : %s\n", "Gateway",    ip4addr_ntoa(&xGatewayIPAddr));                                         
                                
                                SERIAL_printf(hSerial, "Is the network setting you entered correct(y/n)? : ");
                                nLineLength = SHELL_getLine(pInputLine, 1, 0);
                                if ((nLineLength == 1) && strcasecmp(pInputLine, "y"))
                                {
                                    strncpy(xConfig.pSerialNumber, pInputLine, sizeof(xConfig.pSerialNumber) - 1);
                                
                                    xConfig.xNet.bStatic = 1;                            
                                    xConfig.xNet.xIPAddr = xIPAddr;
                                    xConfig.xNet.xNetmask= xNetmask;
                                    xConfig.xNet.xGatewayIPAddr=xGatewayIPAddr;
                                    
                                    bUpdatedConfig = 1;
                                    
                                    SERIAL_printf(hSerial, "%16s : %s\n", "Network settings have changed.",    "STATIC");
                                }
                            }
                            else
                            {
                                SERIAL_printf(hSerial, "You hae entered an incorrect value\n");
                            }
                        }
                        break;
                            
                    case '2':
                        {
                            uint8_t pMAC[6];
                            uint8_t nValue = 0;
                            
                            SERIAL_printf(hSerial, "Set MAC(%02x-%02x-%02x-%02x-%02x-%02x) : ", 
                                          xConfig.xNet.pMAC[0], xConfig.xNet.pMAC[1], 
                                          xConfig.xNet.pMAC[2], xConfig.xNet.pMAC[3], 
                                          xConfig.xNet.pMAC[4], xConfig.xNet.pMAC[5]);
                            nLineLength = SHELL_getLine(pInputLine, sizeof(pInputLine), 0);
                            if (nLineLength == 17)
                            {
                                bool    bInvalid = 0;
                                
                                for(uint32_t i = 0 ; (bInvalid == false) && i < 17 ; i++)
                                {
                                    switch(i % 3)
                                    {
                                    case    0:
                                        {
                                            if ('0' <= pInputLine[i] && pInputLine[i] <= '9')
                                            {
                                                nValue = (pInputLine[i] - '0') << 4;
                                            }
                                            else if ('A' <= pInputLine[i] && pInputLine[i] <= 'F')
                                            {
                                                nValue = (pInputLine[i] - 'A' + 10) << 4;
                                            }
                                            else if ('a' <= pInputLine[i] && pInputLine[i] <= 'f')
                                            {
                                                nValue = (pInputLine[i] - 'a' + 10) << 4;
                                            }
                                        }
                                        break;
                                        
                                    case    1:
                                        {
                                            if ('0' <= pInputLine[i] && pInputLine[i] <= '9')
                                            {
                                                nValue |= (pInputLine[i] - '0');
                                            }
                                            else if ('A' <= pInputLine[i] && pInputLine[i] <= 'F')
                                            {
                                                nValue |= (pInputLine[i] - 'A' + 10);
                                            }
                                            else if ('a' <= pInputLine[i] && pInputLine[i] <= 'f')
                                            {
                                                nValue |= (pInputLine[i] - 'a' + 10);
                                            }
                                            
                                            pMAC[i/3] = nValue;
                                        }
                                        break;
                                        
                                    case    2:
                                        {
                                            if ((pInputLine[i] != '-') && (pInputLine[i] != ':'))
                                            {
                                                bInvalid = true;
                                            }
                                        }
                                        break;
                                    }
                                }
                                        
                                if (!bInvalid)
                                {
                                    memcpy(xConfig.xNet.pMAC, pMAC, 6);
                                    
                                    bUpdatedConfig = 1;
                                    SERIAL_printf(hSerial, "The MAC changed to %02x-%02x-%02x-%02x-%02x-%02x\n",
                                                  xConfig.xNet.pMAC[0], xConfig.xNet.pMAC[1], 
                                                  xConfig.xNet.pMAC[2], xConfig.xNet.pMAC[3], 
                                                  xConfig.xNet.pMAC[4], xConfig.xNet.pMAC[5]); 
                                 }
                            }
                            else
                            {
                                SERIAL_printf(hSerial, "You hae entered an incorrect value\n");
                            }
                        }
                        break;
                            
                    case '3':
                        {
                            SERIAL_printf(hSerial, "Set Server IP (%s) : ", ip4addr_ntoa(&xConfig.xServer.xIPAddr));
                            nLineLength = SHELL_getLine(pInputLine, sizeof(pInputLine), 0);
                            
                            ip4_addr_t xNewServerIP;
                            if (ip4addr_aton(pInputLine, &xNewServerIP) != 0)
                            {
                                SERIAL_printf(hSerial, "Is the server address you entered correct(y/n)? : ");
                                nLineLength = SHELL_getLine(pInputLine, 1, 0);
                                if ((nLineLength == 1) && strcasecmp(pInputLine, "y"))
                                {
                                    xConfig.xServer.xIPAddr = xNewServerIP;
                                    bUpdatedConfig = 1;
                                }
                            }
                            else
                            {
                                SERIAL_printf(hSerial, "You hae entered an incorrect value\n");
                            }
                        }
                        break;
                            
                    case '4':
                        {
                            int bInvalid = 0;
                            int nPort = 0;
                            
                            SERIAL_printf(hSerial, "Set Server Port (%d) : ", xConfig.xServer.nPort);
                            nLineLength = SHELL_getLine( pInputLine, sizeof(pInputLine), 0);

                            if (nLineLength != 0)
                            {
                                for(uint32_t i = 0 ; i < nLineLength ; i++)
                                {
                                    if (pInputLine[i] < '0' || '9' < pInputLine[i])
                                    {
                                        bInvalid = 1;
                                        break;
                                    }
                                    
                                    nPort = nPort*10 + (pInputLine[i] - '0');
                                }
                                
                                if ((bInvalid == 0) && ((0 < nPort) && (nPort < 65536)))
                                {
                                    SERIAL_printf(hSerial, "Is the server port you entered correct(y/n)? : ");
                                    nLineLength = SHELL_getLine(pInputLine, 16, 0);
                                    if ((nLineLength == 1) && strcasecmp(pInputLine, "y"))
                                    {
                                           xConfig.xServer.nPort = nPort;
                                            bUpdatedConfig = 1;
                                    }
                                }
                                else
                                {
                                    SERIAL_printf(hSerial, "You hae entered an incorrect value\n");
                                }
                            }
                        }
                        break;
                            
                    case  'd':
                        {
                            SERIAL_printf(hSerial, "%20s : %s\n", "Model", pBC->pInfo->pName);
                            SERIAL_printf(hSerial, "%20s : %s\n", "Serial Number", xConfig.pSerialNumber);
                            SERIAL_printf(hSerial, "%20s : %s\n", 
                                          "IP Mode",
                                          (xConfig.xNet.bStatic)?"DHCP":"STATIC");
                            SERIAL_printf(hSerial, "%20s : %s\n", "IP Address", ip4addr_ntoa(&xConfig.xNet.xIPAddr));
                            SERIAL_printf(hSerial, "%20s : %s\n", "Netmask", ip4addr_ntoa(&xConfig.xNet.xNetmask));
                            SERIAL_printf(hSerial, "%20s : %s\n", "Gateway", ip4addr_ntoa(&xConfig.xNet.xGatewayIPAddr));
                            SERIAL_printf(hSerial, "%20s : %02x-%02x-%02x-%02x-%02x-%02x\n",
                                          "MAC",
                                          xConfig.xNet.pMAC[0], xConfig.xNet.pMAC[1], 
                                          xConfig.xNet.pMAC[2], xConfig.xNet.pMAC[3], 
                                          xConfig.xNet.pMAC[4], xConfig.xNet.pMAC[5]); 
                            SERIAL_printf(hSerial, "%20s : %s\n", "Server IP Address", ip4addr_ntoa(&xConfig.xServer.xIPAddr));
                            SERIAL_printf(hSerial, "%20s : %d\n", "Server Port", xConfig.xServer.nPort);
                            SERIAL_printf(hSerial, "%20s : %s\n", "F/W No", pFirmwareVersion);
                            
                            BILL_COUNTER_INFO*   pInfos[16];
                            uint32_t    ulCount = BC_GetModelList(pInfos, 16);
                            
                            SERIAL_printf(hSerial, "%20s  : %d\n", "Supported Models", ulCount);
                            for(uint32_t i = 0 ; i < ulCount ; i++)
                            {
                                SERIAL_printf(hSerial, "%20s   %2d - %s\n", "", i+1, pInfos[i]->pName);
                            }
                        }
                        break;

                    case  'f':
                        {
                            SVRIF_stop();
                            CNTIF_stop();
                            SERIAL_close(hSerial);

                            IMAGE_INFO  xInfo;
                            
                            IMAGE_Get(&xInfo);
                            
                            xInfo.nFlags = 0;
                            
                            IMAGE_Set(&xInfo);

                            SYS_reset();
                        }
                        break;

                    case  'p':
                        {
                            SERIAL_printf(hSerial, "%20s : ", "Old Password");
                            nLineLength = SHELL_getLine( pInputLine, sizeof(pInputLine), 0);
                            
                            if (nLineLength == 0)
                            {
                                break;
                            }
                            
                            char    pNewPasswd[32];
                            
                            if (strcmp(xConfig.pPasswd, pInputLine) != 0)
                            {
                                SERIAL_printf(hSerial, "Password do net patch.");
                                break;
                            }

                            memset(pNewPasswd, 0, sizeof(pNewPasswd));
                            SERIAL_printf(hSerial, "%20s : ", "New Password");
                            nLineLength = SHELL_getLine( pNewPasswd, sizeof(pNewPasswd) - 1, 1);
                            if (nLineLength == 0)
                            {
                                break;
                            }
                            
                            SERIAL_printf(hSerial, "%20s : ", "Re-enter New Password");
                            nLineLength = SHELL_getLine( pInputLine, sizeof(pInputLine) - 1, 1);
                            if (nLineLength == 0)
                            {
                                break;
                            }
                            
                            if (strcmp(pNewPasswd, pInputLine) != 0)
                            {
                                SERIAL_printf(hSerial, "New password do net patch.");
                                break;
                            }
                            
                            strcpy(xConfig.pPasswd, pNewPasswd);
                            bUpdatedConfig = 1;
                            SERIAL_printf(hSerial, "Password has been changed.");                        
                        }
                        break;
                        
                     case 's':
                        {
                            char    pSerialNumber[CONFIG_SERIAL_NUMBER_LEN + 1];
                            
                            memset(pSerialNumber, 0, sizeof(pSerialNumber));
                            
                            SERIAL_printf(hSerial, "Set Serial Number (%s) : ", xConfig.pSerialNumber);
                            nLineLength = SHELL_getLine(pSerialNumber, CONFIG_SERIAL_NUMBER_LEN, 0);

                            if (nLineLength > 0)
                            {
                                SERIAL_printf(hSerial, "Is the entered serial number[%s] correct(y/n)? : ", pSerialNumber);
                                nLineLength = SHELL_getLine(pInputLine, 1, 0);
                                if ((nLineLength == 1) && strcasecmp(pInputLine, "y"))
                                {
                                    strncpy(xConfig.pSerialNumber, pSerialNumber, sizeof(xConfig.pSerialNumber) - 1);
                                    bUpdatedConfig = 1;
                                    SERIAL_printf(hSerial, "Serial Number : %s\n", xConfig.pSerialNumber);
                                }
                            }
                            else
                            {
                                SERIAL_printf(hSerial, "You hae entered an incorrect value\n");
                            }
                        }
                        break;
                            
                     case 'R':
                        {
                            SERIAL_printf(hSerial, "Do you want to restart(y/n)? : ");
                            nLineLength = SHELL_getLine(pInputLine, 16, 0);
                            if ((nLineLength == 1) && strcasecmp(pInputLine, "y"))
                            {
                                    SERIAL_close(hSerial);
                                
                                  __DSB();                                                     /* Ensure all outstanding memory accesses included
                                                                                                  buffered write are completed before reset */
                                  SCB->AIRCR  = ((0x5FA << SCB_AIRCR_VECTKEY_Pos)      |
                                                 (SCB->AIRCR & SCB_AIRCR_PRIGROUP_Msk) |
                                                 SCB_AIRCR_SYSRESETREQ_Msk);                   /* Keep priority group unchanged */
                                  __DSB();                                                     /* Ensure completion of memory access */
                                  while(1);
                            }
                        }
                        break;
                            
                     case 'Q':
                        {
                            SERIAL_printf(hSerial, "Do you want to quit(y/n)? : ");
                            nLineLength = SHELL_getLine(pInputLine, 16, 0);
                            if ((nLineLength == 1) && strcasecmp(pInputLine, "y"))
                            {
                                bStop = true;
                            }
                        }
                        break;
                            
                    }
                    
                    if (bUpdatedConfig)
                    {
                        CONFIG_save(&xConfig);
                    }
                }
            }
        }
	}
}


int SHELL_getLine( char* pLine, unsigned int ulMaxLength, int bSecure )
{
    char        cRxedChar;
    int         nReadLength = 0;

	for( ;; )
	{
		while( SERIAL_getc(hSerial, &cRxedChar, 1 ) != RET_OK)
        {
            vTaskDelay(1);
        }

        /* Echo the character back. */
        if (bSecure)
        {
            SERIAL_putc(hSerial, '*', HAL_MAX_DELAY );
        }
        else
        {
            SERIAL_putc(hSerial, cRxedChar, HAL_MAX_DELAY );
        }

        /* Was it the end of the line? */
        if( cRxedChar == '\n')
        {
            /* Just to space the output from the input. */
            SERIAL_puts(hSerial, pNewLine, strlen( pNewLine ), HAL_MAX_DELAY );

            break;
        }
        
        if( ( cRxedChar == '\b' ) || ( cRxedChar == cmdASCII_DEL ) )
        {
            if( nReadLength > 0 )
            {
                nReadLength--;
                pLine[ nReadLength ] = '\0';
            }
        }
        else if( cRxedChar == '\r' )
        {
            // Skip
        }
        else
        {
            if( ( cRxedChar >= ' ' ) && ( cRxedChar <= '~' ) )
            {
                if( nReadLength < ulMaxLength )
                {
                    pLine[ nReadLength ] = cRxedChar;
                    nReadLength++;
                }
            }
        }
	}

    return  nReadLength;
}

RET_VALUE   SHELL_print(char* pString)
{
    /* Just to space the output from the input. */
    SERIAL_puts(hSerial, pString, strlen( pString), HAL_MAX_DELAY );
    
    return  RET_OK;
}

RET_VALUE   SHELL_dump(uint8_t *pBuffer, uint32_t ulLen)
{
    for(uint32_t i = 0 ; i < ulLen ; i++)
    {
        if (i == 0)
        {
            SERIAL_printf(hSerial, "%02x", pBuffer[i]);
        }
        else
        {
            SERIAL_printf(hSerial, " %02x", pBuffer[i]);
        }
    }
    SERIAL_printf(hSerial, "\n");
    
    return  RET_OK;
}

RET_VALUE   SHELL_getIP(char* pPrompt, ip4_addr_t* pOldIP, ip4_addr_t* pNewIP)
{
    char    pInput[32];
    memset(pInput, 0, sizeof(pInput));
    
    SERIAL_printf(hSerial, "%s(%s) : ", pPrompt, ip4addr_ntoa(pOldIP));
    int32_t nLineLength = SHELL_getLine(pInput, sizeof(pInput)-1, 0);

    if (nLineLength == 0)
    {
        *pNewIP = *pOldIP;
    }
    else
    {
        if (ip4addr_aton(pInput, pNewIP) == 0)
        {
            SERIAL_printf(hSerial, "You hae entered an incorrect value\n");
            return  RET_ERROR;
        }
    }

    return  RET_OK;
}