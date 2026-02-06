/*
 *     C/C++ Whetstone Benchmark Single Precision
 *
 *     Original concept        Brian Wichmann NPL      1960's
 *     Original author         Harold Curnow  CCTA     1972
 *     Self timing versions    Roy Longbottom CCTA     1978/87
 *     Optimisation control    Bangor University       1987/90
 *     C/C++ Version           Roy Longbottom          1996
 *     Linux adaptation                                2026
 *
 *     Adapted for Linux: removed conio.h, dos.h, getch(), gets(),
 *     _dos_getdate(). File output and interactive input removed.
 *     Core benchmark logic unchanged.
 */

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define SPDP float
#define Precision "Single"
#define compiler  "GCC/Clang Linux"
#define options   "-O3 -ffast-math"

void whetstones(long xtra, long x100, int calibrate);
void pa(SPDP e[4], SPDP t, SPDP t2);
void po(SPDP e1[4], long j, long k, long l);
void p3(SPDP *x, SPDP *y, SPDP *z, SPDP t, SPDP t1, SPDP t2);
void pout(char title[22], float ops, int type, SPDP checknum,
                SPDP time, int calibrate, int section);

static SPDP loop_time[9];
static SPDP loop_mops[9];
static SPDP loop_mflops[9];
static SPDP TimeUsed;
static SPDP mwips;
static char  headings[9][18];
static SPDP Check;
static SPDP results[9];

SPDP what_time()
{
    SPDP secs;
    clock_t Time;
    Time = clock();
    secs = (SPDP)(Time) / (SPDP)(CLOCKS_PER_SEC);
    return secs;
}

int main(int argc, char *argv[])
{
    int count = 10, calibrate = 1;
    long xtra=1;
    int section;
    long x100 = 100;
    int duration = 10;

    printf("\n %s Precision C/C++ Whetstone Benchmark\n\n", Precision);
    printf("Compiler        %s\n", compiler);
    printf("OptLevel        %s\n\n", options);

    printf("Calibrate\n");
    do
    {
        TimeUsed=0;
        whetstones(xtra,x100,calibrate);
        printf("%11.2f Seconds %10.0lf   Passes (x 100)\n",
                                 TimeUsed,(SPDP)(xtra));
        calibrate = calibrate + 1;
        count = count - 1;
        if (TimeUsed > 2.0)
           count = 0;
        else
           xtra = xtra * 5;
    }
    while (count > 0);

    if (TimeUsed > 0)
                  xtra = (long)((SPDP)(duration * xtra) / TimeUsed);
    if (xtra < 1) xtra = 1;

    calibrate = 0;

    printf("\nUse %ld  passes (x 100)\n", xtra);
    printf("\n          %s Precision C/C++ Whetstone Benchmark", Precision);
    printf("\n                  %s", compiler);
    printf("\n                  %s\n", options);
    printf("\nLoop content                  Result              MFLOPS "
                            "     MOPS   Seconds\n\n");

    TimeUsed=0;
    whetstones(xtra,x100,calibrate);

    printf("\nMWIPS            ");
    if (TimeUsed>0)
      mwips=(float)(xtra) * (float)(x100) / (10 * TimeUsed);
    else
      mwips = 0;

    printf("%39.3f%19.3f\n\n",mwips,TimeUsed);

    if (Check == 0)
       printf("Wrong answer  ");

    return 0;
}

