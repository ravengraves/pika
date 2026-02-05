#!/usr/bin/env python3
"""
Magic Constant Finder - Optimal IEEE 754 bit-trick constants for fast float math.

Usage: python find_magic_constants.py
"""

import numpy as np

# Test values: 10k floats from 0.001 to 1000
X = np.logspace(-3, 3, 10000, dtype=np.float32)
IX = X.view(np.int32)  # Integer bit representation


def find_best(formula, exact, center, radius=300000, coarse=500, fine=200):
    """Two-phase grid search: coarse scan, then fine refinement."""
    
    def max_error(C):
        y = formula(C).view(np.float32)
        return np.nanmax(np.abs(y - exact) / np.abs(exact), axis=1)
    
    # Coarse search
    C = np.arange(center - radius, center + radius, coarse, dtype=np.int32)
    best = C[np.argmin(max_error(C))]
    
    # Fine search
    C = np.arange(best - fine, best + fine + 1, dtype=np.int32)
    errors = max_error(C)
    idx = np.argmin(errors)
    
    return int(C[idx]), float(errors[idx])


if __name__ == "__main__":
    L = 2**23  # Mantissa bits
    
    # 1/x: y = C - I(x)
    c, e = find_best(lambda C: C[:, None] - IX, 1/X, int(2*L*126.95))
    print(f"RCP_MAGIC   = 0x{c:08X}  // 1/x,   {e:.4%} error")
    
    # 1/√x: y = C - (I(x) >> 1)
    c, e = find_best(lambda C: C[:, None] - (IX >> 1), 1/np.sqrt(X), int(1.5*L*126.96))
    print(f"RSQRT_MAGIC = 0x{c:08X}  // 1/√x,  {e:.4%} error")
    
    # √x: y = (I(x) >> 1) + C
    c, e = find_best(lambda C: (IX >> 1) + C[:, None], np.sqrt(X), int(0.5*L*126.93))
    print(f"SQRT_MAGIC  = 0x{c:08X}  // √x,    {e:.4%} error")


# note, I made this more complex to also consider the number of newton steps we were taking
# I also found a better number using an evolutionary algorithm approach for 1/x
