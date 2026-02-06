/* pikabench.c - Benchmark utilities for Pika */

void test_compiler_barrier(void) {
	__asm__ volatile("" ::: "memory");
}

/* Pika emits rt_compiler_barrier() from benchmark_compiler_barrier() calls */
#define benchmark_compiler_barrier test_compiler_barrier
