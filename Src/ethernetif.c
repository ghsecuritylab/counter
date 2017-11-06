/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"
#include "lwip/opt.h"

#include "lwip/timeouts.h"
#include "netif/ethernet.h"
#include "netif/etharp.h"
#include "lwip/ethip6.h"
#include "ethernetif.h"
#include <string.h>
#include "cmsis_os.h"

/* Private define ------------------------------------------------------------*/
/* The time to block waiting for input. */
#define TIME_WAITING_FOR_INPUT ( portMAX_DELAY )
/* Stack size of the interface thread */
#define INTERFACE_THREAD_STACK_SIZE ( 350 )
/* Network interface name */
#define IFNAME0 's'
#define IFNAME1 't'

/* Private variables ---------------------------------------------------------*/
#if defined ( __ICCARM__ ) /*!< IAR Compiler */
  #pragma data_alignment=4   
#endif
__ALIGN_BEGIN ETH_DMADescTypeDef  DMARxDscrTab[ETH_RXBUFNB] __ALIGN_END;/* Ethernet Rx MA Descriptor */

#if defined ( __ICCARM__ ) /*!< IAR Compiler */
  #pragma data_alignment=4   
#endif
__ALIGN_BEGIN ETH_DMADescTypeDef  DMATxDscrTab[ETH_TXBUFNB] __ALIGN_END;/* Ethernet Tx DMA Descriptor */

#if defined ( __ICCARM__ ) /*!< IAR Compiler */
  #pragma data_alignment=4   
#endif
__ALIGN_BEGIN uint8_t Rx_Buff[ETH_RXBUFNB][ETH_RX_BUF_SIZE] __ALIGN_END; /* Ethernet Receive Buffer */

#if defined ( __ICCARM__ ) /*!< IAR Compiler */
  #pragma data_alignment=4   
#endif
__ALIGN_BEGIN uint8_t Tx_Buff[ETH_TXBUFNB][ETH_TX_BUF_SIZE] __ALIGN_END; /* Ethernet Transmit Buffer */

/* USER CODE BEGIN 2 */

/* USER CODE END 2 */
static  void ETH_taskMain( void const * argument );

/* Semaphore to signal incoming packets */
osSemaphoreId s_xSemaphore = NULL;
/* Global Ethernet handle */
ETH_HandleTypeDef xETH;

extern  CONFIG  xConfig;
/* USER CODE BEGIN 3 */

/* USER CODE END 3 */

/* Private functions ---------------------------------------------------------*/

void HAL_ETH_MspInit(ETH_HandleTypeDef* ethHandle)
{
  GPIO_InitTypeDef GPIO_InitStruct;
  if(ethHandle->Instance==ETH)
  {
  /* USER CODE BEGIN ETH_MspInit 0 */

  /* USER CODE END ETH_MspInit 0 */
    /* Enable Peripheral clock */
    __HAL_RCC_ETH_CLK_ENABLE();
  
    /**ETH GPIO Configuration    
    PC1     ------> ETH_MDC
    PC2     ------> ETH_TXD2
    PC3     ------> ETH_TX_CLK
    PA0-WKUP     ------> ETH_CRS
    PA1     ------> ETH_RX_CLK
    PA2     ------> ETH_MDIO
    PA3     ------> ETH_COL
    PA7     ------> ETH_RX_DV
    PC4     ------> ETH_RXD0
    PC5     ------> ETH_RXD1
    PB0     ------> ETH_RXD2
    PB1     ------> ETH_RXD3
    PB10     ------> ETH_RX_ER
    PB11     ------> ETH_TX_EN
    PB12     ------> ETH_TXD0
    PB13     ------> ETH_TXD1
    PB8     ------> ETH_TXD3 
    */
    GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_3|GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* Peripheral interrupt init */
    HAL_NVIC_SetPriority(ETH_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(ETH_IRQn);
  /* USER CODE BEGIN ETH_MspInit 1 */

  /* USER CODE END ETH_MspInit 1 */
  }
}

void HAL_ETH_MspDeInit(ETH_HandleTypeDef* ethHandle)
{
  if(ethHandle->Instance==ETH)
  {
  /* USER CODE BEGIN ETH_MspDeInit 0 */

  /* USER CODE END ETH_MspDeInit 0 */
    /* Disable Peripheral clock */
    __HAL_RCC_ETH_CLK_DISABLE();
  
    /**ETH GPIO Configuration    
    PC1     ------> ETH_MDC
    PC2     ------> ETH_TXD2
    PC3     ------> ETH_TX_CLK
    PA0-WKUP     ------> ETH_CRS
    PA1     ------> ETH_RX_CLK
    PA2     ------> ETH_MDIO
    PA3     ------> ETH_COL
    PA7     ------> ETH_RX_DV
    PC4     ------> ETH_RXD0
    PC5     ------> ETH_RXD1
    PB0     ------> ETH_RXD2
    PB1     ------> ETH_RXD3
    PB10     ------> ETH_RX_ER
    PB11     ------> ETH_TX_EN
    PB12     ------> ETH_TXD0
    PB13     ------> ETH_TXD1
    PB8     ------> ETH_TXD3 
    */
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4 
                          |GPIO_PIN_5);

    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3 
                          |GPIO_PIN_7);

    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_10|GPIO_PIN_11 
                          |GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_8);

    /* Peripheral interrupt Deinit*/
    HAL_NVIC_DisableIRQ(ETH_IRQn);

  /* USER CODE BEGIN ETH_MspDeInit 1 */

  /* USER CODE END ETH_MspDeInit 1 */
  }
}

