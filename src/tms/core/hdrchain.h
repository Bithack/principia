#ifndef _TMS_HDRBUFFER__H_
#define _TMS_HDRBUFFER__H_

struct tms_hdrbuffer {
    struct tms_framebuffer super;
    struct tms_framebuffer *downsampled[6];
};

tms_hdrbuffer_bind();
tms_hdrbuffer_unbind();

#endif
