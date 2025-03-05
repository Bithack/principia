#include "faction.hh"

/** Definitions
 * relationships = How the faction should behave to the specified faction
 *                 Friendly = Help them by default
 *                 Enemy = Attack them by default
 *                 Neutral = Attack only when shot
 * decr moods = How much the mood will decrease with each logic step
 * sensitivity = what mood additions should be multiplied with
 **/

/** Moods
 * 0 = Anger
 * 1 = Bravery
 * 2 = Fear
 **/

faction_info factions[NUM_FACTIONS] = {
    {
        FACTION_ENEMY,
        "Enemy",
        (tvec4){0.9f, 0.6f, 0.6f, 1.0f},
        /* relationships */
        { FRIENDLY, ENEMY, NEUTRAL, ENEMY },
        /* base moods */
        { .2f, .2f, .0f },
        /* decr moods */
        { -.01f, -.01f, -.01f },
        /* sensitivity */
        { .5f, .5f, .5f },
    },
    {
        FACTION_FRIENDLY,
        "Friendly",
        (tvec4){0.6f, 0.9f, 0.6f, 1.0f},
        /* relationships */
        { ENEMY, FRIENDLY, NEUTRAL, ENEMY },
        /* base moods */
        { .2f, .2f, .0f },
        /* decr moods */
        { -.01f, -.01f, -.01f },
        /* sensitivity */
        { .5f, .5f, .5f },
    },
    {
        FACTION_NEUTRAL,
        "Neutral",
        (tvec4){0.8f, 0.8f, 0.8f, 1.0f},
        /* relationships */
        { NEUTRAL, NEUTRAL, FRIENDLY, NEUTRAL },
        /* base moods */
        { .2f, .2f, .0f },
        /* decr moods */
        { -.01f, -.01f, -.01f },
        /* sensitivity */
        { 1.5f, .1f, .1f },
    },
    {
        FACTION_CHAOTIC,
        "Chaotic",
        (tvec4){0.6f, 0.6f, 0.9f, 1.0f},
        /* relationships */
        { ENEMY, ENEMY, ENEMY, ENEMY },
        /* base moods */
        { .4f, .2f, .0f },
        /* decr moods */
        { -.01f, -.01f, -.01f },
        /* sensitivity */
        { 1.5f, .1f, .1f },
    },
};