/**
  * @brief  Ethernet Rx Transfer completed callback
  * @param  xETH: ETH handle
  * @retval None
  */
void HAL_ETH_RxCpltCallback(ETH_HandleTypeDef *xETH)
{
  osSemaphoreRelease(s_xSemaphore);
}

HAL_StatusTypeDef ETH_ReadPHYRegister(uint16_t PHYReg, uint32_t *RegValue)
{
    return  HAL_ETH_ReadPHYRegister(&xETH, PHYReg, RegValue);
}

HAL_StatusTypeDef ETH_WritePHYRegister(uint16_t PHYReg, uint32_t ulRegValue)
{
    return  HAL_ETH_WritePHYRegister(&xETH, PHYReg, ulRegValue);
}

HAL_StatusTypeDef ETH_configMAC(ETH_MACInitTypeDef *pConf)
{
    return  HAL_ETH_ConfigMAC(&xETH, pConf);
}

HAL_StatusTypeDef ETH_start(void)
{
    return  HAL_ETH_Start(&xETH);
}

HAL_StatusTypeDef ETH_stop(void)
{
    return  HAL_ETH_Stop(&xETH);
}

void    ETH_setDuplexMode(uint32_t xMode)
{
    xETH.Init.DuplexMode = xMode;  
}

uint32_t    ETH_getDuplexMode(void)
{
    return  xETH.Init.DuplexMode;  
}

void    ETH_setSpeed(uint32_t xSpeed)
{
    xETH.Init.Speed = xSpeed;  
}

uint32_t    ETH_gsetSpeed(void)
{
    return  xETH.Init.Speed;  
}

uint32_t    ETH_getAutoNego(void)
{
    return  xETH.Init.AutoNegotiation;
}

ETH_DMADescTypeDef *ETH_getTxDesc(void)
{
    return  xETH.TxDesc;
}

HAL_StatusTypeDef ETH_TransmitFrame(uint32_t    ulFrameLength)
{
    return    HAL_ETH_TransmitFrame(&xETH, ulFrameLength);
}

uint32_t    ETH_getDMAStatusRegister(void)
{
    return  xETH.Instance->DMASR;
}

uint32_t    ETH_setDMAStatusRegister(uint32_t ulStatus)
{
    return  xETH.Instance->DMASR = ulStatus;
}

void        ETH_resumeDMATransmission(void)
{
    xETH.Instance->DMATPDR = 0;
}

/*******************************************************************************
                       LL Driver Interface ( LwIP stack --> ETH) 
*******************************************************************************/
/**
 * In this function, the hardware should be initialized.
 * Called from ethernetif_init().
 *
 * @param netif the already initialized lwip network interface structure
 *        for this ethernetif
 */
