#include "crc.hh"
#include "pkgman.hh"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#ifndef _NO_TMS
#include "tms/backend/print.h"
#endif

uint32_t
crc32_uint64(uint32_t crc, uint64_t data)
{
    for (int x=0; x<8; ++x) {
        uint8_t cool = (data >> x*8) & 0xff;
        crc = crc32(crc, &cool, 1);
    }

    return crc;
}

uint32_t
crc32_uint32(uint32_t crc, uint32_t data)
{
    for (int x=0; x<4; ++x) {
        uint8_t cool = (data >> x*8) & 0xff;
        crc = crc32(crc, &cool, 1);
    }

    return crc;
}

uint32_t
crc32_level(const lvlinfo &lvl, const lvlbuf &lb, uint32_t timestamp, uint32_t last_score, int method)
{
#ifndef _NO_TMS
    tms_assertf(method < 5, "only 5 methods of crc32_level are implemented.");

    tms_infof("Using lastscore %u in crc32_level.", last_score);
#endif

    uint32_t crc = crc32(0L, Z_NULL, 0);

    switch (method) {
        case 0: {
            for (uint64_t x=0; x<lb.size; x += CRC_BUFFER_SIZE) {
                uint64_t len = std::min((uint64_t)CRC_BUFFER_SIZE, lb.size-x);

                crc = crc32(crc, lb.buf+x, len);
            }

            crc = crc32_uint64(crc, lvl.seed);
            crc = crc32_uint32(crc, timestamp);
            crc = crc32_uint32(crc, last_score);
        } break;

        case 1: {
            crc = crc32_uint32(crc, timestamp);
            crc = crc32_uint64(crc, lb.size);
            crc = crc32_uint32(crc, last_score);

            for (uint64_t x=0; x<lb.size; x += CRC_BUFFER_SIZE) {
                uint64_t len = std::min((uint64_t)CRC_BUFFER_SIZE, lb.size-x);

                crc = crc32(crc, lb.buf+x, len);
            }
        } break;

        case 2: {
            crc = crc32_uint64(crc, lvl.flags);
            crc = crc32_uint32(crc, last_score);
            crc = crc32_uint32(crc, timestamp);

            for (uint64_t x=0; x<lb.size; x += CRC_BUFFER_SIZE) {
                uint64_t len = std::min((uint64_t)CRC_BUFFER_SIZE, lb.size-x);

                crc = crc32(crc, lb.buf+x, len);
            }

            crc = crc32_uint32(crc, lvl.flags);
            crc = crc32_uint32(crc, lvl.parent_revision);
        } break;

        case 3: {
            crc = crc32_uint64(crc, lvl.flags);
            crc = crc32_uint32(crc, last_score);
            crc = crc32_uint32(crc, lvl.flags);
            crc = crc32_uint32(crc, lvl.bg_color);
            crc = crc32_uint32(crc, lvl.community_id);
            crc = crc32_uint32(crc, last_score);
            crc = crc32_uint32(crc, timestamp);
        } break;

        case 4: {
            crc = crc32_uint32(crc, timestamp);
            crc = crc32_uint64(crc, lvl.flags);
            crc = crc32_uint32(crc, last_score);
            crc = crc32_uint32(crc, lvl.community_id);
            crc = crc32_uint32(crc, last_score);
            crc = crc32_uint32(crc, lvl.bg_color);
            crc = crc32_uint32(crc, last_score);
            crc = crc32_uint32(crc, last_score);
            crc = crc32_uint32(crc, lvl.final_score);
        } break;
    }

    return crc;
}
