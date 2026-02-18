#![allow(static_mut_refs)]
#![allow(unused_assignments)]

// dhrystone_ref.rs - Dhrystone Benchmark 1.1 for Rust
// Adapted from Reinhold P. Weicker's Dhrystone 1.1
// Output format matches C reference

use std::hint::black_box;
use std::time::Instant;

const IDENT1: i32 = 0;
const IDENT2: i32 = 1;
const IDENT3: i32 = 2;
const IDENT4: i32 = 3;
#[allow(dead_code)]
const IDENT5: i32 = 4;

#[derive(Clone, Copy)]
struct DhryRecord {
    ptr_comp: i32,
    discr: i32,
    enum_comp: i32,
    int_comp: i32,
}

// Global state
static mut INT_GLOB: i32 = 0;
static mut BOOL_GLOB: bool = false;
static mut CHAR1_GLOB: u8 = 0;
static mut CHAR2_GLOB: u8 = 0;
static mut ARRAY1_GLOB: [i32; 51] = [0; 51];
static mut ARRAY2_GLOB: [i32; 2601] = [0; 2601];

static mut REC_NEXT: DhryRecord = DhryRecord { ptr_comp: -1, discr: 0, enum_comp: 0, int_comp: 0 };
static mut REC_GLB: DhryRecord = DhryRecord { ptr_comp: 0, discr: 0, enum_comp: 0, int_comp: 0 };

static mut STR_GLB: [u8; 31] = [0; 31];
static mut STR_NEXT: [u8; 31] = [0; 31];
static mut STR1_LOC: [u8; 31] = [0; 31];
static mut STR2_LOC: [u8; 31] = [0; 31];

fn str_set(dst: &mut [u8; 31], src: &[u8]) {
    let n = src.len().min(30);
    dst[..n].copy_from_slice(&src[..n]);
    dst[n] = 0;
}

fn str_cpy(dst: &mut [u8; 31], src: &[u8; 31]) {
    dst.copy_from_slice(src);
}

fn str_cmp(a: &[u8; 31], b: &[u8; 31]) -> i32 {
    for i in 0..31 {
        if a[i] != b[i] {
            return if a[i] < b[i] { -1 } else { 1 };
        }
        if a[i] == 0 { break; }
    }
    0
}

fn func3(ep: i32) -> bool {
    ep == IDENT3
}

fn func1(ch1: u8, ch2: u8) -> i32 {
    if ch1 != ch2 { IDENT1 } else { IDENT2 }
}

unsafe fn func2(s1: &[u8; 31], s2: &[u8; 31]) -> bool {
    let mut intloc: i32 = 1;
    let mut charloc: u8 = 0;
    while intloc <= 1 {
        if func1(s1[intloc as usize], s2[(intloc + 1) as usize]) == IDENT1 {
            charloc = b'A';
            intloc += 1;
        }
    }
    if charloc >= b'W' && charloc <= b'Z' {
        intloc = 7;
    }
    if charloc == b'X' {
        return true;
    }
    if str_cmp(s1, s2) > 0 {
        intloc += 7;
        return true;
    }
    let _ = intloc;
    false
}

unsafe fn proc7(i1: i32, i2: i32, out: &mut i32) {
    let loc = i1 + 2;
    *out = i2 + loc;
}

unsafe fn proc6(ein: i32, eout: &mut i32) {
    *eout = ein;
    if !func3(ein) {
        *eout = IDENT4;
    }
    match ein {
        0 => *eout = IDENT1,
        1 => {
            if INT_GLOB > 100 { *eout = IDENT1; }
            else { *eout = IDENT4; }
        }
        2 => *eout = IDENT2,
        3 => {}
        4 => *eout = IDENT3,
        _ => {}
    }
}

unsafe fn proc8(a1: &mut [i32; 51], a2: &mut [i32; 2601], i1: i32, i2: i32) {
    let loc = i1 + 5;
    a1[loc as usize] = i2;
    a1[(loc + 1) as usize] = a1[loc as usize];
    a1[(loc + 30) as usize] = loc;
    for idx in loc..=(loc + 1) {
        a2[(loc * 51 + idx) as usize] = loc;
    }
    a2[(loc * 51 + loc - 1) as usize] += 1;
    a2[((loc + 20) * 51 + loc) as usize] = a1[loc as usize];
    INT_GLOB = 5;
}

unsafe fn proc5() {
    CHAR1_GLOB = b'A';
    BOOL_GLOB = false;
}

unsafe fn proc4() {
    let bloc: bool = CHAR1_GLOB == b'A';
    if bloc || BOOL_GLOB {
        CHAR2_GLOB = b'B';
    }
}

unsafe fn proc3(out: &mut i32) {
    if REC_GLB.ptr_comp >= 0 {
        *out = REC_GLB.enum_comp;
    } else {
        INT_GLOB = 100;
    }
    proc7(10, INT_GLOB, &mut REC_GLB.int_comp);
}

unsafe fn proc2(io: &mut i32) {
    let mut intloc = *io + 10;
    let mut eloc: i32 = 0;
    loop {
        if CHAR1_GLOB == b'A' {
            intloc -= 1;
            *io = intloc - INT_GLOB;
            eloc = IDENT1;
        }
        if eloc == IDENT1 { break; }
    }
}

