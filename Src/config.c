#include <string.h>
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_flash.h"
#include "config.h"
#include "crc16.h"

#define CONFIG_ADDRESS  0x08030000

const   char pFirmwareVersion[] = "17.09.17.00";
CONFIG* pStoredConfig = (CONFIG* )CONFIG_ADDRESS;

RET_VALUE   CONFIG_init(void)
{
    CONFIG* pStoredConfig = (CONFIG* )CONFIG_ADDRESS;
    
    if (pStoredConfig->nCRC != CRC16_calc((uint16_t *)pStoredConfig + 1, sizeof(CONFIG) - sizeof(uint16_t)))
    {
        CONFIG* pConfig = pvPortMalloc(sizeof(CONFIG));
        if (pConfig != NULL)
        {
            CONFIG_setDefault(pConfig);
            CONFIG_save(pConfig);
            
            vPortFree(pConfig);
        }
        else
        {
            return  RET_ERROR;
        }
    }
    
    return  RET_OK;
}

RET_VALUE   CONFIG_setDefault(CONFIG* pConfig)
{
    strcpy(pConfig->pPasswd, CONFIG_DEFAULT_PASSWORD);
    strcpy(pConfig->pSerialNumber, CONFIG_DEFAULT_SERIAL_NUMBER);
    pConfig->xNet.pMAC[0]    =   0x00;
    pConfig->xNet.pMAC[1]    =   0x40;
    pConfig->xNet.pMAC[2]    =   0x5c;
    pConfig->xNet.pMAC[3]    =   0x01;
    pConfig->xNet.pMAC[4]    =   0x02;
    pConfig->xNet.pMAC[5]    =   0x03;

    pConfig->xNet.bStatic = 0;
    IP4_ADDR(&pConfig->xNet.xIPAddr, 192, 168, 0, 200);
    IP4_ADDR(&pConfig->xNet.xNetmask, 255, 255, 255, 0);
    IP4_ADDR(&pConfig->xNet.xGatewayIPAddr, 192, 168, 0, 1);

    IP4_ADDR(&pConfig->xServer.xIPAddr, 192, 168, 1, 1);
    pConfig->xServer.nPort = 12004;
    
    return  RET_OK;
}

RET_VALUE   CONFIG_load(CONFIG* pConfig)
{
    CONFIG* pStoredConfig = (CONFIG* )CONFIG_ADDRESS;
    
    uint16_t    nCRC = CRC16_calc((uint16_t *)pStoredConfig + 1, sizeof(CONFIG) - sizeof(uint16_t));
    
    if (nCRC != pStoredConfig->nCRC)
    {
        return  RET_ERROR;
    }
    
    memcpy(pConfig, pStoredConfig, sizeof(CONFIG));
    
    return  RET_OK;
}

RET_VALUE   CONFIG_save(CONFIG* pConfig)
{
    CONFIG* pStoredConfig = (CONFIG* )CONFIG_ADDRESS;

    pConfig->nCRC = CRC16_calc((uint16_t *)pConfig + 1, sizeof(CONFIG) - sizeof(uint16_t));
    if (memcmp(pConfig, pStoredConfig, sizeof(CONFIG)) == 0)
    {
        return  RET_OK;
    }
        
    uint32_t*   pWord = (uint32_t*)pConfig;
    
    static  FLASH_EraseInitTypeDef  xEraseInit;
    
    xEraseInit.TypeErase    =   FLASH_TYPEERASE_PAGES;
    xEraseInit.PageAddress  =   CONFIG_ADDRESS;
    xEraseInit.NbPages      =   1;
    
    HAL_FLASH_Unlock();
    uint32_t    xPageError = 0;
    if (HAL_FLASHEx_Erase(&xEraseInit, &xPageError) != HAL_OK)
    {
        return  RET_ERROR;
    }

    for( int i=0; i< (sizeof( CONFIG ) + 3) / 4 ; i++)
    {
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, CONFIG_ADDRESS + (i*4) ,  pWord[i]  );
    }
    HAL_FLASH_Lock();
        
    return  RET_OK;
}
