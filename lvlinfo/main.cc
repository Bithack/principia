#include <stdio.h>
#include <inttypes.h>
#include <set>
#include <map>
#include <math.h>
#include <cmath>

#ifdef _WIN32
FILE *_f_out = stdout;
FILE *_f_err = stderr;

#include <tms/backends/windows/tms/backend/print.h>
#else
#include <tms/backends/linux/print.h>
#endif

#include "pkgman.hh"
#include "tpixel.hh"

#include <Box2D/Common/b2Math.h>
#include <tms/math/vector.h>

enum {
    CHUNK_LOAD_MERGED = 0,
    CHUNK_LOAD_PIXELS = 1,
    CHUNK_LOAD_EMPTY
};

static char tmp[64];

static const char*
property_type_string(uint8_t type)
{
    switch (type) {
        case P_INT8: return "uint8_t";
        case P_INT:  return "uint32_t";
        case P_ID:   return "uint32_t (id)";
        case P_FLT:  return "float";
        case P_STR:  return "string";
        default:
            snprintf(tmp, 63, "Unknown (%" PRIu8 ")", type);
            return tmp;
    }
}

void
read_chunks(const lvlinfo &level, lvlbuf *lb)
{
    int num_chunks = level.num_chunks;
    int n_read;

    printf("Reading %d chunks\n", level.num_chunks);

    for (n_read = 0; (!lb->eof() && n_read < num_chunks); n_read++) {
        for (int x=0; x<num_chunks; x++) {
            size_t ptr = lb->rp;

            int pos_x = lb->r_int32();
            int pos_y = lb->r_int32();
            int generate_phase = lb->r_uint8();
            uint8_t load_method = lb->r_uint8();

            printf("Chunk %d pos X: %d\n", x, pos_x);
            printf("Chunk %d pos Y: %d\n", x, pos_y);
            printf("Chunk %d generate phase: %" PRIu8 "\n", x, generate_phase);
            printf("Chunk %d load method: %" PRIu8 "\n", x, load_method);

            bool soft = (level.flags & LVL_CHUNKED_LEVEL_LOADING);

            //level_chunk *c = this->get_chunk(pos_x, pos_y, soft);

            //tms_debugf("read chunk at %d %d, already found? %p", pos_x, pos_y, c)

            if (true) { // XXX
                switch (load_method) {
                    case CHUNK_LOAD_MERGED:
                        for (int m=0; m<3; m++) {
                            uint8_t num_merged = lb->r_uint8();
                            printf("Chunk %d num merged[%d]: %" PRIu8 "\n", x, m, num_merged);
                            char *buf = (char*)malloc(num_merged*sizeof(struct tpixel_desc));
                            lb->r_buf(buf, num_merged*sizeof(struct tpixel_desc));
                            printf("Read data: '%s'\n", buf);
                            free(buf);
                        }
                        break;

                    case CHUNK_LOAD_PIXELS:
                        {
                            uint8_t pixels[3][16][16];
                            lb->r_buf((char*)pixels, sizeof(uint8_t)*3*16*16);
                            // XXX: print pixels!
                        }
                        break;
                }

            } else {
#if 0
                chunk_pos cp(pos_x, pos_y);

                switch (load_method) {
                    case CHUNK_LOAD_MERGED:
                        /* skip num merged */
                        for (int m=0; m<3; m++) {
                            uint32_t skip = lb->r_uint8();
                            lb->rp += skip*sizeof(struct tpixel_desc);
                        }
                        break;

                    case CHUNK_LOAD_PIXELS:
                        lb->rp += sizeof(uint8_t)*3*16*16;
                        break;
                }

                size_t size = lb->rp - ptr;
                //tms_debugf("reading unloaded chunk %d %d size %u", pos_x, pos_y, (int)size);
                this->chunks.insert(std::make_pair(cp, preload_info(ptr, size)));
#endif
            }
        }
    }
}

static void
print_group(lvlbuf *lb, int version, uint32_t id_modifier=0, b2Vec2 displacement=b2Vec2(0.f,0.f))
{
    printf("New group<%"PRIu32"+%"PRIu32"\n", lb->r_uint32(), id_modifier);
    printf("pos.x: %.2f+%.2f\n", lb->r_float(), displacement.x);
    printf("pos.y: %.2f+%.2f\n", lb->r_float(), displacement.y);
    printf("angle: %.2f\n", lb->r_float());

    if (version >= LEVEL_VERSION_1_5) {
        uint32_t state_size = lb->r_uint32();
        printf("state size: %"PRIu32"\n", state_size);
        lb->rp += state_size;
    }

    printf(">\n");
}

