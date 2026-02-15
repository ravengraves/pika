// whetstone_ref.rs - Whetstone Benchmark for Rust
// Adapted from Roy Longbottom's C/C++ Whetstone Benchmark
// Single precision version matching C output format

use std::hint::black_box;
use std::time::Instant;

fn whet_pa(e: &mut [f32; 4], t: f32, t2: f32) {
    for _ in 0..6 {
        e[0] = (e[0] + e[1] + e[2] - e[3]) * t;
        e[1] = (e[0] + e[1] - e[2] + e[3]) * t;
        e[2] = (e[0] - e[1] + e[2] + e[3]) * t;
        e[3] = (-e[0] + e[1] + e[2] + e[3]) / t2;
    }
}

fn whet_po(e: &mut [f32; 4], j: usize, k: usize, l: usize) {
    e[j] = e[k];
    e[k] = e[l];
    e[l] = e[j];
}

fn whet_p3(x: &mut f32, y: &mut f32, z: &mut f32, t: f32, t1: f32, t2: f32) {
    *x = *y;
    *y = *z;
    *x = t * (*x + *y);
    *y = t1 * (*x + *y);
    *z = (*x + *y) / t2;
}

static mut N3_SEEDS: [i64; 256] = [0; 256];

#[inline(never)]
fn init_n3_seeds() {
    unsafe {
        for i in 0..256 { N3_SEEDS[i] = (i & 1) as i64; }
        black_box(&raw const N3_SEEDS);
    }
}

fn print_line(title: &str, result: f64, mflops: f64, mops: f64, time: f64, is_mflops: bool) {
    if is_mflops {
        println!("{} {:>24.17}     {:>9.3}          {:>9.3}", title, result, mflops, time);
    } else {
        println!("{} {:>24.17}               {:>9.3}{:>9.3}", title, result, mops, time);
    }
}

