#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <png.h>
#include "pkgman.hh"

static lvledit lvl;

static int write_png(const char *outpath, uint8_t *icon, int width, int height) {
    FILE *fp = fopen(outpath, "wb");
    if (!fp) {
        fprintf(stderr, "could not open output file %s\n", outpath);
        return 1;
    }

    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        fclose(fp);
        fprintf(stderr, "png_create_write_struct failed\n");
        return 1;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_write_struct(&png_ptr, NULL);
        fclose(fp);
        fprintf(stderr, "png_create_info_struct failed\n");
        return 1;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fclose(fp);
        fprintf(stderr, "libpng fatal error\n");
        return 1;
    }

    png_init_io(png_ptr, fp);

    png_set_IHDR(png_ptr, info_ptr, width, height, 8,
                 PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    png_write_info(png_ptr, info_ptr);

    for (int y = 0; y < height; ++y) {
        // the level icon is stored upside down (GL coord style?), so let's just
        // write rows in reverse order here...
        png_bytep row = (png_bytep)(icon + (height - 1 - y) * width);
        png_write_row(png_ptr, row);
    }

    png_write_end(png_ptr, NULL);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(fp);
    return 0;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: lvl-icon-extractor <level_path> <out.png>\n");
        return 1;
    }

    const char *path = argv[1];
    const char *out = argv[2];

    if (!lvl.open_from_path(path)) {
        fprintf(stderr, "could not open file %s\n", path);
        return 1;
    }

    const int ICON_W = 128;
    const int ICON_H = 128;

    if (write_png(out, lvl.lvl.icon, ICON_W, ICON_H) != 0) {
        fprintf(stderr, "failed to write png %s\n", out);
        return 1;
    }

    return 0;
}
