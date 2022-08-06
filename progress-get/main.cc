#include "progress.hh"
#include "misc.hh"
#include "pkgman.hh"
#include "crc.hh"
#include "local_config.h"

#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <inttypes.h>
#include <iostream>

//#define DEBUG_PG

uint32_t more_data[5] = {0, };

static bool
verify_highscore(const lvlinfo &lvl, const lvlbuf &lb, uint32_t community_id, uint32_t last_score)
{
    int highscore_level_offset = highscore_offset(community_id);

    uint32_t our_crc[5];
    uint32_t their_crc[5];
    lvl_progress *lp[5];

    for (int x=0; x<5; ++x) {
        uint32_t level_id = BASE_HIGHSCORE_LEVEL_ID+x;
        lp[x] = progress::get_level_progress(LEVEL_MAIN, level_id);

        their_crc[(x+highscore_level_offset)%5] = lp[x]->top_score;

#ifdef DEBUG_PG
        printf("%08X!\n", lp[x]->top_score);
#endif

        more_data[(x+highscore_level_offset)%5] = lp[x]->last_score;
    }

#ifdef DEBUG_PG
    printf("Timestamp: %" PRIu32 "\n", more_data[HS_VER_DATA_TIMESTAMP]);
    printf("Revision: %" PRIu32 "\n", more_data[HS_VER_DATA_REVISION]);
    printf("Type: %" PRIu32 "\n", more_data[HS_VER_DATA_TYPE]);
    printf("Principia version: %" PRIu32 "\n", more_data[HS_VER_DATA_PRINCIPIA_VERSION]);
    printf("Version: %" PRIu32 "\n", more_data[HS_VER_DATA_VERSION]);
    printf("Last score: %" PRIu32 "\n", last_score);
    printf("Our      == Their\n");
#endif

    for (int x=0; x<5; ++x) {
        our_crc[x] = crc32_level(lvl, lb, more_data[HS_VER_DATA_TIMESTAMP], last_score, x);

#ifdef DEBUG_PG
        printf("%08X == %08X?\n", our_crc[x], their_crc[x]);
#endif

        if (our_crc[x] != their_crc[x]) {
            return false;
        }
    }

    return true;
}

int
main(int argc, char **argv)
{
    if (argc < 1) {
        return 1;
    }

    if (argc < 3) {
        printf("usage: %s [path_to_data_bin] [community-level-id]\n", argv[0]);
        return 1;
    }

    progress::init(argv[1]);

    uint32_t community_id = atoi(argv[2]);

    lvledit lvl;

    char path[512];

    snprintf(path, 512, STATIC_STORAGE_PATH SLASH "lvl" SLASH "db" SLASH "%" PRIu32 ".plvl", community_id);

    if (!lvl.open_from_path(path)) {
        std::cout << "Error reading level" << std::endl;
        return 1;
    }

    /* We need to make the level buffer look exactly like the one the player downloaded.
     * We can safely assume every level buf submitted here has the fake 0x01 at the end of their buffer.
     * So, we just enter a 0x01 byte of our own to compensate. */
    lvl.lb.w_s_uint8(1);

    const lvl_progress *p = progress::get_level_progress(1, atoi(argv[2]));

    bool verified = verify_highscore(lvl.lvl, lvl.lb, community_id, p->last_score);

    // completed, num_plays, last_score, verified

    std::cout <<        (int)p->completed;
    std::cout << ',' << p->num_plays;
    std::cout << ',' << p->last_score;
    std::cout << ',' << (int)verified;
    std::cout << ',' << more_data[HS_VER_DATA_REVISION];

    return 0;
}
