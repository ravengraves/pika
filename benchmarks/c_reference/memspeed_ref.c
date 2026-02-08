/*
 *  Memory Speed Benchmark - C Reference Version for Linux
 *
 *  Based on Roy Longbottom's MemSpd2K concept.
 *  Tests memory bandwidth: dot product, vector add, vector copy
 *  for f64, f32, and i32 data types across various working set sizes.
 *
 *  Each test is a separate static inline function to give GCC/Clang
 *  the same optimization context as Pika's transpiled-C functions.
 *  With -ffast-math, this enables outer loop fusion on vector add:
 *  GCC merges multiple outer iterations into a single array pass,
 *  halving memory traffic (reads y once, computes x += y twice).
 *
 *  Matches the Pika memspeed.pka benchmark parameters.
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

#define MEM_MAXN 2097152

static double  mem_da[MEM_MAXN];
static double  mem_db[MEM_MAXN];
static float   mem_fa[4194304];
static float   mem_fb[4194304];
static int32_t mem_ia[4194304];
static int32_t mem_ib[4194304];

static double elapsed_s(struct timespec *start, struct timespec *end) {
    return (end->tv_sec - start->tv_sec) + (end->tv_nsec - start->tv_nsec) / 1e9;
}

static int recal(double elapsed, int n) {
    if (elapsed < 0.1 / 8.0)
        return n * 10;
    return (int)(0.1 * 1.25 / elapsed * n + 1.0);
}

/* === f64 dot product: sum += da[i] * db[i] === */
static inline double mem_dp_dot(int dn) {
    for (int i = 0; i < dn; i++) { mem_da[i] = 1.0; mem_db[i] = 1.0; }
    int nn = 6400000 / dn;
    if (nn < 1) nn = 1;
    double secs;
    for (;;) {
        double sum = 0.0;
        struct timespec ts, te;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        for (int r = 0; r < nn; r++) {
            for (int i = 0; i < dn; i++) sum += mem_da[i] * mem_db[i];
        }
        clock_gettime(CLOCK_MONOTONIC, &te);
        mem_da[0] = sum * 1e-20;  /* anti-DCE */
        secs = elapsed_s(&ts, &te);
        if (secs >= 0.1) break;
        nn = recal(secs, nn);
    }
    return (double)dn * nn * 16.0 / (1000000.0 * secs);
}

/* === f32 dot product: sum += fa[i] * fb[i] === */
static inline double mem_sp_dot(int fn) {
    for (int i = 0; i < fn; i++) { mem_fa[i] = 1.0f; mem_fb[i] = 1.0f; }
    int nn = 6400000 / fn;
    if (nn < 1) nn = 1;
    double secs;
    for (;;) {
        float sum = 0.0f;
        struct timespec ts, te;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        for (int r = 0; r < nn; r++) {
            for (int i = 0; i < fn; i++) sum += mem_fa[i] * mem_fb[i];
        }
        clock_gettime(CLOCK_MONOTONIC, &te);
        mem_fa[0] = sum * 1e-20f;
        secs = elapsed_s(&ts, &te);
        if (secs >= 0.1) break;
        nn = recal(secs, nn);
    }
    return (double)fn * nn * 8.0 / (1000000.0 * secs);
}

/* === i32 accumulate: sum += ia[i] + ib[i] === */
static inline double mem_int_dot(int in) {
    for (int i = 0; i < in; i++) { mem_ia[i] = 0; mem_ib[i] = 0; }
    mem_ib[in - 1] = 1;
    int nn = 6400000 / in;
    if (nn < 1) nn = 1;
    double secs;
    for (;;) {
        int32_t sum = 0;
        struct timespec ts, te;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        for (int r = 0; r < nn; r++) {
            for (int i = 0; i < in; i++) sum += mem_ia[i] + mem_ib[i];
        }
        clock_gettime(CLOCK_MONOTONIC, &te);
        mem_ia[0] = sum;
        secs = elapsed_s(&ts, &te);
        if (secs >= 0.1) break;
        nn = recal(secs, nn);
    }
    return (double)in * nn * 8.0 / (1000000.0 * secs);
}

