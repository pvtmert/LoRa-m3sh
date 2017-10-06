
#include "device.h"

device_t* device_make(unsigned id, struct Node *node) {
	device_t *device = (device_t*) malloc(sizeof(device_t));
	device->identity = id;
	device->node = node;
	// reverse connection
	node->device = device;
	return device;
}

void device_daemon_check_queue(struct Queue *q) {
	if(q->send) {
		list_t *tail = list_last(q->send);
		packet_t *pkg = (packet_t*) tail->data;
		// send packet here
		list_t *prev = list_find(q->recv, tail);
		if(prev) {
			list_t *del = list_del(&prev->next);
			free(del);
		}
	}
	if(q->recv) {
		list_t *tail = list_last(q->recv);
		packet_t *pkg = (packet_t*) tail->data;
		if(!list_has(q->pkgs, (void*)pkg->checksum)) {
			list_push(&q->pkgs, list_make((void*)pkg->checksum));
			print(tail->data, sizeof(packet_t));
		}
		list_t *prev = list_find(q->recv, tail);
		if(prev) {
			list_t *del = list_del(&prev->next);
			free(del);
		}
		printf("DEVICE: packet dequeued %p \n", pthread_self());
	}
	if(list_size(q->pkgs) > q->buf_size) {
		list_t *tail = list_last(q->pkgs);
		list_t *prev = list_find(q->pkgs, tail);
		list_t *del = list_del(&prev->next);
		free(del);
	}
	return;
}

void device_init(device_t *device) {
	struct Queue *q = &device->queue;
	q->send = (NULL);
	q->recv = (NULL);
	q->pkgs = (NULL);
	q->buf_size = 1 << 8;
	device->state = DEVICE_STATE_EXEC;
	return;
}

void* device_daemon(device_t *device) {
	device_init(device);
	while(device->state != DEVICE_STATE_KILL) {
		device_daemon_check_queue(&device->queue);
		usleep(100 * 1000);
		continue;
	}
	printf("NODE KILLED: %llu\n", device->node->id);
	return NULL;
}
