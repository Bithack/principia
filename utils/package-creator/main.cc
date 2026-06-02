#include "pkgman.hh"
#include <jansson.h>
#include <stdio.h>

#define GET_JSON_OBJ(obj, key) json_object_get((obj), (key))

#define GET_JSON_INTEGER(obj, key, def) json_integer_value(GET_JSON_OBJ((obj), (key)) ? GET_JSON_OBJ((obj), (key)) : json_integer((def)))
#define GET_JSON_STRING(obj, key) json_string_value(GET_JSON_OBJ((obj), (key)))

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("usage: package-creator [package-json] [package-file]\n");
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

    json_t *levels = json_object_get(root, "levels");

    // Create pkginfo object from JSON data
    pkginfo p;
    p.version = 3;

    p.community_id = GET_JSON_INTEGER(root, "community_id", 0);
    p.unlock_count = GET_JSON_INTEGER(root, "unlock_count", 2);
    p.first_is_menu = GET_JSON_INTEGER(root, "first_is_menu", 0);
    p.return_on_finish = GET_JSON_INTEGER(root, "return_on_finish", 0);

    strncpy(p.name, GET_JSON_STRING(root, "name"), 255);
    p.name[255] = '\0';

    int num_levels = json_array_size(levels);
    if (num_levels > 255) {
        printf("Too many levels in package (max 255)\n");
        return 1;
    }

    p.num_levels = num_levels;
    if (p.num_levels > 0)
        p.levels = (uint32_t*)malloc(p.num_levels * sizeof(uint32_t));

    for (size_t i = 0; i < p.num_levels; i++)
        p.levels[i] = (uint32_t)json_integer_value(json_array_get(levels, i));

    // Write the package to a file
    p.save_to_path(output_filename);

    return 0;
}
