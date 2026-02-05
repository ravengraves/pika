#!/usr/bin/env python3
"""
Evolutionary Polynomial Coefficient Optimizer for fast trig approximations.

Optimizes coefficients for:
  sin(x) ≈ x * (c0 + c1*x² + c2*x⁴ + c3*x⁶)         on [-π/4, π/4]
  cos(x) ≈ c0 + c1*x² + c2*x⁴ + c3*x⁶               on [-π/4, π/4]
  tan(x) ≈ x * (c0 + c1*x² + c2*x⁴ + c3*x⁶ + c4*x⁸) on [-π/4, π/4]
  atan(x) ≈ x * (c0 + c1*x² + c2*x⁴ + c3*x⁶ + c4*x⁸) on [-1, 1]

Usage: python optimize_trig.py
"""

import numpy as np

rng = np.random.default_rng(42)

# =============================================================================
# Test ranges
# =============================================================================

PI = np.float64(np.pi)
PI4 = np.float32(PI / 4)

N = 5000

# sin/cos/tan: x ∈ [-π/4, π/4]
X_TRIG = np.linspace(-PI4, PI4, N, dtype=np.float32)
X2_TRIG = X_TRIG * X_TRIG

EXACT_SIN = np.sin(X_TRIG).astype(np.float32)
EXACT_COS = np.cos(X_TRIG).astype(np.float32)
EXACT_TAN = np.tan(X_TRIG).astype(np.float32)

# atan: x ∈ [-1, 1]
X_ATAN = np.linspace(-1, 1, N, dtype=np.float32)
X2_ATAN = X_ATAN * X_ATAN
EXACT_ATAN = np.arctan(X_ATAN).astype(np.float32)

# asin: x ∈ [-0.5, 0.5] (core range, extended via identities)
X_ASIN = np.linspace(-0.5, 0.5, N, dtype=np.float32)
EXACT_ASIN = np.arcsin(X_ASIN).astype(np.float32)


# =============================================================================
# Polynomial evaluation
# =============================================================================

def eval_sin(x, x2, c):
    """sin(x) ≈ x * (c0 + c1*x² + c2*x⁴ + c3*x⁶)"""
    x4 = x2 * x2
    poly = c[0] + c[1]*x2 + c[2]*x4 + c[3]*x4*x2
    return x * poly


def eval_cos(x2, c):
    """cos(x) ≈ c0 + c1*x² + c2*x⁴ + c3*x⁶"""
    x4 = x2 * x2
    return c[0] + c[1]*x2 + c[2]*x4 + c[3]*x4*x2


def eval_tan(x, x2, c):
    """tan(x) ≈ x * (c0 + c1*x² + c2*x⁴ + c3*x⁶ + c4*x⁸)"""
    x4 = x2 * x2
    x8 = x4 * x4
    poly = c[0] + c[1]*x2 + c[2]*x4 + c[3]*x4*x2 + c[4]*x8
    return x * poly


def eval_atan(x, x2, c):
    """atan(x) ≈ x * (c0 + c1*x² + c2*x⁴ + c3*x⁶ + c4*x⁸)"""
    x4 = x2 * x2
    x8 = x4 * x4
    poly = c[0] + c[1]*x2 + c[2]*x4 + c[3]*x4*x2 + c[4]*x8
    return x * poly


def eval_asin(x, c):
    """asin(x) ≈ x + x³*(c0 + c1*x² + c2*x⁴ + c3*x⁶)"""
    x2 = x * x
    x3 = x * x2
    x4 = x2 * x2
    poly = c[0] + c[1]*x2 + c[2]*x4 + c[3]*x4*x2
    return x + x3 * poly


# =============================================================================
# Error functions
# =============================================================================

def sin_error(c):
    approx = eval_sin(X_TRIG, X2_TRIG, c.astype(np.float32))
    # Avoid division by zero near x=0
    denom = np.maximum(np.abs(EXACT_SIN), 1e-10)
    return np.max(np.abs(approx - EXACT_SIN) / denom)


