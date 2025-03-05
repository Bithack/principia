#pragma once

#include "entity.hh"
#include "cable.hh"
#include "composable.hh"
#include "ifdevice.hh"

#define EDEV_SOCKET_SIZE .3f

enum {
    SOCK_TAG_NONE = 0,
    SOCK_TAG_SPEED,
    SOCK_TAG_FORCE,
    SOCK_TAG_REVERSE,
    SOCK_TAG_FOCUS,
    SOCK_TAG_VALUE,
    SOCK_TAG_FEEDBACK,
    SOCK_TAG_SET_VALUE,
    SOCK_TAG_SET_ENABLE,
    SOCK_TAG_STATE,
    SOCK_TAG_FD_STATE,
    SOCK_TAG_FD_FORCE,
    SOCK_TAG_TRADEOFF,
    SOCK_TAG_FD_SPEED,
    SOCK_TAG_FD_ERROR,
    SOCK_TAG_ANGLE,
    SOCK_TAG_DISTANCE,
    SOCK_TAG_TICK,
    SOCK_TAG_STATUS,
    SOCK_TAG_ONOFF,
    SOCK_TAG_RESET,
    SOCK_TAG_GENERIC_BOOL,
    SOCK_TAG_INCREASE,
    SOCK_TAG_DECREASE,
    SOCK_TAG_FRACTION,
    SOCK_TAG_MULTIPLIER,

    SOCK_TAG_WIN,
    SOCK_TAG_LOSE,
    SOCK_TAG_RESTART,
    SOCK_TAG_SCORE,
    SOCK_TAG_ADD_1,
    SOCK_TAG_ADD_50,
    SOCK_TAG_ADD_100,
    SOCK_TAG_ADD_250,
    SOCK_TAG_ADD_500,
    SOCK_TAG_SUB_1,
    SOCK_TAG_SUB_50,
    SOCK_TAG_SUB_100,
    SOCK_TAG_SUB_250,
    SOCK_TAG_SUB_500,

    SOCK_TAG_LEFT,
    SOCK_TAG_RIGHT,
    SOCK_TAG_UP,
    SOCK_TAG_DOWN,
    SOCK_TAG_PLUS,
    SOCK_TAG_MINUS,
    SOCK_TAG_ATTACK,
    SOCK_TAG_RESPAWN,
    SOCK_TAG_FREEZE,

    SOCK_TAG__COUNT
};

extern uint64_t edev_step_count;

class isocket
{
  public:
    int ctype;
    plug_base *p;
    b2Vec2 lpos;
    float abias;
    float angle;
    int tag;

    isocket()
    {
        tag = SOCK_TAG_NONE;
        ctype = CABLE_RED;
        p = 0;
        angle = M_PI/2.f;
        abias = 0.f;
    }

    inline void unplug(void)
    {
        if (this->p) {
            this->p->disconnect();
        } else {
            tms_infof("No plug to disconnect.");
        }
    }
};

class socket_in : public isocket
{
  public:
    float value;
    uint64_t step_count;
#if 0
    float pending_value;
    bool pending;
    bool processed;
#endif

    socket_in()
    {
        value = 0.f;
#if 0
        pending = false;
        pending_value = 0.f;
        processed = false;
#endif

        this->p = 0;
        reset();
    }


    inline bool
    is_ready()
    {
        if (this->p != 0) {
            if (this->p->get_other()) {
                if (this->p->get_other()->s) {
                    return (this->step_count == edev_step_count);
                } else {
                    return true;
                }
            } else {
                return (this->step_count == edev_step_count);
            }
        } else {
            return true;
        }
    }

    inline void
    reset()
    {
        this->step_count = edev_step_count;
        this->value = 0.f;
    }

    inline edevice *get_connected_edevice()
    {
        if (this->p != 0) {
            if (this->p->get_other()) {
                return this->p->get_other()->plugged_edev;
            } else
                return this->p->get_edevice();
        } else
            return 0;
    }

    inline float get_value() const
    {
#if 0
        this->processed = true;

        if (this->pending) {
            this->pending = false;
            return this->pending_value;
        }
#endif

        if (this->p == 0) {
            return 0.f;
        } else {
            return this->value;
        }
    }
};

