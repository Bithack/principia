#include "solver_ingame.hh"
#include "entity.hh"
#include "plant.hh"
#include "robot_parts.hh"
#include "robot_base.hh"
#include "robot.hh"
#include "ball.hh"
#include "pipeline.hh"
#include "gear.hh"
#include "conveyor.hh"
#include "impact_sensor.hh"
#include "ragdoll.hh"
#include "game.hh"
#include "explosive.hh"
#include "pixel.hh"
#include "fxemitter.hh"
#include "button.hh"
#include "adventure.hh"
#include "robot_parts.hh"
#include "item.hh"
#include "factory.hh"
#include "minibot.hh"
#include "resource.hh"
#include "soundmanager.hh"
#include "ud2.hh"
#include "spikebot.hh"

#include <tms/bindings/cpp/cpp.hh>

static void begincontact_creature(b2Contact *contact, entity *a, entity *b, int rev);
static void begincontact_plant(b2Contact *contact, entity *a, entity *b, int rev);
static void presolve_creature(b2Contact *contact, entity *a, entity *b, int rev, const b2Manifold *man);
static void presolve_creature_on_creature(b2Contact *contact,
        creature *a, creature *b,
        b2Fixture *fa, b2Fixture *fb,
        const b2Manifold *man,
        const b2WorldManifold &wm);
static void presolve_plant(b2Contact *contact, entity *a, entity *b, int rev, const b2Manifold *man);
static void postsolve_creature(b2Contact *contact, entity *a, entity *b, int rev, const b2ContactImpulse *impulse);
static void postsolve_ragdoll(b2Contact *contact, entity *a, entity *b, int rev, const b2ContactImpulse *impulse);
static void presolve_conveyor(b2Contact *contact, entity *a, entity *b, int rev, const b2Manifold *man);
static void presolve_cog_cog(b2Contact *contact, entity *a, entity *b, int rev, const b2Manifold *man);
static void presolve_pipeline_ball(b2Contact *contact, entity *a, entity *b, int rev, const b2Manifold *man);

static void (*presolve_handler[13][13])(b2Contact *contact, entity *a, entity *b, int rev, const b2Manifold *man) =
{
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, presolve_cog_cog, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, presolve_pipeline_ball},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

bool
enable_emitted_contact(entity *a, entity *b)
{
    /* TODO: Modify the step count if a time multiplier is active. */
    if (a->emitted_by) {
        int emit_delay = EMIT_DELAY_COLLIDE_EMITTER/(G->get_time_mul()+0.0001f);
        if (W->step_count - a->emit_step > emit_delay) {
            //a->emitted_by = 0;
            return true;
        }

        if (a->emitted_by != b->id) {
            //a->emitted_by = 0;
            return true;
        } else
            return false;
    }

    if (b->emitted_by) {
        int emit_delay = EMIT_DELAY_COLLIDE_EMITTER/(G->get_time_mul()+0.0001f);
        if (W->step_count - b->emit_step > emit_delay) {
            //b->emitted_by = 0;
            return true;
        }

        if (b->emitted_by != a->id) {
            //b->emitted_by = 0;
            return true;
        } else
            return false;
    }

    return true;
}

