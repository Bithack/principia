#ifndef _TMS_ERR__H_
#define _TMS_ERR__H_

enum {
    T_OK                         = 0,
    T_OUT_OF_MEM                 = 1,
    T_ATTRIBUTES_MISMATCH        = 2,
    T_MESH_NOT_UPLOADED          = 3,
    T_NO_SHADER                  = 4,
    T_CONT                       = 5,
    T_UNSUPPORTED_FILE_FORMAT    = 6,
    T_COULD_NOT_OPEN             = 7,
    T_READ_ERROR                 = 8,
    T_NO_DATA                    = 9,
    T_COMPILE_ERROR              = 10,

    T_INVALID_OPERATION          = 11,
    T_INVALID_HEADER             = 12,
    T_INCOMPATIBLE               = 13,
    T_INVALID_EVENT              = 14,
    T_ERR                        = 15,
};

/* TODO: T_* enums to string converting function (err.c?) */

#include <tms/backend/print.h>

#endif
