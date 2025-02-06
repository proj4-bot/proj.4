/******************************************************************************
 * Project:  PROJ.4
 * Purpose:  Implement a (currently minimalistic) proj API based primarily
 *           on the PJ_COORD 4D geodetic spatiotemporal data type.
 *
 * Author:   Thomas Knudsen,  thokn@sdfe.dk,  2016-06-09/2016-11-06
 *
 ******************************************************************************
 * Copyright (c) 2016, 2017 Thomas Knudsen/SDFE
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

#define FROM_PROJ_CPP

#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _MSC_VER
#include <strings.h>
#endif

#include <algorithm>
#include <limits>

#include "filemanager.hpp"
#include "geodesic.h"
#include "grids.hpp"
#include "proj.h"
#include "proj_experimental.h"
#include "proj_internal.h"
#include <cmath> /* for isnan */
#include <math.h>

#include "proj/common.hpp"
#include "proj/coordinateoperation.hpp"
#include "proj/internal/internal.hpp"
#include "proj/internal/io_internal.hpp"

using namespace NS_PROJ::internal;

/* Initialize PJ_COORD struct */
PJ_COORD proj_coord(double x, double y, double z, double t) {
    PJ_COORD res;
    res.v[0] = x;
    res.v[1] = y;
    res.v[2] = z;
    res.v[3] = t;
    return res;
}

static PJ_DIRECTION opposite_direction(PJ_DIRECTION dir) {
    return static_cast<PJ_DIRECTION>(-dir);
}

/*****************************************************************************/
int proj_angular_input(PJ *P, enum PJ_DIRECTION dir) {
    /******************************************************************************
        Returns 1 if the operator P expects angular input coordinates when
        operating in direction dir, 0 otherwise.
        dir: {PJ_FWD, PJ_INV}
    ******************************************************************************/
    if (PJ_FWD == dir)
        return pj_left(P) == PJ_IO_UNITS_RADIANS;
    return pj_right(P) == PJ_IO_UNITS_RADIANS;
}

/*****************************************************************************/
int proj_angular_output(PJ *P, enum PJ_DIRECTION dir) {
    /******************************************************************************
        Returns 1 if the operator P provides angular output coordinates when
        operating in direction dir, 0 otherwise.
        dir: {PJ_FWD, PJ_INV}
    ******************************************************************************/
    return proj_angular_input(P, opposite_direction(dir));
}

/*****************************************************************************/
int proj_degree_input(PJ *P, enum PJ_DIRECTION dir) {
    /******************************************************************************
        Returns 1 if the operator P expects degree input coordinates when
        operating in direction dir, 0 otherwise.
        dir: {PJ_FWD, PJ_INV}
    ******************************************************************************/
    if (PJ_FWD == dir)
        return pj_left(P) == PJ_IO_UNITS_DEGREES;
    return pj_right(P) == PJ_IO_UNITS_DEGREES;
}

/*****************************************************************************/
int proj_degree_output(PJ *P, enum PJ_DIRECTION dir) {
    /******************************************************************************
        Returns 1 if the operator P provides degree output coordinates when
        operating in direction dir, 0 otherwise.
        dir: {PJ_FWD, PJ_INV}
    ******************************************************************************/
    return proj_degree_input(P, opposite_direction(dir));
}

/* Geodesic distance (in meter) + fwd and rev azimuth between two points on the
 * ellipsoid */
PJ_COORD proj_geod(const PJ *P, PJ_COORD a, PJ_COORD b) {
    PJ_COORD c;
    if (!P->geod) {
        return proj_coord_error();
    }
    /* Note: the geodesic code takes arguments in degrees */
    geod_inverse(P->geod, PJ_TODEG(a.lpz.phi), PJ_TODEG(a.lpz.lam),
                 PJ_TODEG(b.lpz.phi), PJ_TODEG(b.lpz.lam), c.v, c.v + 1,
                 c.v + 2);

    // cppcheck-suppress uninitvar
    return c;
}

/* Geodesic distance (in meter) between two points with angular 2D coordinates
 */
double proj_lp_dist(const PJ *P, PJ_COORD a, PJ_COORD b) {
    double s12, azi1, azi2;
    /* Note: the geodesic code takes arguments in degrees */
    if (!P->geod) {
        return HUGE_VAL;
    }
    geod_inverse(P->geod, PJ_TODEG(a.lpz.phi), PJ_TODEG(a.lpz.lam),
                 PJ_TODEG(b.lpz.phi), PJ_TODEG(b.lpz.lam), &s12, &azi1, &azi2);
    return s12;
}

/* The geodesic distance AND the vertical offset */
double proj_lpz_dist(const PJ *P, PJ_COORD a, PJ_COORD b) {
    if (HUGE_VAL == a.lpz.lam || HUGE_VAL == b.lpz.lam)
        return HUGE_VAL;
    return hypot(proj_lp_dist(P, a, b), a.lpz.z - b.lpz.z);
}

/* Euclidean distance between two points with linear 2D coordinates */
double proj_xy_dist(PJ_COORD a, PJ_COORD b) {
    return hypot(a.xy.x - b.xy.x, a.xy.y - b.xy.y);
}

/* Euclidean distance between two points with linear 3D coordinates */
double proj_xyz_dist(PJ_COORD a, PJ_COORD b) {
    return hypot(proj_xy_dist(a, b), a.xyz.z - b.xyz.z);
}

static bool inline coord_is_all_nans(PJ_COORD coo) {
    return std::isnan(coo.v[0]) && std::isnan(coo.v[1]) &&
           std::isnan(coo.v[2]) && std::isnan(coo.v[3]);
}

static bool inline coord_has_nans(PJ_COORD coo) {
    return std::isnan(coo.v[0]) || std::isnan(coo.v[1]) ||
           std::isnan(coo.v[2]) || std::isnan(coo.v[3]);
}

/* Measure numerical deviation after n roundtrips fwd-inv (or inv-fwd) */
double proj_roundtrip(PJ *P, PJ_DIRECTION direction, int n, PJ_COORD *coord) {
    int i;
    PJ_COORD t, org;

    if (nullptr == P)
        return HUGE_VAL;

    if (n < 1) {
        proj_log_error(P, _("n should be >= 1"));
        proj_errno_set(P, PROJ_ERR_OTHER_API_MISUSE);
        return HUGE_VAL;
    }

    /* in the first half-step, we generate the output value */
    org = *coord;
    *coord = proj_trans(P, direction, org);
    t = *coord;

    /* now we take n-1 full steps in inverse direction: We are */
    /* out of phase due to the half step already taken */
    for (i = 0; i < n - 1; i++)
        t = proj_trans(P, direction,
                       proj_trans(P, opposite_direction(direction), t));

    /* finally, we take the last half-step */
    t = proj_trans(P, opposite_direction(direction), t);

    /* if we start with any NaN, we expect all NaN as output */
    if (coord_has_nans(org) && coord_is_all_nans(t)) {
        return 0.0;
    }

    /* checking for angular *input* since we do a roundtrip, and end where we
     * begin */
    if (proj_angular_input(P, direction))
        return proj_lpz_dist(P, org, t);

    return proj_xyz_dist(org, t);
}

/**************************************************************************************/
int pj_get_suggested_operation(PJ_CONTEXT *,
                               const std::vector<PJCoordOperation> &opList,
                               const int iExcluded[2], bool skipNonInstantiable,
                               PJ_DIRECTION direction, PJ_COORD coord)
/**************************************************************************************/
{
    const auto normalizeLongitude = [](double x) {
        if (x > 180.0) {
            x -= 360.0;
            if (x > 180.0)
                x = fmod(x + 180.0, 360.0) - 180.0;
        } else if (x < -180.0) {
            x += 360.0;
            if (x < -180.0)
                x = fmod(x + 180.0, 360.0) - 180.0;
        }
        return x;
    };

    // Select the operations that match the area of use
    // and has the best accuracy.
    int iBest = -1;
    double bestAccuracy = std::numeric_limits<double>::max();
    const int nOperations = static_cast<int>(opList.size());
    for (int i = 0; i < nOperations; i++) {
        if (i == iExcluded[0] || i == iExcluded[1]) {
            continue;
        }
        const auto &alt = opList[i];
        bool spatialCriterionOK = false;
        if (direction == PJ_FWD) {
            if (alt.pjSrcGeocentricToLonLat) {
                if (alt.minxSrc == -180 && alt.minySrc == -90 &&
                    alt.maxxSrc == 180 && alt.maxySrc == 90) {
                    spatialCriterionOK = true;
                } else {
                    PJ_COORD tmp = coord;
                    pj_fwd4d(tmp, alt.pjSrcGeocentricToLonLat);
                    if (tmp.xyzt.x >= alt.minxSrc &&
                        tmp.xyzt.y >= alt.minySrc &&
                        tmp.xyzt.x <= alt.maxxSrc &&
                        tmp.xyzt.y <= alt.maxySrc) {
                        spatialCriterionOK = true;
                    }
                }
            } else if (coord.xyzt.x >= alt.minxSrc &&
                       coord.xyzt.y >= alt.minySrc &&
                       coord.xyzt.x <= alt.maxxSrc &&
                       coord.xyzt.y <= alt.maxySrc) {
                spatialCriterionOK = true;
            } else if (alt.srcIsLonLatDegree && coord.xyzt.y >= alt.minySrc &&
                       coord.xyzt.y <= alt.maxySrc) {
                const double normalizedLon = normalizeLongitude(coord.xyzt.x);
                if (normalizedLon >= alt.minxSrc &&
                    normalizedLon <= alt.maxxSrc) {
                    spatialCriterionOK = true;
                }
            } else if (alt.srcIsLatLonDegree && coord.xyzt.x >= alt.minxSrc &&
                       coord.xyzt.x <= alt.maxxSrc) {
                const double normalizedLon = normalizeLongitude(coord.xyzt.y);
                if (normalizedLon >= alt.minySrc &&
                    normalizedLon <= alt.maxySrc) {
                    spatialCriterionOK = true;
                }
            }
        } else {
            if (alt.pjDstGeocentricToLonLat) {
                if (alt.minxDst == -180 && alt.minyDst == -90 &&
                    alt.maxxDst == 180 && alt.maxyDst == 90) {
                    spatialCriterionOK = true;
                } else {
                    PJ_COORD tmp = coord;
                    pj_fwd4d(tmp, alt.pjDstGeocentricToLonLat);
                    if (tmp.xyzt.x >= alt.minxDst &&
                        tmp.xyzt.y >= alt.minyDst &&
                        tmp.xyzt.x <= alt.maxxDst &&
                        tmp.xyzt.y <= alt.maxyDst) {
                        spatialCriterionOK = true;
                    }
                }
            } else if (coord.xyzt.x >= alt.minxDst &&
                       coord.xyzt.y >= alt.minyDst &&
                       coord.xyzt.x <= alt.maxxDst &&
                       coord.xyzt.y <= alt.maxyDst) {
                spatialCriterionOK = true;
            } else if (alt.dstIsLonLatDegree && coord.xyzt.y >= alt.minyDst &&
                       coord.xyzt.y <= alt.maxyDst) {
                const double normalizedLon = normalizeLongitude(coord.xyzt.x);
                if (normalizedLon >= alt.minxDst &&
                    normalizedLon <= alt.maxxDst) {
                    spatialCriterionOK = true;
                }
            } else if (alt.dstIsLatLonDegree && coord.xyzt.x >= alt.minxDst &&
                       coord.xyzt.x <= alt.maxxDst) {
                const double normalizedLon = normalizeLongitude(coord.xyzt.y);
                if (normalizedLon >= alt.minyDst &&
                    normalizedLon <= alt.maxyDst) {
                    spatialCriterionOK = true;
                }
            }
        }

        if (spatialCriterionOK) {
            // The offshore test is for the "Test bug 245 (use +datum=carthage)"
            // of test_cs2cs_various.yaml. The long=10 lat=34 point belongs
            // both to the onshore and offshore Tunisia area of uses, but is
            // slightly onshore. So in a general way, prefer a onshore area
            // to a offshore one.
            if (iBest < 0 ||
                (((alt.accuracy >= 0 && alt.accuracy < bestAccuracy) ||
                  // If two operations have the same accuracy, use
                  // the one that has the smallest area
                  (alt.accuracy == bestAccuracy &&
                   alt.pseudoArea < opList[iBest].pseudoArea &&
                   !(alt.isUnknownAreaName &&
                     !opList[iBest].isUnknownAreaName) &&
                   !opList[iBest].isPriorityOp)) &&
                 !alt.isOffshore)) {

                if (skipNonInstantiable && !alt.isInstantiable()) {
                    continue;
                }
                iBest = i;
                bestAccuracy = alt.accuracy;
            }
        }
    }

    return iBest;
}

//! @cond Doxygen_Suppress
/**************************************************************************************/
PJCoordOperation::~PJCoordOperation() {
    /**************************************************************************************/
    proj_destroy(pj);
    proj_destroy(pjSrcGeocentricToLonLat);
    proj_destroy(pjDstGeocentricToLonLat);
}

/**************************************************************************************/
bool PJCoordOperation::isInstantiable() const {
    /**************************************************************************************/
    if (isInstantiableCached == INSTANTIABLE_STATUS_UNKNOWN)
        isInstantiableCached = proj_coordoperation_is_instantiable(pj->ctx, pj);
    return (isInstantiableCached == 1);
}
//! @endcond

/**************************************************************************************/
static void warnAboutMissingGrid(PJ *P)
/**************************************************************************************/
{
    std::string msg("Attempt to use coordinate operation ");
    msg += proj_get_name(P);
    msg += " failed.";
    int gridUsed = proj_coordoperation_get_grid_used_count(P->ctx, P);
    for (int i = 0; i < gridUsed; ++i) {
        const char *gridName = "";
        int available = FALSE;
        if (proj_coordoperation_get_grid_used(P->ctx, P, i, &gridName, nullptr,
                                              nullptr, nullptr, nullptr,
                                              nullptr, &available) &&
            !available) {
            msg += " Grid ";
            msg += gridName;
            msg += " is not available. "
                   "Consult https://proj.org/resource_files.html for guidance.";
        }
    }
    if (!P->errorIfBestTransformationNotAvailable &&
        P->warnIfBestTransformationNotAvailable) {
        msg += " This might become an error in a future PROJ major release. "
               "Set the ONLY_BEST option to YES or NO. "
               "This warning will no longer be emitted (for the current "
               "transformation instance).";
        P->warnIfBestTransformationNotAvailable = false;
    }
    pj_log(P->ctx,
           P->errorIfBestTransformationNotAvailable ? PJ_LOG_ERROR
                                                    : PJ_LOG_DEBUG,
           msg.c_str());
}