void solver_ingame::BeginContact(b2Contact *contact)
{
    contact->first_contact = true;
    contact->rel_speed = 0.f;

    b2Fixture *a = contact->GetFixtureA();
    b2Fixture *b = contact->GetFixtureB();

    entity *ea = static_cast<entity*>(a->GetUserData());
    entity *eb = static_cast<entity*>(b->GetUserData());

    if (a->IsSensor()) {
        if (ea) {
            G->lock();
            ea->on_touch(a, b);
            G->unlock();
        }
    } else {

    }
    if (b->IsSensor() && eb) {
        G->lock();
        eb->on_touch(b, a);
        G->unlock();
    }

    b2WorldManifold wm;
    contact->GetWorldManifold(&wm);
    b2Vec2 vel1 = a->GetBody()->GetLinearVelocityFromWorldPoint( wm.points[0] );
    b2Vec2 vel2 = b->GetBody()->GetLinearVelocityFromWorldPoint( wm.points[0] );
    b2Vec2 rvel(vel1 - vel2);

    float i;

    if (rvel.x < FLT_EPSILON && rvel.y < FLT_EPSILON) {
        i = 0.f;
    } else {
        i = rvel.Length();
    }
    contact->rel_speed = i;

    if (ea && eb) {
        if (ea->g_id == O_RESOURCE && eb->is_bullet()) {
            contact->SetEnabled(false);
            return;
        } else if (eb->g_id == O_RESOURCE && ea->is_bullet()) {
            contact->SetEnabled(false);
            return;
        }

        if (ea->is_creature()) {
            begincontact_creature(contact, ea, eb, 1);
        } else if (eb->is_creature()) {
            begincontact_creature(contact, ea, eb, 0);
        }

        if (ea->g_id == O_PLANT) {
            begincontact_plant(contact, ea, eb, 1);
        } else if (eb->g_id == O_PLANT) {
            begincontact_plant(contact, ea, eb, 0);
        }

        if (ea->g_id == O_PIXEL && ea->properties[4].v.i8 != 0)
            b->GetBody()->SetSleepingAllowed(false);
        if (eb->g_id == O_PIXEL && eb->properties[4].v.i8 != 0)
            a->GetBody()->SetSleepingAllowed(false);

        if (ea->flag_active(ENTITY_IS_EXPLOSIVE) && eb->flag_active(ENTITY_TRIGGER_EXPLOSIVES)) {
            ((explosive*)ea)->triggered = true;
        }
        if (eb->flag_active(ENTITY_IS_EXPLOSIVE) && ea->flag_active(ENTITY_TRIGGER_EXPLOSIVES)) {
            ((explosive*)eb)->triggered = true;
        }

        if (!a->IsSensor() && !b->IsSensor()) {
            if (ea->g_id == O_MINIBOT) {
                if (static_cast<minibot*>(ea)->roam_can_target(eb)) {
                    static_cast<minibot*>(ea)->eat(eb);
                    contact->SetEnabled(false);
                    return;
                }
            }
            if (eb->g_id == O_MINIBOT) {
                if (static_cast<minibot*>(eb)->roam_can_target(ea)) {
                    static_cast<minibot*>(eb)->eat(ea);
                    contact->SetEnabled(false);
                    return;
                }
            }
        }
    }

    /* dont play any sound when an interactive object being dragged hits the player */
    /*
    if (ea && ea->interactive && eb && eb->id == 0xffffffff && ea->in_dragfield && G->interacting_with(ea)) {
        contact->SetEnabled(false);
        return;
    }
    if (eb && eb->interactive && ea && ea->id == 0xffffffff && eb->in_dragfield && G->interacting_with(eb)) {
        contact->SetEnabled(false);
        return;
    }
    */

    if (a->IsSensor() || b->IsSensor())
        return;

    if (ea && eb && !enable_emitted_contact(ea,eb)) {
        contact->SetEnabled(false);
        return;
    }

    if (ea && ea->flag_active(ENTITY_IS_BULLET)) {
        item *ba = static_cast<item*>(ea);
        if (!ba->on_collide(b, wm.points[0], i)) {
            contact->SetEnabled(false);
            return;
        }
    }
    if (eb && eb->flag_active(ENTITY_IS_BULLET)) {
        item *bb = static_cast<item*>(eb);
        if (!bb->on_collide(a, wm.points[0], i)) {
            contact->SetEnabled(false);
            return;
        }
    }

    if (ea && ea->g_id == O_IMPACT_SENSOR) {
        if (W->level.version < LEVEL_VERSION_1_5 && eb && eb->is_bullet()) {
            ((impact_sensor*)ea)->impulse += i + 5.f;
        } else {
            ((impact_sensor*)ea)->impulse += i;
        }
    }
    if (eb && eb->g_id == O_IMPACT_SENSOR) {
        if (W->level.version < LEVEL_VERSION_1_5 && ea && ea->is_bullet()) {
            ((impact_sensor*)eb)->impulse += i + 5.f;
        } else {
            ((impact_sensor*)eb)->impulse += i;
        }
    }

    i *= .25f;

    /* dont play sheet metal sound when robot feet hit the ground */
    /* TODO: play another softer sound */
    if (ea) {
        if (ea->is_creature()) {
            if (((creature*)ea)->is_foot_fixture(a)) {
                return;
            }
        }
    }
    if (eb) {
        if (eb->is_creature()) {
            if (((creature*)eb)->is_foot_fixture(b)) {
                return;
            }
        }
    }

    if (i > SM_IMPACT_THRESHOLD) {
        if ((ea || eb) && contact->IsEnabled()) {
            m *material_a = 0;
            m *material_b = 0;

            if (ea) {
                material_a = ea->get_material();
            }
            if (eb) {
                material_b = eb->get_material();
            }

            if (!material_a)
                material_a = &m_border;
            if (!material_b)
                material_b = &m_border;

            /*
            if (material_a->type == TYPE_METAL || material_a->type == TYPE_SHEET_METAL || material_a->type == TYPE_METAL2 ||
                material_b->type == TYPE_METAL || material_b->type == TYPE_SHEET_METAL || material_b->type == TYPE_METAL2) {
                if (i < (SM_IMPACT_THRESHOLD * 2.f)) {
                    return;
                }
            }
            */

            //tms_debugf("impulse %f %f %f", i, rvel.x, rvel.y);
            float impulse_modifier = 15.f;
            float volume = i / impulse_modifier;

            uint8_t combined_type = material_a->type | material_b->type;

            switch (combined_type) {
                case C_WOOD2: case C_WOOD_WOOD2: case C_METAL_WOOD2:
                case C_PLASTIC_WOOD2: case C_METAL2_WOOD2:
                    G->play_sound(SND_WOOD_HOLLOWWOOD, wm.points[0].x, wm.points[0].y, rand(), volume);
                    break;

                case C_WOOD:
                    G->play_sound(SND_WOOD_WOOD, wm.points[0].x, wm.points[0].y, rand(), volume*2.f);
                    break;

                case C_WOOD_METAL2:
                case C_WOOD_METAL:
                    G->play_sound(SND_WOOD_METAL, wm.points[0].x, wm.points[0].y, rand(), volume);
                    break;
                case C_WOOD_PLASTIC:
                    //tms_infof("TODO: wood on plastic");
                    break;

                case C_METAL_METAL2:
                case C_METAL2:
                    G->play_sound(SND_METAL_METAL2, wm.points[0].x, wm.points[0].y, rand(), volume);
                    break;

                case C_METAL_PLASTIC:
                    //tms_infof("TODO: metal on plastic");
                    break;

                case C_METAL:
                    G->play_sound(SND_METAL_METAL, wm.points[0].x, wm.points[0].y, rand(), volume);
                    break;

                case C_SHEET_METAL_WOOD2:
                case C_SHEET_METAL:
                case C_SHEET_METAL_METAL2:
                case C_WOOD_SHEET_METAL:
                case C_METAL_SHEET_METAL:
                case C_SHEET_METAL_PLASTIC:
                    G->play_sound(SND_SHEET_METAL, wm.points[0].x, wm.points[0].y, rand(), volume);
                    break;

                case C_PLASTIC:
                    //tms_infof("TODO: plastic on plastic");
                    break;

                case C_RUBBER:
                case C_WOOD_RUBBER:
                case C_METAL_RUBBER:
                case C_SHEET_METAL_RUBBER:
                case C_PLASTIC_RUBBER:
                case C_RUBBER_WOOD2:
                case C_RUBBER_METAL2:
                    G->play_sound(SND_RUBBER, wm.points[0].x, wm.points[0].y, 0, volume*.5f);
                    break;

                case C_STONE:
                //case C_WOOD_STONE:
                //case C_METAL_STONE:
                //case C_SHEET_METAL_STONE:
                //case C_PLASTIC_STONE:
                //case C_RUBBER_STONE:
                //case C_METAL2_STONE:
                //case C_WOOD2_STONE:
                    G->play_sound(SND_STONE_STONE, wm.points[0].x, wm.points[0].y, rand(), volume*1.5f);
                    break;

                default:
                    //tms_errorf("Undefined audio collision: %d", combined_type);
                    break;
            }

            /* XXX: We're currently just assuming we should use the first point,
             * do we have a reason to change this? */
        }
    }
}

