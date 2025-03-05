#include "activator.hh"
#include "world.hh"
#include "creature.hh"
#include "game.hh"

bool
activator::activator_touched(b2Fixture *other)
{
    if (other->IsSensor()) {
        return false;
    }

    entity *e = static_cast<entity*>(other->GetUserData());

    if (!e || !e->is_creature()) {
        return false;
    }

    this->active = true;

    creature *c = static_cast<creature*>(e);

    if (this->flags & ACTIVATOR_REQUIRE_RIDING_CIRCUIT && !c->has_circuit(CREATURE_CIRCUIT_RIDING)) {
        return false;
    }

    std::pair<std::map<creature*, int>::iterator, bool> ret;
    ret = this->visitors.insert(std::pair<creature*, int>(c, 1));
    if (!ret.second) {
        (ret.first)->second += 1;
    } else {
        c->activators.insert(this);
    }

    return true;
}

bool
activator::activator_untouched(b2Fixture *other)
{
    if (other->IsSensor()) return false;

    entity *e = static_cast<entity*>(other->GetUserData());

    if (!e || !e->is_creature()) {
        return false;
    }

    creature *c = static_cast<creature*>(e);

    std::map<creature*, int>::iterator ret = this->visitors.find(c);

    if (ret != this->visitors.end()) {
        if (ret->second > 1) {
            ret->second -= 1;
        } else {
            this->visitors.erase(c);
            c->activators.erase(this);

            this->active = !this->visitors.empty();
        }
    }

    return true;
}
