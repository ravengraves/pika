#define _POSIX_C_SOURCE 199309L
/*
 *  LINPACK 100x100 Benchmark - C11 Reference (v2: register-blocked inner loop)
 *
 *  Original NETLIB source, translated to C by Bonnie Toy 5/88.
 *  PC timing version by Roy Longbottom, September 1996.
 *
 *  Optimization: the column-elimination j-loop processes 4 columns at once,
 *  loading the L column once and applying it to 4 destinations.
 *  Same algorithm as original, just better register/ILP utilization.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define N      100
#define LDA    201
#define LDAA   200
#define NTIMES 10

static double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

/* ---- BLAS (unit-stride, unrolled by 8) ---- */

static void daxpy(int n, double da, const double * restrict dx,
                  double * restrict dy) {
    if (n <= 0 || da == 0.0) return;
    int m = n & 7;
    for (int i = 0; i < m; i++)
        dy[i] += da * dx[i];
    for (int i = m; i < n; i += 8) {
        dy[i]   += da * dx[i];   dy[i+1] += da * dx[i+1];
        dy[i+2] += da * dx[i+2]; dy[i+3] += da * dx[i+3];
        dy[i+4] += da * dx[i+4]; dy[i+5] += da * dx[i+5];
        dy[i+6] += da * dx[i+6]; dy[i+7] += da * dx[i+7];
    }
}

/* 4-column daxpy: load src once, apply to 4 destinations */
static void daxpy4(int n, const double * restrict src,
                   double da0, double * restrict dst0,
                   double da1, double * restrict dst1,
                   double da2, double * restrict dst2,
                   double da3, double * restrict dst3) {
    for (int i = 0; i < n; i++) {
        double s = src[i];
        dst0[i] += da0 * s;
        dst1[i] += da1 * s;
        dst2[i] += da2 * s;
        dst3[i] += da3 * s;
    }
}

