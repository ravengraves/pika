#define _POSIX_C_SOURCE 199309L
/*
 *  LINPACK 100x100 Benchmark - C11 Reference
 *
 *  Original NETLIB source, translated to C by Bonnie Toy 5/88.
 *  PC timing version by Roy Longbottom, September 1996.
 *  Adapted for Linux/C11: removed DOS/Windows deps, uses CLOCK_MONOTONIC.
 *  Core algorithm and unrolled BLAS preserved from Longbottom's version.
 *  Unrolling widened to 8 for AVX-512.
 *
 *  Double precision, unrolled BLAS, n=100, lda=201/200.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define N     100
#define LDA   201
#define LDAA  200
#define NTIMES 10

/* ---- timing ---- */

static double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

/* ---- BLAS (unit-stride, unrolled by 8) ---- */

static void daxpy(int n, double da, const double *dx, double *dy) {
    if (n <= 0 || da == 0.0) return;
    int m = n & 7;
    for (int i = 0; i < m; i++)
        dy[i] += da * dx[i];
    for (int i = m; i < n; i += 8) {
        dy[i]   += da * dx[i];
        dy[i+1] += da * dx[i+1];
        dy[i+2] += da * dx[i+2];
        dy[i+3] += da * dx[i+3];
        dy[i+4] += da * dx[i+4];
        dy[i+5] += da * dx[i+5];
        dy[i+6] += da * dx[i+6];
        dy[i+7] += da * dx[i+7];
    }
}

static void dscal(int n, double da, double *dx) {
    if (n <= 0) return;
    int m = n & 7;
    for (int i = 0; i < m; i++)
        dx[i] *= da;
    for (int i = m; i < n; i += 8) {
        dx[i]   *= da;
        dx[i+1] *= da;
        dx[i+2] *= da;
        dx[i+3] *= da;
        dx[i+4] *= da;
        dx[i+5] *= da;
        dx[i+6] *= da;
        dx[i+7] *= da;
    }
}

static int idamax(int n, const double *dx) {
    if (n < 1) return -1;
    if (n == 1) return 0;
    int imax = 0;
    double dmax = fabs(dx[0]);
    for (int i = 1; i < n; i++) {
        double v = fabs(dx[i]);
        if (v > dmax) { imax = i; dmax = v; }
    }
    return imax;
}

/* ---- matrix generator ---- */

static void matgen(double *a, int lda, int n, double *b, double *norma) {
    int init = 1325;
    *norma = 0.0;
    for (int j = 0; j < n; j++) {
        for (int i = 0; i < n; i++) {
            init = 3125 * init % 65536;
            a[lda*j+i] = (init - 32768.0) / 16384.0;
            if (a[lda*j+i] > *norma) *norma = a[lda*j+i];
        }
    }
    for (int i = 0; i < n; i++) b[i] = 0.0;
    for (int j = 0; j < n; j++)
        for (int i = 0; i < n; i++)
            b[i] += a[lda*j+i];
}

/* ---- dgefa: LU factorization with partial pivoting ---- */

static void dgefa(double *a, int lda, int n, int *ipvt, int *info) {
    *info = 0;
    for (int k = 0; k < n - 1; k++) {
        int l = idamax(n - k, &a[lda*k+k]) + k;
        ipvt[k] = l;

        if (a[lda*k+l] == 0.0) { *info = k; continue; }

        if (l != k) {
            double t = a[lda*k+l];
            a[lda*k+l] = a[lda*k+k];
            a[lda*k+k] = t;
        }

        double t = -1.0 / a[lda*k+k];
        dscal(n - k - 1, t, &a[lda*k+k+1]);

        for (int j = k + 1; j < n; j++) {
            t = a[lda*j+l];
            if (l != k) {
                a[lda*j+l] = a[lda*j+k];
                a[lda*j+k] = t;
            }
            daxpy(n - k - 1, t, &a[lda*k+k+1], &a[lda*j+k+1]);
        }
    }
    ipvt[n-1] = n - 1;
    if (a[lda*(n-1)+(n-1)] == 0.0) *info = n - 1;
}

/* ---- dgesl: solve Ax=b using LU factors ---- */

static void dgesl(double *a, int lda, int n, int *ipvt, double *b) {
    /* forward elimination (solve L*y = b) */
    for (int k = 0; k < n - 1; k++) {
        int l = ipvt[k];
        double t = b[l];
        if (l != k) { b[l] = b[k]; b[k] = t; }
        daxpy(n - k - 1, t, &a[lda*k+k+1], &b[k+1]);
    }
    /* back substitution (solve U*x = y) */
    for (int k = n - 1; k >= 0; k--) {
        b[k] /= a[lda*k+k];
        daxpy(k, -b[k], &a[lda*k], b);
    }
}

/* ---- dmxpy: y += m * x  (unrolled by 16, from Longbottom) ---- */

