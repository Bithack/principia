#include "textbuffer.hh"
#include "material.hh"

static tms::gbuffer *verts;
static tms::gbuffer *verts2;
static tms::gbuffer *indices;
static tms::varray *va;
static tms::varray *va2;
static tms::mesh *mesh;
static tms::mesh *mesh2;
static tms::entity *e;
static tms::entity *e2;

static int n = 0;
static int n2 = 0;

struct vert {
    tvec3 pos;
    tvec2 uv;
    tvec4 color;
};

static struct vert base[4];

void textbuffer::_init()
{
    tms_infof("Initializing textbuffer...");

    verts = new tms::gbuffer(4*TEXTBUFFER_MAX*sizeof(struct vert));
    verts->usage = TMS_GBUFFER_STREAM_DRAW;

    verts2 = new tms::gbuffer(4*TEXTBUFFER_MAX*sizeof(struct vert));
    verts2->usage = TMS_GBUFFER_STREAM_DRAW;

    indices = new tms::gbuffer(6*TEXTBUFFER_MAX*sizeof(uint16_t));
    indices->usage = TMS_GBUFFER_STATIC_DRAW;

    va = new tms::varray(3);
    va->map_attribute("position", 3, GL_FLOAT, verts);
    va->map_attribute("uv", 2, GL_FLOAT, verts);
    va->map_attribute("vcolor", 4, GL_FLOAT, verts);

    va2 = new tms::varray(3);
    va2->map_attribute("position", 3, GL_FLOAT, verts2);
    va2->map_attribute("uv", 2, GL_FLOAT, verts2);
    va2->map_attribute("vcolor", 4, GL_FLOAT, verts2);

    uint16_t *i = (uint16_t*)indices->get_buffer();
    for (int x=0; x<TEXTBUFFER_MAX; x++) {
        int o = x*6;
        int vo = x*4;

        i[o+0] = vo;
        i[o+1] = vo+1;
        i[o+2] = vo+2;
        i[o+3] = vo;
        i[o+4] = vo+2;
        i[o+5] = vo+3;
    }

    indices->upload();

    base[0] = (struct vert){
        (tvec3){.5f,.5f,0.f},
        (tvec2){.5f, 1.f},
        (tvec4){0.f, 0.f, 0.f, 0.f},
    };
    base[1] = (struct vert){
        (tvec3){-.5f,.5f,0.f},
        (tvec2){0.f, 1.f},
        (tvec4){0.f, 0.f, 0.f, 0.f},
    };
    base[2] = (struct vert){
        (tvec3){-.5f,-.5f,0.f},
        (tvec2){0.f, .5f},
        (tvec4){0.f, 0.f, 0.f, 0.f},
    };
    base[3] = (struct vert){
        (tvec3){.5f,-.5f,0.f},
        (tvec2){.5f, .5f},
        (tvec4){0.f, 0.f, 0.f, 0.f},
    };

    reset();
}

void
textbuffer::reset()
{
    n = 0;
    n2 = 0;
}

void
textbuffer::upload()
{
    if (mesh) {
        mesh->i_start = 0;
        mesh->i_count = n*6;
    }
    if (mesh2) {
        mesh2->i_start = 0;
        mesh2->i_count = n2*6;
    }

    if (n) verts->upload_partial(n*4*sizeof(struct vert));
    if (n2) verts2->upload_partial(n2*4*sizeof(struct vert));
}

tms::entity *
textbuffer::get_entity()
{
    if (e) return e;

    e = new tms::entity();
    mesh = new tms::mesh(va, indices);

    e->prio = 3;
    e->set_mesh(mesh);
    e->set_material(&m_charbuf);

    mesh->i_start = 0;
    mesh->i_count = n*9*3;

    return e;
}

void
custom_update(struct tms_entity *e)
{
    tms_debugf("hello");
}

tms::entity *
textbuffer::get_entity2()
{
    if (e2) return e2;

    mesh2 = new tms::mesh(va2, indices);
    mesh2->i_start = 0;
    mesh2->i_count = n2*9*3;

    e2 = new tms::entity();
    e2->prio = 0;
    e2->set_mesh(mesh2);
    e2->set_material(&m_charbuf2);

    return e2;
}

