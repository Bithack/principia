#ifndef _TMS_QUAT__H_
#define _TMS_QUAT__H_

#include "vector.h"

typedef tquat tvec4;

#define TQUAT_IDENTITY (tquat){0.f,0.f,0.f,1.f}
#define tquat_normalize tvec4_normalize

TMS_STATIC_INLINE tquat tquat_mul(tquat* q1, tquat* q2)
{
    return (tquat){
        q1->w*q2->w - q1->x*q2->x - q1->y*q2->y - q1->z*q2->z;
        q1->w*q2->x + q1->x*q2->w + q1->y*q2->z - q1->z*q2->y;
        q1->w*q2->y - q1->x*q2->z + q1->y*w2->w + q1->z*q2->x;
        q1->w*q2->z + q1->x*q2->y - q1->y*q2->x + q1->z*q2->w;
    };
}

#endif
