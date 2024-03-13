/******************************************************************************
 *
 * Project:  PROJ
 * Purpose:  Test ISO19111:2019 implementation
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

#include "gtest_include.h"

#include <cstdio>
#include <limits>
#include <math.h>

#include "proj.h"
#include "proj_constants.h"
#include "proj_experimental.h"

#include "proj/common.hpp"
#include "proj/coordinateoperation.hpp"
#include "proj/coordinatesystem.hpp"
#include "proj/crs.hpp"
#include "proj/datum.hpp"
#include "proj/io.hpp"
#include "proj/metadata.hpp"
#include "proj/util.hpp"

#include <sqlite3.h>

#if !defined(_WIN32)
#include <sys/resource.h>
#endif

#ifndef __MINGW32__
#include <thread>
#endif

using namespace osgeo::proj::common;
using namespace osgeo::proj::crs;
using namespace osgeo::proj::cs;
using namespace osgeo::proj::datum;
using namespace osgeo::proj::io;
using namespace osgeo::proj::metadata;
using namespace osgeo::proj::operation;
using namespace osgeo::proj::util;

namespace {

class CApi : public ::testing::Test {

    static void DummyLogFunction(void *, int, const char *) {}

  protected:
    void SetUp() override {
        m_ctxt = proj_context_create();
        proj_log_func(m_ctxt, nullptr, DummyLogFunction);
    }

    void TearDown() override {
        if (m_ctxt)
            proj_context_destroy(m_ctxt);
    }

    static BoundCRSNNPtr createBoundCRS() {
        return BoundCRS::create(
            GeographicCRS::EPSG_4807, GeographicCRS::EPSG_4326,
            Transformation::create(
                PropertyMap(), GeographicCRS::EPSG_4807,
                GeographicCRS::EPSG_4326, nullptr, PropertyMap(),
                {OperationParameter::create(
                    PropertyMap().set(IdentifiedObject::NAME_KEY, "foo"))},
                {ParameterValue::create(
                    Measure(1.0, UnitOfMeasure::SCALE_UNITY))},
                {}));
    }

    static ProjectedCRSNNPtr createProjectedCRS() {
        PropertyMap propertiesCRS;
        propertiesCRS.set(Identifier::CODESPACE_KEY, "EPSG")
            .set(Identifier::CODE_KEY, 32631)
            .set(IdentifiedObject::NAME_KEY, "WGS 84 / UTM zone 31N");
        return ProjectedCRS::create(
            propertiesCRS, GeographicCRS::EPSG_4326,
            Conversion::createUTM(PropertyMap(), 31, true),
            CartesianCS::createEastingNorthing(UnitOfMeasure::METRE));
    }

    static DerivedProjectedCRSNNPtr createDerivedProjectedCRS() {
        auto derivingConversion = Conversion::create(
            PropertyMap().set(IdentifiedObject::NAME_KEY, "unnamed"),
            PropertyMap().set(IdentifiedObject::NAME_KEY, "PROJ unimplemented"),
            std::vector<OperationParameterNNPtr>{},
            std::vector<ParameterValueNNPtr>{});

        return DerivedProjectedCRS::create(
            PropertyMap().set(IdentifiedObject::NAME_KEY,
                              "derived projectedCRS"),
            createProjectedCRS(), derivingConversion,
            CartesianCS::createEastingNorthing(UnitOfMeasure::METRE));
    }

    static VerticalCRSNNPtr createVerticalCRS() {
        PropertyMap propertiesVDatum;
        propertiesVDatum.set(Identifier::CODESPACE_KEY, "EPSG")
            .set(Identifier::CODE_KEY, 5101)
            .set(IdentifiedObject::NAME_KEY, "Ordnance Datum Newlyn");
        auto vdatum = VerticalReferenceFrame::create(propertiesVDatum);
        PropertyMap propertiesCRS;
        propertiesCRS.set(Identifier::CODESPACE_KEY, "EPSG")
            .set(Identifier::CODE_KEY, 5701)
            .set(IdentifiedObject::NAME_KEY, "ODN height");
        return VerticalCRS::create(
            propertiesCRS, vdatum,
            VerticalCS::createGravityRelatedHeight(UnitOfMeasure::METRE));
    }

    static CompoundCRSNNPtr createCompoundCRS() {
        PropertyMap properties;
        properties.set(Identifier::CODESPACE_KEY, "codespace")
            .set(Identifier::CODE_KEY, "code")
            .set(IdentifiedObject::NAME_KEY, "horizontal + vertical");
        return CompoundCRS::create(
            properties,
            std::vector<CRSNNPtr>{createProjectedCRS(), createVerticalCRS()});
    }

    PJ_CONTEXT *m_ctxt = nullptr;

    struct ObjectKeeper {
        PJ *m_obj = nullptr;
        explicit ObjectKeeper(PJ *obj) : m_obj(obj) {}
        ~ObjectKeeper() { proj_destroy(m_obj); }
        void clear() {
            proj_destroy(m_obj);
            m_obj = nullptr;
        }

        ObjectKeeper(const ObjectKeeper &) = delete;
        ObjectKeeper &operator=(const ObjectKeeper &) = delete;
    };

    struct PjContextKeeper {
        PJ_CONTEXT *m_ctxt = nullptr;
        explicit PjContextKeeper(PJ_CONTEXT *ctxt) : m_ctxt(ctxt) {}
        ~PjContextKeeper() { proj_context_destroy(m_ctxt); }

        PjContextKeeper(const PjContextKeeper &) = delete;
        PjContextKeeper &operator=(const PjContextKeeper &) = delete;
    };

    struct ContextKeeper {
        PJ_OPERATION_FACTORY_CONTEXT *m_op_ctxt = nullptr;
        explicit ContextKeeper(PJ_OPERATION_FACTORY_CONTEXT *op_ctxt)
            : m_op_ctxt(op_ctxt) {}
        ~ContextKeeper() { proj_operation_factory_context_destroy(m_op_ctxt); }

        ContextKeeper(const ContextKeeper &) = delete;
        ContextKeeper &operator=(const ContextKeeper &) = delete;
    };

    struct ObjListKeeper {
        PJ_OBJ_LIST *m_res = nullptr;
        explicit ObjListKeeper(PJ_OBJ_LIST *res) : m_res(res) {}
        ~ObjListKeeper() { proj_list_destroy(m_res); }

        ObjListKeeper(const ObjListKeeper &) = delete;
        ObjListKeeper &operator=(const ObjListKeeper &) = delete;
    };
};

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_create) {
    proj_destroy(nullptr);
    EXPECT_EQ(proj_create(m_ctxt, "invalid"), nullptr);
    {
        auto obj =
            proj_create(m_ctxt, GeographicCRS::EPSG_4326
                                    ->exportToWKT(WKTFormatter::create().get())
                                    .c_str());
        ObjectKeeper keeper(obj);
        EXPECT_NE(obj, nullptr);

        // Check that functions that operate on 'non-C++' PJ don't crash
        constexpr double DEG_TO_RAD = .017453292519943296;
        PJ_COORD coord1;
        coord1.xyzt.x = 2 * DEG_TO_RAD;
        coord1.xyzt.y = 49 * DEG_TO_RAD;
        coord1.xyzt.z = 0;
        coord1.xyzt.t = 0;
        PJ_COORD coord2;
        coord2.xyzt.x = 2 * DEG_TO_RAD;
        coord2.xyzt.y = 50 * DEG_TO_RAD;
        coord2.xyzt.z = 0;
        coord2.xyzt.t = 0;
        EXPECT_EQ(proj_trans(obj, PJ_FWD, coord1).xyzt.x,
                  std::numeric_limits<double>::infinity());

        // and those ones actually work just fine
        EXPECT_NEAR(proj_geod(obj, coord1, coord2).xyzt.x, 111219.409, 1e-3);
        EXPECT_NEAR(proj_lp_dist(obj, coord1, coord2), 111219.409, 1e-3);

        auto info = proj_pj_info(obj);
        EXPECT_EQ(info.id, nullptr);
        ASSERT_NE(info.description, nullptr);
        EXPECT_EQ(info.description, std::string("WGS 84"));
        ASSERT_NE(info.definition, nullptr);
        EXPECT_EQ(info.definition, std::string(""));
    }
    {
        auto obj = proj_create(m_ctxt, "EPSG:4326");
        ObjectKeeper keeper(obj);
        EXPECT_NE(obj, nullptr);
    }
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_create_from_wkt) {

    {
        EXPECT_EQ(
            proj_create_from_wkt(m_ctxt, "invalid", nullptr, nullptr, nullptr),
            nullptr);
    }
    {
        PROJ_STRING_LIST warningList = nullptr;
        PROJ_STRING_LIST errorList = nullptr;
        EXPECT_EQ(proj_create_from_wkt(m_ctxt, "invalid", nullptr, &warningList,
                                       &errorList),
                  nullptr);
        EXPECT_EQ(warningList, nullptr);
        proj_string_list_destroy(warningList);
        EXPECT_NE(errorList, nullptr);
        proj_string_list_destroy(errorList);
    }
    {
        auto obj = proj_create_from_wkt(
            m_ctxt,
            GeographicCRS::EPSG_4326->exportToWKT(WKTFormatter::create().get())
                .c_str(),
            nullptr, nullptr, nullptr);
        ObjectKeeper keeper(obj);
        EXPECT_NE(obj, nullptr);
    }
    {
        auto obj = proj_create_from_wkt(
            m_ctxt,
            "GEOGCS[\"WGS 84\",\n"
            "    DATUM[\"WGS_1984\",\n"
            "        SPHEROID[\"WGS 84\",6378137,298.257223563,\"unused\"]],\n"
            "    PRIMEM[\"Greenwich\",0],\n"
            "    UNIT[\"degree\",0.0174532925199433]]",
            nullptr, nullptr, nullptr);
        ObjectKeeper keeper(obj);
        EXPECT_NE(obj, nullptr);
    }
    {
        PROJ_STRING_LIST warningList = nullptr;
        PROJ_STRING_LIST errorList = nullptr;
        auto obj = proj_create_from_wkt(
            m_ctxt,
            "GEOGCS[\"WGS 84\",\n"
            "    DATUM[\"WGS_1984\",\n"
            "        SPHEROID[\"WGS 84\",6378137,298.257223563,\"unused\"]],\n"
            "    PRIMEM[\"Greenwich\",0],\n"
            "    UNIT[\"degree\",0.0174532925199433]]",
            nullptr, &warningList, &errorList);
        ObjectKeeper keeper(obj);
        EXPECT_NE(obj, nullptr);
        EXPECT_EQ(warningList, nullptr);
        proj_string_list_destroy(warningList);
        EXPECT_NE(errorList, nullptr);
        proj_string_list_destroy(errorList);
    }
    {
        PROJ_STRING_LIST warningList = nullptr;
        PROJ_STRING_LIST errorList = nullptr;
        const char *const options[] = {"STRICT=NO", nullptr};
        auto obj = proj_create_from_wkt(
            m_ctxt,
            "GEOGCS[\"WGS 84\",\n"
            "    DATUM[\"WGS_1984\",\n"
            "        SPHEROID[\"WGS 84\",6378137,298.257223563,\"unused\"]],\n"
            "    PRIMEM[\"Greenwich\",0],\n"
            "    UNIT[\"degree\",0.0174532925199433]]",
            options, &warningList, &errorList);
        ObjectKeeper keeper(obj);
        EXPECT_NE(obj, nullptr);
        EXPECT_EQ(warningList, nullptr);
        proj_string_list_destroy(warningList);
        EXPECT_NE(errorList, nullptr);
        proj_string_list_destroy(errorList);
    }
    {
        PROJ_STRING_LIST warningList = nullptr;
        PROJ_STRING_LIST errorList = nullptr;
        const char *const options[] = {
            "UNSET_IDENTIFIERS_IF_INCOMPATIBLE_DEF=NO", nullptr};
        auto wkt = "PROJCS[\"Merchich / Nord Maroc\","
                   "    GEOGCS[\"Merchich\","
                   "        DATUM[\"Merchich\","
                   "            SPHEROID[\"Clarke 1880 (IGN)\","
                   "6378249.2,293.466021293627]],"
                   "        PRIMEM[\"Greenwich\",0],"
                   "        UNIT[\"grad\",0.015707963267949,"
                   "            AUTHORITY[\"EPSG\",\"9105\"]],"
                   "        AUTHORITY[\"EPSG\",\"4261\"]],"
                   "    PROJECTION[\"Lambert_Conformal_Conic_1SP\"],"
                   "    PARAMETER[\"latitude_of_origin\",37],"
                   "    PARAMETER[\"central_meridian\",-6],"
                   "    PARAMETER[\"scale_factor\",0.999625769],"
                   "    PARAMETER[\"false_easting\",500000],"
                   "    PARAMETER[\"false_northing\",300000],"
                   "    UNIT[\"metre\",1,"
                   "        AUTHORITY[\"EPSG\",\"9001\"]],"
                   "    AXIS[\"Easting\",EAST],"
                   "    AXIS[\"Northing\",NORTH]]";
        auto obj = proj_create_from_wkt(m_ctxt, wkt, options, &warningList,
                                        &errorList);
        ObjectKeeper keeper(obj);
        EXPECT_NE(obj, nullptr);
        EXPECT_EQ(warningList, nullptr);
        proj_string_list_destroy(warningList);
        EXPECT_EQ(errorList, nullptr);
        proj_string_list_destroy(errorList);
    }
    {
        PROJ_STRING_LIST warningList = nullptr;
        PROJ_STRING_LIST errorList = nullptr;
        auto obj = proj_create_from_wkt(
            m_ctxt,
            GeographicCRS::EPSG_4326->exportToWKT(WKTFormatter::create().get())
                .c_str(),
            nullptr, &warningList, &errorList);
        ObjectKeeper keeper(obj);
        EXPECT_NE(obj, nullptr);
        EXPECT_EQ(warningList, nullptr);
        EXPECT_EQ(errorList, nullptr);
    }
    // Warnings: missing projection parameters
    {
        PROJ_STRING_LIST warningList = nullptr;
        PROJ_STRING_LIST errorList = nullptr;
        auto obj = proj_create_from_wkt(
            m_ctxt,
            "PROJCS[\"test\",\n"
            "  GEOGCS[\"WGS 84\",\n"
            "    DATUM[\"WGS_1984\",\n"
            "        SPHEROID[\"WGS 84\",6378137,298.257223563]],\n"
            "    PRIMEM[\"Greenwich\",0],\n"
            "    UNIT[\"degree\",0.0174532925199433]],\n"
            "  PROJECTION[\"Transverse_Mercator\"],\n"
            "  PARAMETER[\"latitude_of_origin\",31],\n"
            "  UNIT[\"metre\",1]]",
            nullptr, &warningList, &errorList);
        ObjectKeeper keeper(obj);
        EXPECT_NE(obj, nullptr);
        EXPECT_NE(warningList, nullptr);
        proj_string_list_destroy(warningList);
        EXPECT_EQ(errorList, nullptr);
        proj_string_list_destroy(errorList);
    }
    {
        auto obj = proj_create_from_wkt(
            m_ctxt,
            "PROJCS[\"test\",\n"
            "  GEOGCS[\"WGS 84\",\n"
            "    DATUM[\"WGS_1984\",\n"
            "        SPHEROID[\"WGS 84\",6378137,298.257223563]],\n"
            "    PRIMEM[\"Greenwich\",0],\n"
            "    UNIT[\"degree\",0.0174532925199433]],\n"
            "  PROJECTION[\"Transverse_Mercator\"],\n"
            "  PARAMETER[\"latitude_of_origin\",31],\n"
            "  UNIT[\"metre\",1]]",
            nullptr, nullptr, nullptr);
        ObjectKeeper keeper(obj);
        EXPECT_NE(obj, nullptr);
    }
    {
        // Invalid ellipsoidal parameter (semi major axis)
        auto obj = proj_create_from_wkt(
            m_ctxt,
            "GEOGCS[\"test\",\n"
            "    DATUM[\"test\",\n"
            "        SPHEROID[\"test\",0,298.257223563,\"unused\"]],\n"
            "    PRIMEM[\"Greenwich\",0],\n"
            "    UNIT[\"degree\",0.0174532925199433]]",
            nullptr, nullptr, nullptr);
        ObjectKeeper keeper(obj);
        EXPECT_EQ(obj, nullptr);
    }
    {
        // Invalid ellipsoidal parameter (inverse flattening)
        auto obj = proj_create_from_wkt(
            m_ctxt,
            "GEOGCS[\"test\",\n"
            "    DATUM[\"test\",\n"
            "        SPHEROID[\"test\",6378137,-1,\"unused\"]],\n"
            "    PRIMEM[\"Greenwich\",0],\n"
            "    UNIT[\"degree\",0.0174532925199433]]",
            nullptr, nullptr, nullptr);
        ObjectKeeper keeper(obj);
        EXPECT_EQ(obj, nullptr);
    }
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_as_wkt) {
    auto obj = proj_create_from_wkt(
        m_ctxt,
        GeographicCRS::EPSG_4326->exportToWKT(WKTFormatter::create().get())
            .c_str(),
        nullptr, nullptr, nullptr);
    ObjectKeeper keeper(obj);
    ASSERT_NE(obj, nullptr);

    {
        auto wkt = proj_as_wkt(m_ctxt, obj, PJ_WKT2_2019, nullptr);
        ASSERT_NE(wkt, nullptr);
        EXPECT_TRUE(std::string(wkt).find("GEOGCRS[") == 0) << wkt;
    }

    {
        auto wkt = proj_as_wkt(m_ctxt, obj, PJ_WKT2_2019_SIMPLIFIED, nullptr);
        ASSERT_NE(wkt, nullptr);
        EXPECT_TRUE(std::string(wkt).find("GEOGCRS[") == 0) << wkt;
        EXPECT_TRUE(std::string(wkt).find("ANGULARUNIT[") == std::string::npos)
            << wkt;
    }

    {
        auto wkt = proj_as_wkt(m_ctxt, obj, PJ_WKT2_2015, nullptr);
        ASSERT_NE(wkt, nullptr);
        EXPECT_TRUE(std::string(wkt).find("GEODCRS[") == 0) << wkt;
    }

    {
        auto wkt = proj_as_wkt(m_ctxt, obj, PJ_WKT2_2015_SIMPLIFIED, nullptr);
        ASSERT_NE(wkt, nullptr);
        EXPECT_TRUE(std::string(wkt).find("GEODCRS[") == 0) << wkt;
        EXPECT_TRUE(std::string(wkt).find("ANGULARUNIT[") == std::string::npos)
            << wkt;
    }

    {
        auto wkt = proj_as_wkt(m_ctxt, obj, PJ_WKT1_GDAL, nullptr);
        ASSERT_NE(wkt, nullptr);
        EXPECT_TRUE(std::string(wkt).find("GEOGCS[\"WGS 84\"") == 0) << wkt;
    }

    {
        auto wkt = proj_as_wkt(m_ctxt, obj, PJ_WKT1_ESRI, nullptr);
        ASSERT_NE(wkt, nullptr);
        EXPECT_TRUE(std::string(wkt).find("GEOGCS[\"GCS_WGS_1984\"") == 0)
            << wkt;
    }

    // MULTILINE=NO
    {
        const char *const options[] = {"MULTILINE=NO", nullptr};
        auto wkt = proj_as_wkt(m_ctxt, obj, PJ_WKT1_GDAL, options);
        ASSERT_NE(wkt, nullptr);
        EXPECT_TRUE(std::string(wkt).find("\n") == std::string::npos) << wkt;
    }

    // INDENTATION_WIDTH=2
    {
        const char *const options[] = {"INDENTATION_WIDTH=2", nullptr};
        auto wkt = proj_as_wkt(m_ctxt, obj, PJ_WKT1_GDAL, options);
        ASSERT_NE(wkt, nullptr);
        EXPECT_TRUE(std::string(wkt).find("\n  DATUM") != std::string::npos)
            << wkt;
    }

    // OUTPUT_AXIS=NO
    {
        const char *const options[] = {"OUTPUT_AXIS=NO", nullptr};
        auto wkt = proj_as_wkt(m_ctxt, obj, PJ_WKT1_GDAL, options);
        ASSERT_NE(wkt, nullptr);
        EXPECT_TRUE(std::string(wkt).find("AXIS") == std::string::npos) << wkt;
    }

    // OUTPUT_AXIS=AUTO
    {
        const char *const options[] = {"OUTPUT_AXIS=AUTO", nullptr};
        auto wkt = proj_as_wkt(m_ctxt, obj, PJ_WKT1_GDAL, options);
        ASSERT_NE(wkt, nullptr);
        EXPECT_TRUE(std::string(wkt).find("AXIS") == std::string::npos) << wkt;
    }

    // OUTPUT_AXIS=YES
    {
        const char *const options[] = {"OUTPUT_AXIS=YES", nullptr};
        auto wkt = proj_as_wkt(m_ctxt, obj, PJ_WKT1_GDAL, options);
        ASSERT_NE(wkt, nullptr);
        EXPECT_TRUE(std::string(wkt).find("AXIS") != std::string::npos) << wkt;
    }

    auto crs4979 = proj_create_from_wkt(
        m_ctxt,
        GeographicCRS::EPSG_4979->exportToWKT(WKTFormatter::create().get())
            .c_str(),
        nullptr, nullptr, nullptr);
    ObjectKeeper keeper_crs4979(crs4979);
    ASSERT_NE(crs4979, nullptr);

    EXPECT_EQ(proj_as_wkt(m_ctxt, crs4979, PJ_WKT1_GDAL, nullptr), nullptr);

    // STRICT=NO
    {
        const char *const options[] = {"STRICT=NO", nullptr};
        auto wkt = proj_as_wkt(m_ctxt, crs4979, PJ_WKT1_GDAL, options);
        ASSERT_NE(wkt, nullptr);
        EXPECT_TRUE(std::string(wkt).find("GEOGCS[\"WGS 84\"") == 0) << wkt;
    }

    // ALLOW_ELLIPSOIDAL_HEIGHT_AS_VERTICAL_CRS=YES
    {
        const char *const options[] = {
            "ALLOW_ELLIPSOIDAL_HEIGHT_AS_VERTICAL_CRS=YES", nullptr};
        auto wkt = proj_as_wkt(m_ctxt, crs4979, PJ_WKT1_GDAL, options);
        ASSERT_NE(wkt, nullptr);
        EXPECT_TRUE(std::string(wkt).find(
                        "COMPD_CS[\"WGS 84 + Ellipsoid (metre)\"") == 0)
            << wkt;
    }

    // ALLOW_LINUNIT_NODE default value
    {
        auto wkt = proj_as_wkt(m_ctxt, crs4979, PJ_WKT1_ESRI, nullptr);
        ASSERT_NE(wkt, nullptr);
        EXPECT_TRUE(std::string(wkt).find("LINUNIT") != std::string::npos)
            << wkt;
    }

    // ALLOW_LINUNIT_NODE=NO
    {
        const char *const options[] = {"ALLOW_LINUNIT_NODE=NO", nullptr};
        auto wkt = proj_as_wkt(m_ctxt, crs4979, PJ_WKT1_ESRI, options);
        ASSERT_NE(wkt, nullptr);
        EXPECT_TRUE(std::string(wkt).find("LINUNIT") == std::string::npos)
            << wkt;
    }

    // unsupported option
    {
        const char *const options[] = {"unsupported=yes", nullptr};
        auto wkt = proj_as_wkt(m_ctxt, obj, PJ_WKT2_2019, options);
        EXPECT_EQ(wkt, nullptr);
    }
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_as_wkt_check_db_use) {
    auto obj = proj_create_from_wkt(
        m_ctxt,
        "GEOGCS[\"AGD66\",DATUM[\"Australian_Geodetic_Datum_1966\","
        "SPHEROID[\"Australian National Spheroid\",6378160,298.25]],"
        "PRIMEM[\"Greenwich\",0],UNIT[\"degree\",0.0174532925199433]]",
        nullptr, nullptr, nullptr);
    ObjectKeeper keeper(obj);
    ASSERT_NE(obj, nullptr);

    auto wkt = proj_as_wkt(m_ctxt, obj, PJ_WKT1_ESRI, nullptr);
    EXPECT_EQ(std::string(wkt),
              "GEOGCS[\"GCS_Australian_1966\",DATUM[\"D_Australian_1966\","
              "SPHEROID[\"Australian\",6378160.0,298.25]],"
              "PRIMEM[\"Greenwich\",0.0],"
              "UNIT[\"Degree\",0.0174532925199433]]");
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_as_wkt_incompatible_WKT1) {
    auto wkt = createBoundCRS()->exportToWKT(WKTFormatter::create().get());
    auto obj =
        proj_create_from_wkt(m_ctxt, wkt.c_str(), nullptr, nullptr, nullptr);
    ObjectKeeper keeper(obj);
    ASSERT_NE(obj, nullptr) << wkt;

    auto wkt1_GDAL = proj_as_wkt(m_ctxt, obj, PJ_WKT1_GDAL, nullptr);
    ASSERT_EQ(wkt1_GDAL, nullptr);
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_as_proj_string) {
    auto obj = proj_create_from_wkt(
        m_ctxt,
        GeographicCRS::EPSG_4326->exportToWKT(WKTFormatter::create().get())
            .c_str(),
        nullptr, nullptr, nullptr);
    ObjectKeeper keeper(obj);
    ASSERT_NE(obj, nullptr);

    {
        auto proj_5 = proj_as_proj_string(m_ctxt, obj, PJ_PROJ_5, nullptr);
        ASSERT_NE(proj_5, nullptr);
        EXPECT_EQ(std::string(proj_5),
                  "+proj=longlat +datum=WGS84 +no_defs +type=crs");
    }
    {
        auto proj_4 = proj_as_proj_string(m_ctxt, obj, PJ_PROJ_4, nullptr);
        ASSERT_NE(proj_4, nullptr);
        EXPECT_EQ(std::string(proj_4),
                  "+proj=longlat +datum=WGS84 +no_defs +type=crs");
    }
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_as_proj_string_incompatible_WKT1) {
    auto obj = proj_create_from_wkt(
        m_ctxt,
        createBoundCRS()->exportToWKT(WKTFormatter::create().get()).c_str(),
        nullptr, nullptr, nullptr);
    ObjectKeeper keeper(obj);
    ASSERT_NE(obj, nullptr);

    auto str = proj_as_proj_string(m_ctxt, obj, PJ_PROJ_5, nullptr);
    ASSERT_EQ(str, nullptr);
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_as_proj_string_approx_tmerc_option_yes) {
    auto obj = proj_create(m_ctxt, "+proj=tmerc +type=crs");
    ObjectKeeper keeper(obj);
    ASSERT_NE(obj, nullptr);

    const char *options[] = {"USE_APPROX_TMERC=YES", nullptr};
    auto str = proj_as_proj_string(m_ctxt, obj, PJ_PROJ_4, options);
    ASSERT_NE(str, nullptr);
    EXPECT_EQ(str,
              std::string("+proj=tmerc +approx +lat_0=0 +lon_0=0 +k=1 +x_0=0 "
                          "+y_0=0 +datum=WGS84 +units=m +no_defs +type=crs"));
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_crs_create_bound_crs_to_WGS84) {
    auto crs = proj_create_from_database(m_ctxt, "EPSG", "4807",
                                         PJ_CATEGORY_CRS, false, nullptr);
    ObjectKeeper keeper(crs);
    ASSERT_NE(crs, nullptr);

    auto res = proj_crs_create_bound_crs_to_WGS84(m_ctxt, crs, nullptr);
    ObjectKeeper keeper_res(res);
    ASSERT_NE(res, nullptr);

    auto proj_4 = proj_as_proj_string(m_ctxt, res, PJ_PROJ_4, nullptr);
    ASSERT_NE(proj_4, nullptr);
    EXPECT_EQ(std::string(proj_4),
              "+proj=longlat +ellps=clrk80ign +pm=paris "
              "+towgs84=-168,-60,320,0,0,0,0 +no_defs +type=crs");

    auto base_crs = proj_get_source_crs(m_ctxt, res);
    ObjectKeeper keeper_base_crs(base_crs);
    ASSERT_NE(base_crs, nullptr);

    auto hub_crs = proj_get_target_crs(m_ctxt, res);
    ObjectKeeper keeper_hub_crs(hub_crs);
    ASSERT_NE(hub_crs, nullptr);

    auto transf = proj_crs_get_coordoperation(m_ctxt, res);
    ObjectKeeper keeper_transf(transf);
    ASSERT_NE(transf, nullptr);

    std::vector<double> values(7, 0);
    EXPECT_TRUE(proj_coordoperation_get_towgs84_values(m_ctxt, transf,
                                                       values.data(), 7, true));
    auto expected = std::vector<double>{-168, -60, 320, 0, 0, 0, 0};
    EXPECT_EQ(values, expected);

    auto res2 = proj_crs_create_bound_crs(m_ctxt, base_crs, hub_crs, transf);
    ObjectKeeper keeper_res2(res2);
    ASSERT_NE(res2, nullptr);

    EXPECT_TRUE(proj_is_equivalent_to(res, res2, PJ_COMP_STRICT));
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_crs_create_bound_crs_to_WGS84_on_invalid_type) {
    auto wkt = createProjectedCRS()->derivingConversion()->exportToWKT(
        WKTFormatter::create().get());
    auto obj =
        proj_create_from_wkt(m_ctxt, wkt.c_str(), nullptr, nullptr, nullptr);
    ObjectKeeper keeper(obj);
    ASSERT_NE(obj, nullptr) << wkt;

    auto res = proj_crs_create_bound_crs_to_WGS84(m_ctxt, obj, nullptr);
    ASSERT_EQ(res, nullptr);
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_get_name) {
    auto obj = proj_create_from_wkt(
        m_ctxt,
        GeographicCRS::EPSG_4326->exportToWKT(WKTFormatter::create().get())
            .c_str(),
        nullptr, nullptr, nullptr);
    ObjectKeeper keeper(obj);
    ASSERT_NE(obj, nullptr);
    auto name = proj_get_name(obj);
    ASSERT_TRUE(name != nullptr);
    EXPECT_EQ(name, std::string("WGS 84"));
    EXPECT_EQ(name, proj_get_name(obj));
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_get_id_auth_name) {
    auto obj = proj_create_from_wkt(
        m_ctxt,
        GeographicCRS::EPSG_4326->exportToWKT(WKTFormatter::create().get())
            .c_str(),
        nullptr, nullptr, nullptr);
    ObjectKeeper keeper(obj);
    ASSERT_NE(obj, nullptr);
    auto auth = proj_get_id_auth_name(obj, 0);
    ASSERT_TRUE(auth != nullptr);
    EXPECT_EQ(auth, std::string("EPSG"));
    EXPECT_EQ(auth, proj_get_id_auth_name(obj, 0));
    EXPECT_EQ(proj_get_id_auth_name(obj, -1), nullptr);
    EXPECT_EQ(proj_get_id_auth_name(obj, 1), nullptr);
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_get_id_code) {
    auto obj = proj_create_from_wkt(
        m_ctxt,
        GeographicCRS::EPSG_4326->exportToWKT(WKTFormatter::create().get())
            .c_str(),
        nullptr, nullptr, nullptr);
    ObjectKeeper keeper(obj);
    ASSERT_NE(obj, nullptr);
    auto code = proj_get_id_code(obj, 0);
    ASSERT_TRUE(code != nullptr);
    EXPECT_EQ(code, std::string("4326"));
    EXPECT_EQ(code, proj_get_id_code(obj, 0));
    EXPECT_EQ(proj_get_id_code(obj, -1), nullptr);
    EXPECT_EQ(proj_get_id_code(obj, 1), nullptr);
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_get_type) {
    {
        auto obj = proj_create_from_wkt(
            m_ctxt,
            GeographicCRS::EPSG_4326->exportToWKT(WKTFormatter::create().get())
                .c_str(),
            nullptr, nullptr, nullptr);
        ObjectKeeper keeper(obj);
        ASSERT_NE(obj, nullptr);
        EXPECT_EQ(proj_get_type(obj), PJ_TYPE_GEOGRAPHIC_2D_CRS);
    }
    {
        auto obj = proj_create_from_wkt(
            m_ctxt,
            GeographicCRS::EPSG_4979->exportToWKT(WKTFormatter::create().get())
                .c_str(),
            nullptr, nullptr, nullptr);
        ObjectKeeper keeper(obj);
        ASSERT_NE(obj, nullptr);
        EXPECT_EQ(proj_get_type(obj), PJ_TYPE_GEOGRAPHIC_3D_CRS);
    }
    {
        auto obj = proj_create_from_wkt(
            m_ctxt,
            GeographicCRS::EPSG_4978->exportToWKT(WKTFormatter::create().get())
                .c_str(),
            nullptr, nullptr, nullptr);
        ObjectKeeper keeper(obj);
        ASSERT_NE(obj, nullptr);
        EXPECT_EQ(proj_get_type(obj), PJ_TYPE_GEOCENTRIC_CRS);
    }
    {
        auto obj =
            proj_create_from_wkt(m_ctxt,
                                 GeographicCRS::EPSG_4326->datum()
                                     ->exportToWKT(WKTFormatter::create().get())
                                     .c_str(),
                                 nullptr, nullptr, nullptr);
        ObjectKeeper keeper(obj);
        ASSERT_NE(obj, nullptr);
        EXPECT_EQ(proj_get_type(obj), PJ_TYPE_GEODETIC_REFERENCE_FRAME);
    }
    {
        auto obj =
            proj_create_from_wkt(m_ctxt,
                                 GeographicCRS::EPSG_4326->ellipsoid()
                                     ->exportToWKT(WKTFormatter::create().get())
                                     .c_str(),
                                 nullptr, nullptr, nullptr);
        ObjectKeeper keeper(obj);
        ASSERT_NE(obj, nullptr);
        EXPECT_EQ(proj_get_type(obj), PJ_TYPE_ELLIPSOID);
    }
    {
        auto obj =
            proj_create_from_wkt(m_ctxt,
                                 createProjectedCRS()
                                     ->exportToWKT(WKTFormatter::create().get())
                                     .c_str(),
                                 nullptr, nullptr, nullptr);
        ObjectKeeper keeper(obj);
        ASSERT_NE(obj, nullptr);
        EXPECT_EQ(proj_get_type(obj), PJ_TYPE_PROJECTED_CRS);
    }
    {
        auto obj = proj_create_from_wkt(
            m_ctxt,
            createDerivedProjectedCRS()
                ->exportToWKT(
                    WKTFormatter::create(WKTFormatter::Convention::WKT2_2019)
                        .get())
                .c_str(),
            nullptr, nullptr, nullptr);
        ObjectKeeper keeper(obj);
        ASSERT_NE(obj, nullptr);
        EXPECT_EQ(proj_get_type(obj), PJ_TYPE_DERIVED_PROJECTED_CRS);
    }
    {
        auto obj =
            proj_create_from_wkt(m_ctxt,
                                 createVerticalCRS()
                                     ->exportToWKT(WKTFormatter::create().get())
                                     .c_str(),
                                 nullptr, nullptr, nullptr);
        ObjectKeeper keeper(obj);
        ASSERT_NE(obj, nullptr);
        EXPECT_EQ(proj_get_type(obj), PJ_TYPE_VERTICAL_CRS);
    }
    {
        auto wkt = "TDATUM[\"Gregorian calendar\",\n"
                   "     CALENDAR[\"proleptic Gregorian\"],\n"
                   "     TIMEORIGIN[0000-01-01]]";

        auto datum =
            proj_create_from_wkt(m_ctxt, wkt, nullptr, nullptr, nullptr);
        ObjectKeeper keeper(datum);
        ASSERT_NE(datum, nullptr);
        EXPECT_EQ(proj_get_type(datum), PJ_TYPE_TEMPORAL_DATUM);
    }
    {
        auto wkt = "ENGINEERINGDATUM[\"Engineering datum\"]";
        auto datum =
            proj_create_from_wkt(m_ctxt, wkt, nullptr, nullptr, nullptr);
        ObjectKeeper keeper(datum);
        EXPECT_EQ(proj_get_type(datum), PJ_TYPE_ENGINEERING_DATUM);
    }
    {
        auto wkt = "PDATUM[\"Mean Sea Level\",ANCHOR[\"1013.25 hPa at 15°C\"]]";
        auto datum =
            proj_create_from_wkt(m_ctxt, wkt, nullptr, nullptr, nullptr);
        ObjectKeeper keeper(datum);
        EXPECT_EQ(proj_get_type(datum), PJ_TYPE_PARAMETRIC_DATUM);
    }
    {
        auto obj =
            proj_create_from_wkt(m_ctxt,
                                 createVerticalCRS()
                                     ->datum()
                                     ->exportToWKT(WKTFormatter::create().get())
                                     .c_str(),
                                 nullptr, nullptr, nullptr);
        ObjectKeeper keeper(obj);
        ASSERT_NE(obj, nullptr);
        EXPECT_EQ(proj_get_type(obj), PJ_TYPE_VERTICAL_REFERENCE_FRAME);
    }
    {
        auto obj =
            proj_create_from_wkt(m_ctxt,
                                 createProjectedCRS()
                                     ->derivingConversion()
                                     ->exportToWKT(WKTFormatter::create().get())
                                     .c_str(),
                                 nullptr, nullptr, nullptr);
        ObjectKeeper keeper(obj);
        ASSERT_NE(obj, nullptr);
        EXPECT_EQ(proj_get_type(obj), PJ_TYPE_CONVERSION);
    }
    {
        auto obj = proj_create_from_wkt(
            m_ctxt,
            createBoundCRS()->exportToWKT(WKTFormatter::create().get()).c_str(),
            nullptr, nullptr, nullptr);
        ObjectKeeper keeper(obj);
        ASSERT_NE(obj, nullptr);
        EXPECT_EQ(proj_get_type(obj), PJ_TYPE_BOUND_CRS);
    }
    {
        auto obj =
            proj_create_from_wkt(m_ctxt,
                                 createBoundCRS()
                                     ->transformation()
                                     ->exportToWKT(WKTFormatter::create().get())
                                     .c_str(),
                                 nullptr, nullptr, nullptr);
        ObjectKeeper keeper(obj);
        ASSERT_NE(obj, nullptr);
        EXPECT_EQ(proj_get_type(obj), PJ_TYPE_TRANSFORMATION);
    }
    {
        auto obj = proj_create_from_wkt(m_ctxt, "AUTHORITY[\"EPSG\", 4326]",
                                        nullptr, nullptr, nullptr);
        ObjectKeeper keeper(obj);
        ASSERT_NE(obj, nullptr);
        EXPECT_EQ(proj_get_type(obj), PJ_TYPE_UNKNOWN);
    }
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_create_from_database) {
    {
        auto crs = proj_create_from_database(m_ctxt, "EPSG", "-1",
                                             PJ_CATEGORY_CRS, false, nullptr);
        ASSERT_EQ(crs, nullptr);
    }
    {
        auto crs = proj_create_from_database(m_ctxt, "EPSG", "4326",
                                             PJ_CATEGORY_CRS, false, nullptr);
        ASSERT_NE(crs, nullptr);
        ObjectKeeper keeper(crs);
        EXPECT_TRUE(proj_is_crs(crs));
        EXPECT_FALSE(proj_is_deprecated(crs));
        EXPECT_EQ(proj_get_type(crs), PJ_TYPE_GEOGRAPHIC_2D_CRS);

        const char *code = proj_get_id_code(crs, 0);
        ASSERT_NE(code, nullptr);
        EXPECT_EQ(std::string(code), "4326");
    }
    {
        auto crs = proj_create_from_database(m_ctxt, "EPSG", "6871",
                                             PJ_CATEGORY_CRS, false, nullptr);
        ASSERT_NE(crs, nullptr);
        ObjectKeeper keeper(crs);
        EXPECT_TRUE(proj_is_crs(crs));
        EXPECT_EQ(proj_get_type(crs), PJ_TYPE_COMPOUND_CRS);
    }
    {
        auto ellipsoid = proj_create_from_database(
            m_ctxt, "EPSG", "7030", PJ_CATEGORY_ELLIPSOID, false, nullptr);
        ASSERT_NE(ellipsoid, nullptr);
        ObjectKeeper keeper(ellipsoid);
        EXPECT_EQ(proj_get_type(ellipsoid), PJ_TYPE_ELLIPSOID);
    }
    {
        auto pm = proj_create_from_database(
            m_ctxt, "EPSG", "8903", PJ_CATEGORY_PRIME_MERIDIAN, false, nullptr);
        ASSERT_NE(pm, nullptr);
        ObjectKeeper keeper(pm);
        EXPECT_EQ(proj_get_type(pm), PJ_TYPE_PRIME_MERIDIAN);
    }
    {
        auto datum = proj_create_from_database(
            m_ctxt, "EPSG", "6326", PJ_CATEGORY_DATUM, false, nullptr);
        ASSERT_NE(datum, nullptr);
        ObjectKeeper keeper(datum);
        EXPECT_EQ(proj_get_type(datum), PJ_TYPE_GEODETIC_REFERENCE_FRAME);
    }
    {
        auto ensemble = proj_create_from_database(
            m_ctxt, "EPSG", "6326", PJ_CATEGORY_DATUM_ENSEMBLE, false, nullptr);
        ASSERT_NE(ensemble, nullptr);
        ObjectKeeper keeper(ensemble);
        EXPECT_EQ(proj_get_type(ensemble), PJ_TYPE_DATUM_ENSEMBLE);
    }
    {
        // International Terrestrial Reference Frame 2008
        auto datum = proj_create_from_database(
            m_ctxt, "EPSG", "1061", PJ_CATEGORY_DATUM, false, nullptr);
        ASSERT_NE(datum, nullptr);
        ObjectKeeper keeper(datum);
        EXPECT_EQ(proj_get_type(datum),
                  PJ_TYPE_DYNAMIC_GEODETIC_REFERENCE_FRAME);
        EXPECT_EQ(proj_dynamic_datum_get_frame_reference_epoch(m_ctxt, datum),
                  2005.0);
    }
    {
        // Norway Normal Null 2000
        auto datum = proj_create_from_database(
            m_ctxt, "EPSG", "1096", PJ_CATEGORY_DATUM, false, nullptr);
        ASSERT_NE(datum, nullptr);
        ObjectKeeper keeper(datum);
        EXPECT_EQ(proj_get_type(datum),
                  PJ_TYPE_DYNAMIC_VERTICAL_REFERENCE_FRAME);
        EXPECT_EQ(proj_dynamic_datum_get_frame_reference_epoch(m_ctxt, datum),
                  2000.0);
    }
    {
        auto op = proj_create_from_database(m_ctxt, "EPSG", "16031",
                                            PJ_CATEGORY_COORDINATE_OPERATION,
                                            false, nullptr);
        ASSERT_NE(op, nullptr);
        ObjectKeeper keeper(op);
        EXPECT_EQ(proj_get_type(op), PJ_TYPE_CONVERSION);

        auto info = proj_pj_info(op);
        EXPECT_NE(info.id, nullptr);
        EXPECT_EQ(info.id, std::string("utm"));
        ASSERT_NE(info.description, nullptr);
        EXPECT_EQ(info.description, std::string("UTM zone 31N"));
        ASSERT_NE(info.definition, nullptr);
        EXPECT_EQ(info.definition, std::string("proj=utm zone=31 ellps=GRS80"));
        EXPECT_EQ(info.accuracy, 0);
    }
    {
        auto op = proj_create_from_database(m_ctxt, "EPSG", "1024",
                                            PJ_CATEGORY_COORDINATE_OPERATION,
                                            false, nullptr);
        ASSERT_NE(op, nullptr);
        ObjectKeeper keeper(op);
        EXPECT_EQ(proj_get_type(op), PJ_TYPE_TRANSFORMATION);

        auto info = proj_pj_info(op);
        EXPECT_NE(info.id, nullptr);
        EXPECT_EQ(info.id, std::string("pipeline"));
        ASSERT_NE(info.description, nullptr);
        EXPECT_EQ(info.description, std::string("MGI to ETRS89 (4)"));
        ASSERT_NE(info.definition, nullptr);
        EXPECT_EQ(
            info.definition,
            std::string("proj=pipeline step proj=axisswap "
                        "order=2,1 step proj=unitconvert xy_in=deg xy_out=rad "
                        "step proj=push v_3 "
                        "step proj=cart ellps=bessel step proj=helmert "
                        "x=601.705 y=84.263 z=485.227 rx=-4.7354 ry=-1.3145 "
                        "rz=-5.393 s=-2.3887 convention=coordinate_frame "
                        "step inv proj=cart ellps=GRS80 "
                        "step proj=pop v_3 "
                        "step proj=unitconvert xy_in=rad xy_out=deg "
                        "step proj=axisswap order=2,1"));
        EXPECT_EQ(info.accuracy, 1);
    }
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_crs) {
    auto crs = proj_create_from_wkt(
        m_ctxt,
        createProjectedCRS()
            ->exportToWKT(
                WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get())
            .c_str(),
        nullptr, nullptr, nullptr);
    ASSERT_NE(crs, nullptr);
    ObjectKeeper keeper(crs);
    EXPECT_TRUE(proj_is_crs(crs));

    auto geodCRS = proj_crs_get_geodetic_crs(m_ctxt, crs);
    ASSERT_NE(geodCRS, nullptr);
    ObjectKeeper keeper_geogCRS(geodCRS);
    EXPECT_TRUE(proj_is_crs(geodCRS));
    auto geogCRS_name = proj_get_name(geodCRS);
    ASSERT_TRUE(geogCRS_name != nullptr);
    EXPECT_EQ(geogCRS_name, std::string("WGS 84"));

    auto h_datum = proj_crs_get_horizontal_datum(m_ctxt, crs);
    ASSERT_NE(h_datum, nullptr);
    ObjectKeeper keeper_h_datum(h_datum);

    auto datum = proj_crs_get_datum(m_ctxt, crs);
    ASSERT_NE(datum, nullptr);
    ObjectKeeper keeper_datum(datum);

    EXPECT_TRUE(proj_is_equivalent_to(h_datum, datum, PJ_COMP_STRICT));

    auto datum_name = proj_get_name(datum);
    ASSERT_TRUE(datum_name != nullptr);
    EXPECT_EQ(datum_name, std::string("World Geodetic System 1984"));

    auto ellipsoid = proj_get_ellipsoid(m_ctxt, crs);
    ASSERT_NE(ellipsoid, nullptr);
    ObjectKeeper keeper_ellipsoid(ellipsoid);
    auto ellipsoid_name = proj_get_name(ellipsoid);
    ASSERT_TRUE(ellipsoid_name != nullptr);
    EXPECT_EQ(ellipsoid_name, std::string("WGS 84"));

    auto ellipsoid_from_datum = proj_get_ellipsoid(m_ctxt, datum);
    ASSERT_NE(ellipsoid_from_datum, nullptr);
    ObjectKeeper keeper_ellipsoid_from_datum(ellipsoid_from_datum);

    EXPECT_EQ(proj_get_ellipsoid(m_ctxt, ellipsoid), nullptr);
    EXPECT_FALSE(proj_is_crs(ellipsoid));

    double a;
    double b;
    int b_is_computed;
    double rf;
    EXPECT_TRUE(proj_ellipsoid_get_parameters(m_ctxt, ellipsoid, nullptr,
                                              nullptr, nullptr, nullptr));
    EXPECT_TRUE(proj_ellipsoid_get_parameters(m_ctxt, ellipsoid, &a, &b,
                                              &b_is_computed, &rf));
    EXPECT_FALSE(proj_ellipsoid_get_parameters(m_ctxt, crs, &a, &b,
                                               &b_is_computed, &rf));
    EXPECT_EQ(a, 6378137);
    EXPECT_NEAR(b, 6356752.31424518, 1e-9);
    EXPECT_EQ(b_is_computed, 1);
    EXPECT_EQ(rf, 298.257223563);
    auto id = proj_get_id_code(ellipsoid, 0);
    ASSERT_TRUE(id != nullptr);
    EXPECT_EQ(id, std::string("7030"));
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_get_celestial_body_name) {

    // Geographic CRS
    {
        auto obj = proj_create_from_database(m_ctxt, "EPSG", "4326",
                                             PJ_CATEGORY_CRS, false, nullptr);
        ASSERT_NE(obj, nullptr);
        ObjectKeeper keeper(obj);
        const char *celestial_body_name =
            proj_get_celestial_body_name(m_ctxt, obj);
        ASSERT_NE(celestial_body_name, nullptr);
        EXPECT_EQ(std::string(celestial_body_name), "Earth");
    }

    // Projected CRS
    {
        auto obj = proj_create_from_database(m_ctxt, "EPSG", "32631",
                                             PJ_CATEGORY_CRS, false, nullptr);
        ASSERT_NE(obj, nullptr);
        ObjectKeeper keeper(obj);
        const char *celestial_body_name =
            proj_get_celestial_body_name(m_ctxt, obj);
        ASSERT_NE(celestial_body_name, nullptr);
        EXPECT_EQ(std::string(celestial_body_name), "Earth");
    }

    // Vertical CRS
    {
        auto obj = proj_create_from_database(m_ctxt, "EPSG", "3855",
                                             PJ_CATEGORY_CRS, false, nullptr);
        ASSERT_NE(obj, nullptr);
        ObjectKeeper keeper(obj);
        const char *celestial_body_name =
            proj_get_celestial_body_name(m_ctxt, obj);
        ASSERT_NE(celestial_body_name, nullptr);
        EXPECT_EQ(std::string(celestial_body_name), "Earth");
    }

    // Compound CRS
    {
        auto obj = proj_create_from_database(m_ctxt, "EPSG", "9518",
                                             PJ_CATEGORY_CRS, false, nullptr);
        ASSERT_NE(obj, nullptr);
        ObjectKeeper keeper(obj);
        const char *celestial_body_name =
            proj_get_celestial_body_name(m_ctxt, obj);
        ASSERT_NE(celestial_body_name, nullptr);
        EXPECT_EQ(std::string(celestial_body_name), "Earth");
    }

    // Geodetic datum
    {
        auto obj = proj_create_from_database(m_ctxt, "EPSG", "6267",
                                             PJ_CATEGORY_DATUM, false, nullptr);
        ASSERT_NE(obj, nullptr);
        ObjectKeeper keeper(obj);
        const char *celestial_body_name =
            proj_get_celestial_body_name(m_ctxt, obj);
        ASSERT_NE(celestial_body_name, nullptr);
        EXPECT_EQ(std::string(celestial_body_name), "Earth");
    }

    // Datum ensemble
    {
        auto obj = proj_create_from_database(
            m_ctxt, "EPSG", "6326", PJ_CATEGORY_DATUM_ENSEMBLE, false, nullptr);
        ASSERT_NE(obj, nullptr);
        ObjectKeeper keeper(obj);
        const char *celestial_body_name =
            proj_get_celestial_body_name(m_ctxt, obj);
        ASSERT_NE(celestial_body_name, nullptr);
        EXPECT_EQ(std::string(celestial_body_name), "Earth");
    }

    // Vertical datum
    {
        auto obj = proj_create_from_database(m_ctxt, "EPSG", "1027",
                                             PJ_CATEGORY_DATUM, false, nullptr);
        ASSERT_NE(obj, nullptr);
        ObjectKeeper keeper(obj);
        const char *celestial_body_name =
            proj_get_celestial_body_name(m_ctxt, obj);
        ASSERT_NE(celestial_body_name, nullptr);
        EXPECT_EQ(std::string(celestial_body_name), "Earth");
    }

    // Ellipsoid
    {
        auto obj = proj_create_from_database(
            m_ctxt, "EPSG", "7030", PJ_CATEGORY_ELLIPSOID, false, nullptr);
        ASSERT_NE(obj, nullptr);
        ObjectKeeper keeper(obj);
        const char *celestial_body_name =
            proj_get_celestial_body_name(m_ctxt, obj);
        ASSERT_NE(celestial_body_name, nullptr);
        EXPECT_EQ(std::string(celestial_body_name), "Earth");
    }

    // Ellipsoid non-EARTH
    {
        auto obj = proj_create_from_database(
            m_ctxt, "ESRI", "107903", PJ_CATEGORY_ELLIPSOID, false, nullptr);
        ASSERT_NE(obj, nullptr);
        ObjectKeeper keeper(obj);
        const char *celestial_body_name =
            proj_get_celestial_body_name(m_ctxt, obj);
        ASSERT_NE(celestial_body_name, nullptr);
        EXPECT_EQ(std::string(celestial_body_name), "Moon");
    }

    // Coordinate operation -> error
    {
        auto obj = proj_create_from_database(m_ctxt, "EPSG", "1591",
                                             PJ_CATEGORY_COORDINATE_OPERATION,
                                             false, nullptr);
        ASSERT_NE(obj, nullptr);
        ObjectKeeper keeper(obj);
        const char *celestial_body_name =
            proj_get_celestial_body_name(m_ctxt, obj);
        ASSERT_EQ(celestial_body_name, nullptr);
    }
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_get_prime_meridian) {
    auto crs = proj_create_from_wkt(
        m_ctxt,
        createProjectedCRS()
            ->exportToWKT(
                WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get())
            .c_str(),
        nullptr, nullptr, nullptr);
    ASSERT_NE(crs, nullptr);
    ObjectKeeper keeper(crs);

    auto pm = proj_get_prime_meridian(m_ctxt, crs);
    ASSERT_NE(pm, nullptr);
    ObjectKeeper keeper_pm(pm);
    auto pm_name = proj_get_name(pm);
    ASSERT_TRUE(pm_name != nullptr);
    EXPECT_EQ(pm_name, std::string("Greenwich"));

    EXPECT_EQ(proj_get_prime_meridian(m_ctxt, pm), nullptr);

    EXPECT_TRUE(proj_prime_meridian_get_parameters(m_ctxt, pm, nullptr, nullptr,
                                                   nullptr));
    double longitude = -1;
    double longitude_unit = 0;
    const char *longitude_unit_name = nullptr;
    EXPECT_TRUE(proj_prime_meridian_get_parameters(
        m_ctxt, pm, &longitude, &longitude_unit, &longitude_unit_name));
    EXPECT_EQ(longitude, 0);
    EXPECT_NEAR(longitude_unit, UnitOfMeasure::DEGREE.conversionToSI(), 1e-10);
    ASSERT_TRUE(longitude_unit_name != nullptr);
    EXPECT_EQ(longitude_unit_name, std::string("degree"));

    auto datum = proj_crs_get_horizontal_datum(m_ctxt, crs);
    ASSERT_NE(datum, nullptr);
    ObjectKeeper keeper_datum(datum);
    auto pm_from_datum = proj_get_prime_meridian(m_ctxt, datum);
    ASSERT_NE(pm_from_datum, nullptr);
    ObjectKeeper keeper_pm_from_datum(pm_from_datum);
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_crs_compound) {
    auto crs = proj_create_from_wkt(
        m_ctxt,
        createCompoundCRS()->exportToWKT(WKTFormatter::create().get()).c_str(),
        nullptr, nullptr, nullptr);
    ASSERT_NE(crs, nullptr);
    ObjectKeeper keeper(crs);
    EXPECT_EQ(proj_get_type(crs), PJ_TYPE_COMPOUND_CRS);

    EXPECT_EQ(proj_crs_get_sub_crs(m_ctxt, crs, -1), nullptr);
    EXPECT_EQ(proj_crs_get_sub_crs(m_ctxt, crs, 2), nullptr);

    auto subcrs_horiz = proj_crs_get_sub_crs(m_ctxt, crs, 0);
    ASSERT_NE(subcrs_horiz, nullptr);
    ObjectKeeper keeper_subcrs_horiz(subcrs_horiz);
    EXPECT_EQ(proj_get_type(subcrs_horiz), PJ_TYPE_PROJECTED_CRS);
    EXPECT_EQ(proj_crs_get_sub_crs(m_ctxt, subcrs_horiz, 0), nullptr);

    auto subcrs_vertical = proj_crs_get_sub_crs(m_ctxt, crs, 1);
    ASSERT_NE(subcrs_vertical, nullptr);
    ObjectKeeper keeper_subcrs_vertical(subcrs_vertical);
    EXPECT_EQ(proj_get_type(subcrs_vertical), PJ_TYPE_VERTICAL_CRS);
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_get_source_target_crs_bound_crs) {
    auto crs = proj_create_from_wkt(
        m_ctxt,
        createBoundCRS()->exportToWKT(WKTFormatter::create().get()).c_str(),
        nullptr, nullptr, nullptr);
    ASSERT_NE(crs, nullptr);
    ObjectKeeper keeper(crs);

    auto sourceCRS = proj_get_source_crs(m_ctxt, crs);
    ASSERT_NE(sourceCRS, nullptr);
    ObjectKeeper keeper_sourceCRS(sourceCRS);
    EXPECT_EQ(std::string(proj_get_name(sourceCRS)), "NTF (Paris)");

    auto targetCRS = proj_get_target_crs(m_ctxt, crs);
    ASSERT_NE(targetCRS, nullptr);
    ObjectKeeper keeper_targetCRS(targetCRS);
    EXPECT_EQ(std::string(proj_get_name(targetCRS)), "WGS 84");
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_get_source_target_crs_transformation) {
    auto obj =
        proj_create_from_wkt(m_ctxt,
                             createBoundCRS()
                                 ->transformation()
                                 ->exportToWKT(WKTFormatter::create().get())
                                 .c_str(),
                             nullptr, nullptr, nullptr);
    ASSERT_NE(obj, nullptr);
    ObjectKeeper keeper(obj);

    auto sourceCRS = proj_get_source_crs(m_ctxt, obj);
    ASSERT_NE(sourceCRS, nullptr);
    ObjectKeeper keeper_sourceCRS(sourceCRS);
    EXPECT_EQ(std::string(proj_get_name(sourceCRS)), "NTF (Paris)");

    auto targetCRS = proj_get_target_crs(m_ctxt, obj);
    ASSERT_NE(targetCRS, nullptr);
    ObjectKeeper keeper_targetCRS(targetCRS);
    EXPECT_EQ(std::string(proj_get_name(targetCRS)), "WGS 84");
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_get_source_crs_of_projected_crs) {
    auto crs = proj_create_from_wkt(
        m_ctxt,
        createProjectedCRS()->exportToWKT(WKTFormatter::create().get()).c_str(),
        nullptr, nullptr, nullptr);
    ASSERT_NE(crs, nullptr);
    ObjectKeeper keeper(crs);

    auto sourceCRS = proj_get_source_crs(m_ctxt, crs);
    ASSERT_NE(sourceCRS, nullptr);
    ObjectKeeper keeper_sourceCRS(sourceCRS);
    EXPECT_EQ(std::string(proj_get_name(sourceCRS)), "WGS 84");
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_get_source_target_crs_conversion_without_crs) {
    auto obj = proj_create_from_database(m_ctxt, "EPSG", "16031",
                                         PJ_CATEGORY_COORDINATE_OPERATION,
                                         false, nullptr);
    ASSERT_NE(obj, nullptr);
    ObjectKeeper keeper(obj);

    auto sourceCRS = proj_get_source_crs(m_ctxt, obj);
    ASSERT_EQ(sourceCRS, nullptr);

    auto targetCRS = proj_get_target_crs(m_ctxt, obj);
    ASSERT_EQ(targetCRS, nullptr);
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_get_source_target_crs_invalid_object) {
    auto obj = proj_create_from_wkt(
        m_ctxt, "ELLIPSOID[\"WGS 84\",6378137,298.257223563]", nullptr, nullptr,
        nullptr);
    ASSERT_NE(obj, nullptr);
    ObjectKeeper keeper(obj);

    auto sourceCRS = proj_get_source_crs(m_ctxt, obj);
    ASSERT_EQ(sourceCRS, nullptr);

    auto targetCRS = proj_get_target_crs(m_ctxt, obj);
    ASSERT_EQ(targetCRS, nullptr);
}

// ---------------------------------------------------------------------------

struct ListFreer {
    PROJ_STRING_LIST list;
    ListFreer(PROJ_STRING_LIST ptrIn) : list(ptrIn) {}
    ~ListFreer() { proj_string_list_destroy(list); }
    ListFreer(const ListFreer &) = delete;
    ListFreer &operator=(const ListFreer &) = delete;
};

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_get_authorities_from_database) {
    auto list = proj_get_authorities_from_database(m_ctxt);
    ListFreer feer(list);
    ASSERT_NE(list, nullptr);
    ASSERT_TRUE(list[0] != nullptr);
    EXPECT_EQ(list[0], std::string("EPSG"));
    ASSERT_TRUE(list[1] != nullptr);
    EXPECT_EQ(list[1], std::string("ESRI"));
    ASSERT_TRUE(list[2] != nullptr);
    EXPECT_EQ(list[2], std::string("IAU_2015"));
    ASSERT_TRUE(list[3] != nullptr);
    EXPECT_EQ(list[3], std::string("IGNF"));
    ASSERT_TRUE(list[4] != nullptr);
    EXPECT_EQ(list[4], std::string("NKG"));
    ASSERT_TRUE(list[5] != nullptr);
    EXPECT_EQ(list[5], std::string("NRCAN"));
    ASSERT_TRUE(list[6] != nullptr);
    EXPECT_EQ(list[6], std::string("OGC"));
    ASSERT_TRUE(list[7] != nullptr);
    EXPECT_EQ(list[7], std::string("PROJ"));
    EXPECT_EQ(list[8], nullptr);
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_get_codes_from_database) {

    auto listTypes =
        std::vector<PJ_TYPE>{PJ_TYPE_ELLIPSOID,

                             PJ_TYPE_PRIME_MERIDIAN,

                             PJ_TYPE_GEODETIC_REFERENCE_FRAME,
                             PJ_TYPE_DYNAMIC_GEODETIC_REFERENCE_FRAME,
                             PJ_TYPE_VERTICAL_REFERENCE_FRAME,
                             PJ_TYPE_DYNAMIC_VERTICAL_REFERENCE_FRAME,
                             PJ_TYPE_DATUM_ENSEMBLE,
                             PJ_TYPE_TEMPORAL_DATUM,
                             PJ_TYPE_ENGINEERING_DATUM,
                             PJ_TYPE_PARAMETRIC_DATUM,

                             PJ_TYPE_CRS,
                             PJ_TYPE_GEODETIC_CRS,
                             PJ_TYPE_GEOCENTRIC_CRS,
                             PJ_TYPE_GEOGRAPHIC_CRS,
                             PJ_TYPE_GEOGRAPHIC_2D_CRS,
                             PJ_TYPE_GEOGRAPHIC_3D_CRS,
                             PJ_TYPE_VERTICAL_CRS,
                             PJ_TYPE_PROJECTED_CRS,
                             PJ_TYPE_COMPOUND_CRS,
                             PJ_TYPE_TEMPORAL_CRS,
                             PJ_TYPE_BOUND_CRS,
                             PJ_TYPE_OTHER_CRS,

                             PJ_TYPE_CONVERSION,
                             PJ_TYPE_TRANSFORMATION,
                             PJ_TYPE_CONCATENATED_OPERATION,
                             PJ_TYPE_OTHER_COORDINATE_OPERATION,

                             PJ_TYPE_UNKNOWN};
    for (const auto &type : listTypes) {
        auto list = proj_get_codes_from_database(m_ctxt, "EPSG", type, true);
        ListFreer feer(list);
        if (type == PJ_TYPE_TEMPORAL_CRS || type == PJ_TYPE_BOUND_CRS ||
            type == PJ_TYPE_UNKNOWN || type == PJ_TYPE_TEMPORAL_DATUM ||
            type == PJ_TYPE_ENGINEERING_DATUM ||
            type == PJ_TYPE_PARAMETRIC_DATUM) {
            EXPECT_EQ(list, nullptr) << type;
        } else {
            ASSERT_NE(list, nullptr) << type;
            ASSERT_NE(list[0], nullptr) << type;
            if (type == PJ_TYPE_DYNAMIC_GEODETIC_REFERENCE_FRAME ||
                type == PJ_TYPE_DYNAMIC_VERTICAL_REFERENCE_FRAME) {
                auto obj = proj_create_from_database(
                    m_ctxt, "EPSG", list[0], PJ_CATEGORY_DATUM, false, nullptr);
                ASSERT_NE(obj, nullptr);
                ObjectKeeper keeper(obj);
                EXPECT_EQ(proj_get_type(obj), type) << type << " " << list[0];
            }
        }
    }
}

// ---------------------------------------------------------------------------

TEST_F(CApi, conversion) {
    auto crs = proj_create_from_database(m_ctxt, "EPSG", "32631",
                                         PJ_CATEGORY_CRS, false, nullptr);
    ASSERT_NE(crs, nullptr);
    ObjectKeeper keeper(crs);

    // invalid object type
    EXPECT_FALSE(proj_coordoperation_get_method_info(m_ctxt, crs, nullptr,
                                                     nullptr, nullptr));

    {
        auto conv = proj_crs_get_coordoperation(m_ctxt, crs);
        ASSERT_NE(conv, nullptr);
        ObjectKeeper keeper_conv(conv);

        ASSERT_EQ(proj_crs_get_coordoperation(m_ctxt, conv), nullptr);
    }

    auto conv = proj_crs_get_coordoperation(m_ctxt, crs);
    ASSERT_NE(conv, nullptr);
    ObjectKeeper keeper_conv(conv);

    EXPECT_TRUE(proj_coordoperation_get_method_info(m_ctxt, conv, nullptr,
                                                    nullptr, nullptr));

    const char *methodName = nullptr;
    const char *methodAuthorityName = nullptr;
    const char *methodCode = nullptr;
    EXPECT_TRUE(proj_coordoperation_get_method_info(
        m_ctxt, conv, &methodName, &methodAuthorityName, &methodCode));

    ASSERT_NE(methodName, nullptr);
    ASSERT_NE(methodAuthorityName, nullptr);
    ASSERT_NE(methodCode, nullptr);
    EXPECT_EQ(methodName, std::string("Transverse Mercator"));
    EXPECT_EQ(methodAuthorityName, std::string("EPSG"));
    EXPECT_EQ(methodCode, std::string("9807"));

    EXPECT_EQ(proj_coordoperation_get_param_count(m_ctxt, conv), 5);
    EXPECT_EQ(proj_coordoperation_get_param_index(m_ctxt, conv, "foo"), -1);
    EXPECT_EQ(
        proj_coordoperation_get_param_index(m_ctxt, conv, "False easting"), 3);

    EXPECT_FALSE(proj_coordoperation_get_param(
        m_ctxt, conv, -1, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr));
    EXPECT_FALSE(proj_coordoperation_get_param(
        m_ctxt, conv, 5, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr));

    const char *name = nullptr;
    const char *nameAuthorityName = nullptr;
    const char *nameCode = nullptr;
    double value = 0;
    const char *valueString = nullptr;
    double valueUnitConvFactor = 0;
    const char *valueUnitName = nullptr;
    const char *unitAuthName = nullptr;
    const char *unitCode = nullptr;
    const char *unitCategory = nullptr;
    EXPECT_TRUE(proj_coordoperation_get_param(
        m_ctxt, conv, 3, &name, &nameAuthorityName, &nameCode, &value,
        &valueString, &valueUnitConvFactor, &valueUnitName, &unitAuthName,
        &unitCode, &unitCategory));
    ASSERT_NE(name, nullptr);
    ASSERT_NE(nameAuthorityName, nullptr);
    ASSERT_NE(nameCode, nullptr);
    EXPECT_EQ(valueString, nullptr);
    ASSERT_NE(valueUnitName, nullptr);
    ASSERT_NE(unitAuthName, nullptr);
    ASSERT_NE(unitCategory, nullptr);
    ASSERT_NE(unitCategory, nullptr);
    EXPECT_EQ(name, std::string("False easting"));
    EXPECT_EQ(nameAuthorityName, std::string("EPSG"));
    EXPECT_EQ(nameCode, std::string("8806"));
    EXPECT_EQ(value, 500000.0);
    EXPECT_EQ(valueUnitConvFactor, 1.0);
    EXPECT_EQ(valueUnitName, std::string("metre"));
    EXPECT_EQ(unitAuthName, std::string("EPSG"));
    EXPECT_EQ(unitCode, std::string("9001"));
    EXPECT_EQ(unitCategory, std::string("linear"));
}

// ---------------------------------------------------------------------------

TEST_F(CApi, transformation_from_boundCRS) {
    auto crs = proj_create_from_wkt(
        m_ctxt,
        createBoundCRS()->exportToWKT(WKTFormatter::create().get()).c_str(),
        nullptr, nullptr, nullptr);
    ASSERT_NE(crs, nullptr);
    ObjectKeeper keeper(crs);

    auto transf = proj_crs_get_coordoperation(m_ctxt, crs);
    ASSERT_NE(transf, nullptr);
    ObjectKeeper keeper_transf(transf);
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_coordoperation_get_grid_used) {
    auto op = proj_create_from_database(m_ctxt, "EPSG", "1312",
                                        PJ_CATEGORY_COORDINATE_OPERATION, true,
                                        nullptr);
    ASSERT_NE(op, nullptr);
    ObjectKeeper keeper(op);

    const std::string old_endpoint = proj_context_get_url_endpoint(m_ctxt);
    proj_context_set_url_endpoint(m_ctxt, "https://example.com");

    EXPECT_EQ(proj_coordoperation_get_grid_used_count(m_ctxt, op), 1);
    const char *shortName = nullptr;
    const char *fullName = nullptr;
    const char *packageName = nullptr;
    const char *url = nullptr;
    int directDownload = 0;
    int openLicense = 0;
    int available = 0;
    EXPECT_EQ(proj_coordoperation_get_grid_used(m_ctxt, op, -1, nullptr,
                                                nullptr, nullptr, nullptr,
                                                nullptr, nullptr, nullptr),
              0);
    EXPECT_EQ(proj_coordoperation_get_grid_used(m_ctxt, op, 1, nullptr, nullptr,
                                                nullptr, nullptr, nullptr,
                                                nullptr, nullptr),
              0);
    EXPECT_EQ(proj_coordoperation_get_grid_used(
                  m_ctxt, op, 0, &shortName, &fullName, &packageName, &url,
                  &directDownload, &openLicense, &available),
              1);
    ASSERT_NE(shortName, nullptr);
    ASSERT_NE(fullName, nullptr);
    ASSERT_NE(packageName, nullptr);
    ASSERT_NE(url, nullptr);
    EXPECT_EQ(shortName, std::string("ca_nrc_ntv1_can.tif"));
    // EXPECT_EQ(fullName, std::string(""));
    EXPECT_EQ(packageName, std::string(""));
    EXPECT_EQ(std::string(url), "https://example.com/ca_nrc_ntv1_can.tif");
    EXPECT_EQ(directDownload, 1);
    EXPECT_EQ(openLicense, 1);

    proj_context_set_url_endpoint(m_ctxt, old_endpoint.c_str());
}

// ---------------------------------------------------------------------------

#ifdef TIFF_ENABLED
TEST_F(CApi, proj_coordoperation_get_grid_used_fullname_caching) {
    // Test bugfix for
    // https://github.com/OSGeo/PROJ/issues/3444#issuecomment-1309499342
    for (int i = 0; i < 2; ++i) {
        const char *proj_string =
            "proj=vgridshift grids=tests/test_vgrid_int16.tif";
        PJ *P = proj_create(m_ctxt, proj_string);
        ObjectKeeper keeper(P);

        const char *shortName = nullptr;
        const char *fullName = nullptr;
        const char *packageName = nullptr;
        const char *url = nullptr;
        int directDownload = 0;
        int openLicense = 0;
        int available = 0;

        proj_coordoperation_get_grid_used(m_ctxt, P, 0, &shortName, &fullName,
                                          &packageName, &url, &directDownload,
                                          &openLicense, &available);

        EXPECT_EQ(std::string(shortName), "tests/test_vgrid_int16.tif");
        EXPECT_TRUE(std::string(fullName).find("tests/test_vgrid_int16.tif") !=
                    std::string::npos)
            << std::string(fullName);
    }
}
#endif

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_coordoperation_is_instantiable) {
    auto op = proj_create_from_database(m_ctxt, "EPSG", "1671",
                                        PJ_CATEGORY_COORDINATE_OPERATION, true,
                                        nullptr);
    ASSERT_NE(op, nullptr);
    ObjectKeeper keeper(op);
    EXPECT_EQ(proj_coordoperation_is_instantiable(m_ctxt, op), 1);
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_create_operations) {
    auto ctxt = proj_create_operation_factory_context(m_ctxt, nullptr);
    ASSERT_NE(ctxt, nullptr);
    ContextKeeper keeper_ctxt(ctxt);

    auto source_crs = proj_create_from_database(
        m_ctxt, "EPSG", "4267", PJ_CATEGORY_CRS, false, nullptr); // NAD27
    ASSERT_NE(source_crs, nullptr);
    ObjectKeeper keeper_source_crs(source_crs);

    auto target_crs = proj_create_from_database(
        m_ctxt, "EPSG", "4269", PJ_CATEGORY_CRS, false, nullptr); // NAD83
    ASSERT_NE(target_crs, nullptr);
    ObjectKeeper keeper_target_crs(target_crs);

    proj_operation_factory_context_set_spatial_criterion(
        m_ctxt, ctxt, PROJ_SPATIAL_CRITERION_PARTIAL_INTERSECTION);

    proj_operation_factory_context_set_grid_availability_use(
        m_ctxt, ctxt, PROJ_GRID_AVAILABILITY_IGNORED);

    auto res = proj_create_operations(m_ctxt, source_crs, target_crs, ctxt);
    ASSERT_NE(res, nullptr);
    ObjListKeeper keeper_res(res);

    EXPECT_EQ(proj_list_get_count(res), 10);

    EXPECT_EQ(proj_list_get(m_ctxt, res, -1), nullptr);
    EXPECT_EQ(proj_list_get(m_ctxt, res, proj_list_get_count(res)), nullptr);
    {
        auto op = proj_list_get(m_ctxt, res, 0);
        ASSERT_NE(op, nullptr);
        ObjectKeeper keeper_op(op);
        EXPECT_FALSE(
            proj_coordoperation_has_ballpark_transformation(m_ctxt, op));
        EXPECT_EQ(proj_get_name(op), std::string("NAD27 to NAD83 (4)"));
    }

    {
        PJ_COORD coord;
        coord.xy.x = 40;
        coord.xy.y = -100;
        int idx = proj_get_suggested_operation(m_ctxt, res, PJ_FWD, coord);
        ASSERT_GE(idx, 0);
        ASSERT_LT(idx, proj_list_get_count(res));
        auto op = proj_list_get(m_ctxt, res, idx);
        ASSERT_NE(op, nullptr);
        ObjectKeeper keeper_op(op);
        // Transformation for USA, using NADCON5
        EXPECT_EQ(proj_get_name(op), std::string("NAD27 to NAD83 (7)"));
    }

    {
        PJ_COORD coord;
        coord.xy.x = 40;
        coord.xy.y = 10;
        int idx = proj_get_suggested_operation(m_ctxt, res, PJ_FWD, coord);
        EXPECT_GE(idx, -1);
    }
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_get_suggested_operation_for_NAD83_to_NAD83_HARN) {
    auto ctxt = proj_create_operation_factory_context(m_ctxt, nullptr);
    ASSERT_NE(ctxt, nullptr);
    ContextKeeper keeper_ctxt(ctxt);

    auto source_crs = proj_create_from_database(
        m_ctxt, "EPSG", "4269", PJ_CATEGORY_CRS, false, nullptr); // NAD83
    ASSERT_NE(source_crs, nullptr);
    ObjectKeeper keeper_source_crs(source_crs);

    auto target_crs = proj_create_from_database(
        m_ctxt, "EPSG", "4152", PJ_CATEGORY_CRS, false, nullptr); // NAD83(HARN)
    ASSERT_NE(target_crs, nullptr);
    ObjectKeeper keeper_target_crs(target_crs);

    proj_operation_factory_context_set_spatial_criterion(
        m_ctxt, ctxt, PROJ_SPATIAL_CRITERION_PARTIAL_INTERSECTION);

    proj_operation_factory_context_set_grid_availability_use(
        m_ctxt, ctxt, PROJ_GRID_AVAILABILITY_IGNORED);

    auto res = proj_create_operations(m_ctxt, source_crs, target_crs, ctxt);
    ASSERT_NE(res, nullptr);
    ObjListKeeper keeper_res(res);

    {
        PJ_COORD coord;
        coord.xy.x = 40;
        coord.xy.y = -100;
        int idx = proj_get_suggested_operation(m_ctxt, res, PJ_FWD, coord);
        ASSERT_GE(idx, 0);
        ASSERT_LT(idx, proj_list_get_count(res));
        auto op = proj_list_get(m_ctxt, res, idx);
        ASSERT_NE(op, nullptr);
        ObjectKeeper keeper_op(op);
        // Transformation for CONUS
        EXPECT_STREQ(proj_get_name(op), "NAD83 to NAD83(HARN) (47)");
    }
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_create_operations_prime_meridian_non_greenwich) {
    auto ctxt = proj_create_operation_factory_context(m_ctxt, nullptr);
    ASSERT_NE(ctxt, nullptr);
    ContextKeeper keeper_ctxt(ctxt);

    auto source_crs = proj_create_from_database(
        m_ctxt, "EPSG", "27562", PJ_CATEGORY_CRS, false,
        nullptr); // "NTF (Paris) / Lambert Centre France"
    ASSERT_NE(source_crs, nullptr);
    ObjectKeeper keeper_source_crs(source_crs);

    auto target_crs = proj_create_from_database(
        m_ctxt, "EPSG", "4258", PJ_CATEGORY_CRS, false, nullptr); // ETRS89
    ASSERT_NE(target_crs, nullptr);
    ObjectKeeper keeper_target_crs(target_crs);

    proj_operation_factory_context_set_spatial_criterion(
        m_ctxt, ctxt, PROJ_SPATIAL_CRITERION_PARTIAL_INTERSECTION);

    proj_operation_factory_context_set_grid_availability_use(
        m_ctxt, ctxt, PROJ_GRID_AVAILABILITY_IGNORED);

    auto res = proj_create_operations(m_ctxt, source_crs, target_crs, ctxt);
    ASSERT_NE(res, nullptr);
    ObjListKeeper keeper_res(res);

    {
        PJ_COORD coord;
        // lat,long=49,-4 if using grid
        coord.xy.x = 136555.58288992;
        coord.xy.y = 463344.51894296;
        int idx = proj_get_suggested_operation(m_ctxt, res, PJ_FWD, coord);
        ASSERT_GE(idx, 0);
        auto op = proj_list_get(m_ctxt, res, idx);
        ASSERT_NE(op, nullptr);
        ObjectKeeper keeper_op(op);
        // Transformation using grid
        EXPECT_EQ(proj_coordoperation_get_grid_used_count(m_ctxt, op), 1);
    }
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_get_suggested_operation_with_operations_without_area_of_use) {
    auto ctxt = proj_create_operation_factory_context(m_ctxt, nullptr);
    ASSERT_NE(ctxt, nullptr);
    ContextKeeper keeper_ctxt(ctxt);

    // NAD83(2011) geocentric
    auto source_crs = proj_create_from_database(
        m_ctxt, "EPSG", "6317", PJ_CATEGORY_CRS, false, nullptr);
    ASSERT_NE(source_crs, nullptr);
    ObjectKeeper keeper_source_crs(source_crs);

    // NAD83(2011) 2D
    auto target_crs = proj_create_from_database(
        m_ctxt, "EPSG", "6318", PJ_CATEGORY_CRS, false, nullptr);
    ASSERT_NE(target_crs, nullptr);
    ObjectKeeper keeper_target_crs(target_crs);

    proj_operation_factory_context_set_spatial_criterion(
        m_ctxt, ctxt, PROJ_SPATIAL_CRITERION_PARTIAL_INTERSECTION);

    proj_operation_factory_context_set_grid_availability_use(
        m_ctxt, ctxt, PROJ_GRID_AVAILABILITY_IGNORED);

    auto res = proj_create_operations(m_ctxt, source_crs, target_crs, ctxt);
    ASSERT_NE(res, nullptr);
    ObjListKeeper keeper_res(res);

    PJ_COORD coord;
    coord.xyz.x = -463930;
    coord.xyz.y = -4414006;
    coord.xyz.z = 4562247;
    int idx = proj_get_suggested_operation(m_ctxt, res, PJ_FWD, coord);
    EXPECT_GE(idx, 0);
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_create_operations_discard_superseded) {
    auto ctxt = proj_create_operation_factory_context(m_ctxt, nullptr);
    ASSERT_NE(ctxt, nullptr);
    ContextKeeper keeper_ctxt(ctxt);

    auto source_crs = proj_create_from_database(
        m_ctxt, "EPSG", "4203", PJ_CATEGORY_CRS, false, nullptr); // AGD84
    ASSERT_NE(source_crs, nullptr);
    ObjectKeeper keeper_source_crs(source_crs);

    auto target_crs = proj_create_from_database(
        m_ctxt, "EPSG", "4326", PJ_CATEGORY_CRS, false, nullptr); // WGS84
    ASSERT_NE(target_crs, nullptr);
    ObjectKeeper keeper_target_crs(target_crs);

    proj_operation_factory_context_set_spatial_criterion(
        m_ctxt, ctxt, PROJ_SPATIAL_CRITERION_PARTIAL_INTERSECTION);

    proj_operation_factory_context_set_grid_availability_use(
        m_ctxt, ctxt, PROJ_GRID_AVAILABILITY_IGNORED);

    proj_operation_factory_context_set_discard_superseded(m_ctxt, ctxt, true);

    auto res = proj_create_operations(m_ctxt, source_crs, target_crs, ctxt);
    ASSERT_NE(res, nullptr);
    ObjListKeeper keeper_res(res);

    EXPECT_EQ(proj_list_get_count(res), 4);
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_create_operations_dont_discard_superseded) {
    auto ctxt = proj_create_operation_factory_context(m_ctxt, nullptr);
    ASSERT_NE(ctxt, nullptr);
    ContextKeeper keeper_ctxt(ctxt);

    auto source_crs = proj_create_from_database(
        m_ctxt, "EPSG", "4203", PJ_CATEGORY_CRS, false, nullptr); // AGD84
    ASSERT_NE(source_crs, nullptr);
    ObjectKeeper keeper_source_crs(source_crs);

    auto target_crs = proj_create_from_database(
        m_ctxt, "EPSG", "4326", PJ_CATEGORY_CRS, false, nullptr); // WGS84
    ASSERT_NE(target_crs, nullptr);
    ObjectKeeper keeper_target_crs(target_crs);

    proj_operation_factory_context_set_spatial_criterion(
        m_ctxt, ctxt, PROJ_SPATIAL_CRITERION_PARTIAL_INTERSECTION);

    proj_operation_factory_context_set_grid_availability_use(
        m_ctxt, ctxt, PROJ_GRID_AVAILABILITY_IGNORED);

    proj_operation_factory_context_set_discard_superseded(m_ctxt, ctxt, false);

    auto res = proj_create_operations(m_ctxt, source_crs, target_crs, ctxt);
    ASSERT_NE(res, nullptr);
    ObjListKeeper keeper_res(res);

    EXPECT_EQ(proj_list_get_count(res), 5);
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_create_operations_with_pivot) {

    auto source_crs = proj_create_from_database(
        m_ctxt, "EPSG", "4230", PJ_CATEGORY_CRS, false, nullptr); // ED50
    ASSERT_NE(source_crs, nullptr);
    ObjectKeeper keeper_source_crs(source_crs);

    auto target_crs = proj_create_from_database(
        m_ctxt, "EPSG", "4171", PJ_CATEGORY_CRS, false, nullptr); // RGF93 v1
    ASSERT_NE(target_crs, nullptr);
    ObjectKeeper keeper_target_crs(target_crs);

    // There is no direct transformations between both

    // Default behavior: allow any pivot
    {
        auto ctxt = proj_create_operation_factory_context(m_ctxt, nullptr);
        ASSERT_NE(ctxt, nullptr);
        ContextKeeper keeper_ctxt(ctxt);

        auto res = proj_create_operations(m_ctxt, source_crs, target_crs, ctxt);
        ASSERT_NE(res, nullptr);
        ObjListKeeper keeper_res(res);
        EXPECT_EQ(proj_list_get_count(res), 1);
        auto op = proj_list_get(m_ctxt, res, 0);
        ASSERT_NE(op, nullptr);
        ObjectKeeper keeper_op(op);

        EXPECT_EQ(
            proj_get_name(op),
            std::string(
                "ED50 to ETRS89 (10) + Inverse of RGF93 v1 to ETRS89 (1)"));
    }

    // Disallow pivots
    {
        auto ctxt = proj_create_operation_factory_context(m_ctxt, nullptr);
        ASSERT_NE(ctxt, nullptr);
        ContextKeeper keeper_ctxt(ctxt);
        proj_operation_factory_context_set_allow_use_intermediate_crs(
            m_ctxt, ctxt, PROJ_INTERMEDIATE_CRS_USE_NEVER);

        auto res = proj_create_operations(m_ctxt, source_crs, target_crs, ctxt);
        ASSERT_NE(res, nullptr);
        ObjListKeeper keeper_res(res);
        EXPECT_EQ(proj_list_get_count(res), 1);
        auto op = proj_list_get(m_ctxt, res, 0);
        ASSERT_NE(op, nullptr);
        ObjectKeeper keeper_op(op);

        EXPECT_EQ(
            proj_get_name(op),
            std::string("Ballpark geographic offset from ED50 to RGF93 v1"));
    }

    // Restrict pivot to ETRS89
    {
        auto ctxt = proj_create_operation_factory_context(m_ctxt, "EPSG");
        ASSERT_NE(ctxt, nullptr);
        ContextKeeper keeper_ctxt(ctxt);

        const char *pivots[] = {"EPSG", "4258", nullptr};
        proj_operation_factory_context_set_allowed_intermediate_crs(
            m_ctxt, ctxt, pivots);

        auto res = proj_create_operations(m_ctxt, source_crs, target_crs, ctxt);
        ASSERT_NE(res, nullptr);
        ObjListKeeper keeper_res(res);
        EXPECT_EQ(proj_list_get_count(res), 1);
        auto op = proj_list_get(m_ctxt, res, 0);
        ASSERT_NE(op, nullptr);
        ObjectKeeper keeper_op(op);

        EXPECT_EQ(
            proj_get_name(op),
            std::string(
                "ED50 to ETRS89 (10) + Inverse of RGF93 v1 to ETRS89 (1)"));
    }

    // Restrict pivot to something unrelated
    {
        auto ctxt = proj_create_operation_factory_context(m_ctxt, "any");
        ASSERT_NE(ctxt, nullptr);
        ContextKeeper keeper_ctxt(ctxt);

        const char *pivots[] = {"EPSG", "4267", nullptr}; // NAD27
        proj_operation_factory_context_set_allowed_intermediate_crs(
            m_ctxt, ctxt, pivots);
        proj_operation_factory_context_set_allow_use_intermediate_crs(
            m_ctxt, ctxt, PROJ_INTERMEDIATE_CRS_USE_ALWAYS);

        auto res = proj_create_operations(m_ctxt, source_crs, target_crs, ctxt);
        ASSERT_NE(res, nullptr);
        ObjListKeeper keeper_res(res);
        EXPECT_EQ(proj_list_get_count(res), 1);
        auto op = proj_list_get(m_ctxt, res, 0);
        ASSERT_NE(op, nullptr);
        ObjectKeeper keeper_op(op);

        EXPECT_EQ(
            proj_get_name(op),
            std::string("Ballpark geographic offset from ED50 to RGF93 v1"));
    }
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_create_operations_allow_ballpark_transformations) {
    auto ctxt = proj_create_operation_factory_context(m_ctxt, nullptr);
    ASSERT_NE(ctxt, nullptr);
    ContextKeeper keeper_ctxt(ctxt);

    auto source_crs = proj_create_from_database(
        m_ctxt, "EPSG", "4267", PJ_CATEGORY_CRS, false, nullptr); // NAD27
    ASSERT_NE(source_crs, nullptr);
    ObjectKeeper keeper_source_crs(source_crs);

    auto target_crs = proj_create_from_database(
        m_ctxt, "EPSG", "4258", PJ_CATEGORY_CRS, false, nullptr); // ETRS89
    ASSERT_NE(target_crs, nullptr);
    ObjectKeeper keeper_target_crs(target_crs);

    proj_operation_factory_context_set_spatial_criterion(
        m_ctxt, ctxt, PROJ_SPATIAL_CRITERION_PARTIAL_INTERSECTION);

    proj_operation_factory_context_set_grid_availability_use(
        m_ctxt, ctxt, PROJ_GRID_AVAILABILITY_IGNORED);

    // Default: allowed implicitly
    {
        auto res = proj_create_operations(m_ctxt, source_crs, target_crs, ctxt);
        ASSERT_NE(res, nullptr);
        ObjListKeeper keeper_res(res);

        EXPECT_EQ(proj_list_get_count(res), 1);
    }

    // Allow explicitly
    {
        proj_operation_factory_context_set_allow_ballpark_transformations(
            m_ctxt, ctxt, true);

        auto res = proj_create_operations(m_ctxt, source_crs, target_crs, ctxt);
        ASSERT_NE(res, nullptr);
        ObjListKeeper keeper_res(res);

        EXPECT_EQ(proj_list_get_count(res), 1);
    }

    // Disallow
    {
        proj_operation_factory_context_set_allow_ballpark_transformations(
            m_ctxt, ctxt, false);

        auto res = proj_create_operations(m_ctxt, source_crs, target_crs, ctxt);
        ASSERT_NE(res, nullptr);
        ObjListKeeper keeper_res(res);

        EXPECT_EQ(proj_list_get_count(res), 0);
    }
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_context_set_database_path_null) {

    EXPECT_TRUE(
        proj_context_set_database_path(m_ctxt, nullptr, nullptr, nullptr));
    auto source_crs = proj_create_from_database(m_ctxt, "EPSG", "4326",
                                                PJ_CATEGORY_CRS, false,
                                                nullptr); // WGS84
    ASSERT_NE(source_crs, nullptr);
    ObjectKeeper keeper_source_crs(source_crs);
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_context_set_database_path_aux) {

    const std::string auxDbName(
        "file:proj_test_aux.db?mode=memory&cache=shared");

    sqlite3 *dbAux = nullptr;
    sqlite3_open_v2(
        auxDbName.c_str(), &dbAux,
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_URI, nullptr);
    ASSERT_TRUE(dbAux != nullptr);
    ASSERT_TRUE(sqlite3_exec(dbAux, "BEGIN", nullptr, nullptr, nullptr) ==
                SQLITE_OK);
    {
        auto ctxt = DatabaseContext::create();
        const auto dbStructure = ctxt->getDatabaseStructure();
        for (const auto &sql : dbStructure) {
            ASSERT_TRUE(sqlite3_exec(dbAux, sql.c_str(), nullptr, nullptr,
                                     nullptr) == SQLITE_OK);
        }
    }

    ASSERT_TRUE(sqlite3_exec(
                    dbAux,
                    "INSERT INTO geodetic_crs VALUES('OTHER','OTHER_4326','WGS "
                    "84',NULL,'geographic 2D','EPSG','6422','EPSG','6326',"
                    "NULL,0);",
                    nullptr, nullptr, nullptr) == SQLITE_OK);
    ASSERT_TRUE(sqlite3_exec(dbAux, "COMMIT", nullptr, nullptr, nullptr) ==
                SQLITE_OK);

    const char *const aux_db_list[] = {auxDbName.c_str(), nullptr};

    EXPECT_TRUE(
        proj_context_set_database_path(m_ctxt, nullptr, aux_db_list, nullptr));

    sqlite3_close(dbAux);

    {
        auto crs = proj_create_from_database(m_ctxt, "EPSG", "4326",
                                             PJ_CATEGORY_CRS, false,
                                             nullptr); // WGS84
        ASSERT_NE(crs, nullptr);
        ObjectKeeper keeper_source_crs(crs);
    }

    {
        auto crs = proj_create_from_database(m_ctxt, "OTHER", "OTHER_4326",
                                             PJ_CATEGORY_CRS, false,
                                             nullptr); // WGS84
        ASSERT_NE(crs, nullptr);
        ObjectKeeper keeper_source_crs(crs);
    }
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_context_set_database_path_error_1) {

    EXPECT_FALSE(proj_context_set_database_path(m_ctxt, "i_do_not_exist.db",
                                                nullptr, nullptr));

    // We will eventually re-open on the default DB
    auto source_crs = proj_create_from_database(m_ctxt, "EPSG", "4326",
                                                PJ_CATEGORY_CRS, false,
                                                nullptr); // WGS84
    ASSERT_NE(source_crs, nullptr);
    ObjectKeeper keeper_source_crs(source_crs);
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_context_set_database_path_error_2) {

    const char *aux_db_list[] = {"i_do_not_exist.db", nullptr};
    EXPECT_FALSE(
        proj_context_set_database_path(m_ctxt, nullptr, aux_db_list, nullptr));

    // We will eventually re-open on the default DB
    auto source_crs = proj_create_from_database(m_ctxt, "EPSG", "4326",
                                                PJ_CATEGORY_CRS, false,
                                                nullptr); // WGS84
    ASSERT_NE(source_crs, nullptr);
    ObjectKeeper keeper_source_crs(source_crs);
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_context_guess_wkt_dialect) {

    EXPECT_EQ(proj_context_guess_wkt_dialect(nullptr, "LOCAL_CS[\"foo\"]"),
              PJ_GUESSED_WKT1_GDAL);

    EXPECT_EQ(proj_context_guess_wkt_dialect(
                  nullptr,
                  "GEOGCS[\"GCS_WGS_1984\",DATUM[\"D_WGS_1984\",SPHEROID[\"WGS_"
                  "1984\",6378137.0,298.257223563]],PRIMEM[\"Greenwich\",0.0],"
                  "UNIT[\"Degree\",0.0174532925199433]]"),
              PJ_GUESSED_WKT1_ESRI);

    EXPECT_EQ(proj_context_guess_wkt_dialect(
                  nullptr,
                  " \n\t\rGEOGCRS[\"WGS 84\",\n"
                  "    DATUM[\"World Geodetic System 1984\",\n"
                  "        ELLIPSOID[\"WGS 84\",6378137,298.257223563]],\n"
                  "    CS[ellipsoidal,2],\n"
                  "        AXIS[\"geodetic latitude (Lat)\",north],\n"
                  "        AXIS[\"geodetic longitude (Lon)\",east],\n"
                  "        UNIT[\"degree\",0.0174532925199433]]"),
              PJ_GUESSED_WKT2_2019);

    EXPECT_EQ(proj_context_guess_wkt_dialect(
                  nullptr,
                  "GEODCRS[\"WGS 84\",\n"
                  "    DATUM[\"World Geodetic System 1984\",\n"
                  "        ELLIPSOID[\"WGS 84\",6378137,298.257223563]],\n"
                  "    CS[ellipsoidal,2],\n"
                  "        AXIS[\"geodetic latitude (Lat)\",north],\n"
                  "        AXIS[\"geodetic longitude (Lon)\",east],\n"
                  "        UNIT[\"degree\",0.0174532925199433]]"),
              PJ_GUESSED_WKT2_2015);

    EXPECT_EQ(proj_context_guess_wkt_dialect(nullptr, "foo"),
              PJ_GUESSED_NOT_WKT);
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_create_from_name) {
    /*
        PJ_OBJ_LIST *proj_create_from_name(
            PJ_CONTEXT *ctx,
            const char *auth_name,
            const char *searchedName,
            const PJ_TYPE* types,
            size_t typesCount,
            int approximateMatch,
            size_t limitResultCount,
            const char* const *options); */
    {
        auto res = proj_create_from_name(m_ctxt, nullptr, "WGS 84", nullptr, 0,
                                         false, 0, nullptr);
        ASSERT_NE(res, nullptr);
        ObjListKeeper keeper_res(res);
        EXPECT_EQ(proj_list_get_count(res), 5);
    }
    {
        auto res = proj_create_from_name(m_ctxt, "xx", "WGS 84", nullptr, 0,
                                         false, 0, nullptr);
        ASSERT_NE(res, nullptr);
        ObjListKeeper keeper_res(res);
        EXPECT_EQ(proj_list_get_count(res), 0);
    }
    {
        const PJ_TYPE types[] = {PJ_TYPE_GEODETIC_CRS, PJ_TYPE_PROJECTED_CRS};
        auto res = proj_create_from_name(m_ctxt, nullptr, "WGS 84", types, 2,
                                         true, 10, nullptr);
        ASSERT_NE(res, nullptr);
        ObjListKeeper keeper_res(res);
        EXPECT_EQ(proj_list_get_count(res), 10);
    }
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_identify) {
    auto obj = proj_create_from_wkt(
        m_ctxt,
        GeographicCRS::EPSG_4807->exportToWKT(WKTFormatter::create().get())
            .c_str(),
        nullptr, nullptr, nullptr);
    ObjectKeeper keeper(obj);
    ASSERT_NE(obj, nullptr);
    {
        auto res = proj_identify(m_ctxt, obj, nullptr, nullptr, nullptr);
        ObjListKeeper keeper_res(res);
        EXPECT_EQ(proj_list_get_count(res), 1);
    }
    {
        int *confidence = nullptr;
        auto res = proj_identify(m_ctxt, obj, "EPSG", nullptr, &confidence);
        ObjListKeeper keeper_res(res);
        EXPECT_EQ(proj_list_get_count(res), 1);
        EXPECT_EQ(confidence[0], 100);
        proj_int_list_destroy(confidence);
    }
    {
        auto objEllps = proj_create_from_wkt(
            m_ctxt,
            Ellipsoid::GRS1980->exportToWKT(WKTFormatter::create().get())
                .c_str(),
            nullptr, nullptr, nullptr);
        ObjectKeeper keeperEllps(objEllps);
        ASSERT_NE(objEllps, nullptr);
        auto res = proj_identify(m_ctxt, objEllps, nullptr, nullptr, nullptr);
        ObjListKeeper keeper_res(res);
        EXPECT_EQ(res, nullptr);
    }
    {
        auto obj2 = proj_create(
            m_ctxt, "+proj=longlat +datum=WGS84 +no_defs +type=crs");
        ObjectKeeper keeper2(obj2);
        ASSERT_NE(obj2, nullptr);
        int *confidence = nullptr;
        auto res = proj_identify(m_ctxt, obj2, nullptr, nullptr, &confidence);
        ObjListKeeper keeper_res(res);
        EXPECT_EQ(proj_list_get_count(res), 4);
        proj_int_list_destroy(confidence);
    }
    {
        auto obj2 = proj_create_from_database(m_ctxt, "IGNF", "ETRS89UTM28",
                                              PJ_CATEGORY_CRS, false, nullptr);
        ObjectKeeper keeper2(obj2);
        ASSERT_NE(obj2, nullptr);
        int *confidence = nullptr;
        auto res = proj_identify(m_ctxt, obj2, "EPSG", nullptr, &confidence);
        ObjListKeeper keeper_res(res);
        EXPECT_EQ(proj_list_get_count(res), 1);
        auto gotCRS = proj_list_get(m_ctxt, res, 0);
        ASSERT_NE(gotCRS, nullptr);
        ObjectKeeper keeper_gotCRS(gotCRS);
        auto auth = proj_get_id_auth_name(gotCRS, 0);
        ASSERT_TRUE(auth != nullptr);
        EXPECT_EQ(auth, std::string("EPSG"));
        auto code = proj_get_id_code(gotCRS, 0);
        ASSERT_TRUE(code != nullptr);
        EXPECT_EQ(code, std::string("25828"));
        EXPECT_EQ(confidence[0], 70);
        proj_int_list_destroy(confidence);
    }
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_get_domain_count) {

    auto crs = proj_create_from_database(m_ctxt, "EPSG", "6316",
                                         PJ_CATEGORY_CRS, false, nullptr);
    ASSERT_NE(crs, nullptr);
    ObjectKeeper keeper(crs);
    EXPECT_EQ(proj_get_domain_count(crs), 2);

    const char *name = nullptr;
    EXPECT_TRUE(proj_get_area_of_use_ex(m_ctxt, crs, 0, nullptr, nullptr,
                                        nullptr, nullptr, &name));
    ASSERT_TRUE(name != nullptr);
    EXPECT_EQ(std::string(name),
              "Bosnia and Herzegovina - east of 19°30'E; Kosovo; Montenegro - "
              "east of 19°30'E; Serbia - between 19°30'E and 22°30'E.");

    const char *scope = proj_get_scope_ex(crs, 0);
    ASSERT_TRUE(scope != nullptr);
    EXPECT_STREQ(scope, "Cadastre, engineering survey, topographic mapping "
                        "(large and medium scale).");

    EXPECT_TRUE(proj_get_area_of_use_ex(m_ctxt, crs, 1, nullptr, nullptr,
                                        nullptr, nullptr, &name));
    ASSERT_TRUE(name != nullptr);
    EXPECT_EQ(std::string(name), "North Macedonia.");

    scope = proj_get_scope_ex(crs, 1);
    ASSERT_TRUE(scope != nullptr);
    EXPECT_STREQ(scope, "Cadastre.");

    EXPECT_FALSE(proj_get_area_of_use_ex(m_ctxt, crs, -1, nullptr, nullptr,
                                         nullptr, nullptr, &name));
    EXPECT_FALSE(proj_get_area_of_use_ex(m_ctxt, crs, 2, nullptr, nullptr,
                                         nullptr, nullptr, &name));
    EXPECT_EQ(proj_get_scope_ex(crs, -1), nullptr);
    EXPECT_EQ(proj_get_scope_ex(crs, 2), nullptr);
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_get_area_of_use) {
    {
        auto crs = proj_create_from_database(m_ctxt, "EPSG", "4326",
                                             PJ_CATEGORY_CRS, false, nullptr);
        ASSERT_NE(crs, nullptr);
        ObjectKeeper keeper(crs);
        EXPECT_TRUE(proj_get_area_of_use(m_ctxt, crs, nullptr, nullptr, nullptr,
                                         nullptr, nullptr));
        const char *name = nullptr;
        double w;
        double s;
        double e;
        double n;
        EXPECT_TRUE(proj_get_area_of_use(m_ctxt, crs, &w, &s, &e, &n, &name));
        EXPECT_EQ(w, -180);
        EXPECT_EQ(s, -90);
        EXPECT_EQ(e, 180);
        EXPECT_EQ(n, 90);
        ASSERT_TRUE(name != nullptr);
        EXPECT_EQ(std::string(name), "World.");
    }
    {
        auto obj = proj_create(m_ctxt, "+proj=longlat +type=crs");
        ObjectKeeper keeper(obj);
        ASSERT_NE(obj, nullptr);
        EXPECT_FALSE(proj_get_area_of_use(m_ctxt, obj, nullptr, nullptr,
                                          nullptr, nullptr, nullptr));
    }
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_coordoperation_get_accuracy) {
    {
        auto crs = proj_create_from_database(m_ctxt, "EPSG", "4326",
                                             PJ_CATEGORY_CRS, false, nullptr);
        ASSERT_NE(crs, nullptr);
        ObjectKeeper keeper(crs);
        EXPECT_EQ(proj_coordoperation_get_accuracy(m_ctxt, crs), -1.0);
    }
    {
        auto obj = proj_create_from_database(m_ctxt, "EPSG", "1170",
                                             PJ_CATEGORY_COORDINATE_OPERATION,
                                             false, nullptr);
        ASSERT_NE(obj, nullptr);
        ObjectKeeper keeper(obj);
        EXPECT_EQ(proj_coordoperation_get_accuracy(m_ctxt, obj), 16.0);
    }
    {
        auto obj = proj_create(m_ctxt, "+proj=helmert");
        ObjectKeeper keeper(obj);
        ASSERT_NE(obj, nullptr);
        EXPECT_EQ(proj_coordoperation_get_accuracy(m_ctxt, obj), -1.0);
    }
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_create_geographic_crs) {

    auto cs = proj_create_ellipsoidal_2D_cs(
        m_ctxt, PJ_ELLPS2D_LATITUDE_LONGITUDE, nullptr, 0);
    ObjectKeeper keeper_cs(cs);
    ASSERT_NE(cs, nullptr);

    {
        auto obj = proj_create_geographic_crs(
            m_ctxt, "WGS 84", "World Geodetic System 1984", "WGS 84", 6378137,
            298.257223563, "Greenwich", 0.0, "Degree", 0.0174532925199433, cs);
        ObjectKeeper keeper(obj);
        ASSERT_NE(obj, nullptr);

        auto objRef =
            proj_create(m_ctxt, GeographicCRS::EPSG_4326
                                    ->exportToWKT(WKTFormatter::create().get())
                                    .c_str());
        ObjectKeeper keeperobjRef(objRef);
        EXPECT_NE(objRef, nullptr);

        EXPECT_TRUE(proj_is_equivalent_to(obj, objRef, PJ_COMP_EQUIVALENT));

        auto datum = proj_crs_get_datum(m_ctxt, obj);
        ObjectKeeper keeper_datum(datum);
        ASSERT_NE(datum, nullptr);

        auto obj2 =
            proj_create_geographic_crs_from_datum(m_ctxt, "WGS 84", datum, cs);
        ObjectKeeper keeperObj(obj2);
        ASSERT_NE(obj2, nullptr);

        EXPECT_TRUE(proj_is_equivalent_to(obj, obj2, PJ_COMP_STRICT));
    }
    {
        auto obj =
            proj_create_geographic_crs(m_ctxt, nullptr, nullptr, nullptr, 1.0,
                                       0.0, nullptr, 0.0, nullptr, 0.0, cs);
        ObjectKeeper keeper(obj);
        ASSERT_NE(obj, nullptr);
    }

    // Datum with GDAL_WKT1 spelling: special case of WGS_1984
    {
        auto obj = proj_create_geographic_crs(
            m_ctxt, "WGS 84", "WGS_1984", "WGS 84", 6378137, 298.257223563,
            "Greenwich", 0.0, "Degree", 0.0174532925199433, cs);
        ObjectKeeper keeper(obj);
        ASSERT_NE(obj, nullptr);

        auto objRef =
            proj_create(m_ctxt, GeographicCRS::EPSG_4326
                                    ->exportToWKT(WKTFormatter::create().get())
                                    .c_str());
        ObjectKeeper keeperobjRef(objRef);
        EXPECT_NE(objRef, nullptr);

        EXPECT_TRUE(proj_is_equivalent_to(obj, objRef, PJ_COMP_EQUIVALENT));
    }

    // Datum with GDAL_WKT1 spelling: database query
    {
        auto obj = proj_create_geographic_crs(
            m_ctxt, "NAD83", "North_American_Datum_1983", "GRS 1980", 6378137,
            298.257222101, "Greenwich", 0.0, "Degree", 0.0174532925199433, cs);
        ObjectKeeper keeper(obj);
        ASSERT_NE(obj, nullptr);

        auto objRef =
            proj_create(m_ctxt, GeographicCRS::EPSG_4269
                                    ->exportToWKT(WKTFormatter::create().get())
                                    .c_str());
        ObjectKeeper keeperobjRef(objRef);
        EXPECT_NE(objRef, nullptr);

        EXPECT_TRUE(proj_is_equivalent_to(obj, objRef, PJ_COMP_EQUIVALENT));
    }

    // Datum with GDAL_WKT1 spelling: database query in alias_name table
    {
        auto crs = proj_create_geographic_crs(
            m_ctxt, "S-JTSK (Ferro)",
            "System_Jednotne_Trigonometricke_Site_Katastralni_Ferro",
            "Bessel 1841", 6377397.155, 299.1528128, "Ferro",
            -17.66666666666667, "Degree", 0.0174532925199433, cs);
        ObjectKeeper keeper(crs);
        ASSERT_NE(crs, nullptr);

        auto datum = proj_crs_get_datum(m_ctxt, crs);
        ASSERT_NE(datum, nullptr);
        ObjectKeeper keeper_datum(datum);

        auto datum_name = proj_get_name(datum);
        ASSERT_TRUE(datum_name != nullptr);
        EXPECT_EQ(datum_name,
                  std::string("System of the Unified Trigonometrical Cadastral "
                              "Network (Ferro)"));
    }

    // WKT1 with (deprecated)
    {
        auto crs = proj_create_geographic_crs(
            m_ctxt, "SAD69 (deprecated)", "South_American_Datum_1969",
            "GRS 1967", 6378160, 298.247167427, "Greenwich", 0, "Degree",
            0.0174532925199433, cs);
        ObjectKeeper keeper(crs);
        ASSERT_NE(crs, nullptr);

        auto name = proj_get_name(crs);
        ASSERT_TRUE(name != nullptr);
        EXPECT_EQ(name, std::string("SAD69"));
        EXPECT_TRUE(proj_is_deprecated(crs));
    }
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_create_geocentric_crs) {
    {
        auto obj = proj_create_geocentric_crs(
            m_ctxt, "WGS 84", "World Geodetic System 1984", "WGS 84", 6378137,
            298.257223563, "Greenwich", 0.0, "Degree", 0.0174532925199433,
            "Metre", 1.0);
        ObjectKeeper keeper(obj);
        ASSERT_NE(obj, nullptr);

        auto objRef =
            proj_create(m_ctxt, GeographicCRS::EPSG_4978
                                    ->exportToWKT(WKTFormatter::create().get())
                                    .c_str());
        ObjectKeeper keeperobjRef(objRef);
        EXPECT_NE(objRef, nullptr);

        EXPECT_TRUE(proj_is_equivalent_to(obj, objRef, PJ_COMP_EQUIVALENT));

        auto datum = proj_crs_get_datum(m_ctxt, obj);
        ObjectKeeper keeper_datum(datum);
        ASSERT_NE(datum, nullptr);

        auto obj2 = proj_create_geocentric_crs_from_datum(m_ctxt, "WGS 84",
                                                          datum, "Metre", 1.0);
        ObjectKeeper keeperObj(obj2);
        ASSERT_NE(obj2, nullptr);

        EXPECT_TRUE(proj_is_equivalent_to(obj, obj2, PJ_COMP_STRICT));
    }
    {
        auto obj = proj_create_geocentric_crs(m_ctxt, nullptr, nullptr, nullptr,
                                              1.0, 0.0, nullptr, 0.0, nullptr,
                                              0.0, nullptr, 0.0);
        ObjectKeeper keeper(obj);
        ASSERT_NE(obj, nullptr);
    }
}