/* === f64 add: da[i] = da[i] + db[i] === */
static inline double mem_dp_add(int dn) {
    for (int i = 0; i < dn; i++) { mem_da[i] = 0.0; mem_db[i] = 1.0; }
    int nn = 6400000 / dn;
    if (nn < 1) nn = 1;
    double secs;
    for (;;) {
        for (int i = 0; i < dn; i++) mem_da[i] = 0.0;
        struct timespec ts, te;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        for (int r = 0; r < nn; r++)
            for (int i = 0; i < dn; i++) mem_da[i] = mem_da[i] + mem_db[i];
        clock_gettime(CLOCK_MONOTONIC, &te);
        secs = elapsed_s(&ts, &te);
        if (secs >= 0.1) break;
        nn = recal(secs, nn);
    }
    return (double)dn * nn * 16.0 / (1000000.0 * secs);
}

/* === f32 add: fa[i] = fa[i] + fb[i] === */
static inline double mem_sp_add(int fn) {
    for (int i = 0; i < fn; i++) { mem_fa[i] = 0.0f; mem_fb[i] = 1.0f; }
    int nn = 6400000 / fn;
    if (nn < 1) nn = 1;
    double secs;
    for (;;) {
        for (int i = 0; i < fn; i++) mem_fa[i] = 0.0f;
        struct timespec ts, te;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        for (int r = 0; r < nn; r++)
            for (int i = 0; i < fn; i++) mem_fa[i] = mem_fa[i] + mem_fb[i];
        clock_gettime(CLOCK_MONOTONIC, &te);
        secs = elapsed_s(&ts, &te);
        if (secs >= 0.1) break;
        nn = recal(secs, nn);
    }
    return (double)fn * nn * 8.0 / (1000000.0 * secs);
}

/* === i32 add: ia[i] = ia[i] + ib[i] === */
static inline double mem_int_add(int in) {
    for (int i = 0; i < in; i++) { mem_ia[i] = 0; mem_ib[i] = 1; }
    int nn = 6400000 / in;
    if (nn < 1) nn = 1;
    double secs;
    for (;;) {
        for (int i = 0; i < in; i++) mem_ia[i] = 0;
        struct timespec ts, te;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        for (int r = 0; r < nn; r++)
            for (int i = 0; i < in; i++) mem_ia[i] = mem_ia[i] + mem_ib[i];
        clock_gettime(CLOCK_MONOTONIC, &te);
        secs = elapsed_s(&ts, &te);
        if (secs >= 0.1) break;
        nn = recal(secs, nn);
    }
    return (double)in * nn * 8.0 / (1000000.0 * secs);
}

/* === f64 copy: da[i] = db[i] === */
static inline double mem_dp_cpy(int dn) {
    for (int i = 0; i < dn; i++) { mem_da[i] = 0.0; mem_db[i] = (double)i; }
    int nn = 6400000 / dn;
    if (nn < 1) nn = 1;
    double secs;
    for (;;) {
        struct timespec ts, te;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        for (int r = 0; r < nn; r++) {
            for (int i = 0; i < dn; i++) mem_da[i] = mem_db[i];
            mem_db[r & 127] = mem_da[0];  /* anti-DCE */
        }
        clock_gettime(CLOCK_MONOTONIC, &te);
        secs = elapsed_s(&ts, &te);
        if (secs >= 0.1) break;
        nn = recal(secs, nn);
    }
    return (double)dn * nn * 16.0 / (1000000.0 * secs);
}

