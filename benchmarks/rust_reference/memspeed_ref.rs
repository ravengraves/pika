// memspeed_ref.rs - Memory Speed Benchmark for Rust
// Adapted from Roy Longbottom's MemSpd2K Benchmark
// Each test is a separate #[inline] function for fair optimization.
// Matches Pika memspeed.pka benchmark parameters.

use std::hint::black_box;
use std::time::Instant;

const MEMD_MAX: usize = 2097152;
const MEMS_MAX: usize = 4194304;
const MIN_SECS: f64 = 0.1;

static mut MEM_XD: [f64; MEMD_MAX] = [0.0; MEMD_MAX];
static mut MEM_YD: [f64; MEMD_MAX] = [0.0; MEMD_MAX];
static mut MEM_XS: [f32; MEMS_MAX] = [0.0; MEMS_MAX];
static mut MEM_YS: [f32; MEMS_MAX] = [0.0; MEMS_MAX];
static mut MEM_XI: [i32; MEMS_MAX] = [0; MEMS_MAX];
static mut MEM_YI: [i32; MEMS_MAX] = [0; MEMS_MAX];

fn recal(elapsed: f64, n: i32) -> i32 {
    if elapsed < MIN_SECS / 8.0 {
        return n * 10;
    }
    (MIN_SECS * 1.25 / elapsed * n as f64 + 1.0) as i32
}

// === f64 dot product: dsum += x[m]*y[m] ===
#[inline]
fn mem_dp_dot(kd: usize) -> f64 {
    unsafe {
        for m in 0..kd { MEM_XD[m] = 1.0; MEM_YD[m] = 1.0; }
        let mut nn = 6_400_000i32 / kd as i32;
        if nn < 1 { nn = 1; }
        let mut secs: f64;
        loop {
            let mut dsum: f64 = 0.0;
            let ts = Instant::now();
            for _ in 0..nn {
                for m in 0..kd {
                    dsum += MEM_XD[m] * MEM_YD[m];
                }
            }
            secs = ts.elapsed().as_secs_f64();
            MEM_XD[0] = black_box(dsum) * 1.0e-20;
            if secs >= MIN_SECS { break; }
            nn = recal(secs, nn);
        }
        kd as f64 * nn as f64 * 16.0 / (1_000_000.0 * secs)
    }
}

// === f64 add: x[m] = x[m] + y[m] ===
#[inline]
fn mem_dp_add(kd: usize) -> f64 {
    unsafe {
        for m in 0..kd { MEM_XD[m] = 0.0; MEM_YD[m] = 1.0; }
        let mut nn = 6_400_000i32 / kd as i32;
        if nn < 1 { nn = 1; }
        let mut secs: f64;
        loop {
            for m in 0..kd { MEM_XD[m] = 0.0; }
            let ts = Instant::now();
            for _ in 0..nn {
                for m in 0..kd {
                    MEM_XD[m] = MEM_XD[m] + MEM_YD[m];
                }
            }
            secs = ts.elapsed().as_secs_f64();
            black_box(MEM_XD[0]);
            if secs >= MIN_SECS { break; }
            nn = recal(secs, nn);
        }
        kd as f64 * nn as f64 * 16.0 / (1_000_000.0 * secs)
    }
}

// === f64 copy: x[m] = y[m] ===
#[inline]
fn mem_dp_cpy(kd: usize) -> f64 {
    unsafe {
        for m in 0..kd { MEM_XD[m] = 0.0; MEM_YD[m] = m as f64; }
        let mut nn = 6_400_000i32 / kd as i32;
        if nn < 1 { nn = 1; }
        let mut secs: f64;
        loop {
            let ts = Instant::now();
            for _ in 0..nn {
                for m in 0..kd {
                    MEM_XD[m] = MEM_YD[m];
                }
            }
            secs = ts.elapsed().as_secs_f64();
            black_box(MEM_XD[0]);
            if secs >= MIN_SECS { break; }
            nn = recal(secs, nn);
        }
        kd as f64 * nn as f64 * 16.0 / (1_000_000.0 * secs)
    }
}

// === f32 dot product: ssum += x[m]*y[m] ===
#[inline]
fn mem_sp_dot(ks: usize) -> f64 {
    unsafe {
        for m in 0..ks { MEM_XS[m] = 1.0; MEM_YS[m] = 1.0; }
        let mut nn = 6_400_000i32 / ks as i32;
        if nn < 1 { nn = 1; }
        let mut secs: f64;
        loop {
            let mut ssum: f32 = 0.0;
            let ts = Instant::now();
            for _ in 0..nn {
                for m in 0..ks {
                    ssum += MEM_XS[m] * MEM_YS[m];
                }
            }
            secs = ts.elapsed().as_secs_f64();
            MEM_XS[0] = black_box(ssum) * 1.0e-20;
            if secs >= MIN_SECS { break; }
            nn = recal(secs, nn);
        }
        ks as f64 * nn as f64 * 8.0 / (1_000_000.0 * secs)
    }
}

// === f32 add: x[m] = x[m] + y[m] ===
#[inline]
fn mem_sp_add(ks: usize) -> f64 {
    unsafe {
        for m in 0..ks { MEM_XS[m] = 0.0; MEM_YS[m] = 1.0; }
        let mut nn = 6_400_000i32 / ks as i32;
        if nn < 1 { nn = 1; }
        let mut secs: f64;
        loop {
            for m in 0..ks { MEM_XS[m] = 0.0; }
            let ts = Instant::now();
            for _ in 0..nn {
                for m in 0..ks {
                    MEM_XS[m] = MEM_XS[m] + MEM_YS[m];
                }
            }
            secs = ts.elapsed().as_secs_f64();
            black_box(MEM_XS[0]);
            if secs >= MIN_SECS { break; }
            nn = recal(secs, nn);
        }
        ks as f64 * nn as f64 * 8.0 / (1_000_000.0 * secs)
    }
}

