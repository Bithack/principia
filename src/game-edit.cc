#include "game.hh"
#include "world.hh"
#include "group.hh"
#include "object_factory.hh"
#include "fxemitter.hh"

#include <set>

static std::set<entity*> elist;

static connection tmp[256];
static int num_tmp = 0;

class query_cb : public b2QueryCallback
{
  public:
    query_cb() {};

    bool ReportFixture(b2Fixture *f)
    {
        entity *e = (entity*)f->GetUserData();

        if (e) {
            elist.insert(e);
        }

        return true;
    }
};

void
game::return_tmp_conn(connection *c)
{
    if (num_tmp > 0 && &tmp[num_tmp-1] == c)
        num_tmp --;
}

connection*
game::get_tmp_conn(void)
{
    if (num_tmp >= 256)
        return 0;

    connection *r = &tmp[num_tmp++];

    r->reset();
    r->owned = false;

    return r;
}

bool
game::add_pair(entity *e1, entity *e2, connection *c)
{
    if (!c) {
        return false;
    }

    if (!e2->allow_connections() && !e2->enjoys_connection(e1->g_id)) {
        return false;
    }

    if (!e2->allow_connection(e1, c->f[1], c->p)) {
        return false;
    }

    if ((e1->flag_active(ENTITY_IS_BRDEVICE) && e1->flag_active(ENTITY_CONNECTED_TO_BREADBOARD)) || (e2->flag_active(ENTITY_IS_BRDEVICE) && e2->flag_active(ENTITY_CONNECTED_TO_BREADBOARD))) {
        return false;
    }

    if (e1->flag_active(ENTITY_IS_STATIC) || e2->flag_active(ENTITY_IS_STATIC)) {
        if (!this->state.sandbox && W->is_puzzle() && W->level.flag_active(LVL_DISABLE_STATIC_CONNS)) {
            return false;
        }
    }

    for (int x=0; x<NUM_HL; ++x) {
        if ((this->hls[x].type & HL_TYPE_ERROR) && (this->hls[x].e == e1 || this->hls[x].e == e2)) {
            return false;
        }
    }

    if (e1 > e2) {
        entity *tmp = e1;
        e1 = e2;
        e2 = tmp;
    }

    if (e1->connected_to(e2)) {
        return false;
    }

    if (!((1 << e1->get_layer()) & this->layer_vis)) {
        return false;
    }
    if (!((1 << e2->get_layer()) & this->layer_vis)) {
        return false;
    }

    /* only add the pair if any of the entities is the currently selected entity */

    /* nevermind */

    /* nevermind above nevermind */

    //if (W->is_paused()) {
        if (e1 != this->selection.e && e2 != this->selection.e) {
            return false;
        }
    //}

    c->update();

    /* if the user is currently selecting a connection type we must make sure
     * each frame that the connection is still valid, if this never passes
     * then the game mode is reset to default to prevent it from blah blah blah
     * this isnt so complicated */
    if (c == this->cs_conn) {
        this->cs_found = true;
    }

    if (c->type == CONN_GEAR || c->type == CONN_RACK) {
        c = this->apply_connection(c, 0);
        return true;
    }

    c_key k(e1,e2);
    return (this->pairs.insert(std::make_pair(k, c)).second);
}

