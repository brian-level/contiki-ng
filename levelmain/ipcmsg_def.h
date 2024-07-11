#pragma once

#include <stdint.h>

#define UUID_LEN_IN_BYTES       (16)

/// MAGIC also serves as message structure version
///
#define IPC_MAGIC               (0xCAFE)

/// For byte stream transports (UART) the ipc mechanism uses a
/// sync byte system to frame messages
///
#define IPC_SYNC_BYTE           (0x3C)
#define IPC_PROLOG_BYTE         (0x66)

/// Size of an IPv6 address in bytes
///
#define IPC_ADDR_SIZE           (16)

/// Size of a device node Id in bytes (LevelHome serial number format)
///
#define IPC_ID_SIZE             (10)

/// Size of a Thread network key in bytes
///
#define IPC_NETWORK_KEY_SIZE    (16)

/// Size of a Thread extended PAN ID in bytes
///
#define IPC_EXT_PAN_ID_SIZE     (8)

/// IPC message operation, i.e. what the recipient is supposed to do with it
///
/// IPC can be used to send messages to/from "proxy" radios and this operation
/// determins if the message is meant to be proxied (TX/RX) or is meant for the
/// proxy itself. for management of the proxy radio.
///
typedef enum 
{
    IPC_LOCAL,              ///< the message is meant for the proxy, not to be interpreted, use type for type
    IPC_UNICAST_TX,         ///< recipient should send this message directly to the final recipient
    IPC_MULTICAST_TX,       ///< recipient should multicast this message
    IPC_UNICAST_RX,         ///< this is message sent directly to the recipient passing this along
    IPC_MULTICAST_RX        ///< this is a multicast message receved
}
ipc_oper_type_t;

/// IPC message types
///
typedef enum
{
    IPC_NOOP,                   ///< 00  do nothing
    IPC_GETV,                   ///< 01  get the firmware version(s)
    IPC_VERS,                   ///< 02  report of firmware version(s)
    IPC_PING,                   ///< 03  ping the recipient
    IPC_PONG,                   ///< 04  pong
    IPC_PERF,                   ///< 05  send a performance/metric probe message
    IPC_FREP,                   ///< 06  performance/metric probe reply
    IPC_SHELL,                  ///< 07  send this shell command string to recipient
    IPC_SHELL_RESP,             ///< 08  the response of the shell command back
    IPC_DISC,                   ///< 09  discovery message from server->gw means return device list, from gw->sg means ping/getv
    IPC_OTA,                    ///< 10  firmware upgrade packet
    IPC_SRVR_REQUEST,           ///< 11  request from a server
    IPC_SRVR_RESPONSE,          ///< 12  response to request back to server
    IPC_SRVR_NOTIFICATION,      ///< 13  notfication to server (async/oob)
    IPC_ZB_REQUEST,             ///< 14  perform a set/req on Zigbee end device
    IPC_ZB_RESPONSE,            ///< 15  reponse from Zigbee end device
    IPC_ZB_NOTIFICATION,        ///< 16  notification from Zigbee end device (async/oob)
    IPC_RX_RSSI,                ///< 17  indication of RSSI as seen by sender when it go its last Rx message
    IPC_TX_POWER,               ///< 18  recipient should set their tx power to the uint32 in the payload
    IPC_ADDRESS_QUERY,          ///< 19  recipient should respond with their network address
    IPC_ADDRESS_CHANGE,         ///< 20  inform a radio's new network address
    IPC_THREAD_PARAMS,          ///< 21  inform a Thread radio what its (new) parameters are
    IPC_THREAD_ROOM,            ///< 22  percent room left for sending messages
    IPC_REBOOT,                 ///< 23  restart
    IPC_DIAG_REQUEST,           ///< 24  request diagnostic info
    IPC_DIAG_RESPONSE,          ///< 25  response with diagnostic info
    IPC_MCAST_DIRECT,           ///< 26  a message addressed to a specific node but using multicast
    IPC_CHIPID_REQUEST,         ///< 27  request for chipID
    IPC_CHIPID_RESPONSE,        ///< 28  response with chipID
    IPC_FACT_RST_REQUEST,       ///< 29  request a factory reset
    IPC_FACT_RST_RESPONSE,      ///< 30  response to a factory reset request
    IPC_ZB_ACK,                 ///< 31  response from Zigbee CPU that we recieved an OK ZED command
    IPC_KEY_EXCHANGE,           ///< 32  key exchange message for secure sessions
    IPC_ZB_REQUEST_SECURE,      ///< 33  encrypted, perform a set/req on Zigbee end device
    IPC_ZB_ACK_SECURE,          ///< 34  encrypted, ack from Zigbee CPU we recieved an OK ZED command
    IPC_ZB_NOTIFICATION_SECURE, ///< 35  encrypted, notification from Zigbee end device (async/oob)
    IPC_UUID_REQUEST,           ///< 36  request provisioned UUID
    IPC_UUID,                   ///< 37  report provisioned UUID
    IPC_THREAD_PARAMS_SECURE,   ///< 38  inform a Thread radio what its (new) parameters are (encrypted)
    IPC_THREAD_PARAMS_UPDATED,  ///< 39  notify that thread parameters have been changed
    IPC_GWCONTROL,              ///< 40  atlantis gateway control messages
    IPC_MESSAGE_STATUS,         ///< 41  meta data about a message, i.e. failure or delivery status
    IPC_TELEMETRY_REQUEST,      ///< 42  request telemetry from target systems
    IPC_TELEMETRY_RESPONSE,     ///< 43  response with telemetry info
}
ipc_message_type_t;

