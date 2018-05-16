#include <stdio.h>

#include "aba.h"

static void a() {
	printf("HERE\n");
}

static void b() {
	for (size_t i = 0; i < 10; ++i) {
		a();
	}
	a();
}

static void print_args(int const argc, char * const * const argv) {
	for (int i = 0; i < argc; ++i)
		printf("%s\n", argv[i]);
}	

int main(int argc, char ** argv) {
	b();
	a();
	aba();
	print_args(argc, argv);
	return 0;
}
