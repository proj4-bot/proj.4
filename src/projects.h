/******************************************************************************
 * Project:  PROJ.4
 * Purpose:  Primary (private) include file for PROJ.4 library.
 * Author:   Gerald Evenden
 *
 ******************************************************************************
 * Copyright (c) 2000, Frank Warmerdam
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

/* General projections header file */
#ifndef PROJECTS_H
#define PROJECTS_H

#ifdef _MSC_VER
#  ifndef _CRT_SECURE_NO_DEPRECATE
#    define _CRT_SECURE_NO_DEPRECATE
#  endif
#  ifndef _CRT_NONSTDC_NO_DEPRECATE
#    define _CRT_NONSTDC_NO_DEPRECATE
#  endif
#endif

/* standard inclusions */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
#define C_NAMESPACE extern "C"
#define C_NAMESPACE_VAR extern "C"
extern "C" {
#else
#define C_NAMESPACE extern
#define C_NAMESPACE_VAR
#endif

#ifndef NULL
#  define NULL  0
#endif

#ifndef FALSE
#  define FALSE 0
#endif

#ifndef TRUE
#  define TRUE  1
#endif

#ifndef MAX
#  define MIN(a,b)      ((a<b) ? a : b)
#  define MAX(a,b)      ((a>b) ? a : b)
#endif

#ifndef ABS
#  define ABS(x)        ((x<0) ? (-1*(x)) : x)
#endif

/* maximum path/filename */
#ifndef MAX_PATH_FILENAME
#define MAX_PATH_FILENAME 1024
#endif

/* prototype hypot for systems where absent */
#ifndef _WIN32
extern double hypot(double, double);
#endif

#ifdef _WIN32_WCE
#  include <wce_stdlib.h>
#  include <wce_stdio.h>
#  define rewind wceex_rewind
#  define getenv wceex_getenv
#  define strdup _strdup
#  define hypot _hypot
#endif

/* enable predefined math constants M_* for MS Visual Studio workaround */
#ifdef _MSC_VER
#define _USE_MATH_DEFINES
#endif

/* some more useful math constants and aliases */
#define M_FORTPI         M_PI_4                   /* pi/4 */
#define M_HALFPI         M_PI_2                   /* pi/2 */
/* M_PI                                               pi */
#define M_PI_HALFPI      4.71238898038468985769   /* 1.5*pi */
#define M_TWOPI          6.28318530717958647693   /* 2*pi */
#define M_TWO_D_PI       M_2_PI                   /* 2/pi */
#define M_TWOPI_HALFPI   7.85398163397448309616   /* 2.5*pi */
/* M_SQRT2                                           sqrt(2) */


/* maximum tag id length for +init and default files */
#ifndef ID_TAG_MAX
#define ID_TAG_MAX 50
#endif

/* Use WIN32 as a standard windows 32 bit declaration */
#if defined(_WIN32) && !defined(WIN32) && !defined(_WIN32_WCE)
#  define WIN32
#endif

#if defined(_WINDOWS) && !defined(WIN32) && !defined(_WIN32_WCE)
#  define WIN32
#endif

/* directory delimiter for DOS support */
#ifdef WIN32
#define DIR_CHAR '\\'
#else
#define DIR_CHAR '/'
#endif

struct projFileAPI_t;

/* proj thread context */
typedef struct {
    int     last_errno;
    int     debug_level;
    void    (*logger)(void *, int, const char *);
    void    *app_data;
    struct projFileAPI_t *fileapi;
} projCtx_t;

/* datum_type values */
#define PJD_UNKNOWN   0
#define PJD_3PARAM    1
#define PJD_7PARAM    2
#define PJD_GRIDSHIFT 3
#define PJD_WGS84     4   /* WGS84 (or anything considered equivalent) */


/* library errors */
#define PJD_ERR_GEOCENTRIC          -45
#define PJD_ERR_AXIS                -47
#define PJD_ERR_GRID_AREA           -48
#define PJD_ERR_CATALOG             -49

#define USE_PROJUV

typedef struct { double u, v; } projUV;
typedef struct { double r, i; } COMPLEX;
typedef struct { double u, v, w; } projUVW;

#ifndef PJ_LIB__
#define XY projUV
#define LP projUV
#define XYZ projUVW
#define LPZ projUVW
#else
typedef struct { double x, y; }     XY;
typedef struct { double lam, phi; } LP;
typedef struct { double x, y, z; } XYZ;
typedef struct { double lam, phi, z; } LPZ;
#endif


/* Forward declarations and typedefs for stuff needed inside the PJ object */
struct PJconsts;
struct pj_opaque;
struct ARG_list;
struct FACTORS;
struct PJ_REGION_S;
typedef struct PJ_REGION_S  PJ_Region;
typedef struct ARG_list paralist;   /* parameter list */
typedef struct PJconsts PJ;         /* the PJ object herself */

struct PJ_REGION_S {
    double ll_long;        /* lower left corner coordinates (radians) */
    double ll_lat;
    double ur_long;        /* upper right corner coordinates (radians) */
    double ur_lat;
};


/* base projection data structure */
struct PJconsts {

    /*************************************************************************************

                            G E N E R A L   C O N T E X T

    **************************************************************************************
	
	    Need some description here - especially about the thread context...
	
    **************************************************************************************/

    projCtx_t *ctx;
    const char *descr;             /* From pj_list.h or individual PJ_*.c file */
    paralist *params;              /* Parameter list */
    struct pj_opaque *opaque;      /* Projection specific parameters, Defined in PJ_*.c */


    /*************************************************************************************

                          F U N C T I O N    P O I N T E R S

    **************************************************************************************
	
	    For projection xxx, these are pointers to functions in the corresponding
		PJ_xxx.c file.

        pj_init() delegates the setup of these to pj_projection_specific_setup_xxx(),
		a name which is currently hidden behind the magic curtain of the PROJECTION
		macro.
		
		As the PROJ.4 de-macroization project expands its coverage, this will change,
		and the setup functions for each projection will become more clearly visible
		in the source code.
	
    **************************************************************************************/

    XY  (*fwd)(LP,    PJ *);
    LP  (*inv)(XY,    PJ *);
    XYZ (*fwd3d)(LPZ, PJ *);
    LPZ (*inv3d)(XYZ, PJ *);

    void (*spc)(LP, PJ *, struct FACTORS *);

    void (*pfree)(PJ *);



    /*************************************************************************************

                          E L L I P S O I D     P A R A M E T E R S

    **************************************************************************************
    
        Despite YAGNI, we add a large number of ellipsoidal shape parameters, which
        are not yet set up in pj_init. They are, however, inexpensive to compute,
        compared to the overall time taken for setting up the complex PJ object
        (cf. e.g. https://en.wikipedia.org/wiki/Angular_eccentricity).
        
        But during single point projections it will often be a useful thing to have
        these readily available without having to recompute at every pj_fwd / pj_inv
        call.
        
        With this wide selection, we should be ready for quite a number of geodetic
        algorithms, without having to incur further ABI breakage.
                          
    **************************************************************************************/
    
    /* The linear parameters */
    
    double  a;                         /* semimajor axis (radius if eccentricity==0) */
    double  b;                         /* semiminor axis */
    double  ra;                        /* 1/a */
    double  rb;                        /* 1/b */

    /* The eccentricities */
    
    double  e;                         /* first  eccentricity */
    double  es;                        /* first  eccentricity squared */
    double  e2;                        /* second eccentricity */
    double  e2s;                       /* second eccentricity squared */
    double  e3;                        /* third  eccentricity */
    double  e3s;                       /* third  eccentricity squared */
    double  one_es;                    /* 1 - e^2 */
    double  rone_es;                   /* 1/one_es */

    /* The flattenings */
    
    double  f;                         /* first  flattening */
    double  f2;                        /* second flattening */
    double  n;                         /* third  flattening */
    double  rf;                        /* 1/f  */
    double  rf2;                       /* 1/f2 */
    double  rn;                        /* 1/n  */
    
    /* This one's for GRS80 */
    double  J;                         /* "Dynamic form factor" */

    double  es_orig, a_orig;           /* es and a before any +proj related adjustment */

    

    /*************************************************************************************

                          C O O R D I N A T E   H A N D L I N G

    **************************************************************************************/

    int     over;                      /* Over-range flag */
    int     geoc;                      /* Geocentric latitude flag */
    int     is_latlong;                /* proj=latlong ... not really a projection at all */
    int     is_geocent;                /* proj=geocent ... not really a projection at all */
    int     left, right;               /* Flags for input/output coordinate types */


    /*************************************************************************************

                       C A R T O G R A P H I C       O F F S E T S

    **************************************************************************************/

    double  lam0, phi0;                /* central longitude, latitude */
    double  x0, y0;                    /* false easting and northing */



    /*************************************************************************************

                                    S C A L I N G

    **************************************************************************************/

    double  k0;                        /* General scaling factor - e.g. the 0.9996 of UTM */
    double  to_meter, fr_meter;        /* Plane coordinate scaling. Internal unit [m] */
    double  vto_meter, vfr_meter;      /* Vertical scaling. Internal unit [m] */



    /*************************************************************************************

                  D A T U M S   A N D   H E I G H T   S Y S T E M S

    **************************************************************************************/

    int     datum_type;                /* PJD_UNKNOWN/3PARAM/7PARAM/GRIDSHIFT/WGS84 */
    double  datum_params[7];           /* Parameters for 3PARAM and 7PARAM */
    struct _pj_gi **gridlist;          /* Description needed */
    int     gridlist_count;

    int     has_geoid_vgrids;          /* Description needed */
    struct _pj_gi **vgridlist_geoid;   /* Description needed */
    int     vgridlist_geoid_count;

    double  from_greenwich;            /* prime meridian offset (in radians) */
    double  long_wrap_center;          /* 0.0 for -180 to 180, actually in radians*/
    int     is_long_wrap_set;
    char    axis[4];                   /* Description needed */


    /* New Datum Shift Grid Catalogs */
    char   *catalog_name;
    struct _PJ_GridCatalog *catalog;

    double  datum_date;                 /* Description needed */

    struct _pj_gi *last_before_grid;    /* Description needed */
    PJ_Region     last_before_region;   /* Description needed */
    double        last_before_date;     /* Description needed */

    struct _pj_gi *last_after_grid;     /* Description needed */
    PJ_Region     last_after_region;    /* Description needed */
    double        last_after_date;      /* Description needed */

};






/* Parameter list (a copy of the +proj=... etc. parameters) */
struct ARG_list {
    paralist *next;
    char used;
    char param[1];    /* This probably should be [0] to be fully standards compliant? */
};



typedef union { double  f; int  i; char *s; } PROJVALUE;


struct PJ_SELFTEST_LIST {
    char    *id;                 /* projection keyword */
    int     (* testfunc)(void);  /* projection entry point */
};

struct PJ_ELLPS {
    char    *id;           /* ellipse keyword name */
    char    *major;        /* a= value */
    char    *ell;          /* elliptical parameter */
    char    *name;         /* comments */
};
struct PJ_UNITS {
    char    *id;           /* units keyword */
    char    *to_meter;     /* multiply by value to get meters */
    char    *name;         /* comments */
};

struct PJ_DATUMS {
    char    *id;           /* datum keyword */
    char    *defn;         /* ie. "to_wgs84=..." */
    char    *ellipse_id;   /* ie from ellipse table */
    char    *comments;     /* EPSG code, etc */
};

struct PJ_PRIME_MERIDIANS {
    char    *id;           /* prime meridian keyword */
    char    *defn;         /* offset from greenwich in DMS format. */
};


struct DERIVS {
    double x_l, x_p;       /* derivatives of x for lambda-phi */
    double y_l, y_p;       /* derivatives of y for lambda-phi */
};

struct FACTORS {
    struct DERIVS der;
    double h, k;           /* meridional, parallel scales */
    double omega, thetap;  /* angular distortion, theta prime */
    double conv;           /* convergence */
    double s;              /* areal scale factor */
    double a, b;           /* max-min scale error */
    int code;              /* info as to analytics, see following */
};


#define IS_ANAL_XL_YL 01    /* derivatives of lon analytic */
#define IS_ANAL_XP_YP 02    /* derivatives of lat analytic */
#define IS_ANAL_HK  04      /* h and k analytic */
#define IS_ANAL_CONV 010    /* convergence analytic */





/* public API */
#include "proj_api.h"











/* Generate pj_list external or make list from include file */

struct PJ_LIST {
    char    *id;                 /* projection keyword */
    PJ *(*proj)(PJ *);           /* projection entry point */
    char    * const *descr;      /* description text */
};


#ifndef USE_PJ_LIST_H
extern struct PJ_LIST pj_list[];
extern struct PJ_SELFTEST_LIST pj_selftest_list[];
#endif



#ifndef PJ_ELLPS__
extern struct PJ_ELLPS pj_ellps[];
#endif

#ifndef PJ_UNITS__
extern struct PJ_UNITS pj_units[];
#endif

#ifndef PJ_DATUMS__
extern struct PJ_DATUMS pj_datums[];
extern struct PJ_PRIME_MERIDIANS pj_prime_meridians[];
#endif



#ifdef PJ_LIB__
/* repetitive projection code (most of them eliminated now) */
/* Will follow up with another project eliminating the last ones */

#define PROJ_HEAD(id, name) static const char des_##id [] = name

#define E_ERROR(err) { pj_ctx_set_errno( P->ctx, err); freeup(P); return(0); }
#define E_ERROR_0 { freeup(P); return(0); }
#define F_ERROR { pj_ctx_set_errno( P->ctx, -20); return(xy); }
#define F3_ERROR { pj_ctx_set_errno( P->ctx, -20); return(xyz); }
#define I_ERROR { pj_ctx_set_errno( P->ctx, -20); return(lp); }
#define I3_ERROR { pj_ctx_set_errno( P->ctx, -20); return(lpz); }

/* cleaned up alternative to most of the "repetitive projection code" macros */
#define PROJECTION(name)                                     \
pj_projection_specific_setup_##name (PJ *P);                 \
C_NAMESPACE_VAR const char * const pj_s_##name = des_##name; \
C_NAMESPACE PJ *pj_##name (PJ *P) {                          \
    if (P)                                                   \
        return pj_projection_specific_setup_##name (P);      \
    P = (PJ*) pj_calloc (1, sizeof(PJ));                     \
    if (0==P)                                                \
        return 0;                                            \
    P->pfree = freeup;                                       \
    P->descr = des_##name;                                   \
    return P;                                                \
}                                                            \
PJ *pj_projection_specific_setup_##name (PJ *P)