/// Transport Types
///
typedef enum
{
    IPC_None   = 0,  ///< No transport
    IPC_Thread = 1,  ///< Thread network transport
    IPC_LoRa   = 2,  ///< LoRa network transport
    IPC_SubGHz = 3,  ///< Sub-GHz network (thread or other non-LoRa on 900MHz)
    IPC_WiFi   = 4,  ///< Wireless network transort
    IPC_Enet   = 5,  ///< Wired network transport
    IPC_IPC    = 6,  ///< Inter-CPU comms
    IPC_BLE    = 7,  ///< Bluetooth
    IPC_Other  = 31  ///< Something else
}
ipc_transport_t;

/// Transport Strategy
///
typedef enum
{
    IPC_TransportThread,             ///< use Thread for messaging
    IPC_TransportLoRa,               ///< use LoRa for messaging
    IPC_TransportThreadAndLoRa,      ///< use Thread, and LoRa for backup (Thread times-out)
    IPC_TransportSubGHz,             ///< use SubGHz radio for messaging
    IPC_TransportThreadAndSubGHz,    ///< use Thread, and SubGHz for backup (Thread times-out)
    IPC_TransportSubGHzAndThread     ///< use SubGHz radio for messaging and Thread for backup
}
ipc_transport_strategy_t;

/// An IPC message. Note that this is layed out so that it is the same size
/// packed or unpacked and still aligned properly, so can be interpreted directly
/// on the wire, and also padded so the large arrays are 8 byte aligned to make
/// memcpy easier, etc. Avoid the urge to make sense of the ordering please and
/// keep this as small as possible
///
/// Functions for sending and receiving IPC messages do so in whole messages
/// only. On the wire messages are framed as needed to ensure reliable
/// message-gram delivery
///
/// Note that the message structure has metadata, addressing data, and payload data
/// To reduce the on-wire size, only a minimal amount of information is sent on
/// the wire
///

/// This is an address used in a message. Either an IPv6 address or device IDs.
/// The transport and message type (unicast/multicast) determine the interpretation
/// For Thread messages, addresses are always IPv6 and for LoRa (for example) the
/// deviceId of src and dst is always used
///
typedef struct
{    /// LoRa (or proprietary) addressing
    uint8_t  srcId[IPC_ID_SIZE];            ///< 0  Source Device Id
    uint8_t  dstId[IPC_ID_SIZE];            ///< 10 Destination Device Id
    uint16_t dstPort;                       ///< 20 Destination port
}
ipc_msg_addr_t;

#define IPC_MSG_ADDR_SIZE (sizeof(uint16_t) + (2 * IPC_ID_SIZE))
_Static_assert(sizeof(ipc_msg_addr_t) == IPC_MSG_ADDR_SIZE, "IPC_MSG_ADDR_SIZE defined wrong");

