#include "aba.h"

void aba() {
	for (size_t i = 0; i < 10000; ++i) {
		asm volatile ("");
	}
}