class socket_out : public isocket
{
  public:
    socket_out()
    {
        this->p = 0;
    }

    bool written_mt();

    bool written()
    {
        plug_base *o;

        if (p) {
            if ((o = p->get_other()) && o->s) {
                if (((socket_in*)o->s)->step_count == edev_step_count)
                    return true;
            } else if (this->p->plug_type == PLUG_MINI_TRANSMITTER) {
                return written_mt();
            } else
                return false;
        }

        return false;
    }

    void write_mt(float v);
    void write(float v);

};

/** 
 * Source of electricity or red wire signal
 **/
class edevice
{
  public:
    socket_in *s_in;
    socket_out *s_out;
    int num_s_in;
    int num_s_out;
    uint64_t step_count;
    bool scaleselect;
    float scalemodifier;
    bool do_solve_electronics;

    edevice()
    {
        this->do_solve_electronics = true;
        this->s_in = new socket_in[8];
        this->s_out = new socket_out[8];
        this->num_s_in = 0;
        this->num_s_out = 0;
        this->scaleselect = false;
        this->scalemodifier = 2.5f;
    }

    ~edevice()
    {
        delete[] s_in;
        delete[] s_out;
    }

    inline bool any_socket_used()
    {
        for (int x=0; x<num_s_in; x++) if (s_in[x].p) return true;
        for (int x=0; x<num_s_out; x++) if (s_out[x].p) return true;
        return false;
    }

    virtual void begin()
    {
        this->step_count = edev_step_count;
        for (int x=0; x<this->num_s_in; x++){
            this->s_in[x].reset();
        }
    };
    virtual edevice* solve_electronics(){return 0;};

    uint8_t get_socket_index(isocket *s);

    /* get the direction of a socket, if its an input our
     * output socket, return -1 if this is not our socket */
    inline int get_socket_dir(isocket *s)
    {
        if (s >= &s_in[0] && s < &s_in[this->num_s_in])
            return CABLE_IN;
        if (s >= &s_out[0] && s < &s_out[this->num_s_out])
            return CABLE_OUT;

        return -1;
    }

    inline int get_outin_mask(int type)
    {
        int mask = 0;

        if (!this->num_s_in && !this->num_s_out)
            return 0;

        for (int x=0; x<num_s_in; x++) {
            if (!this->s_in[x].p && this->s_in[x].ctype == type) {
                mask |= CABLE_OUT;
                break;
            }
        }

        for (int x=0; x<num_s_out; x++) {
            if (!this->s_out[x].p && this->s_out[x].ctype == type) {
                mask |= CABLE_IN;
                break;
            }
        }

        return mask;
    }

    inline int get_inout_mask(int type)
    {
        int mask = 0;

        if (!this->num_s_in && !this->num_s_out)
            return 0;

        for (int x=0; x<num_s_in; x++) {
            if (!this->s_in[x].p && this->s_in[x].ctype == type) {
                mask |= CABLE_IN;
                break;
            }
        }

        for (int x=0; x<num_s_out; x++) {
            if (!this->s_out[x].p && this->s_out[x].ctype == type) {
                mask |= CABLE_OUT;
                break;
            }
        }

        return mask;
    }

    void recreate_all_cable_joints();

    virtual entity *get_entity(void)=0;
    virtual ifdevice *get_ifdevice(void){return 0;};

    friend class plug;
    friend class brcomp;
    friend class ecomp;
};

class ecomp : public composable, public edevice
{
  public:
    ecomp()
    {
        this->set_flag(ENTITY_IS_EDEVICE, true);
        this->type = ENTITY_EDEVICE;
        this->num_s_in = 0;
        this->num_s_out = 0;
    }

    virtual void set_layer(int z);

    edevice *get_edevice(){return (edevice*)this;};
    entity *get_entity(){return (entity*)this;};
};

class ecomp_multiconnect: public ecomp
{
  public:
    connection c_side[4];

    ecomp_multiconnect()
    {
        this->c_side[0].init_owned(0, this); this->c_side[0].type = CONN_GROUP;
        this->c_side[1].init_owned(1, this); this->c_side[1].type = CONN_GROUP;
        this->c_side[2].init_owned(2, this); this->c_side[2].type = CONN_GROUP;
        this->c_side[3].init_owned(3, this); this->c_side[3].type = CONN_GROUP;
    }