void
textbuffer::add_char(glyph *gl,
        float x, float y, float z,
        float r, float g, float b, float a,
        float w, float h)
{
    tms_sprite *s = gl->sprite;
    int bx = 505;
    int by = 950;
    int tx = 530;
    int ty = 1024;

#define TEX_WIDTH  1024.f
#define TEX_HEIGHT 1024.f

    tvec2 uvb = {(float)bx / TEX_WIDTH, (float)by / TEX_HEIGHT};
    tvec2 uvt = {(float)tx / TEX_WIDTH, (float)ty / TEX_HEIGHT};

    if (n < TEXTBUFFER_MAX) {
        struct vert *_b = (struct vert*)verts->get_buffer();
        for (int ix=0; ix<4; ix++) {
            _b[n*4+ix] = base[ix];

            _b[n*4+ix].pos.x *= w;
            _b[n*4+ix].pos.y *= h;

            _b[n*4+ix].pos.x += x;
            _b[n*4+ix].pos.y += y;
            _b[n*4+ix].pos.z += z;

            _b[n*4+ix].color.r = r;
            _b[n*4+ix].color.g = g;
            _b[n*4+ix].color.b = b;
            _b[n*4+ix].color.a = a;

            float x = ((ix == 0 || ix == 3) ? s->tr.x : s->bl.x);
            float y = (ix < 2 ? s->tr.y : s->bl.y);

            _b[n*4+ix].uv.x = x;
            _b[n*4+ix].uv.y = y;
        }

        n++;
    }
}

void
textbuffer::add_bg(
        float x, float y, float z,
        float r, float g, float b, float a,
        float w, float h)
{
    if (n < TEXTBUFFER_MAX) {
        struct vert *_b = (struct vert*)verts->get_buffer();
        for (int ix=0; ix<4; ix++) {
            _b[n*4+ix] = base[ix];

            _b[n*4+ix].pos.x *= w;
            _b[n*4+ix].pos.y *= h;

            _b[n*4+ix].pos.x += x;
            _b[n*4+ix].pos.y += y;
            _b[n*4+ix].pos.z += z;

            _b[n*4+ix].color.r = r;
            _b[n*4+ix].color.g = g;
            _b[n*4+ix].color.b = b;
            _b[n*4+ix].color.a = a;

            _b[n*4+ix].uv.x = .001;
            _b[n*4+ix].uv.y = .999;
        }

        n++;
    }
}

void
textbuffer::add_char2(glyph *gl,
        float x, float y, float z,
        float r, float g, float b, float a,
        float w, float h)
{
    tms_sprite *s = gl->sprite;
    int bx = 505;
    int by = 950;
    int tx = 530;
    int ty = 1024;

#define TEX_WIDTH  1024.f
#define TEX_HEIGHT 1024.f

    tvec2 uvb = {(float)bx / TEX_WIDTH, (float)by / TEX_HEIGHT};
    tvec2 uvt = {(float)tx / TEX_WIDTH, (float)ty / TEX_HEIGHT};

    if (n2 < TEXTBUFFER_MAX) {
        struct vert *_b = (struct vert*)verts2->get_buffer();
        for (int ix=0; ix<4; ix++) {
            _b[n2*4+ix] = base[ix];

            _b[n2*4+ix].pos.x *= w;
            _b[n2*4+ix].pos.y *= h;

            _b[n2*4+ix].pos.x += x;
            _b[n2*4+ix].pos.y += y;
            _b[n2*4+ix].pos.z += z;

            _b[n2*4+ix].color.r = r;
            _b[n2*4+ix].color.g = g;
            _b[n2*4+ix].color.b = b;
            _b[n2*4+ix].color.a = a;

            float x = ((ix == 0 || ix == 3) ? s->tr.x : s->bl.x);
            float y = (ix < 2 ? s->tr.y : s->bl.y);

            _b[n2*4+ix].uv.x = x;
            _b[n2*4+ix].uv.y = y;
        }

        n2++;
    }
}