/**************************************************************************************/
PJ_COORD proj_trans(PJ *P, PJ_DIRECTION direction, PJ_COORD coord) {
    /***************************************************************************************
    Apply the transformation P to the coordinate coord, preferring the 4D
    interfaces if available.

    See also pj_approx_2D_trans and pj_approx_3D_trans in pj_internal.c, which
    work similarly, but prefers the 2D resp. 3D interfaces if available.
    ***************************************************************************************/
    if (nullptr == P || direction == PJ_IDENT)
        return coord;
    if (P->inverted)
        direction = opposite_direction(direction);

    if (P->iso_obj != nullptr && !P->iso_obj_is_coordinate_operation) {
        pj_log(P->ctx, PJ_LOG_ERROR, "Object is not a coordinate operation");
        proj_errno_set(P, PROJ_ERR_INVALID_OP_ILLEGAL_ARG_VALUE);
        return proj_coord_error();
    }

    if (!P->alternativeCoordinateOperations.empty()) {
        constexpr int N_MAX_RETRY = 2;
        int iExcluded[N_MAX_RETRY] = {-1, -1};

        bool skipNonInstantiable = P->skipNonInstantiable &&
                                   !P->warnIfBestTransformationNotAvailable &&
                                   !P->errorIfBestTransformationNotAvailable;
        const int nOperations =
            static_cast<int>(P->alternativeCoordinateOperations.size());

        // We may need several attempts. For example the point at
        // long=-111.5 lat=45.26 falls into the bounding box of the Canadian
        // ntv2_0.gsb grid, except that it is not in any of the subgrids, being
        // in the US. We thus need another retry that will select the conus
        // grid.
        for (int iRetry = 0; iRetry <= N_MAX_RETRY; iRetry++) {
            // Do a first pass and select the operations that match the area of
            // use and has the best accuracy.
            int iBest = pj_get_suggested_operation(
                P->ctx, P->alternativeCoordinateOperations, iExcluded,
                skipNonInstantiable, direction, coord);
            if (iBest < 0) {
                break;
            }
            if (iRetry > 0) {
                const int oldErrno = proj_errno_reset(P);
                if (proj_log_level(P->ctx, PJ_LOG_TELL) >= PJ_LOG_DEBUG) {
                    pj_log(P->ctx, PJ_LOG_DEBUG,
                           proj_context_errno_string(P->ctx, oldErrno));
                }
                pj_log(P->ctx, PJ_LOG_DEBUG,
                       "Did not result in valid result. "
                       "Attempting a retry with another operation.");
            }

            const auto &alt = P->alternativeCoordinateOperations[iBest];
            if (P->iCurCoordOp != iBest) {
                if (proj_log_level(P->ctx, PJ_LOG_TELL) >= PJ_LOG_DEBUG) {
                    std::string msg("Using coordinate operation ");
                    msg += alt.name;
                    pj_log(P->ctx, PJ_LOG_DEBUG, msg.c_str());
                }
                P->iCurCoordOp = iBest;
            }
            PJ_COORD res = coord;
            if (alt.pj->hasCoordinateEpoch)
                coord.xyzt.t = alt.pj->coordinateEpoch;
            if (direction == PJ_FWD)
                pj_fwd4d(res, alt.pj);
            else
                pj_inv4d(res, alt.pj);
            if (proj_errno(alt.pj) == PROJ_ERR_OTHER_NETWORK_ERROR) {
                return proj_coord_error();
            }
            if (res.xyzt.x != HUGE_VAL) {
                return res;
            } else if (P->errorIfBestTransformationNotAvailable ||
                       P->warnIfBestTransformationNotAvailable) {
                warnAboutMissingGrid(alt.pj);
                if (P->errorIfBestTransformationNotAvailable) {
                    proj_errno_set(P, PROJ_ERR_COORD_TRANSFM_NO_OPERATION);
                    return res;
                }
                P->warnIfBestTransformationNotAvailable = false;
                skipNonInstantiable = true;
            }
            if (iRetry == N_MAX_RETRY) {
                break;
            }
            iExcluded[iRetry] = iBest;
        }

        // In case we did not find an operation whose area of use is compatible
        // with the input coordinate, then goes through again the list, and
        // use the first operation that does not require grids.
        NS_PROJ::io::DatabaseContextPtr dbContext;
        try {
            if (P->ctx->cpp_context) {
                dbContext =
                    P->ctx->cpp_context->getDatabaseContext().as_nullable();
            }
        } catch (const std::exception &) {
        }
        for (int i = 0; i < nOperations; i++) {
            const auto &alt = P->alternativeCoordinateOperations[i];
            auto coordOperation =
                dynamic_cast<NS_PROJ::operation::CoordinateOperation *>(
                    alt.pj->iso_obj.get());
            if (coordOperation) {
                if (coordOperation->gridsNeeded(dbContext, true).empty()) {
                    if (P->iCurCoordOp != i) {
                        if (proj_log_level(P->ctx, PJ_LOG_TELL) >=
                            PJ_LOG_DEBUG) {
                            std::string msg("Using coordinate operation ");
                            msg += alt.name;
                            msg += " as a fallback due to lack of more "
                                   "appropriate operations";
                            pj_log(P->ctx, PJ_LOG_DEBUG, msg.c_str());
                        }
                        P->iCurCoordOp = i;
                    }
                    if (direction == PJ_FWD) {
                        pj_fwd4d(coord, alt.pj);
                    } else {
                        pj_inv4d(coord, alt.pj);
                    }
                    return coord;
                }
            }
        }

        proj_errno_set(P, PROJ_ERR_COORD_TRANSFM_NO_OPERATION);
        return proj_coord_error();
    }

    P->iCurCoordOp =
        0; // dummy value, to be used by proj_trans_get_last_used_operation()
    if (P->hasCoordinateEpoch)
        coord.xyzt.t = P->coordinateEpoch;
    if (coord_has_nans(coord))
        coord.v[0] = coord.v[1] = coord.v[2] = coord.v[3] =
            std::numeric_limits<double>::quiet_NaN();
    else if (direction == PJ_FWD)
        pj_fwd4d(coord, P);
    else
        pj_inv4d(coord, P);
    return coord;
}

/*****************************************************************************/
PJ *proj_trans_get_last_used_operation(PJ *P)
/******************************************************************************
    Return the operation used during the last invocation of proj_trans().
    This is especially useful when P has been created with
proj_create_crs_to_crs() and has several alternative operations. The returned
object must be freed with proj_destroy().
******************************************************************************/
{
    if (nullptr == P || P->iCurCoordOp < 0)
        return nullptr;
    if (P->alternativeCoordinateOperations.empty())
        return proj_clone(P->ctx, P);
    return proj_clone(P->ctx,
                      P->alternativeCoordinateOperations[P->iCurCoordOp].pj);
}

/*****************************************************************************/
int proj_trans_array(PJ *P, PJ_DIRECTION direction, size_t n, PJ_COORD *coord) {
    /******************************************************************************
        Batch transform an array of PJ_COORD.

        Performs transformation on all points, even if errors occur on some
    points.

        Individual points that fail to transform will have their components set
    to HUGE_VAL

        Returns 0 if all coordinates are transformed without error, otherwise
        returns a precise error number if all coordinates that fail to transform
        for the same reason, or a generic error code if they fail for different
        reasons.
    ******************************************************************************/
    size_t i;
    int retErrno = 0;
    bool hasSetRetErrno = false;
    bool sameRetErrno = true;

    for (i = 0; i < n; i++) {
        proj_context_errno_set(P->ctx, 0);
        coord[i] = proj_trans(P, direction, coord[i]);
        int thisErrno = proj_errno(P);
        if (thisErrno != 0) {
            if (!hasSetRetErrno) {
                retErrno = thisErrno;
                hasSetRetErrno = true;
            } else if (sameRetErrno && retErrno != thisErrno) {
                sameRetErrno = false;
                retErrno = PROJ_ERR_COORD_TRANSFM;
            }
        }
    }

    proj_context_errno_set(P->ctx, retErrno);

    return retErrno;
}

/*************************************************************************************/
size_t proj_trans_generic(PJ *P, PJ_DIRECTION direction, double *x, size_t sx,
                          size_t nx, double *y, size_t sy, size_t ny, double *z,
                          size_t sz, size_t nz, double *t, size_t st,
                          size_t nt) {
    /**************************************************************************************

        Transform a series of coordinates, where the individual coordinate
    dimension may be represented by an array that is either

            1. fully populated
            2. a null pointer and/or a length of zero, which will be treated as
    a fully populated array of zeroes
            3. of length one, i.e. a constant, which will be treated as a fully
               populated array of that constant value

        The strides, sx, sy, sz, st, represent the step length, in bytes,
    between consecutive elements of the corresponding array. This makes it
    possible for proj_transform to handle transformation of a large class of
    application specific data structures, without necessarily understanding the
    data structure format, as in:

            typedef struct {double x, y; int quality_level; char
    surveyor_name[134];} XYQS; XYQS survey[345]; double height = 23.45; PJ *P =
    {...}; size_t stride = sizeof (XYQS);
            ...
            proj_transform (
                P, PJ_INV, sizeof(XYQS),
                &(survey[0].x), stride, 345,  (*  We have 345 eastings  *)
                &(survey[0].y), stride, 345,  (*  ...and 345 northings. *)
                &height, 1,                   (*  The height is the
    constant  23.45 m *) 0, 0                          (*  and the time is the
    constant 0.00 s *)
            );

        This is similar to the inner workings of the pj_transform function, but
    the stride functionality has been generalized to work for any size of basic
    unit, not just a fixed number of doubles.

        In most cases, the stride will be identical for x, y,z, and t, since
    they will typically be either individual arrays (stride = sizeof(double)),
    or strided views into an array of application specific data structures
    (stride = sizeof (...)).

        But in order to support cases where x, y, z, and t come from
    heterogeneous sources, individual strides, sx, sy, sz, st, are used.

        Caveat: Since proj_transform does its work *in place*, this means that
    even the supposedly constants (i.e. length 1 arrays) will return from the
    call in altered state. Hence, remember to reinitialize between repeated
    calls.

        Return value: Number of transformations completed.

    **************************************************************************************/
    PJ_COORD coord = {{0, 0, 0, 0}};
    size_t i, nmin;
    double null_broadcast = 0;
    double invalid_time = HUGE_VAL;

    if (nullptr == P)
        return 0;

    if (P->inverted)
        direction = opposite_direction(direction);

    /* ignore lengths of null arrays */
    if (nullptr == x)
        nx = 0;
    if (nullptr == y)
        ny = 0;
    if (nullptr == z)
        nz = 0;
    if (nullptr == t)
        nt = 0;

    /* and make the nullities point to some real world memory for broadcasting
     * nulls */
    if (0 == nx)
        x = &null_broadcast;
    if (0 == ny)
        y = &null_broadcast;
    if (0 == nz)
        z = &null_broadcast;
    if (0 == nt)
        t = &invalid_time;

    /* nothing to do? */
    if (0 == nx + ny + nz + nt)
        return 0;

    /* arrays of length 1 are constants, which we broadcast along the longer
     * arrays */
    /* so we need to find the length of the shortest non-unity array to figure
     * out
     */
    /* how many coordinate pairs we must transform */
    nmin = (nx > 1) ? nx : (ny > 1) ? ny : (nz > 1) ? nz : (nt > 1) ? nt : 1;
    if ((nx > 1) && (nx < nmin))
        nmin = nx;
    if ((ny > 1) && (ny < nmin))
        nmin = ny;
    if ((nz > 1) && (nz < nmin))
        nmin = nz;
    if ((nt > 1) && (nt < nmin))
        nmin = nt;

    /* Check validity of direction flag */
    switch (direction) {
    case PJ_FWD:
    case PJ_INV:
        break;
    case PJ_IDENT:
        return nmin;
    }

    /* Arrays of length==0 are broadcast as the constant 0               */
    /* Arrays of length==1 are broadcast as their single value           */
    /* Arrays of length >1 are iterated over (for the first nmin values) */
    /* The slightly convolved incremental indexing is used due           */
    /* to the stride, which may be any size supported by the platform    */
    for (i = 0; i < nmin; i++) {
        coord.xyzt.x = *x;
        coord.xyzt.y = *y;
        coord.xyzt.z = *z;
        coord.xyzt.t = *t;

        coord = proj_trans(P, direction, coord);

        /* in all full length cases, we overwrite the input with the output,  */
        /* and step on to the next element.                                   */
        /* The casts are somewhat funky, but they compile down to no-ops and  */
        /* they tell compilers and static analyzers that we know what we do   */
        if (nx > 1) {
            *x = coord.xyzt.x;
            x = (double *)((void *)(((char *)x) + sx));
        }
        if (ny > 1) {
            *y = coord.xyzt.y;
            y = (double *)((void *)(((char *)y) + sy));
        }
        if (nz > 1) {
            *z = coord.xyzt.z;
            z = (double *)((void *)(((char *)z) + sz));
        }
        if (nt > 1) {
            *t = coord.xyzt.t;
            t = (double *)((void *)(((char *)t) + st));
        }
    }

    /* Last time around, we update the length 1 cases with their transformed
     * alter egos */
    if (nx == 1)
        *x = coord.xyzt.x;
    if (ny == 1)
        *y = coord.xyzt.y;
    if (nz == 1)
        *z = coord.xyzt.z;
    if (nt == 1)
        *t = coord.xyzt.t;

    return i;
}

/*************************************************************************************/
PJ_COORD pj_geocentric_latitude(const PJ *P, PJ_DIRECTION direction,
                                PJ_COORD coord) {
    /**************************************************************************************
        Convert geographical latitude to geocentric (or the other way round if
        direction = PJ_INV)

        The conversion involves a call to the tangent function, which goes
    through the roof at the poles, so very close (the last centimeter) to the
    poles no conversion takes place and the input latitude is copied directly to
    the output.

        Fortunately, the geocentric latitude converges to the geographical at
    the poles, so the difference is negligible.

        For the spherical case, the geographical latitude equals the geocentric,
    and consequently, the input is copied directly to the output.
    **************************************************************************************/
    const double limit = M_HALFPI - 1e-9;
    PJ_COORD res = coord;
    if ((coord.lp.phi > limit) || (coord.lp.phi < -limit) || (P->es == 0))
        return res;
    if (direction == PJ_FWD)
        res.lp.phi = atan(P->one_es * tan(coord.lp.phi));
    else
        res.lp.phi = atan(P->rone_es * tan(coord.lp.phi));

    return res;
}