/// Message Header.  Meta-data about the message NOT sent on the radio (but might be
/// sent over the wire, i.e. a tether or TCP/IP socket).
/// The header is used to manage and route messages internally
///
/// Note: this is part of a larger composition and must be packed and 64-bit aligned
///       don't change this struct without thinkin
///
typedef struct
{
    uint16_t    magic;                      ///< 0  Magic number that confirms this is an IPC msg (and serves as version)
    uint16_t    dstPort;                    ///< 2  destination port on recipient of message
    uint8_t     transport;                  ///< 4  Transport (radio type) of originator of message (ipc_transport_t)
    uint8_t     oper;                       ///< 5  what to do with msg (operaton) (ipc_message_oper_t)
    uint8_t     isMcast;                    ///< 6  non-0 is message is a multicast
    int8_t      rssirx;                     ///< 7  rssi measured when message received (local)
    uint8_t     srcId[IPC_ID_SIZE];         ///< 8  source device Id
    uint8_t     srcIp6[IPC_ADDR_SIZE];      ///< 18 src Ip6 address
    uint8_t     dstId[IPC_ID_SIZE];         ///< 34 destinaton device Id
    uint8_t     dstIp6[IPC_ADDR_SIZE];      ///< 44 destination Ip6 address
    uint16_t    sequence;                   ///< 60 message sequence number for correlation
    uint16_t    crc16;                      ///< 62 crc16 of message header (with .crc16 set 0), the attached msg.msg, and payload bytes
                                            ///< 64 is end alignment, keep it 8 byte aligned
}
ipc_msg_hdr_t;

#define IPC_MSG_HDR_SIZE (4 * sizeof(uint16_t) + (4 * sizeof(uint8_t)) + (2 * IPC_ID_SIZE) + (2 * IPC_ADDR_SIZE))
_Static_assert(sizeof(ipc_msg_hdr_t) == IPC_MSG_HDR_SIZE, "IPC_MSG_HDR_SIZE defined wrong");

/// Set the upper bound for a total message size.  This is the "key" define and used everywhere
/// for allocating for messages, and other defines are derived from this, so this should be the
/// only thing you need to change to change message sizes
///
#define IPC_MAX_MESSAGE         (256)

/// Use this bit in sequence to specify it was not from a server/external generation
#define IPC_INTERNAL_SEQ_BIT   ((uint16_t)1 << 15)

/// This is the size of the header portion of the inner message
#ifdef CONFIG_LORA
    #define IPC_HEADER_SIZE     (sizeof(ipc_msg_addr_t) + 2 * sizeof(uint8_t) + sizeof(uint16_t))
#else
    #define IPC_HEADER_SIZE     (2 * sizeof(uint8_t) + sizeof(uint16_t))
#endif

/// Derive the max payload as what can fit in the payload portion of the message
#define IPC_MAX_PAYLOAD     (IPC_MAX_MESSAGE - IPC_MSG_HDR_SIZE - IPC_HEADER_SIZE)

/// The meat of the message.  This is the only part that needs to be sent on radio/wire
/// and has only the information needed to process and reply to a message
///
/// For use on transports that dont have addressing in the protocol natively
/// (like LoRa, Proprietary/Multicast, etc.) the src/dst node Id needs to be
/// included which adds 20 bytes
///
/// (note payloadLength is in the position it is in so remains compatible with
/// code that uses 16 bit little-ending length in msg content)
///
typedef struct
{
    #ifdef CONFIG_LORA
    ipc_msg_addr_t  addr;                   ///< 0  address of sender (for replies)
    #endif
    uint8_t     type;                       ///< 0 (22) type of msg (ipc_message_type_t)
    int8_t      rssitx;                     ///< 1 (23) rssi measured when message received (remote)
    uint8_t     payloadLength;              ///< 2 (25) payload byte count
    uint8_t     sequence;                   ///< 3 (24) 8 bits LSB of sequence
    uint8_t     payload[IPC_MAX_PAYLOAD];   ///< 4 or 28 payload bytes
}
ipc_msg_msg_t;

_Static_assert(IPC_MAX_PAYLOAD < 256, "IPC_MAX_PAYLOAD cant fit in 1 byte");

#ifdef CONFIG_LORA
    #define IPC_MSG_MSG_SIZE (sizeof(ipc_msg_addr_t) + 2 * sizeof(uint8_t) + sizeof(uint16_t) + IPC_MAX_PAYLOAD)
