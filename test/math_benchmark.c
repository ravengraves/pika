// Final benchmark for pika2c generated math library
#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define N 1000000
#define RUNS 30

static float input[N] __attribute__((aligned(32)));
static float input2[N] __attribute__((aligned(32)));
static float output[N] __attribute__((aligned(32)));
static float output2[N] __attribute__((aligned(32)));
volatile float sink;

static double now_sec(void) {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec + ts.tv_nsec * 1e-9;
}

#define BENCH1(name, libm_fn, pika_fn) \
static void bench_libm_##name(float* o, const float* i, int n) { \
	for (int j = 0; j + 4 <= n; j += 4) { \
		o[j+0] = libm_fn(i[j+0]); o[j+1] = libm_fn(i[j+1]); \
		o[j+2] = libm_fn(i[j+2]); o[j+3] = libm_fn(i[j+3]); \
	} \
} \
static void bench_pika_##name(float* o, const float* i, int n) { \
	for (int j = 0; j + 4 <= n; j += 4) { \
		o[j+0] = pika_fn(i[j+0]); o[j+1] = pika_fn(i[j+1]); \
		o[j+2] = pika_fn(i[j+2]); o[j+3] = pika_fn(i[j+3]); \
	} \
}

#define BENCH2(name, libm_fn, pika_fn) \
static void bench_libm_##name(float* o, const float* x, const float* y, int n) { \
	for (int j = 0; j + 4 <= n; j += 4) { \
		o[j+0] = libm_fn(x[j+0], y[j+0]); o[j+1] = libm_fn(x[j+1], y[j+1]); \
		o[j+2] = libm_fn(x[j+2], y[j+2]); o[j+3] = libm_fn(x[j+3], y[j+3]); \
	} \
} \
static void bench_pika_##name(float* o, const float* x, const float* y, int n) { \
	for (int j = 0; j + 4 <= n; j += 4) { \
		o[j+0] = pika_fn(x[j+0], y[j+0]); o[j+1] = pika_fn(x[j+1], y[j+1]); \
		o[j+2] = pika_fn(x[j+2], y[j+2]); o[j+3] = pika_fn(x[j+3], y[j+3]); \
	} \
}

BENCH1(recip, 1.0f/, math_recip)
BENCH1(rsqrt, 1.0f/sqrtf, math_rsqrt)
BENCH1(sqrt, sqrtf, math_sqrt)
BENCH1(exp, expf, math_exp)
BENCH1(log, logf, math_log)
BENCH1(sin, sinf, math_sin)
BENCH1(cos, cosf, math_cos)
BENCH1(tan, tanf, math_tan)
BENCH1(atan, atanf, math_atan)
BENCH1(asin, asinf, math_asin)
BENCH1(acos, acosf, math_acos)
BENCH2(pow, powf, math_pow)
BENCH2(atan2, atan2f, math_atan2)

static void bench_libm_sincos(float* s, float* c, const float* i, int n) {
	for (int j = 0; j + 4 <= n; j += 4) {
		s[j+0] = sinf(i[j+0]); c[j+0] = cosf(i[j+0]);
		s[j+1] = sinf(i[j+1]); c[j+1] = cosf(i[j+1]);
		s[j+2] = sinf(i[j+2]); c[j+2] = cosf(i[j+2]);
		s[j+3] = sinf(i[j+3]); c[j+3] = cosf(i[j+3]);
	}
}
static void bench_pika_sincos(float* s, float* c, const float* i, int n) {
	for (int j = 0; j + 4 <= n; j += 4) {
		MATH_SINCOS_RESULT r0 = math_sincos(i[j+0]);
		MATH_SINCOS_RESULT r1 = math_sincos(i[j+1]);
		MATH_SINCOS_RESULT r2 = math_sincos(i[j+2]);
		MATH_SINCOS_RESULT r3 = math_sincos(i[j+3]);
		s[j+0] = r0.s; c[j+0] = r0.c;
		s[j+1] = r1.s; c[j+1] = r1.c;
		s[j+2] = r2.s; c[j+2] = r2.c;
		s[j+3] = r3.s; c[j+3] = r3.c;
	}
}