    connection *load_connection(connection &conn);
    void find_pairs();
    void set_as_rect(float width, float height);
};

class edev : public entity, public edevice
{
  public:
    edev()
    {
        this->set_flag(ENTITY_IS_EDEVICE, true);
        this->type = ENTITY_EDEVICE;
        this->num_s_in = 0;
        this->num_s_out = 0;
    }

    edevice *get_edevice(){return (edevice*)this;};
    entity *get_entity(){return (entity*)this;};

    virtual void set_layer(int z);
};

class brcomp : public composable, public edevice
{
  public:
    brcomp() {
        this->layer_mask = 2+4+8;
        this->set_flag(ENTITY_IS_EDEVICE, true);
        this->set_flag(ENTITY_IS_BRDEVICE, true);
        this->type = ENTITY_EDEVICE;
        this->num_s_in = 0;
        this->num_s_out = 0;
    }

    virtual void set_layer(int z);

    edevice *get_edevice(){return (edevice*)this;};
    entity *get_entity(){return (entity*)this;};
};

class brcomp_multiconnect: public brcomp
{
  public:
    connection c_side[4];

    brcomp_multiconnect()
    {
        this->c_side[0].init_owned(0, this); this->c_side[0].type = CONN_GROUP;
        this->c_side[1].init_owned(1, this); this->c_side[1].type = CONN_GROUP;
        this->c_side[2].init_owned(2, this); this->c_side[2].type = CONN_GROUP;
        this->c_side[3].init_owned(3, this); this->c_side[3].type = CONN_GROUP;
    }

    connection *load_connection(connection &conn);
    void find_pairs();
    void set_as_rect(float width, float height);
};

class edev_simpleconnect : public edev, public b2RayCastCallback
{
  public:
    connection c;
    entity *query_result;
    b2Fixture *query_result_fx;
    uint8_t query_frame;

    b2Vec2 query_pt;
    b2Vec2 query_vec; /* Set in the constructor of the
                         object, direction of raycast */

    edev_simpleconnect()
    {
        this->c.init_owned(0, this);
        this->c.type = CONN_PLATE;
        this->query_vec = b2Vec2(0.f, -.5f);
        this->query_pt = b2Vec2(0.f, 0.f);
        this->query_result = 0;
        this->query_frame = 0;
    }

    connection *load_connection(connection &conn) {
        this->c = conn;
        return &this->c;
    };

    void find_pairs();
    float32 ReportFixture(b2Fixture *f, const b2Vec2 &pt, const b2Vec2 &nor, float32 fraction);
};

class ecomp_simpleconnect : public ecomp, public b2RayCastCallback
{
  public:
    connection c;
    entity *query_result;
    b2Fixture *query_result_fx;
    uint8_t query_frame;

    b2Vec2 query_pt;
    b2Vec2 query_vec; /* Set in the constructor of the
                         object, direction of raycast */

    ecomp_simpleconnect()
    {
        this->c.init_owned(0, this);
        this->c.type = CONN_PLATE;
        this->query_vec = b2Vec2(0.f, -.5f);
        this->query_pt = b2Vec2(0.f, 0.f);
        this->query_result = 0;
        this->query_frame = 0;
    }

    connection *load_connection(connection &conn) {
        this->c = conn;
        return &this->c;
    };

    void find_pairs();
    float32 ReportFixture(b2Fixture *f, const b2Vec2 &pt, const b2Vec2 &nor, float32 fraction);
};

class edev_multiconnect: public edev
{
  public:
    connection c_side[4];

    edev_multiconnect()
    {
        this->c_side[0].init_owned(0, this); this->c_side[0].type = CONN_GROUP;
        this->c_side[1].init_owned(1, this); this->c_side[1].type = CONN_GROUP;
        this->c_side[2].init_owned(2, this); this->c_side[2].type = CONN_GROUP;
        this->c_side[3].init_owned(3, this); this->c_side[3].type = CONN_GROUP;
    }

    connection *load_connection(connection &conn);
    void find_pairs();
};