#else
    #define IPC_MSG_MSG_SIZE (2 * sizeof(uint8_t) + sizeof(uint16_t) + IPC_MAX_PAYLOAD)
#endif

_Static_assert(sizeof(ipc_msg_msg_t) == IPC_MSG_MSG_SIZE, "IPC_MSG_MSG_SIZE defined wrong");

/// This is the all the information about a message bundled together.
/// Messages are in thie format everywhere except on radios. When transmitted
/// or received, only the msg part is put on the radio to reduce packet size
/// in order to try and fit in one thread packet (65-70 bytes) for example
///
typedef struct
{
    ipc_msg_hdr_t   hdr;
    ipc_msg_msg_t   msg;
}
ipc_message_t;

_Static_assert(sizeof(ipc_message_t) == IPC_MAX_MESSAGE, "One of IPC_MAX_MESSAGE/HEADER/PAYLOAD defined wrong");
_Static_assert(sizeof(ipc_message_t) == (sizeof(ipc_msg_hdr_t) + sizeof(ipc_msg_msg_t)), "unpadded/unaligned structure");
_Static_assert((sizeof(ipc_message_t) - IPC_MAX_PAYLOAD - IPC_MSG_HDR_SIZE) == IPC_HEADER_SIZE, "IPC header not naturally packed");

/// Derive a preferred max payload to fit easily in one Thread packet
///
/// Thread max payload (this can vary a lot depending on header compression
/// and can be up to 80 bytes, but lets be conservative and use 68
/// to get a useful payload of 48 (20 bytes of header in message)
///
#define IPC_MAX_THREAD_PACKET   (68)

/// Then the preferred max payload is what can fit on the radio after the header (64 bytes)
///
#define IPC_PREFERRED_MAX_PAYLOAD   (IPC_MAX_THREAD_PACKET - IPC_HEADER_SIZE)

/// There are just a few messages with payloads, and just a few fixed payload
/// formats.
///
/// Zigbee/end-device message payload is described in zigbee_ipc.h
///
/// VERS messages are used to respond to GETV(ersion) and DISC(overy)
/// messages and contain a fixed payload
///
/// THREAD_PARAM messages
///
/// Directed multicast messages have a second header in the payload which contains
/// a destination id and actual message header
///
/// OTA messages are used to transfer f/w to devices

/// This is a single version
///
typedef struct
{
    uint8_t vMaj;
    uint8_t vMin;
    uint8_t vPat;
    uint8_t vBld;
}
ipc_version_t;

typedef struct
{
    uint16_t        magic;                  ///< 0  magic number
    uint16_t        subPayloadLength;       ///< 2  extra bytes following struct that are valid
    uint32_t        epoch;                  ///< 4  epoch time
    ipc_version_t   versX[3];               ///< 8  (up to) 3 versions of running code, 1 for each CPU
    ipc_version_t   versF[3];               ///< 20 (up to) 3 versions of ota slot code
    uint8_t         devId[IPC_ID_SIZE];     ///< 32 device Id
    uint8_t         endDeviceCount;         ///< 42 number of end devices managed by reporting device
    uint8_t         padding;                ///< 43 pad to 32 bit boundary
                                            ///< 44 extra payload goes here if subPayloadLength > 0
    // bytes after this field are generic payload/info
}
ipc_version_msg_t;

#define IPC_VERS_MSG_SIZE   ((2 * sizeof(uint16_t)) + sizeof(uint32_t) + (6 * sizeof(ipc_version_t)) + IPC_ID_SIZE + 2)
_Static_assert(sizeof(ipc_version_msg_t) == IPC_VERS_MSG_SIZE, "IPC_VERS_MSG_SIZE defined wrong");

#define IPC_VERS_MAGIC  (0x1234)

typedef struct
{
    uint16_t        panId;                              ///< 0  pan Id
    uint8_t         chan;                               ///< 2  channel
    int8_t          txPower;                            ///< 3  tx Power dbm
    uint8_t         devId[IPC_ID_SIZE];                 ///< 4  device Id
    uint8_t         networkKey[IPC_NETWORK_KEY_SIZE];   ///< 14 network key
    uint8_t         extPanId[IPC_EXT_PAN_ID_SIZE];      ///< 30 extended pan Id
    uint8_t         pad[2];                             ///< 38 padding to get to 32 bit alignment
}
ipc_thread_params_t;

