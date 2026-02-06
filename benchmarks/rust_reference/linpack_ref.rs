// linpack_ref.rs - LINPACK 100x100 Benchmark for Rust
// Ported from Roy Longbottom's 1996 C/C++ version (Unrolled, Double Precision)
// Original source from NETLIB, translated to C by Bonnie Toy 5/88

use std::hint::black_box;
use std::time::Instant;

const N: usize = 100;
const LDA: usize = 201;
const LDAA: usize = 200;
const NTIMES: usize = 10;
const CRAY: f64 = 0.056;

fn second() -> f64 {
    use std::sync::OnceLock;
    static START: OnceLock<Instant> = OnceLock::new();
    START.get_or_init(Instant::now).elapsed().as_secs_f64()
}

fn matgen(a: &mut [f64], lda: usize, n: usize, b: &mut [f64]) -> f64 {
    let mut init: i32 = 1325;
    let mut norma: f64 = 0.0;
    for j in 0..n {
        let col = &mut a[lda * j..lda * j + n];
        for i in 0..n {
            init = (3125_i32).wrapping_mul(init) % 65536;
            let val = (init as f64 - 32768.0) / 16384.0;
            col[i] = val;
            if val > norma {
                norma = val;
            }
        }
    }
    b[..n].fill(0.0);
    for j in 0..n {
        let col = &a[lda * j..lda * j + n];
        for i in 0..n {
            b[i] += col[i];
        }
    }
    norma
}

#[inline(always)]
fn daxpy_sl(da: f64, dx: &[f64], dy: &mut [f64]) {
    if da == 0.0 {
        return;
    }
    for (y, x) in dy.iter_mut().zip(dx.iter()) {
        *y += da * *x;
    }
}

#[inline(always)]
fn ddot_sl(dx: &[f64], dy: &[f64]) -> f64 {
    dx.iter().zip(dy.iter()).map(|(a, b)| a * b).sum()
}

#[inline(always)]
fn dscal_sl(da: f64, dx: &mut [f64]) {
    for x in dx.iter_mut() {
        *x *= da;
    }
}

fn idamax(dx: &[f64]) -> usize {
    let mut itemp: usize = 0;
    let mut dmax = dx[0].abs();
    for (i, val) in dx.iter().enumerate().skip(1) {
        let v = val.abs();
        if v > dmax {
            itemp = i;
            dmax = v;
        }
    }
    itemp
}

fn dgefa(a: &mut [f64], lda: usize, n: usize, ipvt: &mut [i32]) -> i32 {
    let mut info: i32 = 0;
    let nm1 = n - 1;

    for k in 0..nm1 {
        let kp1 = k + 1;

        // find l = pivot index
        let l = idamax(&a[lda * k + k..lda * k + n]) + k;
        ipvt[k] = l as i32;

        if a[lda * k + l] != 0.0 {
            // interchange if necessary
            if l != k {
                a.swap(lda * k + l, lda * k + k);
            }

            // compute multipliers
            let t = -1.0 / a[lda * k + k];
            dscal_sl(t, &mut a[lda * k + k + 1..lda * k + n]);

            // row elimination with column indexing
            // Use raw pointers to avoid borrow conflict (source and dest columns don't overlap)
            let col_len = n - kp1;
            let src_base = lda * k + kp1;
            for j in kp1..n {
                let t = a[lda * j + l];
                if l != k {
                    a[lda * j + l] = a[lda * j + k];
                    a[lda * j + k] = t;
                }
                let dst_base = lda * j + kp1;
                // SAFETY: src [lda*k+kp1 .. lda*k+n] and dst [lda*j+kp1 .. lda*j+n]
                // don't overlap because j > k and lda >= n, so lda*j > lda*k + n.
                unsafe {
                    let src = a.as_ptr().add(src_base);
                    let dst = a.as_mut_ptr().add(dst_base);
                    for i in 0..col_len {
                        *dst.add(i) += t * *src.add(i);
                    }
                }
            }
        } else {
            info = k as i32;
        }
    }
    ipvt[n - 1] = (n - 1) as i32;
    if a[lda * (n - 1) + (n - 1)] == 0.0 {
        info = (n - 1) as i32;
    }
    info
}