typedef void (*fn1)(float*, const float*, int);
typedef void (*fn2)(float*, const float*, const float*, int);
typedef void (*fnsc)(float*, float*, const float*, int);

static double run1(fn1 f) {
	f(output, input, N);
	double best = 1e30;
	for (int r = 0; r < RUNS; r++) {
		double t0 = now_sec();
		f(output, input, N);
		double t1 = now_sec();
		if (t1-t0 < best) best = t1-t0;
	}
	sink = output[N/2];
	return best;
}

static double run2(fn2 f) {
	f(output, input, input2, N);
	double best = 1e30;
	for (int r = 0; r < RUNS; r++) {
		double t0 = now_sec();
		f(output, input, input2, N);
		double t1 = now_sec();
		if (t1-t0 < best) best = t1-t0;
	}
	sink = output[N/2];
	return best;
}

static double runsc(fnsc f) {
	f(output, output2, input, N);
	double best = 1e30;
	for (int r = 0; r < RUNS; r++) {
		double t0 = now_sec();
		f(output, output2, input, N);
		double t1 = now_sec();
		if (t1-t0 < best) best = t1-t0;
	}
	sink = output[N/2] + output2[N/2];
	return best;
}

int main(void) {
	srand(42);
	
	printf("# pika2c Math Library Benchmark\n\n");
	printf("**Test:** 1M elements, 4x unrolled loops, best of %d runs\n\n", RUNS);
	
	printf("| Function | libm (µs) | pika (µs) | Speedup | Status |\n");
	printf("|----------|-----------|-----------|---------|--------|\n");
	
	double l, p, s;
	
	// recip
	for(int i=0; i<N; i++) input[i] = 0.1f + 10.0f*((float)rand()/RAND_MAX);
	l = run1(bench_libm_recip)*1e6; p = run1(bench_pika_recip)*1e6; s = l/p;
	printf("| recip    | %9.0f | %9.0f | %6.2fx | %s |\n", l, p, s, s>=1.5?"🚀 FASTER":s>=1.1?"✓ faster":s>=0.9?"≈ same":"⚠ slower");
	
	// rsqrt
	for(int i=0; i<N; i++) input[i] = 0.01f + 100.0f*((float)rand()/RAND_MAX);
	l = run1(bench_libm_rsqrt)*1e6; p = run1(bench_pika_rsqrt)*1e6; s = l/p;
	printf("| rsqrt    | %9.0f | %9.0f | %6.2fx | %s |\n", l, p, s, s>=1.5?"🚀 FASTER":s>=1.1?"✓ faster":s>=0.9?"≈ same":"⚠ slower");
	
	// sqrt
	l = run1(bench_libm_sqrt)*1e6; p = run1(bench_pika_sqrt)*1e6; s = l/p;
	printf("| sqrt     | %9.0f | %9.0f | %6.2fx | %s |\n", l, p, s, s>=1.5?"🚀 FASTER":s>=1.1?"✓ faster":s>=0.9?"≈ same":"⚠ slower");
	
	// exp
	for(int i=0; i<N; i++) input[i] = -10.0f + 20.0f*((float)rand()/RAND_MAX);
	l = run1(bench_libm_exp)*1e6; p = run1(bench_pika_exp)*1e6; s = l/p;
	printf("| exp      | %9.0f | %9.0f | %6.2fx | %s |\n", l, p, s, s>=1.5?"🚀 FASTER":s>=1.1?"✓ faster":s>=0.9?"≈ same":"⚠ slower");
	
	// log
	for(int i=0; i<N; i++) input[i] = 0.01f + 100.0f*((float)rand()/RAND_MAX);
	l = run1(bench_libm_log)*1e6; p = run1(bench_pika_log)*1e6; s = l/p;
	printf("| log      | %9.0f | %9.0f | %6.2fx | %s |\n", l, p, s, s>=1.5?"🚀 FASTER":s>=1.1?"✓ faster":s>=0.9?"≈ same":"⚠ slower");
	
	// pow
	for(int i=0; i<N; i++) { input[i] = 0.1f + 10.0f*((float)rand()/RAND_MAX); input2[i] = 0.5f + 2.0f*((float)rand()/RAND_MAX); }
	l = run2(bench_libm_pow)*1e6; p = run2(bench_pika_pow)*1e6; s = l/p;
	printf("| pow      | %9.0f | %9.0f | %6.2fx | %s |\n", l, p, s, s>=1.5?"🚀 FASTER":s>=1.1?"✓ faster":s>=0.9?"≈ same":"⚠ slower");
	
	// sin
	for(int i=0; i<N; i++) input[i] = -10.0f + 20.0f*((float)rand()/RAND_MAX);
	l = run1(bench_libm_sin)*1e6; p = run1(bench_pika_sin)*1e6; s = l/p;
	printf("| sin      | %9.0f | %9.0f | %6.2fx | %s |\n", l, p, s, s>=1.5?"🚀 FASTER":s>=1.1?"✓ faster":s>=0.9?"≈ same":"⚠ slower");
	
	// cos
	l = run1(bench_libm_cos)*1e6; p = run1(bench_pika_cos)*1e6; s = l/p;
	printf("| cos      | %9.0f | %9.0f | %6.2fx | %s |\n", l, p, s, s>=1.5?"🚀 FASTER":s>=1.1?"✓ faster":s>=0.9?"≈ same":"⚠ slower");
	
	// sincos
	l = runsc(bench_libm_sincos)*1e6; p = runsc(bench_pika_sincos)*1e6; s = l/p;
	printf("| sincos   | %9.0f | %9.0f | %6.2fx | %s |\n", l, p, s, s>=1.5?"🚀 FASTER":s>=1.1?"✓ faster":s>=0.9?"≈ same":"⚠ slower");
	
	// tan
	l = run1(bench_libm_tan)*1e6; p = run1(bench_pika_tan)*1e6; s = l/p;
	printf("| tan      | %9.0f | %9.0f | %6.2fx | %s |\n", l, p, s, s>=1.5?"🚀 FASTER":s>=1.1?"✓ faster":s>=0.9?"≈ same":"⚠ slower");
	
	// atan
	l = run1(bench_libm_atan)*1e6; p = run1(bench_pika_atan)*1e6; s = l/p;
	printf("| atan     | %9.0f | %9.0f | %6.2fx | %s |\n", l, p, s, s>=1.5?"🚀 FASTER":s>=1.1?"✓ faster":s>=0.9?"≈ same":"⚠ slower");
	
	// asin
	for(int i=0; i<N; i++) input[i] = -0.99f + 1.98f*((float)rand()/RAND_MAX);
	l = run1(bench_libm_asin)*1e6; p = run1(bench_pika_asin)*1e6; s = l/p;
	printf("| asin     | %9.0f | %9.0f | %6.2fx | %s |\n", l, p, s, s>=1.5?"🚀 FASTER":s>=1.1?"✓ faster":s>=0.9?"≈ same":"⚠ slower");
	
	// acos
	l = run1(bench_libm_acos)*1e6; p = run1(bench_pika_acos)*1e6; s = l/p;
	printf("| acos     | %9.0f | %9.0f | %6.2fx | %s |\n", l, p, s, s>=1.5?"🚀 FASTER":s>=1.1?"✓ faster":s>=0.9?"≈ same":"⚠ slower");
	
	// atan2
	for(int i=0; i<N; i++) { input[i] = -10.0f + 20.0f*((float)rand()/RAND_MAX); input2[i] = -10.0f + 20.0f*((float)rand()/RAND_MAX); }
	l = run2(bench_libm_atan2)*1e6; p = run2(bench_pika_atan2)*1e6; s = l/p;
	printf("| atan2    | %9.0f | %9.0f | %6.2fx | %s |\n", l, p, s, s>=1.5?"🚀 FASTER":s>=1.1?"✓ faster":s>=0.9?"≈ same":"⚠ slower");
	
	return 0;
}