def cos_error(c):
    approx = eval_cos(X2_TRIG, c.astype(np.float32))
    return np.max(np.abs(approx - EXACT_COS) / EXACT_COS)


def tan_error(c):
    approx = eval_tan(X_TRIG, X2_TRIG, c.astype(np.float32))
    denom = np.maximum(np.abs(EXACT_TAN), 1e-10)
    return np.max(np.abs(approx - EXACT_TAN) / denom)


def atan_error(c):
    approx = eval_atan(X_ATAN, X2_ATAN, c.astype(np.float32))
    denom = np.maximum(np.abs(EXACT_ATAN), 1e-10)
    return np.max(np.abs(approx - EXACT_ATAN) / denom)


def asin_error(c):
    approx = eval_asin(X_ASIN, c.astype(np.float32))
    denom = np.maximum(np.abs(EXACT_ASIN), 1e-10)
    return np.max(np.abs(approx - EXACT_ASIN) / denom)


# =============================================================================
# Evolutionary optimization
# =============================================================================

def evolve(error_fn, init_coeffs, pop_size=4096, generations=512,
           sigma=0.01, decay=0.99, elite_frac=0.1):
    """
    Evolutionary optimization for polynomial coefficients.
    """
    n_coeffs = len(init_coeffs)
    n_elite = max(1, int(pop_size * elite_frac))
    
    # Initialize population
    pop = rng.normal(init_coeffs, sigma * np.abs(init_coeffs) + 1e-8,
                     (pop_size, n_coeffs))
    
    best_c = init_coeffs.copy()
    best_e = error_fn(init_coeffs)
    
    for gen in range(generations):
        # Evaluate
        errors = np.array([error_fn(c) for c in pop])
        
        # Update best
        idx = np.argmin(errors)
        if errors[idx] < best_e:
            best_e = errors[idx]
            best_c = pop[idx].copy()
        
        # Select elite
        elite_idx = np.argsort(errors)[:n_elite]
        elite = pop[elite_idx]
        
        # Decay and resample
        sigma = sigma * decay
        new_pop = np.zeros((pop_size, n_coeffs))
        for i in range(pop_size):
            parent = elite[i % n_elite]
            mutation = rng.normal(0, sigma * np.abs(parent) + 1e-10, n_coeffs)
            new_pop[i] = parent + mutation
        
        new_pop[0] = best_c  # Elitism
        pop = new_pop
    
    return best_c, best_e


# =============================================================================
# Main
# =============================================================================

