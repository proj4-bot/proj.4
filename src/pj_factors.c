/* projection scale factors */
#define PJ_LIB__
#include <proj.h>
#include "projects.h"

#include <errno.h>

#ifndef DEFAULT_H
#define DEFAULT_H   1e-5    /* radian default for numeric h */
#endif

#define EPS 1.0e-12

int pj_factors(LP lp, PJ *P, double h, struct FACTORS *fac) {
    double cosphi, t, n, r;
    int err;

    err = proj_errno_reset (P);

    /* Check for latitude or longitude overange */
    if ((fabs (lp.phi)-M_HALFPI) > EPS || fabs (lp.lam) > 10.) {
        proj_errno_set (P, PJD_ERR_LAT_OR_LON_EXCEED_LIMIT);
        return 1;
    }

    /* Set a reasonable step size for the numerical derivatives */
    h = fabs (h);
    if (h < EPS)
        h = DEFAULT_H;

    /* If latitude + one step overshoots the pole, move it slightly inside, */
    /* so the numerical derivative still exists */
    if (fabs (lp.phi) > (M_HALFPI - h))
        lp.phi = lp.phi < 0. ? (-M_HALFPI+h) : (M_HALFPI-h);
    else if (P->geoc)
        lp.phi = atan(P->rone_es * tan(lp.phi));

    /* Longitudinal distance from central meridian */
    lp.lam -= P->lam0;
    if (!P->over)
        lp.lam = adjlon(lp.lam);

    /* Derivatives */
    if (pj_deriv (lp, h, P, &(fac->der))) {
        proj_errno_set (P, PJD_ERR_LAT_OR_LON_EXCEED_LIMIT);
        return 1;
    }

    /* Scale factors */
    cosphi = cos (lp.phi);
    fac->h = hypot (fac->der.x_p, fac->der.y_p);
    fac->k = hypot (fac->der.x_l, fac->der.y_l) / cosphi;

    if (P->es != 0.0) {
        t = sin(lp.phi);
        t = 1. - P->es * t * t;
        n = sqrt(t);
        fac->h *= t * n / P->one_es;
        fac->k *= n;
        r = t * t / P->one_es;
    } else
        r = 1.;

    /* Convergence */
    fac->conv = -atan2 (fac->der.x_p, fac->der.y_p);

    /* Areal scale factor */
    fac->s = (fac->der.y_p * fac->der.x_l - fac->der.x_p * fac->der.y_l) * r / cosphi;

    /* Meridian-parallel angle (theta prime) */
    fac->thetap = aasin(P->ctx,fac->s / (fac->h * fac->k));

    /* Tissot ellipse axis */
    t = fac->k * fac->k + fac->h * fac->h;
    fac->a = sqrt(t + 2. * fac->s);
    t = t - 2. * fac->s;
    t = t > 0? sqrt(t): 0;
    fac->b = 0.5 * (fac->a - t);
    fac->a = 0.5 * (fac->a + t);

    /* Angular distortion */
    fac->omega = 2. * aasin(P->ctx, (fac->a - fac->b) / (fac->a + fac->b) );

    proj_errno_restore (P, err);
    return 0;
}