// ---------------------------------------------------------------------------

TEST_F(CApi, check_coord_op_obj_can_be_used_with_proj_trans) {

    {
        auto projCRS = proj_create_conversion_utm(m_ctxt, 31, true);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);

        {
            PJ *pj_used = proj_trans_get_last_used_operation(projCRS);
            ASSERT_EQ(pj_used, nullptr);
        }

        PJ_COORD coord;
        coord.xyzt.x = proj_torad(3.0);
        coord.xyzt.y = 0;
        coord.xyzt.z = 0;
        coord.xyzt.t = 0;
        EXPECT_NEAR(proj_trans(projCRS, PJ_FWD, coord).xyzt.x, 500000.0, 1e-9);

        {
            PJ *pj_used = proj_trans_get_last_used_operation(projCRS);
            ASSERT_TRUE(
                proj_is_equivalent_to(pj_used, projCRS, PJ_COMP_STRICT));
            proj_destroy(pj_used);
        }
    }
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_create_projections) {
    {
        constexpr int invalid_zone_number = 0;
        auto projCRS =
            proj_create_conversion_utm(m_ctxt, invalid_zone_number, 0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_EQ(projCRS, nullptr);
    }

    /* BEGIN: Generated by scripts/create_c_api_projections.py*/
    {
        auto projCRS = proj_create_conversion_utm(m_ctxt, 1, 0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_transverse_mercator(
            m_ctxt, 0, 0, 0, 0, 0, "Degree", 0.0174532925199433, "Metre", 1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS =
            proj_create_conversion_gauss_schreiber_transverse_mercator(
                m_ctxt, 0, 0, 0, 0, 0, "Degree", 0.0174532925199433, "Metre",
                1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS =
            proj_create_conversion_transverse_mercator_south_oriented(
                m_ctxt, 0, 0, 0, 0, 0, "Degree", 0.0174532925199433, "Metre",
                1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_two_point_equidistant(
            m_ctxt, 0, 0, 0, 0, 0, 0, "Degree", 0.0174532925199433, "Metre",
            1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_tunisia_mining_grid(
            m_ctxt, 0, 0, 0, 0, "Degree", 0.0174532925199433, "Metre", 1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_albers_equal_area(
            m_ctxt, 0, 0, 0, 0, 0, 0, "Degree", 0.0174532925199433, "Metre",
            1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_lambert_conic_conformal_1sp(
            m_ctxt, 0, 0, 0, 0, 0, "Degree", 0.0174532925199433, "Metre", 1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS =
            proj_create_conversion_lambert_conic_conformal_1sp_variant_b(
                m_ctxt, 0, 0, 0, 0, 0, 0, "Degree", 0.0174532925199433, "Metre",
                1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_lambert_conic_conformal_2sp(
            m_ctxt, 0, 0, 0, 0, 0, 0, "Degree", 0.0174532925199433, "Metre",
            1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS =
            proj_create_conversion_lambert_conic_conformal_2sp_michigan(
                m_ctxt, 0, 0, 0, 0, 0, 0, 0, "Degree", 0.0174532925199433,
                "Metre", 1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS =
            proj_create_conversion_lambert_conic_conformal_2sp_belgium(
                m_ctxt, 0, 0, 0, 0, 0, 0, "Degree", 0.0174532925199433, "Metre",
                1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_azimuthal_equidistant(
            m_ctxt, 0, 0, 0, 0, "Degree", 0.0174532925199433, "Metre", 1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_guam_projection(
            m_ctxt, 0, 0, 0, 0, "Degree", 0.0174532925199433, "Metre", 1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_bonne(
            m_ctxt, 0, 0, 0, 0, "Degree", 0.0174532925199433, "Metre", 1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS =
            proj_create_conversion_lambert_cylindrical_equal_area_spherical(
                m_ctxt, 0, 0, 0, 0, "Degree", 0.0174532925199433, "Metre", 1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_lambert_cylindrical_equal_area(
            m_ctxt, 0, 0, 0, 0, "Degree", 0.0174532925199433, "Metre", 1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_cassini_soldner(
            m_ctxt, 0, 0, 0, 0, "Degree", 0.0174532925199433, "Metre", 1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_equidistant_conic(
            m_ctxt, 0, 0, 0, 0, 0, 0, "Degree", 0.0174532925199433, "Metre",
            1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_eckert_i(
            m_ctxt, 0, 0, 0, "Degree", 0.0174532925199433, "Metre", 1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_eckert_ii(
            m_ctxt, 0, 0, 0, "Degree", 0.0174532925199433, "Metre", 1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_eckert_iii(
            m_ctxt, 0, 0, 0, "Degree", 0.0174532925199433, "Metre", 1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_eckert_iv(
            m_ctxt, 0, 0, 0, "Degree", 0.0174532925199433, "Metre", 1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_eckert_v(
            m_ctxt, 0, 0, 0, "Degree", 0.0174532925199433, "Metre", 1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_eckert_vi(
            m_ctxt, 0, 0, 0, "Degree", 0.0174532925199433, "Metre", 1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_equidistant_cylindrical(
            m_ctxt, 0, 0, 0, 0, "Degree", 0.0174532925199433, "Metre", 1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_equidistant_cylindrical_spherical(
            m_ctxt, 0, 0, 0, 0, "Degree", 0.0174532925199433, "Metre", 1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_gall(
            m_ctxt, 0, 0, 0, "Degree", 0.0174532925199433, "Metre", 1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_goode_homolosine(
            m_ctxt, 0, 0, 0, "Degree", 0.0174532925199433, "Metre", 1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_interrupted_goode_homolosine(
            m_ctxt, 0, 0, 0, "Degree", 0.0174532925199433, "Metre", 1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_geostationary_satellite_sweep_x(
            m_ctxt, 0, 0, 0, 0, "Degree", 0.0174532925199433, "Metre", 1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_geostationary_satellite_sweep_y(
            m_ctxt, 0, 0, 0, 0, "Degree", 0.0174532925199433, "Metre", 1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_gnomonic(
            m_ctxt, 0, 0, 0, 0, "Degree", 0.0174532925199433, "Metre", 1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_hotine_oblique_mercator_variant_a(
            m_ctxt, 0, 0, 0, 0, 0, 0, 0, "Degree", 0.0174532925199433, "Metre",
            1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_hotine_oblique_mercator_variant_b(
            m_ctxt, 0, 0, 0, 0, 0, 0, 0, "Degree", 0.0174532925199433, "Metre",
            1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS =
            proj_create_conversion_hotine_oblique_mercator_two_point_natural_origin(
                m_ctxt, 0, 0, 0, 0, 0, 0, 0, 0, "Degree", 0.0174532925199433,
                "Metre", 1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_laborde_oblique_mercator(
            m_ctxt, 0, 0, 0, 0, 0, 0, "Degree", 0.0174532925199433, "Metre",
            1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_international_map_world_polyconic(
            m_ctxt, 0, 0, 0, 0, 0, "Degree", 0.0174532925199433, "Metre", 1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_krovak_north_oriented(
            m_ctxt, 0, 0, 0, 0, 0, 0, 0, "Degree", 0.0174532925199433, "Metre",
            1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS =
            proj_create_conversion_krovak(m_ctxt, 0, 0, 0, 0, 0, 0, 0, "Degree",
                                          0.0174532925199433, "Metre", 1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_lambert_azimuthal_equal_area(
            m_ctxt, 0, 0, 0, 0, "Degree", 0.0174532925199433, "Metre", 1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_miller_cylindrical(
            m_ctxt, 0, 0, 0, "Degree", 0.0174532925199433, "Metre", 1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_mercator_variant_a(
            m_ctxt, 0, 0, 0, 0, 0, "Degree", 0.0174532925199433, "Metre", 1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_mercator_variant_b(
            m_ctxt, 0, 0, 0, 0, "Degree", 0.0174532925199433, "Metre", 1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS =
            proj_create_conversion_popular_visualisation_pseudo_mercator(
                m_ctxt, 0, 0, 0, 0, "Degree", 0.0174532925199433, "Metre", 1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_mollweide(
            m_ctxt, 0, 0, 0, "Degree", 0.0174532925199433, "Metre", 1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_new_zealand_mapping_grid(
            m_ctxt, 0, 0, 0, 0, "Degree", 0.0174532925199433, "Metre", 1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_oblique_stereographic(
            m_ctxt, 0, 0, 0, 0, 0, "Degree", 0.0174532925199433, "Metre", 1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_orthographic(
            m_ctxt, 0, 0, 0, 0, "Degree", 0.0174532925199433, "Metre", 1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_american_polyconic(
            m_ctxt, 0, 0, 0, 0, "Degree", 0.0174532925199433, "Metre", 1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_polar_stereographic_variant_a(
            m_ctxt, 0, 0, 0, 0, 0, "Degree", 0.0174532925199433, "Metre", 1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_polar_stereographic_variant_b(
            m_ctxt, 0, 0, 0, 0, "Degree", 0.0174532925199433, "Metre", 1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_robinson(
            m_ctxt, 0, 0, 0, "Degree", 0.0174532925199433, "Metre", 1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_sinusoidal(
            m_ctxt, 0, 0, 0, "Degree", 0.0174532925199433, "Metre", 1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_stereographic(
            m_ctxt, 0, 0, 0, 0, 0, "Degree", 0.0174532925199433, "Metre", 1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_van_der_grinten(
            m_ctxt, 0, 0, 0, "Degree", 0.0174532925199433, "Metre", 1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_wagner_i(
            m_ctxt, 0, 0, 0, "Degree", 0.0174532925199433, "Metre", 1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_wagner_ii(
            m_ctxt, 0, 0, 0, "Degree", 0.0174532925199433, "Metre", 1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_wagner_iii(
            m_ctxt, 0, 0, 0, 0, "Degree", 0.0174532925199433, "Metre", 1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_wagner_iv(
            m_ctxt, 0, 0, 0, "Degree", 0.0174532925199433, "Metre", 1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_wagner_v(
            m_ctxt, 0, 0, 0, "Degree", 0.0174532925199433, "Metre", 1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_wagner_vi(
            m_ctxt, 0, 0, 0, "Degree", 0.0174532925199433, "Metre", 1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_wagner_vii(
            m_ctxt, 0, 0, 0, "Degree", 0.0174532925199433, "Metre", 1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_quadrilateralized_spherical_cube(
            m_ctxt, 0, 0, 0, 0, "Degree", 0.0174532925199433, "Metre", 1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_spherical_cross_track_height(
            m_ctxt, 0, 0, 0, 0, "Degree", 0.0174532925199433, "Metre", 1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_equal_earth(
            m_ctxt, 0, 0, 0, "Degree", 0.0174532925199433, "Metre", 1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_vertical_perspective(
            m_ctxt, 0, 0, 0, 0, 0, 0, "Degree", 0.0174532925199433, "Metre",
            1.0);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS = proj_create_conversion_pole_rotation_grib_convention(
            m_ctxt, 0, 0, 0, "Degree", 0.0174532925199433);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    {
        auto projCRS =
            proj_create_conversion_pole_rotation_netcdf_cf_convention(
                m_ctxt, 0, 0, 0, "Degree", 0.0174532925199433);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);
    }
    /* END: Generated by scripts/create_c_api_projections.py*/
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_cs_get_axis_info) {
    {
        auto crs = proj_create_from_database(m_ctxt, "EPSG", "4326",
                                             PJ_CATEGORY_CRS, false, nullptr);
        ASSERT_NE(crs, nullptr);
        ObjectKeeper keeper(crs);

        auto cs = proj_crs_get_coordinate_system(m_ctxt, crs);
        ASSERT_NE(cs, nullptr);
        ObjectKeeper keeperCs(cs);

        EXPECT_EQ(proj_cs_get_type(m_ctxt, cs), PJ_CS_TYPE_ELLIPSOIDAL);

        EXPECT_EQ(proj_cs_get_axis_count(m_ctxt, cs), 2);

        EXPECT_FALSE(proj_cs_get_axis_info(m_ctxt, cs, -1, nullptr, nullptr,
                                           nullptr, nullptr, nullptr, nullptr,
                                           nullptr));

        EXPECT_FALSE(proj_cs_get_axis_info(m_ctxt, cs, 2, nullptr, nullptr,
                                           nullptr, nullptr, nullptr, nullptr,
                                           nullptr));

        EXPECT_TRUE(proj_cs_get_axis_info(m_ctxt, cs, 0, nullptr, nullptr,
                                          nullptr, nullptr, nullptr, nullptr,
                                          nullptr));

        const char *name = nullptr;
        const char *abbrev = nullptr;
        const char *direction = nullptr;
        double unitConvFactor = 0.0;
        const char *unitName = nullptr;
        const char *unitAuthority = nullptr;
        const char *unitCode = nullptr;

        EXPECT_TRUE(proj_cs_get_axis_info(
            m_ctxt, cs, 0, &name, &abbrev, &direction, &unitConvFactor,
            &unitName, &unitAuthority, &unitCode));
        ASSERT_NE(name, nullptr);
        ASSERT_NE(abbrev, nullptr);
        ASSERT_NE(direction, nullptr);
        ASSERT_NE(unitName, nullptr);
        ASSERT_NE(unitAuthority, nullptr);
        ASSERT_NE(unitCode, nullptr);
        EXPECT_EQ(std::string(name), "Geodetic latitude");
        EXPECT_EQ(std::string(abbrev), "Lat");
        EXPECT_EQ(std::string(direction), "north");
        EXPECT_EQ(unitConvFactor, 0.017453292519943295) << unitConvFactor;
        EXPECT_EQ(std::string(unitName), "degree");
        EXPECT_EQ(std::string(unitAuthority), "EPSG");
        EXPECT_EQ(std::string(unitCode), "9122");
    }

    // Non CRS object
    {
        auto obj = proj_create_from_database(m_ctxt, "EPSG", "1170",
                                             PJ_CATEGORY_COORDINATE_OPERATION,
                                             false, nullptr);
        ASSERT_NE(obj, nullptr);
        ObjectKeeper keeper(obj);
        EXPECT_EQ(proj_crs_get_coordinate_system(m_ctxt, obj), nullptr);

        EXPECT_EQ(proj_cs_get_type(m_ctxt, obj), PJ_CS_TYPE_UNKNOWN);

        EXPECT_EQ(proj_cs_get_axis_count(m_ctxt, obj), -1);

        EXPECT_FALSE(proj_cs_get_axis_info(m_ctxt, obj, 0, nullptr, nullptr,
                                           nullptr, nullptr, nullptr, nullptr,
                                           nullptr));
    }
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_context_get_database_metadata) {
    EXPECT_TRUE(proj_context_get_database_metadata(m_ctxt, "IGNF.VERSION") !=
                nullptr);

    EXPECT_TRUE(proj_context_get_database_metadata(m_ctxt, "FOO") == nullptr);
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_context_get_database_structure) {
    auto list = proj_context_get_database_structure(m_ctxt, nullptr);
    ASSERT_NE(list, nullptr);
    ASSERT_NE(list[0], nullptr);
    proj_string_list_destroy(list);
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_clone) {
    auto obj = proj_create(m_ctxt, "+proj=longlat");
    ObjectKeeper keeper(obj);
    ASSERT_NE(obj, nullptr);

    auto clone = proj_clone(m_ctxt, obj);
    ObjectKeeper keeperClone(clone);
    ASSERT_NE(clone, nullptr);

    EXPECT_TRUE(proj_is_equivalent_to(obj, clone, PJ_COMP_STRICT));
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_clone_of_obj_with_alternative_operations) {
    // NAD27 to NAD83
    auto obj =
        proj_create_crs_to_crs(m_ctxt, "EPSG:4267", "EPSG:4269", nullptr);
    ObjectKeeper keeper(obj);
    ASSERT_NE(obj, nullptr);

    PJ_COORD c;
    c.xyzt.x = 40.5;
    c.xyzt.y = -60;
    c.xyzt.z = 0;
    c.xyzt.t = 2021;
    PJ_COORD c_trans_ref = proj_trans(obj, PJ_FWD, c);
    EXPECT_NE(c_trans_ref.xyzt.x, c.xyzt.x);
    EXPECT_NEAR(c_trans_ref.xyzt.x, c.xyzt.x, 1e-3);
    EXPECT_NEAR(c_trans_ref.xyzt.y, c.xyzt.y, 1e-3);

    PJ *pj_used = proj_trans_get_last_used_operation(obj);
    ASSERT_NE(pj_used, nullptr);
    proj_destroy(pj_used);

    auto clone = proj_clone(m_ctxt, obj);
    ObjectKeeper keeperClone(clone);
    ASSERT_NE(clone, nullptr);

    EXPECT_TRUE(proj_is_equivalent_to(obj, clone, PJ_COMP_STRICT));

    keeper.clear();
    obj = nullptr;
    (void)obj;

    PJ_COORD c_trans = proj_trans(clone, PJ_FWD, c);
    EXPECT_EQ(c_trans.xyzt.x, c_trans_ref.xyzt.x);
    EXPECT_EQ(c_trans.xyzt.y, c_trans_ref.xyzt.y);
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_crs_alter_geodetic_crs) {
    auto projCRS = proj_create_from_wkt(
        m_ctxt,
        createProjectedCRS()->exportToWKT(WKTFormatter::create().get()).c_str(),
        nullptr, nullptr, nullptr);
    ObjectKeeper keeper(projCRS);
    ASSERT_NE(projCRS, nullptr);

    auto newGeodCRS = proj_create(m_ctxt, "+proj=longlat +type=crs");
    ObjectKeeper keeper_newGeodCRS(newGeodCRS);
    ASSERT_NE(newGeodCRS, nullptr);

    auto geodCRS = proj_crs_get_geodetic_crs(m_ctxt, projCRS);
    ObjectKeeper keeper_geodCRS(geodCRS);
    ASSERT_NE(geodCRS, nullptr);

    auto geodCRSAltered =
        proj_crs_alter_geodetic_crs(m_ctxt, geodCRS, newGeodCRS);
    ObjectKeeper keeper_geodCRSAltered(geodCRSAltered);
    ASSERT_NE(geodCRSAltered, nullptr);
    EXPECT_TRUE(
        proj_is_equivalent_to(geodCRSAltered, newGeodCRS, PJ_COMP_STRICT));

    {
        auto projCRSAltered =
            proj_crs_alter_geodetic_crs(m_ctxt, projCRS, newGeodCRS);
        ObjectKeeper keeper_projCRSAltered(projCRSAltered);
        ASSERT_NE(projCRSAltered, nullptr);

        EXPECT_EQ(proj_get_type(projCRSAltered), PJ_TYPE_PROJECTED_CRS);

        auto projCRSAltered_geodCRS =
            proj_crs_get_geodetic_crs(m_ctxt, projCRSAltered);
        ObjectKeeper keeper_projCRSAltered_geodCRS(projCRSAltered_geodCRS);
        ASSERT_NE(projCRSAltered_geodCRS, nullptr);

        EXPECT_TRUE(proj_is_equivalent_to(projCRSAltered_geodCRS, newGeodCRS,
                                          PJ_COMP_STRICT));
    }

    // Check that proj_crs_alter_geodetic_crs preserves deprecation flag
    {
        auto projCRSDeprecated =
            proj_alter_name(m_ctxt, projCRS, "new name (deprecated)");
        ObjectKeeper keeper_projCRSDeprecated(projCRSDeprecated);
        ASSERT_NE(projCRSDeprecated, nullptr);

        auto projCRSAltered =
            proj_crs_alter_geodetic_crs(m_ctxt, projCRSDeprecated, newGeodCRS);
        ObjectKeeper keeper_projCRSAltered(projCRSAltered);
        ASSERT_NE(projCRSAltered, nullptr);

        EXPECT_EQ(proj_get_name(projCRSAltered), std::string("new name"));
        EXPECT_TRUE(proj_is_deprecated(projCRSAltered));
    }
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_crs_alter_cs_angular_unit) {
    auto crs = proj_create_from_wkt(
        m_ctxt,
        GeographicCRS::EPSG_4326->exportToWKT(WKTFormatter::create().get())
            .c_str(),
        nullptr, nullptr, nullptr);
    ObjectKeeper keeper(crs);
    ASSERT_NE(crs, nullptr);

    {
        auto alteredCRS = proj_crs_alter_cs_angular_unit(m_ctxt, crs, "my unit",
                                                         2, nullptr, nullptr);
        ObjectKeeper keeper_alteredCRS(alteredCRS);
        ASSERT_NE(alteredCRS, nullptr);

        auto cs = proj_crs_get_coordinate_system(m_ctxt, alteredCRS);
        ASSERT_NE(cs, nullptr);
        ObjectKeeper keeperCs(cs);
        double unitConvFactor = 0.0;
        const char *unitName = nullptr;

        EXPECT_TRUE(proj_cs_get_axis_info(m_ctxt, cs, 0, nullptr, nullptr,
                                          nullptr, &unitConvFactor, &unitName,
                                          nullptr, nullptr));
        ASSERT_NE(unitName, nullptr);
        EXPECT_EQ(unitConvFactor, 2) << unitConvFactor;
        EXPECT_EQ(std::string(unitName), "my unit");
    }

    {
        auto alteredCRS = proj_crs_alter_cs_angular_unit(
            m_ctxt, crs, "my unit", 2, "my auth", "my code");
        ObjectKeeper keeper_alteredCRS(alteredCRS);
        ASSERT_NE(alteredCRS, nullptr);

        auto cs = proj_crs_get_coordinate_system(m_ctxt, alteredCRS);
        ASSERT_NE(cs, nullptr);
        ObjectKeeper keeperCs(cs);
        double unitConvFactor = 0.0;
        const char *unitName = nullptr;
        const char *unitAuthName = nullptr;
        const char *unitCode = nullptr;

        EXPECT_TRUE(proj_cs_get_axis_info(m_ctxt, cs, 0, nullptr, nullptr,
                                          nullptr, &unitConvFactor, &unitName,
                                          &unitAuthName, &unitCode));
        ASSERT_NE(unitName, nullptr);
        EXPECT_EQ(unitConvFactor, 2) << unitConvFactor;
        EXPECT_EQ(std::string(unitName), "my unit");
        ASSERT_NE(unitAuthName, nullptr);
        EXPECT_EQ(std::string(unitAuthName), "my auth");
        ASSERT_NE(unitCode, nullptr);
        EXPECT_EQ(std::string(unitCode), "my code");
    }
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_crs_alter_cs_linear_unit) {
    auto crs = proj_create_from_wkt(
        m_ctxt,
        createProjectedCRS()->exportToWKT(WKTFormatter::create().get()).c_str(),
        nullptr, nullptr, nullptr);
    ObjectKeeper keeper(crs);
    ASSERT_NE(crs, nullptr);

    {
        auto alteredCRS = proj_crs_alter_cs_linear_unit(m_ctxt, crs, "my unit",
                                                        2, nullptr, nullptr);
        ObjectKeeper keeper_alteredCRS(alteredCRS);
        ASSERT_NE(alteredCRS, nullptr);

        auto cs = proj_crs_get_coordinate_system(m_ctxt, alteredCRS);
        ASSERT_NE(cs, nullptr);
        ObjectKeeper keeperCs(cs);
        double unitConvFactor = 0.0;
        const char *unitName = nullptr;

        EXPECT_TRUE(proj_cs_get_axis_info(m_ctxt, cs, 0, nullptr, nullptr,
                                          nullptr, &unitConvFactor, &unitName,
                                          nullptr, nullptr));
        ASSERT_NE(unitName, nullptr);
        EXPECT_EQ(unitConvFactor, 2) << unitConvFactor;
        EXPECT_EQ(std::string(unitName), "my unit");
    }

    {
        auto alteredCRS = proj_crs_alter_cs_linear_unit(
            m_ctxt, crs, "my unit", 2, "my auth", "my code");
        ObjectKeeper keeper_alteredCRS(alteredCRS);
        ASSERT_NE(alteredCRS, nullptr);

        auto cs = proj_crs_get_coordinate_system(m_ctxt, alteredCRS);
        ASSERT_NE(cs, nullptr);
        ObjectKeeper keeperCs(cs);
        double unitConvFactor = 0.0;
        const char *unitName = nullptr;
        const char *unitAuthName = nullptr;
        const char *unitCode = nullptr;

        EXPECT_TRUE(proj_cs_get_axis_info(m_ctxt, cs, 0, nullptr, nullptr,
                                          nullptr, &unitConvFactor, &unitName,
                                          &unitAuthName, &unitCode));
        ASSERT_NE(unitName, nullptr);
        EXPECT_EQ(unitConvFactor, 2) << unitConvFactor;
        EXPECT_EQ(std::string(unitName), "my unit");
        ASSERT_NE(unitAuthName, nullptr);
        EXPECT_EQ(std::string(unitAuthName), "my auth");
        ASSERT_NE(unitCode, nullptr);
        EXPECT_EQ(std::string(unitCode), "my code");
    }
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_crs_alter_parameters_linear_unit) {
    auto crs = proj_create_from_wkt(
        m_ctxt,
        createProjectedCRS()->exportToWKT(WKTFormatter::create().get()).c_str(),
        nullptr, nullptr, nullptr);
    ObjectKeeper keeper(crs);
    ASSERT_NE(crs, nullptr);

    {
        auto alteredCRS = proj_crs_alter_parameters_linear_unit(
            m_ctxt, crs, "my unit", 2, nullptr, nullptr, false);
        ObjectKeeper keeper_alteredCRS(alteredCRS);
        ASSERT_NE(alteredCRS, nullptr);

        auto wkt = proj_as_wkt(m_ctxt, alteredCRS, PJ_WKT2_2019, nullptr);
        ASSERT_NE(wkt, nullptr);
        EXPECT_TRUE(std::string(wkt).find("500000") != std::string::npos)
            << wkt;
        EXPECT_TRUE(std::string(wkt).find("\"my unit\",2") != std::string::npos)
            << wkt;
    }

    {
        auto alteredCRS = proj_crs_alter_parameters_linear_unit(
            m_ctxt, crs, "my unit", 2, nullptr, nullptr, true);
        ObjectKeeper keeper_alteredCRS(alteredCRS);
        ASSERT_NE(alteredCRS, nullptr);

        auto wkt = proj_as_wkt(m_ctxt, alteredCRS, PJ_WKT2_2019, nullptr);
        ASSERT_NE(wkt, nullptr);
        EXPECT_TRUE(std::string(wkt).find("250000") != std::string::npos)
            << wkt;
        EXPECT_TRUE(std::string(wkt).find("\"my unit\",2") != std::string::npos)
            << wkt;
    }
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_create_engineering_crs) {

    auto crs = proj_create_engineering_crs(m_ctxt, "name");
    ObjectKeeper keeper(crs);
    ASSERT_NE(crs, nullptr);
    auto wkt = proj_as_wkt(m_ctxt, crs, PJ_WKT1_GDAL, nullptr);
    ASSERT_NE(wkt, nullptr);
    EXPECT_EQ(std::string(wkt), "LOCAL_CS[\"name\",\n"
                                "    UNIT[\"metre\",1,\n"
                                "        AUTHORITY[\"EPSG\",\"9001\"]],\n"
                                "    AXIS[\"Easting\",EAST],\n"
                                "    AXIS[\"Northing\",NORTH]]")
        << wkt;
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_alter_name) {

    auto cs = proj_create_ellipsoidal_2D_cs(
        m_ctxt, PJ_ELLPS2D_LONGITUDE_LATITUDE, nullptr, 0);
    ObjectKeeper keeper_cs(cs);
    ASSERT_NE(cs, nullptr);

    auto obj = proj_create_geographic_crs(
        m_ctxt, "WGS 84", "World Geodetic System 1984", "WGS 84", 6378137,
        298.257223563, "Greenwich", 0.0, "Degree", 0.0174532925199433, cs);
    ObjectKeeper keeper(obj);
    ASSERT_NE(obj, nullptr);

    {
        auto alteredObj = proj_alter_name(m_ctxt, obj, "new name");
        ObjectKeeper keeper_alteredObj(alteredObj);
        ASSERT_NE(alteredObj, nullptr);

        EXPECT_EQ(std::string(proj_get_name(alteredObj)), "new name");
        EXPECT_FALSE(proj_is_deprecated(alteredObj));
    }

    {
        auto alteredObj = proj_alter_name(m_ctxt, obj, "new name (deprecated)");
        ObjectKeeper keeper_alteredObj(alteredObj);
        ASSERT_NE(alteredObj, nullptr);

        EXPECT_EQ(std::string(proj_get_name(alteredObj)), "new name");
        EXPECT_TRUE(proj_is_deprecated(alteredObj));
    }
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_alter_id) {

    auto cs = proj_create_ellipsoidal_2D_cs(
        m_ctxt, PJ_ELLPS2D_LONGITUDE_LATITUDE, nullptr, 0);
    ObjectKeeper keeper_cs(cs);
    ASSERT_NE(cs, nullptr);

    auto obj = proj_create_geographic_crs(
        m_ctxt, "WGS 84", "World Geodetic System 1984", "WGS 84", 6378137,
        298.257223563, "Greenwich", 0.0, "Degree", 0.0174532925199433, cs);
    ObjectKeeper keeper(obj);
    ASSERT_NE(obj, nullptr);

    auto alteredObj = proj_alter_id(m_ctxt, obj, "auth", "code");
    ObjectKeeper keeper_alteredObj(alteredObj);
    ASSERT_NE(alteredObj, nullptr);

    EXPECT_EQ(std::string(proj_get_id_auth_name(alteredObj, 0)), "auth");
    EXPECT_EQ(std::string(proj_get_id_code(alteredObj, 0)), "code");

    auto alteredObj2 = proj_alter_id(m_ctxt, alteredObj, "auth2", "code2");
    ObjectKeeper keeper_alteredObj2(alteredObj2);
    ASSERT_NE(alteredObj2, nullptr);

    EXPECT_EQ(std::string(proj_get_id_auth_name(alteredObj2, 0)), "auth2");
    EXPECT_EQ(std::string(proj_get_id_code(alteredObj2, 0)), "code2");
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_create_projected_crs) {

    PJ_PARAM_DESCRIPTION param;
    param.name = "param name";
    param.auth_name = nullptr;
    param.code = nullptr;
    param.value = 0.99;
    param.unit_name = nullptr;
    param.unit_conv_factor = 1.0;
    param.unit_type = PJ_UT_SCALE;

    auto conv = proj_create_conversion(m_ctxt, "conv", "conv auth", "conv code",
                                       "method", "method auth", "method code",
                                       1, &param);
    ObjectKeeper keeper_conv(conv);
    ASSERT_NE(conv, nullptr);

    auto geog_cs = proj_create_ellipsoidal_2D_cs(
        m_ctxt, PJ_ELLPS2D_LONGITUDE_LATITUDE, nullptr, 0);
    ObjectKeeper keeper_geog_cs(geog_cs);
    ASSERT_NE(geog_cs, nullptr);

    auto geogCRS = proj_create_geographic_crs(
        m_ctxt, "WGS 84", "World Geodetic System 1984", "WGS 84", 6378137,
        298.257223563, "Greenwich", 0.0, "Degree", 0.0174532925199433, geog_cs);
    ObjectKeeper keeper_geogCRS(geogCRS);
    ASSERT_NE(geogCRS, nullptr);

    auto cs = proj_create_cartesian_2D_cs(m_ctxt, PJ_CART2D_EASTING_NORTHING,
                                          nullptr, 0);
    ObjectKeeper keeper_cs(cs);
    ASSERT_NE(cs, nullptr);

    auto projCRS =
        proj_create_projected_crs(m_ctxt, "my CRS", geogCRS, conv, cs);
    ObjectKeeper keeper_projCRS(projCRS);
    ASSERT_NE(projCRS, nullptr);
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_create_transformation) {

    PJ_PARAM_DESCRIPTION param;
    param.name = "param name";
    param.auth_name = nullptr;
    param.code = nullptr;
    param.value = 0.99;
    param.unit_name = nullptr;
    param.unit_conv_factor = 1.0;
    param.unit_type = PJ_UT_SCALE;

    auto geog_cs = proj_create_ellipsoidal_2D_cs(
        m_ctxt, PJ_ELLPS2D_LONGITUDE_LATITUDE, nullptr, 0);
    ObjectKeeper keeper_geog_cs(geog_cs);
    ASSERT_NE(geog_cs, nullptr);

    auto source_crs = proj_create_geographic_crs(
        m_ctxt, "Source CRS", "World Geodetic System 1984", "WGS 84", 6378137,
        298.257223563, "Greenwich", 0.0, "Degree", 0.0174532925199433, geog_cs);
    ObjectKeeper keeper_source_crs(source_crs);
    ASSERT_NE(source_crs, nullptr);

    auto target_crs = proj_create_geographic_crs(
        m_ctxt, "WGS 84", "World Geodetic System 1984", "WGS 84", 6378137,
        298.257223563, "Greenwich", 0.0, "Degree", 0.0174532925199433, geog_cs);
    ObjectKeeper keeper_target_crs(target_crs);
    ASSERT_NE(target_crs, nullptr);

    auto interp_crs = proj_create_geographic_crs(
        m_ctxt, "Interpolation CRS", "World Geodetic System 1984", "WGS 84",
        6378137, 298.257223563, "Greenwich", 0.0, "Degree", 0.0174532925199433,
        geog_cs);
    ObjectKeeper keeper_interp_crs(interp_crs);
    ASSERT_NE(interp_crs, nullptr);

    {
        auto transf = proj_create_transformation(
            m_ctxt, "transf", "transf auth", "transf code", source_crs,
            target_crs, interp_crs, "method", "method auth", "method code", 1,
            &param, 0);
        ObjectKeeper keeper_transf(transf);
        ASSERT_NE(transf, nullptr);

        EXPECT_EQ(proj_coordoperation_get_param_count(m_ctxt, transf), 1);

        auto got_source_crs = proj_get_source_crs(m_ctxt, transf);
        ObjectKeeper keeper_got_source_crs(got_source_crs);
        ASSERT_NE(got_source_crs, nullptr);
        EXPECT_TRUE(
            proj_is_equivalent_to(source_crs, got_source_crs, PJ_COMP_STRICT));

        auto got_target_crs = proj_get_target_crs(m_ctxt, transf);
        ObjectKeeper keeper_got_target_crs(got_target_crs);
        ASSERT_NE(got_target_crs, nullptr);
        EXPECT_TRUE(
            proj_is_equivalent_to(target_crs, got_target_crs, PJ_COMP_STRICT));
    }

    {
        auto transf = proj_create_transformation(
            m_ctxt, "transf", "transf auth", "transf code", source_crs,
            target_crs, nullptr, "method", "method auth", "method code", 1,
            &param, -1);
        ObjectKeeper keeper_transf(transf);
        ASSERT_NE(transf, nullptr);
    }
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_create_compound_crs) {

    auto horiz_cs = proj_create_ellipsoidal_2D_cs(
        m_ctxt, PJ_ELLPS2D_LONGITUDE_LATITUDE, nullptr, 0);
    ObjectKeeper keeper_horiz_cs(horiz_cs);
    ASSERT_NE(horiz_cs, nullptr);

    auto horiz_crs = proj_create_geographic_crs(
        m_ctxt, "WGS 84", "World Geodetic System 1984", "WGS 84", 6378137,
        298.257223563, "Greenwich", 0.0, "Degree", 0.0174532925199433,
        horiz_cs);
    ObjectKeeper keeper_horiz_crs(horiz_crs);
    ASSERT_NE(horiz_crs, nullptr);

    auto vert_crs = proj_create_vertical_crs(m_ctxt, "myVertCRS", "myVertDatum",
                                             nullptr, 0.0);
    ObjectKeeper keeper_vert_crs(vert_crs);
    ASSERT_NE(vert_crs, nullptr);

    EXPECT_EQ(proj_get_name(vert_crs), std::string("myVertCRS"));

    auto compound_crs =
        proj_create_compound_crs(m_ctxt, "myCompoundCRS", horiz_crs, vert_crs);
    ObjectKeeper keeper_compound_crss(compound_crs);
    ASSERT_NE(compound_crs, nullptr);

    EXPECT_EQ(proj_get_name(compound_crs), std::string("myCompoundCRS"));

    auto subcrs_horiz = proj_crs_get_sub_crs(m_ctxt, compound_crs, 0);
    ASSERT_NE(subcrs_horiz, nullptr);
    ObjectKeeper keeper_subcrs_horiz(subcrs_horiz);
    EXPECT_TRUE(proj_is_equivalent_to(subcrs_horiz, horiz_crs, PJ_COMP_STRICT));

    auto subcrs_vert = proj_crs_get_sub_crs(m_ctxt, compound_crs, 1);
    ASSERT_NE(subcrs_vert, nullptr);
    ObjectKeeper keeper_subcrs_vert(subcrs_vert);
    EXPECT_TRUE(proj_is_equivalent_to(subcrs_vert, vert_crs, PJ_COMP_STRICT));
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_convert_conversion_to_other_method) {
    {
        auto geog_cs = proj_create_ellipsoidal_2D_cs(
            m_ctxt, PJ_ELLPS2D_LONGITUDE_LATITUDE, nullptr, 0);
        ObjectKeeper keeper_geog_cs(geog_cs);
        ASSERT_NE(geog_cs, nullptr);

        auto geogCRS = proj_create_geographic_crs(
            m_ctxt, "WGS 84", "World Geodetic System 1984", "WGS 84", 6378137,
            298.257223563, "Greenwich", 0.0, "Degree", 0.0174532925199433,
            geog_cs);
        ObjectKeeper keeper_geogCRS(geogCRS);
        ASSERT_NE(geogCRS, nullptr);

        auto cs = proj_create_cartesian_2D_cs(
            m_ctxt, PJ_CART2D_EASTING_NORTHING, nullptr, 0);
        ObjectKeeper keeper_cs(cs);
        ASSERT_NE(cs, nullptr);

        auto conv = proj_create_conversion_mercator_variant_a(
            m_ctxt, 0, 1, 0.99, 2, 3, "Degree", 0.0174532925199433, "Metre",
            1.0);
        ObjectKeeper keeper_conv(conv);
        ASSERT_NE(conv, nullptr);

        auto projCRS =
            proj_create_projected_crs(m_ctxt, "my CRS", geogCRS, conv, cs);
        ObjectKeeper keeper_projCRS(projCRS);
        ASSERT_NE(projCRS, nullptr);

        // Wrong object type
        EXPECT_EQ(
            proj_convert_conversion_to_other_method(
                m_ctxt, projCRS, EPSG_CODE_METHOD_MERCATOR_VARIANT_B, nullptr),
            nullptr);

        auto conv_in_proj = proj_crs_get_coordoperation(m_ctxt, projCRS);
        ObjectKeeper keeper_conv_in_proj(conv_in_proj);
        ASSERT_NE(conv_in_proj, nullptr);

        // 3rd and 4th argument both 0/null
        EXPECT_EQ(proj_convert_conversion_to_other_method(m_ctxt, conv_in_proj,
                                                          0, nullptr),
                  nullptr);

        auto new_conv = proj_convert_conversion_to_other_method(
            m_ctxt, conv_in_proj, EPSG_CODE_METHOD_MERCATOR_VARIANT_B, nullptr);
        ObjectKeeper keeper_new_conv(new_conv);
        ASSERT_NE(new_conv, nullptr);

        EXPECT_FALSE(
            proj_is_equivalent_to(new_conv, conv_in_proj, PJ_COMP_STRICT));
        EXPECT_TRUE(
            proj_is_equivalent_to(new_conv, conv_in_proj, PJ_COMP_EQUIVALENT));

        auto new_conv_from_name = proj_convert_conversion_to_other_method(
            m_ctxt, conv_in_proj, 0, EPSG_NAME_METHOD_MERCATOR_VARIANT_B);
        ObjectKeeper keeper_new_conv_from_name(new_conv_from_name);
        ASSERT_NE(new_conv_from_name, nullptr);

        EXPECT_TRUE(proj_is_equivalent_to(new_conv, new_conv_from_name,
                                          PJ_COMP_STRICT));

        auto new_conv_back = proj_convert_conversion_to_other_method(
            m_ctxt, conv_in_proj, 0, EPSG_NAME_METHOD_MERCATOR_VARIANT_A);
        ObjectKeeper keeper_new_conv_back(new_conv_back);
        ASSERT_NE(new_conv_back, nullptr);

        EXPECT_TRUE(
            proj_is_equivalent_to(conv_in_proj, new_conv_back, PJ_COMP_STRICT));
    }
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_get_non_deprecated) {
    auto crs = proj_create_from_database(m_ctxt, "EPSG", "4226",
                                         PJ_CATEGORY_CRS, false, nullptr);
    ObjectKeeper keeper(crs);
    ASSERT_NE(crs, nullptr);

    auto list = proj_get_non_deprecated(m_ctxt, crs);
    ASSERT_NE(list, nullptr);
    ObjListKeeper keeper_list(list);
    EXPECT_EQ(proj_list_get_count(list), 2);
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_query_geodetic_crs_from_datum) {
    {
        auto list = proj_query_geodetic_crs_from_datum(m_ctxt, nullptr, "EPSG",
                                                       "6326", nullptr);
        ASSERT_NE(list, nullptr);
        ObjListKeeper keeper_list(list);
        EXPECT_GE(proj_list_get_count(list), 3);
    }
    {
        auto list = proj_query_geodetic_crs_from_datum(m_ctxt, "EPSG", "EPSG",
                                                       "6326", "geographic 2D");
        ASSERT_NE(list, nullptr);
        ObjListKeeper keeper_list(list);
        EXPECT_EQ(proj_list_get_count(list), 1);
    }
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_uom_get_info_from_database) {
    {
        EXPECT_FALSE(proj_uom_get_info_from_database(
            m_ctxt, "auth", "code", nullptr, nullptr, nullptr));
    }
    {
        EXPECT_TRUE(proj_uom_get_info_from_database(m_ctxt, "EPSG", "9001",
                                                    nullptr, nullptr, nullptr));
    }
    {
        const char *name = nullptr;
        double conv_factor = 0.0;
        const char *category = nullptr;
        EXPECT_TRUE(proj_uom_get_info_from_database(
            m_ctxt, "EPSG", "9001", &name, &conv_factor, &category));
        ASSERT_NE(name, nullptr);
        ASSERT_NE(category, nullptr);
        EXPECT_EQ(std::string(name), "metre");
        EXPECT_EQ(conv_factor, 1.0);
        EXPECT_EQ(std::string(category), "linear");
    }
    {
        const char *name = nullptr;
        double conv_factor = 0.0;
        const char *category = nullptr;
        EXPECT_TRUE(proj_uom_get_info_from_database(
            m_ctxt, "EPSG", "9102", &name, &conv_factor, &category));
        ASSERT_NE(name, nullptr);
        ASSERT_NE(category, nullptr);
        EXPECT_EQ(std::string(name), "degree");
        EXPECT_NEAR(conv_factor, UnitOfMeasure::DEGREE.conversionToSI(), 1e-10);
        EXPECT_EQ(std::string(category), "angular");
    }
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_grid_get_info_from_database) {
    {
        EXPECT_FALSE(proj_grid_get_info_from_database(m_ctxt, "xxx", nullptr,
                                                      nullptr, nullptr, nullptr,
                                                      nullptr, nullptr));
    }
    {
        EXPECT_TRUE(proj_grid_get_info_from_database(
            m_ctxt, "au_icsm_GDA94_GDA2020_conformal.tif", nullptr, nullptr,
            nullptr, nullptr, nullptr, nullptr));
    }
    {
        const char *full_name = nullptr;
        const char *package_name = nullptr;
        const char *url = nullptr;
        int direct_download = 0;
        int open_license = 0;
        int available = 0;
        EXPECT_TRUE(proj_grid_get_info_from_database(
            m_ctxt, "au_icsm_GDA94_GDA2020_conformal.tif", &full_name,
            &package_name, &url, &direct_download, &open_license, &available));
        ASSERT_NE(full_name, nullptr);
        // empty string expected as the file is not in test data
        EXPECT_TRUE(full_name[0] == 0);
        ASSERT_NE(package_name, nullptr);
        EXPECT_TRUE(package_name[0] == 0); // empty string expected
        ASSERT_NE(url, nullptr);
        EXPECT_EQ(std::string(url),
                  "https://cdn.proj.org/au_icsm_GDA94_GDA2020_conformal.tif");
        EXPECT_EQ(direct_download, 1);
        EXPECT_EQ(open_license, 1);
    }
    // Same test as above, but with PROJ 6 grid name
    {
        const char *full_name = nullptr;
        const char *package_name = nullptr;
        const char *url = nullptr;
        int direct_download = 0;
        int open_license = 0;
        int available = 0;
        EXPECT_TRUE(proj_grid_get_info_from_database(
            m_ctxt, "GDA94_GDA2020_conformal.gsb", &full_name, &package_name,
            &url, &direct_download, &open_license, &available));
        ASSERT_NE(full_name, nullptr);
        // empty string expected as the file is not in test data
        EXPECT_TRUE(full_name[0] == 0);
        ASSERT_NE(package_name, nullptr);
        EXPECT_TRUE(package_name[0] == 0); // empty string expected
        ASSERT_NE(url, nullptr);
        EXPECT_EQ(std::string(url),
                  "https://cdn.proj.org/au_icsm_GDA94_GDA2020_conformal.tif");
        EXPECT_EQ(direct_download, 1);
        EXPECT_EQ(open_license, 1);
    }
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_create_cartesian_2D_cs) {
    {
        auto cs = proj_create_cartesian_2D_cs(
            m_ctxt, PJ_CART2D_EASTING_NORTHING, nullptr, 0);
        ObjectKeeper keeper_cs(cs);
        ASSERT_NE(cs, nullptr);
    }
    {
        auto cs = proj_create_cartesian_2D_cs(
            m_ctxt, PJ_CART2D_NORTHING_EASTING, nullptr, 0);
        ObjectKeeper keeper_cs(cs);
        ASSERT_NE(cs, nullptr);
    }
    {
        auto cs = proj_create_cartesian_2D_cs(
            m_ctxt, PJ_CART2D_NORTH_POLE_EASTING_SOUTH_NORTHING_SOUTH, nullptr,
            0);
        ObjectKeeper keeper_cs(cs);
        ASSERT_NE(cs, nullptr);
    }
    {
        auto cs = proj_create_cartesian_2D_cs(
            m_ctxt, PJ_CART2D_SOUTH_POLE_EASTING_NORTH_NORTHING_NORTH, nullptr,
            0);
        ObjectKeeper keeper_cs(cs);
        ASSERT_NE(cs, nullptr);
    }
    {
        auto cs = proj_create_cartesian_2D_cs(
            m_ctxt, PJ_CART2D_WESTING_SOUTHING, nullptr, 0);
        ObjectKeeper keeper_cs(cs);
        ASSERT_NE(cs, nullptr);
    }
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_get_crs_info_list_from_database) {
    { proj_crs_info_list_destroy(nullptr); }

    { proj_get_crs_list_parameters_destroy(nullptr); }

    // All null parameters
    {
        auto list = proj_get_crs_info_list_from_database(nullptr, nullptr,
                                                         nullptr, nullptr);
        ASSERT_NE(list, nullptr);
        ASSERT_NE(list[0], nullptr);
        EXPECT_NE(list[0]->auth_name, nullptr);
        EXPECT_NE(list[0]->code, nullptr);
        EXPECT_NE(list[0]->name, nullptr);
        proj_crs_info_list_destroy(list);
    }

    // Default parameters
    {
        int result_count = 0;
        auto params = proj_get_crs_list_parameters_create();
        auto list = proj_get_crs_info_list_from_database(m_ctxt, "EPSG", params,
                                                         &result_count);
        proj_get_crs_list_parameters_destroy(params);
        ASSERT_NE(list, nullptr);
        EXPECT_GT(result_count, 1);
        EXPECT_EQ(list[result_count], nullptr);
        bool found4326 = false;
        bool found4978 = false;
        bool found4979 = false;
        bool found32631 = false;
        bool found3855 = false;
        bool found3901 = false;
        for (int i = 0; i < result_count; i++) {
            // EPSG should only include Earth CRS, at least for now...
            EXPECT_EQ(std::string(list[i]->celestial_body_name), "Earth");
            auto code = std::string(list[i]->code);
            if (code == "4326") {
                found4326 = true;
                EXPECT_EQ(std::string(list[i]->auth_name), "EPSG");
                EXPECT_EQ(std::string(list[i]->name), "WGS 84");
                EXPECT_EQ(list[i]->type, PJ_TYPE_GEOGRAPHIC_2D_CRS);
                EXPECT_EQ(list[i]->deprecated, 0);
                EXPECT_EQ(list[i]->bbox_valid, 1);
                EXPECT_EQ(list[i]->west_lon_degree, -180.0);
                EXPECT_EQ(list[i]->south_lat_degree, -90.0);
                EXPECT_EQ(list[i]->east_lon_degree, 180.0);
                EXPECT_EQ(list[i]->north_lat_degree, 90.0);
                EXPECT_TRUE(std::string(list[i]->area_name).find("World") == 0)
                    << std::string(list[i]->area_name);
                EXPECT_EQ(list[i]->projection_method_name, nullptr);
            } else if (code == "4978") {
                found4978 = true;
                EXPECT_EQ(list[i]->type, PJ_TYPE_GEOCENTRIC_CRS);
            } else if (code == "4979") {
                found4979 = true;
                EXPECT_EQ(list[i]->type, PJ_TYPE_GEOGRAPHIC_3D_CRS);
            } else if (code == "32631") {
                found32631 = true;
                EXPECT_EQ(list[i]->type, PJ_TYPE_PROJECTED_CRS);
                EXPECT_EQ(std::string(list[i]->projection_method_name),
                          "Transverse Mercator");
            } else if (code == "3855") {
                found3855 = true;
                EXPECT_EQ(list[i]->type, PJ_TYPE_VERTICAL_CRS);
            } else if (code == "3901") {
                found3901 = true;
                EXPECT_EQ(list[i]->type, PJ_TYPE_COMPOUND_CRS);
            }
            EXPECT_EQ(list[i]->deprecated, 0);
        }
        EXPECT_TRUE(found4326);
        EXPECT_TRUE(found4978);
        EXPECT_TRUE(found4979);
        EXPECT_TRUE(found32631);
        EXPECT_TRUE(found3855);
        EXPECT_TRUE(found3901);
        proj_crs_info_list_destroy(list);
    }

    // Filter on only geodetic crs
    {
        int result_count = 0;
        auto params = proj_get_crs_list_parameters_create();
        params->typesCount = 1;
        auto type = PJ_TYPE_GEODETIC_CRS;
        params->types = &type;
        auto list = proj_get_crs_info_list_from_database(m_ctxt, nullptr,
                                                         params, &result_count);
        bool foundGeog2D = false;
        bool foundGeog3D = false;
        bool foundGeocentric = false;
        bool foundGeodeticCRS =
            false; // for now, only -ocentric ellipsoidal IAU CRS
        for (int i = 0; i < result_count; i++) {
            foundGeog2D |= list[i]->type == PJ_TYPE_GEOGRAPHIC_2D_CRS;
            foundGeog3D |= list[i]->type == PJ_TYPE_GEOGRAPHIC_3D_CRS;
            foundGeocentric |= list[i]->type == PJ_TYPE_GEOCENTRIC_CRS;
            foundGeodeticCRS |= list[i]->type == PJ_TYPE_GEODETIC_CRS;
            EXPECT_TRUE(list[i]->type == PJ_TYPE_GEOGRAPHIC_2D_CRS ||
                        list[i]->type == PJ_TYPE_GEOGRAPHIC_3D_CRS ||
                        list[i]->type == PJ_TYPE_GEOCENTRIC_CRS ||
                        list[i]->type == PJ_TYPE_GEODETIC_CRS);
        }
        EXPECT_TRUE(foundGeog2D);
        EXPECT_TRUE(foundGeog3D);
        EXPECT_TRUE(foundGeocentric);
        EXPECT_TRUE(foundGeodeticCRS);
        proj_get_crs_list_parameters_destroy(params);
        proj_crs_info_list_destroy(list);
    }

    // Filter on only geographic crs
    {
        int result_count = 0;
        auto params = proj_get_crs_list_parameters_create();
        params->typesCount = 1;
        auto type = PJ_TYPE_GEOGRAPHIC_CRS;
        params->types = &type;
        auto list = proj_get_crs_info_list_from_database(m_ctxt, "EPSG", params,
                                                         &result_count);
        bool foundGeog2D = false;
        bool foundGeog3D = false;
        for (int i = 0; i < result_count; i++) {
            foundGeog2D |= list[i]->type == PJ_TYPE_GEOGRAPHIC_2D_CRS;
            foundGeog3D |= list[i]->type == PJ_TYPE_GEOGRAPHIC_3D_CRS;
            EXPECT_TRUE(list[i]->type == PJ_TYPE_GEOGRAPHIC_2D_CRS ||
                        list[i]->type == PJ_TYPE_GEOGRAPHIC_3D_CRS);
        }
        EXPECT_TRUE(foundGeog2D);
        EXPECT_TRUE(foundGeog3D);
        proj_get_crs_list_parameters_destroy(params);
        proj_crs_info_list_destroy(list);
    }

    // Filter on only geographic 2D crs and projected CRS
    {
        int result_count = 0;
        auto params = proj_get_crs_list_parameters_create();
        params->typesCount = 2;
        const PJ_TYPE types[] = {PJ_TYPE_GEOGRAPHIC_2D_CRS,
                                 PJ_TYPE_PROJECTED_CRS};
        params->types = types;
        auto list = proj_get_crs_info_list_from_database(m_ctxt, "EPSG", params,
                                                         &result_count);
        bool foundGeog2D = false;
        bool foundProjected = false;
        for (int i = 0; i < result_count; i++) {
            foundGeog2D |= list[i]->type == PJ_TYPE_GEOGRAPHIC_2D_CRS;
            foundProjected |= list[i]->type == PJ_TYPE_PROJECTED_CRS;
            EXPECT_TRUE(list[i]->type == PJ_TYPE_GEOGRAPHIC_2D_CRS ||
                        list[i]->type == PJ_TYPE_PROJECTED_CRS);
        }
        EXPECT_TRUE(foundGeog2D);
        EXPECT_TRUE(foundProjected);
        proj_get_crs_list_parameters_destroy(params);
        proj_crs_info_list_destroy(list);
    }

    // Filter on bbox (inclusion)
    {
        int result_count = 0;
        auto params = proj_get_crs_list_parameters_create();
        params->bbox_valid = 1;
        params->west_lon_degree = 2;
        params->south_lat_degree = 49;
        params->east_lon_degree = 2.1;
        params->north_lat_degree = 49.1;
        params->typesCount = 1;
        auto type = PJ_TYPE_PROJECTED_CRS;
        params->types = &type;
        auto list = proj_get_crs_info_list_from_database(m_ctxt, "EPSG", params,
                                                         &result_count);
        ASSERT_NE(list, nullptr);
        EXPECT_GT(result_count, 1);
        for (int i = 0; i < result_count; i++) {
            if (list[i]->west_lon_degree < list[i]->east_lon_degree) {
                EXPECT_LE(list[i]->west_lon_degree, params->west_lon_degree);
                EXPECT_GE(list[i]->east_lon_degree, params->east_lon_degree);
            }
            EXPECT_LE(list[i]->south_lat_degree, params->south_lat_degree);
            EXPECT_GE(list[i]->north_lat_degree, params->north_lat_degree);
        }
        proj_get_crs_list_parameters_destroy(params);
        proj_crs_info_list_destroy(list);
    }

    // Filter on bbox (intersection)
    {
        int result_count = 0;
        auto params = proj_get_crs_list_parameters_create();
        params->bbox_valid = 1;
        params->west_lon_degree = 2;
        params->south_lat_degree = 49;
        params->east_lon_degree = 2.1;
        params->north_lat_degree = 49.1;
        params->crs_area_of_use_contains_bbox = 0;
        params->typesCount = 1;
        auto type = PJ_TYPE_PROJECTED_CRS;
        params->types = &type;
        auto list = proj_get_crs_info_list_from_database(m_ctxt, "EPSG", params,
                                                         &result_count);
        ASSERT_NE(list, nullptr);
        EXPECT_GT(result_count, 1);
        for (int i = 0; i < result_count; i++) {
            if (list[i]->west_lon_degree < list[i]->east_lon_degree) {
                EXPECT_LE(list[i]->west_lon_degree, params->west_lon_degree);
                EXPECT_GE(list[i]->east_lon_degree, params->east_lon_degree);
            }
            EXPECT_LE(list[i]->south_lat_degree, params->north_lat_degree);
            EXPECT_GE(list[i]->north_lat_degree, params->south_lat_degree);
        }
        proj_get_crs_list_parameters_destroy(params);
        proj_crs_info_list_destroy(list);
    }

    // Filter on celestial body
    {
        int result_count = 0;
        auto params = proj_get_crs_list_parameters_create();
        params->celestial_body_name = "non existing";
        auto list = proj_get_crs_info_list_from_database(m_ctxt, nullptr,
                                                         params, &result_count);
        ASSERT_NE(list, nullptr);
        EXPECT_EQ(result_count, 0);
        proj_get_crs_list_parameters_destroy(params);
        proj_crs_info_list_destroy(list);
    }

    // Filter on celestial body
    {
        int result_count = 0;
        auto params = proj_get_crs_list_parameters_create();
        params->celestial_body_name = "Earth";
        auto list = proj_get_crs_info_list_from_database(m_ctxt, nullptr,
                                                         params, &result_count);
        ASSERT_NE(list, nullptr);
        EXPECT_GT(result_count, 0);
        proj_get_crs_list_parameters_destroy(params);
        proj_crs_info_list_destroy(list);
    }
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_get_units_from_database) {
    { proj_unit_list_destroy(nullptr); }

    {
        auto list = proj_get_units_from_database(nullptr, nullptr, nullptr,
                                                 true, nullptr);
        ASSERT_NE(list, nullptr);
        ASSERT_NE(list[0], nullptr);
        ASSERT_NE(list[0]->auth_name, nullptr);
        ASSERT_NE(list[0]->code, nullptr);
        ASSERT_NE(list[0]->name, nullptr);
        proj_unit_list_destroy(list);
    }

    {
        int result_count = 0;
        auto list = proj_get_units_from_database(nullptr, "EPSG", "linear",
                                                 false, &result_count);
        ASSERT_NE(list, nullptr);
        EXPECT_GT(result_count, 1);
        EXPECT_EQ(list[result_count], nullptr);
        bool found9001 = false;
        for (int i = 0; i < result_count; i++) {
            EXPECT_EQ(std::string(list[i]->auth_name), "EPSG");
            if (std::string(list[i]->code) == "9001") {
                EXPECT_EQ(std::string(list[i]->name), "metre");
                EXPECT_EQ(std::string(list[i]->category), "linear");
                EXPECT_EQ(list[i]->conv_factor, 1.0);
                ASSERT_NE(list[i]->proj_short_name, nullptr);
                EXPECT_EQ(std::string(list[i]->proj_short_name), "m");
                EXPECT_EQ(list[i]->deprecated, 0);
                found9001 = true;
            }
            EXPECT_EQ(list[i]->deprecated, 0);
        }
        EXPECT_TRUE(found9001);
        proj_unit_list_destroy(list);
    }
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_get_celestial_body_list_from_database) {
    { proj_celestial_body_list_destroy(nullptr); }

    {
        auto list =
            proj_get_celestial_body_list_from_database(nullptr, nullptr, nullptr);
        ASSERT_NE(list, nullptr);
        ASSERT_NE(list[0], nullptr);
        ASSERT_NE(list[0]->auth_name, nullptr);
        ASSERT_NE(list[0]->name, nullptr);
        proj_celestial_body_list_destroy(list);
    }
    {
        int result_count = 0;
        auto list = proj_get_celestial_body_list_from_database(nullptr, "ESRI",
                                                               &result_count);
        ASSERT_NE(list, nullptr);
        EXPECT_GT(result_count, 1);
        EXPECT_EQ(list[result_count], nullptr);
        bool foundGanymede = false;
        for (int i = 0; i < result_count; i++) {
            EXPECT_EQ(std::string(list[i]->auth_name), "ESRI");
            if (std::string(list[i]->name) == "Ganymede") {
                foundGanymede = true;
            }
        }
        EXPECT_TRUE(foundGanymede);
        proj_celestial_body_list_destroy(list);
    }
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_normalize_for_visualization) {

    {
        auto P = proj_create(m_ctxt, "+proj=utm +zone=31 +ellps=WGS84");
        ObjectKeeper keeper_P(P);
        ASSERT_NE(P, nullptr);
        auto Pnormalized = proj_normalize_for_visualization(m_ctxt, P);
        ObjectKeeper keeper_Pnormalized(Pnormalized);
        EXPECT_EQ(Pnormalized, nullptr);
    }

    auto P = proj_create_crs_to_crs(m_ctxt, "EPSG:4326", "EPSG:32631", nullptr);
    ObjectKeeper keeper_P(P);
    ASSERT_NE(P, nullptr);
    auto Pnormalized = proj_normalize_for_visualization(m_ctxt, P);
    ObjectKeeper keeper_Pnormalized(Pnormalized);
    ASSERT_NE(Pnormalized, nullptr);
    auto projstr = proj_as_proj_string(m_ctxt, Pnormalized, PJ_PROJ_5, nullptr);
    ASSERT_NE(projstr, nullptr);
    EXPECT_EQ(std::string(projstr),
              "+proj=pipeline +step +proj=unitconvert +xy_in=deg +xy_out=rad "
              "+step +proj=utm +zone=31 +ellps=WGS84");

    EXPECT_TRUE(proj_degree_input(Pnormalized, PJ_FWD));
    EXPECT_FALSE(proj_degree_output(Pnormalized, PJ_FWD));
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_normalize_for_visualization_with_alternatives) {

    auto P = proj_create_crs_to_crs(m_ctxt, "EPSG:4326", "EPSG:3003", nullptr);
    ObjectKeeper keeper_P(P);
    ASSERT_NE(P, nullptr);
    auto Pnormalized = proj_normalize_for_visualization(m_ctxt, P);
    ObjectKeeper keeper_Pnormalized(Pnormalized);
    ASSERT_NE(Pnormalized, nullptr);

    EXPECT_TRUE(proj_degree_input(Pnormalized, PJ_FWD));
    EXPECT_FALSE(proj_degree_output(Pnormalized, PJ_FWD));

    {
        PJ_COORD c;
        // Approximately Roma
        c.xyzt.x = 12.5;
        c.xyzt.y = 42;
        c.xyzt.z = 0;
        c.xyzt.t = HUGE_VAL;
        c = proj_trans(Pnormalized, PJ_FWD, c);
        EXPECT_NEAR(c.xy.x, 1789912.46264783037, 1e-8);
        EXPECT_NEAR(c.xy.y, 4655716.25402576849, 1e-8);
        auto projstr = proj_pj_info(Pnormalized).definition;
        ASSERT_NE(projstr, nullptr);
        EXPECT_EQ(std::string(projstr),
                  "proj=pipeline step proj=unitconvert xy_in=deg xy_out=rad "
                  "step proj=push v_3 step proj=cart ellps=WGS84 "
                  "step inv proj=helmert x=-104.1 y=-49.1 z=-9.9 rx=0.971 "
                  "ry=-2.917 rz=0.714 s=-11.68 convention=position_vector "
                  "step inv proj=cart ellps=intl step proj=pop v_3 "
                  "step proj=tmerc lat_0=0 lon_0=9 k=0.9996 x_0=1500000 "
                  "y_0=0 ellps=intl");
    }

    {
        PJ_COORD c;
        // Approximately Roma
        c.xyzt.x = 1789912.46264783037;
        c.xyzt.y = 4655716.25402576849;
        c.xyzt.z = 0;
        c.xyzt.t = HUGE_VAL;
        c = proj_trans(Pnormalized, PJ_INV, c);
        EXPECT_NEAR(c.lp.lam, 12.5, 1e-8);
        EXPECT_NEAR(c.lp.phi, 42, 1e-8);
    }
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_normalize_for_visualization_with_alternatives_reverse) {

    auto P = proj_create_crs_to_crs(m_ctxt, "EPSG:3003", "EPSG:4326", nullptr);
    ObjectKeeper keeper_P(P);
    ASSERT_NE(P, nullptr);
    auto Pnormalized = proj_normalize_for_visualization(m_ctxt, P);
    ObjectKeeper keeper_Pnormalized(Pnormalized);
    ASSERT_NE(Pnormalized, nullptr);

    EXPECT_FALSE(proj_degree_input(Pnormalized, PJ_FWD));
    EXPECT_TRUE(proj_degree_output(Pnormalized, PJ_FWD));

    PJ_COORD c;
    // Approximately Roma
    c.xyzt.x = 1789912.46264783037;
    c.xyzt.y = 4655716.25402576849;
    c.xyzt.z = 0;
    c.xyzt.t = HUGE_VAL;
    c = proj_trans(Pnormalized, PJ_FWD, c);
    EXPECT_NEAR(c.lp.lam, 12.5, 1e-8);
    EXPECT_NEAR(c.lp.phi, 42, 1e-8);
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_normalize_for_visualization_on_crs) {

    auto P = proj_create(m_ctxt, "EPSG:4326");
    ObjectKeeper keeper_P(P);
    ASSERT_NE(P, nullptr);
    auto Pnormalized = proj_normalize_for_visualization(m_ctxt, P);
    ObjectKeeper keeper_Pnormalized(Pnormalized);
    ASSERT_NE(Pnormalized, nullptr);
    EXPECT_EQ(proj_get_id_code(Pnormalized, 0), nullptr);

    auto cs = proj_crs_get_coordinate_system(m_ctxt, Pnormalized);
    ASSERT_NE(cs, nullptr);
    ObjectKeeper keeperCs(cs);

    const char *name = nullptr;
    ASSERT_TRUE(proj_cs_get_axis_info(m_ctxt, cs, 0, &name, nullptr, nullptr,
                                      nullptr, nullptr, nullptr, nullptr));
    ASSERT_NE(name, nullptr);
    EXPECT_EQ(std::string(name), "Geodetic longitude");
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_normalize_for_visualization_on_crs_westing_southing) {

    auto P = proj_create(m_ctxt, "EPSG:5513");
    ObjectKeeper keeper_P(P);
    ASSERT_NE(P, nullptr);
    auto Pnormalized = proj_normalize_for_visualization(m_ctxt, P);
    ObjectKeeper keeper_Pnormalized(Pnormalized);
    ASSERT_NE(Pnormalized, nullptr);
    EXPECT_EQ(proj_get_id_code(Pnormalized, 0), nullptr);

    auto cs = proj_crs_get_coordinate_system(m_ctxt, Pnormalized);
    ASSERT_NE(cs, nullptr);
    ObjectKeeper keeperCs(cs);

    const char *name = nullptr;
    ASSERT_TRUE(proj_cs_get_axis_info(m_ctxt, cs, 0, &name, nullptr, nullptr,
                                      nullptr, nullptr, nullptr, nullptr));
    ASSERT_NE(name, nullptr);
    EXPECT_EQ(std::string(name), "Westing");
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_coordoperation_create_inverse) {

    auto P = proj_create(
        m_ctxt, "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
                "+proj=unitconvert +xy_in=deg +xy_out=rad +step +proj=push "
                "+v_3 +step +proj=cart +ellps=evrst30 +step +proj=helmert "
                "+x=293 +y=836 +z=318 +rx=0.5 +ry=1.6 +rz=-2.8 +s=2.1 "
                "+convention=position_vector +step +inv +proj=cart "
                "+ellps=WGS84 +step +proj=pop +v_3 +step +proj=unitconvert "
                "+xy_in=rad +xy_out=deg +step +proj=axisswap +order=2,1");
    ObjectKeeper keeper_P(P);
    ASSERT_NE(P, nullptr);
    auto Pinversed = proj_coordoperation_create_inverse(m_ctxt, P);
    ObjectKeeper keeper_Pinversed(Pinversed);
    ASSERT_NE(Pinversed, nullptr);

    const char *options[] = {"MULTILINE=YES", "INDENTATION_WIDTH=4",
                             "MAX_LINE_LENGTH=40", nullptr};
    auto projstr = proj_as_proj_string(m_ctxt, Pinversed, PJ_PROJ_5, options);
    ASSERT_NE(projstr, nullptr);
    const char *expected_projstr = "+proj=pipeline\n"
                                   "    +step +proj=axisswap +order=2,1\n"
                                   "    +step +proj=unitconvert +xy_in=deg\n"
                                   "          +xy_out=rad\n"
                                   "    +step +proj=push +v_3\n"
                                   "    +step +proj=cart +ellps=WGS84\n"
                                   "    +step +inv +proj=helmert +x=293\n"
                                   "          +y=836 +z=318 +rx=0.5 +ry=1.6\n"
                                   "          +rz=-2.8 +s=2.1\n"
                                   "          +convention=position_vector\n"
                                   "    +step +inv +proj=cart +ellps=evrst30\n"
                                   "    +step +proj=pop +v_3\n"
                                   "    +step +proj=unitconvert +xy_in=rad\n"
                                   "          +xy_out=deg\n"
                                   "    +step +proj=axisswap +order=2,1";
    EXPECT_EQ(std::string(projstr), expected_projstr);
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_get_remarks) {
    // Transformation
    {
        auto co = proj_create_from_database(m_ctxt, "EPSG", "8048",
                                            PJ_CATEGORY_COORDINATE_OPERATION,
                                            false, nullptr);
        ObjectKeeper keeper(co);
        ASSERT_NE(co, nullptr);

        auto remarks = proj_get_remarks(co);
        ASSERT_NE(remarks, nullptr);
        EXPECT_TRUE(std::string(remarks).find(
                        "Scale difference in ppb where 1/billion = 1E-9.") == 0)
            << remarks;
    }

    // Conversion
    {
        auto co = proj_create_from_database(m_ctxt, "EPSG", "3811",
                                            PJ_CATEGORY_COORDINATE_OPERATION,
                                            false, nullptr);
        ObjectKeeper keeper(co);
        ASSERT_NE(co, nullptr);

        auto remarks = proj_get_remarks(co);
        ASSERT_NE(remarks, nullptr);
        EXPECT_EQ(remarks, std::string("Replaces Lambert 2005."));
    }
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_get_scope) {
    // Transformation
    {
        auto co = proj_create_from_database(m_ctxt, "EPSG", "8048",
                                            PJ_CATEGORY_COORDINATE_OPERATION,
                                            false, nullptr);
        ObjectKeeper keeper(co);
        ASSERT_NE(co, nullptr);

        auto scope = proj_get_scope(co);
        ASSERT_NE(scope, nullptr);
        EXPECT_EQ(scope,
                  std::string("Transformation of GDA94 coordinates that have "
                              "been derived through GNSS CORS."));
    }

    // Conversion
    {
        auto co = proj_create_from_database(m_ctxt, "EPSG", "3811",
                                            PJ_CATEGORY_COORDINATE_OPERATION,
                                            false, nullptr);
        ObjectKeeper keeper(co);
        ASSERT_NE(co, nullptr);

        auto scope = proj_get_scope(co);
        ASSERT_NE(scope, nullptr);
        EXPECT_EQ(scope,
                  std::string("Engineering survey, topographic mapping."));
    }

    {
        auto P = proj_create(m_ctxt, "+proj=noop");
        ObjectKeeper keeper(P);
        ASSERT_NE(P, nullptr);
        auto scope = proj_get_scope(P);
        ASSERT_EQ(scope, nullptr);
    }
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_concatoperation_get_step) {
    // Test on a non concatenated operation
    {
        auto co = proj_create_from_database(m_ctxt, "EPSG", "8048",
                                            PJ_CATEGORY_COORDINATE_OPERATION,
                                            false, nullptr);
        ObjectKeeper keeper(co);
        ASSERT_NE(co, nullptr);
        ASSERT_NE(proj_get_type(co), PJ_TYPE_CONCATENATED_OPERATION);

        ASSERT_EQ(proj_concatoperation_get_step_count(m_ctxt, co), 0);
        ASSERT_EQ(proj_concatoperation_get_step(m_ctxt, co, 0), nullptr);
    }
    // Test on a concatenated operation
    {
        auto ctxt = proj_create_operation_factory_context(m_ctxt, nullptr);
        ASSERT_NE(ctxt, nullptr);
        ContextKeeper keeper_ctxt(ctxt);

        // GDA94 / MGA zone 56
        auto source_crs = proj_create_from_database(
            m_ctxt, "EPSG", "28356", PJ_CATEGORY_CRS, false, nullptr);
        ASSERT_NE(source_crs, nullptr);
        ObjectKeeper keeper_source_crs(source_crs);

        // GDA2020 / MGA zone 56
        auto target_crs = proj_create_from_database(
            m_ctxt, "EPSG", "7856", PJ_CATEGORY_CRS, false, nullptr);
        ASSERT_NE(target_crs, nullptr);
        ObjectKeeper keeper_target_crs(target_crs);

        proj_operation_factory_context_set_spatial_criterion(
            m_ctxt, ctxt, PROJ_SPATIAL_CRITERION_PARTIAL_INTERSECTION);

        proj_operation_factory_context_set_grid_availability_use(
            m_ctxt, ctxt, PROJ_GRID_AVAILABILITY_IGNORED);

        auto res = proj_create_operations(m_ctxt, source_crs, target_crs, ctxt);
        ASSERT_NE(res, nullptr);
        ObjListKeeper keeper_res(res);

        ASSERT_GT(proj_list_get_count(res), 0);

        auto op = proj_list_get(m_ctxt, res, 0);
        ASSERT_NE(op, nullptr);
        ObjectKeeper keeper_op(op);

        ASSERT_EQ(proj_get_type(op), PJ_TYPE_CONCATENATED_OPERATION);
        ASSERT_EQ(proj_concatoperation_get_step_count(m_ctxt, op), 3);

        EXPECT_EQ(proj_concatoperation_get_step(m_ctxt, op, -1), nullptr);
        EXPECT_EQ(proj_concatoperation_get_step(m_ctxt, op, 3), nullptr);

        auto step = proj_concatoperation_get_step(m_ctxt, op, 1);
        ASSERT_NE(step, nullptr);
        ObjectKeeper keeper_step(step);

        const char *scope = proj_get_scope(step);
        EXPECT_NE(scope, nullptr);
        EXPECT_NE(std::string(scope), std::string());

        const char *remarks = proj_get_remarks(step);
        EXPECT_NE(remarks, nullptr);
        EXPECT_NE(std::string(remarks), std::string());
    }
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_as_projjson) {
    auto obj = proj_create(
        m_ctxt,
        Ellipsoid::WGS84->exportToJSON(JSONFormatter::create().get()).c_str());
    ObjectKeeper keeper(obj);
    ASSERT_NE(obj, nullptr);

    {
        auto projjson = proj_as_projjson(m_ctxt, obj, nullptr);
        ASSERT_NE(projjson, nullptr);
        EXPECT_EQ(std::string(projjson),
                  "{\n"
                  "  \"$schema\": "
                  "\"https://proj.org/schemas/v0.7/projjson.schema.json\",\n"
                  "  \"type\": \"Ellipsoid\",\n"
                  "  \"name\": \"WGS 84\",\n"
                  "  \"semi_major_axis\": 6378137,\n"
                  "  \"inverse_flattening\": 298.257223563,\n"
                  "  \"id\": {\n"
                  "    \"authority\": \"EPSG\",\n"
                  "    \"code\": 7030\n"
                  "  }\n"
                  "}");
    }
    {
        const char *const options[] = {"INDENTATION_WIDTH=4",
                                       "SCHEMA=", nullptr};
        auto projjson = proj_as_projjson(m_ctxt, obj, options);
        ASSERT_NE(projjson, nullptr);
        EXPECT_EQ(std::string(projjson),
                  "{\n"
                  "    \"type\": \"Ellipsoid\",\n"
                  "    \"name\": \"WGS 84\",\n"
                  "    \"semi_major_axis\": 6378137,\n"
                  "    \"inverse_flattening\": 298.257223563,\n"
                  "    \"id\": {\n"
                  "        \"authority\": \"EPSG\",\n"
                  "        \"code\": 7030\n"
                  "    }\n"
                  "}");
    }
    {
        const char *const options[] = {"MULTILINE=NO", "SCHEMA=", nullptr};
        auto projjson = proj_as_projjson(m_ctxt, obj, options);
        ASSERT_NE(projjson, nullptr);
        EXPECT_EQ(std::string(projjson),
                  "{\"type\":\"Ellipsoid\",\"name\":\"WGS 84\","
                  "\"semi_major_axis\":6378137,"
                  "\"inverse_flattening\":298.257223563,"
                  "\"id\":{\"authority\":\"EPSG\",\"code\":7030}}");
    }
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_context_copy_from_default) {
    auto c_path = proj_context_get_database_path(m_ctxt);
    ASSERT_TRUE(c_path != nullptr);
    std::string path(c_path);

    FILE *f = fopen(path.c_str(), "rb");
    ASSERT_NE(f, nullptr);
    fseek(f, 0, SEEK_END);
    auto length = ftell(f);
    std::string content;
    content.resize(static_cast<size_t>(length));
    fseek(f, 0, SEEK_SET);
    auto read_bytes = fread(&content[0], 1, content.size(), f);
    ASSERT_EQ(read_bytes, content.size());
    fclose(f);
    const char *tempdir = getenv("TEMP");
    if (!tempdir) {
        tempdir = getenv("TMP");
    }
    if (!tempdir) {
        tempdir = "/tmp";
    }
    std::string tmp_filename(std::string(tempdir) +
                             "/test_proj_context_set_autoclose_database.db");
    f = fopen(tmp_filename.c_str(), "wb");
    if (!f) {
        std::cerr << "Cannot create " << tmp_filename << std::endl;
        return;
    }
    fwrite(content.data(), 1, content.size(), f);
    fclose(f);

    auto c_default_path = proj_context_get_database_path(nullptr);
    std::string default_path(c_default_path ? c_default_path : "");
    EXPECT_TRUE(proj_context_set_database_path(nullptr, tmp_filename.c_str(),
                                               nullptr, nullptr));

    PJ_CONTEXT *new_ctx = proj_context_create();
    EXPECT_TRUE(proj_context_set_database_path(
        nullptr, default_path.empty() ? nullptr : default_path.c_str(), nullptr,
        nullptr));

    EXPECT_NE(new_ctx, nullptr);
    PjContextKeeper keeper_ctxt(new_ctx);
    auto c_new_path = proj_context_get_database_path(new_ctx);
    ASSERT_TRUE(c_new_path != nullptr);
    std::string new_db_path(c_new_path);
    ASSERT_EQ(new_db_path, tmp_filename);
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_context_clone) {
    int new_init_rules =
        proj_context_get_use_proj4_init_rules(nullptr, 0) > 0 ? 0 : 1;
    PJ_CONTEXT *new_ctx = proj_context_create();
    EXPECT_NE(new_ctx, nullptr);
    PjContextKeeper keeper_ctxt(new_ctx);
    proj_context_use_proj4_init_rules(new_ctx, new_init_rules);
    PJ_CONTEXT *clone_ctx = proj_context_clone(new_ctx);
    EXPECT_NE(clone_ctx, nullptr);
    PjContextKeeper keeper_clone_ctxt(clone_ctx);
    ASSERT_EQ(proj_context_get_use_proj4_init_rules(new_ctx, 0),
              proj_context_get_use_proj4_init_rules(clone_ctx, 0));
    EXPECT_NE(proj_context_get_use_proj4_init_rules(NULL, 0),
              proj_context_get_use_proj4_init_rules(clone_ctx, 0));
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_create_crs_to_crs_from_pj) {

    auto src = proj_create(m_ctxt, "EPSG:4326");
    ObjectKeeper keeper_src(src);
    ASSERT_NE(src, nullptr);

    auto dst = proj_create(m_ctxt, "EPSG:32631");
    ObjectKeeper keeper_dst(dst);
    ASSERT_NE(dst, nullptr);

    auto P = proj_create_crs_to_crs_from_pj(m_ctxt, src, dst, nullptr, nullptr);
    ObjectKeeper keeper_P(P);
    ASSERT_NE(P, nullptr);
    auto Pnormalized = proj_normalize_for_visualization(m_ctxt, P);
    ObjectKeeper keeper_Pnormalized(Pnormalized);
    ASSERT_NE(Pnormalized, nullptr);
    auto projstr = proj_as_proj_string(m_ctxt, Pnormalized, PJ_PROJ_5, nullptr);
    ASSERT_NE(projstr, nullptr);
    EXPECT_EQ(std::string(projstr),
              "+proj=pipeline +step +proj=unitconvert +xy_in=deg +xy_out=rad "
              "+step +proj=utm +zone=31 +ellps=WGS84");
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_create_crs_to_crs_from_pj_accuracy_filter) {

    auto src = proj_create(m_ctxt, "EPSG:4326"); // WGS 84
    ObjectKeeper keeper_src(src);
    ASSERT_NE(src, nullptr);

    auto dst = proj_create(m_ctxt, "EPSG:4258"); // ETRS89
    ObjectKeeper keeper_dst(dst);
    ASSERT_NE(dst, nullptr);

    // No options
    {
        auto P =
            proj_create_crs_to_crs_from_pj(m_ctxt, src, dst, nullptr, nullptr);
        ObjectKeeper keeper_P(P);
        ASSERT_NE(P, nullptr);
    }

    {
        const char *const options[] = {"ACCURACY=0.05", nullptr};
        auto P =
            proj_create_crs_to_crs_from_pj(m_ctxt, src, dst, nullptr, options);
        ObjectKeeper keeper_P(P);
        ASSERT_EQ(P, nullptr);
    }
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_create_crs_to_crs_from_pj_ballpark_filter) {

    auto src = proj_create(m_ctxt, "EPSG:4267"); // NAD 27
    ObjectKeeper keeper_src(src);
    ASSERT_NE(src, nullptr);

    auto dst = proj_create(m_ctxt, "EPSG:4258"); // ETRS89
    ObjectKeeper keeper_dst(dst);
    ASSERT_NE(dst, nullptr);

    // No options
    {
        auto P =
            proj_create_crs_to_crs_from_pj(m_ctxt, src, dst, nullptr, nullptr);
        ObjectKeeper keeper_P(P);
        ASSERT_NE(P, nullptr);
    }

    {
        const char *const options[] = {"ALLOW_BALLPARK=NO", nullptr};
        auto P =
            proj_create_crs_to_crs_from_pj(m_ctxt, src, dst, nullptr, options);
        ObjectKeeper keeper_P(P);
        ASSERT_EQ(P, nullptr);
    }
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_create_crs_to_crs_coordinate_metadata_in_src) {

    auto P =
        proj_create_crs_to_crs(m_ctxt, "ITRF2014@2025.0", "GDA2020", nullptr);
    ObjectKeeper keeper_P(P);
    ASSERT_NE(P, nullptr);

    PJ_COORD coord;
    coord.xyzt.x = -30;
    coord.xyzt.y = 130;
    coord.xyzt.z = 0;
    coord.xyzt.t = HUGE_VAL;

    coord = proj_trans(P, PJ_FWD, coord);
    EXPECT_NEAR(coord.xyzt.x, -30.0000026655, 1e-10);
    EXPECT_NEAR(coord.xyzt.y, 129.9999983712, 1e-10);
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_create_crs_to_crs_coordinate_metadata_in_target) {

    auto P =
        proj_create_crs_to_crs(m_ctxt, "GDA2020", "ITRF2014@2025.0", nullptr);
    ObjectKeeper keeper_P(P);
    ASSERT_NE(P, nullptr);

    PJ_COORD coord;
    coord.xyzt.x = -30.0000026655;
    coord.xyzt.y = 129.9999983712;
    coord.xyzt.z = 0;
    coord.xyzt.t = HUGE_VAL;

    coord = proj_trans(P, PJ_FWD, coord);
    EXPECT_NEAR(coord.xyzt.x, -30, 1e-10);
    EXPECT_NEAR(coord.xyzt.y, 130, 1e-10);
}

// ---------------------------------------------------------------------------

static void
check_axis_is_latitude(PJ_CONTEXT *ctx, PJ *cs, int axis_number,
                       const char *unit_name = "degree",
                       double unit_conv_factor = 0.017453292519943295,
                       const char *auth = "EPSG", const char *code = "9122") {

    const char *name = nullptr;
    const char *abbrev = nullptr;
    const char *direction = nullptr;
    double unitConvFactor = 0.0;
    const char *unitName = nullptr;
    const char *unitAuthority = nullptr;
    const char *unitCode = nullptr;

    EXPECT_TRUE(proj_cs_get_axis_info(ctx, cs, axis_number, &name, &abbrev,
                                      &direction, &unitConvFactor, &unitName,
                                      &unitAuthority, &unitCode));
    ASSERT_NE(name, nullptr);
    ASSERT_NE(abbrev, nullptr);
    ASSERT_NE(direction, nullptr);
    ASSERT_NE(unitName, nullptr);
    if (auth) {
        ASSERT_NE(unitAuthority, nullptr);
        ASSERT_NE(unitCode, nullptr);
    }
    EXPECT_EQ(std::string(name), "Latitude");
    EXPECT_EQ(std::string(abbrev), "lat");
    EXPECT_EQ(std::string(direction), "north");
    EXPECT_EQ(unitConvFactor, unit_conv_factor) << unitConvFactor;
    EXPECT_EQ(std::string(unitName), unit_name);
    if (auth) {
        EXPECT_EQ(std::string(unitAuthority), auth);
        EXPECT_EQ(std::string(unitCode), code);
    }
}

// ---------------------------------------------------------------------------

static void
check_axis_is_longitude(PJ_CONTEXT *ctx, PJ *cs, int axis_number,
                        const char *unit_name = "degree",
                        double unit_conv_factor = 0.017453292519943295,
                        const char *auth = "EPSG", const char *code = "9122") {

    const char *name = nullptr;
    const char *abbrev = nullptr;
    const char *direction = nullptr;
    double unitConvFactor = 0.0;
    const char *unitName = nullptr;
    const char *unitAuthority = nullptr;
    const char *unitCode = nullptr;

    EXPECT_TRUE(proj_cs_get_axis_info(ctx, cs, axis_number, &name, &abbrev,
                                      &direction, &unitConvFactor, &unitName,
                                      &unitAuthority, &unitCode));
    ASSERT_NE(name, nullptr);
    ASSERT_NE(abbrev, nullptr);
    ASSERT_NE(direction, nullptr);
    ASSERT_NE(unitName, nullptr);
    if (auth) {
        ASSERT_NE(unitAuthority, nullptr);
        ASSERT_NE(unitCode, nullptr);
    }
    EXPECT_EQ(std::string(name), "Longitude");
    EXPECT_EQ(std::string(abbrev), "lon");
    EXPECT_EQ(std::string(direction), "east");
    EXPECT_EQ(unitConvFactor, unit_conv_factor) << unitConvFactor;
    EXPECT_EQ(std::string(unitName), unit_name);
    if (auth) {
        EXPECT_EQ(std::string(unitAuthority), auth);
        EXPECT_EQ(std::string(unitCode), code);
    }
}

// ---------------------------------------------------------------------------

static void check_axis_is_height(PJ_CONTEXT *ctx, PJ *cs, int axis_number,
                                 const char *unit_name = "metre",
                                 double unit_conv_factor = 1,
                                 const char *auth = "EPSG",
                                 const char *code = "9001") {

    const char *name = nullptr;
    const char *abbrev = nullptr;
    const char *direction = nullptr;
    double unitConvFactor = 0.0;
    const char *unitName = nullptr;
    const char *unitAuthority = nullptr;
    const char *unitCode = nullptr;

    EXPECT_TRUE(proj_cs_get_axis_info(ctx, cs, axis_number, &name, &abbrev,
                                      &direction, &unitConvFactor, &unitName,
                                      &unitAuthority, &unitCode));
    ASSERT_NE(name, nullptr);
    ASSERT_NE(abbrev, nullptr);
    ASSERT_NE(direction, nullptr);
    ASSERT_NE(unitName, nullptr);
    if (auth) {
        ASSERT_NE(unitAuthority, nullptr);
        ASSERT_NE(unitCode, nullptr);
    }
    EXPECT_EQ(std::string(name), "Ellipsoidal height");
    EXPECT_EQ(std::string(abbrev), "h");
    EXPECT_EQ(std::string(direction), "up");
    EXPECT_EQ(unitConvFactor, unit_conv_factor) << unitConvFactor;
    EXPECT_EQ(std::string(unitName), unit_name);
    if (auth) {
        EXPECT_EQ(std::string(unitAuthority), auth);
        EXPECT_EQ(std::string(unitCode), code);
    }
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_create_ellipsoidal_3D_cs) {

    {
        auto cs = proj_create_ellipsoidal_3D_cs(
            m_ctxt, PJ_ELLPS3D_LATITUDE_LONGITUDE_HEIGHT, nullptr, 0, nullptr,
            0);
        ObjectKeeper keeper_cs(cs);
        ASSERT_NE(cs, nullptr);

        EXPECT_EQ(proj_cs_get_type(m_ctxt, cs), PJ_CS_TYPE_ELLIPSOIDAL);

        EXPECT_EQ(proj_cs_get_axis_count(m_ctxt, cs), 3);

        check_axis_is_latitude(m_ctxt, cs, 0);

        check_axis_is_longitude(m_ctxt, cs, 1);

        check_axis_is_height(m_ctxt, cs, 2);
    }

    {
        auto cs = proj_create_ellipsoidal_3D_cs(
            m_ctxt, PJ_ELLPS3D_LONGITUDE_LATITUDE_HEIGHT, "foo", 0.5, "bar",
            0.6);
        ObjectKeeper keeper_cs(cs);
        ASSERT_NE(cs, nullptr);

        EXPECT_EQ(proj_cs_get_type(m_ctxt, cs), PJ_CS_TYPE_ELLIPSOIDAL);

        EXPECT_EQ(proj_cs_get_axis_count(m_ctxt, cs), 3);

        check_axis_is_longitude(m_ctxt, cs, 0, "foo", 0.5, nullptr, nullptr);

        check_axis_is_latitude(m_ctxt, cs, 1, "foo", 0.5, nullptr, nullptr);

        check_axis_is_height(m_ctxt, cs, 2, "bar", 0.6, nullptr, nullptr);
    }
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_crs_promote_to_3D) {

    auto crs2D =
        proj_create(m_ctxt, GeographicCRS::EPSG_4326
                                ->exportToWKT(WKTFormatter::create().get())
                                .c_str());
    ObjectKeeper keeper_crs2D(crs2D);
    EXPECT_NE(crs2D, nullptr);

    auto crs3D = proj_crs_promote_to_3D(m_ctxt, nullptr, crs2D);
    ObjectKeeper keeper_crs3D(crs3D);
    EXPECT_NE(crs3D, nullptr);

    auto cs = proj_crs_get_coordinate_system(m_ctxt, crs3D);
    ASSERT_NE(cs, nullptr);
    ObjectKeeper keeperCs(cs);
    EXPECT_EQ(proj_cs_get_axis_count(m_ctxt, cs), 3);

    auto code = proj_get_id_code(crs3D, 0);
    ASSERT_TRUE(code != nullptr);
    EXPECT_EQ(code, std::string("4979"));
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_crs_promote_to_3D_on_coordinate_metadata) {

    auto cm_crs2D = proj_create(m_ctxt, "ITRF2014@2010.0");
    ObjectKeeper keeper_cm_crs2D(cm_crs2D);
    EXPECT_NE(cm_crs2D, nullptr);

    auto cm_crs3D = proj_crs_promote_to_3D(m_ctxt, nullptr, cm_crs2D);
    ObjectKeeper keeper_cm_crs3D(cm_crs3D);
    EXPECT_NE(cm_crs3D, nullptr);

    EXPECT_NEAR(proj_coordinate_metadata_get_epoch(m_ctxt, cm_crs3D), 2010.0,
                1e-10);

    auto crs3D = proj_get_source_crs(m_ctxt, cm_crs3D);
    ObjectKeeper keeper_crs3D(crs3D);
    EXPECT_NE(crs3D, nullptr);

    auto cs = proj_crs_get_coordinate_system(m_ctxt, crs3D);
    ASSERT_NE(cs, nullptr);
    ObjectKeeper keeperCs(cs);
    EXPECT_EQ(proj_cs_get_axis_count(m_ctxt, cs), 3);

    auto code = proj_get_id_code(crs3D, 0);
    ASSERT_TRUE(code != nullptr);
    EXPECT_EQ(code, std::string("7912"));
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_crs_demote_to_2D) {

    auto crs3D =
        proj_create(m_ctxt, GeographicCRS::EPSG_4979
                                ->exportToWKT(WKTFormatter::create().get())
                                .c_str());
    ObjectKeeper keeper_crs3D(crs3D);
    EXPECT_NE(crs3D, nullptr);

    auto crs2D = proj_crs_demote_to_2D(m_ctxt, nullptr, crs3D);
    ObjectKeeper keeper_crs2D(crs2D);
    EXPECT_NE(crs2D, nullptr);

    auto cs = proj_crs_get_coordinate_system(m_ctxt, crs2D);
    ASSERT_NE(cs, nullptr);
    ObjectKeeper keeperCs(cs);
    EXPECT_EQ(proj_cs_get_axis_count(m_ctxt, cs), 2);

    auto code = proj_get_id_code(crs2D, 0);
    ASSERT_TRUE(code != nullptr);
    EXPECT_EQ(code, std::string("4326"));
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_crs_create_projected_3D_crs_from_2D) {

    auto projCRS = proj_create_from_database(m_ctxt, "EPSG", "32631",
                                             PJ_CATEGORY_CRS, false, nullptr);
    ASSERT_NE(projCRS, nullptr);
    ObjectKeeper keeper_projCRS(projCRS);

    {
        auto geog3DCRS = proj_create_from_database(
            m_ctxt, "EPSG", "4979", PJ_CATEGORY_CRS, false, nullptr);
        ASSERT_NE(geog3DCRS, nullptr);
        ObjectKeeper keeper_geog3DCRS(geog3DCRS);

        auto crs3D = proj_crs_create_projected_3D_crs_from_2D(
            m_ctxt, nullptr, projCRS, geog3DCRS);
        ObjectKeeper keeper_crs3D(crs3D);
        EXPECT_NE(crs3D, nullptr);

        EXPECT_EQ(proj_get_type(crs3D), PJ_TYPE_PROJECTED_CRS);

        EXPECT_EQ(std::string(proj_get_name(crs3D)),
                  std::string(proj_get_name(projCRS)));

        auto cs = proj_crs_get_coordinate_system(m_ctxt, crs3D);
        ASSERT_NE(cs, nullptr);
        ObjectKeeper keeperCs(cs);
        EXPECT_EQ(proj_cs_get_axis_count(m_ctxt, cs), 3);
    }

    {
        auto crs3D = proj_crs_create_projected_3D_crs_from_2D(m_ctxt, nullptr,
                                                              projCRS, nullptr);
        ObjectKeeper keeper_crs3D(crs3D);
        EXPECT_NE(crs3D, nullptr);

        EXPECT_EQ(proj_get_type(crs3D), PJ_TYPE_PROJECTED_CRS);

        EXPECT_EQ(std::string(proj_get_name(crs3D)),
                  std::string(proj_get_name(projCRS)));

        auto cs = proj_crs_get_coordinate_system(m_ctxt, crs3D);
        ASSERT_NE(cs, nullptr);
        ObjectKeeper keeperCs(cs);
        EXPECT_EQ(proj_cs_get_axis_count(m_ctxt, cs), 3);
    }
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_crs_create_bound_vertical_crs) {

    auto vert_crs = proj_create_vertical_crs(m_ctxt, "myVertCRS", "myVertDatum",
                                             nullptr, 0.0);
    ObjectKeeper keeper_vert_crs(vert_crs);
    ASSERT_NE(vert_crs, nullptr);

    auto crs4979 = proj_create_from_wkt(
        m_ctxt,
        GeographicCRS::EPSG_4979->exportToWKT(WKTFormatter::create().get())
            .c_str(),
        nullptr, nullptr, nullptr);
    ObjectKeeper keeper_crs4979(crs4979);
    ASSERT_NE(crs4979, nullptr);

    auto bound_crs = proj_crs_create_bound_vertical_crs(m_ctxt, vert_crs,
                                                        crs4979, "foo.gtx");
    ObjectKeeper keeper_bound_crs(bound_crs);
    ASSERT_NE(bound_crs, nullptr);

    auto projCRS = proj_create_from_database(m_ctxt, "EPSG", "32631",
                                             PJ_CATEGORY_CRS, false, nullptr);
    ASSERT_NE(projCRS, nullptr);
    ObjectKeeper keeper_projCRS(projCRS);

    auto compound_crs =
        proj_create_compound_crs(m_ctxt, "myCompoundCRS", projCRS, bound_crs);
    ObjectKeeper keeper_compound_crss(compound_crs);
    ASSERT_NE(compound_crs, nullptr);

    auto proj_4 = proj_as_proj_string(m_ctxt, compound_crs, PJ_PROJ_4, nullptr);
    ASSERT_NE(proj_4, nullptr);
    EXPECT_EQ(std::string(proj_4),
              "+proj=utm +zone=31 +datum=WGS84 +units=m +geoidgrids=foo.gtx "
              "+geoid_crs=WGS84 "
              "+vunits=m +no_defs +type=crs");
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_create_crs_to_crs_with_only_ballpark_transformations) {
    // ETRS89 / UTM zone 31N + EGM96 height to WGS 84 (G1762)
    auto P =
        proj_create_crs_to_crs(m_ctxt, "EPSG:25831+5773", "EPSG:7665", nullptr);
    ObjectKeeper keeper_P(P);
    ASSERT_NE(P, nullptr);
    auto Pnormalized = proj_normalize_for_visualization(m_ctxt, P);
    ObjectKeeper keeper_Pnormalized(Pnormalized);
    ASSERT_NE(Pnormalized, nullptr);

    PJ_COORD coord;
    coord.xyzt.x = 500000;
    coord.xyzt.y = 4500000;
    coord.xyzt.z = 0;
    coord.xyzt.t = 0;
    coord = proj_trans(Pnormalized, PJ_FWD, coord);
    EXPECT_NEAR(coord.xyzt.x, 3.0, 1e-9);
    EXPECT_NEAR(coord.xyzt.y, 40.65085651660555, 1e-9);
    EXPECT_NEAR(coord.xyzt.z, 47.72600023608570, 1e-3);
}

// ---------------------------------------------------------------------------

TEST_F(
    CApi,
    proj_create_crs_to_crs_from_custom_compound_crs_with_NAD83_2011_and_geoidgrid_ref_against_WGS84_to_WGS84_G1762) {

    PJ *P;

    PJ *inCrsH = proj_create_from_database(m_ctxt, "EPSG", "6340",
                                           PJ_CATEGORY_CRS, false, nullptr);
    ASSERT_NE(inCrsH, nullptr);

    PJ *inDummyCrs = proj_create_vertical_crs(m_ctxt, "VerticalDummyCrs",
                                              "DummyDatum", "metre", 1.0);
    ASSERT_NE(inDummyCrs, nullptr);

    auto crs4979 = proj_create_from_database(m_ctxt, "EPSG", "4979",
                                             PJ_CATEGORY_CRS, false, nullptr);
    ASSERT_NE(crs4979, nullptr);

    PJ *inCrsV = proj_crs_create_bound_vertical_crs(m_ctxt, inDummyCrs, crs4979,
                                                    "egm96_15.gtx");
    ASSERT_NE(inCrsV, nullptr);
    proj_destroy(inDummyCrs);
    proj_destroy(crs4979);

    PJ *inCompound =
        proj_create_compound_crs(m_ctxt, "Compound", inCrsH, inCrsV);
    ASSERT_NE(inCompound, nullptr);
    proj_destroy(inCrsH);
    proj_destroy(inCrsV);

    PJ *outCrs = proj_create(m_ctxt, "EPSG:7665");
    ASSERT_NE(outCrs, nullptr);

    // In this particular case, PROJ computes a transformation from NAD83(2011)
    // (EPSG:6318) to WGS84 (EPSG:4979) for the initial horizontal adjustment
    // before the geoidgrids application. There are 6 candidate transformations
    // for that in subzones of the US and one last no-op transformation flagged
    // as ballpark. That one used to be eliminated because by
    // proj_create_crs_to_crs() because there were non Ballpark transformations
    // available. This resulted thus in an error when transforming outside of
    // those few subzones.
    P = proj_create_crs_to_crs_from_pj(m_ctxt, inCompound, outCrs, nullptr,
                                       nullptr);
    ASSERT_NE(P, nullptr);
    proj_destroy(inCompound);
    proj_destroy(outCrs);

    PJ_COORD in_coord;
    in_coord.xyzt.x = 350499.911;
    in_coord.xyzt.y = 3884807.956;
    in_coord.xyzt.z = 150.072;
    in_coord.xyzt.t = 2010;

    PJ_COORD outcoord = proj_trans(P, PJ_FWD, in_coord);
    proj_destroy(P);

    EXPECT_NEAR(outcoord.xyzt.x, 35.09499307271, 1e-9);
    EXPECT_NEAR(outcoord.xyzt.y, -118.64014868921, 1e-9);
    EXPECT_NEAR(outcoord.xyzt.z, 117.655, 1e-3);
}

// ---------------------------------------------------------------------------

TEST_F(
    CApi,
    proj_create_crs_to_crs_from_custom_compound_crs_with_NAD83_2011_and_geoidgrid_ref_against_NAD83_2011_to_WGS84_G1762) {

    PJ *P;

    // NAD83(2011) 2D
    PJ *inCrsH = proj_create_from_database(m_ctxt, "EPSG", "6318",
                                           PJ_CATEGORY_CRS, false, nullptr);
    ASSERT_NE(inCrsH, nullptr);

    PJ *inDummyCrs = proj_create_vertical_crs(m_ctxt, "VerticalDummyCrs",
                                              "DummyDatum", "metre", 1.0);
    ASSERT_NE(inDummyCrs, nullptr);

    // NAD83(2011) 3D
    PJ *inGeog3DCRS = proj_create_from_database(
        m_ctxt, "EPSG", "6319", PJ_CATEGORY_CRS, false, nullptr);
    ASSERT_NE(inCrsH, nullptr);

    // Note: this is actually a bad example since we tell here that egm96_15.gtx
    // is referenced against NAD83(2011)
    PJ *inCrsV = proj_crs_create_bound_vertical_crs(
        m_ctxt, inDummyCrs, inGeog3DCRS, "egm96_15.gtx");
    ASSERT_NE(inCrsV, nullptr);
    proj_destroy(inDummyCrs);
    proj_destroy(inGeog3DCRS);

    PJ *inCompound =
        proj_create_compound_crs(m_ctxt, "Compound", inCrsH, inCrsV);
    ASSERT_NE(inCompound, nullptr);
    proj_destroy(inCrsH);
    proj_destroy(inCrsV);

    // WGS84 (G1762)
    PJ *outCrs = proj_create(m_ctxt, "EPSG:7665");
    ASSERT_NE(outCrs, nullptr);

    P = proj_create_crs_to_crs_from_pj(m_ctxt, inCompound, outCrs, nullptr,
                                       nullptr);
    ASSERT_NE(P, nullptr);
    proj_destroy(inCompound);
    proj_destroy(outCrs);

    PJ_COORD in_coord;
    in_coord.xyzt.x = 35;
    in_coord.xyzt.y = -118;
    in_coord.xyzt.z = 0;
    in_coord.xyzt.t = 2010;

    PJ_COORD outcoord = proj_trans(P, PJ_FWD, in_coord);
    proj_destroy(P);

    EXPECT_NEAR(outcoord.xyzt.x, 35.000003665064803, 1e-9);
    EXPECT_NEAR(outcoord.xyzt.y, -118.00001414221214, 1e-9);
    EXPECT_NEAR(outcoord.xyzt.z, -32.8110, 1e-3);
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_create_vertical_crs_ex) {

    // NAD83(2011) / UTM zone 11N
    auto horiz_crs = proj_create_from_database(m_ctxt, "EPSG", "6340",
                                               PJ_CATEGORY_CRS, false, nullptr);
    ObjectKeeper keeper_horiz_crs(horiz_crs);
    ASSERT_NE(horiz_crs, nullptr);

    const char *options[] = {"ACCURACY=123", nullptr};
    auto vert_crs = proj_create_vertical_crs_ex(
        m_ctxt, "myVertCRS (ftUS)", "myVertDatum", nullptr, nullptr,
        "US survey foot", 0.304800609601219, "PROJ @foo.gtx", nullptr, nullptr,
        nullptr, options);
    ObjectKeeper keeper_vert_crs(vert_crs);
    ASSERT_NE(vert_crs, nullptr);

    auto compound =
        proj_create_compound_crs(m_ctxt, "Compound", horiz_crs, vert_crs);
    ObjectKeeper keeper_compound(compound);
    ASSERT_NE(compound, nullptr);

    // NAD83(2011) 3D
    PJ *geog_crs = proj_create(m_ctxt, "EPSG:6319");
    ObjectKeeper keeper_geog_crs(geog_crs);
    ASSERT_NE(geog_crs, nullptr);

    PJ_OPERATION_FACTORY_CONTEXT *ctxt =
        proj_create_operation_factory_context(m_ctxt, nullptr);
    ASSERT_NE(ctxt, nullptr);
    ContextKeeper keeper_ctxt(ctxt);
    proj_operation_factory_context_set_grid_availability_use(
        m_ctxt, ctxt, PROJ_GRID_AVAILABILITY_IGNORED);
    proj_operation_factory_context_set_spatial_criterion(
        m_ctxt, ctxt, PROJ_SPATIAL_CRITERION_PARTIAL_INTERSECTION);
    PJ_OBJ_LIST *operations =
        proj_create_operations(m_ctxt, compound, geog_crs, ctxt);
    ASSERT_NE(operations, nullptr);
    ObjListKeeper keeper_operations(operations);
    EXPECT_GE(proj_list_get_count(operations), 1);
    auto P = proj_list_get(m_ctxt, operations, 0);
    ObjectKeeper keeper_transform(P);

    auto name = proj_get_name(P);
    ASSERT_TRUE(name != nullptr);
    EXPECT_EQ(name,
              std::string("Inverse of UTM zone 11N + "
                          "Conversion from myVertCRS (ftUS) to myVertCRS + "
                          "Transformation from myVertCRS to NAD83(2011)"));

    auto proj_5 = proj_as_proj_string(m_ctxt, P, PJ_PROJ_5, nullptr);
    ASSERT_NE(proj_5, nullptr);
    EXPECT_EQ(std::string(proj_5),
              "+proj=pipeline "
              "+step +inv +proj=utm +zone=11 +ellps=GRS80 "
              "+step +proj=unitconvert +z_in=us-ft +z_out=m "
              "+step +proj=vgridshift +grids=@foo.gtx +multiplier=1 "
              "+step +proj=unitconvert +xy_in=rad +xy_out=deg "
              "+step +proj=axisswap +order=2,1");

    ASSERT_EQ(proj_coordoperation_get_accuracy(m_ctxt, P), 123.0);
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_create_vertical_crs_ex_with_geog_crs) {

    // NAD83(2011) / UTM zone 11N
    auto horiz_crs = proj_create_from_database(m_ctxt, "EPSG", "6340",
                                               PJ_CATEGORY_CRS, false, nullptr);
    ObjectKeeper keeper_horiz_crs(horiz_crs);
    ASSERT_NE(horiz_crs, nullptr);

    // WGS84
    PJ *wgs84 = proj_create(m_ctxt, "EPSG:4979");
    ObjectKeeper keeper_wgs84(wgs84);
    ASSERT_NE(wgs84, nullptr);

    auto vert_crs = proj_create_vertical_crs_ex(
        m_ctxt, "myVertCRS", "myVertDatum", nullptr, nullptr, "US survey foot",
        0.304800609601219, "PROJ @foo.gtx", nullptr, nullptr, wgs84, nullptr);
    ObjectKeeper keeper_vert_crs(vert_crs);
    ASSERT_NE(vert_crs, nullptr);

    auto compound =
        proj_create_compound_crs(m_ctxt, "Compound", horiz_crs, vert_crs);
    ObjectKeeper keeper_compound(compound);
    ASSERT_NE(compound, nullptr);

    // NAD83(2011) 3D
    PJ *geog_crs = proj_create(m_ctxt, "EPSG:6319");
    ObjectKeeper keeper_geog_crs(geog_crs);
    ASSERT_NE(geog_crs, nullptr);

    PJ_OPERATION_FACTORY_CONTEXT *ctxt =
        proj_create_operation_factory_context(m_ctxt, nullptr);
    ASSERT_NE(ctxt, nullptr);
    ContextKeeper keeper_ctxt(ctxt);
    proj_operation_factory_context_set_grid_availability_use(
        m_ctxt, ctxt, PROJ_GRID_AVAILABILITY_IGNORED);
    proj_operation_factory_context_set_spatial_criterion(
        m_ctxt, ctxt, PROJ_SPATIAL_CRITERION_PARTIAL_INTERSECTION);
    PJ_OBJ_LIST *operations =
        proj_create_operations(m_ctxt, compound, geog_crs, ctxt);
    ASSERT_NE(operations, nullptr);
    ObjListKeeper keeper_operations(operations);
    EXPECT_GE(proj_list_get_count(operations), 1);
    auto P = proj_list_get(m_ctxt, operations, 0);
    ObjectKeeper keeper_transform(P);

    auto name = proj_get_name(P);
    ASSERT_TRUE(name != nullptr);
    EXPECT_EQ(name,
              std::string("Inverse of UTM zone 11N + "
                          "Conversion from myVertCRS to myVertCRS (metre) + "
                          "Transformation from myVertCRS (metre) to WGS 84 "
                          "using NAD83(2011) to WGS 84 (1)"));

    auto proj_5 = proj_as_proj_string(m_ctxt, P, PJ_PROJ_5, nullptr);
    ASSERT_NE(proj_5, nullptr);
    EXPECT_EQ(std::string(proj_5),
              "+proj=pipeline "
              "+step +inv +proj=utm +zone=11 +ellps=GRS80 "
              "+step +proj=unitconvert +z_in=us-ft +z_out=m "
              "+step +proj=vgridshift +grids=@foo.gtx +multiplier=1 "
              "+step +proj=unitconvert +xy_in=rad +xy_out=deg "
              "+step +proj=axisswap +order=2,1");

    // Check that we get the same results after an export of compoundCRS to
    // PROJJSON and a re-import from it.
    auto projjson = proj_as_projjson(m_ctxt, compound, nullptr);
    ASSERT_NE(projjson, nullptr);
    auto compound_from_projjson = proj_create(m_ctxt, projjson);
    ObjectKeeper keeper_compound_from_projjson(compound_from_projjson);
    ASSERT_NE(compound_from_projjson, nullptr);

    PJ_OBJ_LIST *operations2 =
        proj_create_operations(m_ctxt, compound_from_projjson, geog_crs, ctxt);
    ASSERT_NE(operations2, nullptr);
    ObjListKeeper keeper_operations2(operations2);
    EXPECT_GE(proj_list_get_count(operations2), 1);
    auto P2 = proj_list_get(m_ctxt, operations2, 0);
    ObjectKeeper keeper_transform2(P2);

    auto name_bis = proj_get_name(P2);
    ASSERT_TRUE(name_bis != nullptr);
    EXPECT_EQ(std::string(name_bis), std::string(name));

    auto proj_5_bis = proj_as_proj_string(m_ctxt, P2, PJ_PROJ_5, nullptr);
    ASSERT_NE(proj_5_bis, nullptr);
    EXPECT_EQ(std::string(proj_5_bis), std::string(proj_5));
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_create_vertical_crs_ex_implied_accuracy) {

    PJ *crsH = proj_create(m_ctxt, "EPSG:4283"); // GDA94
    ASSERT_NE(crsH, nullptr);
    ObjectKeeper keeper_crsH(crsH);
    PJ *crsV = proj_create(m_ctxt, "EPSG:5711"); // AHD height
    ASSERT_NE(crsV, nullptr);
    ObjectKeeper keeper_crsV(crsV);
    PJ *crsGeoid = proj_create(m_ctxt, "EPSG:4939"); // GDA94 3D
    ASSERT_NE(crsGeoid, nullptr);
    ObjectKeeper keeper_crsGeoid(crsGeoid);

    PJ *vertDatum = proj_crs_get_datum(m_ctxt, crsV);
    ObjectKeeper keeper_vertDatum(vertDatum);
    const char *vertDatumName = proj_get_name(vertDatum);
    const char *vertDatumAuthority = proj_get_id_auth_name(vertDatum, 0);
    const char *vertDatumCode = proj_get_id_code(vertDatum, 0);
    PJ *crsVGeoid = proj_create_vertical_crs_ex(
        m_ctxt, "Vertical", vertDatumName, vertDatumAuthority, vertDatumCode,
        "metre", 1.0, "PROJ au_ga_AUSGeoid09_V1.01.tif", nullptr, nullptr,
        crsGeoid, nullptr);
    ObjectKeeper keeper_crsVGeoid(crsVGeoid);
    PJ *crsCompoundGeoid = proj_create_compound_crs(
        m_ctxt, "Compound with geoid", crsH, crsVGeoid);
    ObjectKeeper keeper_crsCompoundGeoid(crsCompoundGeoid);

    PJ_OPERATION_FACTORY_CONTEXT *ctxt =
        proj_create_operation_factory_context(m_ctxt, nullptr);
    ASSERT_NE(ctxt, nullptr);
    ContextKeeper keeper_ctxt(ctxt);
    proj_operation_factory_context_set_grid_availability_use(
        m_ctxt, ctxt, PROJ_GRID_AVAILABILITY_IGNORED);
    proj_operation_factory_context_set_spatial_criterion(
        m_ctxt, ctxt, PROJ_SPATIAL_CRITERION_PARTIAL_INTERSECTION);
    PJ_OBJ_LIST *operations =
        proj_create_operations(m_ctxt, crsCompoundGeoid, crsGeoid, ctxt);
    ASSERT_NE(operations, nullptr);
    ObjListKeeper keeper_operations(operations);
    EXPECT_GE(proj_list_get_count(operations), 1);
    PJ *transform = proj_list_get(m_ctxt, operations, 0);
    ObjectKeeper keeper_transform(transform);

    // This is the accuracy of operations EPSG:5656 / 5657
    const double acc = proj_coordoperation_get_accuracy(m_ctxt, transform);
    EXPECT_NEAR(acc, 0.15, 1e-10);

    // Check there's an associated area of use
    double west_lon_degree = 0;
    double south_lat_degree = 0;
    double east_lon_degree = 0;
    double north_lat_degree = 0;
    ASSERT_EQ(proj_get_area_of_use(m_ctxt, transform, &west_lon_degree,
                                   &south_lat_degree, &east_lon_degree,
                                   &north_lat_degree, nullptr),
              true);
    EXPECT_LE(north_lat_degree, -10);
    EXPECT_GE(west_lon_degree, 110);
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_create_derived_geographic_crs) {

    PJ *crs_4326 = proj_create(m_ctxt, "EPSG:4326");
    ObjectKeeper keeper_crs_4326(crs_4326);
    ASSERT_NE(crs_4326, nullptr);

    PJ *conversion = proj_create_conversion_pole_rotation_grib_convention(
        m_ctxt, 2, 3, 4, "Degree", 0.0174532925199433);
    ObjectKeeper keeper_conversion(conversion);
    ASSERT_NE(conversion, nullptr);

    PJ *cs = proj_crs_get_coordinate_system(m_ctxt, crs_4326);
    ObjectKeeper keeper_cs(cs);
    ASSERT_NE(cs, nullptr);

    ASSERT_EQ(
        proj_create_derived_geographic_crs(m_ctxt, "my rotated CRS",
                                           conversion, // wrong type of object
                                           conversion, cs),
        nullptr);

    ASSERT_EQ(
        proj_create_derived_geographic_crs(m_ctxt, "my rotated CRS", crs_4326,
                                           crs_4326, // wrong type of object
                                           cs),
        nullptr);

    ASSERT_EQ(proj_create_derived_geographic_crs(
                  m_ctxt, "my rotated CRS", crs_4326, conversion,
                  conversion // wrong type of object
                  ),
              nullptr);

    PJ *derived_crs = proj_create_derived_geographic_crs(
        m_ctxt, "my rotated CRS", crs_4326, conversion, cs);
    ObjectKeeper keeper_derived_crs(derived_crs);
    ASSERT_NE(derived_crs, nullptr);

    EXPECT_FALSE(proj_is_derived_crs(m_ctxt, crs_4326));
    EXPECT_TRUE(proj_is_derived_crs(m_ctxt, derived_crs));

    auto wkt = proj_as_wkt(m_ctxt, derived_crs, PJ_WKT2_2019, nullptr);
    const char *expected_wkt =
        "GEOGCRS[\"my rotated CRS\",\n"
        "    BASEGEOGCRS[\"WGS 84\",\n"
        "        ENSEMBLE[\"World Geodetic System 1984 ensemble\",\n"
        "            MEMBER[\"World Geodetic System 1984 (Transit)\"],\n"
        "            MEMBER[\"World Geodetic System 1984 (G730)\"],\n"
        "            MEMBER[\"World Geodetic System 1984 (G873)\"],\n"
        "            MEMBER[\"World Geodetic System 1984 (G1150)\"],\n"
        "            MEMBER[\"World Geodetic System 1984 (G1674)\"],\n"
        "            MEMBER[\"World Geodetic System 1984 (G1762)\"],\n"
        "            MEMBER[\"World Geodetic System 1984 (G2139)\"],\n"
        "            ELLIPSOID[\"WGS 84\",6378137,298.257223563,\n"
        "                LENGTHUNIT[\"metre\",1]],\n"
        "            ENSEMBLEACCURACY[2.0]],\n"
        "        PRIMEM[\"Greenwich\",0,\n"
        "            ANGLEUNIT[\"degree\",0.0174532925199433]]],\n"
        "    DERIVINGCONVERSION[\"Pole rotation (GRIB convention)\",\n"
        "        METHOD[\"Pole rotation (GRIB convention)\"],\n"
        "        PARAMETER[\"Latitude of the southern pole (GRIB "
        "convention)\",2,\n"
        "            ANGLEUNIT[\"degree\",0.0174532925199433,\n"
        "                ID[\"EPSG\",9122]]],\n"
        "        PARAMETER[\"Longitude of the southern pole (GRIB "
        "convention)\",3,\n"
        "            ANGLEUNIT[\"degree\",0.0174532925199433,\n"
        "                ID[\"EPSG\",9122]]],\n"
        "        PARAMETER[\"Axis rotation (GRIB convention)\",4,\n"
        "            ANGLEUNIT[\"degree\",0.0174532925199433,\n"
        "                ID[\"EPSG\",9122]]]],\n"
        "    CS[ellipsoidal,2],\n"
        "        AXIS[\"geodetic latitude (Lat)\",north,\n"
        "            ORDER[1],\n"
        "            ANGLEUNIT[\"degree\",0.0174532925199433,\n"
        "                ID[\"EPSG\",9122]]],\n"
        "        AXIS[\"geodetic longitude (Lon)\",east,\n"
        "            ORDER[2],\n"
        "            ANGLEUNIT[\"degree\",0.0174532925199433,\n"
        "                ID[\"EPSG\",9122]]]]";

    ASSERT_NE(wkt, nullptr);
    EXPECT_EQ(wkt, std::string(expected_wkt));

    auto proj_5 = proj_as_proj_string(m_ctxt, derived_crs, PJ_PROJ_5, nullptr);
    ASSERT_NE(proj_5, nullptr);
    EXPECT_EQ(proj_5, std::string("+proj=ob_tran +o_proj=longlat +o_lon_p=-4 "
                                  "+o_lat_p=-2 +lon_0=3 +datum=WGS84 +no_defs "
                                  "+type=crs"));
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_create_derived_geographic_crs_netcdf_cf) {

    PJ *crs_4019 = proj_create(m_ctxt, "EPSG:4019");
    ObjectKeeper keeper_crs_4019(crs_4019);
    ASSERT_NE(crs_4019, nullptr);

    PJ *conversion = proj_create_conversion_pole_rotation_netcdf_cf_convention(
        m_ctxt, 2, 3, 4, "Degree", 0.0174532925199433);
    ObjectKeeper keeper_conversion(conversion);
    ASSERT_NE(conversion, nullptr);

    PJ *cs = proj_crs_get_coordinate_system(m_ctxt, crs_4019);
    ObjectKeeper keeper_cs(cs);
    ASSERT_NE(cs, nullptr);

    PJ *derived_crs = proj_create_derived_geographic_crs(
        m_ctxt, "my rotated CRS", crs_4019, conversion, cs);
    ObjectKeeper keeper_derived_crs(derived_crs);
    ASSERT_NE(derived_crs, nullptr);

    auto wkt = proj_as_wkt(m_ctxt, derived_crs, PJ_WKT2_2019, nullptr);
    const char *expected_wkt =
        "GEOGCRS[\"my rotated CRS\",\n"
        "    BASEGEOGCRS[\"Unknown datum based upon the GRS 1980 ellipsoid\",\n"
        "        DATUM[\"Not specified (based on GRS 1980 ellipsoid)\",\n"
        "            ELLIPSOID[\"GRS 1980\",6378137,298.257222101,\n"
        "                LENGTHUNIT[\"metre\",1]]],\n"
        "        PRIMEM[\"Greenwich\",0,\n"
        "            ANGLEUNIT[\"degree\",0.0174532925199433]]],\n"
        "    DERIVINGCONVERSION[\"Pole rotation (netCDF CF convention)\",\n"
        "        METHOD[\"Pole rotation (netCDF CF convention)\"],\n"
        "        PARAMETER[\"Grid north pole latitude (netCDF CF "
        "convention)\",2,\n"
        "            ANGLEUNIT[\"degree\",0.0174532925199433,\n"
        "                ID[\"EPSG\",9122]]],\n"
        "        PARAMETER[\"Grid north pole longitude (netCDF CF "
        "convention)\",3,\n"
        "            ANGLEUNIT[\"degree\",0.0174532925199433,\n"
        "                ID[\"EPSG\",9122]]],\n"
        "        PARAMETER[\"North pole grid longitude (netCDF CF "
        "convention)\",4,\n"
        "            ANGLEUNIT[\"degree\",0.0174532925199433,\n"
        "                ID[\"EPSG\",9122]]]],\n"
        "    CS[ellipsoidal,2],\n"
        "        AXIS[\"geodetic latitude (Lat)\",north,\n"
        "            ORDER[1],\n"
        "            ANGLEUNIT[\"degree\",0.0174532925199433,\n"
        "                ID[\"EPSG\",9122]]],\n"
        "        AXIS[\"geodetic longitude (Lon)\",east,\n"
        "            ORDER[2],\n"
        "            ANGLEUNIT[\"degree\",0.0174532925199433,\n"
        "                ID[\"EPSG\",9122]]]]";

    ASSERT_NE(wkt, nullptr);
    EXPECT_EQ(wkt, std::string(expected_wkt));

    auto proj_5 = proj_as_proj_string(m_ctxt, derived_crs, PJ_PROJ_5, nullptr);
    ASSERT_NE(proj_5, nullptr);
    EXPECT_EQ(proj_5, std::string("+proj=ob_tran +o_proj=longlat +o_lon_p=4 "
                                  "+o_lat_p=2 +lon_0=183 +ellps=GRS80 +no_defs "
                                  "+type=crs"));
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_context_set_sqlite3_vfs_name) {

    PJ_CONTEXT *ctx = proj_context_create();
    proj_log_func(ctx, nullptr, [](void *, int, const char *) -> void {});

    // Set a dummy VFS and check it is taken into account
    // (failure to open proj.db)
    proj_context_set_sqlite3_vfs_name(ctx, "dummy_vfs_name");
    ASSERT_EQ(proj_create(ctx, "EPSG:4326"), nullptr);

    // Restore default VFS
    proj_context_set_sqlite3_vfs_name(ctx, nullptr);
    PJ *crs_4326 = proj_create(ctx, "EPSG:4326");
    ASSERT_NE(crs_4326, nullptr);
    proj_destroy(crs_4326);

    proj_context_destroy(ctx);
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_context_set_sqlite3_vfs_name__from_global_context) {

    // Set a dummy VFS and check it is taken into account
    // (failure to open proj.db)
    proj_context_set_sqlite3_vfs_name(nullptr, "dummy_vfs_name");

    PJ_CONTEXT *ctx = proj_context_create();
    proj_log_func(ctx, nullptr, [](void *, int, const char *) -> void {});

    ASSERT_EQ(proj_create(ctx, "EPSG:4326"), nullptr);

    // Restore default VFS
    proj_context_set_sqlite3_vfs_name(nullptr, nullptr);
    proj_context_destroy(ctx);
}

// ---------------------------------------------------------------------------

TEST_F(CApi, use_proj4_init_rules) {
    PJ_CONTEXT *ctx = proj_context_create();
    proj_context_use_proj4_init_rules(ctx, true);
    ASSERT_TRUE(proj_context_get_use_proj4_init_rules(ctx, true));

    {
        // Test +over
        auto crs = proj_create(ctx, "+init=epsg:28992 +over");
        ObjectKeeper keeper_crs(crs);
        ASSERT_NE(crs, nullptr);

        auto datum = proj_crs_get_datum(ctx, crs);
        ASSERT_NE(datum, nullptr);
        ObjectKeeper keeper_datum(datum);

        auto datum_name = proj_get_name(datum);
        ASSERT_TRUE(datum_name != nullptr);
        EXPECT_EQ(datum_name, std::string("Amersfoort"));

        auto proj_5 = proj_as_proj_string(ctx, crs, PJ_PROJ_5, nullptr);
        ASSERT_NE(proj_5, nullptr);
        EXPECT_EQ(std::string(proj_5),
                  "+proj=sterea +lat_0=52.1561605555556 "
                  "+lon_0=5.38763888888889 +k=0.9999079 +x_0=155000 "
                  "+y_0=463000 +ellps=bessel +units=m +over "
                  "+no_defs +type=crs");
    }

    {
        // Test +over on epsg:3857
        auto crs = proj_create(ctx, "+init=epsg:3857 +over");
        ObjectKeeper keeper_crs(crs);
        ASSERT_NE(crs, nullptr);

        auto proj_5 = proj_as_proj_string(ctx, crs, PJ_PROJ_5, nullptr);
        ASSERT_NE(proj_5, nullptr);
        EXPECT_EQ(std::string(proj_5),
                  "+proj=merc +a=6378137 +b=6378137 +lat_ts=0 +lon_0=0 +x_0=0 "
                  "+y_0=0 +k=1 +units=m +nadgrids=@null +over +wktext "
                  "+no_defs +type=crs");
    }

    proj_context_use_proj4_init_rules(ctx, false);
    ASSERT_TRUE(!proj_context_get_use_proj4_init_rules(ctx, true));
    proj_context_destroy(ctx);
}

// ---------------------------------------------------------------------------

TEST_F(CApi, use_proj4_init_rules_from_global_context) {

    int initial_rules = proj_context_get_use_proj4_init_rules(nullptr, true);
    proj_context_use_proj4_init_rules(nullptr, true);
    PJ_CONTEXT *ctx = proj_context_create();
    ASSERT_TRUE(proj_context_get_use_proj4_init_rules(ctx, true));
    proj_context_destroy(ctx);
    proj_context_use_proj4_init_rules(nullptr, false);
    ctx = proj_context_create();
    ASSERT_TRUE(!proj_context_get_use_proj4_init_rules(ctx, true));
    proj_context_destroy(ctx);
    proj_context_use_proj4_init_rules(nullptr, initial_rules);
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_is_equivalent_to_with_ctx) {
    auto from_epsg = proj_create_from_database(m_ctxt, "EPSG", "7844",
                                               PJ_CATEGORY_CRS, false, nullptr);
    ObjectKeeper keeper_from_epsg(from_epsg);
    ASSERT_NE(from_epsg, nullptr);

    auto wkt = "GEOGCRS[\"GDA2020\",\n"
               "    DATUM[\"GDA2020\",\n"
               "        ELLIPSOID[\"GRS_1980\",6378137,298.257222101,\n"
               "            LENGTHUNIT[\"metre\",1]]],\n"
               "    PRIMEM[\"Greenwich\",0,\n"
               "        ANGLEUNIT[\"Degree\",0.0174532925199433]],\n"
               "    CS[ellipsoidal,2],\n"
               "        AXIS[\"geodetic latitude (Lat)\",north,\n"
               "            ORDER[1],\n"
               "            ANGLEUNIT[\"degree\",0.0174532925199433]],\n"
               "        AXIS[\"geodetic longitude (Lon)\",east,\n"
               "            ORDER[2],\n"
               "            ANGLEUNIT[\"degree\",0.0174532925199433]]]";
    auto from_wkt =
        proj_create_from_wkt(m_ctxt, wkt, nullptr, nullptr, nullptr);
    ObjectKeeper keeper_from_wkt(from_wkt);
    EXPECT_NE(from_wkt, nullptr);

    EXPECT_TRUE(proj_is_equivalent_to_with_ctx(m_ctxt, from_epsg, from_wkt,
                                               PJ_COMP_EQUIVALENT));
}

// ---------------------------------------------------------------------------

TEST_F(CApi, datum_ensemble) {
    auto wkt =
        "GEOGCRS[\"ETRS89\","
        "    ENSEMBLE[\"European Terrestrial Reference System 1989 ensemble\","
        "        MEMBER[\"European Terrestrial Reference Frame 1989\"],"
        "        MEMBER[\"European Terrestrial Reference Frame 1990\"],"
        "        MEMBER[\"European Terrestrial Reference Frame 1991\"],"
        "        MEMBER[\"European Terrestrial Reference Frame 1992\"],"
        "        MEMBER[\"European Terrestrial Reference Frame 1993\"],"
        "        MEMBER[\"European Terrestrial Reference Frame 1994\"],"
        "        MEMBER[\"European Terrestrial Reference Frame 1996\"],"
        "        MEMBER[\"European Terrestrial Reference Frame 1997\"],"
        "        MEMBER[\"European Terrestrial Reference Frame 2000\"],"
        "        MEMBER[\"European Terrestrial Reference Frame 2005\"],"
        "        MEMBER[\"European Terrestrial Reference Frame 2014\"],"
        "        ELLIPSOID[\"GRS 1980\",6378137,298.257222101,"
        "            LENGTHUNIT[\"metre\",1]],"
        "        ENSEMBLEACCURACY[0.1]],"
        "    PRIMEM[\"Greenwich\",0,"
        "        ANGLEUNIT[\"degree\",0.0174532925199433]],"
        "    CS[ellipsoidal,2],"
        "        AXIS[\"geodetic latitude (Lat)\",north,"
        "            ORDER[1],"
        "            ANGLEUNIT[\"degree\",0.0174532925199433]],"
        "        AXIS[\"geodetic longitude (Lon)\",east,"
        "            ORDER[2],"
        "            ANGLEUNIT[\"degree\",0.0174532925199433]]]";
    auto from_wkt =
        proj_create_from_wkt(m_ctxt, wkt, nullptr, nullptr, nullptr);
    ObjectKeeper keeper_from_wkt(from_wkt);
    EXPECT_NE(from_wkt, nullptr);

    auto datum = proj_crs_get_datum(m_ctxt, from_wkt);
    ObjectKeeper keeper_datum(datum);
    ASSERT_EQ(datum, nullptr);

    auto datum_ensemble = proj_crs_get_datum_ensemble(m_ctxt, from_wkt);
    ObjectKeeper keeper_datum_ensemble(datum_ensemble);
    ASSERT_NE(datum_ensemble, nullptr);

    ASSERT_EQ(proj_datum_ensemble_get_member_count(m_ctxt, datum_ensemble), 11);
    ASSERT_EQ(proj_datum_ensemble_get_member(m_ctxt, datum_ensemble, -1),
              nullptr);
    ASSERT_EQ(proj_datum_ensemble_get_member(m_ctxt, datum_ensemble, 11),
              nullptr);

    {
        auto member = proj_datum_ensemble_get_member(m_ctxt, datum_ensemble, 0);
        ObjectKeeper keeper_member(member);
        ASSERT_NE(member, nullptr);

        EXPECT_EQ(proj_get_name(member),
                  std::string("European Terrestrial Reference Frame 1989"));
    }

    {
        auto member =
            proj_datum_ensemble_get_member(m_ctxt, datum_ensemble, 10);
        ObjectKeeper keeper_member(member);
        ASSERT_NE(member, nullptr);

        EXPECT_EQ(proj_get_name(member),
                  std::string("European Terrestrial Reference Frame 2014"));
    }

    ASSERT_EQ(proj_datum_ensemble_get_accuracy(m_ctxt, datum_ensemble), 0.1);

    auto datum_forced = proj_crs_get_datum_forced(m_ctxt, from_wkt);
    ObjectKeeper keeper_datum_forced(datum_forced);
    ASSERT_NE(datum_forced, nullptr);

    EXPECT_EQ(proj_get_name(datum_forced),
              std::string("European Terrestrial Reference System 1989"));

    auto cs = proj_crs_get_coordinate_system(m_ctxt, from_wkt);
    ObjectKeeper keeper_cs(cs);
    EXPECT_NE(cs, nullptr);

    {
        auto built_crs = proj_create_geographic_crs_from_datum(
            m_ctxt, proj_get_name(from_wkt), datum_ensemble, cs);
        ObjectKeeper keeper_built_crs(built_crs);
        EXPECT_NE(built_crs, nullptr);

        EXPECT_TRUE(proj_is_equivalent_to_with_ctx(m_ctxt, built_crs, from_wkt,
                                                   PJ_COMP_EQUIVALENT));
    }

    {
        auto built_crs = proj_create_geocentric_crs_from_datum(
            m_ctxt, proj_get_name(from_wkt), datum_ensemble, "metre", 1.0);
        ObjectKeeper keeper_built_crs(built_crs);
        EXPECT_NE(built_crs, nullptr);
    }
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_crs_is_derived) {
    {
        auto wkt =
            createProjectedCRS()->exportToWKT(WKTFormatter::create().get());
        auto obj = proj_create_from_wkt(m_ctxt, wkt.c_str(), nullptr, nullptr,
                                        nullptr);
        ObjectKeeper keeper(obj);
        ASSERT_NE(obj, nullptr) << wkt;

        EXPECT_TRUE(proj_crs_is_derived(m_ctxt, obj));
    }

    {
        auto wkt = createProjectedCRS()->baseCRS()->exportToWKT(
            WKTFormatter::create().get());
        auto obj = proj_create_from_wkt(m_ctxt, wkt.c_str(), nullptr, nullptr,
                                        nullptr);
        ObjectKeeper keeper(obj);
        ASSERT_NE(obj, nullptr) << wkt;

        EXPECT_FALSE(proj_crs_is_derived(m_ctxt, obj));
    }
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_get_insert_statements) {
    {
        auto session = proj_insert_object_session_create(nullptr);
        EXPECT_NE(session, nullptr);

        EXPECT_EQ(proj_insert_object_session_create(nullptr), nullptr);

        proj_insert_object_session_destroy(nullptr, session);
    }

    { proj_insert_object_session_destroy(nullptr, nullptr); }
    {
        auto wkt = "GEOGCRS[\"myGDA2020\",\n"
                   "    DATUM[\"GDA2020\",\n"
                   "        ELLIPSOID[\"GRS_1980\",6378137,298.257222101,\n"
                   "            LENGTHUNIT[\"metre\",1]]],\n"
                   "    PRIMEM[\"Greenwich\",0,\n"
                   "        ANGLEUNIT[\"Degree\",0.0174532925199433]],\n"
                   "    CS[ellipsoidal,2],\n"
                   "        AXIS[\"geodetic latitude (Lat)\",north,\n"
                   "            ORDER[1],\n"
                   "            ANGLEUNIT[\"degree\",0.0174532925199433]],\n"
                   "        AXIS[\"geodetic longitude (Lon)\",east,\n"
                   "            ORDER[2],\n"
                   "            ANGLEUNIT[\"degree\",0.0174532925199433]]]";
        auto crs = proj_create_from_wkt(m_ctxt, wkt, nullptr, nullptr, nullptr);
        ObjectKeeper keeper_from_wkt(crs);
        EXPECT_NE(crs, nullptr);

        {
            char *code =
                proj_suggests_code_for(m_ctxt, crs, "HOBU", false, nullptr);
            ASSERT_NE(code, nullptr);
            EXPECT_EQ(std::string(code), "MYGDA2020");
            proj_string_destroy(code);
        }

        {
            char *code =
                proj_suggests_code_for(m_ctxt, crs, "HOBU", true, nullptr);
            ASSERT_NE(code, nullptr);
            EXPECT_EQ(std::string(code), "1");
            proj_string_destroy(code);
        }

        const auto sizeOfStringList = [](const char *const *list) {
            if (list == nullptr)
                return -1;
            int size = 0;
            for (auto iter = list; *iter; ++iter) {
                size += 1;
            }
            return size;
        };

        // No session specified: we use a temporary session
        for (int i = 0; i < 2; i++) {
            auto list = proj_get_insert_statements(
                m_ctxt, nullptr, crs, "HOBU", "XXXX", false, nullptr, nullptr);
            ASSERT_NE(list, nullptr);
            ASSERT_NE(list[0], nullptr);
            EXPECT_EQ(std::string(list[0]),
                      "INSERT INTO geodetic_datum VALUES('HOBU',"
                      "'GEODETIC_DATUM_XXXX','GDA2020','','EPSG','7019',"
                      "'EPSG','8901',NULL,NULL,NULL,NULL,NULL,0);");
            EXPECT_EQ(sizeOfStringList(list), 4);
            proj_string_list_destroy(list);
        }

        // Pass an empty list of allowed authorities
        // We cannot reuse the EPSG ellipsoid and prime meridian
        {
            const char *const allowed_authorities[] = {nullptr};
            auto list =
                proj_get_insert_statements(m_ctxt, nullptr, crs, "HOBU", "XXXX",
                                           false, allowed_authorities, nullptr);
            EXPECT_EQ(sizeOfStringList(list), 6);
            proj_string_list_destroy(list);
        }

        // Allow only PROJ
        // We cannot reuse the EPSG ellipsoid and prime meridian
        {
            const char *const allowed_authorities[] = {"PROJ", nullptr};
            auto list =
                proj_get_insert_statements(m_ctxt, nullptr, crs, "HOBU", "XXXX",
                                           false, allowed_authorities, nullptr);
            EXPECT_EQ(sizeOfStringList(list), 6);
            proj_string_list_destroy(list);
        }

        // Allow EPSG
        {
            const char *const allowed_authorities[] = {"EPSG", nullptr};
            auto list =
                proj_get_insert_statements(m_ctxt, nullptr, crs, "HOBU", "XXXX",
                                           false, allowed_authorities, nullptr);
            EXPECT_EQ(sizeOfStringList(list), 4);
            proj_string_list_destroy(list);
        }

        auto session = proj_insert_object_session_create(m_ctxt);
        EXPECT_NE(session, nullptr);

        {
            auto list = proj_get_insert_statements(
                m_ctxt, session, crs, "HOBU", "XXXX", false, nullptr, nullptr);
            ASSERT_NE(list, nullptr);
            ASSERT_NE(list[0], nullptr);
            EXPECT_EQ(std::string(list[0]),
                      "INSERT INTO geodetic_datum VALUES('HOBU',"
                      "'GEODETIC_DATUM_XXXX','GDA2020','','EPSG','7019',"
                      "'EPSG','8901',NULL,NULL,NULL,NULL,NULL,0);");
            proj_string_list_destroy(list);
        }

        // Object already inserted: return empty list
        {
            auto list = proj_get_insert_statements(
                m_ctxt, session, crs, "HOBU", "XXXX", false, nullptr, nullptr);
            ASSERT_NE(list, nullptr);
            ASSERT_EQ(list[0], nullptr);
            proj_string_list_destroy(list);
        }

        proj_insert_object_session_destroy(m_ctxt, session);
    }
}
// ---------------------------------------------------------------------------

TEST_F(CApi, proj_get_geoid_models_from_database) {
    auto findInList = [](PROJ_STRING_LIST list, const std::string &ref) {
        while (list && *list) {
            if (std::string(*list) == ref) {
                return true;
            }
            list++;
        }
        return false;
    };

    auto list =
        proj_get_geoid_models_from_database(m_ctxt, "EPSG", "5703", nullptr);
    ListFreer freer(list);
    EXPECT_TRUE(findInList(list, "GEOID12B"));
    EXPECT_TRUE(findInList(list, "GEOID18"));
    EXPECT_TRUE(findInList(list, "GGM10"));
    EXPECT_FALSE(findInList(list, "OSGM15"));
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_trans_bounds_densify_0) {
    auto P =
        proj_create_crs_to_crs(m_ctxt, "EPSG:4326",
                               "+proj=laea +lat_0=45 +lon_0=-100 +x_0=0 +y_0=0 "
                               "+a=6370997 +b=6370997 +units=m +no_defs",
                               nullptr);
    ObjectKeeper keeper_P(P);
    ASSERT_NE(P, nullptr);
    double out_left;
    double out_bottom;
    double out_right;
    double out_top;
    int success =
        proj_trans_bounds(m_ctxt, P, PJ_FWD, 40, -120, 64, -80, &out_left,
                          &out_bottom, &out_right, &out_top, 0);
    EXPECT_TRUE(success == 1);
    EXPECT_NEAR(out_left, -1684649.41338, 1);
    EXPECT_NEAR(out_bottom, -350356.81377, 1);
    EXPECT_NEAR(out_right, 1684649.41338, 1);
    EXPECT_NEAR(out_top, 2234551.18559, 1);
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_trans_bounds_densify_100) {
    auto P =
        proj_create_crs_to_crs(m_ctxt, "EPSG:4326",
                               "+proj=laea +lat_0=45 +lon_0=-100 +x_0=0 +y_0=0 "
                               "+a=6370997 +b=6370997 +units=m +no_defs",
                               nullptr);
    ObjectKeeper keeper_P(P);
    ASSERT_NE(P, nullptr);
    double out_left;
    double out_bottom;
    double out_right;
    double out_top;
    int success =
        proj_trans_bounds(m_ctxt, P, PJ_FWD, 40, -120, 64, -80, &out_left,
                          &out_bottom, &out_right, &out_top, 100);
    EXPECT_TRUE(success == 1);
    EXPECT_NEAR(out_left, -1684649.41338, 1);
    EXPECT_NEAR(out_bottom, -555777.79210, 1);
    EXPECT_NEAR(out_right, 1684649.41338, 1);
    EXPECT_NEAR(out_top, 2234551.18559, 1);
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_trans_bounds_normalized) {
    auto P =
        proj_create_crs_to_crs(m_ctxt, "EPSG:4326",
                               "+proj=laea +lat_0=45 +lon_0=-100 +x_0=0 +y_0=0 "
                               "+a=6370997 +b=6370997 +units=m +no_defs",
                               nullptr);
    ObjectKeeper keeper_P(P);
    ASSERT_NE(P, nullptr);
    auto normalized_p = proj_normalize_for_visualization(m_ctxt, P);
    ObjectKeeper normal_keeper_P(normalized_p);
    ASSERT_NE(normalized_p, nullptr);
    double out_left;
    double out_bottom;
    double out_right;
    double out_top;
    int success =
        proj_trans_bounds(m_ctxt, normalized_p, PJ_FWD, -120, 40, -80, 64,
                          &out_left, &out_bottom, &out_right, &out_top, 100);
    EXPECT_TRUE(success == 1);
    EXPECT_NEAR(out_left, -1684649.41338, 1);
    EXPECT_NEAR(out_bottom, -555777.79210, 1);
    EXPECT_NEAR(out_right, 1684649.41338, 1);
    EXPECT_NEAR(out_top, 2234551.18559, 1);
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_trans_bounds_antimeridian_xy) {
    auto P = proj_create_crs_to_crs(m_ctxt, "EPSG:4167", "EPSG:3851", nullptr);
    ObjectKeeper keeper_P(P);
    ASSERT_NE(P, nullptr);
    auto normalized_p = proj_normalize_for_visualization(m_ctxt, P);
    ObjectKeeper normal_keeper_P(normalized_p);
    ASSERT_NE(normalized_p, nullptr);
    double out_left;
    double out_bottom;
    double out_right;
    double out_top;
    int success = proj_trans_bounds(m_ctxt, normalized_p, PJ_FWD, 160.6, -55.95,
                                    -171.2, -25.88, &out_left, &out_bottom,
                                    &out_right, &out_top, 21);
    EXPECT_TRUE(success == 1);
    EXPECT_NEAR(out_left, 1722483.900174921, 1);
    EXPECT_NEAR(out_bottom, 5228058.6143420935, 1);
    EXPECT_NEAR(out_right, 4624385.494808555, 1);
    EXPECT_NEAR(out_top, 8692574.544944234, 1);
    double out_left_inv;
    double out_bottom_inv;
    double out_right_inv;
    double out_top_inv;
    int success_inv = proj_trans_bounds(
        m_ctxt, normalized_p, PJ_INV, 1722483.900174921, 5228058.6143420935,
        4624385.494808555, 8692574.544944234, &out_left_inv, &out_bottom_inv,
        &out_right_inv, &out_top_inv, 21);
    EXPECT_TRUE(success_inv == 1);
    EXPECT_NEAR(out_left_inv, 153.2799922, 1);
    EXPECT_NEAR(out_bottom_inv, -56.7471249, 1);
    EXPECT_NEAR(out_right_inv, -162.1813873, 1);
    EXPECT_NEAR(out_top_inv, -24.6148194, 1);
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_trans_bounds_antimeridian) {
    auto P = proj_create_crs_to_crs(m_ctxt, "EPSG:4167", "EPSG:3851", nullptr);
    ObjectKeeper keeper_P(P);
    ASSERT_NE(P, nullptr);
    double out_left;
    double out_bottom;
    double out_right;
    double out_top;
    int success =
        proj_trans_bounds(m_ctxt, P, PJ_FWD, -55.95, 160.6, -25.88, -171.2,
                          &out_left, &out_bottom, &out_right, &out_top, 21);
    EXPECT_TRUE(success == 1);
    EXPECT_NEAR(out_left, 5228058.6143420935, 1);
    EXPECT_NEAR(out_bottom, 1722483.900174921, 1);
    EXPECT_NEAR(out_right, 8692574.544944234, 1);
    EXPECT_NEAR(out_top, 4624385.494808555, 1);
    double out_left_inv;
    double out_bottom_inv;
    double out_right_inv;
    double out_top_inv;
    int success_inv = proj_trans_bounds(
        m_ctxt, P, PJ_INV, 5228058.6143420935, 1722483.900174921,
        8692574.544944234, 4624385.494808555, &out_left_inv, &out_bottom_inv,
        &out_right_inv, &out_top_inv, 21);
    EXPECT_TRUE(success_inv == 1);
    EXPECT_NEAR(out_left_inv, -56.7471249, 1);
    EXPECT_NEAR(out_bottom_inv, 153.2799922, 1);
    EXPECT_NEAR(out_right_inv, -24.6148194, 1);
    EXPECT_NEAR(out_top_inv, -162.1813873, 1);
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_trans_bounds_beyond_global_bounds) {
    auto P = proj_create_crs_to_crs(m_ctxt, "EPSG:6933", "EPSG:4326", nullptr);
    ObjectKeeper keeper_P(P);
    ASSERT_NE(P, nullptr);
    auto normalized_p = proj_normalize_for_visualization(m_ctxt, P);
    ObjectKeeper normal_keeper_P(normalized_p);
    ASSERT_NE(normalized_p, nullptr);
    double out_left;
    double out_bottom;
    double out_right;
    double out_top;
    int success =
        proj_trans_bounds(m_ctxt, normalized_p, PJ_FWD, -17367531.3203125,
                          -7314541.19921875, 17367531.3203125, 7314541.19921875,
                          &out_left, &out_bottom, &out_right, &out_top, 21);
    EXPECT_TRUE(success == 1);
    EXPECT_NEAR(out_left, -180, 1);
    EXPECT_NEAR(out_bottom, -85.0445994113099, 1);
    EXPECT_NEAR(out_right, 180, 1);
    EXPECT_NEAR(out_top, 85.0445994113099, 1);
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_trans_bounds_ignore_inf) {
    auto P =
        proj_create_crs_to_crs(m_ctxt, "OGC:CRS84", "ESRI:102036", nullptr);
    ObjectKeeper keeper_P(P);
    ASSERT_NE(P, nullptr);
    double out_left;
    double out_bottom;
    double out_right;
    double out_top;
    // Before the ellipsoidal version of the gnomonic projection was
    // implemented the WGS84 ellipsoid was treated as a sphere of radius
    // 6378137m and the equator was the "horizon" for the projection with the
    // south polar aspect.
    //
    // The boundary with ndiv = 21 then mapped into a line extending to ymin =
    // -89178007.2 which was the projection of lat = -90d/(ndiv+1), long = 180d.
    //
    // With the implementation of the ellipsoidal gnonomic projection, the
    // horizon is now at lat = +0.3035d.
    //
    // We move the north edge of the box to lat = -90+4.15*(ndiv+1) = +1.3d.
    // The northernmost point on the boundary which is within the horizon is
    // now lat = -90+4.15*ndiv = -2.85d, long = 180d for which y = -116576598.5.
    int success =
        proj_trans_bounds(m_ctxt, P, PJ_FWD, -180.0, -90.0, 180.0, 1.3,
                          &out_left, &out_bottom, &out_right, &out_top, 21);
    EXPECT_TRUE(success == 1);
    EXPECT_NEAR(out_left, 0, 1);
    EXPECT_NEAR(out_bottom, -116576598.5, 1);
    EXPECT_NEAR(out_right, 0, 1);
    EXPECT_NEAR(out_top, 0, 1);
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_trans_bounds_ignore_inf_geographic) {
    auto P = proj_create_crs_to_crs(
        m_ctxt,
        "PROJCS[\"Interrupted_Goode_Homolosine\","
        "GEOGCS[\"GCS_unnamed ellipse\",DATUM[\"D_unknown\","
        "SPHEROID[\"Unknown\",6378137,298.257223563]],"
        "PRIMEM[\"Greenwich\",0],UNIT[\"Degree\",0.0174532925199433]],"
        "PROJECTION[\"Interrupted_Goode_Homolosine\"],"
        "UNIT[\"metre\",1,AUTHORITY[\"EPSG\",\"9001\"]],"
        "AXIS[\"Easting\",EAST],AXIS[\"Northing\",NORTH]]",
        "OGC:CRS84", nullptr);
    ObjectKeeper keeper_P(P);
    ASSERT_NE(P, nullptr);
    double out_left;
    double out_bottom;
    double out_right;
    double out_top;
    int success = proj_trans_bounds(m_ctxt, P, PJ_FWD, -15028000.0, 7515000.0,
                                    -14975000.0, 7556000.0, &out_left,
                                    &out_bottom, &out_right, &out_top, 21);
    EXPECT_TRUE(success == 1);
    EXPECT_NEAR(out_left, -179.2133, 1);
    EXPECT_NEAR(out_bottom, 70.9345, 1);
    EXPECT_NEAR(out_right, -177.9054, 1);
    EXPECT_NEAR(out_top, 71.4364, 1);
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_trans_bounds_noop_geographic) {
    auto P = proj_create_crs_to_crs(m_ctxt, "EPSG:4284", "EPSG:4284", nullptr);
    ObjectKeeper keeper_P(P);
    ASSERT_NE(P, nullptr);
    double out_left;
    double out_bottom;
    double out_right;
    double out_top;
    int success =
        proj_trans_bounds(m_ctxt, P, PJ_FWD, 19.57, 35.14, -168.97, 81.91,
                          &out_left, &out_bottom, &out_right, &out_top, 21);
    EXPECT_TRUE(success == 1);
    EXPECT_NEAR(out_left, 19.57, 1);
    EXPECT_NEAR(out_bottom, 35.14, 1);
    EXPECT_NEAR(out_right, -168.97, 1);
    EXPECT_NEAR(out_top, 81.91, 1);
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_trans_bounds__north_pole_xy) {
    auto P = proj_create_crs_to_crs(m_ctxt, "EPSG:32661", "EPSG:4326", nullptr);
    ObjectKeeper keeper_P(P);
    ASSERT_NE(P, nullptr);
    auto normalized_p = proj_normalize_for_visualization(m_ctxt, P);
    ObjectKeeper normal_keeper_P(normalized_p);
    ASSERT_NE(normalized_p, nullptr);
    double out_left;
    double out_bottom;
    double out_right;
    double out_top;
    int success = proj_trans_bounds(
        m_ctxt, normalized_p, PJ_FWD, -1371213.7625429356, -1405880.71737131,
        5371213.762542935, 5405880.71737131, &out_left, &out_bottom, &out_right,
        &out_top, 21);
    EXPECT_TRUE(success == 1);
    EXPECT_NEAR(out_left, -180.0, 1);
    EXPECT_NEAR(out_bottom, 48.656, 1);
    EXPECT_NEAR(out_right, 180.0, 1);
    EXPECT_NEAR(out_top, 90.0, 1);
    double out_left_inv;
    double out_bottom_inv;
    double out_right_inv;
    double out_top_inv;
    int success_inv = proj_trans_bounds(
        m_ctxt, normalized_p, PJ_INV, -180.0, 60.0, 180.0, 90.0, &out_left_inv,
        &out_bottom_inv, &out_right_inv, &out_top_inv, 21);
    EXPECT_TRUE(success_inv == 1);
    EXPECT_NEAR(out_left_inv, -1371213.76, 1);
    EXPECT_NEAR(out_bottom_inv, -1405880.72, 1);
    EXPECT_NEAR(out_right_inv, 5371213.76, 1);
    EXPECT_NEAR(out_top_inv, 5405880.72, 1);
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_trans_bounds__north_pole) {
    auto P = proj_create_crs_to_crs(m_ctxt, "EPSG:32661", "EPSG:4326", nullptr);
    ObjectKeeper keeper_P(P);
    ASSERT_NE(P, nullptr);
    double out_left;
    double out_bottom;
    double out_right;
    double out_top;
    int success = proj_trans_bounds(m_ctxt, P, PJ_FWD, -1405880.71737131,
                                    -1371213.7625429356, 5405880.71737131,
                                    5371213.762542935, &out_left, &out_bottom,
                                    &out_right, &out_top, 21);
    EXPECT_TRUE(success == 1);
    EXPECT_NEAR(out_left, 48.656, 1);
    EXPECT_NEAR(out_bottom, -180.0, 1);
    EXPECT_NEAR(out_right, 90.0, 1);
    EXPECT_NEAR(out_top, 180.0, 1);
    double out_left_inv;
    double out_bottom_inv;
    double out_right_inv;
    double out_top_inv;
    int success_inv = proj_trans_bounds(m_ctxt, P, PJ_INV, 60.0, -180.0, 90.0,
                                        180.0, &out_left_inv, &out_bottom_inv,
                                        &out_right_inv, &out_top_inv, 21);
    EXPECT_TRUE(success_inv == 1);
    EXPECT_NEAR(out_left_inv, -1405880.72, 1);
    EXPECT_NEAR(out_bottom_inv, -1371213.76, 1);
    EXPECT_NEAR(out_right_inv, 5405880.72, 1);
    EXPECT_NEAR(out_top_inv, 5371213.76, 1);
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_trans_bounds__south_pole_xy) {
    auto P = proj_create_crs_to_crs(m_ctxt, "EPSG:32761", "EPSG:4326", nullptr);
    ObjectKeeper keeper_P(P);
    ASSERT_NE(P, nullptr);
    auto normalized_p = proj_normalize_for_visualization(m_ctxt, P);
    ObjectKeeper normal_keeper_P(normalized_p);
    ASSERT_NE(normalized_p, nullptr);
    double out_left;
    double out_bottom;
    double out_right;
    double out_top;
    int success = proj_trans_bounds(
        m_ctxt, normalized_p, PJ_FWD, -1371213.7625429356, -1405880.71737131,
        5371213.762542935, 5405880.71737131, &out_left, &out_bottom, &out_right,
        &out_top, 21);
    EXPECT_TRUE(success == 1);
    EXPECT_NEAR(out_left, -180.0, 1);
    EXPECT_NEAR(out_bottom, -90, 1);
    EXPECT_NEAR(out_right, 180.0, 1);
    EXPECT_NEAR(out_top, -48.656, 1);
    double out_left_inv;
    double out_bottom_inv;
    double out_right_inv;
    double out_top_inv;
    int success_inv = proj_trans_bounds(
        m_ctxt, normalized_p, PJ_INV, -180.0, -90.0, 180.0, -60.0,
        &out_left_inv, &out_bottom_inv, &out_right_inv, &out_top_inv, 21);
    EXPECT_TRUE(success_inv == 1);
    EXPECT_NEAR(out_left_inv, -1371213.76, 1);
    EXPECT_NEAR(out_bottom_inv, -1405880.72, 1);
    EXPECT_NEAR(out_right_inv, 5371213.76, 1);
    EXPECT_NEAR(out_top_inv, 5405880.72, 1);
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_trans_bounds__south_pole) {
    auto P = proj_create_crs_to_crs(m_ctxt, "EPSG:32761", "EPSG:4326", nullptr);
    ObjectKeeper keeper_P(P);
    ASSERT_NE(P, nullptr);
    double out_left;
    double out_bottom;
    double out_right;
    double out_top;
    int success = proj_trans_bounds(m_ctxt, P, PJ_FWD, -1405880.71737131,
                                    -1371213.7625429356, 5405880.71737131,
                                    5371213.762542935, &out_left, &out_bottom,
                                    &out_right, &out_top, 21);
    EXPECT_TRUE(success == 1);
    EXPECT_NEAR(out_left, -90.0, 1);
    EXPECT_NEAR(out_bottom, -180.0, 1);
    EXPECT_NEAR(out_right, -48.656, 1);
    EXPECT_NEAR(out_top, 180.0, 1);
    double out_left_inv;
    double out_bottom_inv;
    double out_right_inv;
    double out_top_inv;
    int success_inv = proj_trans_bounds(m_ctxt, P, PJ_INV, -90.0, -180.0, -60.0,
                                        180.0, &out_left_inv, &out_bottom_inv,
                                        &out_right_inv, &out_top_inv, 21);
    EXPECT_TRUE(success_inv == 1);
    EXPECT_NEAR(out_left_inv, -1405880.72, 1);
    EXPECT_NEAR(out_bottom_inv, -1371213.76, 1);
    EXPECT_NEAR(out_right_inv, 5405880.72, 1);
    EXPECT_NEAR(out_top_inv, 5371213.76, 1);
}

// ---------------------------------------------------------------------------

TEST_F(CApi, proj_crs_has_point_motion_operation) {
    auto ctxt = proj_create_operation_factory_context(m_ctxt, nullptr);
    ASSERT_NE(ctxt, nullptr);
    ContextKeeper keeper_ctxt(ctxt);

    {
        auto crs = proj_create_from_database(
            m_ctxt, "EPSG", "4267", PJ_CATEGORY_CRS, false, nullptr); // NAD27
        ASSERT_NE(crs, nullptr);
        ObjectKeeper keeper_crs(crs);
        EXPECT_FALSE(proj_crs_has_point_motion_operation(m_ctxt, crs));
    }

    {
        // NAD83(CSRS)v7
        auto crs = proj_create_from_database(m_ctxt, "EPSG", "8255",
                                             PJ_CATEGORY_CRS, false, nullptr);
        ASSERT_NE(crs, nullptr);
        ObjectKeeper keeper_crs(crs);
        EXPECT_TRUE(proj_crs_has_point_motion_operation(m_ctxt, crs));
    }
}

// ---------------------------------------------------------------------------

#if !defined(_WIN32)
TEST_F(CApi, open_plenty_of_contexts) {
    // Test that we only consume 1 file handle for the connection to the
    // database
    std::vector<FILE *> dummyFilePointers;
    std::vector<PJ_CONTEXT *> ctxts;
    // The number of file descriptors that can be opened simultaneously by a
    // process varies across platforms so we make use of getrlimit(2) to
    // retrieve it.
    struct rlimit open_max;
    getrlimit(RLIMIT_NOFILE, &open_max);
    // On some platforms fopen returned nullptrs before reaching limit - 50, we
    // can avoid this by capping the limit to 1024.
    if (open_max.rlim_cur > 1024) {
        open_max.rlim_cur = 1024;
        setrlimit(RLIMIT_NOFILE, &open_max);
    }
    for (rlim_t i = 0; i < open_max.rlim_cur - 50; i++) {
        FILE *f = fopen("/dev/null", "rb");
        ASSERT_TRUE(f != nullptr);
        dummyFilePointers.push_back(f);
    }
    for (int i = 0; i < 100; i++) {
        PJ_CONTEXT *ctxt = proj_context_create();
        ASSERT_TRUE(ctxt != nullptr);
        auto obj = proj_create(ctxt, "EPSG:4326");
        ObjectKeeper keeper(obj);
        EXPECT_NE(obj, nullptr);
        ctxts.push_back(ctxt);
    }
    for (PJ_CONTEXT *ctxt : ctxts) {
        proj_context_destroy(ctxt);
    }
    for (FILE *f : dummyFilePointers) {
        fclose(f);
    }
    proj_cleanup();
}
#endif // !defined(_WIN32)

// ---------------------------------------------------------------------------

#ifndef __MINGW32__
// We need std::thread support

TEST_F(CApi, concurrent_context) {
    // Test that concurrent access to the database is thread safe.
    std::vector<std::thread> threads;
    for (int i = 0; i < 4; i++) {
        threads.emplace_back(std::thread([] {
            for (int j = 0; j < 60; j++) {
                PJ_CONTEXT *ctxt = proj_context_create();
                {
                    auto obj = proj_create(ctxt, "EPSG:4326");
                    ObjectKeeper keeper(obj);
                    EXPECT_NE(obj, nullptr);
                }
                {
                    auto obj = proj_create(
                        ctxt, ("EPSG:" + std::to_string(32600 + j)).c_str());
                    ObjectKeeper keeper(obj);
                    EXPECT_NE(obj, nullptr);
                }
                proj_context_destroy(ctxt);
            }
        }));
    }
    for (auto &t : threads) {
        t.join();
    }
    proj_cleanup();
}

#endif // __MINGW32__

} // namespace