static void dmxpy(int n1, double *y, int n2, int ldm, const double *x,
                  const double *m) {
    int j, jmin;

    /* cleanup: 1 vector */
    j = n2 % 2;
    if (j >= 1) {
        j -= 1;
        for (int i = 0; i < n1; i++)
            y[i] += x[j] * m[ldm*j+i];
    }
    /* cleanup: 2 vectors */
    j = n2 % 4;
    if (j >= 2) {
        j -= 1;
        for (int i = 0; i < n1; i++)
            y[i] = (y[i] + x[j-1]*m[ldm*(j-1)+i]) + x[j]*m[ldm*j+i];
    }
    /* cleanup: 4 vectors */
    j = n2 % 8;
    if (j >= 4) {
        j -= 1;
        for (int i = 0; i < n1; i++)
            y[i] = (((y[i]
                + x[j-3]*m[ldm*(j-3)+i])
                + x[j-2]*m[ldm*(j-2)+i])
                + x[j-1]*m[ldm*(j-1)+i]) + x[j]*m[ldm*j+i];
    }
    /* cleanup: 8 vectors */
    j = n2 % 16;
    if (j >= 8) {
        j -= 1;
        for (int i = 0; i < n1; i++)
            y[i] = (((((((y[i]
                + x[j-7]*m[ldm*(j-7)+i]) + x[j-6]*m[ldm*(j-6)+i])
                + x[j-5]*m[ldm*(j-5)+i]) + x[j-4]*m[ldm*(j-4)+i])
                + x[j-3]*m[ldm*(j-3)+i]) + x[j-2]*m[ldm*(j-2)+i])
                + x[j-1]*m[ldm*(j-1)+i]) + x[j]  *m[ldm*j+i];
    }
    /* main loop: 16 vectors */
    jmin = (n2 % 16) + 16;
    for (j = jmin - 1; j < n2; j += 16) {
        for (int i = 0; i < n1; i++)
            y[i] = (((((((((((((((y[i]
                + x[j-15]*m[ldm*(j-15)+i])
                + x[j-14]*m[ldm*(j-14)+i])
                + x[j-13]*m[ldm*(j-13)+i])
                + x[j-12]*m[ldm*(j-12)+i])
                + x[j-11]*m[ldm*(j-11)+i])
                + x[j-10]*m[ldm*(j-10)+i])
                + x[j- 9]*m[ldm*(j- 9)+i])
                + x[j- 8]*m[ldm*(j- 8)+i])
                + x[j- 7]*m[ldm*(j- 7)+i])
                + x[j- 6]*m[ldm*(j- 6)+i])
                + x[j- 5]*m[ldm*(j- 5)+i])
                + x[j- 4]*m[ldm*(j- 4)+i])
                + x[j- 3]*m[ldm*(j- 3)+i])
                + x[j- 2]*m[ldm*(j- 2)+i])
                + x[j- 1]*m[ldm*(j- 1)+i])
                + x[j]   *m[ldm*j+i];
    }
}

/* ---- machine epsilon ---- */

static double epslon(double x) {
    /* Hardcoded: iterative method hangs under -ffast-math */
    return 2.2204460492503131e-16 * fabs(x);
}

/* ---- main ---- */

static double aa[200*200], a[200*201], b[200], x_vec[200];
static int ipvt[200];
static double check_sum = 0.0;  /* anti-DCE: accumulate dgesl results */

static void print_row(double dgefa_t, double dgesl_t, double total, double ops) {
    double mflops = (total > 0.0) ? ops / (1.0e6 * total) : 0.0;
    printf("  %9.5f  %9.5f  %9.5f  %10.2f\n", dgefa_t, dgesl_t, total, mflops);
}