#define IPC_PARAMS_MSG_SIZE   ((1 * sizeof(uint16_t)) + (4 * sizeof(uint8_t)) + IPC_ID_SIZE + IPC_NETWORK_KEY_SIZE + IPC_EXT_PAN_ID_SIZE)
_Static_assert(sizeof(ipc_thread_params_t) == IPC_PARAMS_MSG_SIZE, "IPC_PARAMS_MSG_SIZE defined wrong");

typedef struct
{
    uint32_t    partitionId;                ///< 0 partition ID
    uint32_t    resetReason;                ///< 4 reset reason
    uint32_t    uptimeSeconds;              ///< 8 uptime in seconds
    uint16_t    numPartitionEvents;         ///< 12 number of Thread partition events
    uint16_t    numRoleChanges;             ///< 14 number of Thread role changes
    uint8_t     numNeighbors;               ///< 16 number of Thread neighbors
    uint8_t     role;                       ///< 17 current Thread role
    uint8_t     linkQualityCountList[4];    ///< 18 Number of neighbors with LQ 0, 1, 2, 3
    uint8_t     pad[2];                     ///< 22 padding to get to 32 bit alignment
}
ipc_diag_rsp_t;

#define IPC_DIAG_RESPONSE_MSG_SIZE   ((3 * sizeof(uint32_t)) + (2 * sizeof(uint16_t)) + (8 * sizeof(uint8_t)))
_Static_assert(sizeof(ipc_diag_rsp_t) == IPC_DIAG_RESPONSE_MSG_SIZE, "IPC_DIAG_RESPONSE_MSG_SIZE defined wrong");

typedef struct
{
    uint8_t     dstId[IPC_ID_SIZE];         ///< 0 intended recipient of message
    uint8_t     subtype;                    ///< 10 actual message type (not IPC_MCAST_DIRECT)
    uint8_t     payloadLength;              ///< 11 remaining length of payload bytes
}
ipc_mcast_direct_t;

#define IPC_MCAST_DIRECT_MSG_SIZE   ((12 * sizeof(uint8_t)))
_Static_assert(sizeof(ipc_mcast_direct_t) == IPC_MCAST_DIRECT_MSG_SIZE, "IPC_MCAST_DIRECT_MSG_SIZE defined wrong");

typedef struct
{
    uint8_t     uuid[UUID_LEN_IN_BYTES];    ///< 0 16-byte unique device identifier, see UUID_t
}
ipc_uuid_rsp_t;

#define IPC_UUID_MSG_SIZE   ((16 * sizeof(uint8_t)))
_Static_assert(sizeof(ipc_mcast_direct_t) == IPC_MCAST_DIRECT_MSG_SIZE, "IPC_MCAST_DIRECT_MSG_SIZE defined wrong");

// BLE Service Data
// Stargate advertises this data under the 16-bit service data flag (0x16)

// BLE service flags bits
#define SVC_FLAGS_OWNING_BITMASK    0x01

// Since we're at the 31-byte maximum for a BLE advertising packet we don't advertise the
// fw version build number
typedef struct
{
    uint8_t vMaj;       ///< 0 fw version major
    uint8_t vMin;       ///< 1 fw version minor
    uint8_t vPat;       ///< 2 fw version patch
    uint8_t svc_flags;  ///< 3 1-byte flags see BLE service flags bits
}
ipc_service_info_t;

#define IPC_SERVICE_INFO_SIZE   ((4 * sizeof(uint8_t)))
_Static_assert(sizeof(ipc_service_info_t) == IPC_SERVICE_INFO_SIZE, "IPC_SERVICE_INFO_SIZE defined wrong");


// OTA is done by programing sectors
//
// A sector is one flash sector,  usually 4k bytes
// The sector data is sent in blocks of a smaller
// size that can be transferred in shorter transmissions
// to increase likelyness of success
//
// addressing starts at 0x0000_0000.  The actual
// address in flash is provided in messaging and is used
// for programming but not for transfer.
//
// A flash_address is an absolute address in flash memory
// A sector_address is the byte offset of a sector from 0
// A block_address is a sector_addres + byte offset of a block
//
// Example, for 4k sectors and blocks of 128, and a
// programming address at 1mb, the 3rd block in the second
// sector would have a
//
// flash_address  == 0x1000_1100
// sector_address == 4096
// block_address  == 4352
//