#endif




int pj_generic_selftest (
    char *e_args,
    char *s_args,
    double tolerance_xy,
    double tolerance_lp,
    int n_fwd,
    int n_inv,
    LP *fwd_in,
    XY *e_fwd_expect,
    XY *s_fwd_expect,
    XY *inv_in,
    LP *e_inv_expect,
    LP *s_inv_expect
);




#define MAX_TAB_ID 80
typedef struct { float lam, phi; } FLP;
typedef struct { int lam, phi; } ILP;

struct CTABLE {
    char id[MAX_TAB_ID]; /* ascii info */
    LP ll;               /* lower left corner coordinates */
    LP del;              /* size of cells */
    ILP lim;             /* limits of conversion matrix */
    FLP *cvs;            /* conversion matrix */
};

typedef struct _pj_gi {
    char *gridname;      /* identifying name of grid, eg "conus" or ntv2_0.gsb */
    char *filename;      /* full path to filename */

    const char *format;  /* format of this grid, (ctable/ntv1/ntv2/missing). */

    int   grid_offset;   /* offset in file, for delayed loading */
    int   must_swap;     /* only for NTv2 */

    struct CTABLE *ct;

    struct _pj_gi *next;
    struct _pj_gi *child;
} PJ_GRIDINFO;

typedef struct {
    PJ_Region region;
    int  priority;           /* higher used before lower */
    double date;             /* year.fraction */
    char *definition;        /* usually the gridname */

    PJ_GRIDINFO  *gridinfo;
    int available;           /* 0=unknown, 1=true, -1=false */
} PJ_GridCatalogEntry;

