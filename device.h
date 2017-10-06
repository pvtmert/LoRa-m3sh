
#ifndef _LORA_DEVICE_H_
#define _LORA_DEVICE_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>

#include <unistd.h>
#include <pthread.h>

#include "list.h"
#include "lora.h"

typedef enum DeviceState {
	DEVICE_STATE_NONE = 0,
	DEVICE_STATE_KILL = 1 << 0,
	DEVICE_STATE_EXEC = 1 << 1,
	DEVICE_STATE_LOCK = 1 << 2,
	DEVICE_STATE_1    = 1 << 3,
	DEVICE_STATE_2    = 1 << 4,
	DEVICE_STATE_3    = 1 << 5,
	DEVICE_STATE_4    = 1 << 6,
	DEVICE_STATE_5    = 1 << 7,
	DEVICE_STATE_BITS = 8,
} device_state_t;

typedef struct Device {
	unsigned identity;
	struct Node *node;
	pthread_t thread;
	struct Queue {
		struct List *send;
		struct List *recv;
		struct List *pkgs;
		unsigned buf_size;
	} queue;
	device_state_t state: DEVICE_STATE_BITS;
} device_t;

device_t* device_make(unsigned, struct Node*);
void* device_daemon(device_t*);

#endif
