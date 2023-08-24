#pragma once

#include "edevice.hh"
#include "world.hh"
#include "prompt.hh"
#include <vector>
#include <set>

/* escript flags */
// INCLUDE_{STRING,TABLE} and LISTEN_ON_INPUT are unused now but need to be kept for backwards compatibility
#define ESCRIPT_INCLUDE_STRING          (1ULL << 0)
#define ESCRIPT_INCLUDE_TABLE           (1ULL << 1)
#define ESCRIPT_LISTEN_ON_INPUT         (1ULL << 2)
#define ESCRIPT_USE_EXTERNAL_EDITOR     (1ULL << 3)

#define ESCRIPT_WORLD   0
#define ESCRIPT_SCREEN  1
#define ESCRIPT_LOCAL   2

#define ESCRIPT_EXTERNAL_PATH_LEN 1024

struct lua_State;
class receiver_base;

struct function_info {
    const char *name;
    int timelimit;
};

struct escript_line {
    float x1, y1, z1;
    float x2, y2, z2;
    float r1, g1, b1, a1;
    float r2, g2, b2, a2;
    float w1, w2;
};

struct escript_sprite {
    float x, y;
    float r;
    float w, h;
    int bx, by;
    int tx, ty;
};

#define DEFAULT_DRAW_WIDTH  128
#define DEFAULT_DRAW_HEIGHT 128

class draw_data {
  private:
    escript *parent;

  public:
    draw_data(escript *parent, int width=DEFAULT_DRAW_WIDTH, int height=DEFAULT_DRAW_HEIGHT, uint8_t num_channels=4);
    ~draw_data();

    void update_effects();
    void resize_texture_buffer(int width, int height, uint8_t num_channels=4);

    struct tms_texture *texture;
    int texture_width;
    int texture_height;
    int texture_num_channels;
    tms::gbuffer *verts;
    tms::gbuffer *indices;
    tms::varray  *va;
    tms::mesh    *sprite_mesh;
    tms::entity  *sprite_ent;
    tms::material mat;
    int sprite_count;
    int pending_sprite_count;
    bool texture_modified;
    bool verts_modified;
};

struct DataStruct {
    char *buf;
    size_t size;
};

class escript : public brcomp_multiconnect, public base_prompt
{
  protected:
    std::vector<struct escript_line> lines;
    std::vector<struct escript_line> pending_lines;

  public:
    struct DataStruct data;
    int events[WORLD_EVENT__NUM];

    char *p_message;
    char *p_btn1;
    uint32_t p_btn1_len;
    char *p_btn2;
    uint32_t p_btn2_len;
    char *p_btn3;
    uint32_t p_btn3_len;

    escript();
    ~escript();
    const char *get_name(){return "Lua Script";};
    void remove_from_world();

    int first_run;
    float val[4];
    bool socket_active[4];
    uint32_t prompt_id;

    bool solving;
    bool run;
    bool has_on_event;
    bool has_step;
    bool has_on_response;
    bool listen_on_input;
    std::set<tms::event*> input_events;

    std::vector<struct escript_sprite> static_sprites;
    uint32_t local_id; /* entity to follow in local screen mode */
    int blending_mode;
    int filtering;
    int coordinate_mode;
    float draw_z;
    tvec4 draw_tint;

    draw_data *normal_draw;
    draw_data *static_draw;

    void update_effects();
    void init();
    void setup();
    void on_pause();
    edevice* solve_electronics();
    void draw_pre_solve(draw_data *draw);
    void draw_post_solve(draw_data *draw);

    std::map<uint32_t, receiver_base*> receivers;

    lua_State *L;

    void add_line(const struct escript_line &line);
    void add_static_sprite(float x, float y, float r, float w, float h, int bx, int by, int tx, int ty, bool add=true);

    void on_load(bool created, bool has_state);
    void pre_write();
    void post_write();

    void write_state(lvlinfo *lvl, lvlbuf *lb);
    void read_state(lvlinfo *lvl, lvlbuf *lb);
    void restore();

    bool requires_delete_confirmation() { return true; }

    void generate_external_path(char *buf);

    base_prompt *get_base_prompt() { return this; }
};
