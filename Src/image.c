#include "image.h"
#include "utils.h"
#include "flash_if.h"

#define IMAGE_INFO_SLOT_SIZE    0x100

typedef struct  IMAGE_INFO_SLOT_STRUCT
{
    uint16_t    nCRC16;
    uint32_t    nIndex;
    IMAGE_INFO  xInfo;
}   IMAGE_INFO_SLOT;

static  uint32_t        nBaseAddress = 0x00000000;
static  int32_t         nCurrent = -1;
static  uint32_t        nMaxCount = 0;

bool    IMAGE_Init(uint32_t nAddress, uint32_t nCount)
{
    nBaseAddress    = nAddress;
    nMaxCount       = nCount;

    for(uint32_t i = 0 ; i < nMaxCount ; i++)
    {
        IMAGE_INFO_SLOT*    pSlot = (IMAGE_INFO_SLOT*)(nBaseAddress + i * IMAGE_INFO_SLOT_SIZE);
        
        if (pSlot->nCRC16 == CRC16_Calc(((const uint8_t*)&pSlot->nIndex), sizeof(IMAGE_INFO_SLOT) - sizeof(pSlot->nCRC16)))
        {
            if (nCurrent == -1)
            {
                nCurrent = i;
            }
            else 
            {
                IMAGE_INFO_SLOT*    pCurrentSlot = (IMAGE_INFO_SLOT*)(nBaseAddress + nCurrent * IMAGE_INFO_SLOT_SIZE);
            
               if (pSlot->nIndex < pCurrentSlot->nIndex)
                {
                    nCurrent = i;
                }
            }
        }
    }

    if (nCurrent == -1)
    {
        IMAGE_INFO  xInfo;

        xInfo.nFlags    = 0;
        xInfo.nStart    = 0;
        xInfo.nSize     = 0;
        xInfo.nCRC16    = 0;

        return  IMAGE_Set(&xInfo);
    }
    
     return  true;
}

bool    IMAGE_Get(IMAGE_INFO* pInfo)
{
    if (nCurrent == -1)
    {
        return  false;
    }
    
    IMAGE_INFO_SLOT*    pCurrentSlot = (IMAGE_INFO_SLOT*)(nBaseAddress + nCurrent * IMAGE_INFO_SLOT_SIZE);
    
    pInfo->nCRC16  = pCurrentSlot->xInfo.nCRC16;
    pInfo->nFlags  = pCurrentSlot->xInfo.nFlags;
    pInfo->nSize   = pCurrentSlot->xInfo.nSize;
    pInfo->nStart  = pCurrentSlot->xInfo.nStart;

    uint32_t    nAddress = nBaseAddress + nCurrent * IMAGE_INFO_SLOT_SIZE;
    Serial_PutString("Load Config from ");
    for(uint32_t i = 0 ; i < 8 ; i++)
    {
        uint8_t nNibble = (nAddress >> (28 - i*4) & 0x0F);
        if (nNibble < 10)
        {
            Serial_PutByte('0' + nNibble);
        }
        else
        {
            Serial_PutByte('A' + nNibble - 10);
        }
    }
    Serial_PutByte('\n');
        
    return  true;
}

bool    IMAGE_Set(IMAGE_INFO* pInfo)
{
    IMAGE_INFO_SLOT xSlot;
    
    if (nCurrent != -1)
    {
        IMAGE_INFO_SLOT*    pCurrentSlot = (IMAGE_INFO_SLOT*)(nBaseAddress + nCurrent * IMAGE_INFO_SLOT_SIZE);
        
        xSlot.nIndex = pCurrentSlot->nIndex + 1;
    }
    else
    {
        xSlot.nIndex = 1;
    }
        
    xSlot.xInfo.nCRC16  = pInfo->nCRC16;
    xSlot.xInfo.nFlags  = pInfo->nFlags;
    xSlot.xInfo.nSize   = pInfo->nSize;
    xSlot.xInfo.nStart  = pInfo->nStart;
    
    xSlot.nCRC16 = CRC16_Calc(((const uint8_t*)&xSlot.nIndex), sizeof(xSlot) - sizeof(xSlot.nCRC16));

    nCurrent = (nCurrent + 1) % nMaxCount;
    
    FLASH_If_EraseSignlePage(nBaseAddress + nCurrent * IMAGE_INFO_SLOT_SIZE);
    FLASH_If_Write(nBaseAddress + nCurrent * IMAGE_INFO_SLOT_SIZE, (uint32_t *)&xSlot, sizeof(xSlot));

    return  true;
}

bool    IMAGE_IsVerified(void)
{
    if (nCurrent == -1)
    {
        return  false;
    }
    
    IMAGE_INFO_SLOT* pSlot = (IMAGE_INFO_SLOT*)(nBaseAddress + nCurrent * IMAGE_INFO_SLOT_SIZE);
    
    return  (pSlot->xInfo.nSize != 0) && (pSlot->xInfo.nCRC16 != CRC16_Calc(((const uint8_t*)pSlot->xInfo.nStart), pSlot->xInfo.nSize));
}

bool    IMAGE_IsAutoBoot(void)
{
    if (nCurrent == -1)
    {
        return  false;
    }
    
    IMAGE_INFO_SLOT* pSlot = (IMAGE_INFO_SLOT*)(nBaseAddress + nCurrent * IMAGE_INFO_SLOT_SIZE);
    
    return  (pSlot->xInfo.nFlags & IMAGE_INFO_AUTO_BOOT) != 0;
}

bool    IMAGE_SetAutoBoot(bool bEnable)
{
    if (nCurrent == -1)
    {
        return  false;
    }
    
    IMAGE_INFO  xInfo;
    
    if (IMAGE_Get(&xInfo) != true)
    {
        return  false;
    }
    
    if (bEnable)
    {
        if ((xInfo.nFlags & IMAGE_INFO_AUTO_BOOT) == IMAGE_INFO_AUTO_BOOT)
        {
            return  true;
        }
        
        xInfo.nFlags |= IMAGE_INFO_AUTO_BOOT;
    }
    else
    {
        if ((xInfo.nFlags & IMAGE_INFO_AUTO_BOOT) != IMAGE_INFO_AUTO_BOOT)
        {
            return  true;
        }
        
        xInfo.nFlags &= ~IMAGE_INFO_AUTO_BOOT;
    }

    return  IMAGE_Set(&xInfo);
}
