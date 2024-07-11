
#pragma once

#define STARGATE_UDP_PORT       (1212)

/* If using a different port for unicast use this by defining it
#define STARGATE_UDP_RESP_PORT  (1213)
*/
#ifdef CONFIG_CONTIKI

#include "contiki.h"
#include "net/routing/routing.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "net/ipv6/uiplib.h"

#define IPv6addrToString(a, s, z)   uiplib_ipaddr_snprint(s, z, a)
#define IPv6addrFromString          uiplib_ip6addrconv

bool ContikiAddrPrefixMatch(const uip_ip6addr_t *inAddr);
int ContikiAddrGetMeshPrefix( uip_ip6addr_t *outPrefix );
int ContikiAddrGetAddr( uip_ipaddr_t *outAddr );
bool ContikiAddrIsMulticastAddr( const uip_ipaddr_t *inAddr );
int ContikiAddrGetMulticastAddr( uip_ipaddr_t *outAddr );
int ContikiAddrInit( const char *inMeshPrefixString );

#endif
