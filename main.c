
#include <dlfcn.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "hash.h"

int main(int argc, const char **argv) {
	//void (*init)(void);
	// void *lib = dlopen("liblora", RTLD_LAZY|RTLD_LOCAL);
	// if(!lib) {
	// 	printf("%s\n", dlerror());
	// } else {
	// 	*(void**) (&init) = dlsym(lib, "init");
	// 	(*init)();
	// 	dlclose(lib);
	// }
	void init(void);
	void shell(const char**);
	if(argc < 2) {
		return 1;
	}
	/* the old code
	switch(hash(argv[1])) {
		case 90905899: // run
			init();
			break;
		case 107912124: // shell
			shell(argv+2);
			break;
		default:
			for(int i=1; i<argc; i++) {
				printf("%s: %llu\n", argv[i], hash(argv[i]));
				continue;
			}
			break;
	}
	*/
	switch(HASH(argv[1])) {
		case HASH("test"):
			init();
			break;
		case HASH("shell"):
			shell(argv+2);
			break;
		default:
			break;
	}
	return 0;
}
