
#include <stdio.h>
#include <stdbool.h>

#include "lora.h"

int main(int argc, char **argv) {
	printf("%x\n", packet_make(0, 0, 0, 0)->magic);
	return 0;
}
