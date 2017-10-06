
#include "device.h"
#include "daemon.h"

void recurse(void *ptr, size_t size, list_t *root, void (*callback)(void*, size_t)) {
	bool first = false;
	if(root) {
		if(list_has(root, ptr)) {
			return;
		}
	} else {
		first = true;
		root = list_make(NULL);
	}
	size_t type = 0;
	void **next = NULL;
	unsigned count = 0;
	callback(ptr, size);
	switch(size) {
		case sizeof(link_t):
			type = sizeof(node_t);
			next = (void**)((link_t*)ptr)->nodes;
			count = ((link_t*)ptr)->count;
			break;
		case sizeof(node_t):
			type = sizeof(link_t);
			next = (void**)((node_t*)ptr)->links;
			count = ((node_t*)ptr)->count;
			break;
	}
	list_add(root, list_init(ptr, size));
	for(int i=0; i<count; i++) {
		recurse(next[i], type, root, callback);
		continue;
	}
	if(first) {
		list_clean(&root, false);
	}
	return;
}

bool contains(void **objects, void *object, unsigned count) {
	for(int i=0; i<count; i++) {
		if(objects[i] == object) {
			return true;
		}
		continue;
	}
	return false;
}

void pair(unsigned count, node_t **nodes, link_t **links) {
	const unsigned cnt = 2;
	for(int i=0; i<count; i++) {
		if(links[i]) {
			//link_assign_ptr(links[i], count, nodes);
			node_t **self = links[i]->nodes;
			if(self && *self) {
				free(*self);
			}
			links[i]->count = cnt;
			self = (node_t**) malloc(cnt * sizeof(node_t*));
			for(int j=0; j<cnt; j++) {
				self[j] = nodes[ (i+j)%count ];
				continue;
			}
			links[i]->nodes = self;
		}
		if(nodes[i]) {
			node_connect_ptr(nodes[i], count, links);
		}
		continue;
	}
	return;
}

void run(unsigned count, node_t **nodes) {
	for(int i=0; i<count; i++) {
		device_t *dev = device_make(i+1, nodes[i]);
		pthread_create(&dev->thread, NULL, device_daemon, dev);
		continue;
	}
	return;
}

void kill(unsigned count, node_t **nodes) {
	for(int i=0; i<count; i++) {
		nodes[i]->device->state = DEVICE_STATE_KILL;
		pthread_join(nodes[i]->device->thread, NULL);
		continue;
	}
	return;
}

int rand(void) {
	static int last;
	if(last < 0) {
		last = 0;
	}
	return last++;
}

unsigned long rand_skip(unsigned long min, unsigned long max, unsigned long skip) {
	return min + ((skip + (rand() % (max-min-1))) % (max-min));
}

void transaction_handler(transaction_t *transaction) {
	printf(">>> TRANSACTION: %p:%p STARTED \n", transaction, pthread_self());
	node_deliver(transaction->entry_pt, transaction->packet);
	transaction->finished = true;
	printf(">>> TRANSACTION: %p:%p ENDED \n", transaction, pthread_self());
	return;
}

void transaction_process(daemon_t *daemon) {
	list_t *transactions = daemon->transactions;
	while(transactions) {
		transaction_t *transaction = (transaction_t*) transactions->data;
		if(!transaction) {
			goto next;
		}
		if(transaction->finished) {
			pthread_join(transaction->thread, NULL);
			list_t *prev = list_find(daemon->transactions, transaction);
			if(prev) {
				list_t *deleted = list_del(&prev->next);
				if(deleted) {
					free(deleted->data);
					free(deleted);
				}
			}
			//free(transaction);
			goto next;
		}
		if(!transaction->thread) {
			pthread_create(&transaction->thread, NULL,
				&transaction_handler, transaction);
			transaction->finished = false;
			goto next;
		}
		next: transactions = transactions->next;
		continue;
	}
	return;
}

void transaction_generate(daemon_t *daemon, unsigned max) {
	unsigned src = rand()%max;
	unsigned dst = rand_skip(0, max, src);
	packet_t *pkg = packet_make(
		daemon->nodes[src]->id,
		daemon->nodes[dst]->id,
		PKG_TYPE_NONE, 3
	);
	packet_prepare(pkg, 4, "test");
	transaction_t *transaction =
		(transaction_t*) malloc(sizeof(transaction_t));
	memset(transaction, 0, sizeof(transaction_t));
	transaction->thread = NULL;
	transaction->packet = pkg;
	transaction->entry_pt = daemon->nodes[rand_skip(0, max, dst)];
	list_push(&daemon->transactions, list_make(transaction));
	return;
}

void main_daemon(daemon_t *daemon) {
	time_t start = time(NULL);
	for(int i=0; i<5; i++) transaction_generate(daemon, 5);
	while(!daemon->runtime || time(NULL) - start < daemon->runtime) {
		msleep(pow(10, daemon->latency.exp) * daemon->latency.base);
		transaction_process(daemon);
		continue;
	}
	printf("##### FINISHING: %p:%p \n", daemon, pthread_self());
	while(daemon->transactions && list_size(daemon->transactions)) {
		transaction_process(daemon);
		continue;
	}
	printf("##### FINISHED: %p:%p \n", daemon, pthread_self());
	return;
}

void init(void) {
	const unsigned lat = 100;
	node_t *nodes[] = {
		node_make(1, false),
		node_make(2, false),
		node_make(3, false),
		node_make(4, false),
		node_make(5, false),
	};
	link_t *links[] = {
		link_make(lat, 1),
		link_make(lat, 2),
		link_make(lat, 3),
		link_make(lat, 4),
		link_make(lat, 5),
	};
	pair(5, nodes, links);
	recurse(nodes[0], sizeof(node_t), NULL, print);
	printf("----------------------------------\n");
	// packet_t *pkg = packet_make(1, 2, 0xFF, 2, "mert", 4);
	// print(pkg, sizeof(packet_t));
	// printf("new chksum: %08X\n", packet_prepare(pkg));
	run(5, nodes);
	daemon_t opts = {
		.id = {
			.min = 1,
			.max = 5,
		},
		.latency = {
			.base = 1,
			.exp = 1,
		},
		.runtime = 5,
		.links = links,
		.nodes = nodes,
		.transactions = NULL,
	};
	pthread_create(&opts.thread, NULL, &main_daemon, &opts);
	pthread_join(opts.thread, NULL);
	kill(5, nodes);
	return;
}
