#define PJ_LIB__
#include <errno.h>
#include <proj.h>
#include "projects.h"

PROJ_HEAD(urm5, "Urmaev V") "\n\tPCyl., Sph., no inv.\n\tn= q= alpha=";

struct pj_opaque {
    double m, rmn, q3, n;
};


static XY s_forward (LP lp, PJ *P) {           /* Spheroidal, forward */
    XY xy = {0.0, 0.0};
    struct pj_opaque *Q = P->opaque;
    double t;

    t = lp.phi = aasin (P->ctx, Q->n * sin (lp.phi));
    xy.x = Q->m * lp.lam * cos (lp.phi);
    t *= t;
    xy.y = lp.phi * (1. + t * Q->q3) * Q->rmn;
    return xy;
}


PJ *PROJECTION(urm5) {
    double alpha, t;
    struct pj_opaque *Q = pj_calloc (1, sizeof (struct pj_opaque));
    if (0==Q)
        return pj_default_destructor(P, ENOMEM);
    P->opaque = Q;
    
    if (pj_param(P->ctx, P->params, "tn").i) {
        Q->n = pj_param(P->ctx, P->params, "dn").f;
        if (Q->n <= 0. || Q->n > 1.)
            return pj_default_destructor(P, PJD_ERR_N_OUT_OF_RANGE);
    } else {
            return pj_default_destructor(P, PJD_ERR_N_OUT_OF_RANGE);
    }
    Q->q3 = pj_param(P->ctx, P->params, "dq").f / 3.;
    alpha = pj_param(P->ctx, P->params, "ralpha").f;
    t = Q->n * sin (alpha);
    Q->m = cos (alpha) / sqrt (1. - t * t);
    Q->rmn = 1. / (Q->m * Q->n);

    P->es = 0.;
    P->inv = 0;
    P->fwd = s_forward;

    return P;
}


#ifndef PJ_SELFTEST
int pj_urm5_selftest (void) {return 0;}
#else
int pj_urm5_selftest (void) {
    double tolerance_lp = 1e-10;
    double tolerance_xy = 1e-7;

    char s_args[] = {"+proj=urm5   +a=6400000    +lat_1=0.5 +lat_2=2 +n=0.5"};

    LP fwd_in[] = {
        { 2, 1},
        { 2,-1},
        {-2, 1},
        {-2,-1}
    };

    XY s_fwd_expect[] = {
        { 223393.6384339639,  111696.81878511712},
        { 223393.6384339639, -111696.81878511712},
        {-223393.6384339639,  111696.81878511712},
        {-223393.6384339639, -111696.81878511712},
    };

    return pj_generic_selftest (0, s_args, tolerance_xy, tolerance_lp, 4, 4, fwd_in, 0, s_fwd_expect, 0, 0, 0);
}
#endif