double proj_torad(double angle_in_degrees) {
    return PJ_TORAD(angle_in_degrees);
}
double proj_todeg(double angle_in_radians) {
    return PJ_TODEG(angle_in_radians);
}

double proj_dmstor(const char *is, char **rs) { return dmstor(is, rs); }

char *proj_rtodms(char *s, double r, int pos, int neg) {
    // 40 is the size used for the buffer in proj.cpp
    size_t arbitrary_size = 40;
    return rtodms(s, arbitrary_size, r, pos, neg);
}

char *proj_rtodms2(char *s, size_t sizeof_s, double r, int pos, int neg) {
    return rtodms(s, sizeof_s, r, pos, neg);
}

/*************************************************************************************/
static PJ *skip_prep_fin(PJ *P) {
    /**************************************************************************************
    Skip prepare and finalize function for the various "helper operations" added
    to P when in cs2cs compatibility mode.
    **************************************************************************************/
    P->skip_fwd_prepare = 1;
    P->skip_fwd_finalize = 1;
    P->skip_inv_prepare = 1;
    P->skip_inv_finalize = 1;
    return P;
}

/*************************************************************************************/
static int cs2cs_emulation_setup(PJ *P) {
    /**************************************************************************************
    If any cs2cs style modifiers are given (axis=..., towgs84=..., ) create the
    4D API equivalent operations, so the preparation and finalization steps in
    the pj_inv/pj_fwd invocators can emulate the behavior of pj_transform and
    the cs2cs app.

    Returns 1 on success, 0 on failure
    **************************************************************************************/
    PJ *Q;
    paralist *p;
    int do_cart = 0;
    if (nullptr == P)
        return 0;

    /* Don't recurse when calling proj_create (which calls us back) */
    if (pj_param_exists(P->params, "break_cs2cs_recursion"))
        return 1;

    /* Swap axes? */
    p = pj_param_exists(P->params, "axis");

    const bool disable_grid_presence_check =
        pj_param_exists(P->params, "disable_grid_presence_check") != nullptr;

    /* Don't axisswap if data are already in "enu" order */
    if (p && (0 != strcmp("enu", p->param))) {
        size_t def_size = 100 + strlen(P->axis);
        char *def = static_cast<char *>(malloc(def_size));
        if (nullptr == def)
            return 0;
        snprintf(def, def_size,
                 "break_cs2cs_recursion     proj=axisswap  axis=%s", P->axis);
        Q = pj_create_internal(P->ctx, def);
        free(def);
        if (nullptr == Q)
            return 0;
        P->axisswap = skip_prep_fin(Q);
    }

    /* Geoid grid(s) given? */
    p = pj_param_exists(P->params, "geoidgrids");
    if (!disable_grid_presence_check && p &&
        strlen(p->param) > strlen("geoidgrids=")) {
        char *gridnames = p->param + strlen("geoidgrids=");
        size_t def_size = 100 + 2 * strlen(gridnames);
        char *def = static_cast<char *>(malloc(def_size));
        if (nullptr == def)
            return 0;
        snprintf(def, def_size,
                 "break_cs2cs_recursion     proj=vgridshift  grids=%s",
                 pj_double_quote_string_param_if_needed(gridnames).c_str());
        Q = pj_create_internal(P->ctx, def);
        free(def);
        if (nullptr == Q)
            return 0;
        P->vgridshift = skip_prep_fin(Q);
    }

    /* Datum shift grid(s) given? */
    p = pj_param_exists(P->params, "nadgrids");
    if (!disable_grid_presence_check && p &&
        strlen(p->param) > strlen("nadgrids=")) {
        char *gridnames = p->param + strlen("nadgrids=");
        size_t def_size = 100 + 2 * strlen(gridnames);
        char *def = static_cast<char *>(malloc(def_size));
        if (nullptr == def)
            return 0;
        snprintf(def, def_size,
                 "break_cs2cs_recursion     proj=hgridshift  grids=%s",
                 pj_double_quote_string_param_if_needed(gridnames).c_str());
        Q = pj_create_internal(P->ctx, def);
        free(def);
        if (nullptr == Q)
            return 0;
        P->hgridshift = skip_prep_fin(Q);
    }

    /* We ignore helmert if we have grid shift */
    p = P->hgridshift ? nullptr : pj_param_exists(P->params, "towgs84");
    while (p) {
        const char *const s = p->param;
        const double *const d = P->datum_params;

        /* We ignore null helmert shifts (common in auto-translated resource
         * files, e.g. epsg) */
        if (0 == d[0] && 0 == d[1] && 0 == d[2] && 0 == d[3] && 0 == d[4] &&
            0 == d[5] && 0 == d[6]) {
            /* If the current ellipsoid is not WGS84, then make sure the */
            /* change in ellipsoid is still done. */
            if (!(fabs(P->a_orig - 6378137.0) < 1e-8 &&
                  fabs(P->es_orig - 0.0066943799901413) < 1e-15)) {
                do_cart = 1;
            }
            break;
        }

        const size_t n = strlen(s);
        if (n <= 8) /* 8==strlen ("towgs84=") */
            return 0;

        const size_t def_max_size = 100 + n;
        std::string def;
        def.reserve(def_max_size);
        def += "break_cs2cs_recursion     proj=helmert exact ";
        def += s;
        def += " convention=position_vector";
        Q = pj_create_internal(P->ctx, def.c_str());
        if (nullptr == Q)
            return 0;
        pj_inherit_ellipsoid_def(P, Q);
        P->helmert = skip_prep_fin(Q);

        break;
    }

    /* We also need cartesian/geographical transformations if we are working in
     */
    /* geocentric/cartesian space or we need to do a Helmert transform. */
    if (P->is_geocent || P->helmert || do_cart) {
        char def[150];
        snprintf(def, sizeof(def),
                 "break_cs2cs_recursion     proj=cart   a=%40.20g  es=%40.20g",
                 P->a_orig, P->es_orig);
        {
            /* In case the current locale does not use dot but comma as decimal
             */
            /* separator, replace it with dot, so that proj_atof() behaves */
            /* correctly. */
            /* TODO later: use C++ ostringstream with
             * imbue(std::locale::classic()) */
            /* to be locale unaware */
            char *next_pos;
            for (next_pos = def; (next_pos = strchr(next_pos, ',')) != nullptr;
                 next_pos++) {
                *next_pos = '.';
            }
        }
        Q = pj_create_internal(P->ctx, def);
        if (nullptr == Q)
            return 0;
        P->cart = skip_prep_fin(Q);

        if (!P->is_geocent) {
            snprintf(def, sizeof(def),
                     "break_cs2cs_recursion     proj=cart  ellps=WGS84");
            Q = pj_create_internal(P->ctx, def);
            if (nullptr == Q)
                return 0;
            P->cart_wgs84 = skip_prep_fin(Q);
        }
    }

    return 1;
}

/*************************************************************************************/
PJ *pj_create_internal(PJ_CONTEXT *ctx, const char *definition) {
    /*************************************************************************************/

    /**************************************************************************************
        Create a new PJ object in the context ctx, using the given definition.
    If ctx==0, the default context is used, if definition==0, or invalid, a
    null-pointer is returned. The definition may use '+' as argument start
    indicator, as in
        "+proj=utm +zone=32", or leave it out, as in "proj=utm zone=32".

        It may even use free formatting "proj  =  utm;  zone  =32  ellps=
    GRS80". Note that the semicolon separator is allowed, but not required.
    **************************************************************************************/
    char *args, **argv;
    size_t argc, n;

    if (nullptr == ctx)
        ctx = pj_get_default_ctx();

    /* Make a copy that we can manipulate */
    n = strlen(definition);
    args = (char *)malloc(n + 1);
    if (nullptr == args) {
        proj_context_errno_set(ctx, PROJ_ERR_OTHER /*ENOMEM*/);
        return nullptr;
    }
    strcpy(args, definition);

    argc = pj_trim_argc(args);
    if (argc == 0) {
        free(args);
        proj_context_errno_set(ctx, PROJ_ERR_INVALID_OP_MISSING_ARG);
        return nullptr;
    }

    argv = pj_trim_argv(argc, args);
    if (!argv) {
        free(args);
        proj_context_errno_set(ctx, PROJ_ERR_OTHER /*ENOMEM*/);
        return nullptr;
    }

    PJ *P = pj_create_argv_internal(ctx, (int)argc, argv);

    free(argv);
    free(args);

    return P;
}

/*************************************************************************************/
PJ *proj_create_argv(PJ_CONTEXT *ctx, int argc, char **argv) {
    /**************************************************************************************
    Create a new PJ object in the context ctx, using the given definition
    argument array argv. If ctx==0, the default context is used, if
    definition==0, or invalid, a null-pointer is returned. The definition
    arguments may use '+' as argument start indicator, as in {"+proj=utm",
    "+zone=32"}, or leave it out, as in {"proj=utm", "zone=32"}.
    **************************************************************************************/

    if (nullptr == ctx)
        ctx = pj_get_default_ctx();
    if (nullptr == argv) {
        proj_context_errno_set(ctx, PROJ_ERR_INVALID_OP_MISSING_ARG);
        return nullptr;
    }

    /* We assume that free format is used, and build a full proj_create
     * compatible string */
    char *c = pj_make_args(argc, argv);
    if (nullptr == c) {
        proj_context_errno_set(ctx, PROJ_ERR_INVALID_OP /* ENOMEM */);
        return nullptr;
    }

    PJ *P = proj_create(ctx, c);

    free((char *)c);
    return P;
}

/*************************************************************************************/
PJ *pj_create_argv_internal(PJ_CONTEXT *ctx, int argc, char **argv) {
    /**************************************************************************************
    For use by pipeline init function.
    **************************************************************************************/
    if (nullptr == ctx)
        ctx = pj_get_default_ctx();
    if (nullptr == argv) {
        proj_context_errno_set(ctx, PROJ_ERR_INVALID_OP_MISSING_ARG);
        return nullptr;
    }

    /* ...and let pj_init_ctx do the hard work */
    /* New interface: forbid init=epsg:XXXX syntax by default */
    const int allow_init_epsg =
        proj_context_get_use_proj4_init_rules(ctx, FALSE);
    PJ *P = pj_init_ctx_with_allow_init_epsg(ctx, argc, argv, allow_init_epsg);

    /* Support cs2cs-style modifiers */
    int ret = cs2cs_emulation_setup(P);
    if (0 == ret)
        return proj_destroy(P);

    return P;
}

/** Create an area of use */
PJ_AREA *proj_area_create(void) { return new PJ_AREA(); }

/** Assign a bounding box to an area of use. */
void proj_area_set_bbox(PJ_AREA *area, double west_lon_degree,
                        double south_lat_degree, double east_lon_degree,
                        double north_lat_degree) {
    area->bbox_set = TRUE;
    area->west_lon_degree = west_lon_degree;
    area->south_lat_degree = south_lat_degree;
    area->east_lon_degree = east_lon_degree;
    area->north_lat_degree = north_lat_degree;
}

/** Assign the name of an area of use. */
void proj_area_set_name(PJ_AREA *area, const char *name) { area->name = name; }

/** Free an area of use */
void proj_area_destroy(PJ_AREA *area) { delete area; }

/************************************************************************/
/*                  proj_context_use_proj4_init_rules()                 */
/************************************************************************/

void proj_context_use_proj4_init_rules(PJ_CONTEXT *ctx, int enable) {
    if (ctx == nullptr) {
        ctx = pj_get_default_ctx();
    }
    ctx->use_proj4_init_rules = enable;
}

/************************************************************************/
/*                              EQUAL()                                 */
/************************************************************************/

static int EQUAL(const char *a, const char *b) {
#ifdef _MSC_VER
    return _stricmp(a, b) == 0;
#else
    return strcasecmp(a, b) == 0;
#endif
}

/************************************************************************/
/*                  proj_context_get_use_proj4_init_rules()             */
/************************************************************************/

int proj_context_get_use_proj4_init_rules(PJ_CONTEXT *ctx,
                                          int from_legacy_code_path) {
    const char *val = getenv("PROJ_USE_PROJ4_INIT_RULES");

    if (ctx == nullptr) {
        ctx = pj_get_default_ctx();
    }

    if (val) {
        if (EQUAL(val, "yes") || EQUAL(val, "on") || EQUAL(val, "true")) {
            return TRUE;
        }
        if (EQUAL(val, "no") || EQUAL(val, "off") || EQUAL(val, "false")) {
            return FALSE;
        }
        pj_log(ctx, PJ_LOG_ERROR,
               "Invalid value for PROJ_USE_PROJ4_INIT_RULES");
    }

    if (ctx->use_proj4_init_rules >= 0) {
        return ctx->use_proj4_init_rules;
    }
    return from_legacy_code_path;
}

/** Adds a " +type=crs" suffix to a PROJ string (if it is a PROJ string) */
std::string pj_add_type_crs_if_needed(const std::string &str) {
    std::string ret(str);
    if ((starts_with(str, "proj=") || starts_with(str, "+proj=") ||
         starts_with(str, "+init=") || starts_with(str, "+title=")) &&
        str.find("type=crs") == std::string::npos) {
        ret += " +type=crs";
    }
    return ret;
}

// ---------------------------------------------------------------------------
static double simple_min(const double *data, const int arr_len) {
    double min_value = data[0];
    for (int iii = 1; iii < arr_len; iii++) {
        if (data[iii] < min_value)
            min_value = data[iii];
    }
    return min_value;
}

// ---------------------------------------------------------------------------
static double simple_max(const double *data, const int arr_len) {
    double max_value = data[0];
    for (int iii = 1; iii < arr_len; iii++) {
        if ((data[iii] > max_value || max_value == HUGE_VAL) &&
            data[iii] != HUGE_VAL)
            max_value = data[iii];
    }
    return max_value;
}

