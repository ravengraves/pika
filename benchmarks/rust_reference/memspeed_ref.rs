// memspeed_ref.rs - Memory Speed Benchmark for Rust
// Adapted from Roy Longbottom's MemSpd2K Benchmark
// Matches Pika memspeed.pka benchmark parameters.

use std::hint::black_box;
use std::time::Instant;

const MEMD_MAX: usize = 524288;
const MEMS_MAX: usize = 1048576;
const MIN_SECS: f64 = 0.1;

fn recal(elapsed: f64, n: i32) -> i32 {
    if elapsed < MIN_SECS / 8.0 {
        return n * 10;
    }
    (MIN_SECS * 1.25 / elapsed * n as f64 + 1.0) as i32
}

// === f64 dot product: dsum += x[m]*y[m] ===
fn mem_dp_dot(xd: &mut [f64], yd: &[f64], kd: usize) -> f64 {
    for m in 0..kd {
        xd[m] = 1.0;
        let _ = yd.len(); // ensure yd is used (init done by caller)
    }
    let mut nn = 6_400_000i32 / kd as i32;
    if nn < 1 { nn = 1; }
    let mut secs: f64;
    loop {
        let mut dsum: f64 = 0.0;
        let ts = Instant::now();
        for _ in 0..nn {
            for m in 0..kd {
                dsum += xd[m] * yd[m];
            }
        }
        secs = ts.elapsed().as_secs_f64();
        xd[0] = black_box(dsum) * 1.0e-20;
        if secs >= MIN_SECS { break; }
        nn = recal(secs, nn);
    }
    kd as f64 * nn as f64 * 16.0 / (1_000_000.0 * secs)
}

// === f64 add: x[m] = x[m] + y[m] ===
fn mem_dp_add(xd: &mut [f64], yd: &[f64], kd: usize) -> f64 {
    let mut nn = 6_400_000i32 / kd as i32;
    if nn < 1 { nn = 1; }
    let mut secs: f64;
    loop {
        for m in 0..kd { xd[m] = 0.0; }
        let ts = Instant::now();
        for _ in 0..nn {
            for m in 0..kd {
                xd[m] = xd[m] + yd[m];
            }
        }
        secs = ts.elapsed().as_secs_f64();
        black_box(xd[0]);
        if secs >= MIN_SECS { break; }
        nn = recal(secs, nn);
    }
    kd as f64 * nn as f64 * 16.0 / (1_000_000.0 * secs)
}

// === f64 copy: x[m] = y[m] ===
fn mem_dp_cpy(xd: &mut [f64], yd: &[f64], kd: usize) -> f64 {
    let mut nn = 6_400_000i32 / kd as i32;
    if nn < 1 { nn = 1; }
    let mut secs: f64;
    loop {
        let ts = Instant::now();
        for _ in 0..nn {
            for m in 0..kd {
                xd[m] = yd[m];
            }
        }
        secs = ts.elapsed().as_secs_f64();
        black_box(xd[0]);
        if secs >= MIN_SECS { break; }
        nn = recal(secs, nn);
    }
    kd as f64 * nn as f64 * 16.0 / (1_000_000.0 * secs)
}

// === f32 dot product: ssum += x[m]*y[m] ===
fn mem_sp_dot(xs: &mut [f32], ys: &[f32], ks: usize) -> f64 {
    let mut nn = 6_400_000i32 / ks as i32;
    if nn < 1 { nn = 1; }
    let mut secs: f64;
    loop {
        let mut ssum: f32 = 0.0;
        let ts = Instant::now();
        for _ in 0..nn {
            for m in 0..ks {
                ssum += xs[m] * ys[m];
            }
        }
        secs = ts.elapsed().as_secs_f64();
        xs[0] = black_box(ssum) * 1.0e-20;
        if secs >= MIN_SECS { break; }
        nn = recal(secs, nn);
    }
    ks as f64 * nn as f64 * 8.0 / (1_000_000.0 * secs)
}