#define OTA_SECTOR_SIZE (4096)
#define OTA_BLOCK_SIZE  (128)
#define OTA_BLOCKS_PER_SECTOR   (OTA_SECTOR_SIZE / OTA_BLOCK_SIZE)

_Static_assert ( (OTA_BLOCKS_PER_SECTOR * OTA_BLOCK_SIZE) == OTA_SECTOR_SIZE, "Block size must be integer divisor of sector size" );

// OTA packet types
//
#define OTA_PKT_NOOP        (0) ///< No meaning
#define OTA_PKT_SOP         (1) ///< Start of Program
#define OTA_PKT_DATA        (2) ///< Data
#define OTA_PKT_EOS         (3) ///< End of Sector
#define OTA_PKT_EOP         (4) ///< End of Program
#define OTA_PKT_NEEDB       (5) ///< Need Block (deprecated, dont use)
#define OTA_PKT_NEEDMANY    (6) ///< Need a bunch of Blocks
#define OTA_PKT_INSTALL     (7) ///< Install f/w
#define OTA_PKT_STATUS      (8) ///< Status of OTA

#pragma pack(push, 1)

/// target cpu types
///
typedef enum 
{
    OTA_TARGET_CPU_NONE,    // none/error
    OTA_TARGET_CPU_52840,
    OTA_TARGET_CPU_5340,
    OTA_TARGET_CPU_FG23,
    OTA_TARGET_CPU_SIM,
    OTA_TARGET_CPU_ZED
}
ota_target_cpu_t;

/// target board types
/// NOTE_: *never* insert a new one, always append, since devices in the field
/// check their board type against ota board type and will reject mismatches
///
typedef enum
{
    OTA_TARGET_BRD_NONE,    ///< none/error
    OTA_TARGET_BRD_52840DK, ///< off the shelf 52840 dk
    OTA_TARGET_BRD_5340DK,  ///< off the shelf 5340 dk
    OTA_TARGET_BRD_21540DK, ///< off the shelf 21540 dk with 52840
    OTA_TARGET_BRD_4210A,   ///< off the shelf SL dk for ZG23
    OTA_TARGET_BRD_SGEVK,   ///< SG EVT first version
    OTA_TARGET_BRD_SGEVT,   ///< SG EVT second version
    OTA_TARGET_BRD_ATEVK,   ///< SG EVT in Atlantis h/w first version
    OTA_TARGET_BRD_ATEVT,   ///< SG EVT in Atlantis second version
    OTA_TARGET_BRD_SGDVT,   ///< SG DVT
    OTA_TARGET_BRD_ATDVT,   ///< SG DVT in Atlantis
    OTA_TARGET_BRD_SG1,     ///< SG version 1
    OTA_TARGET_BRD_AT1,     ///< SG in Atlantis version 1
    OTA_TARGET_BRD_SIM,     ///< simulation
    OTA_TARGET_BRD_ZED      ///< Zigbee End-Device (generic)
}
ota_target_brd_t;

/// define a slot
///
typedef enum
{
    // internal flash
    FLASH_SLOT_PRIMARY      = 0,
    FLASH_SLOT_NET          = 2,
    FLASH_SLOT_NVM          = 3,
    FLASH_SLOT_ZBL          = 4,
    // external flash
    FLASH_SLOT_APP_UPDATE   = 16,
    FLASH_SLOT_COPROC1      = 17,
    FLASH_SLOT_COPROC2      = 18,
    FLASH_SLOT_BLOBULAR     = 19,
    FLASH_SLOT_OTA_LOG      = 32,
    FLASH_SLOT_TELEMETRY    = 33,

    // cpu2 external flash
    FLASH_SLOT_ZED          = 128
}
flash_slot_t;

typedef enum
{
    OTA_STATUS_NOT_RUNNING,
    OTA_STATUS_RUNNING,
    OTA_STATUS_DOWNLOAD_DONE,
    OTA_STATUS_INSTALL_ALLOW,
    OTA_STATUS_INSTALL_BEGIN,
    OTA_STATUS_INSTALL_DONE,
}
ota_status_type_t;

