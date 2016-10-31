/******************************************************************************
 * Project:  PROJ.4
 * Purpose:  Implement some service routines for the PJ_OBSERVATION generic
 *           geodetic data type
 * Author:   Thomas Knudsen,  thokn@sdfe.dk,  2016-06-09/2016-10-11
 *
 ******************************************************************************
 * Copyright (c) 2016, Thomas Knudsen/SDFE
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *****************************************************************************/
#define PJ_OBSERVATION_C
#include <proj.h>
#include <projects.h>
#include <float.h>
#include <math.h>

/* Need some prototypes from proj_api.h */
PJ_CONTEXT *pj_get_default_ctx(void);
PJ_CONTEXT *pj_get_ctx(PJ *);

XY pj_fwd(LP, PJ *);
LP pj_inv(XY, PJ *);

XYZ pj_fwd3d(LPZ, PJ *);
LPZ pj_inv3d(XYZ, PJ *);
PJ *pj_init_ctx(PJ_CONTEXT *, int, char ** );
PJ *pj_init_plus_ctx(PJ_CONTEXT *, const char * );


const PJ_OBSERVATION pj_observation_error = {
    /* Cannot use HUGE_VAL here: MSVC misimplements HUGE_VAL as something that is not compile time constant */
    {{DBL_MAX,DBL_MAX,DBL_MAX,DBL_MAX}},
    {{DBL_MAX,DBL_MAX,DBL_MAX}},
    0, 0
};

const PJ_OBSERVATION pj_observation_null = {
    {{0, 0, 0, 0}},
    {{0, 0, 0}},
    0, 0
};

PJ_OBSERVATION pj_fwdobs (PJ_OBSERVATION obs, PJ *P) {
    if (0!=P->fwd3d) {
        obs.coo.xyz  =  pj_fwd3d (obs.coo.lpz, P);
        return obs;
    }
    if (0!=P->fwd) {
        obs.coo.xy  =  pj_fwd (obs.coo.lp, P);
        return obs;
    }
    pj_ctx_set_errno (P->ctx, EINVAL);
    return pj_observation_error;
}

PJ_OBSERVATION pj_invobs (PJ_OBSERVATION obs, PJ *P) {
    if (0!=P->inv3d) {
        obs.coo.lpz  =  pj_inv3d (obs.coo.xyz, P);
        return obs;
    }
    if (0!=P->inv) {
        obs.coo.lp  =  pj_inv (obs.coo.xy, P);
        return obs;
    }
    pj_ctx_set_errno (P->ctx, EINVAL);
    return pj_observation_error;
}




PJ_OBSERVATION pj_apply (PJ *P, enum pj_direction direction, PJ_OBSERVATION obs) {

    if (0==P)
        return pj_observation_error;

    switch (direction) {
        case PJ_FWD:
            return pj_fwdobs (obs, P);
        case PJ_INV:
            return  pj_invobs (obs, P);
        case 0:
            return obs;
        default:
            break;
    }

    pj_ctx_set_errno (P->ctx, EINVAL);
    return pj_observation_error;
}


/* initial attempt, following a suggestion by Kristian Evers */
double pj_roundtrip(PJ *P, enum pj_direction direction, int n, PJ_OBSERVATION obs) {
    double  d;
    PJ_OBSERVATION o;

    /* multiple roundtrips not supported yet */
    if (n > 1) {
        pj_ctx_set_errno (P->ctx, EINVAL);
        return HUGE_VAL;
    }

    switch (direction) {
        case 1:
            o.coo.xyz  =  pj_fwd3d (obs.coo.lpz, P);
            break;
        case -1:
            o.coo.lpz  =  pj_inv3d (obs.coo.xyz, P);
            break;
        default:
            pj_ctx_set_errno (P->ctx, EINVAL);
            return HUGE_VAL;
    }

    o.coo.xyz = pj_fwd3d (obs.coo.lpz, P);
    o.coo.lpz = pj_inv3d (o.coo.xyz, P);

    /* Should add "both ways" here */
    d = hypot (hypot (o.coo.v[0] - obs.coo.v[0], o.coo.v[1] - obs.coo.v[1]), o.coo.v[2] - obs.coo.v[2]);
    return d;
}

PJ *pj_create (const char *definition) {
    return pj_init_plus (definition);
}

PJ *pj_create_argv (int argc, char **argv) {
    return pj_init (argc, argv);
}


int pj_error (PJ *P) {
    return pj_ctx_get_errno (pj_get_ctx(P));
}

int pj_context_create (PJ *P) {
    PJ_CONTEXT *ctx = pj_ctx_alloc ();
    if (0==ctx) {
        pj_ctx_set_errno (pj_get_ctx(P), ENOMEM);
        return 1;
    }
    pj_set_ctx (P, ctx);
    return 0;
}

void pj_context_modify (PJ *P, int err, int dbg, void (*log)(void *, int, const char *), void *app, void *api) {
    PJ_CONTEXT *ctx = pj_get_ctx(P);
    ctx->last_errno  = err;
    ctx->debug_level = dbg;
    if (0!=log)
        ctx->logger = log;
    if (0!=app)
        ctx->app_data = app;
    if (0!=api)
        ctx->fileapi = api;
}

void pj_context_free (PJ *P) {
    pj_ctx_free (pj_get_ctx (P));
    pj_set_ctx (P, pj_get_default_ctx ());
}