// ---------------------------------------------------------------------------
static int find_previous_index(const int iii, const double *data,
                               const int arr_len) {
    // find index of nearest valid previous value if exists
    int prev_iii = iii - 1;
    if (prev_iii == -1) // handle wraparound
        prev_iii = arr_len - 1;
    while (data[prev_iii] == HUGE_VAL && prev_iii != iii) {
        prev_iii--;
        if (prev_iii == -1) // handle wraparound
            prev_iii = arr_len - 1;
    }
    return prev_iii;
}

// ---------------------------------------------------------------------------
/******************************************************************************
Handles the case when longitude values cross the antimeridian
when calculating the minimum.
Note: The data array must be in a linear ring.
Note: This requires a densified ring with at least 2 additional
        points per edge to correctly handle global extents.
If only 1 additional point:
    |        |
    |RL--x0--|RL--
    |        |
-180    180|-180
If they are evenly spaced and it crosses the antimeridian:
x0 - L = 180
R - x0 = -180
For example:
Let R = -179.9, x0 = 0.1, L = -179.89
x0 - L = 0.1 - -179.9 = 180
R - x0 = -179.89 - 0.1 ~= -180
This is the same in the case when it didn't cross the antimeridian.
If you have 2 additional points:
    |            |
    |RL--x0--x1--|RL--
    |            |
-180        180|-180
If they are evenly spaced and it crosses the antimeridian:
x0 - L = 120
x1 - x0 = 120
R - x1 = -240
For example:
Let R = -179.9, x0 = -59.9, x1 = 60.1 L = -179.89
x0 - L = 59.9 - -179.9 = 120
x1 - x0 = 60.1 - 59.9 = 120
R - x1 = -179.89 - 60.1 ~= -240
However, if they are evenly spaced and it didn't cross the antimeridian:
x0 - L = 120
x1 - x0 = 120
R - x1 = 120
From this, we have a delta that is guaranteed to be significantly
large enough to tell the difference reguarless of the direction
the antimeridian was crossed.
However, even though the spacing was even in the source projection, it isn't
guaranteed in the target geographic projection. So, instead of 240, 200 is used
as it significantly larger than 120 to be sure that the antimeridian was crossed
but smalller than 240 to account for possible irregularities in distances
when re-projecting. Also, 200 ensures latitudes are ignored for axis order
handling.
******************************************************************************/
static double antimeridian_min(const double *data, const int arr_len) {
    double positive_min = HUGE_VAL;
    double min_value = HUGE_VAL;
    int crossed_meridian_count = 0;
    bool positive_meridian = false;

    for (int iii = 0; iii < arr_len; iii++) {
        if (data[iii] == HUGE_VAL)
            continue;
        int prev_iii = find_previous_index(iii, data, arr_len);
        // check if crossed meridian
        double delta = data[prev_iii] - data[iii];
        // 180 -> -180
        if (delta >= 200 && delta != HUGE_VAL) {
            if (crossed_meridian_count == 0)
                positive_min = min_value;
            crossed_meridian_count++;
            positive_meridian = false;
            // -180 -> 180
        } else if (delta <= -200 && delta != HUGE_VAL) {
            if (crossed_meridian_count == 0)
                positive_min = data[iii];
            crossed_meridian_count++;
            positive_meridian = true;
        }
        // positive meridian side min
        if (positive_meridian && data[iii] < positive_min)
            positive_min = data[iii];
        // track general min value
        if (data[iii] < min_value)
            min_value = data[iii];
    }

    if (crossed_meridian_count == 2)
        return positive_min;
    else if (crossed_meridian_count == 4)
        // bounds extends beyond -180/180
        return -180;
    return min_value;
}

// ---------------------------------------------------------------------------
// Handles the case when longitude values cross the antimeridian
// when calculating the minimum.
// Note: The data array must be in a linear ring.
// Note: This requires a densified ring with at least 2 additional
//       points per edge to correctly handle global extents.
// See antimeridian_min docstring for reasoning.
static double antimeridian_max(const double *data, const int arr_len) {
    double negative_max = -HUGE_VAL;
    double max_value = -HUGE_VAL;
    bool negative_meridian = false;
    int crossed_meridian_count = 0;

    for (int iii = 0; iii < arr_len; iii++) {
        if (data[iii] == HUGE_VAL)
            continue;
        int prev_iii = find_previous_index(iii, data, arr_len);
        // check if crossed meridian
        double delta = data[prev_iii] - data[iii];
        // 180 -> -180
        if (delta >= 200 && delta != HUGE_VAL) {
            if (crossed_meridian_count == 0)
                negative_max = data[iii];
            crossed_meridian_count++;
            negative_meridian = true;
            // -180 -> 180
        } else if (delta <= -200 && delta != HUGE_VAL) {
            if (crossed_meridian_count == 0)
                negative_max = max_value;
            negative_meridian = false;
            crossed_meridian_count++;
        }
        // negative meridian side max
        if (negative_meridian &&
            (data[iii] > negative_max || negative_max == HUGE_VAL) &&
            data[iii] != HUGE_VAL)
            negative_max = data[iii];
        // track general max value
        if ((data[iii] > max_value || max_value == HUGE_VAL) &&
            data[iii] != HUGE_VAL)
            max_value = data[iii];
    }
    if (crossed_meridian_count == 2)
        return negative_max;
    else if (crossed_meridian_count == 4)
        // bounds extends beyond -180/180
        return 180;
    return max_value;
}

// ---------------------------------------------------------------------------
// Check if the original projected bounds contains
// the north pole.
// This assumes that the destination CRS is geographic.
static bool contains_north_pole(PJ *projobj, PJ_DIRECTION pj_direction,
                                const double xmin, const double ymin,
                                const double xmax, const double ymax,
                                bool lon_lat_order) {
    double pole_y = 90;
    double pole_x = 0;
    if (!lon_lat_order) {
        pole_y = 0;
        pole_x = 90;
    }
    proj_trans_generic(projobj, opposite_direction(pj_direction), &pole_x,
                       sizeof(double), 1, &pole_y, sizeof(double), 1, nullptr,
                       sizeof(double), 0, nullptr, sizeof(double), 0);
    if (xmin < pole_x && pole_x < xmax && ymax > pole_y && pole_y > ymin)
        return true;
    return false;
}

// ---------------------------------------------------------------------------
// Check if the original projected bounds contains
// the south pole.
// This assumes that the destination CRS is geographic.
static bool contains_south_pole(PJ *projobj, PJ_DIRECTION pj_direction,
                                const double xmin, const double ymin,
                                const double xmax, const double ymax,
                                bool lon_lat_order) {
    double pole_y = -90;
    double pole_x = 0;
    if (!lon_lat_order) {
        pole_y = 0;
        pole_x = -90;
    }
    proj_trans_generic(projobj, opposite_direction(pj_direction), &pole_x,
                       sizeof(double), 1, &pole_y, sizeof(double), 1, nullptr,
                       sizeof(double), 0, nullptr, sizeof(double), 0);
    if (xmin < pole_x && pole_x < xmax && ymax > pole_y && pole_y > ymin)
        return true;
    return false;
}

// ---------------------------------------------------------------------------
// Check if the target CRS of the transformation
// has the longitude latitude axis order.
// This assumes that the destination CRS is geographic.
static int target_crs_lon_lat_order(PJ_CONTEXT *transformer_ctx,
                                    PJ *transformer_pj,
                                    PJ_DIRECTION pj_direction) {
    PJ *target_crs = nullptr;
    if (pj_direction == PJ_FWD)
        target_crs = proj_get_target_crs(transformer_ctx, transformer_pj);
    else if (pj_direction == PJ_INV)
        target_crs = proj_get_source_crs(transformer_ctx, transformer_pj);
    if (target_crs == nullptr) {
        proj_context_log_debug(transformer_ctx,
                               "Unable to retrieve target CRS");
        return -1;
    }
    PJ *coord_system_pj;
    if (proj_get_type(target_crs) == PJ_TYPE_COMPOUND_CRS) {
        PJ *horiz_crs = proj_crs_get_sub_crs(transformer_ctx, target_crs, 0);
        if (!horiz_crs)
            return -1;
        coord_system_pj =
            proj_crs_get_coordinate_system(transformer_ctx, horiz_crs);
        proj_destroy(horiz_crs);
    } else {
        coord_system_pj =
            proj_crs_get_coordinate_system(transformer_ctx, target_crs);
    }
    proj_destroy(target_crs);
    if (coord_system_pj == nullptr) {
        proj_context_log_debug(transformer_ctx,
                               "Unable to get target CRS coordinate system.");
        return -1;
    }
    const char *abbrev = nullptr;
    int success = proj_cs_get_axis_info(transformer_ctx, coord_system_pj, 0,
                                        nullptr, &abbrev, nullptr, nullptr,
                                        nullptr, nullptr, nullptr);
    proj_destroy(coord_system_pj);
    if (success != 1)
        return -1;
    return strcmp(abbrev, "lon") == 0 || strcmp(abbrev, "Lon") == 0;
}

// ---------------------------------------------------------------------------

/** \brief Transform boundary,
 *
 * Transform boundary densifying the edges to account for nonlinear
 * transformations along these edges and extracting the outermost bounds.
 *
 * If the destination CRS is geographic, the first axis is longitude,
 * and xmax < xmin then the bounds crossed the antimeridian.
 * In this scenario there are two polygons, one on each side of the
 * antimeridian. The first polygon should be constructed with (xmin, ymin, 180,
 * ymax) and the second with (-180, ymin, xmax, ymax).
 *
 * If the destination CRS is geographic, the first axis is latitude,
 * and ymax < ymin then the bounds crossed the antimeridian.
 * In this scenario there are two polygons, one on each side of the
 * antimeridian. The first polygon should be constructed with (ymin, xmin, ymax,
 * 180) and the second with (ymin, -180, ymax, xmax).
 *
 * @param context The PJ_CONTEXT object.
 * @param P The PJ object representing the transformation.
 * @param direction The direction of the transformation.
 * @param xmin Minimum bounding coordinate of the first axis in source CRS
 *             (target CRS if direction is inverse).
 * @param ymin Minimum bounding coordinate of the second axis in source CRS.
 *             (target CRS if direction is inverse).
 * @param xmax Maximum bounding coordinate of the first axis in source CRS.
 *             (target CRS if direction is inverse).
 * @param ymax Maximum bounding coordinate of the second axis in source CRS.
 *             (target CRS if direction is inverse).
 * @param out_xmin Minimum bounding coordinate of the first axis in target CRS
 *             (source CRS if direction is inverse).
 * @param out_ymin Minimum bounding coordinate of the second axis in target CRS.
 *             (source CRS if direction is inverse).
 * @param out_xmax Maximum bounding coordinate of the first axis in target CRS.
 *             (source CRS if direction is inverse).
 * @param out_ymax Maximum bounding coordinate of the second axis in target CRS.
 *             (source CRS if direction is inverse).
 * @param densify_pts Recommended to use 21. This is the number of points
 *     to use to densify the bounding polygon in the transformation.
 * @return an integer. 1 if successful. 0 if failures encountered.
 * @since 8.2
 */
