#pragma once

#include <map>
#include <Box2D/Box2D.h>
#include <tms/math/vector.h>
#include "creature.hh"


class entity;
class creature;

enum attachment_type {
    /* No attachment between the activator and the activatee is made. */
    ATTACHMENT_NONE,

    /* A joint is created between the activator and the activatee */
    ATTACHMENT_JOINT,

    /* No joint is created, but the activate is commanded to stand still
     * using the FIXED-flag. */
    ATTACHMENT_FIXED,
};

#define ACTIVATOR_REQUIRE_RIDING_CIRCUIT (1ULL << 0)

class activator
{
  protected:
    std::map<creature*, int> visitors;

  public:
    bool active;
    enum attachment_type attachment_type;
    uint64_t flags;

    activator(enum attachment_type _at, uint64_t _flags=0)
        : active(false)
        , attachment_type(_at)
        , flags(_flags)
    { }

    virtual b2Vec2 get_activator_pos() = 0;
    virtual void activate(creature *by) = 0;
    virtual entity* get_activator_entity() = 0;
    virtual float get_activator_radius() = 0;

    bool activator_touched(b2Fixture *other);
    bool activator_untouched(b2Fixture *other);
};
