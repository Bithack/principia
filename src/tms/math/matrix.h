#ifndef _TMS_MATRIX__H_
#define _TMS_MATRIX__H_

#include <tms/math/vector.h>
#include <string.h>

/** @relates tmatN @{ */
/**
 * tmat4 and tmat3 functions operate on float buffers directly.
 * tmat3_* functions expect a pointer to a buffer of at least 9 floats.
 * tmat4_* functions expect a pointer to a buffer of at least 16 floats.
 *
 * The types tmat3 and tmat4 are not defined.
 * For example, a 4x4 matrix on the stack is defined as float example[16]. If
 * you want it on the heap you would to float *example = malloc(16*sizeof(float))
 *
 * Functions starting with tmatN_set_* will replace the contents of the matrix, all
 * other functions will multiply the current matrix with the new one. The only exception
 * is tmatN_load_identity().
 **/
struct tmatN {
#if defined(__clang__) || defined(_MSC_VER)
    int dummy125;
#endif
}; /* for doc purposes */

#define TMAT3_SIZE (9*sizeof(float))

#define TMAT4_SIZE (16*sizeof(float))
#define TMAT4_IDENTITY {1.0, 0.0, 0.0, 0.0, \
                         0.0, 1.0, 0.0, 0.0, \
                         0.0, 0.0, 1.0, 0.0, \
                         0.0, 0.0, 0.0, 1.0}
#define TMAT3_IDENTITY {1.0, 0.0, 0.0, \
                         0.0, 1.0, 0.0, \
                         0.0, 0.0, 1.0}

/* 4x4 matrix functions */
void tmat4_perspective(float *m, float fov, float aspect, float nearz, float farz);
void tmat4_multiply(float *m1, float *m2);
void tmat4_multiply_reverse(float *m2, float *m1);
void tmat4_rotate(float *m, float angle, float x, float y, float z);
void tmat4_load_identity(float *m);
void tmat4_translate(float *m, float x, float y, float z);
void tmat4_lookat(float result[16], float eyeX, float eyeY, float eyeZ, float centerX, float centerY, float centerZ, float upX, float upY, float upZ);
void tmat4_frustum(float *result, float left, float right, float bottom, float top, float nearVal, float farVal);
void tmat4_dump(float *m);
void tmat4_set_ortho(float *result, float left, float right, float bottom, float top, float near, float far);
int  tmat4_invert(float *out);
void tmat4_scale(float *m, float x, float y, float z);
void tmat4_set_near_plane(float *m, tvec4 *plane);
void tmat4_transpose(float *m);

static inline void tmat4_translate_vec3(float *m, tvec3 *v)
{
    tmat4_translate(m, v->x, v->y, v->z);
}

static inline void
tmat4_copy(float *dest, float *target)
{
    memcpy(dest, target, TMAT4_SIZE);
}

static inline void
tmat4_lerp(float *out, float *m1, float *m2, float blend)
{
    int x;
    for (x=0; x<16; x++)
        out[x] = m1[x]*(1.f-blend) + m2[x]*blend;
}

static inline void
tmat3_copy(float *dest, float *target)
{
    memcpy(dest, target, TMAT3_SIZE);
}

/**
 * Copy the 3x3 sub-matrix and transpose it,
 *
 * @relates tmatN
 **/
static inline void
tmat3_copy_mat4_sub3x3T(float *d, float *s)
{
    d[0] = s[0];
    d[1] = s[4];
    d[2] = s[8];
    d[3] = s[1];
    d[4] = s[5];
    d[5] = s[9];
    d[6] = s[2];
    d[7] = s[6];
    d[8] = s[10];
}

/**
 * Copy the 3x3 sub-matrix
 *
 * @relates tmatN
 **/
static inline void
tmat3_copy_mat4_sub3x3(float *d, float *s)
{
    d[0] = s[0];
    d[1] = s[1];
    d[2] = s[2];
    d[3] = s[4+0];
    d[4] = s[4+1];
    d[5] = s[4+2];
    d[6] = s[8+0];
    d[7] = s[8+1];
    d[8] = s[8+2];
}

void tmat3_load_identity(float *m);

#endif
