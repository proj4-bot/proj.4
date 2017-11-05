#define PJ_LIB__
#include <proj.h>
#include "projects.h"

PROJ_HEAD(calcofi,
    "Cal Coop Ocean Fish Invest Lines/Stations") "\n\tCyl, Sph&Ell";

#include <string.h>
#include <stdio.h>
#include <math.h>
#include <proj_api.h>
#include <errno.h>

/* Conversions for the California Cooperative Oceanic Fisheries Investigations
Line/Station coordinate system following the algorithm of:
Eber, L.E., and  R.P. Hewitt. 1979. Conversion algorithms for the CALCOFI
station grid. California Cooperative Oceanic Fisheries Investigations Reports
20:135-137. (corrected for typographical errors).
http://www.calcofi.org/publications/calcofireports/v20/Vol_20_Eber___Hewitt.pdf
They assume 1 unit of CalCOFI Line == 1/5 degree in longitude or
meridional units at reference point O, and similarly 1 unit of CalCOFI
Station == 1/15 of a degree at O.
By convention, CalCOFI Line/Station conversions use Clarke 1866 but we use
whatever ellipsoid is provided. */


#define EPS10 1.e-10
#define DEG_TO_LINE 5
#define DEG_TO_STATION 15
#define LINE_TO_RAD 0.0034906585039886592
#define STATION_TO_RAD 0.0011635528346628863
#define PT_O_LINE 80 /* reference point O is at line 80,  */
#define PT_O_STATION 60 /* station 60,  */
#define PT_O_LAMBDA -2.1144663887911301 /* lon -121.15 and */
#define PT_O_PHI 0.59602993955606354 /* lat 34.15 */
#define ROTATION_ANGLE 0.52359877559829882 /*CalCOFI angle of 30 deg in rad */


static XY e_forward (LP lp, PJ *P) {          /* Ellipsoidal, forward */
    XY xy = {0.0,0.0};
    double oy; /* pt O y value in Mercator */
    double l1; /* l1 and l2 are distances calculated using trig that sum
               to the east/west distance between point O and point xy */
    double l2;
    double ry; /* r is the point on the same station as o (60) and the same
               line as xy xy, r, o form a right triangle */
    /* if the user has specified +lon_0 or +k0 for some reason,
    we're going to ignore it so that xy is consistent with point O */
    lp.lam = lp.lam + P->lam0;
    if (fabs(fabs(lp.phi) - M_HALFPI) <= EPS10) {
        proj_errno_set(P, PJD_ERR_TOLERANCE_CONDITION);
        return xy;
    }
    xy.x = lp.lam;
    xy.y = -log(pj_tsfn(lp.phi, sin(lp.phi), P->e)); /* Mercator transform xy*/
    oy = -log(pj_tsfn(PT_O_PHI, sin(PT_O_PHI), P->e));
    l1 = (xy.y - oy) * tan(ROTATION_ANGLE);
    l2 = -xy.x - l1 + PT_O_LAMBDA;
    ry = l2 * cos(ROTATION_ANGLE) * sin(ROTATION_ANGLE) + xy.y;
    ry = pj_phi2(P, exp(-ry), 1e-10); /* inverse Mercator */
    xy.x = PT_O_LINE - RAD_TO_DEG *
        (ry - PT_O_PHI) * DEG_TO_LINE / cos(ROTATION_ANGLE);
    xy.y = PT_O_STATION + RAD_TO_DEG *
        (ry - lp.phi) * DEG_TO_STATION / sin(ROTATION_ANGLE);
    /* set a = 1, x0 = 0, and y0 = 0 so that no further unit adjustments
    are done */
    P->a = 1;
    P->x0 = 0;
    P->y0 = 0;
    return xy;
}