typedef struct _PJ_GridCatalog {
    char *catalog_name;

    PJ_Region region; /* maximum extent of catalog data */

    int entry_count;
    PJ_GridCatalogEntry *entries;

    struct _PJ_GridCatalog *next;
} PJ_GridCatalog;


/* procedure prototypes */
double dmstor(const char *, char **);
double dmstor_ctx(projCtx ctx, const char *, char **);
void set_rtodms(int, int);
char *rtodms(char *, double, int, int);
double adjlon(double);
double aacos(projCtx,double), aasin(projCtx,double), asqrt(double), aatan2(double, double);
PROJVALUE pj_param(projCtx ctx, paralist *, const char *);
paralist *pj_mkparam(char *);
int pj_ell_set(projCtx ctx, paralist *, double *, double *);
int pj_datum_set(projCtx,paralist *, PJ *);
int pj_prime_meridian_set(paralist *, PJ *);
int pj_angular_units_set(paralist *, PJ *);
void pj_prepare (PJ *P, const char *description, void (*freeup)(struct PJconsts *), size_t sizeof_struct_opaque);

paralist *pj_clone_paralist( const paralist* );
paralist*pj_search_initcache( const char *filekey );
void pj_insert_initcache( const char *filekey, const paralist *list);

double *pj_enfn(double);
double pj_mlfn(double, double, double, double *);
double pj_inv_mlfn(projCtx, double, double, double *);
double pj_qsfn(double, double, double);
double pj_tsfn(double, double, double);
double pj_msfn(double, double, double);
double pj_phi2(projCtx, double, double);
double pj_qsfn_(double, PJ *);
double *pj_authset(double);
double pj_authlat(double, double *);
COMPLEX pj_zpoly1(COMPLEX, COMPLEX *, int);
COMPLEX pj_zpolyd1(COMPLEX, COMPLEX *, int, COMPLEX *);