void ETH_lowLevelInit(ETH_HandleTypeDef *pETH)
{ 
    uint32_t regvalue = 0;
    HAL_StatusTypeDef hal_eth_init_status;

    /* Init ETH */

    pETH->Instance = ETH;
    pETH->Init.AutoNegotiation = ETH_AUTONEGOTIATION_ENABLE;
    pETH->Init.PhyAddress     = DP83848_PHY_ADDRESS;
    pETH->Init.MACAddr        = xConfig.xNet.pMAC;
    pETH->Init.RxMode         = ETH_RXINTERRUPT_MODE;
    pETH->Init.ChecksumMode   = ETH_CHECKSUM_BY_HARDWARE;
    pETH->Init.MediaInterface = ETH_MEDIA_INTERFACE_MII;

    hal_eth_init_status = HAL_ETH_Init(pETH);
    if (hal_eth_init_status == HAL_OK)
    {
    /* Set netif link flag */  
        SVRIF_setFlags(NETIF_FLAG_LINK_UP);
    }

    /* Initialize Tx Descriptors list: Chain Mode */
    HAL_ETH_DMATxDescListInit(pETH, DMATxDscrTab, &Tx_Buff[0][0], ETH_TXBUFNB);
     
    /* Initialize Rx Descriptors list: Chain Mode  */
    HAL_ETH_DMARxDescListInit(pETH, DMARxDscrTab, &Rx_Buff[0][0], ETH_RXBUFNB);

    /* create a binary semaphore used for informing ethernetif of frame reception */
    osSemaphoreDef(SEM);
    s_xSemaphore = osSemaphoreCreate(osSemaphore(SEM) , 1 );

    /* create the task that handles the ETH_MAC */
    osThreadDef(EthIf, ETH_taskMain, osPriorityRealtime, 0, INTERFACE_THREAD_STACK_SIZE);
    osThreadCreate (osThread(EthIf), 0);
    /* Enable MAC and DMA transmission and reception */
    HAL_ETH_Start(pETH);

    SVRIF_setFlags(NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP);

    /**** Configure PHY to generate an interrupt when Eth Link state changes ****/
    /* Read Register Configuration */
    HAL_ETH_ReadPHYRegister(pETH, PHY_MICR, &regvalue);

    regvalue |= (PHY_MICR_INT_EN | PHY_MICR_INT_OE);

    /* Enable Interrupts */
    HAL_ETH_WritePHYRegister(pETH, PHY_MICR, regvalue );

    /* Read Register Configuration */
    HAL_ETH_ReadPHYRegister(pETH, PHY_MISR, &regvalue);

    regvalue |= PHY_MISR_LINK_INT_EN;

    /* Enable Interrupt on change of link status */
    HAL_ETH_WritePHYRegister(pETH, PHY_MISR, regvalue);
}


/**
 * Should allocate a pbuf and transfer the bytes of the incoming
 * packet from the interface into the pbuf.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return a pbuf filled with the received packet (including MAC header)
 *         NULL on memory error
   */
static struct pbuf * ETH_lowLevelInput(void)
{
  struct pbuf *p = NULL;
  struct pbuf *q = NULL;
  uint16_t len = 0;
  uint8_t *buffer;
  __IO ETH_DMADescTypeDef *dmarxdesc;
  uint32_t bufferoffset = 0;
  uint32_t payloadoffset = 0;
  uint32_t byteslefttocopy = 0;
  uint32_t i=0;
  

  /* get received frame */
  if (HAL_ETH_GetReceivedFrame_IT(&xETH) != HAL_OK)
    return NULL;
  
  /* Obtain the size of the packet and put it into the "len" variable. */
  len = xETH.RxFrameInfos.length;
  buffer = (uint8_t *)xETH.RxFrameInfos.buffer;
  
  if (len > 0)
  {
    /* We allocate a pbuf chain of pbufs from the Lwip buffer pool */
    p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
  }
  
