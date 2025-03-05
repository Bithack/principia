#pragma once

#include "composable.hh"

#define MAX_POLYGONS 128

class polygon : public composable, public b2RayCastCallback
{
    polygon *next; /* next in linked list */
    int      slot;
    void reassign_slot(bool changed);
    void remove_from_slot();
    void add_to_slot(int n);

    entity *q_result;
    b2Fixture *q_result_fx;
    uint8_t q_frame;
    int q_dir;
    int box_type;
    b2Vec2 q_point;
    float q_fraction;

    struct tms_mesh _mesh;

    connection c_side[4];

  public:
    static void _init();
    static void upload_buffers();

    void tick();

    polygon(int material_type);
    ~polygon();
    const char *get_name()
    {
        switch (this->material_type) {
            case MATERIAL_PLASTIC:
            default:
                return "Plastic Polygon";
        }
    }

    void setup();
    void on_pause();
    void on_load(bool created, bool has_state);

    float get_slider_snap(int s){return .05f;};
    float get_slider_value(int s){
        return ((float)this->properties[2].v.f - ENTITY_DENSITY_SCALE_MIN) / ENTITY_DENSITY_SCALE_MAX;
    };
    const char *get_slider_label(int s){return "Density scale";};
    void on_slider_change(int s, float value);
    void set_density_scale(float v)
    {
        this->properties[2].v.f = v;
    }
    float get_density_scale(){return this->properties[2].v.f;};

    b2PolygonShape* get_resizable_shape(){
        /* only allow resizing if we're not connected to anything */
        return this->conn_ll == 0 ? (b2PolygonShape*)this->fx->GetShape() : 0;
    };
    int get_resizable_vertices(int *out)
    {
        for (int x=0; x<4; x++) {
            out[x]=x;
        }
        return 4;
    };

    void set_shape();
    void update_mesh();

    bool on_resize_vertex(int n, b2Vec2 new_pos)
    {
        b2PolygonShape *sh = this->get_resizable_shape();

        if (!sh) return false;

        this->properties[3+n*5].v.f = new_pos.x;
        this->properties[3+n*5+1].v.f = new_pos.y;

#if 1
        b2Vec2 c = sh->m_centroid;
        for (int x=0; x<sh->m_count; x++) {
            sh->m_vertices[x] -= c;
            this->properties[3+x*5+0].v.f = sh->m_vertices[x].x;
            this->properties[3+x*5+1].v.f = sh->m_vertices[x].y;
        }
        b2Vec2 p = this->get_position()+this->get_body(0)->GetWorldVector(c);
        this->set_position(p.x, p.y);
#endif

        this->get_body(0)->ResetMassData();
        this->reassign_slot(true);
        this->update_mesh();

        this->orig.poly.shape.Set(sh->m_vertices, sh->GetVertexCount());

        if (this->fx) this->fx->Refilter();

        return true;
    };

    int material_type;
    bool do_recreate_shape;

    void set_color(tvec4 c);
    tvec4 get_color();

    void find_pairs();
    float32 ReportFixture(b2Fixture *f, const b2Vec2 &pt, const b2Vec2 &nor, float32 fraction);
    connection *load_connection(connection &conn);
};
