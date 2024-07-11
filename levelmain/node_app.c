
#include "node_app.h"
#include "contiki.h"
#include <stdio.h>
#include "contiki-lib.h"
#include "contiki-net.h"
#include "net/ipv6/multicast/uip-mcast6.h"
#include "net/link-stats.h"
#include "shell.h"
#include "shell-commands.h"
#include "sl_sleeptimer.h"
#include "sys/log.h"
#define LOG_MODULE "node"
#define LOG_LEVEL LOG_LEVEL_INFO

#include "contikiaddr.h"
#include "ipcmsg_def.h"

#define DEFAULT_TX_POWER        (18)
#define DEFAULT_TX_PACKET_LEN_ROOT   (70)
#define DEFAULT_TX_PACKET_LEN_NODE   (60)
#define TX_INTERVAL_MS_ROOT     (600)       /* tx every 600ms */
#define TX_INTERVAL_MS_NODE     (2000)      /* tx about every 2s (random jitter added) */

static bool s_is_main_node;
static uint32_t s_txpower;
static bool s_txenable;
static int s_dump_proto;
static uint8_t s_tx_packet_len;
uip_ipaddr_t s_globalSubGHzIp6;
struct uip_udp_conn *s_udp_conn;

int64_t last_tx;
int64_t last_rx;
ipc_message_t s_txmsg;
ipc_message_t s_rxmsg;

int64_t uptime_get( void )
{
    int64_t elapsed = 0;
    uint64_t ticks = sl_sleeptimer_get_tick_count64();
    sl_sleeptimer_tick64_to_ms(ticks, &elapsed);
    return elapsed;
}

void assert_print(const char *name, const int line)
{
    LOG_ERR("%s: %d\n", name, line);
}

void LOG_HEXDUMP_INFO(const void *hex, int len, const char *msg)
{
    static char ns[256];

    LOG_INFO("%s", msg);

    for (int i = 0; i < len && i < 127; i++)
    {
        snprintf(ns + 2*i, sizeof(ns) - 2*i, "%02X", ((uint8_t*)hex)[i]);
    }
    LOG_INFO("%s\n", ns);
}

static int _SendUDP(
                const uip_ip6addr_t *inDestinationAddr,
                const uint16_t inDestinationPort,
                bool inIsMulticast,
                const void *inBytes,
                const size_t inCount
                )
{
    int result = -1;

    if (s_udp_conn)
    {
        uip_udp_packet_sendto(s_udp_conn, inBytes, inCount, inDestinationAddr, UIP_HTONS(inDestinationPort));
        result = 0;
    }

    return result;
}

void NodeMsgReceiveMessage(
                            const ipc_transport_t   inTransport,
                            uint8_t                 *inMessage,
                            size_t                  inMsgLen,
                            const uip_ip6addr_t     *inSrcAddr,
                            bool                    inIsMulticast,
                            int                     inRSSI,
                            int                     inHops
                        )
{
    ipc_message_t *ipcmsg;
    ipc_msg_msg_t *msg;
    bool drop = false;
    int result;

    msg = (ipc_msg_msg_t*)inMessage;
    require(msg, exit);
    require(inSrcAddr, exit);
    
    if (inIsMulticast)
    {
        // drop messages from ourselves.  we can get these from MPL (Thread mcast flooding)
        // forwarding messages we sent before, even if mcast loop is false
        
        drop = !memcmp(inSrcAddr, &s_globalSubGHzIp6, IPC_ADDR_SIZE);
        if (drop)
        {
            LOG_INFO("Dropping self-addressed\n");
            return;
        }
    }
    
    ipcmsg = &s_rxmsg;
    
    if (inMsgLen != (msg->payloadLength + IPC_HEADER_SIZE))
    {
        LOG_ERR("Msg of %u bytes has payload of %u, ignoring\n", inMsgLen, msg->payloadLength);
        goto exit;
    }
    
    if (msg->payloadLength < IPC_MAX_PAYLOAD)
    {
        // for easier printing and shell commands
        msg->payload[msg->payloadLength] = '\0';
    }
    
    // clamp rssi to what fits in a byte
    if (inRSSI < -127)
    {
        inRSSI = -127;
    }
    else if (inRSSI > 0)
    {
        inRSSI = 0;
    }
    
    // reconstruct the original ipc message as best we can from the inner message
    //
    memset(ipcmsg, 0, sizeof(ipc_message_t));
    ipcmsg->hdr.oper = IPC_LOCAL;
    ipcmsg->msg.type = msg->type;
    memcpy(ipcmsg->msg.payload, msg->payload, msg->payloadLength);
    ipcmsg->msg.payloadLength = msg->payloadLength;
    
    memcpy((uip_ipaddr_t*)ipcmsg->hdr.srcIp6, (uint8_t*)inSrcAddr, IPC_ADDR_SIZE);
    ipcmsg->msg.rssitx    = msg->rssitx;
    ipcmsg->msg.sequence  = msg->sequence;
    ipcmsg->hdr.transport = inTransport;
    ipcmsg->hdr.isMcast   = inIsMulticast;
    ipcmsg->hdr.rssirx    = inRSSI;
    
    if (s_dump_proto)
    {
        char xportname[128];
        char addrstr[64];
        
        IPv6addrToString(inSrcAddr, addrstr, sizeof(addrstr));
        
        snprintf(xportname, sizeof(xportname), "Rx Msg %u of %u seq=%u from %s\n",
               msg->type, msg->payloadLength, msg->sequence, addrstr);
            
       if (s_dump_proto > 1)
       {
           LOG_HEXDUMP_INFO(inMessage, inMsgLen, xportname);
       }
       else
       {
           LOG_INFO(xportname);
       }
    }  
exit:
   return;
}

