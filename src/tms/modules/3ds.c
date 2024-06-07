#include <tms/core/glob.h>
#include <tms/math/glob.h>

#define OBJ_NAME_MAX 32

struct vertex {
    tvec3 pos;
    tvec3 nor;
    tvec2 uv;
};

struct tms_mesh *
load_3ds_model(struct tms_model *model,
               SDL_RWops *fp, int *status)
{
    uint16_t chunk_id;
    uint32_t chunk_len;
    uint16_t num_items;
    char object_name[OBJ_NAME_MAX];

    struct vertex *vertex_buf = 0;
    uint16_t *index_buf = 0;
    uint16_t base_index;

    size_t num_vertices = 0;
    size_t num_indices = 0;
    size_t sz;

    long filesz;
    SDL_RWseek(fp, 0, SEEK_END);
    filesz = SDL_RWtell(fp);
    SDL_RWseek(fp, 0, SEEK_SET);

    tms_debugf("3DS FILE SIZE: %d", (int)filesz);

    while (SDL_RWtell(fp) < filesz) {
        SDL_RWread(fp, &chunk_id, 2, 1);
        SDL_RWread(fp, &chunk_len, 4, 1);

        //tms_debugf("chunk id: %x, chunk size: %d", chunk_id, chunk_len);

        switch (chunk_id) {
            case 0x4d4d:
            case 0x3d3d:
                break;

            case 0x4000: { /* object block */
                //tms_debugf("found object chunk");
                int x;
                for (x=0; x<OBJ_NAME_MAX-1; x++) {
                    SDL_RWread(fp, &object_name[x], 1, 1);

                    if (object_name[x] == '\0')
                        break;
                }
                object_name[x] = '\0';
                //tms_debugf("object name: %s", object_name);
            } break;

            case 0x4100:
                break;

            case 0x4110: /* vertices list */
                SDL_RWread(fp, &num_items, 2, 1);

                //tms_debugf("found vertices chunk, num items: %d", num_items);

                tms_assertf(num_vertices == 0, "multiple meshes per model not currently supported");
                num_vertices = num_items;

                sz = model->vertices->size;

                base_index = sz / sizeof(struct vertex);

                if (base_index > 0x7fff) {
                    //tms_infof("WARNING TOO MANY INDICES ----------------------------------------h");
                }
                //tms_infof("base index:%d", base_index);
                tms_gbuffer_realloc(model->vertices, sz + num_items * sizeof(struct vertex));
                vertex_buf = (struct vertex *)(model->vertices->buf+sz);

                for (int x=0; x<num_items; x++) {
                    SDL_RWread(fp, &vertex_buf[x].pos, 4, 3);
                    vertex_buf[x].nor = (tvec3){0,0,0};
                }
                break;

            case 0x4120: /* faces list */
                SDL_RWread(fp, &num_items, 2, 1);
                // tms_debugf("found faces chunk, num items: %d", num_items);
                tms_assertf(num_indices == 0, "face list specified more than once, not supported");
                num_indices = num_items * 3;

                sz = model->indices->size;
                tms_gbuffer_realloc(model->indices, sz+ num_indices * sizeof(uint16_t));
                index_buf = (uint16_t *)(model->indices->buf + sz);

                for (int x=0; x<num_items; x++) {
                    uint16_t _i[4];
                    SDL_RWread(fp, &_i, sizeof(uint16_t)*4, 1);

                    /* calculate this face's normal and update the vertices */
                    tvec3 a = vertex_buf[_i[0]].pos;
                    tvec3 b = vertex_buf[_i[1]].pos;
                    tvec3 c = vertex_buf[_i[2]].pos;

                    tvec3_sub(&b, &a);
                    tvec3_sub(&c, &a);
                    tvec3_cross(&a, b, c);

                    index_buf[x*3+0] = _i[0] + base_index;
                    index_buf[x*3+1] = _i[1] + base_index;
                    index_buf[x*3+2] = _i[2] + base_index;
                    tvec3_add(&vertex_buf[_i[0]].nor, &a);
                    tvec3_add(&vertex_buf[_i[1]].nor, &a);
                    tvec3_add(&vertex_buf[_i[2]].nor, &a);
                }

                break;

            case 0x4140: /* texture coordinates */
                SDL_RWread(fp, &num_items, 2, 1);

                // tms_debugf("found uv mapping chunk, num items: %d", num_items);
                tms_assertf(num_vertices != 0, "oops! texture coordinates specified before vertices list, unsupported at this time");
                //tms_assertf(num_items*sizeof(struct vertex) == vertices->size, "number texture coordinates does not match number of vertices");

                for (int x=0; x<num_items; x++)
                    SDL_RWread(fp, &vertex_buf[x].uv, 4, 2);

                break;

            default:
                SDL_RWseek(fp, chunk_len-6, SEEK_CUR);
                break;
        }
    }

    tms_assertf(num_indices > 0, "found no index list in 3ds file");
    tms_assertf(num_vertices > 0, "found no vertices list in 3ds file");

    /* loop through all vertices and normalize the normals */
    for (int x=0; x<num_vertices; x++)
        tvec3_normalize(&vertex_buf[x].nor);

    struct tms_mesh *mesh = tms_model_create_mesh(model);

    mesh->i_start = ((char*)index_buf - model->indices->buf) / 2;// / sizeof(struct vertex);
    mesh->i_count = num_indices;// / sizeof(struct vertex);

    mesh->v_start = ((char*)vertex_buf - model->vertices->buf);
    mesh->v_count = num_vertices;

    //tms_infof("i start %d", ((char*)index_buf - model->indices->buf));

    *status = T_OK;
    return mesh;
}