// === f32 add: x[m] = x[m] + y[m] ===
fn mem_sp_add(xs: &mut [f32], ys: &[f32], ks: usize) -> f64 {
    let mut nn = 6_400_000i32 / ks as i32;
    if nn < 1 { nn = 1; }
    let mut secs: f64;
    loop {
        for m in 0..ks { xs[m] = 0.0; }
        let ts = Instant::now();
        for _ in 0..nn {
            for m in 0..ks {
                xs[m] = xs[m] + ys[m];
            }
        }
        secs = ts.elapsed().as_secs_f64();
        black_box(xs[0]);
        if secs >= MIN_SECS { break; }
        nn = recal(secs, nn);
    }
    ks as f64 * nn as f64 * 8.0 / (1_000_000.0 * secs)
}

// === f32 copy: x[m] = y[m] ===
fn mem_sp_cpy(xs: &mut [f32], ys: &[f32], ks: usize) -> f64 {
    let mut nn = 6_400_000i32 / ks as i32;
    if nn < 1 { nn = 1; }
    let mut secs: f64;
    loop {
        let ts = Instant::now();
        for _ in 0..nn {
            for m in 0..ks {
                xs[m] = ys[m];
            }
        }
        secs = ts.elapsed().as_secs_f64();
        black_box(xs[0]);
        if secs >= MIN_SECS { break; }
        nn = recal(secs, nn);
    }
    ks as f64 * nn as f64 * 8.0 / (1_000_000.0 * secs)
}

// === i32 accumulate: isum += x[m] + y[m] ===
fn mem_int_dot(xi: &mut [i32], yi: &[i32], ki: usize) -> f64 {
    let mut nn = 6_400_000i32 / ki as i32;
    if nn < 1 { nn = 1; }
    let mut secs: f64;
    loop {
        let mut isum: i32 = 0;
        let ts = Instant::now();
        for _ in 0..nn {
            for m in 0..ki {
                isum += xi[m] + yi[m];
            }
        }
        secs = ts.elapsed().as_secs_f64();
        xi[0] = black_box(isum);
        if secs >= MIN_SECS { break; }
        nn = recal(secs, nn);
    }
    ki as f64 * nn as f64 * 8.0 / (1_000_000.0 * secs)
}

// === i32 add: x[m] = x[m] + y[m] ===
fn mem_int_add(xi: &mut [i32], yi: &[i32], ki: usize) -> f64 {
    let mut nn = 6_400_000i32 / ki as i32;
    if nn < 1 { nn = 1; }
    let mut secs: f64;
    loop {
        for m in 0..ki { xi[m] = 0; }
        let ts = Instant::now();
        for _ in 0..nn {
            for m in 0..ki {
                xi[m] = xi[m] + yi[m];
            }
        }
        secs = ts.elapsed().as_secs_f64();
        black_box(xi[0]);
        if secs >= MIN_SECS { break; }
        nn = recal(secs, nn);
    }
    ki as f64 * nn as f64 * 8.0 / (1_000_000.0 * secs)
}

// === i32 copy: x[m] = y[m] ===
fn mem_int_cpy(xi: &mut [i32], yi: &[i32], ki: usize) -> f64 {
    let mut nn = 6_400_000i32 / ki as i32;
    if nn < 1 { nn = 1; }
    let mut secs: f64;
    loop {
        let ts = Instant::now();
        for _ in 0..nn {
            for m in 0..ki {
                xi[m] = yi[m];
            }
        }
        secs = ts.elapsed().as_secs_f64();
        black_box(xi[0]);
        if secs >= MIN_SECS { break; }
        nn = recal(secs, nn);
    }
    ki as f64 * nn as f64 * 8.0 / (1_000_000.0 * secs)
}

// === bulk copy via copy_from_slice ===
fn mem_rt_cpy(xd: &mut [f64], yd: &[f64], kd: usize) -> f64 {
    let mut nn = 6_400_000i32 / kd as i32;
    if nn < 1 { nn = 1; }
    let mut secs: f64;
    loop {
        let ts = Instant::now();
        for _ in 0..nn {
            xd[..kd].copy_from_slice(&yd[..kd]);
            black_box(&xd[..kd]);
        }
        secs = ts.elapsed().as_secs_f64();
        if secs >= MIN_SECS { break; }
        nn = recal(secs, nn);
    }
    kd as f64 * nn as f64 * 16.0 / (1_000_000.0 * secs)
}

