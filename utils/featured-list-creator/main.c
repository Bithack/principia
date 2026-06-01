#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <jansson.h>

void write_u32(FILE *f, uint32_t value) {
    fwrite(&value, sizeof(uint32_t), 1, f);
}

void write(FILE *f, const char* value) {
    fwrite(value, 1, strlen(value), f);
}

#define GET_JSON_INTEGER(obj, key) json_integer_value(json_object_get((obj), (key)))
#define GET_JSON_STRING(obj, key) json_string_value(json_object_get((obj), (key)))

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s [data.json] [fl.cache]\n", argv[0]);
        return 1;
    }

    const char *input_filename = argv[1];
    const char *output_filename = argv[2];

    json_t *root; json_error_t error;
    root = json_load_file(input_filename, 0, &error);
    if (!root) {
        printf("Error loading JSON file: %s\n", error.text);
        return 1;
    }

    json_t *featured_levels = json_object_get(root, "featured_levels");
    json_t *gettingstarted_list = json_object_get(root, "gettingstarted_list");

    if (json_array_size(featured_levels) > 4 || json_array_size(gettingstarted_list) > 12)
        printf("Array lengths are likely too large for Principia, but carry on.\n");

    FILE *f = fopen(output_filename, "wb+");
    if (!f) {
        printf("Error opening output file\n");
        return 1;
    }

    size_t index;
    json_t *feat_level, *gettingstarted;

    write_u32(f, json_array_size(featured_levels)); // featured_level_count
    json_array_foreach(featured_levels, index, feat_level) {
        uint32_t id = GET_JSON_INTEGER(feat_level, "id");
        const char *name = GET_JSON_STRING(feat_level, "name");
        const char *author = GET_JSON_STRING(feat_level, "author");
        const char *jpeg_image_path = GET_JSON_STRING(feat_level, "jpeg_image");

        FILE *jpeg_file = fopen(jpeg_image_path, "rb");
        if (!jpeg_file) {
            printf("Error opening JPEG image file %s\n", jpeg_image_path);
            fclose(f);
            return 1;
        }

        fseek(jpeg_file, 0, SEEK_END);
        long jpeg_size = ftell(jpeg_file);
        fseek(jpeg_file, 0, SEEK_SET);

        unsigned char *jpeg_stream = malloc(jpeg_size);
        fread(jpeg_stream, 1, jpeg_size, jpeg_file);
        fclose(jpeg_file);

        write_u32(f, id);             // fl_id
        write_u32(f, strlen(name));   // fl_name_size
        write(f, name);               // fl_name
        write_u32(f, strlen(author)); // fl_author_size
        write(f, author);             // fl_author
        write_u32(f, jpeg_size);      // fl_jpegstream_size
        fwrite(jpeg_stream, 1, jpeg_size, f); // fl_jpegstream

        free(jpeg_stream);
    }

    write_u32(f, 0); // Number of contests (unused and slightly broken)

    write_u32(f, json_array_size(gettingstarted_list)); // gettingstarted_list_count
    json_array_foreach(gettingstarted_list, index, gettingstarted) {
        const char *name = GET_JSON_STRING(gettingstarted, "name");
        const char *link = GET_JSON_STRING(gettingstarted, "link");

        write_u32(f, strlen(name)); // gs_name_size
        write(f, name);             // gs_name
        write_u32(f, strlen(link)); // gs_link_size
        write(f, link);             // gs_link
    }

    fclose(f);

    return 0;
}