void whetstones(long xtra, long x100, int calibrate)
{
    long n1,n2,n3,n4,n5,n6,n7,n8,i,ix,n1mult;
    SPDP x,y,z;
    long j,k,l;
    SPDP e1[4];
    SPDP timea;
    SPDP timeb;

    SPDP t =  0.49999975;
    SPDP t0 = t;
    SPDP t1 = 0.50000025;
    SPDP t2 = 2.0;

    Check=0.0;

    n1 = 12*x100;
    n2 = 14*x100;
    n3 = 345*x100;
    n4 = 210*x100;
    n5 = 32*x100;
    n6 = 899*x100;
    n7 = 616*x100;
    n8 = 93*x100;
    n1mult = 10;

    /* Section 1, Array elements */
    e1[0] = 1.0;
    e1[1] = -1.0;
    e1[2] = -1.0;
    e1[3] = -1.0;
    timea = what_time();
    {
        for (ix=0; ix<xtra; ix++)
        {
            for(i=0; i<n1*n1mult; i++)
            {
                e1[0] = (e1[0] + e1[1] + e1[2] - e1[3]) * t;
                e1[1] = (e1[0] + e1[1] - e1[2] + e1[3]) * t;
                e1[2] = (e1[0] - e1[1] + e1[2] + e1[3]) * t;
                e1[3] = (-e1[0] + e1[1] + e1[2] + e1[3]) * t;
            }
            t = 1.0 - t;
        }
        t =  t0;
    }
    timeb = (what_time()-timea)/(SPDP)(n1mult);
    pout("N1 floating point\0",(float)(n1*16)*(float)(xtra),
                         1,e1[3],timeb,calibrate,1);

    /* Section 2, Array as parameter */
    timea = what_time();
    {
        for (ix=0; ix<xtra; ix++)
        {
            for(i=0; i<n2; i++)
            {
               pa(e1,t,t2);
            }
            t = 1.0 - t;
        }
        t =  t0;
    }
    timeb = what_time()-timea;
    pout("N2 floating point\0",(float)(n2*96)*(float)(xtra),
                         1,e1[3],timeb,calibrate,2);

    /* Section 3, Conditional jumps */
    j = 1;
    timea = what_time();
    {
        for (ix=0; ix<xtra; ix++)
        {
            for(i=0; i<n3; i++)
            {
               if(j==1)       j = 2;
               else           j = 3;
               if(j>2)        j = 0;
               else           j = 1;
               if(j<1)        j = 1;
               else           j = 0;
            }
        }
    }
    timeb = what_time()-timea;
    pout("N3 if then else  \0",(float)(n3*3)*(float)(xtra),
                    2,(SPDP)(j),timeb,calibrate,3);

    /* Section 4, Integer arithmetic */
    j = 1;
    k = 2;
    l = 3;
    timea = what_time();
    {
        for (ix=0; ix<xtra; ix++)
        {
            for(i=0; i<n4; i++)
            {
               j = j *(k-j)*(l-k);
               k = l * k - (l-j) * k;
               l = (l-k) * (k+j);
               e1[l-2] = j + k + l;
               e1[k-2] = j * k * l;
            }
        }
    }
    timeb = what_time()-timea;
    x = e1[0]+e1[1];
    pout("N4 fixed point   \0",(float)(n4*15)*(float)(xtra),
                             2,x,timeb,calibrate,4);

    /* Section 5, Trig functions */
    x = 0.5;
    y = 0.5;
    timea = what_time();
    {
        for (ix=0; ix<xtra; ix++)
        {
            for(i=1; i<n5; i++)
            {
               x = t*atan(t2*sin(x)*cos(x)/(cos(x+y)+cos(x-y)-1.0));
               y = t*atan(t2*sin(y)*cos(y)/(cos(x+y)+cos(x-y)-1.0));
            }
            t = 1.0 - t;
        }
        t = t0;
    }
    timeb = what_time()-timea;
    pout("N5 sin,cos etc.  \0",(float)(n5*26)*(float)(xtra),
                             2,y,timeb,calibrate,5);

    /* Section 6, Procedure calls */
    x = 1.0;
    y = 1.0;
    z = 1.0;
    timea = what_time();
    {
        for (ix=0; ix<xtra; ix++)
        {
            for(i=0; i<n6; i++)
            {
               p3(&x,&y,&z,t,t1,t2);
            }
        }
    }
    timeb = what_time()-timea;
    pout("N6 floating point\0",(float)(n6*6)*(float)(xtra),
                            1,z,timeb,calibrate,6);

    /* Section 7, Array refrences */
    j = 0;
    k = 1;
    l = 2;
    e1[0] = 1.0;
    e1[1] = 2.0;
    e1[2] = 3.0;
    timea = what_time();
    {
        for (ix=0; ix<xtra; ix++)
        {
            for(i=0;i<n7;i++)
            {
               po(e1,j,k,l);
            }
        }
    }
    timeb = what_time()-timea;
    pout("N7 assignments   \0",(float)(n7*3)*(float)(xtra),
                        2,e1[2],timeb,calibrate,7);

    /* Section 8, Standard functions */
    x = 0.75;
    timea = what_time();
    {
        for (ix=0; ix<xtra; ix++)
        {
            for(i=0; i<n8; i++)
            {
               x = sqrt(exp(log(x)/t1));
            }
        }
    }
    timeb = what_time()-timea;
    pout("N8 exp,sqrt etc. \0",(float)(n8*4)*(float)(xtra),
                            2,x,timeb,calibrate,8);

    return;
}


void pa(SPDP e[4], SPDP t, SPDP t2)
{
    long j;
    for(j=0;j<6;j++)
    {
       e[0] = (e[0]+e[1]+e[2]-e[3])*t;
       e[1] = (e[0]+e[1]-e[2]+e[3])*t;
       e[2] = (e[0]-e[1]+e[2]+e[3])*t;
       e[3] = (-e[0]+e[1]+e[2]+e[3])/t2;
    }
    return;
}

void po(SPDP e1[4], long j, long k, long l)
{
    e1[j] = e1[k];
    e1[k] = e1[l];
    e1[l] = e1[j];
    return;
}

void p3(SPDP *x, SPDP *y, SPDP *z, SPDP t, SPDP t1, SPDP t2)
{
    *x = *y;
    *y = *z;
    *x = t * (*x + *y);
    *y = t1 * (*x + *y);
    *z = (*x + *y)/t2;
    return;
}

void pout(char title[18], float ops, int type, SPDP checknum,
          SPDP time, int calibrate, int section)
{
    SPDP mops,mflops;

    Check = Check + checknum;
    loop_time[section] = time;
    strcpy (headings[section],title);
    TimeUsed =  TimeUsed + time;
    if (calibrate == 1)
    {
        results[section] = checknum;
    }
    if (calibrate == 0)
    {
        printf("%s %24.17f    ",headings[section],results[section]);

        if (type == 1)
        {
            if (time>0)
            {
                mflops = ops/(1000000L*time);
            }
            else
            {
              mflops = 0;
            }
            loop_mops[section] = 99999;
            loop_mflops[section] = mflops;
            printf(" %9.3f          %9.3f\n",
             loop_mflops[section], loop_time[section]);
        }
        else
        {
            if (time>0)
            {
              mops = ops/(1000000L*time);
            }
            else
            {
              mops = 0;
            }
            loop_mops[section] = mops;
            loop_mflops[section] = 0;
            printf("           %9.3f%9.3f\n",
             loop_mops[section], loop_time[section]);
        }
    }

    return;
}