if __name__ == "__main__":
    print("=" * 70)
    print("EVOLUTIONARY TRIG COEFFICIENT OPTIMIZER")
    print("=" * 70)
    
    results = {}
    
    # -------------------------------------------------------------------------
    # SIN
    # -------------------------------------------------------------------------
    print("\n" + "─" * 70)
    print("[sin(x)] on [-π/4, π/4]")
    print("Formula: sin(x) ≈ x * (c0 + c1*x² + c2*x⁴ + c3*x⁶)")
    print("─" * 70)
    
    taylor_sin = np.array([1.0, -1/6, 1/120, -1/5040])
    print(f"Taylor:    {taylor_sin}")
    print(f"Taylor error: {sin_error(taylor_sin):.6%}")
    
    opt_sin, err_sin = evolve(sin_error, taylor_sin)
    results['sin'] = (opt_sin, err_sin)
    print(f"Optimized: {opt_sin}")
    print(f"Optimized error: {err_sin:.6%}")
    print(f"Improvement: {sin_error(taylor_sin)/err_sin:.1f}x better")
    
    # -------------------------------------------------------------------------
    # COS
    # -------------------------------------------------------------------------
    print("\n" + "─" * 70)
    print("[cos(x)] on [-π/4, π/4]")
    print("Formula: cos(x) ≈ c0 + c1*x² + c2*x⁴ + c3*x⁶")
    print("─" * 70)
    
    taylor_cos = np.array([1.0, -1/2, 1/24, -1/720])
    print(f"Taylor:    {taylor_cos}")
    print(f"Taylor error: {cos_error(taylor_cos):.6%}")
    
    opt_cos, err_cos = evolve(cos_error, taylor_cos)
    results['cos'] = (opt_cos, err_cos)
    print(f"Optimized: {opt_cos}")
    print(f"Optimized error: {err_cos:.6%}")
    print(f"Improvement: {cos_error(taylor_cos)/err_cos:.1f}x better")
    
    # -------------------------------------------------------------------------
    # TAN
    # -------------------------------------------------------------------------
    print("\n" + "─" * 70)
    print("[tan(x)] on [-π/4, π/4]")
    print("Formula: tan(x) ≈ x * (c0 + c1*x² + c2*x⁴ + c3*x⁶ + c4*x⁸)")
    print("─" * 70)
    
    taylor_tan = np.array([1.0, 1/3, 2/15, 17/315, 62/2835])
    print(f"Taylor:    {taylor_tan}")
    print(f"Taylor error: {tan_error(taylor_tan):.6%}")
    
    opt_tan, err_tan = evolve(tan_error, taylor_tan)
    results['tan'] = (opt_tan, err_tan)
    print(f"Optimized: {opt_tan}")
    print(f"Optimized error: {err_tan:.6%}")
    print(f"Improvement: {tan_error(taylor_tan)/err_tan:.1f}x better")
    
    # -------------------------------------------------------------------------
    # ATAN
    # -------------------------------------------------------------------------
    print("\n" + "─" * 70)
    print("[atan(x)] on [-1, 1]")
    print("Formula: atan(x) ≈ x * (c0 + c1*x² + c2*x⁴ + c3*x⁶ + c4*x⁸)")
    print("─" * 70)
    
    taylor_atan = np.array([1.0, -1/3, 1/5, -1/7, 1/9])
    print(f"Taylor:    {taylor_atan}")
    print(f"Taylor error: {atan_error(taylor_atan):.6%}")
    
    opt_atan, err_atan = evolve(atan_error, taylor_atan, pop_size=4096, generations=256,
           sigma=0.01, decay=0.98, elite_frac=0.1)
    results['atan'] = (opt_atan, err_atan)
    print(f"Optimized: {opt_atan}")
    print(f"Optimized error: {err_atan:.6%}")
    print(f"Improvement: {atan_error(taylor_atan)/err_atan:.1f}x better")
    
    # -------------------------------------------------------------------------
    # ASIN
    # -------------------------------------------------------------------------
    print("\n" + "─" * 70)
    print("[asin(x)] on [-0.5, 0.5]")
    print("Formula: asin(x) ≈ x + x³*(c0 + c1*x² + c2*x⁴ + c3*x⁶)")
    print("─" * 70)
    
    # Taylor: asin(x) = x + x³/6 + 3x⁵/40 + 15x⁷/336 + ...
    # Rewritten: x + x³*(1/6 + x²*3/40 + x⁴*15/336 + x⁶*105/3456)
    taylor_asin = np.array([1/6, 3/40, 15/336, 105/3456])
    print(f"Taylor:    {taylor_asin}")
    print(f"Taylor error: {asin_error(taylor_asin):.6%}")
    
    opt_asin, err_asin = evolve(asin_error, taylor_asin)
    results['asin'] = (opt_asin, err_asin)
    print(f"Optimized: {opt_asin}")
    print(f"Optimized error: {err_asin:.6%}")
    print(f"Improvement: {asin_error(taylor_asin)/err_asin:.1f}x better")
    
    # -------------------------------------------------------------------------
    # C Code Output
    # -------------------------------------------------------------------------
    print("\n" + "=" * 70)
    print("C CODE OUTPUT")
    print("=" * 70)
    
    sin_c, sin_e = results['sin']
    cos_c, cos_e = results['cos']
    tan_c, tan_e = results['tan']
    atan_c, atan_e = results['atan']
    asin_c, asin_e = results['asin']
    
    print(f"""
/* Optimized polynomial coefficients for fast trig approximations.
 * Generated by evolutionary optimization.
 *
 * sin:  {sin_error(taylor_sin):.6%} → {sin_e:.6%}
 * cos:  {cos_error(taylor_cos):.6%} → {cos_e:.6%}
 * tan:  {tan_error(taylor_tan):.6%} → {tan_e:.6%}
 * atan: {atan_error(taylor_atan):.6%} → {atan_e:.6%}
 * asin: {asin_error(taylor_asin):.6%} → {asin_e:.6%}
 */

/* sin(x) for x ∈ [-π/4, π/4]: sin(x) ≈ x * (c0 + c1*x² + c2*x⁴ + c3*x⁶) */
#define SIN_C0  {sin_c[0]: .10f}f  /* Taylor:  1.0 */
#define SIN_C1  {sin_c[1]: .10f}f  /* Taylor: -1/6 */
#define SIN_C2  {sin_c[2]: .10f}f  /* Taylor:  1/120 */
#define SIN_C3  {sin_c[3]: .10f}f  /* Taylor: -1/5040 */

/* cos(x) for x ∈ [-π/4, π/4]: cos(x) ≈ c0 + c1*x² + c2*x⁴ + c3*x⁶ */
#define COS_C0  {cos_c[0]: .10f}f  /* Taylor:  1.0 */
#define COS_C1  {cos_c[1]: .10f}f  /* Taylor: -1/2 */
#define COS_C2  {cos_c[2]: .10f}f  /* Taylor:  1/24 */
#define COS_C3  {cos_c[3]: .10f}f  /* Taylor: -1/720 */

/* tan(x) for x ∈ [-π/4, π/4]: tan(x) ≈ x * (c0 + c1*x² + c2*x⁴ + c3*x⁶ + c4*x⁸) */
#define TAN_C0  {tan_c[0]: .10f}f  /* Taylor:  1.0 */
#define TAN_C1  {tan_c[1]: .10f}f  /* Taylor:  1/3 */
#define TAN_C2  {tan_c[2]: .10f}f  /* Taylor:  2/15 */
#define TAN_C3  {tan_c[3]: .10f}f  /* Taylor:  17/315 */
#define TAN_C4  {tan_c[4]: .10f}f  /* Taylor:  62/2835 */

/* atan(x) for x ∈ [-1, 1]: atan(x) ≈ x * (c0 + c1*x² + c2*x⁴ + c3*x⁶ + c4*x⁸) */
#define ATAN_C0 {atan_c[0]: .10f}f  /* Taylor:  1.0 */
#define ATAN_C1 {atan_c[1]: .10f}f  /* Taylor: -1/3 */
#define ATAN_C2 {atan_c[2]: .10f}f  /* Taylor:  1/5 */
#define ATAN_C3 {atan_c[3]: .10f}f  /* Taylor: -1/7 */
#define ATAN_C4 {atan_c[4]: .10f}f  /* Taylor:  1/9 */

/* asin(x) for x ∈ [-0.5, 0.5]: asin(x) ≈ x + x³*(c0 + c1*x² + c2*x⁴ + c3*x⁶) */
#define ASIN_C0 {asin_c[0]: .10f}f  /* Taylor:  1/6 */
#define ASIN_C1 {asin_c[1]: .10f}f  /* Taylor:  3/40 */
#define ASIN_C2 {asin_c[2]: .10f}f  /* Taylor:  15/336 */
#define ASIN_C3 {asin_c[3]: .10f}f  /* Taylor:  105/3456 */

/* === IMPLEMENTATION === */

static inline float sin_fast(float x) {{
    /* Range reduction to [-π/4, π/4] should be done before calling */
    float x2 = x * x;
    float x4 = x2 * x2;
    float poly = SIN_C0 + SIN_C1*x2 + SIN_C2*x4 + SIN_C3*x4*x2;
    return x * poly;
}}

static inline float cos_fast(float x) {{
    /* Range reduction to [-π/4, π/4] should be done before calling */
    float x2 = x * x;
    float x4 = x2 * x2;
    return COS_C0 + COS_C1*x2 + COS_C2*x4 + COS_C3*x4*x2;
}}

static inline float tan_fast(float x) {{
    /* Range reduction to [-π/4, π/4] should be done before calling */
    float x2 = x * x;
    float x4 = x2 * x2;
    float x8 = x4 * x4;
    float poly = TAN_C0 + TAN_C1*x2 + TAN_C2*x4 + TAN_C3*x4*x2 + TAN_C4*x8;
    return x * poly;
}}

static inline float atan_fast(float x) {{
    /* For |x| > 1, use: atan(x) = π/2 - atan(1/x) */
    float x2 = x * x;
    float x4 = x2 * x2;
    float x8 = x4 * x4;
    float poly = ATAN_C0 + ATAN_C1*x2 + ATAN_C2*x4 + ATAN_C3*x4*x2 + ATAN_C4*x8;
    return x * poly;
}}

static inline float asin_fast(float x) {{
    /* For |x| > 0.5, use: asin(x) = π/2 - 2*asin(sqrt((1-x)/2)) */
    float x2 = x * x;
    float x3 = x * x2;
    float x4 = x2 * x2;
    float poly = ASIN_C0 + ASIN_C1*x2 + ASIN_C2*x4 + ASIN_C3*x4*x2;
    return x + x3 * poly;
}}
""")
    
    # -------------------------------------------------------------------------
    # Full sincos with range reduction
    # -------------------------------------------------------------------------
    print("""
/* === FULL IMPLEMENTATIONS WITH RANGE REDUCTION === */

#define PI          3.14159265358979323846f
#define PI_2        1.57079632679489661923f  /* π/2 */
#define PI_4        0.78539816339744830962f  /* π/4 */
#define TWO_OVER_PI 0.63661977236758134308f  /* 2/π */

static inline void sincos_fast(float x, float *s, float *c) {
    /* Range reduction to [-π/4, π/4] */
    float y = x * TWO_OVER_PI;
    int q = (int)(y >= 0 ? y + 0.5f : y - 0.5f);  /* quadrant */
    float r = x - (float)q * PI_2;
    
    /* Core polynomials */
    float r2 = r * r;
    float r4 = r2 * r2;
    
    float sin_r = r * (SIN_C0 + SIN_C1*r2 + SIN_C2*r4 + SIN_C3*r4*r2);
    float cos_r = COS_C0 + COS_C1*r2 + COS_C2*r4 + COS_C3*r4*r2;
    
    /* Reconstruct based on quadrant */
    q = q & 3;  /* mod 4 */
    switch (q) {
        case 0: *s =  sin_r; *c =  cos_r; break;
        case 1: *s =  cos_r; *c = -sin_r; break;
        case 2: *s = -sin_r; *c = -cos_r; break;
        case 3: *s = -cos_r; *c =  sin_r; break;
    }
}

static inline float atan2_fast(float y, float x) {
    /* Handle special cases */
    if (x == 0.0f) {
        if (y > 0.0f) return PI_2;
        if (y < 0.0f) return -PI_2;
        return 0.0f;  /* undefined, return 0 */
    }
    
    float ax = x >= 0 ? x : -x;
    float ay = y >= 0 ? y : -y;
    
    /* Compute atan(|y/x|) or atan(|x/y|) depending on which is smaller */
    float a, t;
    int swap = ay > ax;
    if (swap) {
        t = ax / ay;
    } else {
        t = ay / ax;
    }
    
    /* Core polynomial for atan on [0, 1] */
    float t2 = t * t;
    float t4 = t2 * t2;
    float t8 = t4 * t4;
    a = t * (ATAN_C0 + ATAN_C1*t2 + ATAN_C2*t4 + ATAN_C3*t4*t2 + ATAN_C4*t8);
    
    /* Adjust for quadrant */
    if (swap) a = PI_2 - a;
    if (x < 0) a = PI - a;
    if (y < 0) a = -a;
    
    return a;
}
""")

