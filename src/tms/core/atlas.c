#include "atlas.h"
#include "backend.h"

struct tms_atlas *
tms_atlas_alloc(int width, int height, int num_channels)
{
    struct tms_atlas *t = calloc(1, sizeof (struct tms_atlas));
    tms_texture_init(&t->texture);
    tms_texture_alloc_buffer(&t->texture, width, height, num_channels);
    tms_texture_clear_buffer(&t->texture, 0);

    return t;
}

static struct tms_sprite * create_sprite(struct tms_atlas *t,
                                         int x, int y, int width, int height,
                                         int num_chans, unsigned char *data)
{
    struct tms_sprite *sprite;
    int rowz;

    if (!t || !(sprite = malloc(sizeof (struct tms_sprite))))
        return 0;

    rowz = t->texture.width*t->texture.num_channels;

    if (num_chans > t->texture.num_channels) {
        tms_fatalf("mismatching number of channels (%d, %d)", num_chans, t->texture.num_channels);
    }

    if (height < 0) {
        /* invert height */
        height = -height;
        for (int py=0; py<height; py++) {
            for (int px=0; px<width; px++) {
                for (int pz=0; pz<t->texture.num_channels; pz++) {

                    t->texture.data[rowz*(t->texture.height - 1 - (height-py+y))
                                 + ((x+px)*t->texture.num_channels)+pz]
                        = data[py*width*num_chans + px*num_chans + pz%num_chans];
                }
            }
        }
    } else {
        for (int py=0; py<height; py++) {
            for (int px=0; px<width; px++) {
                for (int pz=0; pz<t->texture.num_channels; pz++) {

                    t->texture.data[rowz*(t->texture.height - 1 - (py+y))
                                 + ((x+px)*t->texture.num_channels)+pz]
                        = data[py*width*num_chans + px*num_chans + pz%num_chans];
                }
            }
        }
    }

    sprite->width = width;
    sprite->height = height;

    sprite->bl = (tvec2){
        (double)(x) / (double)t->texture.width,
        1.f - (double)(y + height) / (double)t->texture.height,
    };

    sprite->tr = (tvec2){
        (double)(x + width) / (double)t->texture.width,
        1.f - (double)(y) / (double)t->texture.height,
    };

    //tms_infof("created sprite: %d %d at %d %d", width, height, x, y);
    //tms_infof("         bl tr: %f %f, %f %f", sprite->bl.x, sprite->bl.y, sprite->tr.x, sprite->tr.y);

    return sprite;
}

struct tms_sprite *
tms_atlas_add_file(struct tms_atlas *t, const char *filename, int invert_y)
{
    struct tms_texture *tx = tms_texture_alloc();
    struct tms_sprite *ret = 0;
    if (tms_texture_load(tx, filename) == T_OK) {

        if (tx->num_channels == t->texture.num_channels) {
            tms_texture_flip_y(tx);
            ret = tms_atlas_add_bitmap(t, tx->width, tx->height, t->texture.num_channels, tx->data);
        } else {
            tms_errorf("Mismatching number of channels %s", filename);
        }

        tms_texture_free(tx);
    } else {
        tms_errorf("Error loading texture %s", filename);
    }

    return ret;
}

/**
 * Add the given bitmap to the spritesheet and return a sprite object.
 *
 * @returns a tms_sprite mapping to the added bitmap in the atlas,
 *          or 0 if the bitmap did not fit.
 **/
struct tms_sprite *
tms_atlas_add_bitmap(struct tms_atlas *t,
                             int width, int height, int num_channels,
                             unsigned char *data)
{
    int space_x, space_y,
        test_x, test_y;

    int r_height = height;
    height = height < 0 ? -height : height;

    if (width > t->texture.width || height > t->texture.height) {
        return 0;
    }

    test_x = t->current_x;
    test_y = t->current_y;

    space_x = t->texture.width - t->current_x;

    if (space_x < (width + t->padding_x)) {
        /* jump to the next "row" */
        //tms_infof("jump to next %d", t->current_height);
        test_y = t->current_y + t->current_height;
        t->current_height = 1;
        test_x = 0;
    }

    space_y = t->texture.height - test_y;

    if (space_y < (height + t->padding_y)) {
        return 0;
    }

    if (t->current_y + height + t->padding_y > t->current_y + t->current_height) {
        //t->current_height = height;
        //t->current_height = t->current_y + height + t->padding_y;
        t->current_height = height + t->padding_y;
    }

    t->current_x = test_x+width+t->padding_x;
    t->current_y = test_y;

    //tms_infof("x %d y %d test_y %d", test_x, t->current_y, test_y);

    return create_sprite(t, test_x, test_y, width, r_height, num_channels, data);
}

void
tms_atlas_free(struct tms_atlas *a)
{
    tms_texture_free_buffer(&a->texture);
    free(a);
}
