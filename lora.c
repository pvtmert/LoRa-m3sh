
#include "lora.h"

///
/// \file lora.h
///
/// includes all functions required for
/// network simulation

////////////////////////////////////////

///
/// generic functions
///

///
/// \brief a 32-bit checksum algorithm.
/// used to checksum packet contents
/// to verify its authenticity
///
/// \param data pointer to data
/// \param words length of data
///
/// \return uint32_t (32-bits unsigned int) generated from sum
///
uint32_t fletcher32(uint16_t const *data, size_t words) {
	uint32_t sum1 = 0xffff, sum2 = 0xffff;
	size_t tlen;
	while (words) {
		tlen = ((words >= 359) ? 359 : words);
		words -= tlen;
		do {
			sum2 += sum1 += *data++;
			tlen--;
		} while (tlen);
		sum1 = (sum1 & 0xffff) + (sum1 >> 16);
		sum2 = (sum2 & 0xffff) + (sum2 >> 16);
	}
	sum1 = (sum1 & 0xffff) + (sum1 >> 16);
	sum2 = (sum2 & 0xffff) + (sum2 >> 16);
	return (sum2 << 16) | sum1;
}

///
/// \brief print error message with varargs.
/// \warning prints to stderr by default.
///
/// \param message message to be formatted
/// \param ... other arguments
///
void errorv(const char *message, ...) {
	va_list args;
	va_start(args, message);
	vfprintf(stderr, message, args);
	va_end(args);
	return;
}

////////////////////////////////////////

///
/// packet related functions
///

///
/// \brief create a new packet.
///
/// \param src source address
/// \param dst destination address
/// \param type flags of packet
/// \param ttl time to live
///
/// \return reference to created packet
///
packet_t* packet_make(const addr_t src, const addr_t dst,
	const packet_type_t type, const uint16_t ttl) {
	packet_t *packet = (packet_t*) malloc(sizeof(packet_t));
	memset(packet, 0, sizeof(packet_t));
	packet->src = src;
	packet->dst = dst;
	packet->ttl = ttl;
	packet->type = type;
	packet->checksum = 0;
//	packet->length = length;
//	memcpy(packet->data, data, length);
	return packet;
}


#define _LORA_PKG_FMT_ "%08X"
///
/// \brief print packet and its data.
///
/// \param pkg packet to be printed
/// \param width character wise screen width
/// used for hexdump-width wrap
///
void packet_dump(packet_t *pkg, size_t width) {
	printf(
		"PKG: " _LORA_PKG_FMT_ " -> " _LORA_PKG_FMT_ "\n"
		"\tTimeToLive: %u\n"
		"\tChecksum:   %08x\n"
		"\tType:       %02x\n"
		"\tDataLen:    %u\n",
		pkg->src,
		pkg->dst,
		pkg->ttl,
		pkg->checksum,
		pkg->type,
		pkg->length
	);
	if(width) {
		for(int i=0; i<pkg->length; i++) {
			if(!(i%width)) {
				printf("\n %x |", i);
			}
			printf(" %02x", *(char*)(pkg->data + i));
			continue;
		}
		printf("\n");
	}
	printf(" --- \n");
	return;
}

///
/// \brief prepare packet for sending.
/// this includes setting data
/// it also calculates checksum
///
/// \param pkg packet to be prepared
/// \param length length of data
/// \param data pointer to data (will be copied)
///
/// \return checksum of modified packet
///
uint32_t packet_prepare(packet_t *pkg, const uint16_t length, const void *data) {
	if(!pkg) {
		return 0;
	}
	memset(pkg->data, 0, sizeof(pkg->data));
	memcpy(pkg->data, data, length);
	pkg->length = length;
	pkg->checksum = 0;
	pkg->checksum = fletcher32((uint16_t const*)pkg, sizeof(packet_t));
	return pkg->checksum;
}

////////////////////////////////////////

///
/// node related functions
///

///
/// \brief creates a 'node'
/// which is a virtual device state
///
/// \param id 64-bit identifier
/// \param internet is node have internet (eg gateway)
///
/// \return reference to created node
///
node_t* node_make(const uint64_t id, const bool internet) {
	node_t *node = (node_t*) malloc(sizeof(node_t));
	memset(node, 0, sizeof(node_t));
	node->internet = internet;
	node->links = NULL;
	node->count = 0;
	node->id = id;
	//*(unsigned short*)(&node->mem_size) = 1 << 7;
	//node->last_pkgs = list_make(NULL);
	node->device = NULL;
	return node;
}