void solver_ingame::EndContact(b2Contact *contact)
{
    contact->first_contact = false;

    b2Fixture *a = contact->GetFixtureA();
    b2Fixture *b = contact->GetFixtureB();

    entity *ea = static_cast<entity*>(a->GetUserData());
    entity *eb = static_cast<entity*>(b->GetUserData());

    if (a->IsSensor() && ea) {
        G->lock();
        ea->on_untouch(a, b);
        G->unlock();
    }
    if (b->IsSensor() && eb) {
        G->lock();
        eb->on_untouch(b, a);
        G->unlock();
    }

    if (ea && eb) {
        if (ea->g_id == O_PIXEL && ea->properties[4].v.i8 != 0)
            b->GetBody()->SetSleepingAllowed(true);
        if (eb->g_id == O_PIXEL && eb->properties[4].v.i8 != 0)
            a->GetBody()->SetSleepingAllowed(true);
    }
}

void solver_ingame::PreSolve(b2Contact *contact, const b2Manifold *manifold)
{
    b2Fixture *a = contact->GetFixtureA();
    b2Fixture *b = contact->GetFixtureB();

    struct ud2_info *a_ud2 = static_cast<struct ud2_info*>(a->GetUserData2());
    struct ud2_info *b_ud2 = static_cast<struct ud2_info*>(b->GetUserData2());

    b2WorldManifold wm;
    contact->GetWorldManifold(&wm);

    entity *ea, *eb;

    ea = static_cast<entity*>(a->GetUserData());
    eb = static_cast<entity*>(b->GetUserData());

    if (ea && eb) {
        /* we have encountered a ladder step fixture.
         * it should not collide with ANYTHING,
         * except creatures that are not climbing */
        if (a_ud2 && a_ud2->get_type() == UD2_LADDER_STEP) {
            bool collide = (ea->g_id == O_LADDER_STEP);
            if (eb->is_creature()) {
                creature *c = static_cast<creature*>(eb);
                float nspeed = c->get_normal_speed();
                if (c->can_climb_ladder()) {
                    if (c->is_foot_fixture(b)) {
                        if (!(b2Dot(wm.normal, c->get_normal_vector(1.f)) > 0.8f) && !c->is_moving_down() && !c->is_currently_climbing()
                                && nspeed > 2.f && nspeed < 5.f) {
                            c->get_body(0)->ApplyForceToCenter(ea->get_body(0)->GetWorldVector(b2Vec2(0.f, 5.f)));
                        }
                        collide = !c->is_moving_down() && !c->is_currently_climbing() && b2Dot(wm.normal, c->get_normal_vector(1.f)) > 0.8f;
                        contact->SetEnabled(collide);
                    } else {
                        collide = false;
                    }
                    if (ea->g_id != O_LADDER_STEP) {
                        c->set_creature_flag(CREATURE_CLIMBING_LADDER, true);
                        c->ladder_id = ea->id;
                        c->ladder_time = 0;
                    }
                } else {
                    collide = false;
                }
            }

            if (eb->g_id == O_LADDER_STEP) {
                collide = false;
            }

            if (eb->g_id == O_ITEM || eb->g_id == O_RESOURCE) {
                collide = false;
            }

            if (!collide) {
                contact->SetEnabled(false);
                return;
            }
        } else if (b_ud2 && b_ud2->get_type() == UD2_LADDER_STEP) {
            bool collide = (eb->g_id == O_LADDER_STEP);
            if (ea->is_creature()) {
                creature *c = static_cast<creature*>(ea);
                float nspeed = c->get_normal_speed();
                if (c->can_climb_ladder()) {
                    if (c->is_foot_fixture(a)) {
                        if (!(b2Dot(wm.normal, c->get_normal_vector(1.f)) < -0.8f) && !c->is_moving_down() && !c->is_currently_climbing()
                                && nspeed > 2.f && nspeed < 5.f) {
                            c->get_body(0)->ApplyForceToCenter(eb->get_body(0)->GetWorldVector(b2Vec2(0.f, 5.f)));
                        }
                        collide = !c->is_moving_down() && !c->is_currently_climbing() && b2Dot(wm.normal, c->get_normal_vector(1.f)) < -0.8f;
                        contact->SetEnabled(collide);
                    } else {
                        collide = false;
                    }

                    if (eb->g_id != O_LADDER_STEP) {
                        c->set_creature_flag(CREATURE_CLIMBING_LADDER, true);
                        c->ladder_id = eb->id;
                        c->ladder_time = 0;
                    }
                } else {
                    collide = false;
                }
            }

            if (ea->g_id == O_LADDER_STEP) {
                collide = false;
            }

            if (ea->g_id == O_ITEM || ea->g_id == O_RESOURCE) {
                collide = false;
            }

            if (!collide) {
                contact->SetEnabled(false);
                return;
            }
        }

        if (ea->g_id == O_LADDER) {
            if (eb->g_id == O_ITEM || eb->g_id == O_RESOURCE) {
                contact->SetEnabled(false);
                return;
            }
        }
        if (eb->g_id == O_LADDER) {
            if (ea->g_id == O_ITEM || ea->g_id == O_RESOURCE) {
                contact->SetEnabled(false);
            }
        }

        if (ea->g_id == O_RESOURCE && eb->is_bullet()) {
            contact->SetEnabled(false);
            return;
        } else if (eb->g_id == O_RESOURCE && ea->is_bullet()) {
            contact->SetEnabled(false);
            return;
        }

        if (ea->g_id == O_PIXEL) {
            pixel *p = static_cast<pixel*>(ea);
            if (p->get_alpha() < .1f) {
                contact->SetEnabled(false);
                return;
            }
        }
        if (eb->g_id == O_PIXEL) {
            pixel *p = static_cast<pixel*>(eb);
            if (p->get_alpha() < .1f) {
                contact->SetEnabled(false);
                return;
            }
        }

        /* robot ladder climbing */
        {
            /* or if the user has a climbing tool */
            if (a_ud2 && a_ud2->get_type() == UD2_CLIMBABLE) {
                if (eb->is_creature()) {
                    creature *c = static_cast<creature*>(eb);
                    if (c->can_climb_ladder() && c->get_body_fixture() == b) {
                        c->set_creature_flag(CREATURE_CLIMBING_LADDER, true);
                        c->ladder_id = ea->id;
                        c->ladder_time = 0;
                    }
                    contact->SetEnabled(false);
                }

                return;
            } else if (b_ud2 && b_ud2->get_type() == UD2_CLIMBABLE) {
                if (ea->is_creature()) {
                    creature *c = static_cast<creature*>(ea);
                    if (c->can_climb_ladder() && c->get_body_fixture() == a) {
                        c->set_creature_flag(CREATURE_CLIMBING_LADDER, true);
                        c->ladder_id = eb->id;
                        c->ladder_time = 0;
                    }
                    contact->SetEnabled(false);
                }

                return;
            }
        }

        if (!a->IsSensor() && !b->IsSensor()) {
            if (!enable_emitted_contact(ea, eb)) {
                contact->SetEnabled(false);
                return;
            }
        }

        if (ea->g_id == O_PLANT) {
            presolve_plant(contact, ea, eb, 1, manifold);
        } else if (eb->g_id == O_PLANT) {
            presolve_plant(contact, ea, eb, 0, manifold);
        }

        if (ea->is_creature()) {
            if (eb->is_creature()) {
                creature *ca = static_cast<creature*>(ea);
                creature *cb = static_cast<creature*>(eb);

                presolve_creature_on_creature(contact, ca, cb, a, b, manifold, wm);
                presolve_creature_on_creature(contact, cb, ca, b, a, manifold, wm);

                if (!ca->feet || !ca->is_foot_fixture(a)) {
                    presolve_creature(contact, ea, eb, 0, manifold);
                    return;
                }
            }

            presolve_creature(contact, ea, eb, 1, manifold);
            return;
        } else if (eb->is_creature()) {
            presolve_creature(contact, ea, eb, 0, manifold);
            return;
        }

        if (ea->g_id == O_CONVEYOR) {
            presolve_conveyor(contact, ea, eb, 1, manifold);
        } else if (eb->g_id == O_CONVEYOR) {
            presolve_conveyor(contact, ea, eb, 0, manifold);
        }

        bool cool = false;

        if (IS_FACTORY(ea->g_id)) {
            factory *fa = static_cast<factory*>(ea);
            if (a == fa->bottom) {
                cool = true;

                b2Vec2 factory_normal;
                tmath_sincos(fa->get_angle(), &factory_normal.x, &factory_normal.y);
                factory_normal.x *= wm.normal.x;
                factory_normal.y *= wm.normal.y;
                //tms_debugf("A factory_normal: %.2f/%.2f", factory_normal.x, factory_normal.y);

                if (factory_normal.y > 0) {
                    contact->SetTangentSpeed(fa->get_tangent_speed());
                }
            }
        } else if (IS_FACTORY(eb->g_id)) {
            factory *fa = static_cast<factory*>(eb);
            if (b == fa->bottom) {
                cool = true;

                b2Vec2 factory_normal;
                tmath_sincos(fa->get_angle(), &factory_normal.x, &factory_normal.y);
                factory_normal.x *= wm.normal.x;
                factory_normal.y *= wm.normal.y;
                //tms_debugf("B factory_normal: %.2f/%.2f", factory_normal.x, factory_normal.y);

                if (factory_normal.y < 0) {
                    contact->SetTangentSpeed(fa->get_tangent_speed());
                }
            }
        }

        if (cool) {
            //tms_debugf("touched factory bottom - %.2f/%.2f", wm.normal.x, wm.normal.y);
        }

        int rev = 0;
        if (ea->type > eb->type) {
            entity *tmp = ea;
            ea = eb;
            eb = tmp;
            rev = 1;
        }

        if (ea->type < 13 && eb->type < 13) {
            if (presolve_handler[ea->type][eb->type]) {
                presolve_handler[ea->type][eb->type](contact, ea, eb, rev, manifold);
            }
        }
    } else {
        if (ea) {
            if (ea->is_creature()) {
                presolve_creature(contact, ea, eb, 1, manifold);
                return;
            }
        }
        if ((eb = static_cast<entity*>(b->GetUserData()))) {
            if (eb->is_creature()) {
                presolve_creature(contact, ea, eb, 0, manifold);
                return;
            }
        }

        if (ea && ea->g_id == O_CONVEYOR) {
            presolve_conveyor(contact, ea, eb, 1, manifold);
        } else if (eb && eb->g_id == O_CONVEYOR) {
            presolve_conveyor(contact, ea, eb, 0, manifold);
        }
    }
}

