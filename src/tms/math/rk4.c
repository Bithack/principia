#include "rk4.h"

/**
 * 4th order Runge-Kutta ODE solver
 **/
void
tmath_ode_solver_rk4(float now, float step,
                   void (*d)(float t, float *v, float *out),
                   float *v, int dim
                   )
{
    int   x;
#ifdef _MSC_VER
	float *t0 = malloc(dim*sizeof(float));
	float *t1 = malloc(dim*sizeof(float));
	float *t2 = malloc(dim*sizeof(float));
	float *t3 = malloc(dim*sizeof(float));
	float *k  = malloc(dim*sizeof(float));
#else
    float t0[dim], t1[dim],
          t2[dim], t3[dim],
          k[dim];
#endif

    d(now, v, t0);
    for (x=0; x<dim; x++) k[x] = v[x] + t0[x] * step/2.f;
    d(now+step/2.f, v, t1);
    for (x=0; x<dim; x++) k[x] = v[x] + t1[x] * step/2.f;
    d(now+step/2.f, v, t2);
    for (x=0; x<dim; x++) k[x] = v[x] + t2[x] * step;
    d(now+step, v, t3);

    for (x=0; x<dim; x++) v[x] = v[x] + (t0[x] + 2.f * t1[x] + 2.f * t2[x] + 2.f * t3[x]) * step/6.f;

#ifdef _MSC_VER
	free(t0);
	free(t1);
	free(t2);
	free(t3);
	free(k);
#endif
}

