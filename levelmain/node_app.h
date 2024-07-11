
#pragma once

#include "contiki.h"
#include "contiki-net.h"
#include <stdint.h>
#include <stdbool.h>
#include "ipcmsg_def.h"

void assert_print(const char *name, const int line);
void LOG_HEXDUMP_INFO(const void *hex, int len, const char *msg);

#define require_noerr(c, l) if((c)) { assert_print(__FILE__, __LINE__); goto l; }
#define require(c, l) if(!(c)) { assert_print(__FILE__, __LINE__); goto l; }

void NodeMsgReceiveMessage(
                            const ipc_transport_t   inTransport,
                            uint8_t                 *inMessage,
                            size_t                  inMsgLen,
                            const uip_ipaddr_t      *inSrcAddr,
                            bool                    inIsMulticast,
                            int                     inRSSI,
                            int                     inHops
                        );
int NodeApplicationSlice(uint32_t *outDelay);
int NodeApplicationInit(const bool inIsRoot);

