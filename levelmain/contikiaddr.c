#include "contikiaddr.h"
#include "net/ipv6/uip-ds6.h"
#include <string.h>
#include <stdlib.h>

#include "sys/log.h"
#define LOG_MODULE "addr"
#define LOG_LEVEL LOG_LEVEL_INFO

#include "node_app.h"

#ifdef CONFIG_CONTIKI

static const char UDP_MESH_LOCAL_MCAST_ADDR[] = "ff03::1";
static const char UDP_GLOBAL_MCAST_ADDR[] = "ff05::beef";

static const uint8_t kNodeLocalScope      = 0;  // Node-Local scope
static const uint8_t kInterfaceLocalScope = 1;  // Interface-Local scope
static const uint8_t kLinkLocalScope      = 2;  // Link-Local scope
static const uint8_t kRealmLocalScope     = 3;  // Realm-Local scope
static const uint8_t kAdminLocalScope     = 4;  // Admin-Local scope
static const uint8_t kSiteLocalScope      = 5;  // Site-Local scope
static const uint8_t kOrgLocalScope       = 8;  // Organization-Local scope
static const uint8_t kGlobalScope         = 14; // Global scope

// We keep track of the following addresses
//

static struct
{
    uip_ip6addr_t ml_prefix;
    uint32_t      ml_prefix_length;

    // Mesh-Local address unicast address ML-EID / ULA  - our local address on the mesh
    //
    uip_ip6addr_t ml_eid;
    bool         have_ml_eid;

    // Global-scope unicast address GUA - a global internet address
    //
    uip_ip6addr_t gua;
    bool         have_gua;

    // Mesh-Local multicast address - used to send / receive multicast messages
    //
    uip_ip6addr_t ml_multicast;

    // Gloal multicast address - used to multicast in global scope
    uip_ip6addr_t gs_multicast;

}
mContikiAddr;

/*
static const char *_ScopeName( uint8_t scope )
{
    switch (scope)
    {
    case kNodeLocalScope:       return "node-local";
    case kInterfaceLocalScope:  return "iface-local";
    case kLinkLocalScope:       return "link-local";
    case kRealmLocalScope:      return "realm-local";
    case kAdminLocalScope:      return "admin-local";
    case kSiteLocalScope:       return "site-local";
    case kOrgLocalScope:        return "local";
    case kGlobalScope:          return "global";
    default:                    return "???";
    }
}
*/

bool ContikiAddrPrefixMatch(const uip_ip6addr_t *inAddr)
{
    // assumes length (which is in bits) is even multiple of 8 which is a safe assumption for us
    return 0 == memcmp(inAddr, &mContikiAddr.ml_prefix, mContikiAddr.ml_prefix_length / 8);
}

int ContikiAddrGetMeshPrefix( uip_ip6addr_t *outPrefix )
{
    int error = -1;

    require(outPrefix, exit);

    *outPrefix = mContikiAddr.ml_prefix;
    error = 0;

exit:
    return error;
}

int ContikiAddrChanged( void )
{
    int error = -1;
    char addrstr[64];
    uint8_t state;

    for (int i = 0; i < UIP_DS6_ADDR_NB; i++)
    {
        state = uip_ds6_if.addr_list[i].state;
        if(
                uip_ds6_if.addr_list[i].isused
            &&  (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)
        )
        {
            IPv6addrToString(&uip_ds6_if.addr_list[i].ipaddr, addrstr, sizeof(addrstr));
            LOG_DBG("Addr %d %c %s", i, (state == ADDR_PREFERRED) ? '*' : ' ', addrstr);

            if (state == ADDR_PREFERRED && ContikiAddrPrefixMatch(&uip_ds6_if.addr_list[i].ipaddr))
            {
                memcpy(&mContikiAddr.ml_eid, &uip_ds6_if.addr_list[i].ipaddr, sizeof(uip_ip6addr_t));
                mContikiAddr.have_ml_eid = true;
            }
        }
    }

    error = 0;
    return error;
}

int ContikiAddrGetAddr( uip_ip6addr_t *outAddr )
{
    int error = -1;

    require(outAddr, exit);

    ContikiAddrChanged();

    if (mContikiAddr.have_gua)
    {
        *outAddr = mContikiAddr.gua;
        error = 0;
    }
    else if (mContikiAddr.have_ml_eid)
    {
        *outAddr = mContikiAddr.ml_eid;
        error = 0;
    }
    else
    {
        static int announcer;

        if ((announcer++ % 128) == 0)
        {
            LOG_ERR("No Ip6 Address for node");
        }
    }

exit:
    return error;
}

bool ContikiAddrIsMulticastAddr( const uip_ip6addr_t *inAddr )
{
    return      0 == memcmp(inAddr, &mContikiAddr.ml_multicast, sizeof(uip_ip6addr_t))
            ||  0 == memcmp(inAddr, &mContikiAddr.gs_multicast, sizeof(uip_ip6addr_t));
}

int ContikiAddrGetMulticastAddr( uip_ip6addr_t *outAddr )
{
    int error = -1;

    require(outAddr, exit);

    // BDD - always restrict multicast to realm-local address
    #if 0
    if (mContikiAddr.have_gua)
    {
        *outAddr = mContikiAddr.gs_multicast;
        error = 0;
    }
    else
    #endif
    {
        *outAddr = mContikiAddr.ml_multicast;
        error = 0;
    }

exit:
    return error;
}

int ContikiAddrInit( const char *inMeshPrefixString )
{
    int error = -1;
    char prefix[64];
    char *prefixLength;

    if (inMeshPrefixString)
    {
        strncpy(prefix, inMeshPrefixString, sizeof(prefix) - 1);
        prefix[sizeof(prefix) - 1] = '\0';
        prefixLength = strchr(prefix, '/');
        if (prefixLength)
        {
            *prefixLength++ = '\0';
            mContikiAddr.ml_prefix_length = strtoul(prefixLength, NULL, 0);
        }
        else
        {
            // assume 64 bit prefix
            mContikiAddr.ml_prefix_length = 64;
        }

        IPv6addrFromString(prefix, &mContikiAddr.ml_prefix);
    }
    else
    {
        memcpy(&mContikiAddr.ml_prefix, uip_ds6_default_prefix(), sizeof(uip_ip6addr_t));
        mContikiAddr.ml_prefix_length = 64;
    }

    mContikiAddr.have_ml_eid = false;
    mContikiAddr.have_gua = false;

    error = !IPv6addrFromString(UDP_MESH_LOCAL_MCAST_ADDR, &mContikiAddr.ml_multicast);
    require_noerr(error, exit);

    error = !IPv6addrFromString(UDP_GLOBAL_MCAST_ADDR, &mContikiAddr.gs_multicast);
    require_noerr(error, exit);

    error = 0;

exit:
    return error;
}
#endif

