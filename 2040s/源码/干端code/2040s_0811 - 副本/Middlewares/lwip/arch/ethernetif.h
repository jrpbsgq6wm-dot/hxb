   /**
 ****************************************************************************************************
 * @file        ethernetif.h
 * @author                  ()
 * @version     V1.0
 * @date        2022-02-14
 * @brief                   
 * @license     Copyright (c) 2020-2032,                           
 ****************************************************************************************************
 * @attention
 *
 *         :                F407      
 *         :www.yuanzige.com
 *         :www.openedv.com
 *         :www..com
 *         :openedv.taobao.com
 *
 *         
 * V1.0 20211014
 *           
 *
 ****************************************************************************************************
 */

#ifndef __ETHERNETIF_H__
#define __ETHERNETIF_H__
#include "lwip/err.h"
#include "lwip/netif.h"


err_t ethernetif_init(struct netif *netif);  /*                */
#endif

