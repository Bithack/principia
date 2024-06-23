#include "progress.hh"
#include <stdio.h>
#include <string.h>
#include <zlib.h>

#include <cstdlib>

#ifdef _NO_TMS
#define tms_infof(...)
#define tms_errorf(...)
#else
#include <tms/bindings/cpp/cpp.hh>
#endif

#define WARNING_STR "Warning: if you edit this file manually, you risk losing all your game data."
#define ID_XOR    0xfa5f0e6f
#define O_XOR     0xaab27f1a
#define X_XOR     0x0f81fea1
#define A_XOR     0xdeadbeef
#define TYPE_SEP  0x3d

std::map<uint32_t, lvl_progress*> progress::levels[3];

bool progress::initialized = false;

lvl_progress*
progress::get_level_progress(int level_type,
                             uint32_t level_id)
{
    if (level_type < 0 || level_type >= 3)
        return 0;

    std::map<uint32_t, lvl_progress*>::iterator i;

    i = levels[level_type].find(level_id);

    if (i != levels[level_type].end())
        return i->second;

    lvl_progress *ret = new lvl_progress();

    levels[level_type].insert(std::pair<uint32_t, lvl_progress*>(level_id, ret));
    return ret;
}

void
progress::init(char *custom_path/*=0*/)
{
    char tmp[1024];
    if (custom_path) {
        strcpy(tmp, custom_path);
    } else {
#ifdef _NO_TMS
        return;
#else
        const char *storage = tbackend_get_storage_path();
        snprintf(tmp, 1023, "%s/data.bin", storage);
#endif
    }

    long crc_pos[4]={0,0,0,0};
    uint32_t crc_set=0;

    uint32_t crc_v=0;
    uint32_t crc_base = 0;

    FILE *fp = fopen(tmp, "rb");

    if (fp) {
        fread(&tmp, 1, strlen(WARNING_STR), fp);

        for (int x=0; x<3; x++) {
            uint32_t nlevels;
            fread(&nlevels, 1, sizeof(uint32_t), fp);

            nlevels ^= A_XOR;

            for (int l=0; l<nlevels; l++) {
                lvl_progress p;
                uint32_t r_nn=0, r_nnn=0;
                uint32_t id;
                switch (l%5) {
                    case 0: fread(tmp, 1, sizeof(uint8_t), fp); break;
                    case 1: fread(tmp, 3, sizeof(uint8_t), fp); break;
                    case 2: fread(tmp, 4, sizeof(uint8_t), fp); break;
                    case 3: fread(tmp, 2, sizeof(uint8_t), fp); break;
                    case 4: fread(tmp, 5, sizeof(uint8_t), fp); break;
                }
                fread(&id, 1, sizeof(uint32_t), fp);
                id ^= ID_XOR;
                id -= l;

                if (crc_set < 4u) {
                    crc_pos[crc_set] = ftell(fp);
                    uint8_t t;
                    fread(&t, 1, sizeof(uint8_t), fp);

                    crc_v |= t << crc_set*8u;

                    crc_set ++;
                }

                fread(&p.num_plays, 1, sizeof(uint32_t), fp);
                r_nn = p.num_plays;
                p.completed = p.num_plays >> 31;
                p.num_plays &= ~(1<<31);

                fread(&p.top_score, 1, sizeof(uint32_t), fp);
                r_nn += p.top_score;
                p.top_score ^= O_XOR;

                //tms_infof("reading: %u, %u", id, p.top_score);

                fread(&p.last_score, 1, sizeof(uint32_t), fp);
                r_nn += p.last_score;
                p.last_score ^= O_XOR;

                fread(&p.time, 1, sizeof(uint32_t), fp);
                r_nn += p.time;
                p.time ^= O_XOR;

                uint32_t n;
                fread(&n, 1, sizeof(uint32_t), fp);
                n ^= X_XOR;

                /* verify lower 16 bits of top score */
                uint32_t t = p.top_score;

                if ((t & 0xffff) != (n & 0xffff)) {
                    tms_infof("invalid score 1");
                    goto err;
                }

                if ( (((t < 1000000000) << 31) != (n & (1<<31)))
                  || ((t < 100000) << 30) != (n & (1<<30))
                  || ((t < 10000000) << 29) != (n & (1<<29))
                  || ((t < 100) << 28) != (n & (1<<28))
                  || ((t < 100000000) << 27) != (n & (1<<27))
                  || ((t < 10000) << 26) != (n & (1<<26))
                  || ((t < 1000000) << 25) != (n & (1<<25))
                  || ((t < 1000) << 24) != (n & (1<<24))
                  || ((t < 10) << 23) != (n & (1<<23))) {
                    tms_infof("invalid score 2");
                    goto err;
                }

                uint32_t nn,nnn;
                fread(&nn, 1, sizeof(uint32_t), fp);
                switch (l%9) {
                    case 0: fread(tmp, 9, sizeof(uint8_t), fp); break;
                    case 1: fread(tmp, 1, sizeof(uint8_t), fp); break;
                    case 2: fread(tmp, 2, sizeof(uint8_t), fp); break;
                    case 3: fread(tmp, 5, sizeof(uint8_t), fp); break;
                    case 4: fread(tmp, 7, sizeof(uint8_t), fp); break;
                    case 5: fread(tmp, 8, sizeof(uint8_t), fp); break;
                    case 6: fread(tmp, 3, sizeof(uint8_t), fp); break;
                    case 7: fread(tmp, 4, sizeof(uint8_t), fp); break;
                    case 8: fread(tmp, 6, sizeof(uint8_t), fp); break;
                }
                fread(&nnn, 1, sizeof(uint32_t), fp);
                if (nn != r_nn) {tms_errorf("invalid r_nn"); goto err;};
                if (nnn != (p.num_plays + p.top_score + p.last_score + p.time)) {tms_errorf("invalid nnn");goto err;};
                crc_base += p.top_score;

                lvl_progress *pp = new lvl_progress();
                *pp = p;
                levels[x].insert(std::pair<uint32_t, lvl_progress*>(id, pp));
            }
        }

        while (crc_set < 4u) {
            crc_pos[crc_set] = ftell(fp);
            uint8_t t;
            fread(&t, 1, sizeof(uint8_t), fp);

            crc_v |= t << crc_set*8u;

            crc_set ++;
        }

        /* verify crc32 */
        long size = ftell(fp);
        if (size < 0) {goto err; };
        char *tcrc = (char*)malloc(size);

        fseek(fp, 0, SEEK_SET);
        fread(tcrc, 1, size, fp);
        fclose(fp);
        fp = 0;

        tms_infof("crc base read: %x", crc_base);
        tms_infof("crc sum read: %x", crc_v);

        for (uint32_t x=0; x<4; x++) {
            tcrc[crc_pos[x]] = (uint8_t)((crc_base >> (x*8u)) & 0xffu);
        }

        uint32_t crc = crc32(0L, (const Bytef*)tcrc, size);
        free(tcrc);

        if (crc != crc_v) {
            tms_infof("crc mismatch");
            //goto err;
        }
        initialized = true;
        return;

    } else {
        tms_infof("data.bin doesn't exist, creating one");
        initialized = true;
        return;
    }

err:
    levels[0].clear();
    levels[1].clear();
    levels[2].clear();
    tms_errorf("ERROR: invalid data in data.bin");
    if (fp) fclose(fp);
    initialized = true;
}