fn dgesl(a: &[f64], lda: usize, n: usize, ipvt: &[i32], b: &mut [f64], job: i32) {
    let nm1 = n - 1;
    if job == 0 {
        // solve a * x = b; first solve l*y = b
        for k in 0..nm1 {
            let l = ipvt[k] as usize;
            let t = b[l];
            if l != k {
                b[l] = b[k];
                b[k] = t;
            }
            let kp1 = k + 1;
            daxpy_sl(t, &a[lda * k + kp1..lda * k + n], &mut b[kp1..n]);
        }

        // now solve u*x = y
        for kb in 0..n {
            let k = n - (kb + 1);
            b[k] /= a[lda * k + k];
            let t = -b[k];
            daxpy_sl(t, &a[lda * k..lda * k + k], &mut b[..k]);
        }
    } else {
        // solve trans(a) * x = b
        for k in 0..n {
            let t = ddot_sl(&a[lda * k..lda * k + k], &b[..k]);
            b[k] = (b[k] - t) / a[lda * k + k];
        }
        for kb in 1..nm1 {
            let k = n - (kb + 1);
            let kp1 = k + 1;
            b[k] += ddot_sl(&a[lda * k + kp1..lda * k + n], &b[kp1..n]);
            let l = ipvt[k] as usize;
            if l != k {
                b.swap(l, k);
            }
        }
    }
}

fn epslon(x: f64) -> f64 {
    let mut a = 4.0_f64 / 3.0;
    let mut eps = 0.0_f64;
    while eps == 0.0 {
        let b = a - 1.0;
        let c = b + b + b;
        eps = (c - 1.0).abs();
        a = black_box(a);
    }
    eps * x.abs()
}

fn dmxpy(n1: usize, y: &mut [f64], n2: usize, ldm: usize, x: &[f64], m: &[f64]) {
    // cleanup odd vector
    let mut j = n2 % 2;
    if j >= 1 {
        let jj = j - 1;
        let col = &m[ldm * jj..];
        let xj = x[jj];
        for i in 0..n1 {
            y[i] += xj * col[i];
        }
    }

    // cleanup odd group of two vectors
    j = n2 % 4;
    if j >= 2 {
        let jj = j - 1;
        let x0 = x[jj - 1];
        let x1 = x[jj];
        let c0 = &m[ldm * (jj - 1)..];
        let c1 = &m[ldm * jj..];
        for i in 0..n1 {
            y[i] = (y[i] + x0 * c0[i]) + x1 * c1[i];
        }
    }

    // cleanup odd group of four vectors
    j = n2 % 8;
    if j >= 4 {
        let jj = j - 1;
        let x0 = x[jj - 3];
        let x1 = x[jj - 2];
        let x2 = x[jj - 1];
        let x3 = x[jj];
        for i in 0..n1 {
            y[i] = (((y[i]
                + x0 * m[ldm * (jj - 3) + i])
                + x1 * m[ldm * (jj - 2) + i])
                + x2 * m[ldm * (jj - 1) + i])
                + x3 * m[ldm * jj + i];
        }
    }

    // cleanup odd group of eight vectors
    j = n2 % 16;
    if j >= 8 {
        let jj = j - 1;
        for i in 0..n1 {
            y[i] = (((((((y[i]
                + x[jj - 7] * m[ldm * (jj - 7) + i])
                + x[jj - 6] * m[ldm * (jj - 6) + i])
                + x[jj - 5] * m[ldm * (jj - 5) + i])
                + x[jj - 4] * m[ldm * (jj - 4) + i])
                + x[jj - 3] * m[ldm * (jj - 3) + i])
                + x[jj - 2] * m[ldm * (jj - 2) + i])
                + x[jj - 1] * m[ldm * (jj - 1) + i])
                + x[jj] * m[ldm * jj + i];
        }
    }

    // main loop - groups of sixteen vectors
    let jmin = (n2 % 16) + 16;
    let mut j = jmin - 1;
    while j < n2 {
        for i in 0..n1 {
            y[i] = (((((((((((((((y[i]
                + x[j - 15] * m[ldm * (j - 15) + i])
                + x[j - 14] * m[ldm * (j - 14) + i])
                + x[j - 13] * m[ldm * (j - 13) + i])
                + x[j - 12] * m[ldm * (j - 12) + i])
                + x[j - 11] * m[ldm * (j - 11) + i])
                + x[j - 10] * m[ldm * (j - 10) + i])
                + x[j - 9] * m[ldm * (j - 9) + i])
                + x[j - 8] * m[ldm * (j - 8) + i])
                + x[j - 7] * m[ldm * (j - 7) + i])
                + x[j - 6] * m[ldm * (j - 6) + i])
                + x[j - 5] * m[ldm * (j - 5) + i])
                + x[j - 4] * m[ldm * (j - 4) + i])
                + x[j - 3] * m[ldm * (j - 3) + i])
                + x[j - 2] * m[ldm * (j - 2) + i])
                + x[j - 1] * m[ldm * (j - 1) + i])
                + x[j] * m[ldm * j + i];
        }
        j += 16;
    }
}

