#include "minibot.hh"
#include "model.hh"
#include "material.hh"
#include "game.hh"
#include "world.hh"
#include "item.hh"
#include "adventure.hh"

#define VISION 8.f

minibot::minibot()
{
    this->robot_type = ROBOT_TYPE_MINIBOT;

    this->set_mesh(mesh_factory::get_mesh(MODEL_MINIBOT));
    this->set_material(&m_robot_tinted);

    this->body_shape = static_cast<b2Shape*>(new b2PolygonShape());
    ((b2PolygonShape*)this->body_shape)->SetAsBox(1.f/2.f, .3f/2.f*1.25f);
    this->width = 1.f/2.f;
    this->height = .3f/2.f*1.25f;

    this->properties[8].v.i8 = FEET_MINIWHEELS;

    this->feet_offset = .05f;
}

minibot::~minibot()
{
}

void
minibot::on_load(bool created, bool has_state)
{
    robot_base::on_load(created, has_state);

    this->look_dir = DIR_LEFT;
    this->i_dir = -1.f;
    this->fixed_dir = true;
}

void
minibot::look_for_target()
{
    b2AABB aabb;
    aabb.lowerBound = this->get_position()+b2Vec2(-VISION, -VISION);
    aabb.upperBound = this->get_position()+b2Vec2(VISION, VISION);

    W->b2->QueryAABB(this, aabb);
}

bool
minibot::roam_neglect()
{
    if (this->roam_target->is_creature()) {
        if (!static_cast<creature*>(this->roam_target)->is_dead()) {
            return true;
        }
    }
    return false;
}

void
minibot::roam_update_dir()
{
    b2Vec2 target_pos = this->roam_target->get_position();
    float tangent_dist = this->get_tangent_distance(target_pos);
    this->new_dir = (tangent_dist > 0.f ? DIR_RIGHT : DIR_LEFT);
}

void
minibot::roam_attack()
{
    /* this is handled in solver_ingame instead */
    /*
    if (this->roam_target && this->shoot_target && this->target_dist < 2.f) {
        G->lock();
        G->absorb(this->roam_target);
        G->unlock();
        this->roam_target = 0;
        this->roam_target_id = 0;
    }
    */
}

void
minibot::eat(entity *e)
{
    // XXX: Disable eating of adventure-bot for now :D
    if (e == adventure::player) return;

    G->lock();
    if (G->absorb(e)) {
        // we successfully absorbed this entity!
        // add its material or whatever to our inventory?
    }
    G->unlock();

    if (this->roam_target_id == e->id) {
        this->roam_target = 0;
        this->roam_target_id = 0;
    }
}

bool
minibot::roam_can_target(entity *e, bool must_see)
{
    if (e->flag_active(ENTITY_IS_CREATURE)) {
        creature *r = static_cast<creature*>(e);
        return r->is_dead();
    }

    item *i = static_cast<item*>(e);

    return (e->g_id == O_ITEM && (
                   i->item_category == ITEM_CATEGORY_LOOSE_HEAD
                || i->item_category == ITEM_CATEGORY_BACK
                || i->item_category == ITEM_CATEGORY_FRONT
                ));
}

void
minibot::roam_set_target_type()
{
    /* XXX TODO we should target those who attack us with TARGET_ENEMY and flee */
    this->roam_target_type = TARGET_ITEM;
}