int NodeMsgSendMessage(ipc_message_t *inMsg)
{
    int result = 1;
    int totlen;
    char addrstr[128];

    require(inMsg, exit);
    addrstr[0] = '\0';

    totlen = IPC_HEADER_SIZE + inMsg->msg.payloadLength;
    
    if (s_dump_proto > 0)
    {
        IPv6addrToString((uip_ip6addr_t *)inMsg->hdr.dstIp6, addrstr, sizeof(addrstr));
    }
    
    result = _SendUDP((uip_ip6addr_t *)inMsg->hdr.dstIp6, inMsg->hdr.dstPort, inMsg->hdr.isMcast, (uint8_t*)&inMsg->msg, totlen);
    require_noerr(result, exit);
    
    if (s_dump_proto > 0)
    {
        LOG_INFO("Tx msg %u of %u  seq:%u to %s:%u  ret=%d\n",
                inMsg->msg.type, totlen, inMsg->msg.sequence, addrstr, inMsg->hdr.dstPort, result);
            
        if (s_dump_proto > 1)
        {
            LOG_HEXDUMP_INFO(&inMsg->msg, totlen, "Bytes: ");
        }
    }
    
exit:
    return result;
}

int NodeMsgUpdate(uint32_t *outDelay)
{
    static uint16_t s_msg_sequence;
    
    int64_t now = uptime_get();
    int64_t tx_interval;
    
    *outDelay = 100;
    
    if (s_is_main_node)
    {
        tx_interval = TX_INTERVAL_MS_ROOT;
    }
    else
    {
        int32_t jitter;
        
        tx_interval = TX_INTERVAL_MS_NODE;
        jitter = (int32_t)(uint32_t)random_rand(); // 0-65535
        jitter /= 65536/64;  // 0-1024
        jitter -= 512;      // -512-+512
        tx_interval += (int64_t)jitter;
        
    }
    
    if (s_txenable && ((now - last_tx) >= tx_interval))
    {
        ipc_message_t *ipcmsg = &s_txmsg;
        
        // send a message
        last_tx = now;
        
        memset(ipcmsg, 0, sizeof(ipc_message_t));
        ipcmsg->hdr.oper = IPC_LOCAL;
        if (s_is_main_node)
        {           
            ipcmsg->msg.type = IPC_PERF;
        }
        else
        {
            ipcmsg->msg.type = IPC_FREP;
        }
        
        for (uint32_t v = s_msg_sequence; v < s_tx_packet_len; v++)
        {
            if (s_is_main_node)
            {
                ipcmsg->msg.payload[v] = v & 0xff;
            }
            else
            {
                ipcmsg->msg.payload[v] = 255 - (v & 0xFF);
            }
        }
        
        ipcmsg->msg.payloadLength = s_tx_packet_len;
        
        memcpy((uip_ipaddr_t*)&ipcmsg->hdr.srcIp6, (uint8_t*)&s_globalSubGHzIp6, IPC_ADDR_SIZE);
        ContikiAddrGetMulticastAddr((uip_ipaddr_t*)&ipcmsg->hdr.dstIp6);
        ipcmsg->hdr.dstPort = STARGATE_UDP_PORT;
        ipcmsg->msg.sequence  = s_msg_sequence++;
        ipcmsg->hdr.transport = IPC_SubGHz;
        ipcmsg->hdr.isMcast   = true;
        
        NodeMsgSendMessage(&s_txmsg);
    }
    
    return 0;
}

