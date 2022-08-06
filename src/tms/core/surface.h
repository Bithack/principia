#ifndef _TMS_SURFACE__H_
#define _TMS_SURFACE__H_

/** @relates tms_surface @{ */

struct tms_wdg;
struct tms_event;
struct tms_ddraw;
struct tms_atlas;

#define TMS_MAX_POINTER_ID 24

struct tms_surface {
    struct tms_wdg **widgets;
    int widget_count;
    int widget_cap;
    float alpha;

    struct tms_atlas *atlas;

    struct tms_wdg *active_widgets[TMS_MAX_POINTER_ID];
    struct tms_ddraw *ddraw; /* XXX: temporary, should not use ddraw */
};

struct tms_surface *tms_surface_alloc(void);
int tms_surface_remove_widget(struct tms_surface *s, struct tms_wdg *w);
int tms_surface_add_widget(struct tms_surface *s, struct tms_wdg *w);
int tms_surface_add_widgets(struct tms_surface *s, int num_widgets, ...);
int tms_surface_render(struct tms_surface *s);
int tms_surface_step(struct tms_surface *s);
int tms_surface_init(struct tms_surface *s);
int tms_surface_handle_input(struct tms_surface *s, const struct tms_event *ev, int action);

#endif
