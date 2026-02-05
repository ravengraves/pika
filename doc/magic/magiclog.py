#!/usr/bin/env python3
"""
Evolutionary Polynomial Coefficient Optimizer for fast exp/log approximations.

Optimizes coefficients for:
  exp(r) ≈ c0 + c1*r + c2*r² + c3*r³ + c4*r⁴ + c5*r⁵   (r ∈ [-ln2/2, ln2/2])
  ln(m)  ≈ 2*(c0*t + c1*t³ + c2*t⁵ + c3*t⁷)            (t = (m-1)/(m+1), m ∈ [1,2))

Usage: python optimize_exp_log.py
"""

import numpy as np

rng = np.random.default_rng(42)

# =============================================================================
# Test ranges
# =============================================================================

LN2 = np.float32(np.log(2))

# exp(r): r ∈ [-ln2/2, ln2/2]
R_EXP = np.linspace(-LN2/2, LN2/2, 5000, dtype=np.float32)
EXACT_EXP = np.exp(R_EXP)

# ln(m): m ∈ [1, 2), t = (m-1)/(m+1)
M_LOG = np.linspace(1.0001, 1.9999, 5000, dtype=np.float32)
T_LOG = (M_LOG - 1) / (M_LOG + 1)
EXACT_LOG = np.log(M_LOG)


# =============================================================================
# Polynomial evaluation
# =============================================================================

def eval_exp_poly(r, c):
    """exp(r) ≈ c0 + c1*r + c2*r² + c3*r³ + c4*r⁴ + c5*r⁵"""
    r2 = r * r
    r4 = r2 * r2
    return c[0] + c[1]*r + c[2]*r2 + c[3]*r2*r + c[4]*r4 + c[5]*r4*r


def eval_log_poly(t, c):
    """ln(m) ≈ 2*(c0*t + c1*t³ + c2*t⁵ + c3*t⁷)"""
    t2 = t * t
    t3 = t * t2
    return 2 * (c[0]*t + c[1]*t3 + c[2]*t3*t2 + c[3]*t3*t2*t2)


def exp_max_error(c):
    """Max relative error for exp polynomial."""
    approx = eval_exp_poly(R_EXP, c.astype(np.float32))
    return np.max(np.abs(approx - EXACT_EXP) / EXACT_EXP)


def log_max_error(c):
    """Max relative error for log polynomial."""
    approx = eval_log_poly(T_LOG, c.astype(np.float32))
    return np.max(np.abs(approx - EXACT_LOG) / np.abs(EXACT_LOG))


# =============================================================================
# Evolutionary optimization
# =============================================================================

def evolve(error_fn, init_coeffs, pop_size=512, generations=200, 
           sigma=0.01, decay=0.9, elite_frac=0.1):
    """
    Evolutionary optimization for polynomial coefficients.
    
    Args:
        error_fn: Function that takes coefficients and returns max error
        init_coeffs: Starting coefficients (e.g., Taylor)
        pop_size: Population size
        generations: Number of generations
        sigma: Initial mutation standard deviation
        decay: Sigma decay per generation
        elite_frac: Fraction of population to keep as elite
    
    Returns:
        (best_coeffs, best_error, history)
    """
    n_coeffs = len(init_coeffs)
    n_elite = max(1, int(pop_size * elite_frac))
    
    # Initialize population around starting coefficients
    pop = rng.normal(init_coeffs, sigma * np.abs(init_coeffs) + 1e-8, 
                     (pop_size, n_coeffs))
    
    best_c = init_coeffs.copy()
    best_e = error_fn(init_coeffs)
    history = [(0, best_e, sigma)]
    
    for gen in range(generations):
        # Evaluate fitness
        errors = np.array([error_fn(c) for c in pop])
        
        # Update best
        idx = np.argmin(errors)
        if errors[idx] < best_e:
            best_e = errors[idx]
            best_c = pop[idx].copy()
            history.append((gen + 1, best_e, sigma))
        
        # Select elite
        elite_idx = np.argsort(errors)[:n_elite]
        elite = pop[elite_idx]
        
        # Decay sigma
        sigma = sigma * decay
        
        # Generate new population
        new_pop = np.zeros((pop_size, n_coeffs))
        for i in range(pop_size):
            parent = elite[i % n_elite]
            mutation = rng.normal(0, sigma * np.abs(parent) + 1e-10, n_coeffs)
            new_pop[i] = parent + mutation
        
        # Elitism: keep best ever
        new_pop[0] = best_c
        pop = new_pop
    
    return best_c, best_e, history


# =============================================================================
# Main
# =============================================================================

