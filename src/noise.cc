#include "noise.h"
#include "rand.h"
#include <limits.h>
#include <sys/types.h>
#define FASTFLOOR(x) ( ((x)>0) ? ((int)x) : (((int)x)-1) )

/*
 * ---------------------------------------------------------------------
 * Static data
 */

/*
 * Permutation table. This is just a random jumble of all numbers 0-255,
 * repeated twice to avoid wrapping the index at 255 for each lookup.
 * This needs to be exactly the same for all instances on all platforms,
 * so it's easiest to just keep it as static explicit data.
 * This also removes the need for any initialisation of this class.
 *
 * Note that making this an int[] instead of a char[] might make the
 * code run faster on platforms with a high penalty for unaligned single
 * byte addressing. Intel x86 is generally single-byte-friendly, but
 * some other CPUs are faster with 4-aligned reads.
 * However, a char[] is smaller, which avoids cache trashing, and that
 * is probably the most important aspect on most architectures.
 * This array is accessed a *lot* by the noise functions.
 * A vector-valued noise over 3D accesses it 96 times, and a
 * float-valued 4D noise 64 times. We want this to fit in the cache!
 */
static unsigned char perm[512] = {151,160,137,91,90,15,
  131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
  190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
  88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
  77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
  102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
  135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
  5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
  223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
  129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
  251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
  49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
  138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180,
  151,160,137,91,90,15,
  131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
  190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
  88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
  77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
  102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
  135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
  5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
  223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
  129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
  251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
  49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
  138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
};

/*
 * ---------------------------------------------------------------------
 */

/*
 * Helper functions to compute gradients-dot-residualvectors (1D to 4D)
 * Note that these generate gradients of more than unit length. To make
 * a close match with the value range of classic Perlin noise, the final
 * noise values need to be rescaled to fit nicely within [-1,1].
 * (The simplex noise functions as such also have different scaling.)
 * Note also that these noise functions are the most practical and useful
 * signed version of Perlin noise. To return values according to the
 * RenderMan specification from the SL noise() and pnoise() functions,
 * the noise values need to be scaled and offset to [0,1], like this:
 * float SLnoise = (SimplexNoise1234::noise(x,y,z) + 1.0) * 0.5;
 */

static float  grad1( int hash, float x ) {
    int h = hash & 15;
    float grad = 1.0f + (h & 7);   /* Gradient value 1.0, 2.0, ..., 8.0 */
    if (h&8) grad = -grad;         /* Set a random sign for the gradient */
    return ( grad * x );           /* Multiply the gradient with the distance */
}

static float  grad2( int hash, float x, float y ) {
    int h = hash & 7;      /* Convert low 3 bits of hash code */
    float u = h<4 ? x : y;  /* into 8 simple gradient directions, */
    float v = h<4 ? y : x;  /* and compute the dot product with (x,y). */
    return ((h&1)? -u : u) + ((h&2)? -2.0f*v : 2.0f*v);
}


void
_noise_init_perm(unsigned long seed)
{
    init_genrand(seed);

    for (int x=0; x<256; x++) {
        perm[x] = x;
    }

    size_t j,i;
    int t;
    for (i = 0; i < 255; i++) {
      j = i + genrand_int32() / (UINT_MAX / (256-i) + 1);
      t = perm[j];
      perm[j] = perm[i];
      perm[i] = t;
    }

    for (int x=0; x<256; x++)
        perm[x+256] = perm[x];
}

/* 1D simplex noise */
float
_noise1 (float x)
{
  int i0 = FASTFLOOR(x);
  int i1 = i0 + 1;
  float x0 = x - i0;
  float x1 = x0 - 1.0f;
  float t1 = 1.0f - x1*x1;
  float n0, n1;

  float t0 = 1.0f - x0*x0;
/*  if(t0 < 0.0f) t0 = 0.0f; // this never happens for the 1D case */
  t0 *= t0;
  n0 = t0 * t0 * grad1(perm[i0 & 0xff], x0);

/*  if(t1 < 0.0f) t1 = 0.0f; // this never happens for the 1D case */
  t1 *= t1;
  n1 = t1 * t1 * grad1(perm[i1 & 0xff], x1);
  /* The maximum value of this noise is 8*(3/4)^4 = 2.53125 */
  /* A factor of 0.395 would scale to fit exactly within [-1,1], but */
  /* we want to match PRMan's 1D noise, so we scale it down some more. */
  return 0.25f * (n0 + n1);
}

/* 2D simplex noise */
float
_noise2 (float x, float y)
{
#define F2 0.366025403f /* F2 = 0.5*(sqrt(3.0)-1.0) */
#define G2 0.211324865f /* G2 = (3.0-Math.sqrt(3.0))/6.0 */

    float n0, n1, n2; /* Noise contributions from the three corners */

    /* Skew the input space to determine which simplex cell we're in */
    float s = (x+y)*F2; /* Hairy factor for 2D */
    float xs = x + s;
    float ys = y + s;
    int i = FASTFLOOR(xs);
    int j = FASTFLOOR(ys);

    float t = (float)(i+j)*G2;
    float X0 = i-t; /* Unskew the cell origin back to (x,y) space */
    float Y0 = j-t;
    float x0 = x-X0; /* The x,y distances from the cell origin */
    float y0 = y-Y0;

    float x1, y1, x2, y2;
    int ii, jj;
    float t0, t1, t2;

    /* For the 2D case, the simplex shape is an equilateral triangle. */
    /* Determine which simplex we are in. */
    int i1, j1; /* Offsets for second (middle) corner of simplex in (i,j) coords */
    if(x0>y0) {i1=1; j1=0;} /* lower triangle, XY order: (0,0)->(1,0)->(1,1) */
    else {i1=0; j1=1;}      /* upper triangle, YX order: (0,0)->(0,1)->(1,1) */

    /* A step of (1,0) in (i,j) means a step of (1-c,-c) in (x,y), and */
    /* a step of (0,1) in (i,j) means a step of (-c,1-c) in (x,y), where */
    /* c = (3-sqrt(3))/6 */

    x1 = x0 - i1 + G2; /* Offsets for middle corner in (x,y) unskewed coords */
    y1 = y0 - j1 + G2;
    x2 = x0 - 1.0f + 2.0f * G2; /* Offsets for last corner in (x,y) unskewed coords */
    y2 = y0 - 1.0f + 2.0f * G2;

    /* Wrap the integer indices at 256, to avoid indexing perm[] out of bounds */
    ii = i % 256;
    jj = j % 256;

    /* Calculate the contribution from the three corners */
    t0 = 0.5f - x0*x0-y0*y0;
    if(t0 < 0.0f) n0 = 0.0f;
    else {
      t0 *= t0;
      n0 = t0 * t0 * grad2(perm[ii+perm[jj]], x0, y0);
    }

    t1 = 0.5f - x1*x1-y1*y1;
    if(t1 < 0.0f) n1 = 0.0f;
    else {
      t1 *= t1;
      n1 = t1 * t1 * grad2(perm[ii+i1+perm[jj+j1]], x1, y1);
    }

    t2 = 0.5f - x2*x2-y2*y2;
    if(t2 < 0.0f) n2 = 0.0f;
    else {
      t2 *= t2;
      n2 = t2 * t2 * grad2(perm[ii+1+perm[jj+1]], x2, y2);
    }

    /* Add contributions from each corner to get the final noise value. */
    /* The result is scaled to return values in the interval [-1,1]. */
    return 40.0f * (n0 + n1 + n2); /* TODO: The scale factor is preliminary! */
}
