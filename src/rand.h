#ifndef _MTRAND__H_
#define _MTRAND__H_

#ifdef __cplusplus
extern "C" {
#endif

void init_genrand(unsigned long s);
unsigned long genrand_int32(void);

#ifdef __cplusplus
}
#endif


#endif
