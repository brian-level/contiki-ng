
#define LOG_CONF_LEVEL_RPL                          LOG_LEVEL_WARN
#define LOG_CONF_LEVEL_MPL                          LOG_LEVEL_ERR
#define LOG_CONF_LEVEL_TCPIP                        LOG_LEVEL_WARN
#define LOG_CONF_LEVEL_IPV6                         LOG_LEVEL_WARN
#define LOG_CONF_LEVEL_6LOWPAN                      LOG_LEVEL_WARN
#define LOG_CONF_LEVEL_MAC                          LOG_LEVEL_ERR
#define LOG_CONF_LEVEL_FRAMER                       LOG_LEVEL_ERR
#define LOG_CONF_LEVEL_TSCH                         LOG_LEVEL_ERR
#define LOG_CONF_LEVEL_EFR32                        LOG_LEVEL_WARN

//#define TSCH_LOG_CONF_PER_SLOT                      0
//#define TSCH_ADAPTIVE_TIME_SYNC                     1

#define QUEUEBUF_CONF_NUM   32

#define ROLL_TM_CONF_BUFF_NUM   6
#define MPL_CONF_SEED_SET_SIZE   6

#include "net/ipv6/multicast/uip-mcast6-engines.h"

//#define UIP_MCAST6_CONF_ENGINE UIP_MCAST6_ENGINE_MPL
#define UIP_MCAST6_CONF_ENGINE UIP_MCAST6_ENGINE_ROLL_TM

/* For Imin: Use 16 over CSMA, 64 over Contiki MAC */
#define MPL_CONF_DATA_MESSAGE_IMIN      16
#define MPL_CONF_CONTROL_MESSAGE_IMIN   16

#define UIP_CONF_ND6_SEND_RA         0
#define UIP_CONF_ROUTER              1
#define UIP_MCAST6_ROUTE_CONF_ROUTES 1

#define NETSTACK_MAX_ROUTE_ENTRIES   32

#define NBR_TABLE_CONF_MAX_NEIGHBORS 64
#define UIP_CONF_MAX_ROUTES          64

#define MPL_CONF_DOMAIN_SET_SIZE 2

#undef UIP_CONF_TCP
#define UIP_CONF_TCP 0

#define SICLOWPAN_CONF_FRAG 1
#define SICSLOWPAN_CONF_REASS_CONTEXTS  16

#define UIP_CONF_BROADCAST 1
#define UIP_CONF_REASSEMBLY 1
#define UIP_CONF_IPV6_REASSEMBLY 1
