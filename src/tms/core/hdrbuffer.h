#ifndef _TMS_HDRBUFFER__H_
#define _TMS_HDRBUFFER__H_

#include "framebuffer.h"

struct tms_hdrbuffer {
    struct tms_fb super;
    struct tms_fb *downsampled;
    struct tms_fb *luminance[5];
};

struct tms_hdrbuffer* tms_hdrbuffer_alloc(void);
void tms_hdrbuffer_unbind(struct tms_hdrbuffer *h);
void tms_hdrbuffer_bind(struct tms_hdrbuffer *h);

#endif
