#!/usr/bin/env python3
"""
Evolutionary Magic Constant Optimizer for fast sqrt/rcp/rsqrt approximations.

Optimizes constants for IEEE 754 bit manipulation tricks:
  1/x   : y = C - I(x)           then Newton: y = y * (2 - x*y)
  1/√x  : y = C - (I(x) >> 1)    then Newton: y = y * (1.5 - 0.5*x*y²)
  √x    : y = (I(x) >> 1) + C    then Newton: y = 0.5 * (y + x/y)

Where I(x) is the integer bit representation of float x.

Usage: python optimize_sqrt_rcp_rsqrt.py
"""

import numpy as np

rng = np.random.default_rng(42)

# =============================================================================
# Test samples
# =============================================================================

N = 15000
X = np.logspace(-3, 3, N, dtype=np.float32)  # [0.001, 1000]
IX = X.view(np.int32)

EXACT_RCP = np.float32(1) / X
EXACT_RSQRT = np.float32(1) / np.sqrt(X)
EXACT_SQRT = np.sqrt(X)


# =============================================================================
# Bit trick + Newton evaluation
# =============================================================================

def eval_rcp(C, n_newton):
    """1/x: y = C - I(x), then n Newton iterations."""
    C = np.int32(C)
    iy = C - IX
    y = iy.view(np.float32).copy()
    for _ in range(n_newton):
        y = y * (np.float32(2) - X * y)
    return np.nanmax(np.abs(y - EXACT_RCP) / EXACT_RCP)


def eval_rsqrt(C, n_newton):
    """1/√x: y = C - (I(x) >> 1), then n Newton iterations."""
    C = np.int32(C)
    iy = C - (IX >> 1)
    y = iy.view(np.float32).copy()
    for _ in range(n_newton):
        y = y * (np.float32(1.5) - np.float32(0.5) * X * y * y)
    return np.nanmax(np.abs(y - EXACT_RSQRT) / EXACT_RSQRT)


def eval_sqrt(C, n_newton):
    """√x: y = (I(x) >> 1) + C, then n Newton iterations."""
    C = np.int32(C)
    iy = (IX >> 1) + C
    y = iy.view(np.float32).copy()
    for _ in range(n_newton):
        y = np.float32(0.5) * (y + X / y)
    return np.nanmax(np.abs(y - EXACT_SQRT) / EXACT_SQRT)


# =============================================================================
# Evolutionary optimization
# =============================================================================

def evolve(eval_fn, n_newton, center, pop_size=300, generations=15,
           sigma=100000, decay=0.5, elite_frac=0.1):
    """
    Evolutionary optimization for magic constant.
    
    Args:
        eval_fn: Function(C, n_newton) -> max_error
        n_newton: Number of Newton iterations
        center: Initial center estimate
        pop_size: Population size
        generations: Number of generations
        sigma: Initial mutation standard deviation
        decay: Sigma decay per generation
        elite_frac: Fraction of population to keep as elite
    
    Returns:
        (best_constant, best_error, history)
    """
    n_elite = max(1, int(pop_size * elite_frac))
    
    # Initialize population
    pop = rng.normal(center, sigma, pop_size).astype(np.int64)
    
    best_c = center
    best_e = eval_fn(center, n_newton)
    history = [(0, best_c, best_e)]
    
    for gen in range(generations):
        # Evaluate fitness
        errors = np.array([eval_fn(int(c), n_newton) for c in pop])
        
        # Update best
        idx = np.argmin(errors)
        if errors[idx] < best_e:
            best_e = errors[idx]
            best_c = int(pop[idx])
            history.append((gen + 1, best_c, best_e))
        
        # Select elite
        elite_idx = np.argsort(errors)[:n_elite]
        elite = pop[elite_idx]
        elite_center = int(np.median(elite))
        
        # Decay sigma and resample
        sigma = max(1, int(sigma * decay))
        pop = rng.normal(elite_center, sigma, pop_size).astype(np.int64)
        pop[0] = best_c  # Elitism
    
    # Final local refinement
    local = np.arange(best_c - 100, best_c + 101, dtype=np.int32)
    errors = np.array([eval_fn(int(c), n_newton) for c in local])
    idx = np.argmin(errors)
    
    final_c = int(local[idx])
    final_e = errors[idx]
    if final_e < best_e:
        best_c, best_e = final_c, final_e
        history.append(("final", best_c, best_e))
    
    return best_c, best_e, history


# =============================================================================
# Main
# =============================================================================

