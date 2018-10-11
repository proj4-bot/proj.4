/******************************************************************************
 *
 * Project:  PROJ
 * Purpose:  C API wraper of C++ API
 * Author:   Even Rouault <even dot rouault at spatialys dot com>
 *
 ******************************************************************************
 * Copyright (c) 2018, Even Rouault <even dot rouault at spatialys dot com>
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
 ****************************************************************************/

#ifndef FROM_PROJ_CPP
#define FROM_PROJ_CPP
#endif

#include <cassert>
#include <cstdarg>
#include <cstring>
#include <map>

#include "proj/common.hpp"
#include "proj/coordinateoperation.hpp"
#include "proj/crs.hpp"
#include "proj/datum.hpp"
#include "proj/io.hpp"
#include "proj/metadata.hpp"
#include "proj/util.hpp"

#include "proj/internal/internal.hpp"

// PROJ include order is sensitive
// clang-format off
#include "proj_internal.h"
#include "proj.h"
#include "projects.h"
// clang-format on

using namespace NS_PROJ::common;
using namespace NS_PROJ::crs;
using namespace NS_PROJ::datum;
using namespace NS_PROJ::io;
using namespace NS_PROJ::internal;
using namespace NS_PROJ::metadata;
using namespace NS_PROJ::operation;
using namespace NS_PROJ::util;

// ---------------------------------------------------------------------------

static void proj_log_error(PJ_CONTEXT *ctx, const char *function,
                           const char *text) {
    std::string msg(function);
    msg += ": ";
    msg += text;
    ctx->logger(ctx->app_data, PJ_LOG_ERROR, msg.c_str());
}

// ---------------------------------------------------------------------------

/** \brief Opaque object representing a Ellipsoid, Datum, CRS or Coordinate
 * Operation. Should be used by at most one thread at a time. */
struct PJ_OBJ {
    //! @cond Doxygen_Suppress
    PJ_CONTEXT *ctx;
    BaseObjectNNPtr obj;

    // cached results
    std::map<PJ_WKT_TYPE, std::string> mapWKTString{};
    std::map<PJ_PROJ_STRING_TYPE, std::string> mapPROJString{};
    bool gridsNeededAsked = false;
    std::vector<CoordinateOperation::GridDescription> gridsNeeded{};

    explicit PJ_OBJ(PJ_CONTEXT *ctxIn, const BaseObjectNNPtr &objIn)
        : ctx(ctxIn), obj(objIn) {}

    PJ_OBJ(const PJ_OBJ &) = delete;
    PJ_OBJ &operator=(const PJ_OBJ &) = delete;
    //! @endcond
};

// ---------------------------------------------------------------------------

/** \brief Instanciate an object from a WKT string.
 *
 * This function calls osgeo::proj::io::WKTParser::createFromWKT()
 *
 * The returned object must be unreferenced with proj_obj_unref() after use.
 * It should be used by at most one thread at a time.
 *
 * @param ctx PROJ context, or NULL for default context
 * @param wkt WKT string (must not be NULL)
 * @return Object that must be unreferenced with proj_obj_unref(), or NULL in
 * case of error.
 */
PJ_OBJ *proj_obj_create_from_wkt(PJ_CONTEXT *ctx, const char *wkt) {
    if (ctx == nullptr) {
        ctx = pj_get_default_ctx();
    }
    assert(wkt);
    try {
        return new PJ_OBJ(ctx, WKTParser().createFromWKT(wkt));
    } catch (const std::exception &e) {
        proj_log_error(ctx, __FUNCTION__, e.what());
        return nullptr;
    }
}

// ---------------------------------------------------------------------------

/** \brief Instanciate an object from a PROJ string.
 *
 * This function calls osgeo::proj::io::PROJStringParser::createFromPROJString()
 *
 * The returned object must be unreferenced with proj_obj_unref() after use.
 * It should be used by at most one thread at a time.
 *
 * @param ctx PROJ context, or NULL for default context
 * @param proj_string PROJ string (must not be NULL)
 * @return Object that must be unreferenced with proj_obj_unref(), or NULL in
 * case of error.
 */