##Output

# ──────────────────────────────────────────────────────────────────────
# [sin(x)] on [-π/4, π/4]
# Formula: sin(x) ≈ x * (c0 + c1*x² + c2*x⁴ + c3*x⁶)
# ──────────────────────────────────────────────────────────────────────
# Taylor:    [ 1.00000000e+00 -1.66666667e-01  8.33333333e-03 -1.98412698e-04]
# Taylor error: 0.000051%
# Optimized: [ 1.00000002e+00 -1.66667186e-01  8.33525463e-03 -1.98607875e-04]
# Optimized error: 0.000014%
# Improvement: 3.6x better

# ──────────────────────────────────────────────────────────────────────
# [cos(x)] on [-π/4, π/4]
# Formula: cos(x) ≈ c0 + c1*x² + c2*x⁴ + c3*x⁶
# ──────────────────────────────────────────────────────────────────────
# Taylor:    [ 1.         -0.5         0.04166667 -0.00138889]
# Taylor error: 0.000506%
# Optimized: [ 1.00000023 -0.50000436  0.0416777  -0.00138201]
# Optimized error: 0.000030%
# Improvement: 17.0x better

# ──────────────────────────────────────────────────────────────────────
# [tan(x)] on [-π/4, π/4]
# Formula: tan(x) ≈ x * (c0 + c1*x² + c2*x⁴ + c3*x⁶ + c4*x⁸)
# ──────────────────────────────────────────────────────────────────────
# Taylor:    [1.         0.33333333 0.13333333 0.05396825 0.02186949]
# Taylor error: 0.082898%
# Optimized: [1.00001374 0.33250106 0.14157085 0.0265363  0.0555204 ]
# Optimized error: 0.001379%
# Improvement: 60.1x better

