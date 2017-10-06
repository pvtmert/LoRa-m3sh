
#ifndef _LORA_DAEMON_H_
#define _LORA_DAEMON_H_

typedef struct Daemon {
	struct ID {
		short min;
		short max;
	} id;
	struct Latency {
		unsigned base;
		unsigned exp;
	} latency;
	pthread_t thread;
	uint64_t runtime;
	link_t **links;
	node_t **nodes;
	list_t *transactions;
} daemon_t;

typedef struct Transaction {
	pthread_t thread;
	packet_t *packet;
	node_t *entry_pt;
	bool finished;
} transaction_t;

static int msleep(unsigned long mseconds) {
	return usleep(mseconds * 1000);
}

#endif