#define BUTTON_THRESHOLD .75f

void solver_ingame::PostSolve(b2Contact *contact, const b2ContactImpulse *impulse)
{
    b2Fixture *a = contact->GetFixtureA();
    b2Fixture *b = contact->GetFixtureB();

    b2WorldManifold wm;
    contact->GetWorldManifold(&wm);

    entity *ea, *eb;
    ea = static_cast<entity*>(a->GetUserData());
    eb = static_cast<entity*>(b->GetUserData());

    float bullet_modifier = 5.f;

    if (ea) {
        if (ea->is_creature()) {
            postsolve_creature(contact, ea, eb, 1, impulse);
            //return;
        } else if (ea->g_id == O_DUMMY) {
            postsolve_ragdoll(contact, ea, eb, 1, impulse);
        } else if ((ea->g_id == O_BUTTON || ea->g_id == O_TOGGLE_BUTTON)) {
            button *bt = static_cast<button*>(ea);

            if (bt->switch_fx == a || bt->body->GetLocalVector(wm.normal).y > 0.8f) {
                float i = 0.f;
                for (int x = 0; x < impulse->count; ++x) {
                    i += impulse->normalImpulses[x];
                }

                /* We multiple the impulse caused if the causer is a robot */
                if (eb && eb->is_robot()) {
                    i *= 1.75f;
                }

                if (i > BUTTON_THRESHOLD) {
                    bt->press();
                }
            }
        } else if (ea->g_id == O_PRESSURE_SENSOR) {
            /* pressure sensor */
            impact_sensor *is = static_cast<impact_sensor*>(ea);
            for (int x = 0; x < impulse->count; ++x) {
                G->lock();
                is->add_impulse(impulse->normalImpulses[x]);
                G->unlock();
            }
        } else if (ea->g_id == O_LAND_MINE) {
            float i = 0.f;
            for (int x = 0; x < impulse->count; ++x)
                i += impulse->normalImpulses[x];

            if (i >= ((explosive*)ea)->properties[0].v.f) {
                ((explosive*)ea)->triggered = true;
            }
        }
    }
    if (eb) {
        if (eb->is_creature()) {
            postsolve_creature(contact, ea, eb, 0, impulse);
            //return;
        } else if (eb->g_id == O_DUMMY) {
            postsolve_ragdoll(contact, ea, eb, 0, impulse);
        } else if ((eb->g_id == O_BUTTON || eb->g_id == O_TOGGLE_BUTTON)) {
            button *bt = static_cast<button*>(eb);

            b2WorldManifold wm;
            contact->GetWorldManifold(&wm);
            if (bt->switch_fx == b || bt->body->GetLocalVector(wm.normal).y > 0.8f) {
                float i = 0.f;
                for (int x = 0; x < impulse->count; ++x) {
                    i += impulse->normalImpulses[x];
                }

                /* We multiple the impulse caused if the causer is a robot */
                if (ea && ea->is_robot()) {
                    i *= 1.75f;
                }

                if (i > BUTTON_THRESHOLD) {
                    bt->press();
                }
            }
        } else if (eb->g_id == O_PRESSURE_SENSOR) {
            impact_sensor *is = static_cast<impact_sensor*>(eb);
            for (int x = 0; x < impulse->count; ++x) {
                G->lock();
                is->add_impulse(impulse->normalImpulses[x]);
                G->unlock();
            }
        } else if (eb->g_id == O_LAND_MINE) {
            float i = 0.f;
            for (int x = 0; x < impulse->count; ++x)
                i += impulse->normalImpulses[x];

            if (i >= ((explosive*)eb)->properties[0].v.f) {
                ((explosive*)eb)->triggered = true;
            }
        }
    }
}

