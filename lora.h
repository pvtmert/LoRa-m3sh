
#ifndef _LORA_H_
#define _LORA_H_

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <time.h>

#include "list.h"
#include "device.h"

#define _LORA_PKG_SIZE_ 230
#define _LORA_MAX_CONN_ 128
#define _LORA_MAGIC_ ((0x4d << 0) | (0x33 << 8) | (0x53 << 16) | (0x48 << 24))

// lora brcast addrs: 0, 1
// https://www.libelium.com/forum/viewtopic.php?p=51639&sid=ac0f0fdc3d14536e8031191e6f277990#p51667
// https://www.thethingsnetwork.org/wiki/LoRaWAN/Address-Space

///
/// packet type enum
///
typedef enum PacketType {
	PKG_TYPE_NONE = 0,
	PKG_TYPE_OK   = 1 << 0,
	PKG_TYPE_ACK  = 1 << 1,
	PKG_TYPE_RLY  = 1 << 2,
	PKG_TYPE_404  = 1 << 3,
	PKG_TYPE_FIND = 1 << 4,
	PKG_TYPE_CONT = 1 << 5,
	PKG_TYPE_RES3 = 1 << 6,
	PKG_TYPE_RES4 = 1 << 7,
	PKG_TYPE_BITS = 8,
} packet_type_t;


///
/// node flag enum
///
typedef enum NodeFlag {
	NODE_FLAG_NONE = 0,
	NODE_FLAG_FREE = 1 << 0,
	NODE_FLAG_LOCK = 1 << 1,
	NODE_FLAG_A    = 1 << 2,
	NODE_FLAG_B    = 1 << 3,
	NODE_FLAG_C    = 1 << 4,
	NODE_FLAG_D    = 1 << 5,
	NODE_FLAG_E    = 1 << 6,
	NODE_FLAG_F    = 1 << 7,
	NODE_FLAG_BITS = 8,
} node_flag_t;

///
/// default addressing type
///
typedef uint32_t addr_t;

///
/// \struct Packet
/// \brief packet type definition.
/// defines type 'packet'
/// uses 'packed' attribute to send minimum
/// number of bytes over network to maintain
/// high throughput and low redundancy
///
/// \var checksum 64-bit checksum of packet, divideed into 2 parts,
///      1st element of the array is the packet header checksum
///      2nd element of the array is the packet data checksum
/// \var magic magic 'number' actually 4-chars to identify packet
/// \var src source address where packet is originated from
/// \var dst destination address where packet should be delivered to
/// \var ttl time to live value of the packet
/// \var length length of the data of the packet
/// \var data fixed size byte array of data
/// \var type type and flags of packet
///
typedef struct Packet {
	uint32_t magic;
	uint32_t checksum;
	addr_t src;
	addr_t dst;
	uint16_t ttl;
	uint8_t length;
	packet_type_t type: PKG_TYPE_BITS;
	uint32_t data_crc;
	uint8_t data[_LORA_PKG_SIZE_];
} __attribute__((packed)) packet_t;

///
/// node type definition
///
/// \var id 64-bit node identifier
/// \var internet does it have internet connection (eg gateway)
/// \var count number of links that are connected
/// \var links pointer to link-ptr (array of link-pointers)
/// \var device pointer to device (thread) which is virtual processor
/// \var flag indicates status flags of this node
///
typedef struct Node {
	uint64_t id;
	bool internet;
	unsigned count;
	struct Link **links;
	struct Device *device;
//	list_t *last_pkgs;
//	const unsigned short mem_size;
	node_flag_t flag: NODE_FLAG_BITS;
} node_t;

///
/// link type definition
///
/// \var latency base latency of this node
/// \var deviation maximum deviation level for latency
/// \var nodes pointer to node-ptr (array of node-pointers)
/// \var count number of nodes that are connected
///
typedef struct Link {
	unsigned long latency;
	unsigned deviation;
	struct Node **nodes;
	unsigned count;
} link_t;

////////////////////////////////////////

void print(void *, size_t);

////////////////////////////////////////

packet_t* packet_make(
	const addr_t,
	const addr_t,
	const packet_type_t,
	const uint16_t
);
uint32_t packet_prepare(packet_t*, const uint8_t, const void*);
uint32_t packet_getsum(packet_t*);
bool packet_verify(packet_t*);

////////////////////////////////////////

node_t* node_make(
	const uint64_t,
	const bool
);

void node_deliver(node_t*, packet_t*);
void node_connect_ptr(node_t*, unsigned, link_t**);
void node_connect_va(node_t*, unsigned, va_list);
void node_connect(node_t*, unsigned, ...);

////////////////////////////////////////

link_t* link_make(
	unsigned long,
	unsigned
);

void link_assign_ptr(link_t*, unsigned, node_t**);
void link_assign_va(link_t*, unsigned, va_list);
void link_assign(link_t*, unsigned, ...);
void link_deliver(link_t*, packet_t*);

////////////////////////////////////////

#endif