PJ_OBJ *proj_obj_create_from_proj_string(PJ_CONTEXT *ctx,
                                         const char *proj_string) {
    if (ctx == nullptr) {
        ctx = pj_get_default_ctx();
    }
    assert(proj_string);
    try {
        return new PJ_OBJ(ctx,
                          PROJStringParser().createFromPROJString(proj_string));
    } catch (const std::exception &e) {
        proj_log_error(ctx, __FUNCTION__, e.what());
        return nullptr;
    }
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress

/** Auxiliary structure to PJ_CONTEXT storing C++ context stuff. */
struct projCppContext {
    DatabaseContextNNPtr databaseContext;

    explicit projCppContext(const DatabaseContextNNPtr &databaseContextIn)
        : databaseContext(databaseContextIn) {}
};

// ---------------------------------------------------------------------------

void proj_context_delete_cpp_context(struct projCppContext *cppContext) {
    delete cppContext;
}

//! @endcond

// ---------------------------------------------------------------------------

/** \brief Instanciate an object from a database lookup.
 *
 * The returned object must be unreferenced with proj_obj_unref() after use.
 * It should be used by at most one thread at a time.
 *
 * @param ctx Context, or NULL for default context.
 * @param auth_name Authority name (must not be NULL)
 * @param code Object code (must not be NULL)
 * @param category Object category
 * @param usePROJAlternativeGridNames Whether PROJ alternative grid names
 * should be substituted to the official grid names. Only used on
 * transformations
 * @param options should be set to NULL for now
 * @return Object that must be unreferenced with proj_obj_unref(), or NULL in
 * case of error.
 */
PJ_OBJ *proj_obj_create_from_database(PJ_CONTEXT *ctx, const char *auth_name,
                                      const char *code,
                                      PJ_OBJ_CATEGORY category,
                                      int usePROJAlternativeGridNames,
                                      char **options) {
    assert(auth_name);
    assert(code);
    (void)options;

    try {
        if (ctx->cpp_context == nullptr) {
            ctx->cpp_context =
                new projCppContext(DatabaseContext::createWithPJContext(ctx));
        }
        auto factory = AuthorityFactory::create(
            ctx->cpp_context->databaseContext, auth_name);
        switch (category) {
        case PJ_OBJ_CATEGORY_ELLIPSOID:
            return new PJ_OBJ(ctx, factory->createEllipsoid(code));

        case PJ_OBJ_CATEGORY_DATUM:
            return new PJ_OBJ(ctx, factory->createDatum(code));

        case PJ_OBJ_CATEGORY_CRS:
            return new PJ_OBJ(ctx,
                              factory->createCoordinateReferenceSystem(code));

        case PJ_OBJ_CATEGORY_COORDINATE_OPERATION:
            return new PJ_OBJ(ctx, factory->createCoordinateOperation(
                                       code, usePROJAlternativeGridNames != 0));
        }
    } catch (const std::exception &e) {
        proj_log_error(ctx, __FUNCTION__, e.what());
    }
    return nullptr;
}

// ---------------------------------------------------------------------------

/** \brief Drops a reference on an object.
 *
 * This method should be called one and exactly one for each function
 * returning a PJ_OBJ*
 *
 * @param obj Object, or NULL.
 */
void proj_obj_unref(PJ_OBJ *obj) { delete obj; }

// ---------------------------------------------------------------------------

/** \brief Return the type of an object.
 *
 * @param obj Object (must not be NULL)
 * @return its type.
 */
PJ_OBJ_TYPE proj_obj_get_type(PJ_OBJ *obj) {
    assert(obj);
    if (nn_dynamic_pointer_cast<Ellipsoid>(obj->obj)) {
        return PJ_OBJ_TYPE_ELLIPSOID;
    }

    if (nn_dynamic_pointer_cast<DynamicGeodeticReferenceFrame>(obj->obj)) {
        return PJ_OBJ_TYPE_DYNAMIC_GEODETIC_REFERENCE_FRAME;
    }
    if (nn_dynamic_pointer_cast<GeodeticReferenceFrame>(obj->obj)) {
        return PJ_OBJ_TYPE_GEODETIC_REFERENCE_FRAME;
    }
    if (nn_dynamic_pointer_cast<DynamicVerticalReferenceFrame>(obj->obj)) {
        return PJ_OBJ_TYPE_DYNAMIC_VERTICAL_REFERENCE_FRAME;
    }
    if (nn_dynamic_pointer_cast<VerticalReferenceFrame>(obj->obj)) {
        return PJ_OBJ_TYPE_VERTICAL_REFERENCE_FRAME;
    }
    if (nn_dynamic_pointer_cast<DatumEnsemble>(obj->obj)) {
        return PJ_OBJ_TYPE_DATUM_ENSEMBLE;
    }

    if (nn_dynamic_pointer_cast<GeographicCRS>(obj->obj)) {
        return PJ_OBJ_TYPE_GEOGRAPHIC_CRS;
    }
    if (nn_dynamic_pointer_cast<GeodeticCRS>(obj->obj)) {
        return PJ_OBJ_TYPE_GEODETIC_CRS;
    }
    if (nn_dynamic_pointer_cast<VerticalCRS>(obj->obj)) {
        return PJ_OBJ_TYPE_VERTICAL_CRS;
    }
    if (nn_dynamic_pointer_cast<ProjectedCRS>(obj->obj)) {
        return PJ_OBJ_TYPE_PROJECTED_CRS;
    }
    if (nn_dynamic_pointer_cast<CompoundCRS>(obj->obj)) {
        return PJ_OBJ_TYPE_COMPOUND_CRS;
    }
    if (nn_dynamic_pointer_cast<TemporalCRS>(obj->obj)) {
        return PJ_OBJ_TYPE_TEMPORAL_CRS;
    }
    if (nn_dynamic_pointer_cast<BoundCRS>(obj->obj)) {
        return PJ_OBJ_TYPE_BOUND_CRS;
    }
    if (nn_dynamic_pointer_cast<CRS>(obj->obj)) {
        return PJ_OBJ_TYPE_OTHER_CRS;
    }

    if (nn_dynamic_pointer_cast<Conversion>(obj->obj)) {
        return PJ_OBJ_TYPE_CONVERSION;
    }
    if (nn_dynamic_pointer_cast<Transformation>(obj->obj)) {
        return PJ_OBJ_TYPE_TRANSFORMATION;
    }
    if (nn_dynamic_pointer_cast<ConcatenatedOperation>(obj->obj)) {
        return PJ_OBJ_TYPE_CONCATENATED_OPERATION;
    }
    if (nn_dynamic_pointer_cast<CoordinateOperation>(obj->obj)) {
        return PJ_OBJ_TYPE_OTHER_COORDINATE_OPERATION;
    }

    return PJ_OBJ_TYPE_UNKNOWN;
}

// ---------------------------------------------------------------------------

/** \brief Return whether an object is a CRS
 *
 * @param obj Object (must not be NULL)
 */
int proj_obj_is_crs(PJ_OBJ *obj) {
    assert(obj);
    return nn_dynamic_pointer_cast<CRS>(obj->obj) != nullptr;
}

// ---------------------------------------------------------------------------

/** \brief Get the name of an object.
 *
 * The lifetime of the returned string is the same as the input obj parameter.
 *
 * @param obj Object (must not be NULL)
 * @return a string, or NULL in case of error or missing name.
 */
const char *proj_obj_get_name(PJ_OBJ *obj) {
    assert(obj);
    auto identifiable = nn_dynamic_pointer_cast<IdentifiedObject>(obj->obj);
    if (!identifiable) {
        proj_log_error(obj->ctx, __FUNCTION__,
                       "Object type not castable to IdentifiedObject");
        return nullptr;
    }
    if (!identifiable->name()->description().has_value()) {
        return nullptr;
    }
    return identifiable->name()->description()->c_str();
}

// ---------------------------------------------------------------------------

/** \brief Get the authority name / codespace of an identifier of an object.
 *
 * The lifetime of the returned string is the same as the input obj parameter.
 *
 * @param obj Object (must not be NULL)
 * @param index Index of the identifier. 0 = first identifier
 * @return a string, or NULL in case of error or missing name.
 */
const char *proj_obj_get_id_auth_name(PJ_OBJ *obj, int index) {
    assert(obj);
    auto identifiable = nn_dynamic_pointer_cast<IdentifiedObject>(obj->obj);
    if (!identifiable) {
        proj_log_error(obj->ctx, __FUNCTION__,
                       "Object type not castable to IdentifiedObject");
        return nullptr;
    }
    if (index < 0 ||
        static_cast<size_t>(index) >= identifiable->identifiers().size()) {
        return nullptr;
    }
    if (!identifiable->identifiers()[index]->codeSpace().has_value()) {
        return nullptr;
    }
    return identifiable->identifiers()[index]->codeSpace()->c_str();
}

// ---------------------------------------------------------------------------

/** \brief Get the code of an identifier of an object.
 *
 * The lifetime of the returned string is the same as the input obj parameter.
 *
 * @param obj Object (must not be NULL)
 * @param index Index of the identifier. 0 = first identifier
 * @return a string, or NULL in case of error or missing name.
 */
const char *proj_obj_get_id_code(PJ_OBJ *obj, int index) {
    assert(obj);
    auto identifiable = nn_dynamic_pointer_cast<IdentifiedObject>(obj->obj);
    if (!identifiable) {
        proj_log_error(obj->ctx, __FUNCTION__,
                       "Object type not castable to IdentifiedObject");
        return nullptr;
    }
    if (index < 0 ||
        static_cast<size_t>(index) >= identifiable->identifiers().size()) {
        return nullptr;
    }
    return identifiable->identifiers()[index]->code().c_str();
}

// ---------------------------------------------------------------------------

/** \brief Get a WKT representation of an object.
 *
 * The lifetime of the returned string is the same as the input obj parameter.
 *
 * This function calls osgeo::proj::io::IWKTExportable::exportToWKT().
 *
 * This function may return NULL if the object is not compatible with an
 * export to the requested type.
 *
 * @param obj Object (must not be NULL)
 * @param type WKT version.
 * @param options should be set to NULL for now
 * @return a string, or NULL in case of error.
 */
const char *proj_obj_as_wkt(PJ_OBJ *obj, PJ_WKT_TYPE type, char **options) {
    assert(obj);
    (void)options;
    auto iter = obj->mapWKTString.find(type);
    if (iter != obj->mapWKTString.end()) {
        return iter->second.c_str();
    }
    auto wktExportable = nn_dynamic_pointer_cast<IWKTExportable>(obj->obj);
    if (!wktExportable) {
        proj_log_error(obj->ctx, __FUNCTION__,
                       "Object type not exportable to WKT");
        return nullptr;
    }
    WKTFormatter::Convention convention = WKTFormatter::Convention::WKT2_2018;
    switch (type) {
    case PJ_WKT2_2018:
        convention = WKTFormatter::Convention::WKT2_2018;
        break;
    case PJ_WKT2_2018_SIMPLIFIED:
        convention = WKTFormatter::Convention::WKT2_2018_SIMPLIFIED;
        break;
    case PJ_WKT2_2015:
        convention = WKTFormatter::Convention::WKT2_2015;
        break;
    case PJ_WKT2_2015_SIMPLIFIED:
        convention = WKTFormatter::Convention::WKT2_2015_SIMPLIFIED;
        break;
    case PJ_WKT1_GDAL:
        convention = WKTFormatter::Convention::WKT1_GDAL;
        break;
    }
    try {
        auto wkt = wktExportable->exportToWKT(WKTFormatter::create(convention));
        obj->mapWKTString[type] = wkt;
        return obj->mapWKTString[type].c_str();
    } catch (const std::exception &e) {
        proj_log_error(obj->ctx, __FUNCTION__, e.what());
        return nullptr;
    }
}

// ---------------------------------------------------------------------------

/** \brief Get a PROJ string representation of an object.
 *
 * The lifetime of the returned string is the same as the input obj parameter.
 *
 * This function calls
 * osgeo::proj::io::IPROJStringExportable::exportToPROJString().
 *
 * This function may return NULL if the object is not compatible with an
 * export to the requested type.
 *
 * @param obj Object (must not be NULL)
 * @param type PROJ String version.
 * @param options NULL-terminated list of strings with "KEY=VALUE" format. or
 * NULL.
 * The currently recognized option is USE_ETMERC=YES to use
 * +proj=etmerc instead of +proj=tmerc
 * @return a string, or NULL in case of error.
 */
const char *proj_obj_as_proj_string(PJ_OBJ *obj, PJ_PROJ_STRING_TYPE type,
                                    char **options) {
    assert(obj);
    auto iter = obj->mapPROJString.find(type);
    if (iter != obj->mapPROJString.end()) {
        return iter->second.c_str();
    }
    auto exportable = nn_dynamic_pointer_cast<IPROJStringExportable>(obj->obj);
    if (!exportable) {
        proj_log_error(obj->ctx, __FUNCTION__,
                       "Object type not exportable to PROJ");
        return nullptr;
    }
    PROJStringFormatter::Convention convention =
        PROJStringFormatter::Convention::PROJ_5;
    switch (type) {
    case PJ_PROJ_5:
        convention = PROJStringFormatter::Convention::PROJ_5;
        break;
    case PJ_PROJ_4:
        convention = PROJStringFormatter::Convention::PROJ_4;
        break;
    }
    try {
        if (obj->ctx->cpp_context == nullptr) {
            obj->ctx->cpp_context = new projCppContext(
                DatabaseContext::createWithPJContext(obj->ctx));
        }
        auto formatter = PROJStringFormatter::create(
            convention, obj->ctx->cpp_context->databaseContext);
        if (options != nullptr && options[0] != nullptr) {
            if (ci_equal(options[0], "USE_ETMERC=YES")) {
                formatter->setUseETMercForTMerc(true);
            }
        }
        auto wkt = exportable->exportToPROJString(formatter);
        obj->mapPROJString[type] = wkt;
        return obj->mapPROJString[type].c_str();
    } catch (const std::exception &e) {
        proj_log_error(obj->ctx, __FUNCTION__, e.what());
        return nullptr;
    }
}

// ---------------------------------------------------------------------------

static GeodeticCRSPtr extractGeodeticCRS(PJ_OBJ *crs, const char *fname) {
    assert(crs);
    auto l_crs = nn_dynamic_pointer_cast<CRS>(crs->obj);
    if (!l_crs) {
        proj_log_error(crs->ctx, fname, "Object is not a CRS");
        return nullptr;
    }
    auto geodCRS = l_crs->extractGeodeticCRS();
    if (!geodCRS) {
        proj_log_error(crs->ctx, fname, "CRS has no geodetic CRS");
        return nullptr;
    }
    return geodCRS;
}

// ---------------------------------------------------------------------------

/** \brief Get the geodeticCRS / geographicCRS from a CRS
 *
 * The returned object must be unreferenced with proj_obj_unref() after
 * use.
 * It should be used by at most one thread at a time.
 *
 * @param crs Objet of type CRS (must not be NULL)
 * @return Object that must be unreferenced with proj_obj_unref(), or NULL
 * in case of error.
 */
PJ_OBJ *proj_obj_crs_get_geodetic_crs(PJ_OBJ *crs) {
    auto geodCRS = extractGeodeticCRS(crs, __FUNCTION__);
    if (!geodCRS) {
        return nullptr;
    }
    return new PJ_OBJ(crs->ctx, NN_CHECK_ASSERT(geodCRS));
}

// ---------------------------------------------------------------------------

/** \brief Get a CRS component from a CompoundCRS
 *
 * The returned object must be unreferenced with proj_obj_unref() after
 * use.
 * It should be used by at most one thread at a time.
 *
 * @param crs Objet of type CRS (must not be NULL)
 * @param index Index of the CRS component (typically 0 = horizontal, 1 =
 * vertical)
 * @return Object that must be unreferenced with proj_obj_unref(), or NULL
 * in case of error.
 */
PJ_OBJ *proj_obj_crs_get_sub_crs(PJ_OBJ *crs, int index) {
    assert(crs);
    auto l_crs = nn_dynamic_pointer_cast<CompoundCRS>(crs->obj);
    if (!l_crs) {
        proj_log_error(crs->ctx, __FUNCTION__, "Object is not a CompoundCRS");
        return nullptr;
    }
    auto components = l_crs->componentReferenceSystems();
    if (index < 0 || static_cast<size_t>(index) >= components.size()) {
        return nullptr;
    }
    return new PJ_OBJ(crs->ctx, components[index]);
}

// ---------------------------------------------------------------------------

/** \brief Returns potentially
 * a BoundCRS, with a transformation to EPSG:4326, wrapping this CRS
 *
 * The returned object must be unreferenced with proj_obj_unref() after
 * use.
 * It should be used by at most one thread at a time.
 *
 * This is the same as method
 * osgeo::proj::crs::CRS::createBoundCRSToWGS84IfPossible()
 *
 * @param crs Objet of type CRS (must not be NULL)
 * @return Object that must be unreferenced with proj_obj_unref(), or NULL
 * in case of error.
 */
PJ_OBJ *proj_obj_crs_create_bound_crs_to_WGS84(PJ_OBJ *crs) {
    assert(crs);
    auto l_crs = nn_dynamic_pointer_cast<CRS>(crs->obj);
    if (!l_crs) {
        proj_log_error(crs->ctx, __FUNCTION__, "Object is not a CRS");
        return nullptr;
    }
    return new PJ_OBJ(crs->ctx, l_crs->createBoundCRSToWGS84IfPossible());
}

// ---------------------------------------------------------------------------

/** \brief Get the ellipsoid from a CRS or a GeodeticReferenceFrame.
 *
 * The returned object must be unreferenced with proj_obj_unref() after
 * use.
 * It should be used by at most one thread at a time.
 *
 * @param obj Objet of type CRS or GeodeticReferenceFrame (must not be NULL)
 * @return Object that must be unreferenced with proj_obj_unref(), or NULL
 * in case of error.
 */
PJ_OBJ *proj_obj_get_ellipsoid(PJ_OBJ *obj) {
    if (nn_dynamic_pointer_cast<CRS>(obj->obj)) {
        auto geodCRS = extractGeodeticCRS(obj, __FUNCTION__);
        if (!geodCRS) {
            return nullptr;
        }
        return new PJ_OBJ(obj->ctx, geodCRS->ellipsoid());
    } else {
        auto datum = nn_dynamic_pointer_cast<GeodeticReferenceFrame>(obj->obj);
        if (datum) {
            return new PJ_OBJ(obj->ctx, datum->ellipsoid());
        }
    }
    proj_log_error(obj->ctx, __FUNCTION__,
                   "Object is not a CRS or GeodeticReferenceFrame");
    return nullptr;
}

// ---------------------------------------------------------------------------

/** \brief Get the horizontal datum from a CRS
 *
 * The returned object must be unreferenced with proj_obj_unref() after
 * use.
 * It should be used by at most one thread at a time.
 *
 * @param crs Objet of type CRS (must not be NULL)
 * @return Object that must be unreferenced with proj_obj_unref(), or NULL
 * in case of error.
 */
PJ_OBJ *proj_obj_crs_get_horizontal_datum(PJ_OBJ *crs) {
    auto geodCRS = extractGeodeticCRS(crs, __FUNCTION__);
    if (!geodCRS) {
        return nullptr;
    }
    auto datum = geodCRS->datum();
    if (!datum) {
        auto datumEnsemble = geodCRS->datumEnsemble();
        if (datumEnsemble) {
            return new PJ_OBJ(crs->ctx, NN_CHECK_ASSERT(datumEnsemble));
        }
        proj_log_error(crs->ctx, __FUNCTION__, "CRS has no datum");
        return nullptr;
    }
    return new PJ_OBJ(crs->ctx, NN_CHECK_ASSERT(datum));
}

// ---------------------------------------------------------------------------

/** \brief Return ellipsoid parameters.
 *
 * @param ellipsoid Object of type Ellipsoid (must not be NULL)
 * @param pSemiMajorMetre Pointer to a value to store the semi-major axis in
 * metre. or NULL
 * @param pSemiMinorMetre Pointer to a value to store the semi-minor axis in
 * metre. or NULL
 * @param pIsSemiMinorComputed Pointer to a boolean value to indicate if the
 * semi-minor value was computed. If FALSE, its value comes from the
 * definition. or NULL
 * @param pInverseFlattening Pointer to a value to store the inverse
 * flattening. or NULL
 * @return TRUE in case of success.
 */
int proj_obj_ellipsoid_get_parameters(PJ_OBJ *ellipsoid,
                                      double *pSemiMajorMetre,
                                      double *pSemiMinorMetre,
                                      int *pIsSemiMinorComputed,
                                      double *pInverseFlattening) {
    assert(ellipsoid);
    auto l_ellipsoid = nn_dynamic_pointer_cast<Ellipsoid>(ellipsoid->obj);
    if (!l_ellipsoid) {
        proj_log_error(ellipsoid->ctx, __FUNCTION__,
                       "Object is not a Ellipsoid");
        return FALSE;
    }

    if (pSemiMajorMetre) {
        *pSemiMajorMetre = l_ellipsoid->semiMajorAxis().getSIValue();
    }
    if (pSemiMinorMetre) {
        *pSemiMinorMetre = l_ellipsoid->computeSemiMinorAxis().getSIValue();
    }
    if (pIsSemiMinorComputed) {
        *pIsSemiMinorComputed = !(l_ellipsoid->semiMinorAxis().has_value());
    }
    if (pInverseFlattening) {
        *pInverseFlattening =
            l_ellipsoid->computeInverseFlattening().getSIValue();
    }
    return TRUE;
}

// ---------------------------------------------------------------------------

/** \brief Get the prime meridian of a CRS or a GeodeticReferenceFrame.
 *
 * The returned object must be unreferenced with proj_obj_unref() after
 * use.
 * It should be used by at most one thread at a time.
 *
 * @param obj Objet of type CRS or GeodeticReferenceFrame (must not be NULL)
 * @return Object that must be unreferenced with proj_obj_unref(), or NULL
 * in case of error.
 */

PJ_OBJ *proj_obj_get_prime_meridian(PJ_OBJ *obj) {
    if (nn_dynamic_pointer_cast<CRS>(obj->obj)) {
        auto geodCRS = extractGeodeticCRS(obj, __FUNCTION__);
        if (!geodCRS) {
            return nullptr;
        }
        return new PJ_OBJ(obj->ctx, geodCRS->primeMeridian());
    } else {
        auto datum = nn_dynamic_pointer_cast<GeodeticReferenceFrame>(obj->obj);
        if (datum) {
            return new PJ_OBJ(obj->ctx, datum->primeMeridian());
        }
    }
    proj_log_error(obj->ctx, __FUNCTION__,
                   "Object is not a CRS or GeodeticReferenceFrame");
    return nullptr;
}

// ---------------------------------------------------------------------------

/** \brief Return prime meridian parameters.
 *
 * @param prime_meridian Object of type PrimeMeridian (must not be NULL)
 * @param pLongitude Pointer to a value to store the longitude of the prime
 * meridian, in its native unit. or NULL
 * @param pLongitudeUnitConvFactor Pointer to a value to store the conversion
 * factor of the prime meridian longitude unit to radian. or NULL
 * @param pLongitudeUnitName Pointer to a string value to store the unit name.
 * or NULL
 * @return TRUE in case of success.
 */
int proj_obj_prime_meridian_get_parameters(PJ_OBJ *prime_meridian,
                                           double *pLongitude,
                                           double *pLongitudeUnitConvFactor,
                                           const char **pLongitudeUnitName) {
    assert(prime_meridian);
    auto l_pm = nn_dynamic_pointer_cast<PrimeMeridian>(prime_meridian->obj);
    if (!l_pm) {
        proj_log_error(prime_meridian->ctx, __FUNCTION__,
                       "Object is not a PrimeMeridian");
        return false;
    }
    const auto &longitude = l_pm->longitude();
    if (pLongitude) {
        *pLongitude = longitude.value();
    }
    if (pLongitudeUnitConvFactor) {
        *pLongitudeUnitConvFactor = longitude.unit().conversionToSI();
    }
    if (pLongitudeUnitName) {
        *pLongitudeUnitName = longitude.unit().name().c_str();
    }
    return true;
}

// ---------------------------------------------------------------------------

/** \brief Return the base CRS of a BoundCRS or the source CRS of a
 * CoordinateOperation.
 *
 * The returned object must be unreferenced with proj_obj_unref() after
 * use.
 * It should be used by at most one thread at a time.
 *
 * @param obj Objet of type BoundCRS or CoordinateOperation (must not be NULL)
 * @return Object that must be unreferenced with proj_obj_unref(), or NULL
 * in case of error, or missing source CRS.
 */
PJ_OBJ *proj_obj_get_source_crs(PJ_OBJ *obj) {
    assert(obj);
    auto boundCRS = nn_dynamic_pointer_cast<BoundCRS>(obj->obj);
    if (boundCRS) {
        return new PJ_OBJ(obj->ctx, boundCRS->baseCRS());
    }
    auto co = nn_dynamic_pointer_cast<CoordinateOperation>(obj->obj);
    if (co) {
        auto sourceCRS = co->sourceCRS();
        if (sourceCRS) {
            return new PJ_OBJ(obj->ctx, NN_CHECK_ASSERT(sourceCRS));
        }
        return nullptr;
    }
    proj_log_error(obj->ctx, __FUNCTION__,
                   "Object is not a BoundCRS or a CoordinateOperation");
    return nullptr;
}

// ---------------------------------------------------------------------------

/** \brief Return the hub CRS of a BoundCRS or the target CRS of a
 * CoordinateOperation.
 *
 * The returned object must be unreferenced with proj_obj_unref() after
 * use.
 * It should be used by at most one thread at a time.
 *
 * @param obj Objet of type BoundCRS or CoordinateOperation (must not be NULL)
 * @return Object that must be unreferenced with proj_obj_unref(), or NULL
 * in case of error, or missing target CRS.
 */
PJ_OBJ *proj_obj_get_target_crs(PJ_OBJ *obj) {
    assert(obj);
    auto boundCRS = nn_dynamic_pointer_cast<BoundCRS>(obj->obj);
    if (boundCRS) {
        return new PJ_OBJ(obj->ctx, boundCRS->hubCRS());
    }
    auto co = nn_dynamic_pointer_cast<CoordinateOperation>(obj->obj);
    if (co) {
        auto targetCRS = co->targetCRS();
        if (targetCRS) {
            return new PJ_OBJ(obj->ctx, NN_CHECK_ASSERT(targetCRS));
        }
        return nullptr;
    }
    proj_log_error(obj->ctx, __FUNCTION__,
                   "Object is not a BoundCRS or a CoordinateOperation");
    return nullptr;
}

// ---------------------------------------------------------------------------

static PROJ_STRING_LIST set_to_string_list(const std::set<std::string> &set) {
    auto ret = new char *[set.size() + 1];
    size_t i = 0;
    for (const auto &str : set) {
        ret[i] = new char[str.size() + 1];
        std::memcpy(ret[i], str.c_str(), str.size() + 1);
        i++;
    }
    ret[i] = nullptr;
    return ret;
}

// ---------------------------------------------------------------------------

/** \brief Return the list of authorities used in the database.
 *
 * The returned list is NULL terminated and must be freed with
 * proj_free_string_list().
 *
 * @param ctx PROJ context, or NULL for default context
 *
 * @return a NULL terminated list of NUL-terminated strings that must be
 * freed with proj_free_string_list(), or NULL in case of error.
 */
PROJ_STRING_LIST proj_get_authorities_from_database(PJ_CONTEXT *ctx) {
    try {
        if (ctx->cpp_context == nullptr) {
            ctx->cpp_context =
                new projCppContext(DatabaseContext::createWithPJContext(ctx));
        }
        return set_to_string_list(
            ctx->cpp_context->databaseContext->getAuthorities());
    } catch (const std::exception &e) {
        proj_log_error(ctx, __FUNCTION__, e.what());
    }
    return nullptr;
}

// ---------------------------------------------------------------------------

/** \brief Returns the set of authority codes of the given object type.
 *
 * The returned list is NULL terminated and must be freed with
 * proj_free_string_list().
 *
 * @param ctx PROJ context, or NULL for default context.
 * @param auth_name Authority name (must not be NULL)
 * @param type Object type.
 * @param allow_deprecated whether we should return deprecated objects as well.
 *
 * @return a NULL terminated list of NUL-terminated strings that must be
 * freed with proj_free_string_list(), or NULL in case of error.
 */
PROJ_STRING_LIST proj_get_codes_from_database(PJ_CONTEXT *ctx,
                                              const char *auth_name,
                                              PJ_OBJ_TYPE type,
                                              int allow_deprecated) {
    assert(auth_name);
    try {
        if (ctx->cpp_context == nullptr) {
            ctx->cpp_context =
                new projCppContext(DatabaseContext::createWithPJContext(ctx));
        }
        auto factory = AuthorityFactory::create(
            ctx->cpp_context->databaseContext, auth_name);

        AuthorityFactory::ObjectType typeInternal =
            AuthorityFactory::ObjectType::CRS;
        switch (type) {
        case PJ_OBJ_TYPE_ELLIPSOID:
            typeInternal = AuthorityFactory::ObjectType::ELLIPSOID;
            break;

        case PJ_OBJ_TYPE_GEODETIC_REFERENCE_FRAME:
        case PJ_OBJ_TYPE_DYNAMIC_GEODETIC_REFERENCE_FRAME:
            typeInternal =
                AuthorityFactory::ObjectType::GEODETIC_REFERENCE_FRAME;
            break;

        case PJ_OBJ_TYPE_VERTICAL_REFERENCE_FRAME:
        case PJ_OBJ_TYPE_DYNAMIC_VERTICAL_REFERENCE_FRAME:
            typeInternal =
                AuthorityFactory::ObjectType::VERTICAL_REFERENCE_FRAME;
            break;

        case PJ_OBJ_TYPE_DATUM_ENSEMBLE:
            typeInternal = AuthorityFactory::ObjectType::DATUM;
            break;

        case PJ_OBJ_TYPE_GEODETIC_CRS:
            typeInternal = AuthorityFactory::ObjectType::GEODETIC_CRS;
            break;

        case PJ_OBJ_TYPE_GEOGRAPHIC_CRS:
            typeInternal = AuthorityFactory::ObjectType::GEOGRAPHIC_CRS;
            break;

        case PJ_OBJ_TYPE_VERTICAL_CRS:
            typeInternal = AuthorityFactory::ObjectType::VERTICAL_CRS;
            break;

        case PJ_OBJ_TYPE_PROJECTED_CRS:
            typeInternal = AuthorityFactory::ObjectType::PROJECTED_CRS;
            break;

        case PJ_OBJ_TYPE_COMPOUND_CRS:
            typeInternal = AuthorityFactory::ObjectType::COMPOUND_CRS;
            break;

        case PJ_OBJ_TYPE_TEMPORAL_CRS:
            return nullptr;

        case PJ_OBJ_TYPE_BOUND_CRS:
            return nullptr;

        case PJ_OBJ_TYPE_OTHER_CRS:
            typeInternal = AuthorityFactory::ObjectType::CRS;
            break;

        case PJ_OBJ_TYPE_CONVERSION:
            typeInternal = AuthorityFactory::ObjectType::CONVERSION;
            break;

        case PJ_OBJ_TYPE_TRANSFORMATION:
            typeInternal = AuthorityFactory::ObjectType::TRANSFORMATION;
            break;

        case PJ_OBJ_TYPE_CONCATENATED_OPERATION:
            typeInternal = AuthorityFactory::ObjectType::CONCATENATED_OPERATION;
            break;

        case PJ_OBJ_TYPE_OTHER_COORDINATE_OPERATION:
            typeInternal = AuthorityFactory::ObjectType::COORDINATE_OPERATION;
            break;

        case PJ_OBJ_TYPE_UNKNOWN:
            return nullptr;
        }

        return set_to_string_list(
            factory->getAuthorityCodes(typeInternal, allow_deprecated != 0));

    } catch (const std::exception &e) {
        proj_log_error(ctx, __FUNCTION__, e.what());
    }
    return nullptr;
}

// ---------------------------------------------------------------------------

/** Free a list of NULL terminated strings. */
void proj_free_string_list(PROJ_STRING_LIST list) {
    if (list) {
        for (size_t i = 0; list[i] != nullptr; i++) {
            delete[] list[i];
        }
        delete[] list;
    }
}

// ---------------------------------------------------------------------------

/** \brief Return the Conversion of a DerivedCRS (such as a ProjectedCRS),
 * or the Transformation from the baseCRS to the hubCRS of a BoundCRS
 *
 * The returned object must be unreferenced with proj_obj_unref() after
 * use.
 * It should be used by at most one thread at a time.
 *
 * @param crs Objet of type DerivedCRS or BoundCRSs (must not be NULL)
 * @param pMethodName Pointer to a string value to store the method
 * (projection) name. or NULL
 * @param pMethodAuthorityName Pointer to a string value to store the method
 * authority name. or NULL
 * @param pMethodCode Pointer to a string value to store the method
 * code. or NULL
 * @return Object of type SingleOperation that must be unreferenced with
 * proj_obj_unref(), or NULL in case of error.
 */
PJ_OBJ *proj_obj_crs_get_coordoperation(PJ_OBJ *crs, const char **pMethodName,
                                        const char **pMethodAuthorityName,
                                        const char **pMethodCode) {
    assert(crs);
    SingleOperationPtr co;

    auto derivedCRS = nn_dynamic_pointer_cast<DerivedCRS>(crs->obj);
    if (derivedCRS) {
        co = derivedCRS->derivingConversion().as_nullable();
    } else {
        auto boundCRS = nn_dynamic_pointer_cast<BoundCRS>(crs->obj);
        if (boundCRS) {
            co = boundCRS->transformation().as_nullable();
        } else {
            proj_log_error(crs->ctx, __FUNCTION__,
                           "Object is not a DerivedCRS or BoundCRS");
            return nullptr;
        }
    }

    const auto &method = co->method();
    const auto &method_ids = method->identifiers();
    if (pMethodName) {
        *pMethodName = method->name()->description()->c_str();
    }
    if (pMethodAuthorityName) {
        if (!method_ids.empty()) {
            *pMethodAuthorityName =
                method->identifiers()[0]->codeSpace()->c_str();
        } else {
            *pMethodAuthorityName = nullptr;
        }
    }
    if (pMethodCode) {
        *pMethodCode = method->identifiers()[0]->code().c_str();
    }
    return new PJ_OBJ(crs->ctx, NN_CHECK_ASSERT(co));
}

// ---------------------------------------------------------------------------

/** \brief Return the number of parameters of a SingleOperation
 *
 * @param coordoperation Objet of type SingleOperation or derived classes
 * (must not be NULL)
 */

int proj_coordoperation_get_param_count(PJ_OBJ *coordoperation) {
    assert(coordoperation);
    auto co = nn_dynamic_pointer_cast<SingleOperation>(coordoperation->obj);
    if (!co) {
        proj_log_error(coordoperation->ctx, __FUNCTION__,
                       "Object is not a SingleOperation");
        return 0;
    }
    return static_cast<int>(co->parameterValues().size());
}

// ---------------------------------------------------------------------------

/** \brief Return the index of a parameter of a SingleOperation
 *
 * @param coordoperation Objet of type SingleOperation or derived classes
 * (must not be NULL)
 * @param name Parameter name. Must not be NULL
 * @return index (>=0), or -1 in case of error.
 */

int proj_coordoperation_get_param_index(PJ_OBJ *coordoperation,
                                        const char *name) {
    assert(coordoperation);
    assert(name);
    auto co = nn_dynamic_pointer_cast<SingleOperation>(coordoperation->obj);
    if (!co) {
        proj_log_error(coordoperation->ctx, __FUNCTION__,
                       "Object is not a SingleOperation");
        return -1;
    }
    int index = 0;
    for (const auto &genParam : co->method()->parameters()) {
        if (Identifier::isEquivalentName(*genParam->name()->description(),
                                         name)) {
            return index;
        }
        index++;
    }
    return -1;
}

// ---------------------------------------------------------------------------

/** \brief Return a parameter of a SingleOperation
 *
 * @param coordoperation Objet of type SingleOperation or derived classes
 * (must not be NULL)
 * @param index Parameter index.
 * @param pName Pointer to a string value to store the parameter name. or NULL
 * @param pNameAuthorityName Pointer to a string value to store the parameter
 * authority name. or NULL
 * @param pNameCode Pointer to a string value to store the parameter
 * code. or NULL
 * @param pValue Pointer to a double value to store the parameter
 * value (if numeric). or NULL
 * @param pValueString Pointer to a string value to store the parameter
 * value (if of type string). or NULL
 * @param pValueUnitConvFactor Pointer to a double value to store the parameter
 * unit conversion factor. or NULL
 * @param pValueUnitName Pointer to a string value to store the parameter
 * unit name. or NULL
 * @return TRUE in case of success.
 */

int proj_coordoperation_get_param(PJ_OBJ *coordoperation, int index,
                                  const char **pName,
                                  const char **pNameAuthorityName,
                                  const char **pNameCode, double *pValue,
                                  const char **pValueString,
                                  double *pValueUnitConvFactor,
                                  const char **pValueUnitName) {
    assert(coordoperation);
    auto co = nn_dynamic_pointer_cast<SingleOperation>(coordoperation->obj);
    if (!co) {
        proj_log_error(coordoperation->ctx, __FUNCTION__,
                       "Object is not a SingleOperation");
        return false;
    }
    const auto &parameters = co->method()->parameters();
    const auto &values = co->parameterValues();
    if (index < 0 || static_cast<size_t>(index) >= parameters.size() ||
        static_cast<size_t>(index) >= values.size()) {
        proj_log_error(coordoperation->ctx, __FUNCTION__, "Invalid index");
        return false;
    }

    const auto &param = parameters[index];
    const auto &param_ids = param->identifiers();
    if (pName) {
        *pName = param->name()->description()->c_str();
    }
    if (pNameAuthorityName) {
        if (!param_ids.empty()) {
            *pNameAuthorityName = param_ids[0]->codeSpace()->c_str();
        } else {
            *pNameAuthorityName = nullptr;
        }
    }
    if (pNameCode) {
        if (!param_ids.empty()) {
            *pNameCode = param_ids[0]->code().c_str();
        } else {
            *pNameCode = nullptr;
        }
    }

    const auto &value = values[index];
    ParameterValuePtr paramValue = nullptr;
    auto opParamValue = nn_dynamic_pointer_cast<OperationParameterValue>(value);
    if (opParamValue) {
        paramValue = opParamValue->parameterValue().as_nullable();
    }
    if (pValue) {
        *pValue = 0;
        if (paramValue) {
            if (paramValue->type() == ParameterValue::Type::MEASURE) {
                *pValue = paramValue->value().value();
            }
        }
    }
    if (pValueString) {
        *pValueString = nullptr;
        if (paramValue) {
            if (paramValue->type() == ParameterValue::Type::FILENAME) {
                *pValueString = paramValue->valueFile().c_str();
            } else if (paramValue->type() == ParameterValue::Type::STRING) {
                *pValueString = paramValue->stringValue().c_str();
            }
        }
    }
    if (pValueUnitConvFactor) {
        *pValueUnitConvFactor = 0;
        if (paramValue) {
            if (paramValue->type() == ParameterValue::Type::MEASURE) {
                *pValueUnitConvFactor =
                    paramValue->value().unit().conversionToSI();
            }
        }
    }
    if (pValueUnitName) {
        *pValueUnitName = nullptr;
        if (paramValue) {
            if (paramValue->type() == ParameterValue::Type::MEASURE) {
                *pValueUnitName = paramValue->value().unit().name().c_str();
            }
        }
    }

    return true;
}

// ---------------------------------------------------------------------------

/** \brief Return the number of grids used by a CoordinateOperation
 *
 * @param coordoperation Objet of type CoordinateOperation or derived classes
 * (must not be NULL)
 */

int proj_coordoperation_get_grid_used_count(PJ_OBJ *coordoperation) {
    assert(coordoperation);
    auto co = nn_dynamic_pointer_cast<CoordinateOperation>(coordoperation->obj);
    if (!co) {
        proj_log_error(coordoperation->ctx, __FUNCTION__,
                       "Object is not a CoordinateOperation");
        return 0;
    }
    if (!coordoperation->gridsNeededAsked) {
        coordoperation->gridsNeededAsked = true;
        const auto gridsNeeded =
            co->gridsNeeded(coordoperation->ctx->cpp_context->databaseContext);
        for (const auto &gridDesc : gridsNeeded) {
            coordoperation->gridsNeeded.emplace_back(gridDesc);
        }
    }
    return static_cast<int>(coordoperation->gridsNeeded.size());
}

// ---------------------------------------------------------------------------

/** \brief Return a parameter of a SingleOperation
 *
 * @param coordoperation Objet of type SingleOperation or derived classes
 * (must not be NULL)
 * @param index Parameter index.
 * @param pShortName Pointer to a string value to store the grid short name. or
 * NULL
 * @param pFullName Pointer to a string value to store the grid full filename.
 * or NULL
 * @param pPackageName Pointer to a string value to store the package name where
 * the grid might be found. or NULL
 * @param pPackageURL Pointer to a string value to store the package URL where
 * the grid might be found. or NULL
 * @param pAvailable Pointer to a int (boolean) value to store whether the grid
 * is available at runtime. or NULL
 * @return TRUE in case of success.
 */

int proj_coordoperation_get_grid_used(PJ_OBJ *coordoperation, int index,
                                      const char **pShortName,
                                      const char **pFullName,
                                      const char **pPackageName,
                                      const char **pPackageURL,
                                      int *pAvailable) {
    const int count = proj_coordoperation_get_grid_used_count(coordoperation);
    if (index < 0 || index >= count) {
        proj_log_error(coordoperation->ctx, __FUNCTION__, "Invalid index");
        return false;
    }

    if (pShortName) {
        *pShortName = coordoperation->gridsNeeded[index].shortName.c_str();
    }

    if (pFullName) {
        *pFullName = coordoperation->gridsNeeded[index].fullName.c_str();
    }

    if (pPackageName) {
        *pPackageName = coordoperation->gridsNeeded[index].packageName.c_str();
    }

    if (pPackageURL) {
        *pPackageURL = coordoperation->gridsNeeded[index].packageURL.c_str();
    }

    if (pAvailable) {
        *pAvailable = coordoperation->gridsNeeded[index].available;
    }

    return true;
}

// ---------------------------------------------------------------------------

/** \brief Opaque object representing an operation factory context. */
struct PJ_OPERATION_FACTORY_CONTEXT {
    //! @cond Doxygen_Suppress
    PJ_CONTEXT *ctx;
    CoordinateOperationContextNNPtr operationContext;

    explicit PJ_OPERATION_FACTORY_CONTEXT(
        PJ_CONTEXT *ctxIn,
        const CoordinateOperationContextNNPtr &operationContextIn)
        : ctx(ctxIn), operationContext(operationContextIn) {}

    PJ_OPERATION_FACTORY_CONTEXT(const PJ_OPERATION_FACTORY_CONTEXT &) = delete;
    PJ_OPERATION_FACTORY_CONTEXT &
    operator=(const PJ_OPERATION_FACTORY_CONTEXT &) = delete;
    //! @endcond
};

// ---------------------------------------------------------------------------

/** \brief Instanciate a context for building coordinate operations between
 * two CRS.
 *
 * The returned object must be unreferenced with
 * proj_operation_factory_context_unref() after use.
 *
 * @param ctx Context, or NULL for default context.
 * @return Object that must be unreferenced with
 * proj_operation_factory_context_unref(), or NULL in
 * case of error.
 */
PJ_OPERATION_FACTORY_CONTEXT *
proj_create_operation_factory_context(PJ_CONTEXT *ctx) {
    try {
        if (ctx->cpp_context == nullptr) {
            ctx->cpp_context =
                new projCppContext(DatabaseContext::createWithPJContext(ctx));
        }
        auto factory = CoordinateOperationFactory::create();
        auto authFactory = AuthorityFactory::create(
            ctx->cpp_context->databaseContext, std::string());
        auto operationContext =
            CoordinateOperationContext::create(authFactory, nullptr, 0.0);
        return new PJ_OPERATION_FACTORY_CONTEXT(ctx, operationContext);
    } catch (const std::exception &e) {
        proj_log_error(ctx, __FUNCTION__, e.what());
    }
    return nullptr;
}

// ---------------------------------------------------------------------------

/** \brief Drops a reference on an object.
 *
 * This method should be called one and exactly one for each function
 * returning a PJ_OPERATION_FACTORY_CONTEXT*
 *
 * @param ctxt Object, or NULL.
 */
void proj_operation_factory_context_unref(PJ_OPERATION_FACTORY_CONTEXT *ctxt) {
    delete ctxt;
}

// ---------------------------------------------------------------------------

/** \brief Set the desired accuracy of the resulting coordinate transformations.
 * @param ctxt Operation factory context. must not be NULL
 * @param accuracy Accuracy in meter (or 0 to disable the filter).
 */
void proj_operation_factory_context_set_desired_accuracy(
    PJ_OPERATION_FACTORY_CONTEXT *ctxt, double accuracy) {
    assert(ctxt);
    ctxt->operationContext->setDesiredAccuracy(accuracy);
}

// ---------------------------------------------------------------------------

/** \brief Set the desired area of interest for the resulting coordinate
 * transformations.
 *
 * For an area of interest crossing the anti-meridian, west_lon will be
 * greater than east_lon.
 *
 * @param ctxt Operation factory context. must not be NULL
 * @param west_lon West longitude (in degrees).
 * @param south_lat South latitude (in degrees).
 * @param east_lon East longitude (in degrees).
 * @param north_lat North latitude (in degrees).
 */
void proj_operation_factory_context_set_area_of_interest(
    PJ_OPERATION_FACTORY_CONTEXT *ctxt, double west_lon, double south_lat,
    double east_lon, double north_lat) {
    assert(ctxt);
    ctxt->operationContext->setAreaOfInterest(
        Extent::createFromBBOX(west_lon, south_lat, east_lon, north_lat));
}

// ---------------------------------------------------------------------------

/** \brief Set how source and target CRS extent should be used
 * when considering if a transformation can be used (only takes effect if
 * no area of interest is explicitly defined).
 *
 * The default is PJ_CRS_EXTENT_SMALLEST.
 *
 * @param ctxt Operation factory context. must not be NULL
 * @param use How source and target CRS extent should be used.
 */
void proj_operation_factory_context_set_crs_extent_use(
    PJ_OPERATION_FACTORY_CONTEXT *ctxt, PROJ_CRS_EXTENT_USE use) {
    assert(ctxt);
    switch (use) {
    case PJ_CRS_EXTENT_NONE:
        ctxt->operationContext->setSourceAndTargetCRSExtentUse(
            CoordinateOperationContext::SourceTargetCRSExtentUse::NONE);
        break;

    case PJ_CRS_EXTENT_BOTH:
        ctxt->operationContext->setSourceAndTargetCRSExtentUse(
            CoordinateOperationContext::SourceTargetCRSExtentUse::BOTH);
        break;

    case PJ_CRS_EXTENT_INTERSECTION:
        ctxt->operationContext->setSourceAndTargetCRSExtentUse(
            CoordinateOperationContext::SourceTargetCRSExtentUse::INTERSECTION);
        break;

    case PJ_CRS_EXTENT_SMALLEST:
        ctxt->operationContext->setSourceAndTargetCRSExtentUse(
            CoordinateOperationContext::SourceTargetCRSExtentUse::SMALLEST);
        break;
    }
}

// ---------------------------------------------------------------------------

/** \brief Set the spatial criterion to use when comparing the area of
 * validity of coordinate operations with the area of interest / area of
 * validity of
 * source and target CRS.
 *
 * The default is PROJ_SPATIAL_CRITERION_STRICT_CONTAINMENT.
 *
 * @param ctxt Operation factory context. must not be NULL
 * @param criterion patial criterion to use
 */
void PROJ_DLL proj_operation_factory_context_set_spatial_criterion(
    PJ_OPERATION_FACTORY_CONTEXT *ctxt, PROJ_SPATIAL_CRITERION criterion) {
    assert(ctxt);
    switch (criterion) {
    case PROJ_SPATIAL_CRITERION_STRICT_CONTAINMENT:
        ctxt->operationContext->setSpatialCriterion(
            CoordinateOperationContext::SpatialCriterion::STRICT_CONTAINMENT);
        break;

    case PROJ_SPATIAL_CRITERION_PARTIAL_INTERSECTION:
        ctxt->operationContext->setSpatialCriterion(
            CoordinateOperationContext::SpatialCriterion::PARTIAL_INTERSECTION);
        break;
    }
}

// ---------------------------------------------------------------------------

/** \brief Set how grid availability is used.
 *
 * The default is USE_FOR_SORTING.
 *
 * @param ctxt Operation factory context. must not be NULL
 * @param use how grid availability is used.
 */
void PROJ_DLL proj_operation_factory_context_set_grid_availability_use(
    PJ_OPERATION_FACTORY_CONTEXT *ctxt, PROJ_GRID_AVAILABILITY_USE use) {
    assert(ctxt);
    switch (use) {
    case PROJ_GRID_AVAILABILITY_USED_FOR_SORTING:
        ctxt->operationContext->setGridAvailabilityUse(
            CoordinateOperationContext::GridAvailabilityUse::USE_FOR_SORTING);
        break;

    case PROJ_GRID_AVAILABILITY_DISCARD_OPERATION_IF_MISSING_GRID:
        ctxt->operationContext->setGridAvailabilityUse(
            CoordinateOperationContext::GridAvailabilityUse::
                DISCARD_OPERATION_IF_MISSING_GRID);
        break;

    case PROJ_GRID_AVAILABILITY_IGNORED:
        ctxt->operationContext->setGridAvailabilityUse(
            CoordinateOperationContext::GridAvailabilityUse::
                IGNORE_GRID_AVAILABILITY);
        break;
    }
}

// ---------------------------------------------------------------------------

/** \brief Set whether PROJ alternative grid names should be substituted to
 * the official authority names.
 *
 * The default is true.
 *
 * @param ctxt Operation factory context. must not be NULL
 * @param usePROJNames whether PROJ alternative grid names should be used
 */
void proj_operation_factory_context_set_use_proj_alternative_grid_names(
    PJ_OPERATION_FACTORY_CONTEXT *ctxt, int usePROJNames) {
    assert(ctxt);
    ctxt->operationContext->setUsePROJAlternativeGridNames(usePROJNames != 0);
}

// ---------------------------------------------------------------------------

/** \brief Opaque object representing a set of operation results. */
struct PJ_OPERATION_RESULT {
    //! @cond Doxygen_Suppress
    PJ_CONTEXT *ctx;
    std::vector<CoordinateOperationNNPtr> ops;

    explicit PJ_OPERATION_RESULT(
        PJ_CONTEXT *ctxIn, const std::vector<CoordinateOperationNNPtr> &opsIn)
        : ctx(ctxIn), ops(opsIn) {}

    PJ_OPERATION_RESULT(const PJ_OPERATION_RESULT &) = delete;
    PJ_OPERATION_RESULT &operator=(const PJ_OPERATION_RESULT &) = delete;
    //! @endcond
};

// ---------------------------------------------------------------------------

/** \brief Find a list of CoordinateOperation from source_crs to target_crs.
 *
 * The operations are sorted with the most relevant ones first: by
 * descending
 * area (intersection of the transformation area with the area of interest,
 * or intersection of the transformation with the area of use of the CRS),
 * and
 * by increasing accuracy. Operations with unknown accuracy are sorted last,
 * whatever their area.
 *
 * @param source_crs source CRS. Must not be NULL.
 * @param target_crs source CRS. Must not be NULL.
 * @param operationContext Search context. Must not be NULL.
 * @return a result set that must be unreferenced with
 * proj_operation_result_unref(), or NULL in
 * case of error.
 */
PJ_OPERATION_RESULT *
proj_obj_create_operations(PJ_OBJ *source_crs, PJ_OBJ *target_crs,
                           PJ_OPERATION_FACTORY_CONTEXT *operationContext) {
    assert(source_crs);
    assert(target_crs);
    assert(operationContext);

    auto sourceCRS = nn_dynamic_pointer_cast<CRS>(source_crs->obj);
    if (!sourceCRS) {
        proj_log_error(operationContext->ctx, __FUNCTION__,
                       "source_crs is not a CRS");
        return nullptr;
    }
    auto targetCRS = nn_dynamic_pointer_cast<CRS>(target_crs->obj);
    if (!targetCRS) {
        proj_log_error(operationContext->ctx, __FUNCTION__,
                       "target_crs is not a CRS");
        return nullptr;
    }

    try {
        auto factory = CoordinateOperationFactory::create();
        return new PJ_OPERATION_RESULT(
            operationContext->ctx,
            factory->createOperations(NN_CHECK_ASSERT(sourceCRS),
                                      NN_CHECK_ASSERT(targetCRS),
                                      operationContext->operationContext));
    } catch (const std::exception &e) {
        proj_log_error(operationContext->ctx, __FUNCTION__, e.what());
        return nullptr;
    }
}

// ---------------------------------------------------------------------------

/** \brief Return the number of CoordinateOperation in the result set
 *
 * @param result Objet of type PJ_OPERATION_RESULT (must not be NULL)
 */
int proj_operation_result_get_count(PJ_OPERATION_RESULT *result) {
    assert(result);
    return static_cast<int>(result->ops.size());
}

// ---------------------------------------------------------------------------

/** \brief Return a CoordinateOperation in the result set
 *
 * The returned object must be unreferenced with proj_obj_unref() after
 * use.
 * It should be used by at most one thread at a time.
 *
 * @param result Objet of type PJ_OPERATION_RESULT (must not be NULL)
 * @param index Index
 * @return a new object that must be unreferenced with proj_obj_unref(),
 * or nullptr in case of error.
 */

PJ_OBJ *proj_operation_result_get(PJ_OPERATION_RESULT *result, int index) {
    assert(result);
    if (index < 0 || index >= proj_operation_result_get_count(result)) {
        proj_log_error(result->ctx, __FUNCTION__, "Invalid index");
        return nullptr;
    }
    return new PJ_OBJ(result->ctx, result->ops[index]);
}

// ---------------------------------------------------------------------------

/** \brief Drops a reference on an object.
 *
 * This method should be called one and exactly one for each function
 * returning a PJ_OPERATION_RESULT*
 *
 * @param result Object, or NULL.
 */
void proj_operation_result_unref(PJ_OPERATION_RESULT *result) { delete result; }