static XY s_forward (LP lp, PJ *P) {           /* Spheroidal, forward */
    XY xy = {0.0,0.0};
    double oy;
    double l1;
    double l2;
    double ry;
    lp.lam = lp.lam + P->lam0;
    if (fabs(fabs(lp.phi) - M_HALFPI) <= EPS10) {
        proj_errno_set(P, PJD_ERR_TOLERANCE_CONDITION);
        return xy;
    }
    xy.x = lp.lam;
    xy.y = log(tan(M_FORTPI + .5 * lp.phi));
    oy = log(tan(M_FORTPI + .5 * PT_O_PHI));
    l1 = (xy.y - oy) * tan(ROTATION_ANGLE);
    l2 = -xy.x - l1 + PT_O_LAMBDA;
    ry = l2 * cos(ROTATION_ANGLE) * sin(ROTATION_ANGLE) + xy.y;
    ry = M_HALFPI - 2. * atan(exp(-ry));
    xy.x = PT_O_LINE - RAD_TO_DEG *
        (ry - PT_O_PHI) * DEG_TO_LINE / cos(ROTATION_ANGLE);
    xy.y = PT_O_STATION + RAD_TO_DEG *
        (ry - lp.phi) * DEG_TO_STATION / sin(ROTATION_ANGLE);
    P->a = 1;
    P->x0 = 0;
    P->y0 = 0;
    return xy;
}


static LP e_inverse (XY xy, PJ *P) {          /* Ellipsoidal, inverse */
    LP lp = {0.0,0.0};
    double ry; /* y value of point r */
    double oymctr; /* Mercator-transformed y value of point O */
    double rymctr; /* Mercator-transformed ry */
    double xymctr; /* Mercator-transformed xy.y */
    double l1;
    double l2;
    /* turn x and y back into Line/Station */
    xy.x /= P->ra;
    xy.y /= P->ra;
    ry = PT_O_PHI - LINE_TO_RAD * (xy.x - PT_O_LINE) *
        cos(ROTATION_ANGLE);
    lp.phi = ry - STATION_TO_RAD * (xy.y - PT_O_STATION) * sin(ROTATION_ANGLE);
    oymctr = -log(pj_tsfn(PT_O_PHI, sin(PT_O_PHI), P->e));
    rymctr = -log(pj_tsfn(ry, sin(ry), P->e));
    xymctr = -log(pj_tsfn(lp.phi, sin(lp.phi), P->e));
    l1 = (xymctr - oymctr) * tan(ROTATION_ANGLE);
    l2 = (rymctr - xymctr) / (cos(ROTATION_ANGLE) * sin(ROTATION_ANGLE));
    lp.lam = PT_O_LAMBDA - (l1 + l2);
    P->over = 1;
    return lp;
}


static LP s_inverse (XY xy, PJ *P) {           /* Spheroidal, inverse */
    LP lp = {0.0,0.0};
    double ry;
    double oymctr;
    double rymctr;
    double xymctr;
    double l1;
    double l2;
    xy.x /= P->ra;
    xy.y /= P->ra;
    ry = PT_O_PHI - LINE_TO_RAD * (xy.x - PT_O_LINE) *
        cos(ROTATION_ANGLE);
    lp.phi = ry - STATION_TO_RAD * (xy.y - PT_O_STATION) * sin(ROTATION_ANGLE);
    oymctr = log(tan(M_FORTPI + .5 * PT_O_PHI));
    rymctr = log(tan(M_FORTPI + .5 * ry));
    xymctr = log(tan(M_FORTPI + .5 * lp.phi));
    l1 = (xymctr - oymctr) * tan(ROTATION_ANGLE);
    l2 = (rymctr - xymctr) / (cos(ROTATION_ANGLE) * sin(ROTATION_ANGLE));
    lp.lam = PT_O_LAMBDA - (l1 + l2);
    P->over = 1;
    return lp;
}


PJ *PROJECTION(calcofi) {
    P->opaque = 0;

    if (P->es != 0.0) { /* ellipsoid */
        P->inv = e_inverse;
        P->fwd = e_forward;
    } else { /* sphere */
        P->inv = s_inverse;
        P->fwd = s_forward;
    }
    return P;
}


int pj_calcofi_selftest (void) {return 10000;}