static void
presolve_cog_cog(b2Contact *contact, entity *a, entity *b, int rev, const b2Manifold *man)
{
    gear *ga = (gear*)a;
    gear *gb = (gear*)b;

    if (ga->outer_fixture == contact->GetFixtureA() && gb->outer_fixture == contact->GetFixtureB()) {

        if (ga->connected_to(gb)) {
            contact->SetEnabled(false);
            return;
        }

        contact->SetFriction(0.f);

        b2Vec2 c = ga->get_body(0)->GetPosition() + gb->get_body(0)->GetPosition();
        c *= .5f;

        b2Vec2 p1 = ga->get_body(0)->GetLocalPoint(c);
        b2Vec2 p2 = gb->get_body(0)->GetLocalPoint(c);

        p1 *= 1.f/(p1.Length());
        p2 *= 1.f/(p2.Length());

        double s1 = (M_PI/6.f) / ga->get_ratio();
        double s2 = (M_PI/6.f) / gb->get_ratio();

        double a1 = tmath_atan2add(p1.y, p1.x) + (M_PI/12.f)/ga->get_ratio();
        double a2 = tmath_atan2add(p2.y, p2.x);

        double na1 = roundf(a1/s1)*s1;
        double na2 = roundf(a2/s2)*s2;

        a1 = na1-a1;
        a2 = -(na2-a2);

        /*
        a1 = fabs(remainder(a1/ ((M_PI/6.) / ga->get_ratio()), 1.));
        a2 = fabs(remainder(a2/ ((M_PI/6.) / gb->get_ratio()), 1.));

        a1 -= .5f;
        a2 -= .5f;
        */

        //a2 = -a2;

        //a1 = fabsf(a1);
        //a2 = fabsf(a2);

        double diff = a2-a1;

#define EPS .1f

        //if ()
        //
        //if (fabsf(a1) < .2f) tms_infof("A1 ALLOWED");
        //if (fabsf(a2) < .2f) tms_infof("A2 ALLOWED");

        //bool a1_allowed = fabsf(a1) < .2f;
        //bool a2_allowed = fabsf(a2) < .2f;

        /*
        if (a1_allowed != a2_allowed) {
            contact->SetEnabled(false);
            tms_infof("test");

        }
        */

        //if (fabsf(diff) > EPS && fabsf(diff) < (1.f-EPS)) {
        if (std::abs(diff) < EPS) {
            contact->SetEnabled(false);

            /* attach the gears to each other */
            if (ga->joint)
                ga->pending = gb;

            //tms_infof("fitting %f %f %f", diff, a1, a2);
        }else {
            //tms_infof("diff %f %f %f", diff, a1, a2);
        }
    }
}

