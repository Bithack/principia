#include "polygon.hh"
#include "world.hh"
#include "game.hh"
#include "ui.hh"

struct tms_varray  *va;
struct tms_gbuffer *vbuf;
struct tms_gbuffer *ibuf;
polygon            *slots[MAX_POLYGONS];
bool                modified; /* if the vertex buffer has been modified */

struct vertex {
    tvec3 pos;
    tvec3 nor;
    tvec3 col;
};

void
polygon::_init()
{
    vbuf = tms_gbuffer_alloc(20 * MAX_POLYGONS * sizeof(struct vertex));
    vbuf->usage = GL_STATIC_DRAW;

    ibuf = tms_gbuffer_alloc(90 * MAX_POLYGONS * sizeof(uint16_t));

    va = tms_varray_alloc(3);
    tms_varray_map_attribute(va, "position", 3, GL_FLOAT, vbuf);
    tms_varray_map_attribute(va, "normal", 3, GL_FLOAT, vbuf);
    tms_varray_map_attribute(va, "color", 3, GL_FLOAT, vbuf);

    uint16_t *i = (uint16_t*)tms_gbuffer_get_buffer(ibuf);

    /* 5 sides, last one is front */
    for (int x=0; x<5; x++) {
        i[x*6+0] = x*4+0;
        i[x*6+1] = x*4+1;
        i[x*6+2] = x*4+2;
        i[x*6+3] = x*4+0;
        i[x*6+4] = x*4+2;
        i[x*6+5] = x*4+3;
    }
    /* we're at 5*6 = 30 */

    /* 4 small triangles at the corners */
    for (unsigned x=0; x<4; x++) {
        i[30 + x*3 + 0] = ((x-1)%4)*4 + 2;
        i[30 + x*3 + 1] = ((x)%4)*4 + 3;
        i[30 + x*3 + 2] = 16 + x; /* connected to front face */
    }

    /* we're at 30 + 4*3 = 42 */
    /* connect sides with each other and the front*/
    for (unsigned x=0; x<4; x++) {
        i[42 + x*12 + 0] = ((x-1)%4)*4 + 1;
        i[42 + x*12 + 1] = ((x)%4)*4 + 0;
        i[42 + x*12 + 2] = ((x)%4)*4 + 3;
        i[42 + x*12 + 3] = ((x-1)%4)*4 + 1;
        i[42 + x*12 + 4] = ((x)%4)*4 + 3;
        i[42 + x*12 + 5] = ((x-1)%4)*4 + 2;
        /* and to the front */
        i[42 + x*12 + 6] = ((x)%4)*4 + 3;
        i[42 + x*12 + 7] = ((x)%4)*4 + 2;
        i[42 + x*12 + 8] = 16 + (1+x)%4;
        i[42 + x*12 + 9] = ((x)%4)*4 + 3;
        i[42 + x*12 +10] = 16 + (1+x)%4;
        i[42 + x*12 +11] = 16 + (x)%4;
    }

    /* duplicate the index list */
    for (int y=1; y<MAX_POLYGONS; y++) {
        for (int x=0; x<90; x++) {
            i[y*90+x] = i[x]+20*y;
        }
    }

    tms_gbuffer_upload(ibuf);
}

void
polygon::upload_buffers()
{
    if (modified) {
        tms_gbuffer_upload(vbuf); /* XXX TODO upload partial size */
    }
}

