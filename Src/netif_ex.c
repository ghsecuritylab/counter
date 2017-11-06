/* Standard includes. */
#include "string.h"
#include "stdio.h"

/* FreeRTOS includes. */
#include <lwip/sockets.h> 
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "task.h"
#include "semphr.h"
#include "trace.h"
#include "assert.h"
#include "ethernetif.h"
#include "stm32f1xx_hal.h"
#include "netif_ex.h"

static err_t NETIF_lowLevelOutput(struct netif *netif, struct pbuf *p);

err_t NETIF_initCB(struct netif *netif)
{
    LWIP_ASSERT("netif != NULL", (netif != NULL));

#if LWIP_NETIF_HOSTNAME
    /* Initialize interface hostname */
    netif->hostname = "lwip";
#endif /* LWIP_NETIF_HOSTNAME */

    netif->name[0] = 'f';
    netif->name[1] = 't';

#if LWIP_IPV4
#if LWIP_ARP || LWIP_ETHERNET
#if LWIP_ARP
    netif->output = etharp_output;
#else
    /* The user should write ist own code in low_level_output_arp_off function */
    netif->output = low_level_output_arp_off;
#endif /* LWIP_ARP */
#endif /* LWIP_ARP || LWIP_ETHERNET */
#endif /* LWIP_IPV4 */

#if LWIP_IPV6
    netif->output_ip6 = ethip6_output;
#endif /* LWIP_IPV6 */

    netif->linkoutput = NETIF_lowLevelOutput;

    /* initialize the hardware */
    netif->mtu = 1500;

    return ERR_OK;
}

err_t NETIF_lowLevelOutput(struct netif *netif, struct pbuf *p)
{
  err_t errval;
  struct pbuf *q;
  __IO ETH_DMADescTypeDef *DmaTxDesc = ETH_getTxDesc();
  uint8_t *buffer = (uint8_t *)(DmaTxDesc->Buffer1Addr);
  uint32_t framelength = 0;
  uint32_t bufferoffset = 0;
  uint32_t byteslefttocopy = 0;
  uint32_t payloadoffset = 0;
  
  /* copy frame from pbufs to driver buffers */
  for(q = p; q != NULL; q = q->next)
    {
      /* Is this buffer available? If not, goto error */
      if((DmaTxDesc->Status & ETH_DMATXDESC_OWN) != (uint32_t)RESET)
      {
        errval = ERR_USE;
        goto error;
      }
    
      /* Get bytes in current lwIP buffer */
      byteslefttocopy = q->len;
      payloadoffset = 0;
    
      /* Check if the length of data to copy is bigger than Tx buffer size*/
      while( (byteslefttocopy + bufferoffset) > ETH_TX_BUF_SIZE )
      {
        /* Copy data to Tx buffer*/
        memcpy( (uint8_t*)((uint8_t*)buffer + bufferoffset), (uint8_t*)((uint8_t*)q->payload + payloadoffset), (ETH_TX_BUF_SIZE - bufferoffset) );
      
        /* Point to next descriptor */
        DmaTxDesc = (ETH_DMADescTypeDef *)(DmaTxDesc->Buffer2NextDescAddr);
      
        /* Check if the buffer is available */
        if((DmaTxDesc->Status & ETH_DMATXDESC_OWN) != (uint32_t)RESET)
        {
          errval = ERR_USE;
          goto error;
        }
      
        buffer = (uint8_t *)(DmaTxDesc->Buffer1Addr);
      
        byteslefttocopy = byteslefttocopy - (ETH_TX_BUF_SIZE - bufferoffset);
        payloadoffset = payloadoffset + (ETH_TX_BUF_SIZE - bufferoffset);
        framelength = framelength + (ETH_TX_BUF_SIZE - bufferoffset);
        bufferoffset = 0;
      }
    
      /* Copy the remaining bytes */
      memcpy( (uint8_t*)((uint8_t*)buffer + bufferoffset), (uint8_t*)((uint8_t*)q->payload + payloadoffset), byteslefttocopy );
      bufferoffset = bufferoffset + byteslefttocopy;
      framelength = framelength + byteslefttocopy;
    }
  
  /* Prepare transmit descriptors to give to DMA */ 
  ETH_TransmitFrame(framelength);
  
  errval = ERR_OK;
  
error:
  
  /* When Transmit Underflow flag is set, clear it and issue a Transmit Poll Demand to resume transmission */
  if ((ETH_getDMAStatusRegister() & ETH_DMASR_TUS) != (uint32_t)RESET)
  {
    /* Clear TUS ETHERNET DMA flag */
    ETH_setDMAStatusRegister(ETH_DMASR_TUS);

    /* Resume DMA transmission*/
    ETH_resumeDMATransmission();
  }
  return errval;
}

