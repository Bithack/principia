#ifndef _NOISE__H_
#define _NOISE__H_

void  _noise_init_perm(unsigned long seed);
float _noise1 (float x);
float _noise2 (float x, float y);
float _noise3 (float x, float y, float z);
float _noise4 (float x, float y, float z, float w);

#endif
