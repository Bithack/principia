#pragma once

#define CRC_BUFFER_SIZE 8192

#include "pkgman.hh"

#include "zlib.h"

uint32_t crc32_uint64(uint32_t crc, uint64_t data);

uint32_t crc32_uint32(uint32_t crc, uint32_t data);

uint32_t crc32_level(
        const lvlinfo &lvl,
        const lvlbuf &lb,
        uint32_t timestamp,
        uint32_t last_score,
        int method);
