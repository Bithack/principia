#include "gameman.hh"
#include "model.hh"
#include "material.hh"
#include "game.hh"

gameman::gameman()
{
    this->menu_scale = .75f;

    this->set_mesh(mesh_factory::get_mesh(MODEL_GAMEMAN));
    this->set_material(&m_metal);

    this->scaleselect = true;

    this->num_s_in = 13;
    this->num_s_out = 1;

    delete [] s_in;
    this->s_in = new socket_in[this->num_s_in];

    for (int x=0; x<2; x++) {
        this->s_in[x].lpos = b2Vec2(-.6f + x*.3f, .3f);
        this->s_in[x].ctype = CABLE_RED;
        this->s_in[x].angle = M_PI/2.f;
    }

    for (int x=2; x<12; x++) {
        int y = (x-2)/5;
        this->s_in[x].lpos = b2Vec2(-.6f + ((x-2)%5)*.3f, 0.f - y*.3f);
        this->s_in[x].ctype = CABLE_RED;
        this->s_in[x].angle = M_PI/2.f;
    }

    this->s_in[12].lpos = b2Vec2(-.6f + .6f, .3f);

    this->s_in[0].tag = SOCK_TAG_WIN;
    this->s_in[1].tag = SOCK_TAG_LOSE;
    this->s_in[2].tag = SOCK_TAG_ADD_1;
    this->s_in[3].tag = SOCK_TAG_ADD_50;
    this->s_in[4].tag = SOCK_TAG_ADD_100;
    this->s_in[5].tag = SOCK_TAG_ADD_250;
    this->s_in[6].tag = SOCK_TAG_ADD_500;
    this->s_in[7].tag = SOCK_TAG_SUB_1;
    this->s_in[8].tag = SOCK_TAG_SUB_50;
    this->s_in[9].tag = SOCK_TAG_SUB_100;
    this->s_in[10].tag = SOCK_TAG_SUB_250;
    this->s_in[11].tag = SOCK_TAG_SUB_500;

    this->s_out[0].lpos = b2Vec2(.6f, .3f);
    this->s_out[0].ctype = CABLE_RED;
    this->s_out[0].angle = M_PI/2.f;

    this->set_as_rect(1.6f/2.f, .950f/2.f);
}

edevice*
gameman::solve_electronics(void)
{
    if (!this->s_out[0].written()) {
        if (W->level.final_score == 0) {
            this->s_out[0].write(0.f);
        } else {
            this->s_out[0].write(fminf((float)G->get_real_score() / (float)W->level.final_score, 1.f));
        }
    }

    for (int x=0; x<12; x++) {
        if (!this->s_in[x].is_ready()) {
            return this->s_in[x].get_connected_edevice();
        }
    }

    bool win = (bool)roundf(this->s_in[0].get_value());
    bool lose = (bool)roundf(this->s_in[1].get_value());
    bool restart = (bool)roundf(this->s_in[12].get_value());

    if (lose) {
        G->finish(false);
    }
    if (win) {
        G->finish(true);
    }
    if (restart) {
        G->restart_level();
    }

    static const int points[] = {
        1, 50, 100, 250, 500
    };

    for (int x=0; x<5; x++) {
        if ((bool)roundf(this->s_in[2+x].get_value())) {
            G->add_score(points[x]);
        }
    }

    for (int x=0; x<5; x++) {
        if ((bool)roundf(this->s_in[7+x].get_value())) {
            G->add_score(-points[x]);
        }
    }

    return 0;
}