static void
postsolve_creature(b2Contact *contact, entity *a, entity *b, int rev, const b2ContactImpulse *impulse)
{
    creature *c;

    b2Fixture *fa = contact->GetFixtureA(), *fb = contact->GetFixtureB();
    b2Fixture *cf, *of;
    entity *oe;

    if (rev) {
        c = static_cast<creature*>(a);
        cf = fa;
        of = fb;
        oe = b;
        if (fa == c->get_sensor_fixture())
            return;
    } else {
        c = static_cast<creature*>(b);
        cf = fb;
        of = fa;
        oe = a;
        if (fb == c->get_sensor_fixture())
            return;
    }

    if (c->is_foot_fixture(cf) && W->level.flag_active(LVL_DISABLE_FALL_DAMAGE)) {
        /* ignoring fall damage */
        return;
    }

    float i = 0.f;

    if (contact->first_contact) {
        contact->first_contact = false;

        float i = 0.f;
        float multiplier = c->get_damage_multiplier(cf);
        float sensitivity = c->get_damage_sensitivity(cf);

        for (int x=0; x<impulse->count; ++x)
            i += impulse->normalImpulses[x];

        b2MassData m;
        cf->GetBody()->GetMassData(&m);

        i *= 1.f/(m.mass);
        i *= contact->rel_speed * multiplier;

        if (i > sensitivity) {
            G->lock();
            c->damage((i-sensitivity)*.35f, cf, DAMAGE_TYPE_FORCE, DAMAGE_SOURCE_WORLD, 0);
            G->unlock();
        }
    }
}

