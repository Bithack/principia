#pragma once

#include "edevice.hh"

class gearbox : public edev, public b2QueryCallback
{
    public:
        b2Body *body2;
        int mslot;
        int active_conf;
        bool checked_b2conn;
        connection *c_b2conn;
        b2GearJoint *gearj;

        connection c_out;
        entity *q_result;
        b2Vec2 q_point;
        uint8_t q_frame;

        class configuration
        {
            public:
                int pos;
                float ratio;
                float ratio2;
        };

        struct tms_entity *aent[3];

        int min_pos, max_pos;

        configuration configs[16];
        int num_configs;

        gearbox();
        ~gearbox();

        uint32_t get_num_bodies();
        b2Body *get_body(uint8_t n);
        bool allow_connection(entity *asker, uint8_t frame, b2Vec2 p);
        void connection_create_joint(connection *c);
        void find_pairs();
        bool ReportFixture(b2Fixture *f);
        void set_layer(int n);
        void set_position(float x, float y, uint8_t frame=0);
        connection * load_connection(connection &conn);
        void add_to_world();
        void remove_from_world();
        void on_load(bool created, bool has_state);
        void update(void);
        void step();
        void update_configurations();
        const char *get_name(){return "Gearbox";};
        float get_ratio();
        edevice* solve_electronics();

        void create_gearjoint();
};
