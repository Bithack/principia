
#include <math.h>

#include "intersect.h"

/**
 * Compare the point and the line in a sense similar to strcmp(),
 * the function will return < 0 if the point is "below" the line,
 * 0 if the point is on the line, and > 0 if the point is above the
 * line.
 *
 * @relates tintersect
 **/
float
tintersect_point_line_cmp(tvec2 *l1, tvec2 *l2, tvec2 *p)
{
    return (l2->x - l1->x) * (p->y - l1->y) - (l2->y - l1->y) * (p->x - l1->x);
}

/**
 * Intersect two infinite lines
 *
 * @relates tintersect
 **/
int
tintersect_lines(tvec2 a1, tvec2 a2, tvec2 b1, tvec2 b2, tvec2 *point)
{
    float d1 = tvec2_det(a1, a2);
    float d2 = tvec2_det(b1, b2);

    tvec2 v1 = (tvec2){a1.x - a2.x, a1.y - a2.y};
    tvec2 v2 = (tvec2){b1.x - b2.x, b1.y - b2.y};

    float d3 = tvec2_det(v1, v2);

    point->x = tvec2_detf(d1, v1.x, d2, v2.x) / d3;
    point->y = tvec2_detf(d1, v1.y, d2, v2.y) / d3;

    return 1;
}

int
tintersect_segments(tvec2 a1, tvec2 a2, tvec2 b1, tvec2 b2, tvec2 *point)
{
    float d = (b2.y-b1.y)*(a2.x-a1.x) - (b2.x-b1.x)*(a2.y-a1.y);
    if (d == 0.f)
        return 0;

    float a = ((b2.x-b1.x)*(a1.y-b1.y) - (b2.y-b1.y)*(a1.x-b1.x)) / d;
    float b = ((a2.x-a1.x)*(a1.y-b1.y) - (a2.y-a1.y)*(a1.x-b1.x)) / d;

    if (a < 0.f || a > 1.f)
        return 0;
    if (b < 0.f || b > 1.f)
        return 0;

    point->x = a1.x + (a2.x-a1.x)*a;
    point->y = a1.y + (a2.y-a1.y)*a;

    return 1;
}

/**
 * Intersect a ray with a plane.
 *
 * @relates tintersect
 **/
int
tintersect_ray_plane(tvec3 *origin, tvec3 *dir,
                     tvec4 *plane, tvec3 *intersection)
{
    float d = tvec3_dot(dir, (tvec3*)plane);
    if (fabsf(d)>0.00001f) {
        float t = -(tvec3_dot(origin, (tvec3*)plane) + plane->w)/d;
        if (t<0.f) return 0;
        intersection->x = origin->x + (dir->x*t);
        intersection->y = origin->y + (dir->y*t);
        intersection->z = origin->z + (dir->z*t);
        return 1;
    } else {
        /* TODO: check if the origin is in the plane */
    }
    return 0;
}

int
tintersect_point_rect(tvec2 *point, tvec2 *rect_pos, tvec2 *rect_size)
{
    return ((point->x > rect_pos->x-(rect_size->w/2)) &&
            (point->x < rect_pos->x+(rect_size->w/2)) &&
            (point->y > rect_pos->y-(rect_size->h/2)) &&
            (point->y < rect_pos->y+(rect_size->h/2)));
}