int pj_deriv(LP, double, PJ *, struct DERIVS *);
int pj_factors(LP, PJ *, double, struct FACTORS *);

struct PW_COEF {/* row coefficient structure */
    int m;      /* number of c coefficients (=0 for none) */
    double *c;  /* power coefficients */
};

/* Approximation structures and procedures */

typedef struct {    /* Chebyshev or Power series structure */
    projUV a, b;        /* power series range for evaluation */
                    /* or Chebyshev argument shift/scaling */
    struct PW_COEF *cu, *cv;
    int mu, mv;     /* maximum cu and cv index (+1 for count) */
    int power;      /* != 0 if power series, else Chebyshev */
} Tseries;

Tseries *mk_cheby(projUV, projUV, double, projUV *, projUV (*)(projUV), int, int, int);
projUV bpseval(projUV, Tseries *);
projUV bcheval(projUV, Tseries *);
projUV biveval(projUV, Tseries *);
void *vector1(int, int);
void **vector2(int, int, int);
void freev2(void **v, int nrows);
int bchgen(projUV, projUV, int, int, projUV **, projUV(*)(projUV));
int bch2bps(projUV, projUV, projUV **, int, int);



/* nadcon related protos */
LP nad_intr(LP, struct CTABLE *);
LP nad_cvt(LP, int, struct CTABLE *);
struct CTABLE *nad_init(projCtx ctx, char *);
struct CTABLE *nad_ctable_init( projCtx ctx, PAFile fid );
int nad_ctable_load( projCtx ctx, struct CTABLE *, PAFile fid );
struct CTABLE *nad_ctable2_init( projCtx ctx, PAFile fid );
int nad_ctable2_load( projCtx ctx, struct CTABLE *, PAFile fid );
void nad_free(struct CTABLE *);