int NodeApplicationSlice(uint32_t *outDelay)
{
    static uip_ip6addr_t oldAddr;
    uip_ip6addr_t newAddr;
    uint32_t min_delay = 100;
    uint32_t delay;
    char addrstr[64];
    int result;
    
    // check if address changed
    //
    result = ContikiAddrGetAddr(&newAddr);
    if (!result)
    {
        if (memcmp(&newAddr, &oldAddr, sizeof(oldAddr)))
        {
            memcpy(&oldAddr, &newAddr, sizeof(oldAddr));
            
            IPv6addrToString(&newAddr, addrstr, sizeof(addrstr));
            LOG_INFO("IPv6 Address changed to %s\n", addrstr);
            
            if (ContikiAddrPrefixMatch(&newAddr))
            {
                uip_ip6addr_t mcastAddr;
                
                memcpy(&s_globalSubGHzIp6, &newAddr, IPC_ADDR_SIZE);
                
                // have an on-net address, so we're attached
                result = ContikiAddrGetMulticastAddr(&mcastAddr);
                require_noerr(result, exit);
                
                uip_ds6_maddr_t *rv = uip_ds6_maddr_add(&mcastAddr);
                if (rv)
                {
                    IPv6addrToString(&mcastAddr, addrstr, sizeof(addrstr));
                    LOG_INFO("Joined multicast group %s\n", addrstr);
                }
                
                s_udp_conn = udp_new(NULL, UIP_HTONS(0), NULL);
                require(s_udp_conn != NULL, exit);
                
                udp_bind(s_udp_conn, UIP_HTONS(STARGATE_UDP_PORT));
                
                NETSTACK_RADIO.set_value(RADIO_PARAM_TXPOWER, s_txpower);
                
                s_txenable = true;
            }
        }
    }
    
    result = NodeMsgUpdate(&delay);
    if (delay < min_delay)
    {
        min_delay = delay;
    }
    
    *outDelay = delay;
    exit:
    return result;
}

/*---------------------------------------------------------------------------*/
static
PT_THREAD(cmd_txpower(struct pt *pt, shell_output_func output, char *args))
{
    int txpower;

    PT_BEGIN(pt);
    if (args && *args)
    {
        txpower = strtol(args, NULL, 0);
        NETSTACK_RADIO.set_value(RADIO_PARAM_TXPOWER, txpower);
    }

    NETSTACK_RADIO.get_value(RADIO_PARAM_TXPOWER, &txpower);
    SHELL_OUTPUT(output, "r\nTx Power is %d dbm\n", txpower);
    PT_END(pt);
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(cmd_channel(struct pt *pt, shell_output_func output, char *args))
{
    int channel;

    PT_BEGIN(pt);
    if (args && *args)
    {
        channel = (int)strtoul(args, NULL, 0);
        NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, channel);
    }

    NETSTACK_RADIO.get_value(RADIO_PARAM_CHANNEL, &channel);
    SHELL_OUTPUT(output, "\r\nChannel is %d\n", channel);
    PT_END(pt);
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(cmd_panid(struct pt *pt, shell_output_func output, char *args))
{
    int panid;

    PT_BEGIN(pt);
    if (args && *args)
    {
        panid = (int)strtoul(args, NULL, 0);
        NETSTACK_RADIO.set_value(RADIO_PARAM_PAN_ID, panid);
    }

    NETSTACK_RADIO.get_value(RADIO_PARAM_PAN_ID, &panid);
    SHELL_OUTPUT(output, "\r\npanid is 0x%04X\n", panid);
    PT_END(pt);
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(cmd_start(struct pt *pt, shell_output_func output, char *args))
{
    PT_BEGIN(pt);
    s_txenable = true;
    SHELL_OUTPUT(output, "\r\nStarted\n");
    PT_END(pt);
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(cmd_stop(struct pt *pt, shell_output_func output, char *args))
{
    PT_BEGIN(pt);
    s_txenable = false;
    SHELL_OUTPUT(output, "\r\nStopped\n");
    PT_END(pt);
}
/*---------------------------------------------------------------------------*/
const struct shell_command_t level_shell_commands[] = {
    { "txpower",              cmd_txpower,              "'> txpower': Set/Get Tx Power in dbm" },
    { "channel",              cmd_channel,              "'> channel': Set/Get channel" },
    { "panid",                cmd_panid,                "'> panid': Set/Get panid" },
    { "start",                cmd_start,                "'> start': Start transmitting" },
    { "stop",                 cmd_stop,                 "'> stop': Stop transmitting" },
    { NULL, NULL, NULL },
};

static struct shell_command_set_t level_shell_command_set = {
    .next = NULL,
    .commands = level_shell_commands,
};

int NodeApplicationInit(const bool inIsRoot)
{
    int result = -1;
    
    s_is_main_node = inIsRoot;
    s_txpower = DEFAULT_TX_POWER;
    
    shell_command_set_register(&level_shell_command_set);
    
    result = ContikiAddrInit(NULL);
    require_noerr(result, exit);
    
    if (inIsRoot)
    {
        // Initialize DAG root
        NETSTACK_ROUTING.root_start();
    }
    
    LOG_INFO("Initialized node %s\n", s_is_main_node ? "Star-root" : "Node");
    
    s_dump_proto = 1;   
    s_txenable = false;
    if (inIsRoot)
    {
        s_tx_packet_len = DEFAULT_TX_PACKET_LEN_ROOT;
    }
    else
    {
        s_tx_packet_len = DEFAULT_TX_PACKET_LEN_NODE;
    }
    result = 0;
exit:
    return result;
}


