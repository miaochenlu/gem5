#include <stdio.h>
#include <stdint.h>
#ifdef _MSC_VER
#include <intrin.h> /* for rdtscp and clflush */
#pragma optimize("gt",on)
#else
#include <x86intrin.h> /* for rdtscp and clflush */
#endif

int main() {
	char a = 1;
	int junk = 0;
	printf("begin flushing\n");
	// printf("address of p: %p\n", &a);
	_mm_clflush(&a);
	_mm_mfence();
	a = 2;
	_mm_clflush(&a);
	_mm_mfence();
}