fn print_time(atime: &[[f64; 15]; 9], row: usize) {
    eprintln!(
        "{:11.5}{:11.5}{:11.5}{:11.2}{:11.4}{:11.4}",
        atime[0][row], atime[1][row], atime[2][row],
        atime[3][row], atime[4][row], atime[5][row]
    );
}

fn main() {
    let mut aa = vec![0.0_f64; LDAA * LDAA];
    let mut a = vec![0.0_f64; LDA * (N + 1)];
    let mut b = vec![0.0_f64; N];
    let mut x = vec![0.0_f64; N];
    let mut ipvt = vec![0_i32; N];
    let mut atime = [[0.0_f64; 15]; 9];

    let n = N;
    let lda = LDA;
    let ldaa = LDAA;
    let ops = (2.0 * (n as f64).powi(3)) / 3.0 + 2.0 * (n as f64).powi(2);

    eprintln!("Unrolled Double Precision Linpack Benchmark - Rust Version\n");

    // --- Single pass ---
    matgen(&mut a, lda, n, &mut b);
    let t1 = second();
    let _info = dgefa(&mut a, lda, n, &mut ipvt);
    atime[0][0] = second() - t1;
    let t1 = second();
    dgesl(&a, lda, n, &ipvt, &mut b, 0);
    atime[1][0] = second() - t1;
    let total = atime[0][0] + atime[1][0];

    // Compute residual
    x[..n].copy_from_slice(&b[..n]);
    let norma = matgen(&mut a, lda, n, &mut b);
    for v in b[..n].iter_mut() {
        *v = -*v;
    }
    dmxpy(n, &mut b, n, lda, &x, &a);

    let mut resid: f64 = 0.0;
    let mut normx: f64 = 0.0;
    for i in 0..n {
        resid = resid.max(b[i].abs());
        normx = normx.max(x[i].abs());
    }
    let eps = epslon(1.0);
    let residn = resid / (n as f64 * norma * normx * eps);
    let x1 = x[0] - 1.0;
    let x2 = x[n - 1] - 1.0;

    println!("norm resid      resid           machep         x[0]-1          x[n-1]-1");
    println!(
        "{:6.1} {:17.8e}{:17.8e}{:17.8e}{:17.8e}\n",
        residn, resid, eps, x1, x2
    );

    eprintln!("Times are reported for matrices of order        {:5}", n);
    eprintln!("1 pass times for array with leading dimension of{:5}\n", lda);
    eprintln!("      dgefa      dgesl      total     Mflops       unit      ratio");

    atime[2][0] = total;
    if total > 0.0 {
        atime[3][0] = ops / (1.0e6 * total);
        atime[4][0] = 2.0 / atime[3][0];
    }
    atime[5][0] = total / CRAY;
    print_time(&atime, 0);

    // --- Calculate matgen overhead ---
    eprintln!("\nCalculating matgen overhead");
    let mut pass: i32 = -20;
    let mut loop_count = NTIMES;
    loop {
        let time1 = second();
        pass += 1;
        for _ in 0..loop_count {
            matgen(&mut a, lda, n, &mut b);
        }
        let overhead1 = second() - time1;
        eprintln!("{:10} times {:6.2} seconds", loop_count, overhead1);
        if overhead1 > 5.0 {
            pass = 0;
        }
        if pass < 0 {
            if overhead1 < 0.1 {
                loop_count *= 10;
            } else {
                loop_count *= 2;
            }
        }
        if pass >= 0 {
            break;
        }
    }
    let time1 = second();
    for _ in 0..loop_count {
        matgen(&mut a, lda, n, &mut b);
    }
    let overhead1 = (second() - time1) / loop_count as f64;
    eprintln!("Overhead for 1 matgen {:12.5} seconds\n", overhead1);

    // --- Calculate matgen/dgefa passes for 5 seconds ---
    eprintln!("Calculating matgen/dgefa passes for 5 seconds");
    pass = -20;
    let mut ntimes = NTIMES;
    loop {
        let time1 = second();
        pass += 1;
        for _ in 0..ntimes {
            matgen(&mut a, lda, n, &mut b);
            dgefa(&mut a, lda, n, &mut ipvt);
        }
        let time2 = second() - time1;
        eprintln!("{:10} times {:6.2} seconds", ntimes, time2);
        if time2 > 5.0 {
            pass = 0;
        }
        if pass < 0 {
            if time2 < 0.1 {
                ntimes *= 10;
            } else {
                ntimes *= 2;
            }
        }
        if pass >= 0 {
            ntimes = (5.0 * ntimes as f64 / time2) as usize;
            if ntimes == 0 {
                ntimes = 1;
            }
            break;
        }
    }

    eprintln!("Passes used {:10}\n", ntimes);
    eprintln!("Times for array with leading dimension of{:4}\n", lda);
    eprintln!("      dgefa      dgesl      total     Mflops       unit      ratio");

    // --- Execute 5 passes (lda=201) ---
    let tm2 = ntimes as f64 * overhead1;
    atime[3][6] = 0.0;

    for j in 1..6_usize {
        let t1 = second();
        for _ in 0..ntimes {
            matgen(&mut a, lda, n, &mut b);
            dgefa(&mut a, lda, n, &mut ipvt);
        }
        atime[0][j] = (second() - t1 - tm2) / ntimes as f64;

        let t1 = second();
        for _ in 0..ntimes {
            dgesl(&a, lda, n, &ipvt, &mut b, 0);
        }
        atime[1][j] = (second() - t1) / ntimes as f64;

        let total = atime[0][j] + atime[1][j];
        atime[2][j] = total;
        atime[3][j] = ops / (1.0e6 * total);
        atime[4][j] = 2.0 / atime[3][j];
        atime[5][j] = total / CRAY;
        atime[3][6] += atime[3][j];

        print_time(&atime, j);
    }
    atime[3][6] /= 5.0;
    eprintln!("Average                          {:11.2}", atime[3][6]);

    // --- Calculate matgen2 overhead (ldaa=200) ---
    eprintln!("\nCalculating matgen2 overhead");
    let time1 = second();
    for _ in 0..loop_count {
        matgen(&mut aa, ldaa, n, &mut b);
    }
    let overhead2 = (second() - time1) / loop_count as f64;
    eprintln!("Overhead for 1 matgen {:12.5} seconds\n", overhead2);
    eprintln!("Times for array with leading dimension of{:4}\n", ldaa);
    eprintln!("      dgefa      dgesl      total     Mflops       unit      ratio");

    // --- Execute 5 passes (ldaa=200) ---
    let tm2 = ntimes as f64 * overhead2;
    atime[3][12] = 0.0;

    for j in 7..12_usize {
        let t1 = second();
        for _ in 0..ntimes {
            matgen(&mut aa, ldaa, n, &mut b);
            dgefa(&mut aa, ldaa, n, &mut ipvt);
        }
        atime[0][j] = (second() - t1 - tm2) / ntimes as f64;

        let t1 = second();
        for _ in 0..ntimes {
            dgesl(&aa, ldaa, n, &ipvt, &mut b, 0);
        }
        atime[1][j] = (second() - t1) / ntimes as f64;

        let total = atime[0][j] + atime[1][j];
        atime[2][j] = total;
        atime[3][j] = ops / (1.0e6 * total);
        atime[4][j] = 2.0 / atime[3][j];
        atime[5][j] = total / CRAY;
        atime[3][12] += atime[3][j];

        print_time(&atime, j);
    }
    atime[3][12] /= 5.0;
    eprintln!("Average                          {:11.2}", atime[3][12]);

    // --- Final result ---
    let mflops = atime[3][6].min(atime[3][12]);
    eprintln!();
    eprintln!(
        "Unrolled Double  Precision {:11.2} Mflops\n",
        black_box(mflops)
    );
}
