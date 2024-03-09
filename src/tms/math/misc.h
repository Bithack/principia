#ifndef _MATH_MISC__H_
#define _MATH_MISC__H_

#include <stdlib.h>
#include <math.h>

#ifdef __cplusplus
extern "C"
{
#endif

static inline float trandf(float min, float max)
{
    return min + rand() / (float)RAND_MAX * ( max - min );
}

static inline float twrapf(float x, float min, float max)
{
    float range = max - min;

    if (x >= min) {
        if (x <= max) {
            return x;
        } else if (x < max + range) {
            return x - range;
        }
    } else if (x >= min - range) {
        return x + range;
    }

    return fmodf(x - min, range) + min;
}

static inline float tclampf(float d, float min, float max)
{
    const float t = d < min ? min : d;
    return t > max ? max : t;
}

static inline int tclampi(int d, int min, int max)
{
    const int t = d < min ? min : d;
    return t > max ? max : t;
}

static inline float tmath_adist(float a, float b)
{
    const float PI2 = M_PI*2.f;

    int i, x;
    float d, dn;
    float n = fmod(b, PI2);
    float m = roundf(a / PI2) * PI2;

    float t[3] = {
        m + n,
        m + n - PI2,
        m + n + PI2
    };

    for (x=1, dn = fabsf(a-t[0]), i=0; x<3; x++) {
        if ((d = fabsf(a-t[x])) < dn) {
            i = x;
            dn = d;
        }
    }

    return t[i]-a;
}

#ifdef TMS_FAST_MATH

void tmath_sincos(float x, float *r0, float *r1);
float tmath_atan2(float y, float x);
float tmath_sqrt(float x);

#else

#define tmath_sincos(x,y,z) sincosf(x,y,z)
#define tmath_atan2(y,x) atan2f(y,x)
#define tmath_sqrt(x) sqrtf(x)

#endif

static inline float tmath_atan2add(float y, float x)
{
    float a = tmath_atan2(y,x);
    if (a < 0.f) a += M_PI*2.f;
    return a;
}

static inline double tmath_logstep(double in, double min_value, double max_value)
{
    double min_position = 0.f;
    double max_position = 1.f;

    min_value = log(min_value);
    max_value = log(max_value);

    /* Calculate adjustment factor */
    double scale = (max_value-min_value) / (max_position-min_position);

    return exp(min_value + scale*(in-min_position));
}

static inline double tmath_logstep_position(double value, double min_value, double max_value)
{
    double min_position = 0.f;
    double max_position = 1.f;

    min_value = log(min_value);
    max_value = log(max_value);

    /* Calculate adjustment factor */
    double scale = (max_value-min_value) / (max_position-min_position);

    return (log(value)-min_value) / scale + min_position;
}

#define DEGTORAD 0.0174532925199432957f
#define RADTODEG 57.295779513082320876f

#ifdef __cplusplus
}
#endif

#endif
