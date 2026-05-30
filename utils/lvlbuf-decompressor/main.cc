#include <stdio.h>
#include <stdlib.h>
#include "pkgman.hh"

static lvledit lvl;

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: lvlbuf-decompressor <level_path> <out_path>\n");
        return 1;
    }

    const char *in = argv[1];
    const char *out = argv[2];

    if (!lvl.open_from_path(in)) {
        fprintf(stderr, "could not open file %s\n", in);
        return 1;
    }

    // abort for older level versions
    if (lvl.lvl.version < LEVEL_VERSION_1_5) {
        fprintf(stderr, "Level versions older than 1.5 don't use compression for their level buffer, aborting\n");
        return 1;
    }

    if (lvl.lvl.compression_length > 0)
        lvl.lb.zuncompress(lvl.lvl);

    int header = lvl.lvl.get_size();
    uint64_t body_len = 0;
    if (lvl.lb.size > (uint64_t)header)
        body_len = lvl.lb.size - (uint64_t)header;

    FILE *fp = fopen(out, "wb");
    if (!fp) {
        fprintf(stderr, "could not open output file %s\n", out);
        return 1;
    }

    if (body_len)
        fwrite(lvl.lb.buf + header, 1, body_len, fp);

    fclose(fp);
    return 0;
}
