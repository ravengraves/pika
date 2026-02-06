/*
 *  Livermore Loops (LLNL Kernels) - C Reference Version for Linux
 *
 *  Subset of the 24 LLNL computational kernels.
 *  Originally from Greg Astfalk, AT&T / Frank McMahon (LLNL).
 *  Matches the Pika livermore.pka benchmark parameters.
 *
 *  Adapted for Linux: standalone, no Windows dependencies.
 */

#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdint.h>

#define LIVER_N     1001
#define LIVER_NRUNS 100000

static double liver_x[LIVER_N];
static double liver_y[LIVER_N];
static double liver_z[LIVER_N];
static double liver_u[LIVER_N];
static double liver_v[LIVER_N];
static double liver_w[LIVER_N];

/* 25x25 matrix multiply */
static double liver_ma[625];
static double liver_mb[625];
static double liver_mc[625];

static uint64_t liver_seed = 12345;

static double liver_rand(void) {
    liver_seed = liver_seed * 6364136223846793005ULL + 1442695040888963407ULL;
    uint32_t bits = (uint32_t)(liver_seed >> 33);
    return (double)bits / 2147483648.0;
}

static void liver_init_arrays(void) {
    liver_seed = 12345;
    for (int i = 0; i < LIVER_N; i++) {
        liver_x[i] = liver_rand() * 2.0 - 1.0;
        liver_y[i] = liver_rand() * 2.0 - 1.0;
        liver_z[i] = liver_rand() * 2.0 - 1.0;
        liver_u[i] = liver_rand() * 2.0 - 1.0;
        liver_v[i] = liver_rand() * 2.0 - 1.0;
        liver_w[i] = liver_rand() * 2.0 - 1.0;
    }
    for (int i = 0; i < 625; i++) {
        liver_ma[i] = liver_rand();
        liver_mb[i] = liver_rand();
        liver_mc[i] = 0.0;
    }
}

static double elapsed_s(struct timespec *start, struct timespec *end) {
    return (end->tv_sec - start->tv_sec) + (end->tv_nsec - start->tv_nsec) / 1e9;
}

static void liver_report(const char *name, double flops_per_iter, int n_elem, int runs, double elapsed) {
    double total_ops = flops_per_iter * (double)n_elem * (double)runs;
    double mflops = total_ops / (elapsed * 1000000.0);
    printf("%s  %.2f MFLOPS  %.4f sec\n", name, mflops, elapsed);
}

/* Kernel 1: Hydro fragment */
static void liver_kernel1(void) {
    double q = 0.5, r = 0.3, t = 0.7;
    int nn = LIVER_N - 12;
    struct timespec ts, te;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    for (int run = 0; run < LIVER_NRUNS; run++) {
        for (int k = 0; k < nn; k++) {
            liver_x[k] = q + liver_y[k] * (r * liver_z[k+10] + t * liver_z[k+11]);
        }
        __asm__ volatile("" ::: "memory");
    }
    clock_gettime(CLOCK_MONOTONIC, &te);
    liver_report("K1  Hydro fragment    ", 5.0, nn, LIVER_NRUNS, elapsed_s(&ts, &te));
}

/* Kernel 3: Inner product */
static void liver_kernel3(void) {
    int nn = LIVER_N;
    struct timespec ts, te;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    for (int run = 0; run < LIVER_NRUNS; run++) {
        double q = 0.0;
        for (int k = 0; k < nn; k++) {
            q += liver_z[k] * liver_x[k];
        }
        liver_x[0] = liver_x[0] + q * 1.0e-20;
    }
    clock_gettime(CLOCK_MONOTONIC, &te);
    liver_report("K3  Inner product     ", 2.0, nn, LIVER_NRUNS, elapsed_s(&ts, &te));
}