static void
presolve_plant(b2Contact *contact, entity *a, entity *b, int rev, const b2Manifold *man)
{
    plant *p;
    entity *o;

    b2Fixture *fa = contact->GetFixtureA(), *fb = contact->GetFixtureB();

    b2Fixture *pf, *of;

    if (rev) {
        p = ((plant*)a);
        o = ((entity*)b);
        pf = fa;
        of = fb;
    } else {
        p = ((plant*)b);
        o = ((entity*)a);
        pf = fb;
        of = fa;
    }

    plant_section *s = (plant_section*)pf->GetUserData2();

    if (s && o) {

        if (p == o) {
            contact->SetEnabled(false);
            return;
        }

        if (!p->c.pending && (s == p->root_branch.first) && (o->g_id == O_TPIXEL || o->g_id == O_CHUNK)) {
            contact->SetEnabled(false);
        }
    } else if (o) {
        if (pf == p->fx && (o->g_id == O_TPIXEL || o->g_id == O_CHUNK)) {
            contact->SetEnabled(false);
        }
    }
}

static void
begincontact_plant(b2Contact *contact, entity *a, entity *b, int rev)
{
}

static void
begincontact_creature(b2Contact *contact, entity *a, entity *b, int rev)
{
    creature *c;
    entity *o;

    b2Fixture *fa = contact->GetFixtureA(), *fb = contact->GetFixtureB();

    if (fa->GetUserData() == fb->GetUserData()) {
        contact->SetEnabled(false);
        return;
    }

    b2Fixture *cf, *of;

    if (rev) {
        c = ((creature*)a);
        o = ((entity*)b);
        cf = fa;
        of = fb;
    } else {
        c = ((creature*)b);
        o = ((entity*)a);
        cf = fb;
        of = fa;
    }

    if (cf->IsSensor()) return;

    if (o) {
        if (c->is_robot()) {
            robot_base *r = (robot_base*)c;

            if (r->is_sensor_fixture(cf)) {
                return;
            }

            if (o->g_id == O_ITEM) {
                item *con = static_cast<item*>(o);
                bool require_interact = true;
                contact->SetEnabled(false);

                switch (con->item_category) {
                    case ITEM_CATEGORY_WEAPON:
                    case ITEM_CATEGORY_TOOL:
                        require_interact = false;
                        break;
                }

                G->lock();
                if (!require_interact || G->interacting_with(o) || (r->roam_target_type == TARGET_ITEM && r->roam_target_id == o->id)) {
                    if (!con->flag_active(ENTITY_IS_ABSORBED)) {
                        if (r->is_alive()) {
                            if (o->emitted_by != r->id && r->consume(con, false)) {
                                G->absorb(o);
                                r->consume_timer = 1.f;
                                G->play_sound(SND_DROP_ABSORB, 0.f, 0.f, 0, 1.f, false, 0, true);
                            }
                        }
                    }
                }
                G->unlock();

            }
        }

        if (c->is_dead()) {
            if (o->g_id == O_RESOURCE) {
                contact->SetEnabled(false);
            }
            return;
        }

        if (o->g_id == O_RESOURCE) {
            resource *d = static_cast<resource*>(o);
            contact->SetEnabled(false);
            G->lock();
            if (G->absorb(o)) {
                c->add_resource(d->resource_type, d->amount);
                G->play_sound(SND_DROP_ABSORB, 0.f, 0.f, 0, 1.f, false, 0, true);
            }
            G->unlock();
        }
    }
}