void
textbuffer::add_text(p_text *text, float scale)
{
    p_text::text_glyph *g = 0;
    for (int i=0; i<text->num_glyphs; ++i) {
        g = &text->glyphs[i];

        textbuffer::add_char(g->parent,
                g->x, g->y,
                2.0,
                1.0, 1.0, 1.0, 1.0,
                scale * g->get_bw(),
                scale * g->get_bh()
                );
    }
}

void
textbuffer::add_text(const char *text, p_font *font,
        float x, float y, float z,
        float r, float g, float b, float a,
        float scale,
        uint8_t horizontal_align/*=ALIGN_CENTER*/,
        uint8_t vertical_align/*=ALIGN_CENTER*/,
        bool bg/*=false*/
        )
{
    float width = 0.f;
    float max_width = 0.f;
    float line_height = font->get_height() * scale;
    float height = line_height;
    int slen = strlen(text);
    struct glyph *gl;

    for (int i=0; i<slen; ++i) {
        if ((gl = font->get_glyph(text[i]))) {
            if (gl->newline) {
                height += line_height;
                width = 0.f;
                continue;
            }

            width += gl->ax * scale;

            if (width > max_width) {
                max_width = width;
            }
        }
    }

    if (width == 0.f) {
        height -= line_height;
    }

    width = max_width;

    float mid_x = x;
    float mid_y = y;

    if (horizontal_align == ALIGN_CENTER) {
        x -= max_width/2.f;
    } else if (horizontal_align == ALIGN_RIGHT) {
        x -= max_width;
    }

    float base_x = x;

    if (vertical_align == ALIGN_CENTER) {
        y += height/2.f;
    } else if (vertical_align == ALIGN_TOP) {
        y -= height;
    }

    if (bg) {
        textbuffer::add_bg(mid_x,mid_y+line_height*.75f,z,0,0,0,.5*a, max_width+.1, height+.1);
    }

    for (int i=0; i<slen; ++i) {
        if ((gl = font->get_glyph(text[i]))) {
            float x2 = x + (gl->bl + gl->bw/2.f) * scale;
            float y2 = y + (gl->bt - gl->bh/2.f) * scale;

            if (gl->newline) {
                x = base_x;
                y -= line_height;
                continue;
            }

            x += gl->ax * scale;
            textbuffer::add_char(gl,
                    x2,
                    y2,
                    z + 0.01f,
                    r, g, b, a,
                    gl->bw*scale,
                    gl->bh*scale
                    );
        }
    }
}

void
textbuffer::add_text2(const char *text, p_font *font,
        float x, float y, float z,
        float r, float g, float b, float a,
        float scale,
        uint8_t horizontal_align/*=ALIGN_CENTER*/,
        uint8_t vertical_align/*=ALIGN_CENTER*/
        )
{
    float width = 0.f;
    float height = font->get_height() * scale;
    int slen = strlen(text);
    struct glyph *gl;

    for (int i=0; i<slen; ++i) {
        if ((gl = font->get_glyph(text[i]))) {
            width += gl->ax * scale;
        }
    }

    if (horizontal_align == ALIGN_CENTER) {
        x -= width/2.f;
    } else if (horizontal_align == ALIGN_RIGHT) {
        x -= width;
    }

    if (vertical_align == ALIGN_CENTER) {
        y -= height/2.f;
    } else if (vertical_align == ALIGN_TOP) {
        y -= height;
    }

    for (int i=0; i<slen; ++i) {
        if ((gl = font->get_glyph(text[i]))) {
            float x2 = x + (gl->bl + gl->bw/2.f) * scale;
            float y2 = y + (gl->bt - gl->bh/2.f) * scale;

            x += gl->ax * scale;
            textbuffer::add_char2(gl,
                    x2,
                    y2,
                    z + 0.01f,
                    .0f, .0f, .0f, 1.f,
                    gl->bw*scale,
                    gl->bh*scale
                    );
        }
    }
}
