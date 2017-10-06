
#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#include "device.h"
#include "daemon.h"

#include "hash.h"

#define LENGTH(obj, type) ( sizeof(obj)/sizeof(type) )
#define ERROR(params, ...) ( fprintf(stderr, params "\n", ##__VA_ARGS__ ) )
#define WARNING(params, ...) ( fprintf(stderr, params "\n", ##__VA_ARGS__ ) )

typedef struct ShellCmd {
	int (*fp)(const char**);
	const char *cmd;
	const int ident;
} shell_cmd_t;

static list_t
	*shell_devs = NULL,
	*shell_nodes = NULL,
	*shell_links = NULL,
	*shell_packets = NULL;

static daemon_t looper;
extern void init(void);

///
/// \brief tokenizer for given string.
/// divides string to tokens (words)
/// so commands can be parsed and passed
/// easily
///
/// \param str string reference to be tokenized
///
/// \return reference to char-ptr (string-array)
///
char **tokenize(const char *str) {
	volatile unsigned size = 0;
	char **res = NULL;
	char *temp = NULL;
	// strsep: arg1 cast `char**` to fix warn
	while(temp = strsep((char**)&str, " \t\r\n\0")) {
		if(!strlen(temp)) {
			continue;
		}
		size += 1;
		res = (char**) realloc(res, size * sizeof(char*));
		res[size-1] = temp;
		continue;
	}
	// add extra null delimiter
	res = (char**) realloc(res, ++size * sizeof(char*));
	res[size-1] = NULL;
	return res;
}

void shell_looper(daemon_t *d) {
	time_t start = time(NULL);
	while(time(NULL) - start < d->runtime) {
		// simulation running
		malloc(1);
		continue;
	}
	d->thread = NULL;
	return;
}

int shell_cmd_exit(const char **args, ...) {
	return 0;
}

int shell_cmd_status(const char **args, ...) {
	return 1;
}

int shell_cmd_start(const char **args, ...) {
	if(args && *args) {
		WARNING("start call does not require argument");
	}
	if(!looper.thread) {
		pthread_create(&looper.thread, NULL, &shell_looper, &looper);
	} else {
		ERROR("a thread (%p) was already active", looper.thread);
	}
	return 1;
}

int shell_cmd_stop(const char **args, ...) {
	return 1;
}

int shell_cmd_dev_add(const char **args, ...) {
	return 1;
}

int shell_cmd_dev_del(const char **args, ...) {
	return 1;
}

int shell_cmd_node_add(const char **args, ...) {
	return 1;
}

int shell_cmd_node_del(const char **args, ...) {
	return 1;
}

int shell_cmd_packet_make(const char **args, ...) {
	return 1;
}

int shell_cmd_link_make(const char **args, ...) {
	return 1;
}

int shell_cmd_list(const char **args, ...) {
	char **list = args;
	while(list && *list) {
		printf("arg: %s\n", *(list++));
		continue;
	}
	return 1;
}

int shell_cmd_type(const char **args, ...) {
	return 1;
}

int shell_cmd_help(const char **args, ...) {
	printf(
		" --- HELP --- \n"
		"Contents:\n"
		"\t1) lorem ipsum\n"
		"\t2) dolor sit amet\n"
		"\n"
	);
	return 1;
}


int shell_cmd_prefs(const char **args, bool state) {
	if(!args && !*args) {
		return 2;
	}
	switch(HASH(args[1])) {
		case HASH("runtime"):
			if(state)
				looper.runtime = atoll(args[2]);
			printf("runtime: %ld\n", looper.runtime);
			break;
		default:
			ERROR("unknown variable");
			break;
	}
	return 1;
}

int shell_cmd_prefs_get(const char **args, ...) {
	return shell_cmd_prefs(args, false);
}

int shell_cmd_prefs_set(const char **args, ...) {
	return shell_cmd_prefs(args, true);
}

static const
shell_cmd_t commands[] = {
	{
		.fp = &shell_cmd_start,
		.cmd = "start",
	},
	{
		.fp = &shell_cmd_stop,
		.cmd = "stop",
	},
	{
		.fp = &shell_cmd_exit,
		.cmd = "q",
	},
	{
		.fp = &shell_cmd_list,
		.cmd = "ls",
	},
	{
		.fp = &shell_cmd_type,
		.cmd = "cat",
	},
	{
		.fp = &shell_cmd_help,
		.cmd = "help",
	},
	{
		.fp = &shell_cmd_help,
		.cmd = "?",
	},
	{
		.fp = &shell_cmd_prefs_get,
		.cmd = "prefs",
	},
	{
		.fp = &shell_cmd_prefs_set,
		.cmd = "prefs",
	},
};

char* prompt(const char *fmt, ...) {
	va_list args;
	char buffer[80] = {0};
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
	if(!fgets(buffer, sizeof(buffer), stdin)) {
		return NULL;
	}
	unsigned length = 1 + strlen(buffer);
	char *res = (char*) calloc(length, sizeof(char));
	if(res) {
		strncpy(res, buffer, length);
	}
	return res;
}

void shell_free(void **obj) {
	if(!obj || !*obj) {
		return;
	}
	free(*obj);
	*obj = NULL;
	return;
}

void shell(const char **args) {
	for(int i=0; i<LENGTH(commands, shell_cmd_t); i++) {
		*(int*)(&commands[i].ident) = HASH(commands[i].cmd);
		continue;
	}
	volatile short retcode = 1;
	volatile char *input = NULL;
	volatile char **tokens = NULL;
	while(retcode) {
		if(!(input = prompt("%%lora>"))) {
			printf("\n");
			return;
		}
		tokens = tokenize(input);
		for(int i=0; tokens && *tokens && i<LENGTH(commands, shell_cmd_t); i++) {
			if(strncmp(commands[i].cmd, *tokens, strlen(commands[i].cmd))) {
				continue;
			}
			retcode = (commands[i].fp)(tokens+1);
			break;
		}
		shell_free(&tokens);
		shell_free(&input);
		continue;
	}
	shell_free(&input);
	return;
}
