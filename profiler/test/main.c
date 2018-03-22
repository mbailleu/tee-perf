#include <stdio.h>

static void a() {
	printf("HERE\n");
}

static void b() {
	for (size_t i = 0; i < 10; ++i) {
		a();
	}
	asm volatile (
			"nop\n"
			"nop\n"
			"nop\n"
			"nop\n"
		);
	a();
}

int main() {
	b();
	a();
	return 0;
}