int proj_trans_bounds(PJ_CONTEXT *context, PJ *P, PJ_DIRECTION direction,
                      const double xmin, const double ymin, const double xmax,
                      const double ymax, double *out_xmin, double *out_ymin,
                      double *out_xmax, double *out_ymax,
                      const int densify_pts) {
    *out_xmin = HUGE_VAL;
    *out_ymin = HUGE_VAL;
    *out_xmax = HUGE_VAL;
    *out_ymax = HUGE_VAL;

    if (P == nullptr) {
        proj_log_error(P, _("NULL P object not allowed."));
        proj_errno_set(P, PROJ_ERR_INVALID_OP_ILLEGAL_ARG_VALUE);
        return false;
    }
    if (densify_pts < 0 || densify_pts > 10000) {
        proj_log_error(P, _("densify_pts must be between 0-10000."));
        proj_errno_set(P, PROJ_ERR_INVALID_OP_ILLEGAL_ARG_VALUE);
        return false;
    }

    PJ_PROJ_INFO pj_info = proj_pj_info(P);
    if (pj_info.id == nullptr) {
        proj_log_error(P, _("NULL transformation not allowed,"));
        proj_errno_set(P, PROJ_ERR_INVALID_OP_ILLEGAL_ARG_VALUE);
        return false;
    }
    if (strcmp(pj_info.id, "noop") == 0 || direction == PJ_IDENT) {
        *out_xmin = xmin;
        *out_xmax = xmax;
        *out_ymin = ymin;
        *out_ymax = ymax;
        return true;
    }

    bool degree_output = proj_degree_output(P, direction) != 0;
    bool degree_input = proj_degree_input(P, direction) != 0;
    if (degree_output && densify_pts < 2) {
        proj_log_error(
            P,
            _("densify_pts must be at least 2 if the output is geographic."));
        proj_errno_set(P, PROJ_ERR_INVALID_OP_ILLEGAL_ARG_VALUE);
        return false;
    }

    int side_pts = densify_pts + 1; // add one because we are densifying
    const int boundary_len = side_pts * 4;
    std::vector<double> x_boundary_array;
    std::vector<double> y_boundary_array;
    try {
        x_boundary_array.resize(boundary_len);
        y_boundary_array.resize(boundary_len);
    } catch (const std::exception &e) // memory allocation failure
    {
        proj_log_error(P, e.what());
        proj_errno_set(P, PROJ_ERR_INVALID_OP_ILLEGAL_ARG_VALUE);
        return false;
    }
    double delta_x = 0;
    double delta_y = 0;
    bool north_pole_in_bounds = false;
    bool south_pole_in_bounds = false;
    bool input_lon_lat_order = false;
    bool output_lon_lat_order = false;
    if (degree_input) {
        int in_order_lon_lat =
            target_crs_lon_lat_order(context, P, opposite_direction(direction));
        if (in_order_lon_lat == -1)
            return false;
        input_lon_lat_order = in_order_lon_lat != 0;
    }
    if (degree_output) {
        int out_order_lon_lat = target_crs_lon_lat_order(context, P, direction);
        if (out_order_lon_lat == -1)
            return false;
        output_lon_lat_order = out_order_lon_lat != 0;
        north_pole_in_bounds = contains_north_pole(
            P, direction, xmin, ymin, xmax, ymax, output_lon_lat_order);
        south_pole_in_bounds = contains_south_pole(
            P, direction, xmin, ymin, xmax, ymax, output_lon_lat_order);
    }

    if (degree_input && xmax < xmin) {
        if (!input_lon_lat_order) {
            proj_log_error(P, _("latitude max < latitude min."));
            proj_errno_set(P, PROJ_ERR_INVALID_OP_ILLEGAL_ARG_VALUE);
            return false;
        }
        // handle antimeridian
        delta_x = (xmax - xmin + 360.0) / side_pts;
    } else {
        delta_x = (xmax - xmin) / side_pts;
    }
    if (degree_input && ymax < ymin) {
        if (input_lon_lat_order) {
            proj_log_error(P, _("latitude max < latitude min."));
            proj_errno_set(P, PROJ_ERR_INVALID_OP_ILLEGAL_ARG_VALUE);
            return false;
        }
        // handle antimeridian
        delta_y = (ymax - ymin + 360.0) / side_pts;
    } else {
        delta_y = (ymax - ymin) / side_pts;
    }

    // build densified bounding box
    // Note: must be a linear ring for antimeridian logic
    for (int iii = 0; iii < side_pts; iii++) {
        // xmin boundary
        y_boundary_array[iii] = ymax - iii * delta_y;
        x_boundary_array[iii] = xmin;
        // ymin boundary
        y_boundary_array[iii + side_pts] = ymin;
        x_boundary_array[iii + side_pts] = xmin + iii * delta_x;
        // xmax boundary
        y_boundary_array[iii + side_pts * 2] = ymin + iii * delta_y;
        x_boundary_array[iii + side_pts * 2] = xmax;
        // ymax boundary
        y_boundary_array[iii + side_pts * 3] = ymax;
        x_boundary_array[iii + side_pts * 3] = xmax - iii * delta_x;
    }
    proj_trans_generic(P, direction, &x_boundary_array[0], sizeof(double),
                       boundary_len, &y_boundary_array[0], sizeof(double),
                       boundary_len, nullptr, 0, 0, nullptr, 0, 0);

    if (!degree_output) {
        *out_xmin = simple_min(&x_boundary_array[0], boundary_len);
        *out_xmax = simple_max(&x_boundary_array[0], boundary_len);
        *out_ymin = simple_min(&y_boundary_array[0], boundary_len);
        *out_ymax = simple_max(&y_boundary_array[0], boundary_len);
    } else if (north_pole_in_bounds && output_lon_lat_order) {
        *out_xmin = -180;
        *out_ymin = simple_min(&y_boundary_array[0], boundary_len);
        *out_xmax = 180;
        *out_ymax = 90;
    } else if (north_pole_in_bounds) {
        *out_xmin = simple_min(&x_boundary_array[0], boundary_len);
        *out_ymin = -180;
        *out_xmax = 90;
        *out_ymax = 180;
    } else if (south_pole_in_bounds && output_lon_lat_order) {
        *out_xmin = -180;
        *out_ymin = -90;
        *out_xmax = 180;
        *out_ymax = simple_max(&y_boundary_array[0], boundary_len);
    } else if (south_pole_in_bounds) {
        *out_xmin = -90;
        *out_ymin = -180;
        *out_xmax = simple_max(&x_boundary_array[0], boundary_len);
        *out_ymax = 180;
    } else if (output_lon_lat_order) {
        *out_xmin = antimeridian_min(&x_boundary_array[0], boundary_len);
        *out_xmax = antimeridian_max(&x_boundary_array[0], boundary_len);
        *out_ymin = simple_min(&y_boundary_array[0], boundary_len);
        *out_ymax = simple_max(&y_boundary_array[0], boundary_len);
    } else {
        *out_xmin = simple_min(&x_boundary_array[0], boundary_len);
        *out_xmax = simple_max(&x_boundary_array[0], boundary_len);
        *out_ymin = antimeridian_min(&y_boundary_array[0], boundary_len);
        *out_ymax = antimeridian_max(&y_boundary_array[0], boundary_len);
    }
    return true;
}

/*****************************************************************************/
static void reproject_bbox(PJ *pjGeogToCrs, double west_lon, double south_lat,
                           double east_lon, double north_lat, double &minx,
                           double &miny, double &maxx, double &maxy) {
    /*****************************************************************************/

    minx = -std::numeric_limits<double>::max();
    miny = -std::numeric_limits<double>::max();
    maxx = std::numeric_limits<double>::max();
    maxy = std::numeric_limits<double>::max();

    if (!(west_lon == -180.0 && east_lon == 180.0 && south_lat == -90.0 &&
          north_lat == 90.0)) {
        minx = -minx;
        miny = -miny;
        maxx = -maxx;
        maxy = -maxy;

        constexpr int N_STEPS = 20;
        constexpr int N_STEPS_P1 = N_STEPS + 1;
        constexpr int XY_SIZE = N_STEPS_P1 * 4;
        std::vector<double> x(XY_SIZE);
        std::vector<double> y(XY_SIZE);
        const double step_lon = (east_lon - west_lon) / N_STEPS;
        const double step_lat = (north_lat - south_lat) / N_STEPS;
        for (int j = 0; j <= N_STEPS; j++) {
            x[j] = west_lon + j * step_lon;
            y[j] = south_lat;
            x[N_STEPS_P1 + j] = x[j];
            y[N_STEPS_P1 + j] = north_lat;
            x[N_STEPS_P1 * 2 + j] = west_lon;
            y[N_STEPS_P1 * 2 + j] = south_lat + j * step_lat;
            x[N_STEPS_P1 * 3 + j] = east_lon;
            y[N_STEPS_P1 * 3 + j] = y[N_STEPS_P1 * 2 + j];
        }
        proj_trans_generic(pjGeogToCrs, PJ_FWD, &x[0], sizeof(double), XY_SIZE,
                           &y[0], sizeof(double), XY_SIZE, nullptr, 0, 0,
                           nullptr, 0, 0);
        for (int j = 0; j < XY_SIZE; j++) {
            if (x[j] != HUGE_VAL && y[j] != HUGE_VAL) {
                minx = std::min(minx, x[j]);
                miny = std::min(miny, y[j]);
                maxx = std::max(maxx, x[j]);
                maxy = std::max(maxy, y[j]);
            }
        }
    }
}

/*****************************************************************************/
static PJ *add_coord_op_to_list(
    int idxInOriginalList, PJ *op, double west_lon, double south_lat,
    double east_lon, double north_lat, PJ *pjGeogToSrc, PJ *pjGeogToDst,
    const PJ *pjSrcGeocentricToLonLat, const PJ *pjDstGeocentricToLonLat,
    const char *areaName, std::vector<PJCoordOperation> &altCoordOps) {
    /*****************************************************************************/

    double minxSrc;
    double minySrc;
    double maxxSrc;
    double maxySrc;
    double minxDst;
    double minyDst;
    double maxxDst;
    double maxyDst;

    double w = west_lon / 180 * M_PI;
    double s = south_lat / 180 * M_PI;
    double e = east_lon / 180 * M_PI;
    double n = north_lat / 180 * M_PI;
    if (w > e) {
        e += 2 * M_PI;
    }
    // Integrate cos(lat) between south_lat and north_lat
    const double pseudoArea = (e - w) * (std::sin(n) - std::sin(s));

    if (pjSrcGeocentricToLonLat) {
        minxSrc = west_lon;
        minySrc = south_lat;
        maxxSrc = east_lon;
        maxySrc = north_lat;
    } else {
        reproject_bbox(pjGeogToSrc, west_lon, south_lat, east_lon, north_lat,
                       minxSrc, minySrc, maxxSrc, maxySrc);
    }

    if (pjDstGeocentricToLonLat) {
        minxDst = west_lon;
        minyDst = south_lat;
        maxxDst = east_lon;
        maxyDst = north_lat;
    } else {
        reproject_bbox(pjGeogToDst, west_lon, south_lat, east_lon, north_lat,
                       minxDst, minyDst, maxxDst, maxyDst);
    }

    if (minxSrc <= maxxSrc && minxDst <= maxxDst) {
        const char *c_name = proj_get_name(op);
        std::string name(c_name ? c_name : "");

        const double accuracy = proj_coordoperation_get_accuracy(op->ctx, op);
        altCoordOps.emplace_back(
            idxInOriginalList, minxSrc, minySrc, maxxSrc, maxySrc, minxDst,
            minyDst, maxxDst, maxyDst, op, name, accuracy, pseudoArea, areaName,
            pjSrcGeocentricToLonLat, pjDstGeocentricToLonLat);
        op = nullptr;
    }
    return op;
}

namespace {
struct ObjectKeeper {
    PJ *m_obj = nullptr;
    explicit ObjectKeeper(PJ *obj) : m_obj(obj) {}
    ~ObjectKeeper() { proj_destroy(m_obj); }
    ObjectKeeper(const ObjectKeeper &) = delete;
    ObjectKeeper &operator=(const ObjectKeeper &) = delete;
};
} // namespace

/*****************************************************************************/
static PJ *create_operation_to_geog_crs(PJ_CONTEXT *ctx, const PJ *crs) {
    /*****************************************************************************/

    std::unique_ptr<ObjectKeeper> keeper;
    if (proj_get_type(crs) == PJ_TYPE_COORDINATE_METADATA) {
        auto tmp = proj_get_source_crs(ctx, crs);
        assert(tmp);
        keeper.reset(new ObjectKeeper(tmp));
        crs = tmp;
    }
    (void)keeper;

    // Create a geographic 2D long-lat degrees CRS that is related to the
    // CRS
    auto geodetic_crs = proj_crs_get_geodetic_crs(ctx, crs);
    if (!geodetic_crs) {
        proj_context_log_debug(ctx, "Cannot find geodetic CRS matching CRS");
        return nullptr;
    }

    auto geodetic_crs_type = proj_get_type(geodetic_crs);
    if (geodetic_crs_type == PJ_TYPE_GEOCENTRIC_CRS ||
        geodetic_crs_type == PJ_TYPE_GEOGRAPHIC_2D_CRS ||
        geodetic_crs_type == PJ_TYPE_GEOGRAPHIC_3D_CRS) {
        auto datum = proj_crs_get_datum_forced(ctx, geodetic_crs);
        assert(datum);
        auto cs = proj_create_ellipsoidal_2D_cs(
            ctx, PJ_ELLPS2D_LONGITUDE_LATITUDE, nullptr, 0);
        auto ellps = proj_get_ellipsoid(ctx, datum);
        proj_destroy(datum);
        double semi_major_metre = 0;
        double inv_flattening = 0;
        proj_ellipsoid_get_parameters(ctx, ellps, &semi_major_metre, nullptr,
                                      nullptr, &inv_flattening);
        // It is critical to set the prime meridian to 0
        auto temp = proj_create_geographic_crs(
            ctx, "unnamed crs", "unnamed datum", proj_get_name(ellps),
            semi_major_metre, inv_flattening, "Reference prime meridian", 0,
            nullptr, 0, cs);
        proj_destroy(ellps);
        proj_destroy(cs);
        proj_destroy(geodetic_crs);
        geodetic_crs = temp;
        geodetic_crs_type = proj_get_type(geodetic_crs);
    }
    if (geodetic_crs_type != PJ_TYPE_GEOGRAPHIC_2D_CRS) {
        // Shouldn't happen
        proj_context_log_debug(ctx, "Cannot find geographic CRS matching CRS");
        proj_destroy(geodetic_crs);
        return nullptr;
    }

    // Create the transformation from this geographic 2D CRS to the source CRS
    auto operation_ctx = proj_create_operation_factory_context(ctx, nullptr);
    proj_operation_factory_context_set_spatial_criterion(
        ctx, operation_ctx, PROJ_SPATIAL_CRITERION_PARTIAL_INTERSECTION);
    proj_operation_factory_context_set_grid_availability_use(
        ctx, operation_ctx,
        PROJ_GRID_AVAILABILITY_DISCARD_OPERATION_IF_MISSING_GRID);
    auto target_crs_2D = proj_crs_demote_to_2D(ctx, nullptr, crs);
    auto op_list_to_geodetic =
        proj_create_operations(ctx, geodetic_crs, target_crs_2D, operation_ctx);
    proj_destroy(target_crs_2D);
    proj_operation_factory_context_destroy(operation_ctx);
    proj_destroy(geodetic_crs);

    const int nOpCount = op_list_to_geodetic == nullptr
                             ? 0
                             : proj_list_get_count(op_list_to_geodetic);
    if (nOpCount == 0) {
        proj_context_log_debug(
            ctx, "Cannot compute transformation from geographic CRS to CRS");
        proj_list_destroy(op_list_to_geodetic);
        return nullptr;
    }
    PJ *opGeogToCrs = nullptr;
    // Use in priority operations *without* grids
    for (int i = 0; i < nOpCount; i++) {
        auto op = proj_list_get(ctx, op_list_to_geodetic, i);
        assert(op);
        if (proj_coordoperation_get_grid_used_count(ctx, op) == 0) {
            opGeogToCrs = op;
            break;
        }
        proj_destroy(op);
    }
    if (opGeogToCrs == nullptr) {
        opGeogToCrs = proj_list_get(ctx, op_list_to_geodetic, 0);
        assert(opGeogToCrs);
    }
    proj_list_destroy(op_list_to_geodetic);
    return opGeogToCrs;
}

/*****************************************************************************/
static PJ *create_operation_geocentric_crs_to_geog_crs(PJ_CONTEXT *ctx,
                                                       const PJ *geocentric_crs)
/*****************************************************************************/
{
    assert(proj_get_type(geocentric_crs) == PJ_TYPE_GEOCENTRIC_CRS);

    auto datum = proj_crs_get_datum_forced(ctx, geocentric_crs);
    assert(datum);
    auto cs = proj_create_ellipsoidal_2D_cs(ctx, PJ_ELLPS2D_LONGITUDE_LATITUDE,
                                            nullptr, 0);
    auto ellps = proj_get_ellipsoid(ctx, datum);
    proj_destroy(datum);
    double semi_major_metre = 0;
    double inv_flattening = 0;
    proj_ellipsoid_get_parameters(ctx, ellps, &semi_major_metre, nullptr,
                                  nullptr, &inv_flattening);
    // It is critical to set the prime meridian to 0
    auto lon_lat_crs = proj_create_geographic_crs(
        ctx, "unnamed crs", "unnamed datum", proj_get_name(ellps),
        semi_major_metre, inv_flattening, "Reference prime meridian", 0,
        nullptr, 0, cs);
    proj_destroy(ellps);
    proj_destroy(cs);

    // Create the transformation from this geocentric CRS to the lon-lat one
    auto operation_ctx = proj_create_operation_factory_context(ctx, nullptr);
    proj_operation_factory_context_set_spatial_criterion(
        ctx, operation_ctx, PROJ_SPATIAL_CRITERION_PARTIAL_INTERSECTION);
    proj_operation_factory_context_set_grid_availability_use(
        ctx, operation_ctx,
        PROJ_GRID_AVAILABILITY_DISCARD_OPERATION_IF_MISSING_GRID);
    auto op_list =
        proj_create_operations(ctx, geocentric_crs, lon_lat_crs, operation_ctx);
    proj_operation_factory_context_destroy(operation_ctx);
    proj_destroy(lon_lat_crs);

    const int nOpCount = op_list == nullptr ? 0 : proj_list_get_count(op_list);
    if (nOpCount != 1) {
        proj_context_log_debug(ctx, "Cannot compute transformation from "
                                    "geocentric CRS to geographic CRS");
        proj_list_destroy(op_list);
        return nullptr;
    }

    auto op = proj_list_get(ctx, op_list, 0);
    assert(op);
    proj_list_destroy(op_list);
    return op;
}

