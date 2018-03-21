#include <stdio.h>

static void a() {
	printf("HERE\n");
}

static void b() {
	for (size_t i = 0; i < 10; ++i) {
		a();
	}
}

int main() {
	b();
	return 0;
}
