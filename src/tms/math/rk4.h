#ifndef _TMATH_RK4__H_
#define _TMATH_RK4__H_

void tmath_ode_solver_rk4(float now, float step, void (*d)(float t, float *v, float *out), float *v, int dim);

#endif