if __name__ == "__main__":
    print("=" * 70)
    print("EVOLUTIONARY POLYNOMIAL COEFFICIENT OPTIMIZER")
    print("=" * 70)
    
    # -------------------------------------------------------------------------
    # Optimize exp coefficients
    # -------------------------------------------------------------------------
    print("\n[exp(r) polynomial, r ∈ [-ln2/2, ln2/2]]")
    print("Formula: exp(r) ≈ c0 + c1*r + c2*r² + c3*r³ + c4*r⁴ + c5*r⁵\n")
    
    taylor_exp = np.array([1.0, 1.0, 0.5, 1/6, 1/24, 1/120])
    print(f"Taylor coefficients:    {taylor_exp}")
    print(f"Taylor max error:       {exp_max_error(taylor_exp):.6%}")
    
    print("\nRunning evolutionary optimization...")
    opt_exp, err_exp, hist_exp = evolve(exp_max_error, taylor_exp, 
                                         generations=1024, sigma=0.25)
    
    print(f"Optimized coefficients: {opt_exp}")
    print(f"Optimized max error:    {err_exp:.6%}")
    print(f"Improvement:            {exp_max_error(taylor_exp)/err_exp:.1f}x better")
    
    # -------------------------------------------------------------------------
    # Optimize log coefficients
    # -------------------------------------------------------------------------
    print("\n" + "-" * 70)
    print("\n[ln(m) polynomial, m ∈ [1,2), t = (m-1)/(m+1)]")
    print("Formula: ln(m) ≈ 2*(c0*t + c1*t³ + c2*t⁵ + c3*t⁷)\n")
    
    taylor_log = np.array([1.0, 1/3, 1/5, 1/7])
    print(f"Taylor coefficients:    {taylor_log}")
    print(f"Taylor max error:       {log_max_error(taylor_log):.6%}")
    
    print("\nRunning evolutionary optimization...")
    opt_log, err_log, hist_log = evolve(log_max_error, taylor_log,
                                         generations=1024, sigma=0.25)
    
    print(f"Optimized coefficients: {opt_log}")
    print(f"Optimized max error:    {err_log:.6%}")
    print(f"Improvement:            {log_max_error(taylor_log)/err_log:.1f}x better")
    
    # -------------------------------------------------------------------------
    # Output C code
    # -------------------------------------------------------------------------
    print("\n" + "=" * 70)
    print("C CODE OUTPUT")
    print("=" * 70)
    
    print(f"""
/* Optimized polynomial coefficients for fast exp/log approximations.
 * Generated by evolutionary optimization.
 * 
 * exp: {exp_max_error(taylor_exp):.6%} → {err_exp:.6%} ({exp_max_error(taylor_exp)/err_exp:.1f}x better)
 * log: {log_max_error(taylor_log):.6%} → {err_log:.6%} ({log_max_error(taylor_log)/err_log:.1f}x better)
 */

/* exp(r) for r ∈ [-ln2/2, ln2/2]: exp(r) ≈ c0 + c1*r + c2*r² + c3*r³ + c4*r⁴ + c5*r⁵ */
#define EXP_C0  {opt_exp[0]:.10f}f  /* Taylor: 1.0 */
#define EXP_C1  {opt_exp[1]:.10f}f  /* Taylor: 1.0 */
#define EXP_C2  {opt_exp[2]:.10f}f  /* Taylor: 0.5 */
#define EXP_C3  {opt_exp[3]:.10f}f  /* Taylor: 1/6 ≈ 0.1667 */
#define EXP_C4  {opt_exp[4]:.10f}f  /* Taylor: 1/24 ≈ 0.0417 */
#define EXP_C5  {opt_exp[5]:.10f}f  /* Taylor: 1/120 ≈ 0.0083 */

/* ln(m) for m ∈ [1,2), t=(m-1)/(m+1): ln(m) ≈ 2*(c0*t + c1*t³ + c2*t⁵ + c3*t⁷) */
#define LOG_C0  {opt_log[0]:.10f}f  /* Taylor: 1.0 */
#define LOG_C1  {opt_log[1]:.10f}f  /* Taylor: 1/3 ≈ 0.3333 */
#define LOG_C2  {opt_log[2]:.10f}f  /* Taylor: 1/5 = 0.2 */
#define LOG_C3  {opt_log[3]:.10f}f  /* Taylor: 1/7 ≈ 0.1429 */

static inline float exp_fast(float x) {{
    /* Range reduction: x = k*ln2 + r, r ∈ [-ln2/2, ln2/2] */
    int32_t k = (int32_t)(x * 1.4426950408889634f + (x >= 0 ? 0.5f : -0.5f));
    float r = x - (float)k * 0.6931471805599453f;
    
    /* Polynomial approximation */
    float r2 = r * r;
    float p = EXP_C5;
    p = p * r + EXP_C4;
    p = p * r + EXP_C3;
    p = p * r + EXP_C2;
    p = p * r + EXP_C1;
    p = p * r + EXP_C0;
    
    /* Reconstruction: exp(x) = exp(r) * 2^k */
    union {{ float f; int32_t i; }} u;
    u.i = (k + 127) << 23;
    return p * u.f;
}}

static inline float log_fast(float x) {{
    /* Extract exponent and mantissa: x = m * 2^e, m ∈ [1,2) */
    union {{ float f; uint32_t u; }} v = {{x}};
    int32_t e = (int32_t)((v.u >> 23) & 0xff) - 127;
    v.u = (v.u & 0x007fffff) | 0x3f800000;
    float m = v.f;
    
    /* Transform: t = (m-1)/(m+1) ∈ [0, 1/3] */
    float t = (m - 1.0f) / (m + 1.0f);
    float t2 = t * t;
    
    /* Polynomial approximation */
    float a = LOG_C3;
    a = a * t2 + LOG_C2;
    a = a * t2 + LOG_C1;
    a = a * t2 + LOG_C0;
    
    /* Reconstruction: ln(x) = ln(m) + e*ln(2) */
    return 2.0f * t * a + (float)e * 0.6931471805599453f;
}}
""")