/* === f32 copy: fa[i] = fb[i] === */
static inline double mem_sp_cpy(int fn) {
    for (int i = 0; i < fn; i++) { mem_fa[i] = 0.0f; mem_fb[i] = (float)i; }
    int nn = 6400000 / fn;
    if (nn < 1) nn = 1;
    double secs;
    for (;;) {
        struct timespec ts, te;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        for (int r = 0; r < nn; r++) {
            for (int i = 0; i < fn; i++) mem_fa[i] = mem_fb[i];
            mem_fb[r & 127] = mem_fa[0];
        }
        clock_gettime(CLOCK_MONOTONIC, &te);
        secs = elapsed_s(&ts, &te);
        if (secs >= 0.1) break;
        nn = recal(secs, nn);
    }
    return (double)fn * nn * 8.0 / (1000000.0 * secs);
}

/* === i32 copy: ia[i] = ib[i] === */
static inline double mem_int_cpy(int in) {
    for (int i = 0; i < in; i++) { mem_ia[i] = 0; mem_ib[i] = i; }
    int nn = 6400000 / in;
    if (nn < 1) nn = 1;
    double secs;
    for (;;) {
        struct timespec ts, te;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        for (int r = 0; r < nn; r++) {
            for (int i = 0; i < in; i++) mem_ia[i] = mem_ib[i];
            mem_ib[r & 127] = mem_ia[0];
        }
        clock_gettime(CLOCK_MONOTONIC, &te);
        secs = elapsed_s(&ts, &te);
        if (secs >= 0.1) break;
        nn = recal(secs, nn);
    }
    return (double)in * nn * 8.0 / (1000000.0 * secs);
}

/* === memcpy (string.h) === */
static inline double mem_rt_cpy(int dn) {
    for (int i = 0; i < dn; i++) { mem_da[i] = 0.0; mem_db[i] = (double)i; }
    size_t bytes = (size_t)dn * 8;
    int nn = 6400000 / dn;
    if (nn < 1) nn = 1;
    double secs;
    for (;;) {
        struct timespec ts, te;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        for (int r = 0; r < nn; r++) {
            memcpy(mem_da, mem_db, bytes);
            __asm__ volatile("" :: "r"(mem_da) : "memory");
        }
        clock_gettime(CLOCK_MONOTONIC, &te);
        secs = elapsed_s(&ts, &te);
        if (secs >= 0.1) break;
        nn = recal(secs, nn);
    }
    return (double)dn * nn * 16.0 / (1000000.0 * secs);
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);

    printf("C Memory Speed Benchmark\n");
    printf("Adapted from Roy Longbottom's MemSpd2K\n");
    printf("Minimum 0.1 seconds per test\n\n");

    int sizes[] = {250, 500, 1000, 2000, 4000, 8000, 16000, 32000, 64000, 128000, 256000, 1024000, 2048000};

    printf("Memory  s=s+x[m]*y[m] Int+   x[m]=x[m]+y[m]       x[m]=y[m]        memcpy\n");
    printf("KBytes  Dble   Sngl   Int    Dble   Sngl   Int    Dble   Sngl   Int    f64\n");
    printf(" Used   MB/S   MB/S   MB/S   MB/S   MB/S   MB/S   MB/S   MB/S   MB/S  MB/S\n\n");

    for (int si = 0; si < 13; si++) {
        int n = sizes[si];
        int dn = n < MEM_MAXN ? n : MEM_MAXN;
        int fn = n * 2 < 4194304 ? n * 2 : 4194304;
        int in = fn;
        int kb = n * 16 / 1000;

        double r0 = mem_dp_dot(dn);
        double r1 = mem_sp_dot(fn);
        double r2 = mem_int_dot(in);
        double r3 = mem_dp_add(dn);
        double r4 = mem_sp_add(fn);
        double r5 = mem_int_add(in);
        double r6 = mem_dp_cpy(dn);
        double r7 = mem_sp_cpy(fn);
        double r8 = mem_int_cpy(in);
        double r9 = mem_rt_cpy(dn);

        printf("%5d %6.0f %6.0f %6.0f %6.0f %6.0f %6.0f %6.0f %6.0f %6.0f %6.0f\n",
               kb, r0, r1, r2, r3, r4, r5, r6, r7, r8, r9);
    }

    printf("\nDone.\n");
    return 0;
}