fn main() {
    let x100: i64 = 100000;
    let xtra: i64 = 1;

    let mut t: f32 = 0.49999975;
    let t0: f32 = t;
    let t1: f32 = 0.50000025;
    let t2: f32 = 2.0;

    let n1: i64 = 12 * x100;
    let n2: i64 = 14 * x100;
    let n3: i64 = 345 * x100;
    let n4: i64 = 210 * x100;
    let n5: i64 = 32 * x100;
    let n6: i64 = 899 * x100;
    let n7: i64 = 616 * x100;
    let n8: i64 = 93 * x100;

    let mut e1: [f32; 4] = [0.0; 4];
    let mut total_time: f64 = 0.0;

    // Section results (buffered for end-of-benchmark printing)
    let mut sec_result: [f64; 9] = [0.0; 9];
    let mut sec_mflops: [f64; 9] = [0.0; 9];
    let mut sec_mops: [f64; 9] = [0.0; 9];
    let mut sec_time: [f64; 9] = [0.0; 9];

    // --- Section 1: Array elements ---
    e1[0] = 1.0; e1[1] = -1.0; e1[2] = -1.0; e1[3] = -1.0;
    let ts1 = Instant::now();
    for _ in 0..xtra {
        for _ in 0..(n1 * 10) {
            e1[0] = (e1[0] + e1[1] + e1[2] - e1[3]) * t;
            e1[1] = (e1[0] + e1[1] - e1[2] + e1[3]) * t;
            e1[2] = (e1[0] - e1[1] + e1[2] + e1[3]) * t;
            e1[3] = (-e1[0] + e1[1] + e1[2] + e1[3]) * t;
        }
        t = 1.0 - t;
    }
    t = t0;
    let sec1 = ts1.elapsed().as_secs_f64() / 10.0;
    total_time += sec1;
    sec_result[1] = e1[3] as f64;
    sec_mflops[1] = (n1 * 16) as f64 * xtra as f64 / (1000000.0 * sec1);
    sec_time[1] = sec1;

    // --- Section 2: Array as parameter ---
    let ts2 = Instant::now();
    for _ in 0..xtra {
        for _ in 0..n2 {
            whet_pa(&mut e1, t, t2);
        }
        t = 1.0 - t;
    }
    t = t0;
    let sec2 = ts2.elapsed().as_secs_f64();
    total_time += sec2;
    sec_result[2] = e1[3] as f64;
    sec_mflops[2] = (n2 * 96) as f64 * xtra as f64 / (1000000.0 * sec2);
    sec_time[2] = sec2;

    // --- Section 3: Conditional jumps ---
    init_n3_seeds();
    let mut j: i64 = 1;
    let ts3 = Instant::now();
    for _ in 0..xtra {
        for i in 0..n3 {
            if j == 1 { j = 2; } else if j == 0 { j = 3; } else { break; }
            if j > 2 { j = 0; } else { j = 1; }
            if j < 1 { j = 1; } else { j = 0; }
            unsafe {
                N3_SEEDS[(i & 255) as usize] = j;
                std::arch::asm!("", options(nostack, preserves_flags));
                j = N3_SEEDS[((i + 1) & 255) as usize];
            }
        }
    }
    let sec3 = ts3.elapsed().as_secs_f64();
    total_time += sec3;
    sec_result[3] = j as f64;
    sec_mops[3] = (n3 * 3) as f64 * xtra as f64 / (1000000.0 * sec3);
    sec_time[3] = sec3;

    // --- Section 4: Integer arithmetic ---
    j = 1;
    let mut k: i64 = 2;
    let mut l: i64 = 3;
    let ts4 = Instant::now();
    for _ in 0..xtra {
        for _ in 0..n4 {
            j = j * (k - j) * (l - k);
            k = l * k - (l - j) * k;
            l = (l - k) * (k + j);
            e1[(l - 2) as usize] = (j + k + l) as f32;
            e1[(k - 2) as usize] = (j * k * l) as f32;
            unsafe { std::arch::asm!("/* {0} {1} {2} */", inlateout(reg) j, inlateout(reg) k, inlateout(reg) l, options(nostack, preserves_flags)); }
        }
    }
    let sec4 = ts4.elapsed().as_secs_f64();
    total_time += sec4;
    let x4: f64 = e1[0] as f64 + e1[1] as f64;
    sec_result[4] = x4;
    sec_mops[4] = (n4 * 15) as f64 * xtra as f64 / (1000000.0 * sec4);
    sec_time[4] = sec4;

    // --- Section 5: Trig functions ---
    let mut x: f32 = 0.5;
    let mut y: f32 = 0.5;
    let ts5 = Instant::now();
    for _ in 0..xtra {
        for _ in 1..n5 {
            x = t * (t2 * x.sin() * x.cos() / ((x + y).cos() + (x - y).cos() - 1.0)).atan();
            y = t * (t2 * y.sin() * y.cos() / ((x + y).cos() + (x - y).cos() - 1.0)).atan();
        }
        t = 1.0 - t;
    }
    t = t0;
    let sec5 = ts5.elapsed().as_secs_f64();
    total_time += sec5;
    sec_result[5] = y as f64;
    sec_mops[5] = (n5 * 26) as f64 * xtra as f64 / (1000000.0 * sec5);
    sec_time[5] = sec5;

    // --- Section 6: Procedure calls ---
    x = 1.0;
    y = 1.0;
    let mut z: f32 = 1.0;
    let ts6 = Instant::now();
    for _ in 0..xtra {
        for _ in 0..n6 {
            whet_p3(&mut x, &mut y, &mut z, t, t1, t2);
        }
    }
    let sec6 = ts6.elapsed().as_secs_f64();
    total_time += sec6;
    sec_result[6] = z as f64;
    sec_mflops[6] = (n6 * 6) as f64 * xtra as f64 / (1000000.0 * sec6);
    sec_time[6] = sec6;

    // --- Section 7: Array references ---
    j = 0; k = 1; l = 2;
    e1[0] = 1.0; e1[1] = 2.0; e1[2] = 3.0;
    let ts7 = Instant::now();
    for _ in 0..xtra {
        for _ in 0..n7 {
            whet_po(&mut e1, j as usize, k as usize, l as usize);
            unsafe { std::arch::asm!("", options(nostack, preserves_flags)); }
        }
    }
    let sec7 = ts7.elapsed().as_secs_f64();
    total_time += sec7;
    sec_result[7] = e1[2] as f64;
    sec_mops[7] = (n7 * 3) as f64 * xtra as f64 / (1000000.0 * sec7);
    sec_time[7] = sec7;

    // --- Section 8: Standard functions ---
    x = 0.75;
    let ts8 = Instant::now();
    for _ in 0..xtra {
        for _ in 0..n8 {
            x = (x.ln() / t1).exp().sqrt();
        }
    }
    let sec8 = ts8.elapsed().as_secs_f64();
    total_time += sec8;
    sec_result[8] = x as f64;
    sec_mops[8] = (n8 * 4) as f64 * xtra as f64 / (1000000.0 * sec8);
    sec_time[8] = sec8;

    // --- Accumulate check ---
    let mut whet_check: f64 = 0.0;
    for s in 1..=8 {
        whet_check += sec_result[s];
    }

    // --- Print results (C format) ---
    println!();
    println!(" Single Precision Rust Whetstone Benchmark");
    println!("         rustc Linux");
    println!("         -C opt-level=3");
    println!();
    println!("Loop content                  Result              MFLOPS      MOPS   Seconds");
    println!();

    print_line("N1 floating point", sec_result[1], sec_mflops[1], sec_mops[1], sec_time[1], true);
    print_line("N2 floating point", sec_result[2], sec_mflops[2], sec_mops[2], sec_time[2], true);
    print_line("N3 if then else  ", sec_result[3], sec_mflops[3], sec_mops[3], sec_time[3], false);
    print_line("N4 fixed point   ", sec_result[4], sec_mflops[4], sec_mops[4], sec_time[4], false);
    print_line("N5 sin,cos etc.  ", sec_result[5], sec_mflops[5], sec_mops[5], sec_time[5], false);
    print_line("N6 floating point", sec_result[6], sec_mflops[6], sec_mops[6], sec_time[6], true);
    print_line("N7 assignments   ", sec_result[7], sec_mflops[7], sec_mops[7], sec_time[7], false);
    print_line("N8 exp,sqrt etc. ", sec_result[8], sec_mflops[8], sec_mops[8], sec_time[8], false);

    println!();
    let mwips = xtra as f64 * x100 as f64 / (10.0 * total_time);
    println!("MWIPS            {:>39.3}{:>19.3}", mwips, total_time);
    println!();

    if whet_check == 0.0 {
        println!("Wrong answer");
    }
}
