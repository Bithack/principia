#ifndef _UD2_INFO__H_
#define _UD2_INFO__H_

#include <inttypes.h>

enum {
    UD2_TPIXEL_DESC,
    UD2_PLANT_SECTION,
    UD2_LADDER_STEP,
    UD2_CLIMBABLE,
};

struct ud2_info
{
    uint32_t ud2_type;

    ud2_info(uint32_t _type)
        : ud2_type(_type)
    { }

    inline uint32_t get_type() const
    {
        return this->ud2_type;
    }
};

#endif