if __name__ == "__main__":
    print("=" * 70)
    print("EVOLUTIONARY MAGIC CONSTANT OPTIMIZER")
    print("=" * 70)
    print(f"Test samples: {N} points in [0.001, 1000]")
    
    L = 2**23  # Mantissa bits
    
    # Theoretical starting points
    centers = {
        'rcp':   int(2.0 * L * 126.95),
        'rsqrt': int(1.5 * L * 126.955),
        'sqrt':  int(0.5 * L * 126.93),
    }
    
    eval_fns = {
        'rcp':   eval_rcp,
        'rsqrt': eval_rsqrt,
        'sqrt':  eval_sqrt,
    }
    
    names = {
        'rcp':   '1/x',
        'rsqrt': '1/√x',
        'sqrt':  '√x',
    }
    
    results = {}
    
    for key in ['rcp', 'rsqrt', 'sqrt']:
        print(f"\n{'─'*70}")
        print(f"[{names[key]}]")
        print(f"{'─'*70}")
        
        for n_newton in [0, 1, 2]:
            c, e, hist = evolve(eval_fns[key], n_newton, centers[key])
            results[(key, n_newton)] = (c, e)
            
            err_fmt = f"{e:.6%}" if n_newton == 2 else f"{e:.4%}"
            print(f"  {n_newton} Newton: 0x{c:08X}  error: {err_fmt}")
    
    # Compare rsqrt with famous constants
    print(f"\n{'─'*70}")
    print("[Comparison: 1/√x]")
    print(f"{'─'*70}")
    
    famous = {
        'Quake III (1999)': 0x5F3759DF,
        'Lomont (2003)':    0x5F375A86,
    }
    
    print(f"{'Constant':<22} {'0 Newton':<12} {'1 Newton':<12} {'2 Newton':<14}")
    for name, c in famous.items():
        e0 = eval_rsqrt(c, 0)
        e1 = eval_rsqrt(c, 1)
        e2 = eval_rsqrt(c, 2)
        print(f"{name:<22} {e0:.4%}       {e1:.4%}       {e2:.6%}")
    
    for n in [0, 1, 2]:
        c = results[('rsqrt', n)][0]
        e0 = eval_rsqrt(c, 0)
        e1 = eval_rsqrt(c, 1)
        e2 = eval_rsqrt(c, 2)
        print(f"{'Ours (opt for ' + str(n) + 'N)':<22} {e0:.4%}       {e1:.4%}       {e2:.6%}")
    
    # Output C code
    print(f"\n{'='*70}")
    print("C CODE OUTPUT")
    print(f"{'='*70}")
    
    print(f"""
/* Optimized magic constants for IEEE 754 float32 bit tricks.
 * Generated by evolutionary optimization over {N} test points in [0.001, 1000].
 * 
 * Usage:
 *   uint32_t i = *(uint32_t*)&x;
 *   i = CONSTANT - i;  // or appropriate formula
 *   float y = *(float*)&i;
 *   // Apply Newton-Raphson iterations as needed
 */

/* 1/x (reciprocal): y = C - I(x) */
#define RCP_MAGIC_0N    0x{results[('rcp',0)][0]:08X}U  /* {results[('rcp',0)][1]:.4%} max error */
#define RCP_MAGIC_1N    0x{results[('rcp',1)][0]:08X}U  /* {results[('rcp',1)][1]:.4%} max error */
#define RCP_MAGIC_2N    0x{results[('rcp',2)][0]:08X}U  /* {results[('rcp',2)][1]:.6%} max error */

/* 1/√x (inverse sqrt): y = C - (I(x) >> 1) */
#define RSQRT_MAGIC_0N  0x{results[('rsqrt',0)][0]:08X}U  /* {results[('rsqrt',0)][1]:.4%} max error */
#define RSQRT_MAGIC_1N  0x{results[('rsqrt',1)][0]:08X}U  /* {results[('rsqrt',1)][1]:.4%} max error */
#define RSQRT_MAGIC_2N  0x{results[('rsqrt',2)][0]:08X}U  /* {results[('rsqrt',2)][1]:.6%} max error */

/* √x (sqrt): y = (I(x) >> 1) + C */
#define SQRT_MAGIC_0N   0x{results[('sqrt',0)][0]:08X}U  /* {results[('sqrt',0)][1]:.4%} max error */
#define SQRT_MAGIC_1N   0x{results[('sqrt',1)][0]:08X}U  /* {results[('sqrt',1)][1]:.4%} max error */
#define SQRT_MAGIC_2N   0x{results[('sqrt',2)][0]:08X}U  /* {results[('sqrt',2)][1]:.6%} max error */

/* Bit casting helpers */
static inline uint32_t f32_to_u32(float x) {{
    union {{ float f; uint32_t u; }} v = {{x}};
    return v.u;
}}

static inline float u32_to_f32(uint32_t u) {{
    union {{ uint32_t u; float f; }} v = {{u}};
    return v.f;
}}

/* Reciprocal: 1/x with 2 Newton iterations ({results[('rcp',2)][1]:.6%} error) */
static inline float rcp_fast(float x) {{
    uint32_t i = f32_to_u32(x);
    i = RCP_MAGIC_2N - i;
    float y = u32_to_f32(i);
    y = y * (2.0f - x * y);
    y = y * (2.0f - x * y);
    return y;
}}

/* Inverse sqrt: 1/√x with 2 Newton iterations ({results[('rsqrt',2)][1]:.6%} error) */
static inline float rsqrt_fast(float x) {{
    uint32_t i = f32_to_u32(x);
    i = RSQRT_MAGIC_2N - (i >> 1);
    float y = u32_to_f32(i);
    y = y * (1.5f - 0.5f * x * y * y);
    y = y * (1.5f - 0.5f * x * y * y);
    return y;
}}

/* Square root: √x with 2 Newton iterations ({results[('sqrt',2)][1]:.6%} error) */
static inline float sqrt_fast(float x) {{
    uint32_t i = f32_to_u32(x);
    i = (i >> 1) + SQRT_MAGIC_2N;
    float y = u32_to_f32(i);
    y = 0.5f * (y + x / y);
    y = 0.5f * (y + x / y);
    return y;
}}
""")