/*****************************************************************************/
PJ *proj_create_crs_to_crs(PJ_CONTEXT *ctx, const char *source_crs,
                           const char *target_crs, PJ_AREA *area) {
    /******************************************************************************
        Create a transformation pipeline between two known coordinate reference
        systems.

        See docs/source/development/reference/functions.rst

    ******************************************************************************/
    if (!ctx) {
        ctx = pj_get_default_ctx();
    }

    PJ *src;
    PJ *dst;
    try {
        std::string source_crs_modified(pj_add_type_crs_if_needed(source_crs));
        std::string target_crs_modified(pj_add_type_crs_if_needed(target_crs));

        src = proj_create(ctx, source_crs_modified.c_str());
        if (!src) {
            proj_context_log_debug(ctx, "Cannot instantiate source_crs");
            return nullptr;
        }

        dst = proj_create(ctx, target_crs_modified.c_str());
        if (!dst) {
            proj_context_log_debug(ctx, "Cannot instantiate target_crs");
            proj_destroy(src);
            return nullptr;
        }
    } catch (const std::exception &) {
        return nullptr;
    }

    auto ret = proj_create_crs_to_crs_from_pj(ctx, src, dst, area, nullptr);
    proj_destroy(src);
    proj_destroy(dst);
    return ret;
}

/*****************************************************************************/
std::vector<PJCoordOperation>
pj_create_prepared_operations(PJ_CONTEXT *ctx, const PJ *source_crs,
                              const PJ *target_crs, PJ_OBJ_LIST *op_list)