/// Start-of-Program message
///
typedef struct {
    uint8_t  type;              ///< set to OTA_PKT_SOP
    uint8_t  flags;
    uint8_t  target_slot;
    uint8_t  target_cpu;
    uint8_t  target_brd;
    uint8_t  version_maj;
    uint8_t  version_min;
    uint8_t  version_pat;
    uint8_t  version_bld;

    /// TODO - security negotiation here
    /// can't just let anyone write flash
}
ota_sop_t;

/// flags for SOP
///
#define OTA_SOP_FLAG_NONE           (0x00)
#define OTA_SOP_FLAG_NOERASE        (0x01)  ///< don't erase flash while writing

/// Send a data block
///
typedef struct {
    uint8_t  type;              ///< set to OTA_PKT_DATA
    uint8_t  flags;
    uint16_t crc16;
    uint32_t block_address;
    uint8_t  data[OTA_BLOCK_SIZE];
}
ota_data_t;

#define OTA_DATA_FLAG_NONE          (0x00)

_Static_assert ( sizeof(ota_data_t) < IPC_MAX_PAYLOAD, "Radio packet overflow" );

/// End-of-sector message
///
typedef struct {
    uint8_t  type;              ///< set to OTA_PKT_EOS
    uint8_t  flags;
    uint16_t sector_crc16;
    uint32_t sector_address;
    uint32_t flash_address;
}
ota_eos_t;

/// End-of-Program message
///
typedef struct {
    uint8_t  type;              ///< set to OTA_PKT_EOP
    uint8_t  flags;
    uint32_t flash_address;
    uint32_t program_size;
}
ota_eop_t;

#define OTA_EOP_FLAG_NONE           (0x00)
#define OTA_EOP_FLAG_INSTALL        (0x01)  ///< install this program now
#define OTA_EOP_FLAG_VERIFY_SKIP    (0x02)  ///< skip verify before install

/// Install message
///
/// you can separate download and install by NOT setting the install
/// flag in the EOP message and later sending an install message.
///
/// The group is a byte that influences how many recipients (more or less)
/// should actually do the install.  Each recipient will generate a random
/// 8 bit number and compare it to group. If it is <=, it will actually do
/// the install. If >, it will skip.
///
/// For example, if you want 50% of the nodes (more or less) to install
/// you would multicast an install packet with group set to 128, and
/// and if you want 25% you would use 64, etc.
///
/// The group is only applied if the OTA_INSTALL_FLAG_GROUP bit is set
/// in the flags. If not, the install includes all recipients but you
/// should set group to 255 in the message in that case for clarity
///
/// For some targets (cpu1) an "install" means swap contents of ext-flash
/// and internal-flash. That could be a downgrade in some cases.  Normally
/// that's not allowed and the install ignored, but you can set the
/// flag bit OTA_INSTALL_FLAG_FORCE to bypass version checking
///
typedef struct {
    uint8_t  type;              ///< set to OTA_PKT_INSTALL
    uint8_t  flags;             ///< see defines below
    uint8_t  target_slot;       ///< source slot for install
    uint8_t  group;             ///< threshold for selective install if multicast
    uint32_t attempts;          ///< how many times to attempt to dfu (for cpu2/3, not cpu1)
}
ota_install_t;

#define OTA_INSTALL_FLAG_NONE       (0x00)
#define OTA_INSTALL_FLAG_GROUP      (0x01)  ///< use the group field to pick sub-group to install
#define OTA_INSTALL_FLAG_FORCE      (0x02)  ///< force swap even if ext flash lower version (cpu1)

// Note: the needmany can handle needing 1 block just
// as well as the needb, so needb might be deprecated
// soon.  Use needmany for all cases in new code

typedef struct {
    uint8_t  type;
    uint8_t  flags;
    uint16_t reserved;
    uint32_t block_address;
}
ota_needb_t;

typedef struct {
    uint8_t  type;
    uint8_t  flags;
    uint16_t reserved;
    uint32_t sector_address;
    uint32_t block_mask;
}
ota_needmany_t;

typedef struct {
    uint8_t  type;
    uint8_t  status_type;
    uint16_t status_value;
}
ota_status_t;

#pragma pack(pop)