polygon::polygon(int material_type)
{
    this->next = 0;
    this->slot = -1;

    tms_mesh_init(&this->_mesh, va, ibuf);
    this->_mesh.i_start = 0;
    this->_mesh.i_count = 0;

    this->set_mesh(&this->_mesh);

    this->set_flag(ENTITY_HAS_CONFIG, true);
    this->set_flag(ENTITY_DO_TICK, true);
    this->set_flag(ENTITY_IS_RESIZABLE, true);

    this->num_sliders = 1;

    this->dialog_id = DIALOG_POLYGON;

    this->material_type = material_type;

    switch (this->material_type) {
        case MATERIAL_PLASTIC:
        default:
            this->set_material(&m_pixel); /* XXX properties do not match plastic */
            break;
    }

    this->set_num_properties(
              1 /* sublayer depth */
            + 1 /* sublayer alignment */
            + 1 /* density scale */
            + 4 * (2 + 3)); /* four corners * x, y, r, g, b */
    this->properties[0].type = P_INT8;
    this->properties[0].v.i8 = 3;
    this->properties[1].type = P_INT8;
    this->properties[1].v.i8 = 0;
    this->properties[2].type = P_FLT;
    this->properties[2].v.f = 1.f;

    b2Vec2 def[] = {
        b2Vec2(1.f, 1.f),
        b2Vec2(-1.f, 1.f),
        b2Vec2(-1.f, -1.f),
        b2Vec2(1.f, -1.f),
    };

    tvec3 def_color = (tvec3){.3f, .3f, .5f};

    for (int x=0; x<4; ++x) {
        this->properties[3 + (x * 5) + 0].type = P_FLT;
        this->properties[3 + (x * 5) + 0].v.f = def[x].x;
        this->properties[3 + (x * 5) + 1].type = P_FLT;
        this->properties[3 + (x * 5) + 1].v.f = def[x].y;
        this->properties[3 + (x * 5) + 2].type = P_FLT;
        this->properties[3 + (x * 5) + 2].v.f = def_color.r;
        this->properties[3 + (x * 5) + 3].type = P_FLT;
        this->properties[3 + (x * 5) + 3].v.f = def_color.g;
        this->properties[3 + (x * 5) + 4].type = P_FLT;
        this->properties[3 + (x * 5) + 4].v.f = def_color.b;
    }

    this->do_recreate_shape = false;
    this->set_shape();

    this->c_side[0].init_owned(0, this);
    this->c_side[0].type = CONN_GROUP;
    this->c_side[1].init_owned(1, this);
    this->c_side[1].type = CONN_GROUP;
    this->c_side[2].init_owned(2, this);
    this->c_side[2].type = CONN_GROUP;
    this->c_side[3].init_owned(3, this);
    this->c_side[3].type = CONN_GROUP;

    float qw = .5f/2.f+0.15f;
    float qh = .5f/2.f+0.15f;
    this->query_sides[0].Set(0.f,  qh); /* up */
    this->query_sides[1].Set(-qw, 0.f); /* left */
    this->query_sides[2].Set(0.f, -qh); /* down */
    this->query_sides[3].Set( qw, 0.f); /* right */
}

polygon::~polygon()
{
    this->remove_from_slot();
}

void
polygon::on_load(bool created, bool has_state)
{
    this->set_shape();
    this->reassign_slot(false);
    this->update_mesh();
}

void
polygon::remove_from_slot()
{
    if (this->slot != -1) {
        polygon *c = slots[this->slot];

        if (c == this) {
            slots[this->slot] = this->next;
        } else {
            do {
                if (c->next == this) {
                    c->next = this->next;
                    break;
                }
            } while ((c = c->next));
        }
    }

    this->next = 0;
    this->slot = -1;
}

void
polygon::add_to_slot(int n)
{
    if (!slots[n]){
        slots[n] = this;
    } else {
        polygon *c = slots[n];

        do {
            if (c->next == 0) {
                c->next = this;
                break;
            }
        } while ((c = c->next));
    }

    this->slot = n;
}

void
polygon::reassign_slot(bool changed)
{
    if (this->slot != -1) {
        if (slots[this->slot] == this && this->next == 0) {
            /* we're the only on in this slot, don't need to do anything */
            return;
        } else {
            if (changed) {
                /* we've changed our data, easiest solution is to find a new slot */
                tms_debugf("we changed and wasn't alone in this slot, removing from slot");
                this->remove_from_slot();
            } else {
                /* we didn't change, we already have a slot, do nothing */
                return;
            }
        }
    }

    int x;
    for (x=0; x<MAX_POLYGONS; x++) {
        /* reuse a slot if the vertices are identical or if the slot is unused */
        if (slots[x] == 0) break;

        /* compare our corner properties with the first one in this slot */
        polygon *o = slots[x];

        bool match = true;

        if ((match = (o->properties[0].v.i8 == this->properties[0].v.i8 && o->properties[1].v.i8 == this->properties[1].v.i8))) {
            for (int c=0; c<4; c++) {
                if (o->properties[3+c*5+0].v.f != this->properties[3+c*5+0].v.f
                    || o->properties[3+c*5+1].v.f != this->properties[3+c*5+1].v.f
                    || o->properties[3+c*5+2].v.f != this->properties[3+c*5+2].v.f
                    || o->properties[3+c*5+3].v.f != this->properties[3+c*5+3].v.f
                    || o->properties[3+c*5+4].v.f != this->properties[3+c*5+4].v.f
                    ) {
                    match = false;
                    break;
                }
            }
        }

        if (match) {
            tms_debugf("found a perfect match, reusing slot");
            break;
        }
    }

    if (x < MAX_POLYGONS) {
        this->add_to_slot(x);
        this->mesh->i_start = 90 * x;
        this->mesh->i_count = 90;
    }

    tms_debugf("assigned slot %d", this->slot);
}

void
polygon::setup()
{
    this->do_recreate_shape = false;
}

void
polygon::on_slider_change(int s, float value)
{
    this->help_set_density_scale(value);
    G->show_numfeed(this->properties[2].v.f);
}

void
polygon::tick()
{
    if (this->do_recreate_shape) {
        this->set_shape();
        this->reassign_slot(true);
        this->update_mesh();
        this->do_recreate_shape = false;
    }
}

