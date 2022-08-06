#include <png.h>
#include <tms/core/glob.h>

static int load_png(struct tms_texture *tex, FILE *fp);

void
tmod_png_init(void)
{
    tms_register_image_loader(load_png, "png");
}


static int
load_png(struct tms_texture *tex, FILE *fp)
{
    char header[8];
    png_struct *png = 0;
    png_info *info = 0;
    int passes;

    fread(header, 1, 8, fp);
    if (png_sig_cmp(header, 0, 8) != 0)
        goto error;

    png = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
    info = png_create_info_struct(png);

    if (setjmp(png_jmpbuf(png))) {
        png_destroy_read_struct(&png, &info, 0);
        return T_READ_ERROR;
    }

    png_init_io(png, fp);
    png_set_sig_bytes(png, 8);

    png_read_png(png, info, PNG_TRANSFORM_EXPAND, 0);

    tex->width = png_get_image_width(png, info);
    tex->height = png_get_image_height(png, info);
    tex->num_channels = (png_get_color_type(png, info) == PNG_COLOR_TYPE_RGBA ? 4 : 3); 

    tms_debugf("png dimensions: %dx%d", tex->width, tex->height);

    unsigned rbytes = png_get_rowbytes(png, info);
    (tex->data = malloc(rbytes*tex->height))
        || tms_fatalf("out of mem (png)");
    
    png_bytepp rp = png_get_rows(png, info);
    for (int x=0; x<tex->height; x++) {
        /* swap row order since opengl uses vertically flipped order */
        memcpy(tex->data+rbytes*(tex->height-1-x), rp[x], rbytes);
    }

    png_destroy_read_struct(&png, &info, 0);

    return T_OK;

error:
    /* XXX */
    tms_fatalf("png error");
    return T_READ_ERROR;
}
