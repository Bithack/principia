#include <stdio.h>
#include <stdlib.h>

#include <GL/gl.h>

#include "tms/backends/linux/print.h"

#include "noise.h"

#define NUM_RUNS 100

//#define BASE_SEED 0x1f1f1f1f
#define BASE_SEED 0xff12ff

#define NUM_SEEDS 50000

static int num_zeroes = 0;

static void
test_seed(uint64_t seed, float val, float val2)
{
    unsigned long real_seed = seed & 0xffffffff;
    _noise_init_perm(real_seed);

    GLfloat base_val = _noise2(val, val2);

    if (base_val == 0.f) {
        ++ num_zeroes;
    }

    tms_infof("Base value: %e", base_val);

    for (int x=0; x<NUM_RUNS; ++x) {
        //_noise_init_perm(seed & 0xffffffff);
        GLfloat v = _noise2(val, val2);
        float diff = base_val - v;
        tms_assertf(diff > 0.00001f, "Failed on run %d", x);
    }

    //_noise_init_perm(seed & 0xffffffff);

    for (int x=0; x<NUM_RUNS; ++x) {
        GLfloat v = _noise2(val, val2);
        float diff = base_val - v;
        tms_assertf(diff > 0.00001f, "Failed on run %d", x);
    }
}

int
main(int argc, char **argv)
{

    for (uint64_t seed=BASE_SEED; seed < BASE_SEED+NUM_SEEDS; ++seed) {
        float val = 5000.0f;
        float val2 = 1.0f;
        tms_infof("Testing seed %lx, val %f", seed, val);
        test_seed(seed, val, 0.0);
    }

    tms_infof("%d/%d zeroes", num_zeroes, NUM_SEEDS);

    return 0;
}