/* higher level handling of datum grid shift files */

int pj_apply_vgridshift( PJ *defn, const char *listname,
                         PJ_GRIDINFO ***gridlist_p,
                         int *gridlist_count_p,
                         int inverse,
                         long point_count, int point_offset,
                         double *x, double *y, double *z );
int pj_apply_gridshift_2( PJ *defn, int inverse,
                          long point_count, int point_offset,
                          double *x, double *y, double *z );
int pj_apply_gridshift_3( projCtx ctx,
                          PJ_GRIDINFO **gridlist, int gridlist_count,
                          int inverse, long point_count, int point_offset,
                          double *x, double *y, double *z );

PJ_GRIDINFO **pj_gridlist_from_nadgrids( projCtx, const char *, int * );
void pj_deallocate_grids();

PJ_GRIDINFO *pj_gridinfo_init( projCtx, const char * );
int pj_gridinfo_load( projCtx, PJ_GRIDINFO * );
void pj_gridinfo_free( projCtx, PJ_GRIDINFO * );

PJ_GridCatalog *pj_gc_findcatalog( projCtx, const char * );
PJ_GridCatalog *pj_gc_readcatalog( projCtx, const char * );
void pj_gc_unloadall( projCtx );
int pj_gc_apply_gridshift( PJ *defn, int inverse,
                           long point_count, int point_offset,
                           double *x, double *y, double *z );
