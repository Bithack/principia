#pragma once

#define CABLE_BLACK 0
#define CABLE_RED   1
#define CABLE_BLUE  2

#define CABLE_IN 1
#define CABLE_OUT 2
#define CABLE_INOUT 3

#define CABLE_TWIST_DEC .25f

#define CABLE_Z .375f

#define PLUG_PLUG 1
#define PLUG_JUMPER 2
#define PLUG_RECEIVER 3
#define PLUG_MINI_TRANSMITTER 4

#define CABLE_MAX_EXTRA_LENGTH 5.f

#include "entity.hh"
#include <set>

class ifdevice;
class edevice;
class isocket;
class socket_in;
class socket_out;
class plug;

/* cable properties */
#define CABLE_PROP_P0_PX 0
#define CABLE_PROP_P0_PY 1
#define CABLE_PROP_P1_PX 2
#define CABLE_PROP_P1_PY 3
#define CABLE_PROP_E0    4
#define CABLE_PROP_E1    5
#define CABLE_PROP_E0I   6
#define CABLE_PROP_E1I   7

class cable : public entity
{
  public:
    joint_info *ji;

    void create_joint(void);
    void destroy_joint(void);
    bool freeze;
    bool ready;
    float length;
    float extra_length;
    float saved_length;
    int ctype;
    plug *p[2];
    int p_in, p_out;
    b2RopeJoint *joint;

    cable(int type);
    ~cable();
    void construct();
    bool connect(plug *p, edevice *e, isocket *s);
    bool connect(plug *p, edevice *e, uint8_t s);
    void disconnect(plug *p);
    virtual void add_to_world();
    virtual void remove_from_world();
    virtual void set_position(float x, float y, uint8_t frame=0);

    inline plug* get_other(plug *p)
    {
        if (p == this->p[0])
            return this->p[1];
        else
            return this->p[0];
    }

    /* NOTE: this actually returns the inverted values,
     * so we can simply AND cable masks and socket masks */
    inline int get_inout_mask(int type)
    {
        int mask = 0;

        if (type != this->ctype)
            return CABLE_OUT|CABLE_IN;

        if (this->p_in == -1) mask |= CABLE_OUT;
        if (this->p_out == -1) mask |= CABLE_IN;

        return mask;
    }

    virtual const char *get_name(){
        switch (this->ctype) {
            case CABLE_BLACK: return "Power Cable";
            case CABLE_RED: return "Signal Cable";
            default: case CABLE_BLUE: return "Interface Cable";

        }
    };

    float get_slider_snap(int s);
    float get_slider_value(int s);
    const char *get_slider_label(int s){return "Extra length";};
    void on_slider_change(int s, float value);

    void ghost_update(void);
    void update(void);
    void pre_write();
    void on_load(bool created, bool has_state);

    static struct tms_entity *get_entity(void);
    static void reset_counter(void);
    static void upload_buffers(void);
    static void _init(void);

    friend class plug;
};

class plug_base : public entity
{
  public:
    isocket *s;
    cable *c;
    uint8_t plug_type;

    plug_base()
    {
        this->s = 0;
        this->c = 0;
        this->plug_type = 0;
        this->plugged_edev = 0;
        this->update_method = ENTITY_UPDATE_CUSTOM;
        this->layer_mask = 15;
        this->set_flag(ENTITY_IS_PLUG, true);
    }
    edevice             *plugged_edev;
    std::set<edevice *>  pending;

    void on_grab(game *g);
    void on_release(game *g);
    virtual b2Vec2 get_position(void);

    void update(void);
    virtual int connect(edevice *e, isocket *s) = 0;
    virtual void add_to_world();
    virtual void create_body() = 0;
    virtual int get_dir() = 0;
    virtual void disconnect(void){};

    void on_paused_touch(b2Fixture *my, b2Fixture *other);
    void on_paused_untouch(b2Fixture *my, b2Fixture *other);
    void on_pause();
    void setup();

    virtual plug_base* get_other(){return 0;};
    virtual ifdevice *find_ifdevice(){return 0;};
    virtual void set_layer(int z);

    uint8_t get_socket_index(void);

    inline bool is_connected()
    {
        return (this->s != 0);
    }
};

class plug : public plug_base
{
  public:
    plug(cable *c);
    const char *get_name() { return "Plug"; }

    entity* get_property_entity(void){return this->c;};

    void update_mesh();
    void create_body();
    void remove_from_world();
    float get_angle(void);
    int get_dir();

    int connect(edevice *e, isocket *s);
    void disconnect(void);
    void pre_write();

    ifdevice *find_ifdevice();

    plug_base* get_other()
    {
        if (c) return c->get_other(this);
        return 0;
    }

    friend class cable;
};