/*****************************************************************************/
{
    PJ *pjGeogToSrc = nullptr;
    PJ *pjSrcGeocentricToLonLat = nullptr;
    if (proj_get_type(source_crs) == PJ_TYPE_GEOCENTRIC_CRS) {
        pjSrcGeocentricToLonLat =
            create_operation_geocentric_crs_to_geog_crs(ctx, source_crs);
        if (!pjSrcGeocentricToLonLat) {
            // shouldn't happen
            return {};
        }
    } else {
        pjGeogToSrc = create_operation_to_geog_crs(ctx, source_crs);
        if (!pjGeogToSrc) {
            proj_context_log_debug(
                ctx, "Cannot create transformation from geographic "
                     "CRS of source CRS to source CRS");
            return {};
        }
    }

    PJ *pjGeogToDst = nullptr;
    PJ *pjDstGeocentricToLonLat = nullptr;
    if (proj_get_type(target_crs) == PJ_TYPE_GEOCENTRIC_CRS) {
        pjDstGeocentricToLonLat =
            create_operation_geocentric_crs_to_geog_crs(ctx, target_crs);
        if (!pjDstGeocentricToLonLat) {
            // shouldn't happen
            proj_destroy(pjSrcGeocentricToLonLat);
            proj_destroy(pjGeogToSrc);
            return {};
        }
    } else {
        pjGeogToDst = create_operation_to_geog_crs(ctx, target_crs);
        if (!pjGeogToDst) {
            proj_context_log_debug(
                ctx, "Cannot create transformation from geographic "
                     "CRS of target CRS to target CRS");
            proj_destroy(pjSrcGeocentricToLonLat);
            proj_destroy(pjGeogToSrc);
            return {};
        }
    }

    try {
        std::vector<PJCoordOperation> preparedOpList;

        // Iterate over source->target candidate transformations and reproject
        // their long-lat bounding box into the source CRS.
        const auto op_count = proj_list_get_count(op_list);
        for (int i = 0; i < op_count; i++) {
            auto op = proj_list_get(ctx, op_list, i);
            assert(op);
            double west_lon = 0.0;
            double south_lat = 0.0;
            double east_lon = 0.0;
            double north_lat = 0.0;

            const char *areaName = nullptr;
            if (!proj_get_area_of_use(ctx, op, &west_lon, &south_lat, &east_lon,
                                      &north_lat, &areaName)) {
                west_lon = -180;
                south_lat = -90;
                east_lon = 180;
                north_lat = 90;
            }

            if (west_lon <= east_lon) {
                op = add_coord_op_to_list(
                    i, op, west_lon, south_lat, east_lon, north_lat,
                    pjGeogToSrc, pjGeogToDst, pjSrcGeocentricToLonLat,
                    pjDstGeocentricToLonLat, areaName, preparedOpList);
            } else {
                auto op_clone = proj_clone(ctx, op);

                op = add_coord_op_to_list(
                    i, op, west_lon, south_lat, 180, north_lat, pjGeogToSrc,
                    pjGeogToDst, pjSrcGeocentricToLonLat,
                    pjDstGeocentricToLonLat, areaName, preparedOpList);
                op_clone = add_coord_op_to_list(
                    i, op_clone, -180, south_lat, east_lon, north_lat,
                    pjGeogToSrc, pjGeogToDst, pjSrcGeocentricToLonLat,
                    pjDstGeocentricToLonLat, areaName, preparedOpList);
                proj_destroy(op_clone);
            }

            proj_destroy(op);
        }

        proj_destroy(pjGeogToSrc);
        proj_destroy(pjGeogToDst);
        proj_destroy(pjSrcGeocentricToLonLat);
        proj_destroy(pjDstGeocentricToLonLat);
        return preparedOpList;
    } catch (const std::exception &) {
        proj_destroy(pjGeogToSrc);
        proj_destroy(pjGeogToDst);
        proj_destroy(pjSrcGeocentricToLonLat);
        proj_destroy(pjDstGeocentricToLonLat);
        return {};
    }
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
static const char *getOptionValue(const char *option,
                                  const char *keyWithEqual) noexcept {
    if (ci_starts_with(option, keyWithEqual)) {
        return option + strlen(keyWithEqual);
    }
    return nullptr;
}
//! @endcond

/*****************************************************************************/
PJ *proj_create_crs_to_crs_from_pj(PJ_CONTEXT *ctx, const PJ *source_crs,
                                   const PJ *target_crs, PJ_AREA *area,
                                   const char *const *options) {
    /******************************************************************************
        Create a transformation pipeline between two known coordinate reference
        systems.

        See docs/source/development/reference/functions.rst

    ******************************************************************************/
    if (!ctx) {
        ctx = pj_get_default_ctx();
    }
    pj_load_ini(
        ctx); // to set ctx->errorIfBestTransformationNotAvailableDefault

    const char *authority = nullptr;
    double accuracy = -1;
    bool allowBallparkTransformations = true;
    bool forceOver = false;
    bool warnIfBestTransformationNotAvailable =
        ctx->warnIfBestTransformationNotAvailableDefault;
    bool errorIfBestTransformationNotAvailable =
        ctx->errorIfBestTransformationNotAvailableDefault;
    for (auto iter = options; iter && iter[0]; ++iter) {
        const char *value;
        if ((value = getOptionValue(*iter, "AUTHORITY="))) {
            authority = value;
        } else if ((value = getOptionValue(*iter, "ACCURACY="))) {
            accuracy = pj_atof(value);
        } else if ((value = getOptionValue(*iter, "ALLOW_BALLPARK="))) {
            if (ci_equal(value, "yes"))
                allowBallparkTransformations = true;
            else if (ci_equal(value, "no"))
                allowBallparkTransformations = false;
            else {
                ctx->logger(ctx->logger_app_data, PJ_LOG_ERROR,
                            "Invalid value for ALLOW_BALLPARK option.");
                return nullptr;
            }
        } else if ((value = getOptionValue(*iter, "ONLY_BEST="))) {
            warnIfBestTransformationNotAvailable = false;
            if (ci_equal(value, "yes"))
                errorIfBestTransformationNotAvailable = true;
            else if (ci_equal(value, "no"))
                errorIfBestTransformationNotAvailable = false;
            else {
                ctx->logger(ctx->logger_app_data, PJ_LOG_ERROR,
                            "Invalid value for ONLY_BEST option.");
                return nullptr;
            }
        } else if ((value = getOptionValue(*iter, "FORCE_OVER="))) {
            if (ci_equal(value, "yes")) {
                forceOver = true;
            }
        } else {
            std::string msg("Unknown option :");
            msg += *iter;
            ctx->logger(ctx->logger_app_data, PJ_LOG_ERROR, msg.c_str());
            return nullptr;
        }
    }

    auto operation_ctx = proj_create_operation_factory_context(ctx, authority);
    if (!operation_ctx) {
        return nullptr;
    }

    proj_operation_factory_context_set_allow_ballpark_transformations(
        ctx, operation_ctx, allowBallparkTransformations);

    if (accuracy >= 0) {
        proj_operation_factory_context_set_desired_accuracy(ctx, operation_ctx,
                                                            accuracy);
    }

    if (area && area->bbox_set) {
        proj_operation_factory_context_set_area_of_interest(
            ctx, operation_ctx, area->west_lon_degree, area->south_lat_degree,
            area->east_lon_degree, area->north_lat_degree);

        if (!area->name.empty()) {
            proj_operation_factory_context_set_area_of_interest_name(
                ctx, operation_ctx, area->name.c_str());
        }
    }

    proj_operation_factory_context_set_spatial_criterion(
        ctx, operation_ctx, PROJ_SPATIAL_CRITERION_PARTIAL_INTERSECTION);
    proj_operation_factory_context_set_grid_availability_use(
        ctx, operation_ctx,
        (errorIfBestTransformationNotAvailable ||
         warnIfBestTransformationNotAvailable ||
         proj_context_is_network_enabled(ctx))
            ? PROJ_GRID_AVAILABILITY_KNOWN_AVAILABLE
            : PROJ_GRID_AVAILABILITY_DISCARD_OPERATION_IF_MISSING_GRID);

    auto op_list =
        proj_create_operations(ctx, source_crs, target_crs, operation_ctx);
    proj_operation_factory_context_destroy(operation_ctx);

    if (!op_list) {
        return nullptr;
    }

    auto op_count = proj_list_get_count(op_list);
    if (op_count == 0) {
        proj_list_destroy(op_list);

        proj_context_log_debug(ctx, "No operation found matching criteria");
        return nullptr;
    }

    ctx->forceOver = forceOver;

    const int old_debug_level = ctx->debug_level;
    if (errorIfBestTransformationNotAvailable ||
        warnIfBestTransformationNotAvailable)
        ctx->debug_level = PJ_LOG_NONE;
    PJ *P = proj_list_get(ctx, op_list, 0);
    ctx->debug_level = old_debug_level;
    assert(P);

    if (P != nullptr) {
        P->errorIfBestTransformationNotAvailable =
            errorIfBestTransformationNotAvailable;
        P->warnIfBestTransformationNotAvailable =
            warnIfBestTransformationNotAvailable;
        P->skipNonInstantiable = warnIfBestTransformationNotAvailable;
    }

    const bool mayNeedToReRunWithDiscardMissing =
        (errorIfBestTransformationNotAvailable ||
         warnIfBestTransformationNotAvailable) &&
        !proj_context_is_network_enabled(ctx);
    int singleOpIsInstanciable = -1;
    if (P != nullptr && op_count == 1 && mayNeedToReRunWithDiscardMissing) {
        singleOpIsInstanciable = proj_coordoperation_is_instantiable(ctx, P);
    }

    const auto backup_errno = proj_context_errno(ctx);
    if (P == nullptr ||
        (op_count == 1 && (!mayNeedToReRunWithDiscardMissing ||
                           errorIfBestTransformationNotAvailable ||
                           singleOpIsInstanciable == static_cast<int>(true)))) {
        proj_list_destroy(op_list);
        ctx->forceOver = false;

        if (P != nullptr && (errorIfBestTransformationNotAvailable ||
                             warnIfBestTransformationNotAvailable)) {
            if (singleOpIsInstanciable < 0) {
                singleOpIsInstanciable =
                    proj_coordoperation_is_instantiable(ctx, P);
            }
            if (!singleOpIsInstanciable) {
                warnAboutMissingGrid(P);
                if (errorIfBestTransformationNotAvailable) {
                    proj_destroy(P);
                    return nullptr;
                }
            }
        }

        if (P != nullptr) {
            P->over = forceOver;
        }
        return P;
    } else if (op_count == 1 && mayNeedToReRunWithDiscardMissing &&
               !singleOpIsInstanciable) {
        warnAboutMissingGrid(P);
    }

    if (errorIfBestTransformationNotAvailable ||
        warnIfBestTransformationNotAvailable)
        ctx->debug_level = PJ_LOG_NONE;
    auto preparedOpList =
        pj_create_prepared_operations(ctx, source_crs, target_crs, op_list);
    ctx->debug_level = old_debug_level;

    ctx->forceOver = false;
    proj_list_destroy(op_list);

    if (preparedOpList.empty()) {
        proj_destroy(P);
        return nullptr;
    }

    bool foundInstanciableAndNonBallpark = false;

    for (auto &op : preparedOpList) {
        op.pj->over = forceOver;
        op.pj->errorIfBestTransformationNotAvailable =
            errorIfBestTransformationNotAvailable;
        op.pj->warnIfBestTransformationNotAvailable =
            warnIfBestTransformationNotAvailable;
        if (mayNeedToReRunWithDiscardMissing &&
            !foundInstanciableAndNonBallpark) {
            if (!proj_coordoperation_has_ballpark_transformation(op.pj->ctx,
                                                                 op.pj) &&
                op.isInstantiable()) {
                foundInstanciableAndNonBallpark = true;
            }
        }
    }
    if (mayNeedToReRunWithDiscardMissing && !foundInstanciableAndNonBallpark) {
        // Re-run proj_create_operations with
        // PROJ_GRID_AVAILABILITY_DISCARD_OPERATION_IF_MISSING_GRID
        // Can happen for example for NAD27->NAD83 transformation when we
        // have no grid and thus have fallback to Helmert transformation and
        // a WGS84 intermediate.
        operation_ctx = proj_create_operation_factory_context(ctx, authority);
        if (operation_ctx) {
            proj_operation_factory_context_set_allow_ballpark_transformations(
                ctx, operation_ctx, allowBallparkTransformations);

            if (accuracy >= 0) {
                proj_operation_factory_context_set_desired_accuracy(
                    ctx, operation_ctx, accuracy);
            }

            if (area && area->bbox_set) {
                proj_operation_factory_context_set_area_of_interest(
                    ctx, operation_ctx, area->west_lon_degree,
                    area->south_lat_degree, area->east_lon_degree,
                    area->north_lat_degree);

                if (!area->name.empty()) {
                    proj_operation_factory_context_set_area_of_interest_name(
                        ctx, operation_ctx, area->name.c_str());
                }
            }

            proj_operation_factory_context_set_spatial_criterion(
                ctx, operation_ctx,
                PROJ_SPATIAL_CRITERION_PARTIAL_INTERSECTION);
            proj_operation_factory_context_set_grid_availability_use(
                ctx, operation_ctx,
                PROJ_GRID_AVAILABILITY_DISCARD_OPERATION_IF_MISSING_GRID);

            op_list = proj_create_operations(ctx, source_crs, target_crs,
                                             operation_ctx);
            proj_operation_factory_context_destroy(operation_ctx);

            if (op_list) {
                ctx->forceOver = forceOver;
                ctx->debug_level = PJ_LOG_NONE;
                auto preparedOpList2 = pj_create_prepared_operations(
                    ctx, source_crs, target_crs, op_list);
                ctx->debug_level = old_debug_level;
                ctx->forceOver = false;
                proj_list_destroy(op_list);

                if (!preparedOpList2.empty()) {
                    // Append new lists of operations to previous one
                    std::vector<PJCoordOperation> newOpList;
                    for (auto &&op : preparedOpList) {
                        if (!proj_coordoperation_has_ballpark_transformation(
                                op.pj->ctx, op.pj)) {
                            newOpList.emplace_back(std::move(op));
                        }
                    }
                    for (auto &&op : preparedOpList2) {
                        op.pj->over = forceOver;
                        op.pj->errorIfBestTransformationNotAvailable =
                            errorIfBestTransformationNotAvailable;
                        op.pj->warnIfBestTransformationNotAvailable =
                            warnIfBestTransformationNotAvailable;
                        newOpList.emplace_back(std::move(op));
                    }
                    preparedOpList = std::move(newOpList);
                } else {
                    // We get there in "cs2cs --only-best --no-ballpark
                    // EPSG:4326+3855 EPSG:4979" use case, where the initial
                    // create_operations returned 1 operation, and the retry
                    // with
                    // PROJ_GRID_AVAILABILITY_DISCARD_OPERATION_IF_MISSING_GRID
                    // returned 0.
                    if (op_count == 1 &&
                        errorIfBestTransformationNotAvailable) {
                        if (singleOpIsInstanciable < 0) {
                            singleOpIsInstanciable =
                                proj_coordoperation_is_instantiable(ctx, P);
                        }
                        if (!singleOpIsInstanciable) {
                            proj_destroy(P);
                            proj_context_errno_set(ctx, backup_errno);
                            return nullptr;
                        }
                    }
                }
            }
        }
    }

    // If there's finally juste a single result, return it directly
    if (preparedOpList.size() == 1) {
        auto retP = preparedOpList[0].pj;
        preparedOpList[0].pj = nullptr;
        proj_destroy(P);
        return retP;
    }

    P->alternativeCoordinateOperations = std::move(preparedOpList);
    // The returned P is rather dummy
    P->descr = "Set of coordinate operations";
    P->over = forceOver;
    P->iso_obj = nullptr;
    P->fwd = nullptr;
    P->inv = nullptr;
    P->fwd3d = nullptr;
    P->inv3d = nullptr;
    P->fwd4d = nullptr;
    P->inv4d = nullptr;

    return P;
}

/*****************************************************************************/
int proj_errno(const PJ *P) {
    /******************************************************************************
        Read an error level from the context of a PJ.
    ******************************************************************************/
    return proj_context_errno(pj_get_ctx((PJ *)P));
}

/*****************************************************************************/
int proj_context_errno(PJ_CONTEXT *ctx) {
    /******************************************************************************
        Read an error directly from a context, without going through a PJ
        belonging to that context.
    ******************************************************************************/
    if (nullptr == ctx)
        ctx = pj_get_default_ctx();
    return ctx->last_errno;
}

/*****************************************************************************/
int proj_errno_set(const PJ *P, int err) {
    /******************************************************************************
        Set context-errno, bubble it up to the thread local errno, return err
    ******************************************************************************/
    /* Use proj_errno_reset to explicitly clear the error status */
    if (0 == err)
        return 0;

    /* For P==0 err goes to the default context */
    proj_context_errno_set(pj_get_ctx((PJ *)P), err);
    errno = err;

    return err;
}

/*****************************************************************************/
int proj_errno_restore(const PJ *P, int err) {
    /******************************************************************************
        Use proj_errno_restore when the current function succeeds, but the
        error flag was set on entry, and stored/reset using proj_errno_reset
        in order to monitor for new errors.

        See usage example under proj_errno_reset ()
    ******************************************************************************/
    if (0 == err)
        return 0;
    proj_errno_set(P, err);
    return 0;
}

/*****************************************************************************/
int proj_errno_reset(const PJ *P) {
    /******************************************************************************
        Clears errno in the context and thread local levels
        through the low level pj_ctx interface.

        Returns the previous value of the errno, for convenient reset/restore
        operations:

        int foo (PJ *P) {
            // errno may be set on entry, but we need to reset it to be able to
            // check for errors from "do_something_with_P(P)"
            int last_errno = proj_errno_reset (P);

            // local failure
            if (0==P)
                return proj_errno_set (P, 42);

            // call to function that may fail
            do_something_with_P (P);

            // failure in do_something_with_P? - keep latest error status
            if (proj_errno(P))
                return proj_errno (P);

            // success - restore previous error status, return 0
            return proj_errno_restore (P, last_errno);
        }
    ******************************************************************************/
    int last_errno;
    last_errno = proj_errno(P);

    proj_context_errno_set(pj_get_ctx((PJ *)P), 0);
    errno = 0;
    return last_errno;
}

/* Create a new context based on the default context */
PJ_CONTEXT *proj_context_create(void) {
    return new (std::nothrow) pj_ctx(*pj_get_default_ctx());
}

PJ_CONTEXT *proj_context_destroy(PJ_CONTEXT *ctx) {
    if (nullptr == ctx)
        return nullptr;

    /* Trying to free the default context is a no-op (since it is statically
     * allocated) */
    if (pj_get_default_ctx() == ctx)
        return nullptr;

    delete ctx;
    return nullptr;
}

/*****************************************************************************/
static char *path_append(char *buf, const char *app, size_t *buf_size) {
    /******************************************************************************
        Helper for proj_info() below. Append app to buf, separated by a
        semicolon. Also handle allocation of longer buffer if needed.

        Returns buffer and adjusts *buf_size through provided pointer arg.
    ******************************************************************************/
    char *p;
    size_t len, applen = 0, buflen = 0;
#ifdef _WIN32
    const char *delim = ";";
#else
    const char *delim = ":";
#endif

    /* Nothing to do? */
    if (nullptr == app)
        return buf;
    applen = strlen(app);
    if (0 == applen)
        return buf;

    /* Start checking whether buf is long enough */
    if (nullptr != buf)
        buflen = strlen(buf);
    len = buflen + applen + strlen(delim) + 1;

    /* "pj_realloc", so to speak */
    if (*buf_size < len) {
        p = static_cast<char *>(calloc(2 * len, sizeof(char)));
        if (nullptr == p) {
            free(buf);
            return nullptr;
        }
        *buf_size = 2 * len;
        if (buf != nullptr)
            strcpy(p, buf);
        free(buf);
        buf = p;
    }
    assert(buf);

    /* Only append a delimiter if something's already there */
    if (0 != buflen)
        strcat(buf, delim);
    strcat(buf, app);
    return buf;
}

static const char *empty = {""};
static PJ_INFO info = {0, 0, 0, nullptr, nullptr, nullptr, nullptr, 0};

/*****************************************************************************/
PJ_INFO proj_info(void) {
    /******************************************************************************
        Basic info about the current instance of the PROJ.4 library.

        Returns PJ_INFO struct.
    ******************************************************************************/
    size_t buf_size = 0;
    char *buf = nullptr;

    pj_acquire_lock();

    info.major = PROJ_VERSION_MAJOR;
    info.minor = PROJ_VERSION_MINOR;
    info.patch = PROJ_VERSION_PATCH;

    info.version = pj_get_version();
    info.release = pj_get_release();

    /* build search path string */
    auto ctx = pj_get_default_ctx();
    if (ctx->search_paths.empty()) {
        const auto searchpaths = pj_get_default_searchpaths(ctx);
        for (const auto &path : searchpaths) {
            buf = path_append(buf, path.c_str(), &buf_size);
        }
    } else {
        for (const auto &path : ctx->search_paths) {
            buf = path_append(buf, path.c_str(), &buf_size);
        }
    }

    if (info.searchpath != empty)
        free(const_cast<char *>(info.searchpath));
    info.searchpath = buf ? buf : empty;

    info.paths = ctx->c_compat_paths;
    info.path_count = static_cast<int>(ctx->search_paths.size());

    pj_release_lock();
    return info;
}

/*****************************************************************************/
PJ_PROJ_INFO proj_pj_info(PJ *P) {
    /******************************************************************************
        Basic info about a particular instance of a projection object.

        Returns PJ_PROJ_INFO struct.
    ******************************************************************************/
    PJ_PROJ_INFO pjinfo;
    char *def;

    memset(&pjinfo, 0, sizeof(PJ_PROJ_INFO));

    pjinfo.accuracy = -1.0;

    if (nullptr == P)
        return pjinfo;

    /* coordinate operation description */
    if (!P->alternativeCoordinateOperations.empty()) {
        if (P->iCurCoordOp >= 0) {
            P = P->alternativeCoordinateOperations[P->iCurCoordOp].pj;
        } else {
            PJ *candidateOp = nullptr;
            // If there's just a single coordinate operation which is
            // instantiable, use it.
            for (const auto &op : P->alternativeCoordinateOperations) {
                if (op.isInstantiable()) {
                    if (candidateOp == nullptr) {
                        candidateOp = op.pj;
                    } else {
                        candidateOp = nullptr;
                        break;
                    }
                }
            }
            if (candidateOp) {
                P = candidateOp;
            } else {
                pjinfo.id = "unknown";
                pjinfo.description = "unavailable until proj_trans is called";
                pjinfo.definition = "unavailable until proj_trans is called";
                return pjinfo;
            }
        }
    }

    /* projection id */
    if (pj_param(P->ctx, P->params, "tproj").i)
        pjinfo.id = pj_param(P->ctx, P->params, "sproj").s;

    pjinfo.description = P->descr;
    if (P->iso_obj) {
        auto identifiedObj =
            dynamic_cast<NS_PROJ::common::IdentifiedObject *>(P->iso_obj.get());
        if (identifiedObj) {
            pjinfo.description = identifiedObj->nameStr().c_str();
        }
    }

    // accuracy
    if (P->iso_obj) {
        auto conv = dynamic_cast<const NS_PROJ::operation::Conversion *>(
            P->iso_obj.get());
        if (conv) {
            pjinfo.accuracy = 0.0;
        } else {
            auto op =
                dynamic_cast<const NS_PROJ::operation::CoordinateOperation *>(
                    P->iso_obj.get());
            if (op) {
                const auto &accuracies = op->coordinateOperationAccuracies();
                if (!accuracies.empty()) {
                    try {
                        pjinfo.accuracy = std::stod(accuracies[0]->value());
                    } catch (const std::exception &) {
                    }
                }
            }
        }
    }

    /* projection definition */
    if (P->def_full)
        def = P->def_full;
    else
        def = pj_get_def(P, 0); /* pj_get_def takes a non-const PJ pointer */
    if (nullptr == def)
        pjinfo.definition = empty;
    else
        pjinfo.definition = pj_shrink(def);
    /* Make proj_destroy clean this up eventually */
    P->def_full = def;

    pjinfo.has_inverse = pj_has_inverse(P);
    return pjinfo;
}

/*****************************************************************************/
PJ_GRID_INFO proj_grid_info(const char *gridname) {
    /******************************************************************************
        Information about a named datum grid.

        Returns PJ_GRID_INFO struct.
    ******************************************************************************/
    PJ_GRID_INFO grinfo;

    /*PJ_CONTEXT *ctx = proj_context_create(); */
    PJ_CONTEXT *ctx = pj_get_default_ctx();
    memset(&grinfo, 0, sizeof(PJ_GRID_INFO));

    const auto fillGridInfo = [&grinfo, ctx,
                               gridname](const NS_PROJ::Grid &grid,
                                         const std::string &format) {
        const auto &extent = grid.extentAndRes();

        /* name of grid */
        strncpy(grinfo.gridname, gridname, sizeof(grinfo.gridname) - 1);

        /* full path of grid */
        if (!pj_find_file(ctx, gridname, grinfo.filename,
                          sizeof(grinfo.filename) - 1)) {
            // Can happen when using a remote grid
            grinfo.filename[0] = 0;
        }

        /* grid format */
        strncpy(grinfo.format, format.c_str(), sizeof(grinfo.format) - 1);

        /* grid size */
        grinfo.n_lon = grid.width();
        grinfo.n_lat = grid.height();

        /* cell size */
        grinfo.cs_lon = extent.resX;
        grinfo.cs_lat = extent.resY;

        /* bounds of grid */
        grinfo.lowerleft.lam = extent.west;
        grinfo.lowerleft.phi = extent.south;
        grinfo.upperright.lam = extent.east;
        grinfo.upperright.phi = extent.north;
    };

    {
        const auto gridSet = NS_PROJ::VerticalShiftGridSet::open(ctx, gridname);
        if (gridSet) {
            const auto &grids = gridSet->grids();
            if (!grids.empty()) {
                const auto &grid = grids.front();
                fillGridInfo(*grid, gridSet->format());
                return grinfo;
            }
        }
    }

    {
        const auto gridSet =
            NS_PROJ::HorizontalShiftGridSet::open(ctx, gridname);
        if (gridSet) {
            const auto &grids = gridSet->grids();
            if (!grids.empty()) {
                const auto &grid = grids.front();
                fillGridInfo(*grid, gridSet->format());
                return grinfo;
            }
        }
    }
    strcpy(grinfo.format, "missing");
    return grinfo;
}

/*****************************************************************************/
PJ_INIT_INFO proj_init_info(const char *initname) {
    /******************************************************************************
        Information about a named init file.

        Maximum length of initname is 64.

        Returns PJ_INIT_INFO struct.

        If the init file is not found all members of the return struct are set
        to the empty string.

        If the init file is found, but the metadata is missing, the value is
        set to "Unknown".
    ******************************************************************************/
    int file_found;
    char param[80], key[74];
    paralist *start, *next;
    PJ_INIT_INFO ininfo;
    PJ_CONTEXT *ctx = pj_get_default_ctx();

    memset(&ininfo, 0, sizeof(PJ_INIT_INFO));

    file_found =
        pj_find_file(ctx, initname, ininfo.filename, sizeof(ininfo.filename));
    if (!file_found || strlen(initname) > 64) {
        if (strcmp(initname, "epsg") == 0 || strcmp(initname, "EPSG") == 0) {
            const char *val;

            proj_context_errno_set(ctx, 0);

            strncpy(ininfo.name, initname, sizeof(ininfo.name) - 1);
            strcpy(ininfo.origin, "EPSG");
            val = proj_context_get_database_metadata(ctx, "EPSG.VERSION");
            if (val) {
                strncpy(ininfo.version, val, sizeof(ininfo.version) - 1);
            }
            val = proj_context_get_database_metadata(ctx, "EPSG.DATE");
            if (val) {
                strncpy(ininfo.lastupdate, val, sizeof(ininfo.lastupdate) - 1);
            }
            return ininfo;
        }

        if (strcmp(initname, "IGNF") == 0) {
            const char *val;

            proj_context_errno_set(ctx, 0);

            strncpy(ininfo.name, initname, sizeof(ininfo.name) - 1);
            strcpy(ininfo.origin, "IGNF");
            val = proj_context_get_database_metadata(ctx, "IGNF.VERSION");
            if (val) {
                strncpy(ininfo.version, val, sizeof(ininfo.version) - 1);
            }
            val = proj_context_get_database_metadata(ctx, "IGNF.DATE");
            if (val) {
                strncpy(ininfo.lastupdate, val, sizeof(ininfo.lastupdate) - 1);
            }
            return ininfo;
        }

        return ininfo;
    }

    /* The initial memset (0) makes strncpy safe here */
    strncpy(ininfo.name, initname, sizeof(ininfo.name) - 1);
    strcpy(ininfo.origin, "Unknown");
    strcpy(ininfo.version, "Unknown");
    strcpy(ininfo.lastupdate, "Unknown");

    strncpy(key, initname, 64); /* make room for ":metadata\0" at the end */
    key[64] = 0;
    memcpy(key + strlen(key), ":metadata", 9 + 1);
    strcpy(param, "+init=");
    /* The +strlen(param) avoids a cppcheck false positive warning */
    strncat(param + strlen(param), key, sizeof(param) - 1 - strlen(param));

    start = pj_mkparam(param);
    pj_expand_init(ctx, start);

    if (pj_param(ctx, start, "tversion").i)
        strncpy(ininfo.version, pj_param(ctx, start, "sversion").s,
                sizeof(ininfo.version) - 1);

    if (pj_param(ctx, start, "torigin").i)
        strncpy(ininfo.origin, pj_param(ctx, start, "sorigin").s,
                sizeof(ininfo.origin) - 1);

    if (pj_param(ctx, start, "tlastupdate").i)
        strncpy(ininfo.lastupdate, pj_param(ctx, start, "slastupdate").s,
                sizeof(ininfo.lastupdate) - 1);

    for (; start; start = next) {
        next = start->next;
        free(start);
    }

    return ininfo;
}

/*****************************************************************************/
PJ_FACTORS proj_factors(PJ *P, PJ_COORD lp) {
    /******************************************************************************
        Cartographic characteristics at point lp.

        Characteristics include meridian, parallel and areal scales, angular
        distortion, meridian/parallel, meridian convergence and scale error.

        returns PJ_FACTORS. If unsuccessful, error number is set and the
        struct returned contains NULL data.
    ******************************************************************************/
    PJ_FACTORS factors = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    struct FACTORS f;

    if (nullptr == P)
        return factors;

    auto pj = P;
    auto type = proj_get_type(pj);

    PJ *horiz = nullptr;
    if (pj->cached_op_for_proj_factors) {
        pj = pj->cached_op_for_proj_factors;
    } else {
        if (type == PJ_TYPE_COMPOUND_CRS) {
            horiz = proj_crs_get_sub_crs(pj->ctx, pj, 0);
            pj = horiz;
            type = proj_get_type(pj);
        }

        if (type == PJ_TYPE_PROJECTED_CRS) {
            // If it is a projected CRS, then compute the factors on the
            // conversion associated to it. We need to start from a temporary
            // geographic CRS using the same datum as the one of the projected
            // CRS, and with input coordinates being in longitude, latitude
            // order in radian, to be consistent with the expectations of the lp
            // input parameter. We also need to create a modified projected CRS
            // with a normalized easting/northing axis order in metre, so the
            // resulting operation is just a single step pipeline with no
            // axisswap or unitconvert steps.

            auto ctx = pj->ctx;
            auto geodetic_crs = proj_get_source_crs(ctx, pj);
            assert(geodetic_crs);
            auto pm = proj_get_prime_meridian(ctx, geodetic_crs);
            double pm_longitude = 0;
            proj_prime_meridian_get_parameters(ctx, pm, &pm_longitude, nullptr,
                                               nullptr);
            proj_destroy(pm);
            PJ *geogCRSNormalized;
            auto cs = proj_create_ellipsoidal_2D_cs(
                ctx, PJ_ELLPS2D_LONGITUDE_LATITUDE, "Radian", 1.0);
            if (pm_longitude != 0) {
                auto ellipsoid = proj_get_ellipsoid(ctx, geodetic_crs);
                double semi_major_metre = 0;
                double inv_flattening = 0;
                proj_ellipsoid_get_parameters(ctx, ellipsoid, &semi_major_metre,
                                              nullptr, nullptr,
                                              &inv_flattening);
                geogCRSNormalized = proj_create_geographic_crs(
                    ctx, "unname crs", "unnamed datum",
                    proj_get_name(ellipsoid), semi_major_metre, inv_flattening,
                    "reference prime meridian", 0, nullptr, 0, cs);
                proj_destroy(ellipsoid);
            } else {
                auto datum = proj_crs_get_datum(ctx, geodetic_crs);
                auto datum_ensemble =
                    proj_crs_get_datum_ensemble(ctx, geodetic_crs);
                geogCRSNormalized = proj_create_geographic_crs_from_datum(
                    ctx, "unnamed crs", datum ? datum : datum_ensemble, cs);
                proj_destroy(datum);
                proj_destroy(datum_ensemble);
            }
            proj_destroy(cs);
            auto conversion = proj_crs_get_coordoperation(ctx, pj);
            auto projCS = proj_create_cartesian_2D_cs(
                ctx, PJ_CART2D_EASTING_NORTHING, "metre", 1.0);
            auto projCRSNormalized = proj_create_projected_crs(
                ctx, nullptr, geodetic_crs, conversion, projCS);
            assert(projCRSNormalized);
            proj_destroy(geodetic_crs);
            proj_destroy(conversion);
            proj_destroy(projCS);
            auto newOp = proj_create_crs_to_crs_from_pj(
                ctx, geogCRSNormalized, projCRSNormalized, nullptr, nullptr);
            proj_destroy(geogCRSNormalized);
            proj_destroy(projCRSNormalized);
            assert(newOp);
            // For debugging:
            // printf("%s\n", proj_as_proj_string(ctx, newOp, PJ_PROJ_5,
            // nullptr));

            P->cached_op_for_proj_factors = newOp;
            pj = newOp;
        } else if (type != PJ_TYPE_CONVERSION &&
                   type != PJ_TYPE_TRANSFORMATION &&
                   type != PJ_TYPE_CONCATENATED_OPERATION &&
                   type != PJ_TYPE_OTHER_COORDINATE_OPERATION) {
            proj_log_error(P, _("Invalid type for P object"));
            proj_errno_set(P, PROJ_ERR_INVALID_OP_ILLEGAL_ARG_VALUE);
            if (horiz)
                proj_destroy(horiz);
            return factors;
        }
    }

    const int ret = pj_factors(lp.lp, P, pj, 0.0, &f);
    if (horiz)
        proj_destroy(horiz);
    if (ret)
        return factors;

    factors.meridional_scale = f.h;
    factors.parallel_scale = f.k;
    factors.areal_scale = f.s;

    factors.angular_distortion = f.omega;
    factors.meridian_parallel_angle = f.thetap;
    factors.meridian_convergence = f.conv;

    factors.tissot_semimajor = f.a;
    factors.tissot_semiminor = f.b;

    /* Raw derivatives, for completeness's sake */
    factors.dx_dlam = f.der.x_l;
    factors.dx_dphi = f.der.x_p;
    factors.dy_dlam = f.der.y_l;
    factors.dy_dphi = f.der.y_p;

    return factors;
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress

// Returns true if the passed operation uses NADCON5 grids for NAD83 to
// NAD83(HARN)
static bool isSpecialCaseForNAD83_to_NAD83HARN(const std::string &opName) {
    return opName.find("NAD83 to NAD83(HARN) (47)") != std::string::npos ||
           opName.find("NAD83 to NAD83(HARN) (48)") != std::string::npos ||
           opName.find("NAD83 to NAD83(HARN) (49)") != std::string::npos ||
           opName.find("NAD83 to NAD83(HARN) (50)") != std::string::npos;
}

// Returns true if the passed operation uses "GDA94 to WGS 84 (1)", which
// is the null transformation
static bool isSpecialCaseForGDA94_to_WGS84(const std::string &opName) {
    return opName.find("GDA94 to WGS 84 (1)") != std::string::npos;
}

// Returns true if the passed operation uses "GDA2020 to WGS 84 (2)", which
// is the null transformation
static bool isSpecialCaseForWGS84_to_GDA2020(const std::string &opName) {
    return opName.find("GDA2020 to WGS 84 (2)") != std::string::npos;
}

PJCoordOperation::PJCoordOperation(
    int idxInOriginalListIn, double minxSrcIn, double minySrcIn,
    double maxxSrcIn, double maxySrcIn, double minxDstIn, double minyDstIn,
    double maxxDstIn, double maxyDstIn, PJ *pjIn, const std::string &nameIn,
    double accuracyIn, double pseudoAreaIn, const char *areaNameIn,
    const PJ *pjSrcGeocentricToLonLatIn, const PJ *pjDstGeocentricToLonLatIn)
    : idxInOriginalList(idxInOriginalListIn), minxSrc(minxSrcIn),
      minySrc(minySrcIn), maxxSrc(maxxSrcIn), maxySrc(maxySrcIn),
      minxDst(minxDstIn), minyDst(minyDstIn), maxxDst(maxxDstIn),
      maxyDst(maxyDstIn), pj(pjIn), name(nameIn), accuracy(accuracyIn),
      pseudoArea(pseudoAreaIn), areaName(areaNameIn ? areaNameIn : ""),
      isOffshore(areaName.find("- offshore") != std::string::npos),
      isUnknownAreaName(areaName.empty() || areaName == "unknown"),
      isPriorityOp(isSpecialCaseForNAD83_to_NAD83HARN(name) ||
                   isSpecialCaseForGDA94_to_WGS84(name) ||
                   isSpecialCaseForWGS84_to_GDA2020(name)),
      pjSrcGeocentricToLonLat(pjSrcGeocentricToLonLatIn
                                  ? proj_clone(pjSrcGeocentricToLonLatIn->ctx,
                                               pjSrcGeocentricToLonLatIn)
                                  : nullptr),
      pjDstGeocentricToLonLat(pjDstGeocentricToLonLatIn
                                  ? proj_clone(pjDstGeocentricToLonLatIn->ctx,
                                               pjDstGeocentricToLonLatIn)
                                  : nullptr) {
    const auto IsLonLatOrLatLon = [](const PJ *crs, bool &isLonLatDegreeOut,
                                     bool &isLatLonDegreeOut) {
        const auto eType = proj_get_type(crs);
        if (eType == PJ_TYPE_GEOGRAPHIC_2D_CRS ||
            eType == PJ_TYPE_GEOGRAPHIC_3D_CRS) {
            const auto cs = proj_crs_get_coordinate_system(crs->ctx, crs);
            const char *direction = "";
            double conv_factor = 0;
            constexpr double EPS = 1e-14;
            if (proj_cs_get_axis_info(crs->ctx, cs, 0, nullptr, nullptr,
                                      &direction, &conv_factor, nullptr,
                                      nullptr, nullptr) &&
                ci_equal(direction, "East")) {
                isLonLatDegreeOut = fabs(conv_factor - M_PI / 180) < EPS;
            } else if (proj_cs_get_axis_info(crs->ctx, cs, 1, nullptr, nullptr,
                                             &direction, &conv_factor, nullptr,
                                             nullptr, nullptr) &&
                       ci_equal(direction, "East")) {
                isLatLonDegreeOut = fabs(conv_factor - M_PI / 180) < EPS;
            }
            proj_destroy(cs);
        }
    };

    const auto source = proj_get_source_crs(pj->ctx, pj);
    if (source) {
        IsLonLatOrLatLon(source, srcIsLonLatDegree, srcIsLatLonDegree);
        proj_destroy(source);
    }

    const auto target = proj_get_target_crs(pj->ctx, pj);
    if (target) {
        IsLonLatOrLatLon(target, dstIsLonLatDegree, dstIsLatLonDegree);
        proj_destroy(target);
    }
}

//! @endcond