# /* Optimized polynomial coefficients for fast exp/log approximations.
#  * Generated by evolutionary optimization.
#  *
#  * exp: 0.000337% → 0.000064% (5.3x better)
#  * log: 0.001790% → 0.000158% (11.4x better)
#  */

# /* exp(r) for r ∈ [-ln2/2, ln2/2]: exp(r) ≈ c0 + c1*r + c2*r² + c3*r³ + c4*r⁴ + c5*r⁵ */
# #define EXP_C0  0.9999995106f  /* Taylor: 1.0 */
# #define EXP_C1  1.0000011034f  /* Taylor: 1.0 */
# #define EXP_C2  0.5000197248f  /* Taylor: 0.5 */
# #define EXP_C3  0.1666572048f  /* Taylor: 1/6 ≈ 0.1667 */
# #define EXP_C4  0.0416685804f  /* Taylor: 1/24 ≈ 0.0417 */
# #define EXP_C5  0.0083365676f  /* Taylor: 1/120 ≈ 0.0083 */

# /* ln(m) for m ∈ [1,2), t=(m-1)/(m+1): ln(m) ≈ 2*(c0*t + c1*t³ + c2*t⁵ + c3*t⁷) */
# #define LOG_C0  1.0000011730f  /* Taylor: 1.0 */
# #define LOG_C1  0.3331588525f  /* Taylor: 1/3 ≈ 0.3333 */
# #define LOG_C2  0.2032034977f  /* Taylor: 1/5 = 0.2 */
# #define LOG_C3  0.1397988930f  /* Taylor: 1/7 ≈ 0.1429 */

# /* Optimized polynomial coefficients for fast exp/log approximations.
#  * Generated by evolutionary optimization.
#  *
#  * exp: 0.000337% → 0.000059% (5.7x better)
#  * log: 0.001790% → 0.000267% (6.7x better)
#  */

# /* exp(r) for r ∈ [-ln2/2, ln2/2]: exp(r) ≈ c0 + c1*r + c2*r² + c3*r³ + c4*r⁴ + c5*r⁵ */
# #define EXP_C0  0.9999995571f  /* Taylor: 1.0 */
# #define EXP_C1  1.0000005574f  /* Taylor: 1.0 */
# #define EXP_C2  0.5000163151f  /* Taylor: 0.5 */
# #define EXP_C3  0.1666656788f  /* Taylor: 1/6 ≈ 0.1667 */
# #define EXP_C4  0.0416993585f  /* Taylor: 1/24 ≈ 0.0417 */
# #define EXP_C5  0.0083249631f  /* Taylor: 1/120 ≈ 0.0083 */

# /* ln(m) for m ∈ [1,2), t=(m-1)/(m+1): ln(m) ≈ 2*(c0*t + c1*t³ + c2*t⁵ + c3*t⁷) */
# #define LOG_C0  0.9999976501f  /* Taylor: 1.0 */
# #define LOG_C1  0.3333371190f  /* Taylor: 1/3 ≈ 0.3333 */
# #define LOG_C2  0.2015250302f  /* Taylor: 1/5 = 0.2 */
# #define LOG_C3  0.1421099340f  /* Taylor: 1/7 ≈ 0.1429 */