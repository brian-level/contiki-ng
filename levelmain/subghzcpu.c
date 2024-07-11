
#include "contiki.h"
#include <stdio.h>
#include "contiki-lib.h"
#include "contiki-net.h"
#include "net/ipv6/multicast/uip-mcast6.h"
#include "net/link-stats.h"
#include "sys/log.h"
#define LOG_MODULE "MAIN"
#define LOG_LEVEL LOG_LEVEL_INFO

#include "node_app.h"
#include "ipcmsg_def.h"
#include "contikiaddr.h"

// check node app at least every 100ms
//
#define MAX_LOOP_DELAY  (CLOCK_CONF_SECOND / 10)

// used for process_poll in TimeSignalApplicationEvent to call here
process_event_t app_event;

void MCU_ScheduleReboot(uint32_t delay)
{
    LOG_ERR("OOOOOO");
    watchdog_reboot();
}

#ifdef BOOTLOADER_ENABLE
void SystemInit2(void)
{
}
#endif

// On brd4210a, EFR32_PA5 in on EXP header pin 7
//
#ifdef BOARD_IS_DK
#define LEVEL_ROOT_DETECT_PORT  gpioPortA
#define LEVEL_ROOT_DETECT_PIN   (7)
#else
// its an stargate build: sgevk, sgevt, etc.
#define LEVEL_ROOT_DETECT_PORT  gpioPortA
#define LEVEL_ROOT_DETECT_PIN   (7)
#endif

bool is_root_node(void)
{
    // read gpio on dk to determine if we want to be the root node
    //
    bool isroot = false;
#ifdef BOARD_IS_DK
    // there are no accessible gpio on real h/w so they will always be nodes
    //
    // setup detect pin as a pulled-down input, if it is 0 its
    // not attached (non root) and if 1, its tied high
    //
    // attach a jumper wire from EXP header pin 20 (+3.3v) to
    // EXP header pin 13 to make the root node detect active
    //
    GPIO_PinModeSet(LEVEL_ROOT_DETECT_PORT, LEVEL_ROOT_DETECT_PIN, gpioModeInputPull, 0);
    if (GPIO_PinInGet(LEVEL_ROOT_DETECT_PORT, LEVEL_ROOT_DETECT_PIN))
    {
        isroot = true;
    }
#endif
    return isroot;
}

PROCESS(app_process, "Application Process");
AUTOSTART_PROCESSES(&app_process);

PROCESS_THREAD(app_process, ev, data)
{
    static struct etimer timer;
    static bool started = false;
    static uint32_t delay = MAX_LOOP_DELAY;
    int result;
    int count = 0;
    
    PROCESS_BEGIN();

    while (1)
    {
        if (ev == PROCESS_EVENT_INIT)
        {
            bool is_root = is_root_node();
#ifdef ROOT_NODE
            // allow compile time override
            is_root = true;
#endif
            app_event = process_alloc_event();
            printf("SubGHz CPU starting\n");
            result = NodeApplicationInit(is_root);
            require_noerr(result, exit);
            etimer_reset(&timer);
            etimer_set(&timer, delay);
            started = true;
        }
        else if (ev == PROCESS_EVENT_TIMER)
        {
            etimer_reset(&timer);
            etimer_set(&timer, delay);
        }
        else if (ev == tcpip_event)
        {
            ipc_msg_msg_t *msg;
            uip_ip6addr_t peerAddr;
            uip_ip6addr_t destAddr;
            int rssi = 0;
            int hops = 0;

            if(uip_newdata())
            {
                count++;
                memcpy(&peerAddr, &UIP_IP_BUF->srcipaddr, sizeof(peerAddr));
                memcpy(&destAddr, &UIP_IP_BUF->destipaddr, sizeof(destAddr));

                LOG_DBG("In: [0x%08lx], TTL %u, total %u\n",
                        (unsigned long)uip_ntohl((unsigned long) *((uint32_t *)(uip_appdata))),
                        UIP_IP_BUF->ttl, count);

                /*
                char ipstr[64];

                IPv6addrToString(&UIP_IP_BUF->srcipaddr, ipstr, sizeof(ipstr));
                printf("packet from: \n", ipstr);

                IPv6addrToString(&UIP_IP_BUF->destipaddr, ipstr, sizeof(ipstr));
                printf("packet to: %s\n", ipstr);
                */
                msg = (ipc_msg_msg_t*)uip_appdata;

                /* validate msg wont crash things */
                do // try
                {
                    if (msg->type > IPC_MESSAGE_STATUS)
                    {
                        LOG_WARN("Got msg with bad type %u, dropping\n", msg->type);
                        break;
                    }
                    if (msg->payloadLength > IPC_MAX_PAYLOAD)
                    {
                        LOG_WARN("Got msg with bad payload len %u, dropping\n", msg->payloadLength);
                        break;
                    }

                    NETSTACK_RADIO.get_value(RADIO_PARAM_LAST_RSSI, &rssi);
                    if (rssi < -127)
                    {
                        rssi = -127;
                    }

                    msg->rssitx = (int8_t)rssi;

                    NodeMsgReceiveMessage(
                                    IPC_TransportSubGHz,
                                    uip_appdata,
                                    msg->payloadLength + IPC_HEADER_SIZE,
                                    &peerAddr,
                                    ContikiAddrIsMulticastAddr(&destAddr),
                                    rssi,
                                    (int)(uint32_t)hops
                                  );
                }
                while (0); // catch
            }
        }
        else if (ev == app_event)
        {
            printf("app event\n");
        }

        result = NodeApplicationSlice(&delay);
        result = 0; // nothing to do if errors out so ignore
        if (delay < 1)
        {
            delay = 1;
        }

        watchdog_periodic();

        // this is "return"
        PROCESS_YIELD();
    }

    PROCESS_END();
exit:
    return result;
}

#include <stddef.h>
#include <stdint.h>

void dwt_dump(void)
{
    printf("DWT Dump:");

    printf("  0x%08x DWT_FUNC0: 0x%08x",
                0, DWT->FUNCTION1);
    printf("  0x%08x DWT_COMP0: 0x%08x",
                0, DWT->COMP1);
}

void dwt_reset(void) {
    DWT->FUNCTION1 = 0xD0000000;
}

void dwt_install_watchpoint(
        int comp_id,
        uint32_t func,
        uint32_t comp,
        uint32_t mask)
{
  DWT->COMP1 = comp;
  // set last since this will enable the comparator
  DWT->FUNCTION1 = 0xD0000815;
}