int pj_gc_apply_gridshift( PJ *defn, int inverse,
                           long point_count, int point_offset,
                           double *x, double *y, double *z );

PJ_GRIDINFO *pj_gc_findgrid( projCtx ctx,
                             PJ_GridCatalog *catalog, int after,
                             LP location, double date,
                             PJ_Region *optional_region,
                             double *grid_date );

double pj_gc_parsedate( projCtx, const char * );

void *proj_mdist_ini(double);
double proj_mdist(double, double, double, const void *);
double proj_inv_mdist(projCtx ctx, double, const void *);
void *pj_gauss_ini(double, double, double *,double *);
LP pj_gauss(projCtx, LP, const void *);
LP pj_inv_gauss(projCtx, LP, const void *);

extern char const pj_release[];

struct PJ_ELLPS           *pj_get_ellps_ref( void );
struct PJ_DATUMS          *pj_get_datums_ref( void );
struct PJ_UNITS           *pj_get_units_ref( void );
struct PJ_LIST            *pj_get_list_ref( void );
struct PJ_SELFTEST_LIST   *pj_get_selftest_list_ref ( void );
struct PJ_PRIME_MERIDIANS *pj_get_prime_meridians_ref( void );

double pj_atof( const char* nptr );
double pj_strtod( const char *nptr, char **endptr );











#ifdef PJ_LIB__
/* Omega, Phi, Kappa: Rotations */
typedef struct {double o, p, k;} OPK;

/* Northing, Easting, and geodetic height */
typedef struct {double n, e, h;} NEh;

/* Northing, Easting, and orthometric height */
typedef struct {double n, e, H;} NEH;

/* Northing, Easting, and some kind of height */
typedef struct {double n, e, z;} NEZ;

/* Red, green and blue (e.g. for LiDAR colouring) */
typedef struct {double r, g, b;} PJ_RGB;

/* Avoid explicit type-punning: Use a union */
typedef union {
    XYZ xyz;
    LPZ lpz;
    OPK opk;
    NEh neh;
    NEH neH;
    NEZ nez;
    PJ_RGB rgb;
    XY  xy;
    LP  lp;
} COORDINATE;    /* or perhaps TRIPLET / PJ_TRIPLET? */


/* Apply the most appropriate projection function. No-op if none appropriate */
COORDINATE pj_apply_projection (COORDINATE point, int direction, PJ *P);

/* For debugging etc. */
int pj_show_coordinate (char *banner, COORDINATE point, int angular);
void pj_log_coordinate (projCtx ctx, int level, const char *banner, COORDINATE point, int angular);

int pj_set_isomorphic (PJ *P);
int pj_is_isomorphic (PJ *P);
int pj_is_pipeline (PJ *P);
int pj_pipeline_angular_output (PJ *P, int direction);
int pj_pipeline_angular_input (PJ *P, int direction);
int pj_pipeline_verbose (PJ *P);
int pj_pipeline_steps (PJ *P);
void pj_log_pipeline_steps (PJ *P, int level);
#endif


















#ifdef __cplusplus
}
#endif

#endif /* end of basic projections header */