static void dscal(int n, double da, double *dx) {
    if (n <= 0) return;
    int m = n & 7;
    for (int i = 0; i < m; i++) dx[i] *= da;
    for (int i = m; i < n; i += 8) {
        dx[i]   *= da; dx[i+1] *= da; dx[i+2] *= da; dx[i+3] *= da;
        dx[i+4] *= da; dx[i+5] *= da; dx[i+6] *= da; dx[i+7] *= da;
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

/* ---- matgen ---- */

static void matgen(double *a, int lda, int n, double *b, double *norma) {
    int init = 1325;
    *norma = 0.0;
    for (int j = 0; j < n; j++)
        for (int i = 0; i < n; i++) {
            init = 3125 * init % 65536;
            a[lda*j+i] = (init - 32768.0) / 16384.0;
            if (a[lda*j+i] > *norma) *norma = a[lda*j+i];
        }
    for (int i = 0; i < n; i++) b[i] = 0.0;
    for (int j = 0; j < n; j++)
        for (int i = 0; i < n; i++)
            b[i] += a[lda*j+i];
}

/* ---- dgefa: LU with partial pivoting, register-blocked j-loop ---- */

static void dgefa(double *a, int lda, int n, int *ipvt, int *info) {
    *info = 0;
    for (int k = 0; k < n - 1; k++) {
        int l = idamax(n - k, &a[lda*k+k]) + k;
        ipvt[k] = l;
        if (a[lda*k+l] == 0.0) { *info = k; continue; }

        if (l != k) {
            double t = a[lda*k+l]; a[lda*k+l] = a[lda*k+k]; a[lda*k+k] = t;
        }

        double s = -1.0 / a[lda*k+k];
        dscal(n - k - 1, s, &a[lda*k+k+1]);

        int rem = n - (k + 1);
        int j = k + 1;
        const double *src = &a[lda*k+k+1];

        /* process 4 columns at a time */
        for (; j + 3 < n; j += 4) {
            double t0 = a[lda*(j+0)+l]; if (l != k) { a[lda*(j+0)+l] = a[lda*(j+0)+k]; a[lda*(j+0)+k] = t0; }
            double t1 = a[lda*(j+1)+l]; if (l != k) { a[lda*(j+1)+l] = a[lda*(j+1)+k]; a[lda*(j+1)+k] = t1; }
            double t2 = a[lda*(j+2)+l]; if (l != k) { a[lda*(j+2)+l] = a[lda*(j+2)+k]; a[lda*(j+2)+k] = t2; }
            double t3 = a[lda*(j+3)+l]; if (l != k) { a[lda*(j+3)+l] = a[lda*(j+3)+k]; a[lda*(j+3)+k] = t3; }

            daxpy4(rem, src,
                   t0, &a[lda*(j+0)+k+1], t1, &a[lda*(j+1)+k+1],
                   t2, &a[lda*(j+2)+k+1], t3, &a[lda*(j+3)+k+1]);
        }
        /* cleanup: remaining columns */
        for (; j < n; j++) {
            double t = a[lda*j+l];
            if (l != k) { a[lda*j+l] = a[lda*j+k]; a[lda*j+k] = t; }
            daxpy(rem, t, src, &a[lda*j+k+1]);
        }
    }
    ipvt[n-1] = n - 1;
    if (a[lda*(n-1)+(n-1)] == 0.0) *info = n - 1;
}

/* ---- dgesl: solve Ax=b ---- */

static void dgesl(double *a, int lda, int n, int *ipvt, double *b) {
    for (int k = 0; k < n - 1; k++) {
        int l = ipvt[k];
        double t = b[l];
        if (l != k) { b[l] = b[k]; b[k] = t; }
        daxpy(n - k - 1, t, &a[lda*k+k+1], &b[k+1]);
    }
    for (int k = n - 1; k >= 0; k--) {
        b[k] /= a[lda*k+k];
        daxpy(k, -b[k], &a[lda*k], b);
    }
}

/* ---- dmxpy (16-wide, residual check) ---- */

static void dmxpy(int n1, double *y, int n2, int ldm, const double *x,
                  const double *m) {
    int j, jmin;
    j = n2 % 2;
    if (j >= 1) { j -= 1; for (int i = 0; i < n1; i++) y[i] += x[j]*m[ldm*j+i]; }
    j = n2 % 4;
    if (j >= 2) { j -= 1; for (int i = 0; i < n1; i++)
        y[i] = (y[i] + x[j-1]*m[ldm*(j-1)+i]) + x[j]*m[ldm*j+i]; }
    j = n2 % 8;
    if (j >= 4) { j -= 1; for (int i = 0; i < n1; i++)
        y[i] = (((y[i] + x[j-3]*m[ldm*(j-3)+i]) + x[j-2]*m[ldm*(j-2)+i])
            + x[j-1]*m[ldm*(j-1)+i]) + x[j]*m[ldm*j+i]; }
    j = n2 % 16;
    if (j >= 8) { j -= 1; for (int i = 0; i < n1; i++)
        y[i] = (((((((y[i]
            + x[j-7]*m[ldm*(j-7)+i]) + x[j-6]*m[ldm*(j-6)+i])
            + x[j-5]*m[ldm*(j-5)+i]) + x[j-4]*m[ldm*(j-4)+i])
            + x[j-3]*m[ldm*(j-3)+i]) + x[j-2]*m[ldm*(j-2)+i])
            + x[j-1]*m[ldm*(j-1)+i]) + x[j]*m[ldm*j+i]; }
    jmin = (n2 % 16) + 16;
    for (j = jmin - 1; j < n2; j += 16) {
        for (int i = 0; i < n1; i++)
            y[i] = (((((((((((((((y[i]
                + x[j-15]*m[ldm*(j-15)+i]) + x[j-14]*m[ldm*(j-14)+i])
                + x[j-13]*m[ldm*(j-13)+i]) + x[j-12]*m[ldm*(j-12)+i])
                + x[j-11]*m[ldm*(j-11)+i]) + x[j-10]*m[ldm*(j-10)+i])
                + x[j- 9]*m[ldm*(j- 9)+i]) + x[j- 8]*m[ldm*(j- 8)+i])
                + x[j- 7]*m[ldm*(j- 7)+i]) + x[j- 6]*m[ldm*(j- 6)+i])
                + x[j- 5]*m[ldm*(j- 5)+i]) + x[j- 4]*m[ldm*(j- 4)+i])
                + x[j- 3]*m[ldm*(j- 3)+i]) + x[j- 2]*m[ldm*(j- 2)+i])
                + x[j- 1]*m[ldm*(j- 1)+i]) + x[j]*m[ldm*j+i];
    }
}

static double epslon(double x) {
    return 2.2204460492503131e-16 * fabs(x);
}

/* ---- main ---- */

static double aa[200*200], a[200*201], b_vec[200], x_vec[200];
static int ipvt[200];
static double check_sum = 0.0;

static void print_row(double dgefa_t, double dgesl_t, double total, double ops) {
    double mflops = (total > 0.0) ? ops / (1.0e6 * total) : 0.0;
    printf("  %9.5f  %9.5f  %9.5f  %10.2f\n", dgefa_t, dgesl_t, total, mflops);
}

int main(void) {
    int n = N, info;
    double norma, normx, resid, residn, eps;
    double ops = (2.0 * (double)n * n * n) / 3.0 + 2.0 * (double)n * n;

    printf("Register-blocked (4-wide) Double Precision Linpack (n=%d)\n", n);
    printf("Compiler     GCC/Clang C11 Linux\n");
    printf("Optimisation -O3 -ffast-math -march=native\n\n");

    /* verification */
    matgen(a, LDA, n, b_vec, &norma);
    double t1 = now_sec();
    dgefa(a, LDA, n, ipvt, &info);
    double td0 = now_sec() - t1;
    t1 = now_sec();
    dgesl(a, LDA, n, ipvt, b_vec);
    double ts0 = now_sec() - t1;

    for (int i = 0; i < n; i++) x_vec[i] = b_vec[i];
    matgen(a, LDA, n, b_vec, &norma);
    for (int i = 0; i < n; i++) b_vec[i] = -b_vec[i];
    dmxpy(n, b_vec, n, LDA, x_vec, a);

    resid = 0.0; normx = 0.0;
    for (int i = 0; i < n; i++) {
        if (fabs(b_vec[i]) > resid) resid = fabs(b_vec[i]);
        if (fabs(x_vec[i]) > normx) normx = fabs(x_vec[i]);
    }
    eps = epslon(1.0);
    residn = resid / ((double)n * norma * normx * eps);

    printf("norm resid      resid           machep         x[0]-1          x[n-1]-1\n");
    printf("%6.1f %17.8e%17.8e%17.8e%17.8e\n\n",
           residn, resid, eps, x_vec[0] - 1.0, x_vec[n-1] - 1.0);
    printf("1 pass (lda=%d):\n", LDA);
    printf("      dgefa      dgesl      total      Mflops\n");
    print_row(td0, ts0, td0 + ts0, ops);

    /* calibrate */
    int loop = NTIMES;
    double overhead1;
    for (int pass = -20; pass < 0; ) {
        double t = now_sec();
        for (int i = 0; i < loop; i++) matgen(a, LDA, n, b_vec, &norma);
        overhead1 = now_sec() - t;
        pass++;
        if (overhead1 > 5.0) break;
        if (overhead1 < 0.1) loop *= 10; else loop *= 2;
    }
    overhead1 /= (double)loop;

    int ntimes = NTIMES;
    {
        double elapsed = 0;
        for (int pass = -20; pass < 0; ) {
            double t = now_sec();
            for (int i = 0; i < ntimes; i++) {
                matgen(a, LDA, n, b_vec, &norma);
                dgefa(a, LDA, n, ipvt, &info);
            }
            elapsed = now_sec() - t;
            pass++;
            if (elapsed > 5.0) break;
            if (elapsed < 0.1) ntimes *= 10; else ntimes *= 2;
        }
        ntimes = (int)(5.0 * ntimes / elapsed);
        if (ntimes < 1) ntimes = 1;
    }
    printf("Passes: %d\n", ntimes);

    /* lda=201 */
    printf("\nResults for lda=%d (%d passes):\n", LDA, ntimes);
    printf("      dgefa      dgesl      total      Mflops\n");
    double tm2 = ntimes * overhead1;
    double avg1 = 0.0;
    for (int j = 0; j < 5; j++) {
        t1 = now_sec();
        for (int i = 0; i < ntimes; i++) {
            matgen(a, LDA, n, b_vec, &norma);
            dgefa(a, LDA, n, ipvt, &info);
        }
        double td = (now_sec() - t1 - tm2) / ntimes;
        t1 = now_sec();
        for (int i = 0; i < ntimes; i++) { dgesl(a, LDA, n, ipvt, b_vec); check_sum += b_vec[0]; }
        double ts = (now_sec() - t1) / ntimes;
        double tot = td + ts;
        avg1 += ops / (1.0e6 * tot);
        print_row(td, ts, tot, ops);
    }
    avg1 /= 5.0;
    printf("  Average                            %10.2f Mflops\n", avg1);

    /* lda=200 */
    double overhead2;
    { double t = now_sec();
      for (int i = 0; i < loop; i++) matgen(aa, LDAA, n, b_vec, &norma);
      overhead2 = (now_sec() - t) / (double)loop; }
    printf("\nResults for lda=%d (%d passes):\n", LDAA, ntimes);
    printf("      dgefa      dgesl      total      Mflops\n");
    tm2 = ntimes * overhead2;
    double avg2 = 0.0;
    for (int j = 0; j < 5; j++) {
        t1 = now_sec();
        for (int i = 0; i < ntimes; i++) {
            matgen(aa, LDAA, n, b_vec, &norma);
            dgefa(aa, LDAA, n, ipvt, &info);
        }
        double td = (now_sec() - t1 - tm2) / ntimes;
        t1 = now_sec();
        for (int i = 0; i < ntimes; i++) { dgesl(aa, LDAA, n, ipvt, b_vec); check_sum += b_vec[0]; }
        double ts = (now_sec() - t1) / ntimes;
        double tot = td + ts;
        avg2 += ops / (1.0e6 * tot);
        print_row(td, ts, tot, ops);
    }
    avg2 /= 5.0;
    printf("  Average                            %10.2f Mflops\n", avg2);

    double mflops = (avg1 < avg2) ? avg1 : avg2;
    printf("\n=== Register-blocked (4-wide) DP Linpack  %10.2f Mflops ===\n", mflops);
    printf("Checksum %e\n", check_sum);
    return 0;
}