static bool
print_entity(lvlbuf *lb, int version, uint32_t id_modifier=0, b2Vec2 displacement=b2Vec2(0.f,0.f))
{
    uint8_t g_id, np, nc;
    uint32_t state_size = 0;
    uint32_t group_id;
    uint32_t id;

    g_id = lb->r_uint8();
    id = lb->r_uint32() + id_modifier;
    group_id = (uint32_t)lb->r_uint16();
    group_id = group_id | (lb->r_uint16() << 16);

    printf("New entity<%"PRIu8 ", %"PRIu32"+%"PRIu32"\n", g_id, id, id_modifier);

    printf("Group ID: %"PRIu32"\n", group_id);

    if (group_id != 0) group_id += id_modifier;

    np = lb->r_uint8();

    printf("Num properties: %" PRIu8 "\n", np);

    if (version >= LEVEL_VERSION_1_5) {
        nc = lb->r_uint8();
        state_size = lb->r_uint32();
    } else {
        nc = 0;
    }

    printf("Num chunks: %" PRIu8 "\n", nc);
    printf("State size: %" PRIu32 "\n", state_size);

    tvec2 pos;
    pos.x = lb->r_float();
    pos.y = lb->r_float();

    /* if we're grouped, the pos is local within the group, do not add displacement 
     * since displacement has already been added to the group itself */
    if (group_id == 0) {
        printf("Pos X: %.2f\n", pos.x);
        printf("Pos Y: %.2f\n", pos.y);
    } else {
        printf("Pos X: %.2f+%.2f\n", pos.x, displacement.x);
        printf("Pos Y: %.2f+%.2f\n", pos.y, displacement.y);
    }


    float angle = lb->r_float();
    uint8_t layer = lb->r_uint8();

    printf("Angle: %.2f\n", angle);
    printf("Layer: %" PRIu8 "\n", layer);

    if (version >= LEVEL_VERSION_1_5) {
        printf("Flags: %"PRIu64"\n", lb->r_uint64());

        for (int x=0; x<nc; x++) {
            printf("Chunk [%d] X: %"PRId32"\n", x, lb->r_int32());
            printf("Chunk [%d] Y: %"PRId32"\n", x, lb->r_int32());
        }

        lb->rp += state_size;
    } else {
        printf("Axis rot? %s\n", lb->r_uint8() ? "yes" : "no");

        if (version >= 10) {
            printf("Moveable? %s\n", lb->r_uint8() ? "yes" : "no");
        }
    }

    for (int x=0; x<np; x++) {
        uint8_t type = lb->r_uint8();

        int n = x + 1;

        printf("Property [%d/%d] type: %s\n", n, np, property_type_string(type));

        switch (type) {
            case P_INT8:
                printf("Property [%d/%d] value: %" PRIu8 "\n", n, np, lb->r_uint8());
                break;
            case P_INT:
                printf("Property [%d/%d] value: %" PRIu32 "\n", n, np, lb->r_uint32());
                break;
            case P_ID:
                printf("Property [%d/%d] value: %" PRIu32 " (+%" PRIu32 ")\n", n, np, lb->r_uint32(), id_modifier);
                break;
            case P_FLT:
                printf("Property [%d/%d] value: %.6f\n", n, np, lb->r_float());
                break;
            case P_STR:
                {
                    uint32_t len = 0;

                    if (version >= LEVEL_VERSION_1_5) {
                        len = lb->r_uint32();
                    } else {
                        len = lb->r_uint16();
                    }

                    char *buf = (char*)malloc(len);
                    lb->r_buf(buf, len);

                    printf("Property [%d/%d] value: '%s'[%"PRIu32"]\n", n, np, buf, len);

                    free(buf);
                }
                break;

            default:
                fprintf(stderr, "Unknown property type: %" PRIu8 ". Not sure how to proceed :(\n", type);
                break;
        }
    }

    printf(">\n");

    return true;
}

static void
print_cable(lvlbuf *lb, int version, uint64_t flags, uint32_t id_modifier=0, b2Vec2 displacement=b2Vec2(0.f,0.f))
{
    uint8_t ctype = lb->r_uint8();
    uint32_t id = lb->r_uint32();

    printf("New cable<%" PRIu8 ", %"PRIu32"+%"PRIu32"\n", ctype, id, id_modifier);

    if (version >= 11) {
        printf("Extra length: %.2f\n", lb->r_float());
    }

    if (version >= LEVEL_VERSION_1_5) {
        printf("Saved length: %.2f\n", lb->r_float());
    }

    if (version >= LEVEL_VERSION_1_0) {
        printf("Moveable? %s\n", lb->r_uint8() ? "yes" : "no");
    }

    printf("Reading plug data...\n");
    for (int x=0; x<2; x++) {
        printf("Plug %d Entity ID: %"PRIu32"+%"PRIu32"\n", x, lb->r_uint32(), id_modifier);

        printf("Plug %d socket index: %" PRIu8 "\n", x, lb->r_uint8());

        printf("Plug %d Pos X: %.2f+%.2f\n", x, lb->r_float(), displacement.x);
        printf("Plug %d Pos Y: %.2f+%.2f\n", x, lb->r_float(), displacement.y);
    }

    printf(">\n");
}