static void
presolve_creature(b2Contact *contact, entity *a, entity *b, int rev, const b2Manifold *man)
{
    creature *c = 0;
    robot_base *r = 0;
    entity *o;
    float base_tangent = 0.f;

    b2Fixture *fa = contact->GetFixtureA(), *fb = contact->GetFixtureB();

    if (fa->GetUserData() == fb->GetUserData()) {
        contact->SetEnabled(false);
        return;
    }

    b2Fixture *cf, *of;

    if (rev) {
        c = ((creature*)a);
        o = ((entity*)b);
        cf = fa;
        of = fb;
    } else {
        c = ((creature*)b);
        o = ((entity*)a);
        cf = fb;
        of = fa;
    }

    if (o && o->g_id == O_RESOURCE) {
        contact->SetEnabled(false);
        return;
    }

    if (o && o->g_id == O_CONVEYOR) {
        base_tangent = ((conveyor*)o)->get_tangent_speed();
    } else if (o && IS_FACTORY(o->g_id) && ((factory*)o)->bottom == of) {
        base_tangent = ((factory*)o)->get_tangent_speed();
    }

    if (c->is_foot_fixture(cf)) {
        if (o && o->g_id == O_ITEM) {
            item *i = static_cast<item*>(o);
            if (i->get_item_type() == ITEM_BULLET || i->get_item_type() == ITEM_SHOTGUN_PELLET) {
                contact->SetEnabled(false);
                return;
            }
        }

        contact->SetFriction(b2MixFriction(fa->GetFriction(), fb->GetFriction()));
        contact->SetTangentSpeed(0.f);

        c->feet->handle_contact(contact, cf, of, man, base_tangent, rev);
    } else {
        if (c->on_ground <= 0.f) {
            b2WorldManifold wm;
            contact->GetWorldManifold(&wm);
            b2Vec2 vel = of->GetBody()->GetLinearVelocityFromWorldPoint(wm.points[0]);
            /*c->set_ground_speed(
                    (b2Dot(c->get_tangent_vector(1.f), vel)+base_tangent),
                    b2Dot(c->get_normal_vector(1.f), vel)
                    );*/
/*
            c->last_ground_speed = b2Vec2(
                    (b2Dot(c->get_tangent_vector(1.f), vel)+base_tangent),
                    b2Dot(c->get_normal_vector(1.f), vel)
                    );
                    */
            //tms_debugf("settings ground speed to %f %f due to coll", c->last_ground_speed.x, c->last_ground_speed.y);
        }

        if (o && o->g_id == O_CONVEYOR) {
            contact->SetTangentSpeed(((conveyor*)o)->get_tangent_speed());
        } else if (o && IS_FACTORY(o->g_id) && ((factory*)o)->bottom == of) {
            contact->SetTangentSpeed(((factory*)o)->get_tangent_speed());
        }
    }
}

static void
presolve_creature_on_creature(b2Contact *contact,
        creature *a, creature *b,
        b2Fixture *fa, b2Fixture *fb,
        const b2Manifold *man,
        const b2WorldManifold &wm)
{
    if (a->is_spikebot() && !b->is_spikebot()) {
        spikebot *sb = static_cast<spikebot*>(a);

        if (sb->is_enemy(b)) {
            G->lock();
            b->damage(sb->get_damage(), fb, DAMAGE_TYPE_FORCE, DAMAGE_SOURCE_WORLD, 0);

            if (rand()%10 == 0) {
                G->emit(new spark_effect(
                            wm.points[0],
                            b->get_layer()
                            ), 0);
                G->emit(new smoke_effect(
                            wm.points[0],
                            a->get_layer(),
                            .5f,
                            .5f
                            ), 0);
            }
            G->unlock();
        }
    }
}

static void
presolve_conveyor(b2Contact *contact, entity *a, entity *b, int rev, const b2Manifold *man)
{
    conveyor *c;

    if (rev) {
        c = static_cast<conveyor*>(a);
    } else {
        c = static_cast<conveyor*>(b);
    }

    contact->SetTangentSpeed(c->get_tangent_speed());
}

static void
presolve_pipeline_ball(b2Contact *contact, entity *a, entity *_b, int rev, const b2Manifold *man)
{
    pipeline *p = (pipeline*)a;
    ball *b = (ball*)_b;

    if (!p->used) {
        G->lock();
        p->take(b);
        G->unlock();
        contact->SetEnabled(false);
    } else if (p->b == b)
        contact->SetEnabled(false);
}

static void
postsolve_ragdoll(b2Contact *contact, entity *a, entity *b, int rev, const b2ContactImpulse *impulse)
{
    ragdoll *r;
    b2Fixture *r_fixture;
    b2Body *r_body;
    struct limb *r_limb;

    entity *o;
    b2Fixture *o_fixture;
    b2Body *o_body;

    b2Fixture *fa = contact->GetFixtureA(), *fb = contact->GetFixtureB();

    if (rev) {
        r = static_cast<ragdoll*>(a);
        r_fixture = fa;

        o = b;
        o_fixture = fb;
    } else {
        r = static_cast<ragdoll*>(b);
        r_fixture = fb;

        o = a;
        o_fixture = fa;
    }

    if (r->properties[9*3].v.f == 100.f) {
        /* The ragdoll is indestructible, abort */
        return;
    }

    r_body = r_fixture->GetBody();
    o_body = o_fixture->GetBody();

    r_limb = r->get_limb(r_body);

    /* XXX: These variables require tweaking */
    float threshold = 8.f;
    float durability_multiplier = 0.03f;

    float i = 0.f;

    for (int x=0; x<impulse->count; ++x)
        i += impulse->normalImpulses[x];

    threshold *= (r->properties[9*3].v.f * durability_multiplier);

    if (i > threshold) {
        if (o && o == r) {
            /* XXX: Ragdoll collided with itself, do nothing? */
        } else {
            b2JointEdge *je = 0;

            if (r_limb)
                je = (*r_limb->body)->GetJointList();
            else
                je = r->body->GetJointList();

            for (; je != NULL && i > threshold; je = je->next) {
                G->lock();
                G->destroy_joint(je->joint);
                G->unlock();

                i -= 1.f;
            }
        }
    }
}
