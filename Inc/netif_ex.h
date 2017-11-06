#ifndef NETIF_EX_H__
#define NETIF_EX_H__

err_t NETIF_initCB(struct netif *netif);
void NETIF_updateConfig(struct netif *netif);
void NETIF_notifyConnectionChanged(struct netif *netif);
void NETIF_setLink(struct netif *netif);


#endif