void
progress::commit()
{
#ifndef _NO_TMS

    if (!initialized) {
        return;
    }

    char filename[1024];
    const char *storage = tbackend_get_storage_path();
    snprintf(filename, 1023, "%s/data.bin", storage);

    long crc_pos[4]={0,0,0,0};
    uint32_t crc_set=0;

    FILE *fp = fopen(filename, "w+b");

    if (fp) {
        uint32_t crc_val = 0;
        fwrite(WARNING_STR, 1, strlen(WARNING_STR), fp);

        for (int x=0; x<3; x++) {
            uint32_t t = levels[x].size() ^ A_XOR;
            fwrite(&t, 1, sizeof(uint32_t), fp);
            uint32_t c = 0;

            for (std::map<uint32_t, lvl_progress*>::iterator i = levels[x].begin();
                    i != levels[x].end(); i++) {
                lvl_progress p = *i->second;

                uint32_t id = (i->first+c) ^ ID_XOR;
                switch (c%5) {
                    case 0: fwrite(i->second, 1, sizeof(uint8_t), fp); break;
                    case 1: fwrite(i->second, 3, sizeof(uint8_t), fp); break;
                    case 2: fwrite(i->second, 4, sizeof(uint8_t), fp); break;
                    case 3: fwrite(i->second, 2, sizeof(uint8_t), fp); break;
                    case 4: fwrite(i->second, 5, sizeof(uint8_t), fp); break;
                }
                fwrite(&id, 1, sizeof(uint32_t), fp);
                if (crc_set < 4) {
                    crc_pos[crc_set] = ftell(fp);
                    uint8_t xx = (uint8_t)levels[1].size();
                    fwrite(&xx, 1, sizeof(uint8_t), fp);
                    crc_set ++;
                }
                uint32_t n = p.num_plays | (p.completed << 31), nn=0,nnn=0;
                nn += n;
                nnn += p.num_plays;
                fwrite(&n, 1, sizeof(uint32_t), fp);
                n = p.top_score ^ O_XOR;
                nn += n;
                nnn += p.top_score;
                fwrite(&n, 1, sizeof(uint32_t), fp);
                n = p.last_score ^ O_XOR;
                nn += n;
                nnn += p.last_score;
                fwrite(&n, 1, sizeof(uint32_t), fp);
                n = p.time ^ O_XOR;
                nn += n;
                nnn += p.time;
                fwrite(&n, 1, sizeof(uint32_t), fp);
                crc_val += p.top_score;

                n =(
                    ((uint32_t)(p.top_score < 1000000000) << 31)
                  | ((uint32_t)(p.top_score < 100000) << 30)
                  | ((uint32_t)(p.top_score < 10000000) << 29)
                  | ((uint32_t)(p.top_score < 100) << 28)
                  | ((uint32_t)(p.top_score < 100000000) << 27)
                  | ((uint32_t)(p.top_score < 10000) << 26)
                  | ((uint32_t)(p.top_score < 1000000) << 25)
                  | ((uint32_t)(p.top_score < 1000) << 24)
                  | ((uint32_t)(p.top_score < 10) << 23)
                  | (p.top_score & 0xffff)
                  ) ^ X_XOR;
                fwrite(&n, 1, sizeof(uint32_t), fp);
                fwrite(&nn, 1, sizeof(uint32_t), fp);
                switch (c%9) {
                    case 0: fwrite(i->second, 9, sizeof(uint8_t), fp); break;
                    case 1: fwrite(i->second, 1, sizeof(uint8_t), fp); break;
                    case 2: fwrite(i->second, 2, sizeof(uint8_t), fp); break;
                    case 3: fwrite(i->second, 5, sizeof(uint8_t), fp); break;
                    case 4: fwrite(i->second, 7, sizeof(uint8_t), fp); break;
                    case 5: fwrite(i->second, 8, sizeof(uint8_t), fp); break;
                    case 6: fwrite(i->second, 3, sizeof(uint8_t), fp); break;
                    case 7: fwrite(i->second, 4, sizeof(uint8_t), fp); break;
                    case 8: fwrite(i->second, 6, sizeof(uint8_t), fp); break;
                }
                fwrite(&nnn, 1, sizeof(uint32_t), fp);

                c++;
            }
        }

        for (; crc_set<4; crc_set++) {
            crc_pos[crc_set] = ftell(fp);
            uint8_t xx = (uint8_t)levels[1].size();
            fwrite(&xx, 1, sizeof(uint8_t), fp);
        }
        long size = ftell(fp);

        for (uint32_t x=0; x<4; x++) {
            fseek(fp, crc_pos[x], SEEK_SET);
            uint8_t v = (uint8_t)((crc_val >> (x*8u)) & 0xffu);
            fwrite(&v, 1, sizeof(uint8_t), fp);
        }

        fseek(fp, 0, SEEK_SET);

        size_t bl;
        unsigned char b[1024];
        uint32_t crc = crc32(0L, Z_NULL, 0);
        while ((bl = fread(b, 1, 1024, fp)) > 0) {
            crc = crc32(crc, b, bl);
        }

        for (uint32_t x=0; x<4; x++) {
            fseek(fp, crc_pos[x], SEEK_SET);
            uint8_t v = (uint8_t)((crc >> (x*8u)) & 0xffu);
            fwrite(&v, 1, sizeof(uint8_t), fp);
        }

        fclose(fp);
    } else {
        tms_errorf("Error: could not save progress %s", filename);
    }
#endif
}
