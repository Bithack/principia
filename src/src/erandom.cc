#include "erandom.hh"

edevice*
erandom::solve_electronics(void)
{
    this->s_out[0].write((float)rand()/(float)RAND_MAX);
    return 0;
}