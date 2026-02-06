// livermore_ref.rs - Livermore Loops (LLNL Kernels) for Rust
// Subset of the 24 LLNL computational kernels.
// Matches Pika livermore.pka benchmark parameters.

use std::hint::black_box;
use std::time::Instant;

const LIVER_N: usize = 1001;
const LIVER_NRUNS: usize = 100_000;

static mut LIVER_X: [f64; LIVER_N] = [0.0; LIVER_N];
static mut LIVER_Y: [f64; LIVER_N] = [0.0; LIVER_N];
static mut LIVER_Z: [f64; LIVER_N] = [0.0; LIVER_N];
static mut LIVER_U: [f64; LIVER_N] = [0.0; LIVER_N];
#[allow(dead_code)]
static mut LIVER_V: [f64; LIVER_N] = [0.0; LIVER_N];
#[allow(dead_code)]
static mut LIVER_W: [f64; LIVER_N] = [0.0; LIVER_N];

static mut LIVER_MA: [f64; 625] = [0.0; 625];
static mut LIVER_MB: [f64; 625] = [0.0; 625];
static mut LIVER_MC: [f64; 625] = [0.0; 625];

static mut LIVER_SEED: u64 = 12345;

unsafe fn liver_rand() -> f64 {
    LIVER_SEED = LIVER_SEED.wrapping_mul(6364136223846793005).wrapping_add(1442695040888963407);
    let bits = (LIVER_SEED >> 33) as u32;
    bits as f64 / 2147483648.0
}

unsafe fn liver_init_arrays() {
    LIVER_SEED = 12345;
    for i in 0..LIVER_N {
        LIVER_X[i] = liver_rand() * 2.0 - 1.0;
        LIVER_Y[i] = liver_rand() * 2.0 - 1.0;
        LIVER_Z[i] = liver_rand() * 2.0 - 1.0;
        LIVER_U[i] = liver_rand() * 2.0 - 1.0;
        LIVER_V[i] = liver_rand() * 2.0 - 1.0;
        LIVER_W[i] = liver_rand() * 2.0 - 1.0;
    }
    for i in 0..625 {
        LIVER_MA[i] = liver_rand();
        LIVER_MB[i] = liver_rand();
        LIVER_MC[i] = 0.0;
    }
}

fn liver_report(name: &str, flops_per_iter: f64, n_elem: usize, runs: usize, elapsed: f64) {
    let total_ops = flops_per_iter * n_elem as f64 * runs as f64;
    let mflops = total_ops / (elapsed * 1_000_000.0);
    println!("{}  {:.2} MFLOPS  {:.4} sec", name, mflops, elapsed);
}

// Kernel 1: Hydro fragment
unsafe fn liver_kernel1() {
    let q: f64 = 0.5;
    let r: f64 = 0.3;
    let t: f64 = 0.7;
    let nn = LIVER_N - 12;

    let ts = Instant::now();
    for _ in 0..LIVER_NRUNS {
        for k in 0..nn {
            LIVER_X[k] = q + LIVER_Y[k] * (r * LIVER_Z[k + 10] + t * LIVER_Z[k + 11]);
        }
        black_box(&LIVER_X);
    }
    let elapsed = ts.elapsed().as_secs_f64();
    liver_report("K1  Hydro fragment    ", 5.0, nn, LIVER_NRUNS, elapsed);
}

// Kernel 3: Inner product
unsafe fn liver_kernel3() {
    let nn = LIVER_N;

    let ts = Instant::now();
    for _ in 0..LIVER_NRUNS {
        let mut q: f64 = 0.0;
        for k in 0..nn {
            q += LIVER_Z[k] * LIVER_X[k];
        }
        LIVER_X[0] = LIVER_X[0] + q * 1.0e-20;
    }
    let elapsed = ts.elapsed().as_secs_f64();
    liver_report("K3  Inner product     ", 2.0, nn, LIVER_NRUNS, elapsed);
}

// Kernel 5: Tri-diagonal elimination
unsafe fn liver_kernel5() {
    let nn = LIVER_N;

    let ts = Instant::now();
    for _ in 0..LIVER_NRUNS {
        for i in 1..nn {
            LIVER_X[i] = LIVER_Z[i] * (LIVER_Y[i] - LIVER_X[i - 1]);
        }
    }
    let elapsed = ts.elapsed().as_secs_f64();
    liver_report("K5  Tri-diag elim     ", 3.0, nn - 1, LIVER_NRUNS, elapsed);
}

