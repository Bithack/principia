#ifndef _TMS_ERR__H_
#define _TMS_ERR__H_

enum {
    T_OK = 0,
    T_ERR,
    T_OUT_OF_MEM,
    T_CONT,
    T_COULD_NOT_OPEN,
    T_NO_DATA,

};

#include <tms/backend/print.h>

#endif