///
/// \brief print node and its information
///
/// \param node reference to be printed
/// \param links also include link-reference addresses
///
void node_dump(node_t *node, bool links) {
	printf(
		"NODE: %llu, hasConnection: %s, thread:%p \n",
		node->id,
		node->internet ? "true" : "false",
		pthread_self()
	);
	if(links) {
		for(int i=0; i<node->count; i++) {
			link_t *self = node->links[i];
			printf("\tLink: %p\n", self);
			continue;
		}
		printf("\n");
	}
	printf(" --- \n");
	return;
}

///
/// \brief called when node supposed to receive packet
/// and it belongs to that node
/// (generally called from link)
///
/// \param node reference to node
/// \param pkg reference to packet
///
void node_recv(node_t *node, packet_t *pkg) {
	printf("NODE RECV node.id:%p pkg:%p thread:%p \n",
		node->id, pkg, pthread_self());
	if(node->device && node->device->queue.recv) {
		list_push(&node->device->queue.recv, list_make(pkg));
	}
	print(pkg, sizeof(packet_t));
	return;
}

///
/// \brief called when node should relay the packet
/// node relays packet to all available links
///
/// \param node reference to node
/// \param pkg reference to packet
///
void node_relay(node_t *node, packet_t *pkg) {
	printf("NODE RELAY node.id:%p pkg:%p thread:%p \n",
		(void*)node->id, pkg, pthread_self());
	if(pkg->ttl < 1) {
		printf("TTL = %hu \n", pkg->ttl);
		return;
	}
	packet_t *copy = pkg;
	//(packet_t*) malloc(sizeof(packet_t));
	//memcpy(copy, pkg, sizeof(packet_t));
	copy->ttl -= 1;
	for(int i=0; i<node->count; i++) {
		link_t *self = node->links[i];
		link_deliver(self, copy);
		continue;
	}
	return;
}

///
/// \brief called when node receives packet from link
///
///
/// \param node reference to node
/// \param pkg reference to packet
///
void node_deliver(node_t *node, packet_t *pkg) {
	printf("NODE DELIVER: node.id:%p pkg:%p thread:%p \n\t",
		node->id, pkg, pthread_self());
	if(pkg->src == node->id) {
		printf("returned to start\n");
		return;
	}
	if(!pkg->checksum) {
		printf("zero checksum package not allowed\n");
		return;
	}
	// if(list_has(node->last_pkgs, (void*)pkg->checksum) != NULL) {
	// 	printf("already have pkg.chksum:%p\n", (void*)pkg->checksum);
	// 	return;
	// }
	// list_push(&node->last_pkgs, list_make((void*)pkg->checksum));
	// if(list_size(node->last_pkgs) > node->mem_size) {
	// 	list_t *keep = list_find(node->last_pkgs, list_last(node->last_pkgs));
	// 	list_clean(&keep, false);
	// }
	if(pkg->dst == node->id) {
		node_recv(node, pkg);
	} else {
		node_relay(node, pkg);
	}
	return;
}

///
/// \brief connect nodes via count number of links
/// \note links must be pre-allocated beforehand
/// this function uses ptr to link-ptr
///
/// \param node reference to node
/// \param count how many links to be added
/// \param links actual references to those links (to pointers)
///
void node_connect_ptr(node_t *node, unsigned count, link_t **links) {
	unsigned offset = node->count;
	node->links = (link_t**) realloc(
		node->links,
		(offset + count) * sizeof(link_t*)
	);
	for(int i=0; i<count; i++) {
		node->links[offset + i] = links[i];
		continue;
	}
	node->count += count;
	return;
}

///
/// \brief connect nodes via count number of links
/// links must be passed as a va_list typed
/// variable argument list
///
/// \param node reference to node
/// \param count number of links
/// \param args va_list that contains pointer-to links
///
void node_connect_va(node_t *node, unsigned count, va_list args) {
	link_t **temp = (link_t**) malloc(count * sizeof(link_t*));
	for(int i=0; i<count; i++) {
		temp[i] = va_arg(args, link_t*);
		continue;
	}
	node_connect_ptr(node, count, temp);
	free(temp);
	return;
}