#if LWIP_NETIF_LINK_CALLBACK
/**
  * @brief  Link callback function, this function is called on change of link status
  *         to update low level driver configuration.
* @param  netif: The network interface
  * @retval None
  */
void NETIF_updateConfig(struct netif *netif)
{
  __IO uint32_t tickstart = 0;
  uint32_t regvalue = 0;
  
  if(netif_is_link_up(netif))
  { 
    /* Restart the auto-negotiation */
    if(ETH_getAutoNego() != ETH_AUTONEGOTIATION_DISABLE)
    {
      /* Enable Auto-Negotiation */
      ETH_WritePHYRegister(PHY_BCR, PHY_AUTONEGOTIATION);
      
      /* Get tick */
      tickstart = HAL_GetTick();
      
      /* Wait until the auto-negotiation will be completed */
      do
      {
        ETH_ReadPHYRegister(PHY_BSR, &regvalue);
        
        /* Check for the Timeout ( 1s ) */
        if((HAL_GetTick() - tickstart ) > 1000)
        {     
          /* In case of timeout */ 
          goto error;
        }   
      } while (((regvalue & PHY_AUTONEGO_COMPLETE) != PHY_AUTONEGO_COMPLETE));
      
      /* Read the result of the auto-negotiation */
      ETH_ReadPHYRegister(PHY_SR, &regvalue);
      
      /* Configure the MAC with the Duplex Mode fixed by the auto-negotiation process */
      if((regvalue & PHY_DUPLEX_STATUS) != (uint32_t)RESET)
      {
        /* Set Ethernet duplex mode to Full-duplex following the auto-negotiation */
        ETH_setDuplexMode(ETH_MODE_FULLDUPLEX);  
      }
      else
      {
        /* Set Ethernet duplex mode to Half-duplex following the auto-negotiation */
        ETH_setDuplexMode(ETH_MODE_HALFDUPLEX);           
      }
      /* Configure the MAC with the speed fixed by the auto-negotiation process */
      if(regvalue & PHY_SPEED_STATUS)
      {  
        /* Set Ethernet speed to 10M following the auto-negotiation */
        ETH_setSpeed(ETH_SPEED_10M); 
      }
      else
      {   
        /* Set Ethernet speed to 100M following the auto-negotiation */ 
        ETH_setSpeed(ETH_SPEED_100M);
      }
    }
    else /* AutoNegotiation Disable */
    {
    error :
      /* Check parameters */
      assert_param(IS_ETH_SPEED(ETH_getSpeed()));
      assert_param(IS_ETH_DUPLEX_MODE(ETH_getDuplexMode()));
      
      /* Set MAC Speed and Duplex Mode to PHY */
      ETH_WritePHYRegister(PHY_BCR, ((uint16_t)(ETH_getDuplexMode() >> 3) |(uint16_t)(ETH_getSpeed() >> 1))); 
    }

    /* ETHERNET MAC Re-Configuration */
    ETH_configMAC((ETH_MACInitTypeDef *) NULL);

    /* Restart MAC interface */
    ETH_start();   
  }
  else
  {
    /* Stop MAC interface */
    ETH_stop();
  }

  NETIF_notifyConnectionChanged(netif);
}

/* USER CODE BEGIN 8 */
/**
  * @brief  This function notify user about link status changement.
  * @param  netif: the network interface
  * @retval None
  */
__weak void NETIF_notifyConnectionChanged(struct netif *netif)
{
  /* NOTE : This is function could be implemented in user file 
            when the callback is needed,
  */

}
/* USER CODE END 8 */ 
#endif /* LWIP_NETIF_LINK_CALLBACK */

/**
  * @brief  This function sets the netif link status.
  * @param  netif: the network interface
  * @retval None
  */
void NETIF_setLink(struct netif *netif)
{
  uint32_t regvalue = 0;
  
  /* Read PHY_MISR*/
  ETH_ReadPHYRegister(PHY_MISR, &regvalue);
  
  /* Check wxETHer the link interrupt has occurred or not */
  if((regvalue & PHY_LINK_INTERRUPT) != (uint16_t)RESET)
  {
    /* Read PHY_SR*/
    ETH_ReadPHYRegister(PHY_SR, &regvalue);
    
    /* Check wxETHer the link is up or down*/
    if((regvalue & PHY_LINK_STATUS)!= (uint16_t)RESET)
    {
      netif_set_link_up(netif);
    }
    else
    {
      netif_set_link_down(netif);
    }
  }
}
