#include "solver.hh"
#include "entity.hh"
#include "gear.hh"
#include <tms/bindings/cpp/cpp.hh>

#define NUM_HANDLERS 13

static void presolve_cog_cog(b2Contact *contact, entity *a, entity *b, int rev, const b2Manifold *man);

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
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

static void (*begin_handler[13][13])(b2Contact *contact, entity *a, entity *b, int rev) =
{
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

static void (*end_handler[13][13])(b2Contact *contact, entity *a, entity *b, int rev) =
{
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

void solver::BeginContact(b2Contact *contact)
{
    b2Fixture *a = contact->GetFixtureA();
    b2Fixture *b = contact->GetFixtureB();

    if (a->IsSensor()) {
        if (b->IsSensor()) {
            entity *ea = static_cast<entity*>(a->GetUserData());
            if (ea) {
                ea->on_paused_touch(a, b);
            }
            entity *eb = static_cast<entity*>(b->GetUserData());
            if (eb) {
                eb->on_paused_touch(b, a);
            }
            return;
        }

        entity *ea = static_cast<entity*>(a->GetUserData());
        if (ea) {
            ea->on_paused_touch(a, b);
        }

        return;
    } else if (b->IsSensor()) {
        if (a->IsSensor()) {
            entity *ea = static_cast<entity*>(a->GetUserData());
            if (ea) {
                ea->on_paused_touch(a, b);
            }
            entity *eb = static_cast<entity*>(b->GetUserData());
            if (eb) {
                eb->on_paused_touch(b, a);
            }
            return;
        }

        entity *ea = static_cast<entity*>(b->GetUserData());
        if (ea)
            ea->on_paused_touch(b, a);

        return;
    }

    entity *ea, *eb;

    if ((ea = static_cast<entity*>(a->GetUserData())) && (eb = static_cast<entity*>(b->GetUserData())) ){
        int rev = 0;
        if (ea->type > eb->type) {
            entity *tmp = ea;
            ea = eb;
            eb = tmp;
            rev = 1;
        }

        if (ea->type < 13 && eb->type < 13 && begin_handler[ea->type][eb->type]) {
            begin_handler[ea->type][eb->type](contact, ea, eb, rev);
        }
    }
}

void solver::EndContact(b2Contact *contact)
{
    b2Fixture *a = contact->GetFixtureA();
    b2Fixture *b = contact->GetFixtureB();

    if (a->IsSensor()) {
        if (b->IsSensor())
            return;

        entity *ea = static_cast<entity*>(a->GetUserData());
        if (ea) {
            ea->on_paused_untouch(a, b);
        }

        return;
    } else if (b->IsSensor()) {
        entity *ea = static_cast<entity*>(b->GetUserData());
        if (ea)
            ea->on_paused_untouch(b, a);

        return;
    }

    entity *ea, *eb;

    if ((ea = static_cast<entity*>(a->GetUserData())) && (eb = static_cast<entity*>(b->GetUserData())) ){
        int rev = 0;
        if (ea->type > eb->type) {
            entity *tmp = ea;
            ea = eb;
            eb = tmp;
            rev = 1;
        }

        if (end_handler[ea->type][eb->type]) {
            end_handler[ea->type][eb->type](contact, ea, eb, rev);
        }
    }
}

void solver::PreSolve(b2Contact *contact, const b2Manifold *manifold)
{
    b2Fixture *a = contact->GetFixtureA();
    b2Fixture *b = contact->GetFixtureB();

    entity *ea, *eb;

    if ((ea = static_cast<entity*>(a->GetUserData())) && (eb = static_cast<entity*>(b->GetUserData())) ){
        int rev = 0;
        if (ea->type > eb->type) {
            entity *tmp = ea;
            ea = eb;
            eb = tmp;
            rev = 1;
        }

        if (ea->flag_active(ENTITY_IS_CREATURE) && eb->flag_active(ENTITY_IS_CREATURE) && ea->id == eb->id) {
            contact->SetEnabled(false);
            return;
        }

        //if (ea && eb && ea->body && eb->body && ea->type < NUM_HANDLERS && eb->type < NUM_HANDLERS && ea->type > 0 && eb->type > 0) {
            if (presolve_handler[ea->type][eb->type]) {
                presolve_handler[ea->type][eb->type](contact, ea, eb, rev, manifold);
            }
        //}
    }
}

void solver::PostSolve(b2Contact *contact, const b2ContactImpulse *impulse)
{

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

        }else {
        }
    }
}