// === f32 copy: x[m] = y[m] ===
#[inline]
fn mem_sp_cpy(ks: usize) -> f64 {
    unsafe {
        for m in 0..ks { MEM_XS[m] = 0.0; MEM_YS[m] = m as f32; }
        let mut nn = 6_400_000i32 / ks as i32;
        if nn < 1 { nn = 1; }
        let mut secs: f64;
        loop {
            let ts = Instant::now();
            for _ in 0..nn {
                for m in 0..ks {
                    MEM_XS[m] = MEM_YS[m];
                }
            }
            secs = ts.elapsed().as_secs_f64();
            black_box(MEM_XS[0]);
            if secs >= MIN_SECS { break; }
            nn = recal(secs, nn);
        }
        ks as f64 * nn as f64 * 8.0 / (1_000_000.0 * secs)
    }
}

// === i32 accumulate: isum += x[m] + y[m] ===
#[inline]
fn mem_int_dot(ki: usize) -> f64 {
    unsafe {
        for m in 0..ki { MEM_XI[m] = 0; MEM_YI[m] = 0; }
        MEM_YI[ki - 1] = 1;
        let mut nn = 6_400_000i32 / ki as i32;
        if nn < 1 { nn = 1; }
        let mut secs: f64;
        loop {
            let mut isum: i32 = 0;
            let ts = Instant::now();
            for _ in 0..nn {
                for m in 0..ki {
                    isum += MEM_XI[m] + MEM_YI[m];
                }
            }
            secs = ts.elapsed().as_secs_f64();
            MEM_XI[0] = black_box(isum);
            if secs >= MIN_SECS { break; }
            nn = recal(secs, nn);
        }
        ki as f64 * nn as f64 * 8.0 / (1_000_000.0 * secs)
    }
}

// === i32 add: x[m] = x[m] + y[m] ===
#[inline]
fn mem_int_add(ki: usize) -> f64 {
    unsafe {
        for m in 0..ki { MEM_XI[m] = 0; MEM_YI[m] = 1; }
        let mut nn = 6_400_000i32 / ki as i32;
        if nn < 1 { nn = 1; }
        let mut secs: f64;
        loop {
            for m in 0..ki { MEM_XI[m] = 0; }
            let ts = Instant::now();
            for _ in 0..nn {
                for m in 0..ki {
                    MEM_XI[m] = MEM_XI[m] + MEM_YI[m];
                }
            }
            secs = ts.elapsed().as_secs_f64();
            black_box(MEM_XI[0]);
            if secs >= MIN_SECS { break; }
            nn = recal(secs, nn);
        }
        ki as f64 * nn as f64 * 8.0 / (1_000_000.0 * secs)
    }
}

// === i32 copy: x[m] = y[m] ===
#[inline]
fn mem_int_cpy(ki: usize) -> f64 {
    unsafe {
        for m in 0..ki { MEM_XI[m] = 0; MEM_YI[m] = m as i32; }
        let mut nn = 6_400_000i32 / ki as i32;
        if nn < 1 { nn = 1; }
        let mut secs: f64;
        loop {
            let ts = Instant::now();
            for _ in 0..nn {
                for m in 0..ki {
                    MEM_XI[m] = MEM_YI[m];
                }
            }
            secs = ts.elapsed().as_secs_f64();
            black_box(MEM_XI[0]);
            if secs >= MIN_SECS { break; }
            nn = recal(secs, nn);
        }
        ki as f64 * nn as f64 * 8.0 / (1_000_000.0 * secs)
    }
}

// === bulk copy via copy_from_slice ===
#[inline]
fn mem_rt_cpy(kd: usize) -> f64 {
    unsafe {
        for m in 0..kd { MEM_XD[m] = 0.0; MEM_YD[m] = m as f64; }
        let mut nn = 6_400_000i32 / kd as i32;
        if nn < 1 { nn = 1; }
        let mut secs: f64;
        loop {
            let ts = Instant::now();
            for _ in 0..nn {
                let xd: &mut [f64] = &mut *(&raw mut MEM_XD);
                let yd: &[f64] = &*(&raw const MEM_YD);
                xd[..kd].copy_from_slice(&yd[..kd]);
                black_box(&xd[..kd]);
            }
            secs = ts.elapsed().as_secs_f64();
            if secs >= MIN_SECS { break; }
            nn = recal(secs, nn);
        }
        kd as f64 * nn as f64 * 16.0 / (1_000_000.0 * secs)
    }
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

    let sizes = [250, 500, 1000, 2000, 4000, 8000, 16000, 32000, 64000, 128000, 256000, 1024000, 2048000];

    for &kd in &sizes {
        let ks = kd * 2;
        let ki = kd * 2;
        let mem_kb = kd * 16 / 1000;

        let r0 = mem_dp_dot(kd);
        let r1 = mem_sp_dot(ks);
        let r2 = mem_int_dot(ki);
        let r3 = mem_dp_add(kd);
        let r4 = mem_sp_add(ks);
        let r5 = mem_int_add(ki);
        let r6 = mem_dp_cpy(kd);
        let r7 = mem_sp_cpy(ks);
        let r8 = mem_int_cpy(ki);
        let r9 = mem_rt_cpy(kd);

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
