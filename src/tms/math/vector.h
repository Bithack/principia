#ifndef _TMATH_VECTOR__H_
#define _TMATH_VECTOR__H_

#include <math.h>

/**
 * 2D vector.
 * Can be accessed using various naming conventions depending on intention. For
 * texture coordinates, V.u and V.v, and for space coordinates V.x and V.y.
 **/
typedef struct tvec2 {
    union {float x; float r; float s; float u; float w;};
    union {float y; float g; float t; float v; float h;};
} tvec2;

/**
 * 3D vector.
 * Can be accessed using various naming conventions depending on intention.
 * For example: x y z, r g b, s t p.
 **/
typedef struct tvec3 {
    union {float x; float r; float s;};
    union {float y; float g; float t;};
    union {float z; float b; float p;};
} tvec3;

/**
 * 4-float general purpose vector. Use this for plane equations and homogenous 3d coordinates.
 * Can be accessed using various naming conventions, x y z w, r g b a, etc.
 **/
typedef struct tvec4 {
    union {float x; float r; float s;};
    union {float y; float g; float t;};
    union {float z; float b; float p;};
    union {float w; float a; float q;};
} tvec4;

/**
 * @relates tvec3 @{
 **/
#define TVEC3_INLINE(v) (v).x, (v).y, (v).z
#define TVEC3_INLINE_N(v) -(v).x, -(v).y, -(v).z
#define TVEC3_INLINE_M(v, m) (v).x*m, (v).y*m, (v).z*m
static inline void  tvec3_normalize(tvec3 *v);
static inline float tvec3_magnitude(tvec3 *v);
static inline float tvec3_dot(tvec3 *v1, tvec3 *v2);
static inline void  tvec3_cross(tvec3 *r, tvec3 v1, tvec3 v2);
void  tvec3_mul_mat3(tvec3 *v, float *m);
void  tvec3_project_mat4(tvec3 *v, float *m);

static inline tvec3 tvec3f(float x, float y, float z)
{
    struct tvec3 r = {
        x, y, z
    };

    return r;
    //return (tvec3){x,y,z};
}

static inline void tvec3_add(tvec3 *a, tvec3 *b)
{
    a->x += b->x;
    a->y += b->y;
    a->z += b->z;
}

static inline void tvec3_sub(tvec3 *a, tvec3 *b)
{
    a->x -= b->x;
    a->y -= b->y;
    a->z -= b->z;
}

static inline void tvec3_copy(tvec3 *a, tvec3 *b)
{
    a->x = b->x;
    a->y = b->y;
    a->z = b->z;
}

static inline void tvec3_mul(tvec3 *a, float m)
{
    a->x*=m;
    a->y*=m;
    a->z*=m;
}

static inline tvec3
tvec3_lerp(tvec3 a, tvec3 b, float f)
{
    tvec3 tmp = b;
    tvec3_sub(&tmp, &a);
    tvec3_mul(&tmp, f);
    return tvec3f(
        a.x+tmp.x,
        a.y+tmp.y,
        a.z+tmp.z
    );
    /*
    float fi = 1.-f;
    return (tvec3){
        f*a.x + fi*b.x,
        f*a.y + fi*b.y,
        f*a.z + fi*b.z
    };
    */
}

static inline void tvec3_normalize(tvec3 *v)
{
    float a = tvec3_magnitude(v);
    if (a == 0.0f)
        return;
    v->x = v->x/a;
    v->y = v->y/a;
    v->z = v->z/a;
}

static inline float tvec3_magnitude(tvec3 *v)
{
    return sqrt(tvec3_dot(v,v));
}

static inline void tvec3_cross(tvec3 *r, tvec3 v1, tvec3 v2)
{
    tvec3 tmp;
    tmp.x = v1.y*v2.z - v1.z*v2.y;
    tmp.y = v1.z*v2.x - v1.x*v2.z;
    tmp.z = v1.x*v2.y - v1.y*v2.x;

    r->x = tmp.x;
    r->y = tmp.y;
    r->z = tmp.z;
}

static inline float tvec3_dot(tvec3 *v1, tvec3 *v2)
{
    return v1->x*v2->x + v1->y*v2->y + v1->z*v2->z;
}
/** @} */

/**
 * @relates tvec4 @{
 **/
#define TVEC4_INLINE(v) (v).x, (v).y, (v).z, (v).w /**< Let the preprocessor expand this tvec4 to 4 floats separated by comma. */
#define TVEC4_INLINE_N(v) -(v).x, -(v).y, -(v).z, -(v).w /**< Same as TVEC4_INLINE() but adds a minus sign in front of each value. */
#define TVEC4_INLINE_M(v, m) (v).x*m, (v).y*m, (v).z*m, (v).w*m
static inline float tvec4_dot(tvec4 *v1, tvec4 *v2);
static inline void  tvec4_mul(tvec4 *r, float f);
void  tvec4_mul_mat4(tvec4 *v, float *m);
static inline void tvec4_copy(tvec4 *a, tvec4 *b)
{
    a->x = b->x;
    a->y = b->y;
    a->z = b->z;
    a->w = b->w;
}
static inline float tvec4_dot(tvec4 *v1, tvec4 *v2)
{
    return v1->x*v2->x + v1->y*v2->y + v1->z*v2->z + v1->w*v2->w;
}

static inline tvec4 tvec4f(float r, float g, float b, float a)
{
    struct tvec4 ret = {
        r,
        g,
        b,
        a
    };
    return ret;
}

static inline void  tvec4_mul(tvec4 *r, float f)
{
    r->x*=f;
    r->y*=f;
    r->z*=f;
    r->w*=f;
}
/** @} **/

/**
 * @relates tvec2 @{
 **/
static inline tvec2 tvec2f(float x, float y)
{
    struct tvec2 ret = {
        x,
        y
    };
    return ret;
    //return (tvec2){x,y};
}

static inline float tvec2_magnitude(tvec2 *v)
{
    return sqrt(v->x*v->x+v->y*v->y);
}

static inline tvec2 tvec2_sub(tvec2 a, tvec2 b)
{
    return tvec2f(a.x-b.x, a.y-b.y);
}

static inline tvec2 tvec2_mul(tvec2 v1, float x)
{
    return tvec2f(v1.x*x, v1.y*x);
}

static inline float tvec2_dot(tvec2 v1, tvec2 v2)
{
    return v1.x*v2.x + v1.y*v2.y;
}

static inline float tvec2_dist(tvec2 a, tvec2 b)
{
    a = tvec2_sub(b, a);
    return tvec2_magnitude(&a);
}

static inline float tvec2_distsq(tvec2 a, tvec2 b)
{
    a = tvec2_sub(b, a);
    return a.x*a.x+a.y*a.y;
}


static inline tvec2 tvec2_add(tvec2 a, tvec2 b)
{
    return tvec2f(a.x+b.x, a.y+b.y);
}

static inline void tvec2_normalize(tvec2 *v)
{
    float a = tvec2_magnitude(v);
    if (a == 0.0f)
        return;
    v->x /= a;
    v->y /= a;

}

static inline void tvec2_copy(tvec2 *a, tvec2 *b)
{
    a->x = b->x;
    a->y = b->y;
}

static inline void tvec2_normal_add(tvec2 *a, float x, float y)
{
    a->x += x;
    a->y += y;
    tvec2_normalize(a);
}

static inline float tvec2_det(tvec2 a, tvec2 b)
{
    return a.x * b.y - a.y*b.x;
}

static inline float tvec2_detf(float ax, float ay, float bx, float by)
{
    return ax * by - ay*bx;
}

#endif