/* Kernel 5: Tri-diagonal elimination */
static void liver_kernel5(void) {
    int nn = LIVER_N;
    struct timespec ts, te;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    for (int run = 0; run < LIVER_NRUNS; run++) {
        for (int i = 1; i < nn; i++) {
            liver_x[i] = liver_z[i] * (liver_y[i] - liver_x[i-1]);
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &te);
    liver_report("K5  Tri-diag elim     ", 3.0, nn - 1, LIVER_NRUNS, elapsed_s(&ts, &te));
}

/* Kernel 7: Equation of state fragment */
static void liver_kernel7(void) {
    double q = 0.5, r = 0.3, t = 0.7;
    int nn = LIVER_N - 7;
    struct timespec ts, te;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    for (int run = 0; run < LIVER_NRUNS; run++) {
        for (int k = 0; k < nn; k++) {
            liver_x[k] = liver_u[k] +
                r * (liver_z[k] + r * liver_y[k]) +
                t * (liver_u[k+3] +
                    r * (liver_u[k+2] + r * liver_u[k+1]) +
                    t * (liver_u[k+6] +
                        q * (liver_u[k+5] + q * liver_u[k+4])));
        }
        __asm__ volatile("" ::: "memory");
    }
    clock_gettime(CLOCK_MONOTONIC, &te);
    liver_report("K7  EOS fragment      ", 16.0, nn, LIVER_NRUNS, elapsed_s(&ts, &te));
}

/* Kernel 11: First sum (prefix sum) */
static void liver_kernel11(void) {
    int nn = LIVER_N;
    struct timespec ts, te;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    for (int run = 0; run < LIVER_NRUNS; run++) {
        liver_x[0] = liver_y[0];
        for (int k = 1; k < nn; k++) {
            liver_x[k] = liver_x[k-1] + liver_y[k];
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &te);
    liver_report("K11 First sum         ", 1.0, nn - 1, LIVER_NRUNS, elapsed_s(&ts, &te));
}

/* Kernel 12: First difference */
static void liver_kernel12(void) {
    int nn = LIVER_N - 1;
    struct timespec ts, te;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    for (int run = 0; run < LIVER_NRUNS; run++) {
        for (int k = 0; k < nn; k++) {
            liver_x[k] = liver_y[k+1] - liver_y[k];
        }
        __asm__ volatile("" ::: "memory");
    }
    clock_gettime(CLOCK_MONOTONIC, &te);
    liver_report("K12 First difference  ", 1.0, nn, LIVER_NRUNS, elapsed_s(&ts, &te));
}

/* Kernel 21: Matrix*matrix product (25x25) */
#define LIVER_SZ 25
static void liver_kernel21(void) {
    struct timespec ts, te;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    for (int run = 0; run < LIVER_NRUNS; run++) {
        for (int idx = 0; idx < 625; idx++) liver_mc[idx] = 0.0;
        for (int j = 0; j < LIVER_SZ; j++) {
            for (int k = 0; k < LIVER_SZ; k++) {
                for (int i = 0; i < LIVER_SZ; i++) {
                    liver_mc[j*LIVER_SZ+k] += liver_ma[j*LIVER_SZ+i] * liver_mb[i*LIVER_SZ+k];
                }
            }
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &te);
    double flops = 2.0 * 25.0 * 25.0 * 25.0;
    double el = elapsed_s(&ts, &te);
    double mflops = flops * (double)LIVER_NRUNS / (el * 1000000.0);
    printf("K21 Matrix*matrix       %.2f MFLOPS  %.4f sec\n", mflops, el);
}

/* Kernel 24: Find minimum */
static void liver_kernel24(void) {
    int nn = LIVER_N;
    struct timespec ts, te;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    double min_val = 0.0;
    for (int run = 0; run < LIVER_NRUNS; run++) {
        min_val = liver_x[0];
        for (int k = 1; k < nn; k++) {
            if (liver_x[k] < min_val) {
                min_val = liver_x[k];
            }
        }
    }
    liver_x[0] = liver_x[0] + min_val * 1.0e-20;
    clock_gettime(CLOCK_MONOTONIC, &te);
    liver_report("K24 Find minimum      ", 2.0, nn, LIVER_NRUNS, elapsed_s(&ts, &te));
}

int main(void) {
    printf("C Livermore Loops (LLNL Kernels)\n");
    printf("Array size: %d  Runs: %d\n\n", LIVER_N, LIVER_NRUNS);
    printf("Kernel                    MFLOPS     Seconds\n");
    printf("------------------------  ---------  --------\n");

    liver_init_arrays();

    liver_kernel1();
    liver_kernel3();
    liver_kernel5();
    liver_kernel7();
    liver_kernel11();
    liver_kernel12();
    liver_kernel21();
    liver_kernel24();

    printf("\nDone.\n");
    return 0;
}