connection *
game::apply_connection(connection *c, int option)
{
    if (W->level.type == LCAT_ADVENTURE && !W->is_paused() && option != -1) {
        /* if we're in game, do not allow player to connect anything to an auto protector loop,
         * otherwise the player can just connect things to bosses to move their centre of mass and
         * beat them easily. Platforms do not protect for new connections. */

        tms_debugf("can't connect to this m8");
        if (c->e->is_protected() || c->o->is_protected()) {
            /* add a spark effect and drop all interactions */
            this->emit(new spark_effect(
                        c->p,
                        c->e->get_layer()
                        ), 0);
            this->drop_interacting();
            return 0;
        }
    }
    /* make a clone of the connection if it is not owned by an object */
    if (!c->owned) c = c->clone();

    tms_assertf(c->e, "connection missing first entity");
    tms_assertf(c->o, "connection missing second entity");

    tms_debugf("apply connection, type %d, option %d", c->type, option);
    tms_debugf("e_id: %u, o_id: %u", c->e->id, c->o?c->o->id:0);

    if (c->o->g_id == O_CHUNK) {
        tms_debugf("creating connection to chunk");
    }

    //this->drop_if_interacting(c->e);
    //this->drop_if_interacting(c->o);

    if (option != -1) {
        c->option = option;
        for (int x=0; x<NUM_CA; x++) {
            if (this->ca[x].life < 0.f || this->ca[x].life > 1.f) {
                this->ca[x].life = 1.f;
                this->ca[x].p = c->p;
                this->ca[x].dir = -1.f;
                break;
            }
        }
    }

    W->insert_connection(c);

    if (c->type == CONN_BR) {
        tms_errorf("error: CONN_BR invalid");
        return 0;
    }

    if (option == 1 && c->type != CONN_CUSTOM) {
        /* always create pivot joint */
        c->type = CONN_PIVOT;
        c->apply();
        c->create_joint(true);
        c->e->update_protection_status();

        return c;
    } else if (c->type == CONN_GEAR || c->type == CONN_RACK) {
        c->apply();
        c->create_joint(true);
        c->e->update_protection_status();

        return c;
    }

    if (c->max_force < INFINITY && c->type == CONN_GROUP) {
        tms_infof("max force below infinity (%f), forcing to CONN_PLATE", c->max_force);
        c->type = CONN_PLATE;
    }

#if 0
    /* adventure mode */
    if (!W->is_paused() && !W->initial_add && c->type == CONN_GROUP) {
        tms_debugf("converted group joint to plate joint");
        c->type = CONN_PLATE;
    }
#endif

    if (c->type == CONN_GROUP) {
        if (c->e->flag_active(ENTITY_IS_COMPOSABLE) && c->o->flag_active(ENTITY_IS_COMPOSABLE)
            && (this->state.sandbox || (c->e->is_moveable() && c->o->is_moveable()))) {
            tms_infof("both objects composable");
            /* both objects are composable, add them to a group */
            c->apply();
            if (c->o->gr) {
                c->o->gr->add(c);
            } else if (c->e->gr) {
                c->e->gr->add(c);
            } else {
                tms_infof("Creating a new group for the objects");
                group *g = new group();
                g->id = of::get_next_id();
                W->add(g);
                g->add(c);

                this->add_entity(g);
            }

            c->e->update_protection_status();
            return c;
        } else {
            tms_infof("both objects are NOT composable -------------------------------------------");
            c->type = CONN_WELD;
            c->apply();
            c->create_joint(true);
            c->e->update_protection_status();

            return c;
        }
    }

    if (c->type == CONN_CUSTOM) {
        c->apply();
        c->e->connection_create_joint(c);
        c->e->update_protection_status();

        return c;
    }

    if (c->o->allow_connections() || (c->o->enjoys_connection(c->e->g_id) && c->e->enjoys_connection(c->o->g_id))) {
        c->apply();
        c->create_joint(true);
        c->e->update_protection_status();

        return c;
    }

    return 0;
}

void
game::update_pairs()
{
    this->pairs.clear();
    elist.clear();
    num_tmp = 0;

    this->cs_found = false;

    if (this->selection.enabled() && this->selection.e) {
        /* find all objects near the currently selected object, and
         * call their find_pairs methods */
        query_cb cb;
        b2AABB aabb;

        b2Vec2 origo;

        origo = this->selection.e->get_position();

        aabb.lowerBound.Set(-4.f + origo.x, -4.f + origo.y);
        aabb.upperBound.Set(4.f + origo.x, 4.f + origo.y);
        W->b2->QueryAABB(&cb, aabb);

        if (!(!this->state.sandbox && W->is_puzzle() && W->level.flag_active(LVL_DISABLE_CONNECTIONS))) {
            for (std::set<entity*>::iterator i = elist.begin(); i != elist.end(); i++) {
                entity *e = *i;

                if (e->get_body(0) == 0 || e->get_body(0)->GetType() != b2_kinematicBody) {
                    if (W->is_paused() || (W->is_adventure() && e->is_protected() == false)) {
                        e->find_pairs();
                    }
                }
            }
        }
    }

    if (this->get_mode() == GAME_MODE_SELECT_CONN_TYPE && !this->cs_found && this->cs_conn) {
        this->set_mode(GAME_MODE_DEFAULT);
        this->cs_conn = 0;
    }
}