///
/// \brief connect nodes via count number of links.
/// you should have count number of links after count arg
/// example: node_connect(node, 3, link1, link2, link3)
///
/// \param node reference to node
/// \param count number of links
/// \param ... arguments to
///
void node_connect(node_t *node, unsigned count, ...) {
	va_list args;
	va_start(args, count);
	node_connect_va(node, count, args);
	va_end(args);
	return;
}

////////////////////////////////////////

///
/// link related functions
///

///
/// \brief allocate and initialize link object
/// with given latency and deviation
///
/// \param latency base latency level
/// \param deviation deviation level
///
/// \return reference to created link
///
link_t* link_make(unsigned long latency, unsigned deviation) {
	link_t *link = (link_t*) malloc(sizeof(link_t));
	memset(link, 0, sizeof(link_t));
	link->deviation = deviation;
	link->latency = latency;
	link->nodes = NULL;
	link->count = 0;
	return link;
}

///
/// \brief print link and its parameters
///
/// \param link reference to link
/// \param nodes also print node addresses?
///
void link_dump(link_t *link, bool nodes) {
	printf(
		"LINK: Latency/Deviation: %lu / %u, thread:%p \n",
		link->latency,
		link->deviation,
		pthread_self()
	);
	if(nodes) {
		for(int i=0; i<link->count; i++) {
			node_t *self = link->nodes[i];
			printf("\tNode: %p\n", self);
			continue;
		}
		printf("\n");
	}
	printf(" --- \n");
	return;
}

#define _LORA_MS_MULTIP 1000
///
/// \brief causes link to publish packet to connected nodes
/// called when link got a packet
///
/// \param link reference to link
/// \param pkg packet-reference to be delivered
///
void link_deliver(link_t *link, packet_t *pkg) {
	printf("LINK DELIVER: link:%p pkg:%p thread:%p \n",
		link, pkg, pthread_self());
	srand(time(NULL));
	usleep(link->latency * _LORA_MS_MULTIP);
	for(int i=0; i<link->count; i++) {
		node_t *self = link->nodes[i];
		usleep(_LORA_MS_MULTIP * (rand() % link->deviation));
		node_deliver(self, pkg);
		continue;
	}
	return;
}

///
/// \brief assign nodes to a link via ptr to node-ptr
///
/// \param link reference to link
/// \param count number of nodes to connect
/// \param nodes pointer-to node-pointers
///
void link_assign_ptr(link_t *link, unsigned count, node_t **nodes) {
	unsigned offset = link->count;
	link->nodes = (node_t**) realloc(
		link->nodes,
		(offset + count) * sizeof(node_t*)
	);
	for(int i=0; i<count; i++) {
		link->nodes[offset + i] = nodes[i];
		continue;
	}
	link->count += count;
	return;
}

///
/// \brief assign nodes to a link via variable argument list of nodes
///
/// \param link reference to link
/// \param count number of nodes
/// \param args va_list typed node-pointers
///
void link_assign_va(link_t *link, unsigned count, va_list args) {
	node_t **temp = (node_t**) malloc(count * sizeof(node_t*));
	for(int i=0; i<count; i++) {
		temp[i] = va_arg(args, node_t*);
		continue;
	}
	link_assign_ptr(link, count, temp);
	free(temp);
	return;
}

///
/// \brief assign count number of nodes to a link
/// via variable sized arguments
///
/// \param link reference to link
/// \param count number of nodes to connect
/// \param ... arguments to nodes
///
void link_assign(link_t *link, unsigned count, ...) {
	va_list args;
	va_start(args, count);
	link_assign_va(link, count, args);
	va_end(args);
	return;
}

////////////////////////////////////////

///
/// \brief prints given object according to its size
/// uses switch statement to find out object type
///
/// \param obj object-reference to be used
/// \param size sizeof of object to determine its type
///
void print(void *obj, size_t size) {
	switch(size) {
		case sizeof(packet_t):
			packet_dump(obj, 16);
			break;
		case sizeof(node_t):
			node_dump(obj, true);
			break;
		case sizeof(link_t):
			link_dump(obj, true);
			break;
		default:
			errorv("> unknown type");
	}
	//printf("\r");
	fflush(stdout);
	return;
}