static void
print_connection(lvlbuf *lb, int version, uint64_t flags, uint32_t id_modifier=0, b2Vec2 displacement=b2Vec2(0.f,0.f))
{
    uint8_t ctype = lb->r_uint8();
    uint32_t e_id = lb->r_uint32();
    uint32_t o_id = lb->r_uint32();

    printf("New connection<%"PRIu8 "\n", ctype);
    printf("Entity ID: %"PRIu32"+%"PRIu32"\n", e_id, id_modifier);
    printf("Other  ID: %"PRIu32"\n", o_id);

    uint32_t chunk_pos_x = 0;
    uint32_t chunk_pos_y = 0;
    uint32_t e_data = 0;
    uint32_t o_data = 0;

    if (version >= LEVEL_VERSION_1_5) {
        printf("Chunk Pos X: %"PRIu32"\n", lb->r_uint32());
        printf("Chunk Pos Y: %"PRIu32"\n", lb->r_uint32());
        printf("Chunk e data: %"PRIu32"\n", lb->r_uint32());
        printf("Chunk o data: %"PRIu32"\n", lb->r_uint32());
    }

    printf("Owned? %s\n", lb->r_uint8() ? "yes" : "no");
    printf("Fixed? %s\n", lb->r_uint8() ? "yes" : "no");
    printf("o_index: %"PRIu8 "\n", lb->r_uint8());
    printf("Local X: %.2f\n", lb->r_float());
    printf("Local Y: %.2f\n", lb->r_float());

    printf("p_s X (whatever this is): %.2f\n", lb->r_float());
    printf("p_s Y (whatever this is): %.2f\n", lb->r_float());

    printf("Frame 0: %"PRIu8 "\n", lb->r_uint8());
    printf("Frame 1: %"PRIu8 "\n", lb->r_uint8());

    if (version >= 4) {
        printf("Max force: %f\n", lb->r_float());
    }

    if (version >= 5) {
        printf("Option: %" PRIu8 "\n", lb->r_uint8());
    }

    if (version >= 8) {
        printf("Damping: %.2f\n", lb->r_float());
    }

    if (version >= 14) {
        printf("Angle: %.2f\n", lb->r_float());
        printf("Render type: %" PRIu8 "\n", lb->r_uint8());
    }

    if (version >= LEVEL_VERSION_1_2_4) {
        printf("Relative angle: %.2f\n", lb->r_float());
    }

    printf(">\n");
}

static bool
load_buffer(const lvlinfo &level, lvlbuf *buf, uint32_t id_modifier=0, b2Vec2 displacement=b2Vec2(0.f,0.f))
{
    /* XXX keep in sync with chunk_preloader::preload() */
    uint32_t num_entities, num_groups, num_connections, num_cables, n_read = 0, num_chunks = 0;

    num_groups = level.num_groups;
    num_entities = level.num_entities;
    num_connections = level.num_connections;
    num_cables = level.num_cables;

    for (n_read = 0; (!buf->eof() && n_read < num_groups); n_read++) {
        print_group(buf, level.version, id_modifier, displacement);
    }

    for (n_read = 0; (!buf->eof() && n_read < num_entities); n_read++) {
        bool ret = print_entity(buf, level.version, id_modifier, displacement);

        if (!ret) {
            tms_errorf("error while reading entities from level file");
            return false;
        }
    }

    for (n_read = 0; (!buf->eof() && n_read < num_cables); n_read++) {
        print_cable(buf, level.version, level.flags, id_modifier, displacement);
    }

    for (n_read = 0; (!buf->eof() && n_read < num_connections); n_read++) {
        print_connection(buf, level.version, level.flags, id_modifier, displacement);
    }

    return true;
}

static void
print_level_data(const lvlinfo &level, lvlbuf *lb)
{
    /* After the level header has been read, we need to hop along to the data! */
    /* If there is any state data, that is. */
    lb->rp += level.state_size;

    load_buffer(level, lb);

    if (level.flag_active(LVL_CHUNKED_LEVEL_LOADING) == false) {
        /* preloader is always used to load and write chunks, disregarding chunked level loading flag */
        read_chunks(level, lb);
    }
}

int
main(int argc, char **argv)
{
    if (argc == 2) {
        lvlbuf lb;
        lvlinfo level;
        FILE *fh = fopen(argv[1], "rb");

        if (!fh) {
            fprintf(stderr, "Error opening file at path '%s'", argv[1]);
            return 1;
        }

        fseek(fh, 0, SEEK_END);
        long size = ftell(fh);
        fseek(fh, 0, SEEK_SET);

        printf("Level filesize: %ld\n", size);

        lb.ensure(size);

        fread(lb.buf, 1, size, fh);

        fclose(fh);

        lb.size = size;

        if (!level.read(&lb)) {
            tms_errorf("Error reading level headers!");
            return 0;
        }

        level.print();

        print_level_data(level, &lb);
    } else {
        printf("usage: lvlinfo <path>\n");

        return 1;
    }

    return 0;
}
