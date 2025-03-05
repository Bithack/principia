#include "tiles.hh"
#include "object_factory.hh"

#include <stdio.h>
#include <tms/backend/print.h>
#include <string.h>

struct tile_load_data tile_factory::tiles[NUM_TILES] = {
    { "data/tiles/nomad_hideout_0.txt" },
    { "data/tiles/nomad_hideout_1.txt" },
};

enum section_type {
    SECTION_NONE,

    SECTION_HEADER,
    SECTION_TILESETS,
    SECTION_LAYER,
    SECTION_OBJECT,
};

static bool
read_kv(char *buf, char **key, char **value)
{
    char *tok = strchr(buf, '=');
    if (tok) {
        *key = buf;
        *value = tok+1;

        if (tok) {
            *tok = 0;
        }

        return true;
    }

    return false;
}

void
tile_factory::init()
{
    for (int x=0; x<NUM_TILES; ++x) {
        tile_factory::tiles[x].tm = 0;

        tile_factory::tiles[x].tm = new tilemap();
        tilemap *tm = tile_factory::tiles[x].tm;

        FILE *fh = fopen(tile_factory::tiles[x].path, "rb");
        if (!fh) {
            tms_errorf("Error opening tilemap at %s", tile_factory::tiles[x].path);
            continue;
        }

        tm->width = -1;
        tm->height = -1;

        char buf[512];

        section_type section = SECTION_NONE;
        int layer_id = -1;
        bool reading_data = false;
        int i = 0;

        int object_type = -1;
        tvec2 object_location;

        while (fgets(buf, 512, fh) != NULL) {
            size_t sz = strlen(buf);

            buf[sz-1] = 0;

            if (sz < 2) continue;
            if (buf[0] == '#' || buf[0] == '\n' || buf[0] == ' ') continue;

            if (buf[0] == '[') {
                layer_id = -1;

                section_type new_section = SECTION_NONE;

                if (strcmp(buf, "[header]") == 0) {
                    new_section = SECTION_HEADER;
                } else if (strcmp(buf, "[tilesets]") == 0) {
                    new_section = SECTION_TILESETS;
                } else if (strcmp(buf, "[layer]") == 0) {
                    new_section = SECTION_LAYER;
                } else if (strcmp(buf, "[object]") == 0) {
                    new_section = SECTION_OBJECT;
                }

                if (new_section != SECTION_NONE) {
                    section = new_section;
                }
            }

            switch (section) {
                case SECTION_HEADER:
                    {
                        char *key, *value;

                        if (read_kv(buf, &key, &value)) {
                            if (tm->width == -1 && strcmp(key, "width") == 0) {
                                tm->width = atoi(value);
                            } else if (tm->height == -1 && strcmp(key, "height") == 0) {
                                tm->height = atoi(value);
                            }
                        }
                    }
                    break;

                case SECTION_LAYER:
                    {
                        if (layer_id == -1 && strchr(buf, '=') == NULL) continue;

                        char *key, *value;

                        if (read_kv(buf, &key, &value)) {
                            reading_data = false;
                            if (strcmp(key, "type") == 0) {
                                layer_id = atoi(value);
                                if (layer_id < 0 || layer_id > 2) {
                                    layer_id = -1;
                                    continue;
                                }
                            } else if (reading_data == false && layer_id != -1 && strcmp(key, "data") == 0) {
                                reading_data = true;
                                i = 0;
                                tm->layers[layer_id] = (int*)calloc(tm->width*tm->height, sizeof(int));
                            }
                        }

                        if (reading_data) {
                            char *pch = strtok(buf, ",");
                            while (pch != NULL) {
                                if (pch && i < tm->width*tm->height) {
                                    tm->layers[layer_id][i++] = atoi(pch);
                                }

                                pch = strtok(NULL, ",");
                            }
                        }
                    }
                    break;

                case SECTION_OBJECT:
                    {
                        char *key, *value;

                        if (read_kv(buf, &key, &value)) {

                            if (object_type == -1 && strcmp(key, "type") == 0) {
                                object_type = atoi(value);
                            } else if (strcmp(key, "location") == 0) {
                                int n = 0;
                                char *pch = strtok(value, ",");
                                while (pch != NULL) {
                                    switch (n) {
                                        case 0:
                                            object_location.x = atof(pch);
                                            break;
                                        case 1:
                                            object_location.y = atof(pch);
                                            break;
                                        case 2:
                                            object_location.x += atof(pch)/1.5f;
                                            break;
                                        case 3:
                                            //object_location.y -= atof(pch)/2.f;
                                            break;
                                    }

                                    if (++n == 4) break;
                                    pch = strtok(NULL, ",");
                                }

                                if (object_type >= 0 && object_type < MAX_OF_ID) {
                                    const struct tile_object to = {
                                        object_type,
                                        object_location,
                                    };

                                    tm->entities.push_back(to);
                                }

                                object_type = -1;
                            }
                        }
                    }
                    break;

                case SECTION_TILESETS:
                case SECTION_NONE:
                    break;
            }
        }

        fclose(fh);
    }
}
