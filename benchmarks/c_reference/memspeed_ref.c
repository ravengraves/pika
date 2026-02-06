/*
 *  Memory Speed Benchmark - C Reference Version for Linux
 *
 *  Based on Roy Longbottom's MemSpd2K concept.
 *  Tests memory bandwidth: dot product, vector add, vector copy
 *  for f64, f32, and i32 data types across various working set sizes.
 *
 *  Matches the Pika memspeed.pka benchmark parameters.
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

#define MEM_MAXN 524288

static double  mem_da[MEM_MAXN];
static double  mem_db[MEM_MAXN];
static float   mem_fa[1048576];
static float   mem_fb[1048576];
static int32_t mem_ia[1048576];
static int32_t mem_ib[1048576];

static double elapsed_s(struct timespec *start, struct timespec *end) {
    return (end->tv_sec - start->tv_sec) + (end->tv_nsec - start->tv_nsec) / 1e9;
}

static void mem_init(void) {
    for (int i = 0; i < MEM_MAXN; i++) {
        mem_da[i] = 1.0 + (double)(i % 1000) * 0.001;
        mem_db[i] = 2.0 - (double)(i % 1000) * 0.001;
    }
    for (int i = 0; i < 1048576; i++) {
        mem_fa[i] = 1.0f + (float)(i % 1000) * 0.001f;
        mem_fb[i] = 2.0f - (float)(i % 1000) * 0.001f;
    }
    for (int i = 0; i < 1048576; i++) {
        mem_ia[i] = i;
        mem_ib[i] = 1048576 - i;
    }
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);

    printf("C Memory Speed Benchmark\n");
    printf("Adapted from Roy Longbottom's MemSpd2K\n");
    printf("Minimum 0.1 seconds per test\n\n");

    mem_init();

    int sizes[] = {250, 500, 1000, 2000, 4000, 8000, 16000, 32000, 64000, 128000, 256000};

    printf("Memory  s=s+x[m]*y[m] Int+   x[m]=x[m]+y[m]       x[m]=y[m]        memcpy\n");
    printf("KBytes  Dble   Sngl   Int    Dble   Sngl   Int    Dble   Sngl   Int    f64\n");
    printf(" Used   MB/S   MB/S   MB/S   MB/S   MB/S   MB/S   MB/S   MB/S   MB/S  MB/S\n\n");

    for (int si = 0; si < 11; si++) {
        int n = sizes[si];
        int dn = n < MEM_MAXN ? n : MEM_MAXN;
        int fn = n < 1048576 ? n : 1048576;
        int in = fn;
        int kb = n * 16 / 1000;  /* f64 working set in KB */
        double results[10];
        struct timespec ts, te;

        /* f64 dot */
        { int reps = 1024; double elapsed;
          do { reps *= 2; clock_gettime(CLOCK_MONOTONIC, &ts);
            double sum = 0.0;
            for (int r = 0; r < reps; r++) {
                double s = 0.0;
                for (int i = 0; i < dn; i++) s += mem_da[i] * mem_db[i];
                sum += s;
            }
            clock_gettime(CLOCK_MONOTONIC, &te); elapsed = elapsed_s(&ts, &te);
            mem_da[0] += sum * 1e-30;  /* anti-DCE */
          } while (elapsed < 0.1);
          results[0] = 16.0 * dn * reps / (elapsed * 1000000.0);
        }

        /* f32 dot */
        { int reps = 1024; double elapsed;
          do { reps *= 2; clock_gettime(CLOCK_MONOTONIC, &ts);
            float sum = 0.0f;
            for (int r = 0; r < reps; r++) {
                float s = 0.0f;
                for (int i = 0; i < fn; i++) s += mem_fa[i] * mem_fb[i];
                sum += s;
            }
            clock_gettime(CLOCK_MONOTONIC, &te); elapsed = elapsed_s(&ts, &te);
            mem_fa[0] += sum * 1e-30f;
          } while (elapsed < 0.1);
          results[1] = 8.0 * fn * reps / (elapsed * 1000000.0);
        }

        /* i32 dot */
        { int reps = 1024; double elapsed;
          do { reps *= 2; clock_gettime(CLOCK_MONOTONIC, &ts);
            int32_t sum = 0;
            for (int r = 0; r < reps; r++) {
                int32_t s = 0;
                for (int i = 0; i < in; i++) s += mem_ia[i] * mem_ib[i];
                sum += s;
            }
            clock_gettime(CLOCK_MONOTONIC, &te); elapsed = elapsed_s(&ts, &te);
            mem_ia[0] += (int32_t)(sum >> 30);
          } while (elapsed < 0.1);
          results[2] = 8.0 * in * reps / (elapsed * 1000000.0);
        }

        /* f64 add */
        { int reps = 1024; double elapsed;
          do { reps *= 2; clock_gettime(CLOCK_MONOTONIC, &ts);
            for (int r = 0; r < reps; r++)
                for (int i = 0; i < dn; i++) mem_da[i] = mem_da[i] + mem_db[i];
            clock_gettime(CLOCK_MONOTONIC, &te); elapsed = elapsed_s(&ts, &te);
          } while (elapsed < 0.1);
          results[3] = 16.0 * dn * reps / (elapsed * 1000000.0);
        }

        /* f32 add */
        { int reps = 1024; double elapsed;
          do { reps *= 2; clock_gettime(CLOCK_MONOTONIC, &ts);
            for (int r = 0; r < reps; r++)
                for (int i = 0; i < fn; i++) mem_fa[i] = mem_fa[i] + mem_fb[i];
            clock_gettime(CLOCK_MONOTONIC, &te); elapsed = elapsed_s(&ts, &te);
          } while (elapsed < 0.1);
          results[4] = 8.0 * fn * reps / (elapsed * 1000000.0);
        }

        /* i32 add */
        { int reps = 1024; double elapsed;
          do { reps *= 2; clock_gettime(CLOCK_MONOTONIC, &ts);
            for (int r = 0; r < reps; r++)
                for (int i = 0; i < in; i++) mem_ia[i] = mem_ia[i] + mem_ib[i];
            clock_gettime(CLOCK_MONOTONIC, &te); elapsed = elapsed_s(&ts, &te);
          } while (elapsed < 0.1);
          results[5] = 8.0 * in * reps / (elapsed * 1000000.0);
        }

        /* f64 copy */
        { int reps = 1024; double elapsed;
          do { reps *= 2; clock_gettime(CLOCK_MONOTONIC, &ts);
            for (int r = 0; r < reps; r++) {
                for (int i = 0; i < dn; i++) mem_da[i] = mem_db[i];
                mem_db[r & 127] = mem_da[0];  /* anti-DCE: varies per rep */
            }
            clock_gettime(CLOCK_MONOTONIC, &te); elapsed = elapsed_s(&ts, &te);
          } while (elapsed < 0.1);
          results[6] = 16.0 * dn * reps / (elapsed * 1000000.0);
        }

        /* f32 copy */
        { int reps = 1024; double elapsed;
          do { reps *= 2; clock_gettime(CLOCK_MONOTONIC, &ts);
            for (int r = 0; r < reps; r++) {
                for (int i = 0; i < fn; i++) mem_fa[i] = mem_fb[i];
                mem_fb[r & 127] = mem_fa[0];
            }
            clock_gettime(CLOCK_MONOTONIC, &te); elapsed = elapsed_s(&ts, &te);
          } while (elapsed < 0.1);
          results[7] = 8.0 * fn * reps / (elapsed * 1000000.0);
        }

        /* i32 copy */
        { int reps = 1024; double elapsed;
          do { reps *= 2; clock_gettime(CLOCK_MONOTONIC, &ts);
            for (int r = 0; r < reps; r++) {
                for (int i = 0; i < in; i++) mem_ia[i] = mem_ib[i];
                mem_ib[r & 127] = mem_ia[0];
            }
            clock_gettime(CLOCK_MONOTONIC, &te); elapsed = elapsed_s(&ts, &te);
          } while (elapsed < 0.1);
          results[8] = 8.0 * in * reps / (elapsed * 1000000.0);
        }

        /* memcpy (string.h) */
        { int reps = 1024; double elapsed;
          size_t bytes = (size_t)dn * 8;
          do { reps *= 2; clock_gettime(CLOCK_MONOTONIC, &ts);
            for (int r = 0; r < reps; r++) {
                memcpy(mem_da, mem_db, bytes);
                __asm__ volatile("" :: "r"(mem_da) : "memory");
            }
            clock_gettime(CLOCK_MONOTONIC, &te); elapsed = elapsed_s(&ts, &te);
          } while (elapsed < 0.1);
          results[9] = 16.0 * dn * reps / (elapsed * 1000000.0);
        }

        printf("%5d %6.0f %6.0f %6.0f %6.0f %6.0f %6.0f %6.0f %6.0f %6.0f %6.0f\n",
               kb, results[0], results[1], results[2],
               results[3], results[4], results[5],
               results[6], results[7], results[8], results[9]);
    }

    printf("\nDone.\n");
    return 0;
}
