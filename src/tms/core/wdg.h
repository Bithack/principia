#ifndef _TMS_WDG__H_
#define _TMS_WDG__H_

#define WDG_MAX_P 24

#define TMS_WDG_CHECKBOX   0
#define TMS_WDG_BUTTON     1
#define TMS_WDG_SLIDER     2
#define TMS_WDG_ROLLER     3
#define TMS_WDG_RADIAL     4
#define TMS_WDG_SPINNER    5
#define TMS_WDG_VSLIDER    6
#define TMS_WDG_FIELD      7
#define TMS_WDG_BTN_RADIAL 8 /* radial with button in center */

#include <tms/math/vector.h>

#ifdef __cplusplus
extern "C" {
#endif

struct tms_surface;
struct tms_sprite;

struct tms_wdg
{
    struct tms_surface *surface;
    tvec2 pos;
    tvec2 size;
    float extra_up;
    float extra_right;
    float extra_down;
    float extra_left;
    float value[2];
    float ghost[2];
    float snap[2];
    float alpha;
    int focused;
    int hovered;
    int faded;
    int enable_ghost;
    int type;
    int clickthrough;
    void *data;
    void *data2;
    void *data3;
    tvec3 *color;

    struct tms_sprite *s[2];

    void (*render)(struct tms_wdg *w, struct tms_surface *s);
    void (*post_render)(struct tms_wdg *w, struct tms_surface *s);
    void (*on_change)(struct tms_wdg *w, float values[2]);
    void (*on_focus_change)(struct tms_wdg *w);

    void (*touch_down)(struct tms_wdg *w, int pid, int bid, int ox, int oy, float rx, float ry);
    void (*touch_up)(struct tms_wdg *w, int pid, int bid, int ox, int oy, float rx, float ry);
    void (*touch_drag)(struct tms_wdg *w, int pid, int bid, int ox, int oy, float rx, float ry);

    void (*mouse_over)(struct tms_wdg *w);
    void (*mouse_out)(struct tms_wdg *w);
};

struct tms_wdg *tms_wdg_alloc(int type, struct tms_sprite *s0, struct tms_sprite *s1);
void tms_wdg_set_active(struct tms_wdg *w, int active);
void tms_wdg_init(struct tms_wdg *w, int type, struct tms_sprite *s0, struct tms_sprite *s1);
void tms_wdg_free(struct tms_wdg *wdg);

/* various render methods that we need to have accessible from the outside */
void tms_wdg_slider_render(struct tms_wdg *w, struct tms_surface *s);
void tms_wdg_vslider_render(struct tms_wdg *w, struct tms_surface *s);
void tms_wdg_field_render(struct tms_wdg *w, struct tms_surface *s);
void tms_wdg_radial_render(struct tms_wdg *w, struct tms_surface *s);

#ifdef __cplusplus
}
#endif

#endif