void
polygon::on_pause()
{
    this->setup();
}

void
polygon::set_shape()
{
    b2Vec2 verts[4];

    for (int x=0; x<4; x++) {
        verts[x].x = this->properties[3 + (x*5) + 0].v.f;
        verts[x].y = this->properties[3 + (x*5) + 1].v.f;
    }

    this->set_as_poly(verts, 4);

    uint8_t d = this->properties[0].v.i8;
    if (this->properties[1].v.i8) {
        /* front alignment */
        this->layer_mask = 1 + (d > 0)*2 + (d > 1)*4 + (d > 2) * 8;
    } else {
        /* back alignment */
        this->layer_mask = 8 + (d > 0)*4 + (d > 1)*2 + (d > 2) * 1;
    }

    if (this->body) this->recreate_shape();
}

const b2Vec2
get_midpoint(const b2Vec2& p1, const b2Vec2& p2)
{
    return b2Vec2((p1.x+p2.x)/2.f, (p1.y+p2.y)/2.f);
}

void
polygon::update_mesh()
{
    if (this->slot == -1) {
        this->mesh->i_count = 0;
        this->mesh->i_start = 0;
        return;
    }

    struct vertex *v = (struct vertex*)tms_gbuffer_get_buffer(vbuf);
    v += this->slot*20;

    float depth = (std::min((int)this->properties[0].v.i8, 3)+1.f) * .25f;
    bool front_align = this->properties[1].v.i8;

    float back_z = front_align ? .5f - depth : -.5f;
    float front_z = front_align ? .5f : -.5f + depth;
    static const float b = .05; /* bevel */

    property *p = this->properties; /* just to make things cleaner */

    for (int x=0; x<4; x++) {
        tvec3 n = (tvec3){0.f, 0.f, 0.f};

        /* second point - first point, rotated 90 degrees clockwise */
        n.x = (p[3 + ((x+1)%4)*5 + 1].v.f-p[3 + (x%4)*5 + 1].v.f);
        n.y = -(p[3 + ((x+1)%4)*5 + 0].v.f-p[3 + (x%4)*5 + 0].v.f);
        tvec2_normalize((tvec2*)&n);

        v[x*4 + 0].pos = (tvec3){p[3 + (x%4)*5 + 0].v.f + n.x*b, p[3 + (x%4)*5 + 1].v.f+n.y*b, back_z};
        v[x*4 + 0].nor = n;
        v[x*4 + 0].col = (tvec3){p[3 + (x%4)*5 + 2].v.f, p[3 + (x%4)*5 + 3].v.f, p[3 + (x%4)*5 + 4].v.f};

        v[x*4 + 1].pos = (tvec3){p[3 + ((x+1)%4)*5 + 0].v.f+n.x*b, p[3 + ((x+1)%4)*5 + 1].v.f+n.y*b, back_z};
        v[x*4 + 1].nor = n;
        v[x*4 + 1].col = (tvec3){p[3 + ((x+1)%4)*5 + 2].v.f, p[3 + ((x+1)%4)*5 + 3].v.f, p[3 + ((x+1)%4)*5 + 4].v.f};

        v[x*4 + 2].pos = (tvec3){p[3 + ((x+1)%4)*5 + 0].v.f+n.x*b, p[3 + ((x+1)%4)*5 + 1].v.f+n.y*b, front_z};
        v[x*4 + 2].nor = n;
        v[x*4 + 2].col = (tvec3){p[3 + ((x+1)%4)*5 + 2].v.f, p[3 + ((x+1)%4)*5 + 3].v.f, p[3 + ((x+1)%4)*5 + 4].v.f};

        v[x*4 + 3].pos = (tvec3){p[3 + (x%4)*5 + 0].v.f+n.x*b, p[3 + (x%4)*5 + 1].v.f+n.y*b, front_z};
        v[x*4 + 3].nor = n;
        v[x*4 + 3].col = (tvec3){p[3 + (x%4)*5 + 2].v.f, p[3 + (x%4)*5 + 3].v.f, p[3 + (x%4)*5 + 4].v.f};

        if (_tms.gamma_correct) {
            for (int y=0; y<4; y++) {
                v[x*4+y].col.x = powf(v[x*4+y].col.x, _tms.gamma);
                v[x*4+y].col.y = powf(v[x*4+y].col.y, _tms.gamma);
                v[x*4+y].col.z = powf(v[x*4+y].col.z, _tms.gamma);
            }
        }
    }

    /* front side */
    tvec3 n = (tvec3){0.f, 0.f, 1.f};
    v[16 + 0].pos = (tvec3){p[3 + 0*5 + 0].v.f, p[3 + 0*5 + 1].v.f, front_z+b};
    v[16 + 0].nor = n;
    v[16 + 0].col = (tvec3){p[3 + 0*5 + 2].v.f, p[3 + 0*5 + 3].v.f, p[3 + 0*5 + 4].v.f};
    v[16 + 1].pos = (tvec3){p[3 + 1*5 + 0].v.f, p[3 + 1*5 + 1].v.f, front_z+b};
    v[16 + 1].nor = n;
    v[16 + 1].col = (tvec3){p[3 + 1*5 + 2].v.f, p[3 + 1*5 + 3].v.f, p[3 + 1*5 + 4].v.f};
    v[16 + 2].pos = (tvec3){p[3 + 2*5 + 0].v.f, p[3 + 2*5 + 1].v.f, front_z+b};
    v[16 + 2].nor = n;
    v[16 + 2].col = (tvec3){p[3 + 2*5 + 2].v.f, p[3 + 2*5 + 3].v.f, p[3 + 2*5 + 4].v.f};
    v[16 + 3].pos = (tvec3){p[3 + 3*5 + 0].v.f, p[3 + 3*5 + 1].v.f, front_z+b};
    v[16 + 3].nor = n;
    v[16 + 3].col = (tvec3){p[3 + 3*5 + 2].v.f, p[3 + 3*5 + 3].v.f, p[3 + 3*5 + 4].v.f};

    if (_tms.gamma_correct) {
        for (int y=0; y<4; y++) {
            v[16+y].col.x = powf(v[16+y].col.x, _tms.gamma);
            v[16+y].col.y = powf(v[16+y].col.y, _tms.gamma);
            v[16+y].col.z = powf(v[16+y].col.z, _tms.gamma);
        }
    }
    modified = true;
}