// Kernel 7: Equation of state fragment
unsafe fn liver_kernel7() {
    let q: f64 = 0.5;
    let r: f64 = 0.3;
    let t: f64 = 0.7;
    let nn = LIVER_N - 7;

    let ts = Instant::now();
    for _ in 0..LIVER_NRUNS {
        for k in 0..nn {
            LIVER_X[k] = LIVER_U[k]
                + r * (LIVER_Z[k] + r * LIVER_Y[k])
                + t * (LIVER_U[k + 3]
                    + r * (LIVER_U[k + 2] + r * LIVER_U[k + 1])
                    + t * (LIVER_U[k + 6]
                        + q * (LIVER_U[k + 5] + q * LIVER_U[k + 4])));
        }
        black_box(&LIVER_X);
    }
    let elapsed = ts.elapsed().as_secs_f64();
    liver_report("K7  EOS fragment      ", 16.0, nn, LIVER_NRUNS, elapsed);
}

// Kernel 11: First sum (prefix sum)
unsafe fn liver_kernel11() {
    let nn = LIVER_N;

    let ts = Instant::now();
    for _ in 0..LIVER_NRUNS {
        LIVER_X[0] = LIVER_Y[0];
        for k in 1..nn {
            LIVER_X[k] = LIVER_X[k - 1] + LIVER_Y[k];
        }
    }
    let elapsed = ts.elapsed().as_secs_f64();
    liver_report("K11 First sum         ", 1.0, nn - 1, LIVER_NRUNS, elapsed);
}

// Kernel 12: First difference
unsafe fn liver_kernel12() {
    let nn = LIVER_N - 1;

    let ts = Instant::now();
    for _ in 0..LIVER_NRUNS {
        for k in 0..nn {
            LIVER_X[k] = LIVER_Y[k + 1] - LIVER_Y[k];
        }
        black_box(&LIVER_X);
    }
    let elapsed = ts.elapsed().as_secs_f64();
    liver_report("K12 First difference  ", 1.0, nn, LIVER_NRUNS, elapsed);
}

// Kernel 21: Matrix*matrix product (25x25)
const LIVER_SZ: usize = 25;

unsafe fn liver_kernel21() {
    let ts = Instant::now();
    for _ in 0..LIVER_NRUNS {
        for idx in 0..625 {
            LIVER_MC[idx] = 0.0;
        }
        for j in 0..LIVER_SZ {
            for k in 0..LIVER_SZ {
                for i in 0..LIVER_SZ {
                    LIVER_MC[j * LIVER_SZ + k] +=
                        LIVER_MA[j * LIVER_SZ + i] * LIVER_MB[i * LIVER_SZ + k];
                }
            }
        }
    }
    let elapsed = ts.elapsed().as_secs_f64();
    let flops: f64 = 2.0 * 25.0 * 25.0 * 25.0;
    let mflops = flops * LIVER_NRUNS as f64 / (elapsed * 1_000_000.0);
    println!("K21 Matrix*matrix       {:.2} MFLOPS  {:.4} sec", mflops, elapsed);
}

// Kernel 24: Find minimum
unsafe fn liver_kernel24() {
    let nn = LIVER_N;

    let ts = Instant::now();
    let mut min_val: f64 = 0.0;
    for _ in 0..LIVER_NRUNS {
        min_val = LIVER_X[0];
        for k in 1..nn {
            if LIVER_X[k] < min_val {
                min_val = LIVER_X[k];
            }
        }
    }
    LIVER_X[0] = LIVER_X[0] + min_val * 1.0e-20;
    let elapsed = ts.elapsed().as_secs_f64();
    liver_report("K24 Find minimum      ", 2.0, nn, LIVER_NRUNS, elapsed);
}

fn main() {
    println!("Rust Livermore Loops (LLNL Kernels)");
    println!("Array size: {}  Runs: {}", LIVER_N, LIVER_NRUNS);
    println!();
    println!("Kernel                    MFLOPS     Seconds");
    println!("------------------------  ---------  --------");

    unsafe {
        liver_init_arrays();

        liver_kernel1();
        liver_kernel3();
        liver_kernel5();
        liver_kernel7();
        liver_kernel11();
        liver_kernel12();
        liver_kernel21();
        liver_kernel24();
    }

    println!();
    println!("Done.");
}
