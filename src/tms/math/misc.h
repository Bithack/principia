#ifndef _MATH_MISC__H_
#define _MATH_MISC__H_

#include <stdlib.h>
#include <math.h>
#include <tms/util/util.h>

#ifdef __cplusplus
extern "C"
{
#endif

static inline float tms_modf(float a, float b)
{
    return a - b*(floorf(a/b));
}

#define RANDF_MAX 2147483647.f

static inline float trandf(float min, float max)
{
    return min+(float)(rand())/((float)(RANDF_MAX/(max-min)));
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

static inline float tclampf(float x, float a, float b)
{
    if (x < a) x = a;
    else if (x > b) x = b;
    return x;
}

static inline double tclamp(double x, double a, double b)
{
    if (x < a) x = a;
    else if (x > b) x = b;
    return x;
}

static inline int tclampi(int x, int a, int b)
{
    if (x < a) x = a;
    else if (x > b) x = b;
    return x;
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

#if defined(TMS_BACKEND_ANDROID)
static double tmath_log2(double n)
{
    return log(n) / log(2.);
}
#else
#define tmath_log2(n) log2(n)
#endif

#ifdef TMS_FAST_MATH
void tmath_sincos(float x, float *r0, float *r1);
float tmath_sin(float x);
static inline float tmath_cos(float x){return tmath_sin(x+M_PI_2);};
float tmath_pow(float x, float n);
float tmath_atan2(float y, float x);
float tmath_sqrt(float x);
#else
#ifdef TMS_BACKEND_MOBILE
static inline void tmath_sincos(float x, float *y, float *z) {
    *y = sinf(x);
    *z = cosf(x);
}
//#define tmath_sincos(x,y,z) do {*(y) = sinf(x); *(z) = cosf(x);}while(0)
//#define tmath_sincos(x,y,z) __sincosf(x,y,z)
#else
#define tmath_sincos(x,y,z) sincosf(x,y,z)
#endif
#define tmath_sin(x) sinf(x)
#define tmath_cos(x) cosf(x)
#define tmath_pow(x,n) powf(x,n)
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