unsafe fn proc1() {
    // NextRecord = *PtrGlb (struct copy)
    REC_NEXT = REC_GLB;
    str_cpy(&mut STR_NEXT, &STR_GLB);
    black_box(&STR_NEXT);
    REC_GLB.int_comp = 5;
    REC_NEXT.int_comp = REC_GLB.int_comp;
    REC_NEXT.ptr_comp = REC_GLB.ptr_comp;
    proc3(&mut REC_NEXT.enum_comp);
    if REC_NEXT.discr == IDENT1 {
        REC_NEXT.int_comp = 6;
        proc6(REC_GLB.enum_comp, &mut REC_NEXT.enum_comp);
        REC_NEXT.ptr_comp = REC_GLB.ptr_comp;
        proc7(REC_NEXT.int_comp, 10, &mut REC_NEXT.int_comp);
    } else {
        REC_GLB = REC_NEXT;
    }
}

fn main() {
    unsafe {
        // Initialize records
        REC_GLB.ptr_comp = 0;
        REC_GLB.discr = IDENT1;
        REC_GLB.enum_comp = IDENT3;
        REC_GLB.int_comp = 40;
        str_set(&mut STR_GLB, b"DHRYSTONE PROGRAM, SOME STRING");
        str_set(&mut STR1_LOC, b"DHRYSTONE PROGRAM, 1'ST STRING");

        let loops: u64 = 100_000_000;

        ARRAY2_GLOB[8 * 51 + 7] = 10;

        let start = Instant::now();

        for _ in 0..loops {
            proc5();
            proc4();
            let mut intloc1: i32 = 2;
            let intloc2: i32 = 3;
            let mut intloc3: i32 = 0;
            str_set(&mut STR2_LOC, b"DHRYSTONE PROGRAM, 2'ND STRING");
            black_box(&STR2_LOC);
            let mut enumloc: i32 = IDENT2;
            BOOL_GLOB = !func2(&STR1_LOC, &STR2_LOC);
            black_box(BOOL_GLOB);
            while intloc1 < intloc2 {
                intloc3 = 5 * intloc1 - intloc2;
                proc7(intloc1, intloc2, &mut intloc3);
                intloc1 += 1;
            }
            proc8(&mut ARRAY1_GLOB, &mut ARRAY2_GLOB, intloc1, intloc3);
            proc1();
            let mut ch = b'A';
            while ch <= CHAR2_GLOB {
                black_box(ch); // prevent loop elimination
                if enumloc == func1(ch, b'C') {
                    proc6(IDENT1, &mut enumloc);
                }
                ch += 1;
            }
            intloc3 = intloc2 * intloc1;
            let intloc2_new = intloc3 / intloc1;
            black_box(7 * (intloc3 - intloc2_new) - intloc1);
            proc2(&mut intloc1);
            black_box(intloc1); // preserve Proc2 + final computations
        }

        let elapsed = start.elapsed().as_secs_f64();

        println!();
        println!("Dhrystone Benchmark, Version 1.1 (Language: Rust)");
        println!();

        println!("{:>12.0} runs {:6.2} seconds ", loops as f64, elapsed);
        println!();

        print!("Array2Glob8/7: ");
        if ARRAY2_GLOB[8 * 51 + 7] == (loops as i32) + 10 {
            print!("O.K.  ");
        } else {
            print!("WRONG ");
        }
        println!("{:>12.0}", ARRAY2_GLOB[8 * 51 + 7] as f64);

        let usec = elapsed * 1_000_000.0 / loops as f64;
        let dps = loops as f64 / elapsed;
        let vax_mips = dps / 1757.0;

        println!();
        println!("Microseconds for one run through Dhrystone: {:>12.2} ", usec);
        println!("Dhrystones per Second:                      {:>10.0} ", dps);
        println!("VAX  MIPS rating =                          {:>12.2} ", vax_mips);
        println!();

        // Anti-DCE: print all global state
        println!("Global values (anti-DCE check):");
        println!("  IntGlob:      {}", INT_GLOB);
        println!("  BoolGlob:     {}", BOOL_GLOB as i32);
        println!("  Char1Glob:    {}", CHAR1_GLOB as i32);
        println!("  Char2Glob:    {}", CHAR2_GLOB as i32);
        println!("  Array1Glob[8]:{}", ARRAY1_GLOB[8]);
        println!("  RecGlb.int_comp:  {}", REC_GLB.int_comp);
        println!("  RecGlb.enum_comp: {}", REC_GLB.enum_comp);
        println!("  RecNext.discr:    {}", REC_NEXT.discr);
        println!("  RecNext.enum_comp:{}", REC_NEXT.enum_comp);
        println!("  RecNext.int_comp: {}", REC_NEXT.int_comp);
        println!("  RecNext.ptr_comp: {}", REC_NEXT.ptr_comp);
        let mut str_chk: i32 = 0;
        for i in 0..31 {
            str_chk += STR_GLB[i] as i32 + STR_NEXT[i] as i32;
        }
        println!("  StrChecksum:      {}", str_chk);
    }
}