  if (p != NULL)
  {
    dmarxdesc = xETH.RxFrameInfos.FSRxDesc;
    bufferoffset = 0;
    for(q = p; q != NULL; q = q->next)
    {
      byteslefttocopy = q->len;
      payloadoffset = 0;
      
      /* Check if the length of bytes to copy in current pbuf is bigger than Rx buffer size*/
      while( (byteslefttocopy + bufferoffset) > ETH_RX_BUF_SIZE )
      {
        /* Copy data to pbuf */
        memcpy( (uint8_t*)((uint8_t*)q->payload + payloadoffset), (uint8_t*)((uint8_t*)buffer + bufferoffset), (ETH_RX_BUF_SIZE - bufferoffset));
        
        /* Point to next descriptor */
        dmarxdesc = (ETH_DMADescTypeDef *)(dmarxdesc->Buffer2NextDescAddr);
        buffer = (uint8_t *)(dmarxdesc->Buffer1Addr);
        
        byteslefttocopy = byteslefttocopy - (ETH_RX_BUF_SIZE - bufferoffset);
        payloadoffset = payloadoffset + (ETH_RX_BUF_SIZE - bufferoffset);
        bufferoffset = 0;
      }
      /* Copy remaining data in pbuf */
      memcpy( (uint8_t*)((uint8_t*)q->payload + payloadoffset), (uint8_t*)((uint8_t*)buffer + bufferoffset), byteslefttocopy);
      bufferoffset = bufferoffset + byteslefttocopy;
    }
  }  
  
    /* Release descriptors to DMA */
    /* Point to first descriptor */
    dmarxdesc = xETH.RxFrameInfos.FSRxDesc;
    /* Set Own bit in Rx descriptors: gives the buffers back to DMA */
    for (i=0; i< xETH.RxFrameInfos.SegCount; i++)
    {  
      dmarxdesc->Status |= ETH_DMARXDESC_OWN;
      dmarxdesc = (ETH_DMADescTypeDef *)(dmarxdesc->Buffer2NextDescAddr);
    }
    
    /* Clear Segment_Count */
    xETH.RxFrameInfos.SegCount =0;  
  
  /* When Rx Buffer unavailable flag is set: clear it and resume reception */
  if ((xETH.Instance->DMASR & ETH_DMASR_RBUS) != (uint32_t)RESET)  
  {
    /* Clear RBUS ETHERNET DMA flag */
    xETH.Instance->DMASR = ETH_DMASR_RBUS;
    /* Resume DMA reception */
    xETH.Instance->DMARPDR = 0;
  }
  return p;
}

/**
 * This function should be called when a packet is ready to be read
 * from the interface. It uses the function ETH_lowLevelInput() that
 * should handle the actual reception of bytes from the network
 * interface. Then the type of the received packet is determined and
 * the appropriate input function is called.
 *
 * @param netif the lwip network interface structure for this ethernetif
 */
void ETH_taskMain( void const * argument ) 
{
  struct pbuf *p;
  
  for( ;; )
  {
    if (osSemaphoreWait( s_xSemaphore, TIME_WAITING_FOR_INPUT)==osOK)
    {
      do
      {   
        p = ETH_lowLevelInput( );
        if   (p != NULL)
        {
          if (SVRIF_input(p) != ERR_OK )
          {
            pbuf_free(p);
          }
        }
      } while(p!=NULL);
    }
  }
}

#if !LWIP_ARP
/**
 * This function has to be completed by user in case of ARP OFF.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return ERR_OK if ...
 */
static err_t low_level_output_arp_off(struct netif *netif, struct pbuf *q, const ip4_addr_t *ipaddr)
{  
  err_t errval;
  errval = ERR_OK;
    
/* USER CODE BEGIN 5 */ 
    
/* USER CODE END 5 */  
    
  return errval;
  
}
#endif /* LWIP_ARP */ 

/**
* @brief This function handles Ethernet global interrupt.
*/
void ETH_IRQHandler(void)
{
  /* USER CODE BEGIN ETH_IRQn 0 */

  /* USER CODE END ETH_IRQn 0 */
  HAL_ETH_IRQHandler(&xETH);
  /* USER CODE BEGIN ETH_IRQn 1 */

  /* USER CODE END ETH_IRQn 1 */
}

/* USER CODE END 9 */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

