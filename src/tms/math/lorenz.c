#include "lorenz.h"

#define SIGMA 10.0f
#define RAYLEIGH 28.f
#define PRANDTL 8.f/3.f

void
tmath_deriv_lorenz(float time, float *in, float *out)
{
    out[0] = -SIGMA * in[0] + SIGMA*in[1];
    out[1] = -in[0] * in[2] + RAYLEIGH*in[0]-in[1];
    out[2] = in[0]*in[1] - PRANDTL*in[2];
}