void
polygon::set_color(tvec4 c)
{
    int corner = G->get_selected_shape_corner();

    if (corner != -1) {
        this->properties[3+5*corner+2 + 0].v.f = c.r;
        this->properties[3+5*corner+2 + 1].v.f = c.g;
        this->properties[3+5*corner+2 + 2].v.f = c.b;
    }

    this->do_recreate_shape = true;
}

tvec4
polygon::get_color()
{
    int corner = G->get_selected_shape_corner();

    if (corner != -1) {
        float r = this->properties[3+5*corner+2 + 0].v.f;
        float g = this->properties[3+5*corner+2 + 1].v.f;
        float b = this->properties[3+5*corner+2 + 2].v.f;

        return tvec4f(r, g, b, 1.0f);
    } else {
        return tvec4f(0.f, 0.f, 0.f, 0.f);
    }
}

void
polygon::find_pairs()
{
    connection *c;

    b2PolygonShape *sh = this->get_resizable_shape();

    if (!sh) {
        return;
    }

    for (int x=0; x<4; ++x) {
        c = &this->c_side[x];

        const b2Vec2 &corner1 = this->local_to_world(sh->m_vertices[(x+1)%4], 0);
        const b2Vec2 &corner2 = this->local_to_world(sh->m_vertices[(x+2)%4], 0);
        const b2Vec2 &midpoint = get_midpoint(corner1, corner2);

        if (c->pending) {
            this->q_result = 0;

            const float dx = corner2.x-corner1.x;
            const float dy = corner2.y-corner1.y;

            b2Vec2 vec(dy, -dx);
            vec.Normalize();
            vec *= this->query_len;

            W->raycast(this, midpoint, midpoint+vec);

            if (this->q_result) {
                c->o = this->q_result;
                c->angle = atan2f(vec.y, vec.x);
                c->render_type = CONN_RENDER_SMALL;
                c->f[0] = 0;
                c->f[1] = this->q_frame;
                c->p = vec;
                c->p *= this->q_fraction * .5;
                c->p += midpoint;
                c->o_data = this->q_result->get_fixture_connection_data(this->q_result_fx);
                G->add_pair(this, this->q_result, c);
            }
        }
    }

}

float32
polygon::ReportFixture(b2Fixture *f, const b2Vec2 &pt, const b2Vec2 &nor, float32 fraction)
{
    if (f->IsSensor()) {
        return -1.f;
    }

    b2Body *b = f->GetBody();
    entity *e = (entity*)f->GetUserData();

    if (e && e != this && e->allow_connections()
            //&& e->get_layer() == this->get_layer() && (e->layer_mask & 6) != 0
            && world::fixture_in_layer(f, this->get_layer(), 6)
            ) {
        this->q_result = e;
        this->q_result_fx = f;
        this->q_frame = (uint8_t)(uintptr_t)b->GetUserData();
    }

    return -1;
}

connection *
polygon::load_connection(connection &conn)
{
    if (conn.o_index >= 0 && conn.o_index <= 3) {
        this->c_side[conn.o_index] = conn;
        return &this->c_side[conn.o_index];
    }

    return 0;
}