int main(void) {
    int n = N, info;
    double norma, normx, resid, residn, eps;
    double ops = (2.0 * (double)n * n * n) / 3.0 + 2.0 * (double)n * n;
    printf("Unrolled Double Precision Linpack Benchmark (n=%d)\n", n);
    printf("Compiler     GCC/Clang C11 Linux\n");
    printf("Optimisation -O3 -ffast-math -march=native\n\n");

    /* ---- single-pass verification ---- */
    matgen(a, LDA, n, b, &norma);
    double t1 = now_sec();
    dgefa(a, LDA, n, ipvt, &info);
    double time_dgefa0 = now_sec() - t1;
    t1 = now_sec();
    dgesl(a, LDA, n, ipvt, b);
    double time_dgesl0 = now_sec() - t1;
    double total0 = time_dgefa0 + time_dgesl0;

    /* compute residual */
    for (int i = 0; i < n; i++) x_vec[i] = b[i];
    matgen(a, LDA, n, b, &norma);
    for (int i = 0; i < n; i++) b[i] = -b[i];
    dmxpy(n, b, n, LDA, x_vec, a);

    resid = 0.0; normx = 0.0;
    for (int i = 0; i < n; i++) {
        if (fabs(b[i]) > resid) resid = fabs(b[i]);
        if (fabs(x_vec[i]) > normx) normx = fabs(x_vec[i]);
    }
    eps = epslon(1.0);
    residn = resid / ((double)n * norma * normx * eps);

    printf("norm resid      resid           machep         x[0]-1          x[n-1]-1\n");
    printf("%6.1f %17.8e%17.8e%17.8e%17.8e\n\n",
           residn, resid, eps, x_vec[0] - 1.0, x_vec[n-1] - 1.0);

    printf("1 pass (lda=%d):\n", LDA);
    printf("      dgefa      dgesl      total      Mflops\n");
    print_row(time_dgefa0, time_dgesl0, total0, ops);

    /* ---- calibrate matgen overhead ---- */
    printf("\nCalibrating matgen overhead...\n");
    int loop = NTIMES;
    double overhead1;
    for (int pass = -20; pass < 0; ) {
        double t = now_sec();
        for (int i = 0; i < loop; i++)
            matgen(a, LDA, n, b, &norma);
        overhead1 = now_sec() - t;
        printf("  %8d times  %6.2f sec\n", loop, overhead1);
        pass++;
        if (overhead1 > 5.0) break;
        if (overhead1 < 0.1) loop *= 10;
        else                 loop *= 2;
    }
    overhead1 /= (double)loop;
    printf("  Overhead per matgen: %.6f sec\n", overhead1);

    /* ---- calibrate passes for ~5 seconds ---- */
    printf("\nCalibrating matgen+dgefa passes for 5 sec...\n");
    int ntimes = NTIMES;
    {
        double elapsed = 0;
        for (int pass = -20; pass < 0; ) {
            double t = now_sec();
            for (int i = 0; i < ntimes; i++) {
                matgen(a, LDA, n, b, &norma);
                dgefa(a, LDA, n, ipvt, &info);
            }
            elapsed = now_sec() - t;
            printf("  %8d times  %6.2f sec\n", ntimes, elapsed);
            pass++;
            if (elapsed > 5.0) break;
            if (elapsed < 0.1) ntimes *= 10;
            else               ntimes *= 2;
        }
        ntimes = (int)(5.0 * ntimes / elapsed);
        if (ntimes < 1) ntimes = 1;
    }
    printf("  Passes: %d\n", ntimes);

    /* ---- timed runs with lda=201 ---- */
    printf("\nResults for lda=%d (%d passes):\n", LDA, ntimes);
    printf("      dgefa      dgesl      total      Mflops\n");
    double tm2 = ntimes * overhead1;
    double avg1 = 0.0;
    for (int j = 0; j < 5; j++) {
        t1 = now_sec();
        for (int i = 0; i < ntimes; i++) {
            matgen(a, LDA, n, b, &norma);
            dgefa(a, LDA, n, ipvt, &info);
        }
        double td = (now_sec() - t1 - tm2) / ntimes;
        t1 = now_sec();
        for (int i = 0; i < ntimes; i++) {
            dgesl(a, LDA, n, ipvt, b);
            check_sum += b[0];
        }
        double ts = (now_sec() - t1) / ntimes;
        double tot = td + ts;
        double mf = ops / (1.0e6 * tot);
        avg1 += mf;
        print_row(td, ts, tot, ops);
    }
    avg1 /= 5.0;
    printf("  Average                            %10.2f Mflops\n", avg1);

    /* ---- compute overhead for lda=200 ---- */
    printf("\nCalibrating matgen2 overhead (lda=%d)...\n", LDAA);
    double overhead2;
    {
        double t = now_sec();
        for (int i = 0; i < loop; i++)
            matgen(aa, LDAA, n, b, &norma);
        overhead2 = (now_sec() - t) / (double)loop;
    }
    printf("  Overhead per matgen: %.6f sec\n", overhead2);

    /* ---- timed runs with lda=200 ---- */
    printf("\nResults for lda=%d (%d passes):\n", LDAA, ntimes);
    printf("      dgefa      dgesl      total      Mflops\n");
    tm2 = ntimes * overhead2;
    double avg2 = 0.0;
    for (int j = 0; j < 5; j++) {
        t1 = now_sec();
        for (int i = 0; i < ntimes; i++) {
            matgen(aa, LDAA, n, b, &norma);
            dgefa(aa, LDAA, n, ipvt, &info);
        }
        double td = (now_sec() - t1 - tm2) / ntimes;
        t1 = now_sec();
        for (int i = 0; i < ntimes; i++) {
            dgesl(aa, LDAA, n, ipvt, b);
            check_sum += b[0];
        }
        double ts = (now_sec() - t1) / ntimes;
        double tot = td + ts;
        double mf = ops / (1.0e6 * tot);
        avg2 += mf;
        print_row(td, ts, tot, ops);
    }
    avg2 /= 5.0;
    printf("  Average                            %10.2f Mflops\n", avg2);

    /* ---- final result ---- */
    double mflops = (avg1 < avg2) ? avg1 : avg2;
    printf("\n=== Unrolled Double Precision  %10.2f Mflops ===\n", mflops);
    printf("Checksum %e\n", check_sum);

    return 0;
}