# ──────────────────────────────────────────────────────────────────────
# [atan(x)] on [-1, 1]
# Formula: atan(x) ≈ x * (c0 + c1*x² + c2*x⁴ + c3*x⁶ + c4*x⁸)
# ──────────────────────────────────────────────────────────────────────
# Taylor:    [ 1.         -0.33333333  0.2        -0.14285714  0.11111111]
# Taylor error: 6.305388%
# Optimized: [ 0.99902484, -0.33017794,  0.21883819, -0.17810562,  0.07658158]
# Optimized error: 0.013791%
# Improvement: 457.2x better

# ──────────────────────────────────────────────────────────────────────
# [atan(x)] on [-1, 1]  (evolutionary + Nelder Mead refinement)
# Formula: atan(x) ≈ x * (c0 + c1*x² + c2*x⁴ + c3*x⁶ + c4*x⁸)
# ──────────────────────────────────────────────────────────────────────
# Taylor:    [ 1.         -0.33333333  0.2        -0.14285714  0.11111111]
# Taylor error: 6.305388%
# Optimized: [ 1.00002819, -0.33301428,  0.190849, -0.10012556,  0.02770223]
# Optimized error: 0.0057%
# Improvement: 1115x  better

# ──────────────────────────────────────────────────────────────────────
# [asin(x)] on [-0.5, 0.5]
# Formula: asin(x) ≈ x + x³*(c0 + c1*x² + c2*x⁴ + c3*x⁶)
# ──────────────────────────────────────────────────────────────────────
# Taylor:    [0.16666667 0.075      0.04464286 0.03038194]
# Taylor error: 0.002595%
# Optimized: [0.1666788  0.07487722 0.04353529 0.04288391]
# Optimized error: 0.000034%
# Improvement: 76.0x better
