#include <tms/core/glob.h>

#include <stdlib.h>
#include <stdio.h>
#include <jpeglib.h>

static int load_jpeg(struct tms_texture *tex, FILE *f);

void
tmod_jpeg_init(void)
{
    tms_register_image_loader(load_jpeg, "jpg");
    tms_register_image_loader(load_jpeg, "jpeg");
}
    
static int
load_jpeg(struct tms_texture *tex, FILE *f)
{
    struct jpeg_decompress_struct jpeg;
    struct jpeg_error_mgr jerr;

    jpeg.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&jpeg);
    jpeg_stdio_src(&jpeg, f);

    if (jpeg_read_header(&jpeg, 1) != JPEG_HEADER_OK)
        return T_READ_ERROR;

    tex->width = jpeg.image_width;
    tex->height = jpeg.image_height;

    jpeg_start_decompress(&jpeg);

    if (!(tex->data = malloc(jpeg.output_height*jpeg.output_width*jpeg.output_components))) {
        jpeg_destroy_decompress(&jpeg);
        return T_OUT_OF_MEM;
    }

    while (jpeg.output_scanline < jpeg.output_height) {
        char *d = tex->data+jpeg.output_scanline*jpeg.output_components*jpeg.output_width;
        jpeg_read_scanlines(&jpeg, &d, 1);
    }

    tms_infof("JPEG num_channels: %d", jpeg.jpeg_color_space);

    jpeg_finish_decompress(&jpeg);
    jpeg_destroy_decompress(&jpeg);

    tex->num_channels = jpeg.jpeg_color_space;

    return T_OK;
}
