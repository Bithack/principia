#ifndef _TMS_TRANSITION__H_
#define _TMS_TRANSITION__H_

struct tms_transition {
    int (*begin)(struct tms_screen *old, struct tms_screen *n);
    int (*render)(long time);
    int (*end)(void);
};

#endif
