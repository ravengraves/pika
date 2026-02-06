// mandelbrot_ref.rs - Mandelbrot set benchmark for Rust
// Plots Mandelbrot set [-1.5-i, 0.5+i] on NxN bitmap.
// Outputs PBM P4 format to stdout.
//
// Processes 4 rows x 8 pixels = 32 pixels at a time.
// The wider inner loop (k=0..31) gives the compiler more
// independent work to pack into SIMD and fill execution ports.
// Batch early-exit: 6 batches of 8 iters + 2 final = 50 total.
//
// Matches Pika mandelbrot.pka benchmark.

use std::io::{BufWriter, Write};

const MAND_N: i32 = 16000;

fn main() {
    let n8 = (MAND_N + 7) & !7;
    let byte_width = n8 as usize / 8;
    let inv_n: f64 = 2.0 / n8 as f64;

    let stdout = std::io::stdout();
    let mut out = BufWriter::new(stdout.lock());

    write!(out, "P4\n{} {}\n", n8, n8).unwrap();

    // 4 row buffers
    let mut rows = vec![0u8; byte_width * 4];

    for py in (0..n8).step_by(4) {
        let ci = [
            inv_n * (py + 0) as f64 - 1.0,
            inv_n * (py + 1) as f64 - 1.0,
            inv_n * (py + 2) as f64 - 1.0,
            inv_n * (py + 3) as f64 - 1.0,
        ];

        for xbyte in 0..byte_width {
            // 32 pixels: 4 rows x 8 columns
            let mut cr = [0.0f64; 32];
            let mut zr = [0.0f64; 32];
            let mut zi = [0.0f64; 32];
            let mut ci_arr = [0.0f64; 32];

            for k in 0..8 {
                let px = xbyte * 8 + k;
                let c = inv_n * px as f64 - 1.5;
                cr[k]      = c;  cr[k + 8]  = c;  cr[k + 16] = c;  cr[k + 24] = c;
                zr[k]      = c;  zr[k + 8]  = c;  zr[k + 16] = c;  zr[k + 24] = c;
                zi[k]      = ci[0]; zi[k + 8]  = ci[1]; zi[k + 16] = ci[2]; zi[k + 24] = ci[3];
                ci_arr[k]  = ci[0]; ci_arr[k + 8] = ci[1]; ci_arr[k + 16] = ci[2]; ci_arr[k + 24] = ci[3];
            }

            let mut escaped_all = false;

            // 6 batches of 8 iterations = 48
            for _ in 0..6 {
                for _ in 0..8 {
                    for k in 0..32 {
                        let zr2 = zr[k] * zr[k];
                        let zi2 = zi[k] * zi[k];
                        zi[k] = 2.0 * zr[k] * zi[k] + ci_arr[k];
                        zr[k] = zr2 - zi2 + cr[k];
                    }
                }

                let mut all_out = true;
                for k in 0..32 {
                    if zr[k] * zr[k] + zi[k] * zi[k] <= 4.0 {
                        all_out = false;
                    }
                }
                if all_out {
                    escaped_all = true;
                    break;
                }
            }

            if !escaped_all {
                // 2 final iterations (48+2=50)
                for _ in 0..2 {
                    for p in 0..32 {
                        let r2 = zr[p] * zr[p];
                        let i2 = zi[p] * zi[p];
                        zi[p] = 2.0 * zr[p] * zi[p] + ci_arr[p];
                        zr[p] = r2 - i2 + cr[p];
                    }
                }
            }

            // Extract 4 bytes (one per row), MSB = leftmost pixel
            let mut b = [0u8; 4];
            if !escaped_all {
                for row in 0..4 {
                    let base = row * 8;
                    let mut byte = 0u8;
                    for p in 0..8 {
                        let idx = base + p;
                        if zr[idx] * zr[idx] + zi[idx] * zi[idx] <= 4.0 {
                            byte |= 128 >> p;
                        }
                    }
                    b[row] = byte;
                }
            }

            rows[0 * byte_width + xbyte] = b[0];
            rows[1 * byte_width + xbyte] = b[1];
            rows[2 * byte_width + xbyte] = b[2];
            rows[3 * byte_width + xbyte] = b[3];
        }

        // Write 4 rows
        for row in 0..4 {
            out.write_all(&rows[row * byte_width..(row + 1) * byte_width]).unwrap();
        }
    }

    out.flush().unwrap();
}