fn write_mb(val: f64) {
    if val < 10.0 {
        print!("      ");
    } else if val < 100.0 {
        print!("     ");
    } else if val < 1000.0 {
        print!("    ");
    } else if val < 10000.0 {
        print!("   ");
    } else if val < 100000.0 {
        print!("  ");
    } else {
        print!(" ");
    }
    print!("{:.0}", val);
}

fn main() {
    println!("Rust Memory Speed Benchmark");
    println!("Adapted from Roy Longbottom's MemSpd2K");
    println!("Minimum {:.1} seconds per test\n", MIN_SECS);

    println!("Memory  s=s+x[m]*y[m] Int+   x[m]=x[m]+y[m]       x[m]=y[m]        memcpy");
    println!("KBytes  Dble   Sngl   Int    Dble   Sngl   Int    Dble   Sngl   Int    f64");
    println!(" Used   MB/S   MB/S   MB/S   MB/S   MB/S   MB/S   MB/S   MB/S   MB/S  MB/S\n");

    // Heap-allocate arrays — gives LLVM proper aliasing info via &mut/& slices
    let mut xd = vec![0.0f64; MEMD_MAX];
    let mut yd = vec![0.0f64; MEMD_MAX];
    let mut xs = vec![0.0f32; MEMS_MAX];
    let mut ys = vec![0.0f32; MEMS_MAX];
    let mut xi = vec![0i32; MEMS_MAX];
    let mut yi = vec![0i32; MEMS_MAX];

    let sizes = [250, 500, 1000, 2000, 4000, 8000, 16000, 32000, 64000, 128000, 256000];

    for &kd in &sizes {
        let ks = kd * 2;
        let ki = kd * 2;
        let mem_kb = kd * 16 / 1000;

        // Init arrays for each size
        for m in 0..kd { xd[m] = 1.0; yd[m] = 1.0; }
        for m in 0..ks { xs[m] = 1.0; ys[m] = 1.0; }
        for m in 0..ki { xi[m] = 0; yi[m] = 0; }
        yi[ki - 1] = 1;
        for m in 0..kd { yd[m] = 1.0; } // f64 dot/add/copy init

        let r0 = mem_dp_dot(&mut xd, &yd, kd);
        let r1 = mem_sp_dot(&mut xs, &ys, ks);
        let r2 = mem_int_dot(&mut xi, &yi, ki);

        for m in 0..kd { yd[m] = 1.0; } // re-init for add
        let r3 = mem_dp_add(&mut xd, &yd, kd);
        for m in 0..ks { ys[m] = 1.0; }
        let r4 = mem_sp_add(&mut xs, &ys, ks);
        for m in 0..ki { yi[m] = 1; }
        let r5 = mem_int_add(&mut xi, &yi, ki);

        for m in 0..kd { yd[m] = m as f64; }
        let r6 = mem_dp_cpy(&mut xd, &yd, kd);
        for m in 0..ks { ys[m] = m as f32; }
        let r7 = mem_sp_cpy(&mut xs, &ys, ks);
        for m in 0..ki { yi[m] = m as i32; }
        let r8 = mem_int_cpy(&mut xi, &yi, ki);

        for m in 0..kd { yd[m] = m as f64; }
        let r9 = mem_rt_cpy(&mut xd, &yd, kd);

        if mem_kb < 10 {
            print!("    ");
        } else if mem_kb < 100 {
            print!("   ");
        } else if mem_kb < 1000 {
            print!("  ");
        } else if mem_kb < 10000 {
            print!(" ");
        }
        print!("{}", mem_kb);

        write_mb(r0);
        write_mb(r1);
        write_mb(r2);
        write_mb(r3);
        write_mb(r4);
        write_mb(r5);
        write_mb(r6);
        write_mb(r7);
        write_mb(r8);
        write_mb(r9);
        println!();
    }

    println!();
    println!("Done.");
}
