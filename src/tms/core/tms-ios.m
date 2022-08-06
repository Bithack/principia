#import <QuartzCore/CAAnimation.h>
#include <stdint.h>

uint64_t tms_IOS_get_time()
{
    return (uint64_t)round(CACurrentMediaTime()*1000000.);
}