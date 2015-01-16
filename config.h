/**
 * \file config.h
 *
 *
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "stdint.h"
#include "stdbool.h"

/*- Definitions ------------------------------------------------------------*/
#define HAL_UART_CHANNEL        1
#define HAL_UART_RX_FIFO_SIZE   1
#define HAL_UART_TX_FIFO_SIZE   1024

#define LED_NETWORK       1
#define LED_DATA          0

#define SYS_SECURITY_MODE                   0

#define NWK_BUFFERS_AMOUNT                  10
#define NWK_DUPLICATE_REJECTION_TABLE_SIZE  50
#define NWK_DUPLICATE_REJECTION_TTL         2000 // ms
#define NWK_ROUTE_TABLE_SIZE                100
#define NWK_ROUTE_DEFAULT_SCORE             3
#define NWK_ACK_WAIT_TIME                   1000 // ms
#define NWK_GROUPS_AMOUNT                   3
#define NWK_ROUTE_DISCOVERY_TABLE_SIZE      5
#define NWK_ROUTE_DISCOVERY_TIMEOUT         1000 // ms

#define NWK_ENABLE_ROUTING
//#define NWK_ENABLE_SECURITY
//#define NWK_ENABLE_ROUTE_DISCOVERY

#define PHY_ENABLE_RANDOM_NUMBER_GENERATOR


#endif // _CONFIG_H_
