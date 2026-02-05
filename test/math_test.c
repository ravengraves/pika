/*
 * Basic accuracy tests for the math functions implemented in stdlib/math.pka.
 *
 * The test compiles the generated C file from `stdlib/math.pka` and
 * compares each function's output against the equivalent routine in the
 * system <math.h>.
 */

#include <assert.h>
#include <math.h>
#include <stdio.h>

/* Declarations for functions generated from math.pka */
float math_recip(float);
float math_recip_fast(float);
float math_rsqrt(float);
float math_rsqrt_fast(float);
float math_sqrt(float);
float math_sqrt_fast(float);
float math_exp(float);
float math_log(float);
// Trig function prototypes
float math_sin(float);
float math_cos(float);
float math_tan(float);
float math_atan(float);
float math_atan2(float, float);

// Helper to compare close values within a tolerance.
int close_to(float a, float b, float eps) {
	return fabsf(a - b) <= eps;
}

int main(void) {
	/* A set of inputs covering positive, negative and extreme values */
	float vals[5000] = {0};
	for (int i=0; i<5000; i+= 1) {
		vals[i] = ((float)i - 2500.0)/2.5;
	}
	const size_t n = sizeof(vals)/sizeof(vals[0]);
	const float eps = 1e-4f; // tolerance for float comparisons

	for (size_t i = 0; i < n; ++i) {
		float v = vals[i];
		if (v != 0.0f) {
			// printf("computed %f, expected %f\n", math_recip_fast(v), 1.0f/v);
			assert(close_to(math_recip(v), 1.0f/v, eps* fabs(1.0f/v)));
			assert(close_to(math_recip_fast(v), 1.0f/v, 0.07* fabs(1.0f/v) ));
		}
		if (v >= 0.0f) {
			// printf("computed %f, expected %f\n", math_sqrt_fast(v), sqrtf(v));
			assert(close_to(math_sqrt(v), sqrtf(v), eps*sqrtf(v)));
			assert(close_to(math_sqrt_fast(v), sqrtf(v), eps + 1000*eps * sqrtf(v)));
			// rsqrt approximates 1/sqrt(v)
			assert(close_to(math_rsqrt(v), 1.0f/sqrtf(v), eps*1.0f/sqrtf(v)));
			assert(close_to(math_rsqrt_fast(v), 1.0f/sqrtf(v), 0.07*1.0f/sqrtf(v)));
		}
		if (fabs(v) < 86) {
			// printf("computed %f, expected %f\n", math_log(v), logf(v));
			assert(close_to(math_exp(v), expf(v), eps*expf(v)));
		}
		if (v > 0.0f) {
			// printf("computed %f, expected %f\n", math_log(v), logf(v));
			assert(close_to(math_log(v), logf(v), eps*fabs(logf(v))));
		}
	}

	/* Now test trig functions */
	const float pi = 3.14159265358979323846f;
	const float trig_eps = 5e-5f;
	/* range from -2π to 2π */
	const float trig_vals[] = {
		-2.0f * pi, -1.8f * pi, -pi, -0.1f * pi, -0.5f * pi,
		-0.25f * pi, 0.0f,
		0.25f * pi, 0.5f * pi, 0.75f * pi, pi, 1.5f * pi, 2.0f * pi
	};
	const size_t tv = sizeof(trig_vals)/sizeof(trig_vals[0]);
	for (size_t j = 0; j < tv; ++j) {
		float v = trig_vals[j];
		/* compare sin */
		assert(close_to(math_sin(v), sinf(v), trig_eps));
		/* compare cos */
		assert(close_to(math_cos(v), cosf(v), trig_eps));
		/* compare tan where defined */
		if (fabsf(cosf(v)) > 1e-3f) {
			assert(close_to(math_tan(v), tanf(v), trig_eps));
		}
		/* compare atan */
		assert(close_to(math_atan(v), atanf(v), trig_eps));
	}
	/* atan2 tests */
	struct { float y; float x; } atan2_vals[] = {
		{1.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f}, {-1.0f, 1.0f},
		{1.0f, -1.0f}, {0.0f, -1.0f}, {-1.0f, -1.0f}
	};
	const size_t atan2_n = sizeof(atan2_vals)/sizeof(atan2_vals[0]);
	for (size_t k = 0; k < atan2_n; ++k) {
		float y = atan2_vals[k].y;
		float x = atan2_vals[k].x;
		assert(close_to(math_atan2(y, x), atan2f(y, x), trig_eps));
	}
	printf("All math accuracy tests passed.\n");
	return 0;
}
