/******************************************************************************
 *
 * Project:  PROJ
 * Purpose:  ISO19111:2018 implementation
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

#include "proj/metadata.hpp"
#include "proj/common.hpp"
#include "proj/internal.hpp"
#include "proj/io.hpp"
#include "proj/io_internal.hpp"
#include "proj/util.hpp"

#include <algorithm>
#include <memory>
#include <sstream> // std::ostringstream
#include <string>
#include <vector>

using namespace NS_PROJ::internal;
using namespace NS_PROJ::io;
using namespace NS_PROJ::util;

NS_PROJ_START
namespace metadata {

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
struct Citation::Private {
    optional<std::string> title{};
};
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
Citation::Citation() : d(internal::make_unique<Private>()) {}
//! @endcond

// ---------------------------------------------------------------------------

/** \brief Constructs a citation by its title. */
Citation::Citation(const std::string &titleIn)
    : d(internal::make_unique<Private>()) {
    d->title = titleIn;
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
Citation::Citation(const Citation &other)
    : d(internal::make_unique<Private>(*(other.d))) {}

// ---------------------------------------------------------------------------

Citation::~Citation() = default;

// ---------------------------------------------------------------------------

Citation &Citation::operator=(const Citation &other) {
    if (this != &other) {
        *d = *other.d;
    }
    return *this;
}
//! @endcond

// ---------------------------------------------------------------------------

/** \brief Returns the name by which the cited resource is known. */
const optional<std::string> &Citation::title() const { return d->title; }

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
struct GeographicExtent::Private {};
//! @endcond

// ---------------------------------------------------------------------------

GeographicExtent::GeographicExtent() : d(internal::make_unique<Private>()) {}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
GeographicExtent::~GeographicExtent() = default;
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
struct GeographicBoundingBox::Private {
    double west_{};
    double south_{};
    double east_{};
    double north_{};

    Private(double west, double south, double east, double north)
        : west_(west), south_(south), east_(east), north_(north) {}
};
//! @endcond

// ---------------------------------------------------------------------------

GeographicBoundingBox::GeographicBoundingBox(double west, double south,
                                             double east, double north)
    : GeographicExtent(),
      d(internal::make_unique<Private>(west, south, east, north)) {}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
GeographicBoundingBox::~GeographicBoundingBox() = default;
//! @endcond

// ---------------------------------------------------------------------------

/** \brief Returns the western-most coordinate of the limit of the dataset
 * extent.
 *
 * The unit is degrees.
 *
 * If eastBoundLongitude < westBoundLongitude(), then the bounding box crosses
 * the anti-meridian.
 */
double GeographicBoundingBox::westBoundLongitude() const { return d->west_; }

// ---------------------------------------------------------------------------

/** \brief Returns the southern-most coordinate of the limit of the dataset
 * extent.
 *
 * The unit is degrees.
 */
double GeographicBoundingBox::southBoundLatitude() const { return d->south_; }

// ---------------------------------------------------------------------------

/** \brief Returns the eastern-most coordinate of the limit of the dataset
 * extent.
 *
 * The unit is degrees.
 *
 * If eastBoundLongitude < westBoundLongitude(), then the bounding box crosses
 * the anti-meridian.
 */
double GeographicBoundingBox::eastBoundLongitude() const { return d->east_; }

// ---------------------------------------------------------------------------

/** \brief Returns the northern-most coordinate of the limit of the dataset
 * extent.
 *
 * The unit is degrees.
 */
double GeographicBoundingBox::northBoundLatitude() const { return d->north_; }

// ---------------------------------------------------------------------------

/** \brief Instanciate a GeographicBoundingBox.
 *
 * If east < west, then the bounding box crosses the anti-meridian.
 *
 * @param west Western-most coordinate of the limit of the dataset extent (in
 * degrees).
 * @param south Southern-most coordinate of the limit of the dataset extent (in
 * degrees).
 * @param east Eastern-most coordinate of the limit of the dataset extent (in
 * degrees).
 * @param north Northern-most coordinate of the limit of the dataset extent (in
 * degrees).
 * @return a new GeographicBoundingBox.
 */
GeographicBoundingBoxNNPtr GeographicBoundingBox::create(double west,
                                                         double south,
                                                         double east,
                                                         double north) {
    return GeographicBoundingBox::nn_make_shared<GeographicBoundingBox>(
        west, south, east, north);
}

// ---------------------------------------------------------------------------

bool GeographicBoundingBox::isEquivalentTo(const util::BaseObjectNNPtr &other,
                                           util::IComparable::Criterion) const {
    auto otherExtent =
        util::nn_dynamic_pointer_cast<GeographicBoundingBox>(other);
    if (!otherExtent)
        return false;
    return westBoundLongitude() == otherExtent->westBoundLongitude() &&
           southBoundLatitude() == otherExtent->southBoundLatitude() &&
           eastBoundLongitude() == otherExtent->eastBoundLongitude() &&
           northBoundLatitude() == otherExtent->northBoundLatitude();
}

// ---------------------------------------------------------------------------

bool GeographicBoundingBox::contains(const GeographicExtentNNPtr &other) const {
    auto otherExtent =
        util::nn_dynamic_pointer_cast<GeographicBoundingBox>(other);
    if (!otherExtent) {
        return false;
    }

    if (!(southBoundLatitude() <= otherExtent->southBoundLatitude() &&
          northBoundLatitude() >= otherExtent->northBoundLatitude())) {
        return false;
    }

    if (westBoundLongitude() == -180.0 && eastBoundLongitude() == 180.0) {
        return true;
    }

    if (otherExtent->westBoundLongitude() == -180.0 &&
        otherExtent->eastBoundLongitude() == 180.0) {
        return false;
    }

    // Normal bounding box ?
    if (westBoundLongitude() < eastBoundLongitude()) {
        if (otherExtent->westBoundLongitude() <
            otherExtent->eastBoundLongitude()) {
            return westBoundLongitude() <= otherExtent->westBoundLongitude() &&
                   eastBoundLongitude() >= otherExtent->eastBoundLongitude();
        } else {
            return false;
        }
        // No: crossing antimerian
    } else {
        if (otherExtent->westBoundLongitude() <
            otherExtent->eastBoundLongitude()) {
            return false;
        } else {
            return westBoundLongitude() <= otherExtent->westBoundLongitude() &&
                   eastBoundLongitude() >= otherExtent->eastBoundLongitude();
        }
    }
}

// ---------------------------------------------------------------------------

bool GeographicBoundingBox::intersects(
    const GeographicExtentNNPtr &other) const {
    auto otherExtent =
        util::nn_dynamic_pointer_cast<GeographicBoundingBox>(other);
    if (!otherExtent) {
        return false;
    }

    if (northBoundLatitude() < otherExtent->southBoundLatitude() ||
        southBoundLatitude() > otherExtent->northBoundLatitude()) {
        return false;
    }

    if (westBoundLongitude() == -180.0 && eastBoundLongitude() == 180.0 &&
        otherExtent->westBoundLongitude() > otherExtent->eastBoundLongitude()) {
        return true;
    }

    if (otherExtent->westBoundLongitude() == -180.0 &&
        otherExtent->eastBoundLongitude() == 180.0 &&
        westBoundLongitude() > eastBoundLongitude()) {
        return true;
    }

    // Normal bounding box ?
    if (westBoundLongitude() <= eastBoundLongitude()) {
        if (otherExtent->westBoundLongitude() <
            otherExtent->eastBoundLongitude()) {
            if (std::max(westBoundLongitude(),
                         otherExtent->westBoundLongitude()) <
                std::min(eastBoundLongitude(),
                         otherExtent->eastBoundLongitude())) {
                return true;
            }
            return false;
        }

        return intersects(GeographicBoundingBox::create(
                   otherExtent->westBoundLongitude(),
                   otherExtent->southBoundLatitude(), 180.0,
                   otherExtent->northBoundLatitude())) ||
               intersects(GeographicBoundingBox::create(
                   -180.0, otherExtent->southBoundLatitude(),
                   otherExtent->eastBoundLongitude(),
                   otherExtent->northBoundLatitude()));

        // No: crossing antimerian
    } else {
        if (otherExtent->westBoundLongitude() <=
            otherExtent->eastBoundLongitude()) {
            return otherExtent->intersects(GeographicBoundingBox::create(
                westBoundLongitude(), southBoundLatitude(),
                eastBoundLongitude(), northBoundLatitude()));
        }

        return true;
    }
}

// ---------------------------------------------------------------------------

GeographicExtentPtr
GeographicBoundingBox::intersection(const GeographicExtentNNPtr &other) const {
    auto otherExtent =
        util::nn_dynamic_pointer_cast<GeographicBoundingBox>(other);
    if (!otherExtent) {
        return nullptr;
    }

    if (northBoundLatitude() < otherExtent->southBoundLatitude() ||
        southBoundLatitude() > otherExtent->northBoundLatitude()) {
        return nullptr;
    }

    if (westBoundLongitude() == -180.0 && eastBoundLongitude() == 180.0 &&
        otherExtent->westBoundLongitude() > otherExtent->eastBoundLongitude()) {
        auto res = GeographicBoundingBox::create(
            otherExtent->westBoundLongitude(),
            std::max(southBoundLatitude(), otherExtent->southBoundLatitude()),
            otherExtent->eastBoundLongitude(),
            std::min(northBoundLatitude(), otherExtent->northBoundLatitude()));
        return util::nn_static_pointer_cast<GeographicExtent>(res);
    }

    if (otherExtent->westBoundLongitude() == -180.0 &&
        otherExtent->eastBoundLongitude() == 180.0 &&
        westBoundLongitude() > eastBoundLongitude()) {
        auto res = GeographicBoundingBox::create(
            westBoundLongitude(),
            std::max(southBoundLatitude(), otherExtent->southBoundLatitude()),
            eastBoundLongitude(),
            std::min(northBoundLatitude(), otherExtent->northBoundLatitude()));
        return util::nn_static_pointer_cast<GeographicExtent>(res);
    }

    // Normal bounding box ?
    if (westBoundLongitude() <= eastBoundLongitude()) {
        if (otherExtent->westBoundLongitude() <
            otherExtent->eastBoundLongitude()) {
            auto res = GeographicBoundingBox::create(
                std::max(westBoundLongitude(),
                         otherExtent->westBoundLongitude()),
                std::max(southBoundLatitude(),
                         otherExtent->southBoundLatitude()),
                std::min(eastBoundLongitude(),
                         otherExtent->eastBoundLongitude()),
                std::min(northBoundLatitude(),
                         otherExtent->northBoundLatitude()));
            if (res->westBoundLongitude() < res->eastBoundLongitude()) {
                return util::nn_static_pointer_cast<GeographicExtent>(res);
            }
            return nullptr;
        }

        // Return larger of two parts of the multipolygon
        auto raw_inter1 = intersection(GeographicBoundingBox::create(
            otherExtent->westBoundLongitude(),
            otherExtent->southBoundLatitude(), 180.0,
            otherExtent->northBoundLatitude()));
        auto inter1 =
            std::dynamic_pointer_cast<GeographicBoundingBox>(raw_inter1);
        auto raw_inter2 = intersection(GeographicBoundingBox::create(
            -180.0, otherExtent->southBoundLatitude(),
            otherExtent->eastBoundLongitude(),
            otherExtent->northBoundLatitude()));
        auto inter2 =
            std::dynamic_pointer_cast<GeographicBoundingBox>(raw_inter2);
        if (!inter1) {
            return inter2;
        }
        if (!inter2) {
            return inter1;
        }
        if (inter1->eastBoundLongitude() - inter1->westBoundLongitude() >
            inter2->eastBoundLongitude() - inter2->westBoundLongitude()) {
            return inter1;
        }
        return inter2;
        // No: crossing antimerian
    } else {
        if (otherExtent->westBoundLongitude() <=
            otherExtent->eastBoundLongitude()) {
            return otherExtent->intersection(GeographicBoundingBox::create(
                westBoundLongitude(), southBoundLatitude(),
                eastBoundLongitude(), northBoundLatitude()));
        }

        auto res = GeographicBoundingBox::create(
            std::max(westBoundLongitude(), otherExtent->westBoundLongitude()),
            std::max(southBoundLatitude(), otherExtent->southBoundLatitude()),
            std::min(eastBoundLongitude(), otherExtent->eastBoundLongitude()),
            std::min(northBoundLatitude(), otherExtent->northBoundLatitude()));
        return util::nn_static_pointer_cast<GeographicExtent>(res);
    }
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
struct VerticalExtent::Private {
    double minimum_{};
    double maximum_{};
    common::UnitOfMeasureNNPtr unit_;

    Private(double minimum, double maximum,
            const common::UnitOfMeasureNNPtr &unit)
        : minimum_(minimum), maximum_(maximum), unit_(unit) {}
};
//! @endcond

// ---------------------------------------------------------------------------

VerticalExtent::VerticalExtent(double minimumIn, double maximumIn,
                               const common::UnitOfMeasureNNPtr &unitIn)
    : d(internal::make_unique<Private>(minimumIn, maximumIn, unitIn)) {}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
VerticalExtent::~VerticalExtent() = default;
//! @endcond

// ---------------------------------------------------------------------------

/** \brief Returns the minimum of the vertical extent.
 */
double VerticalExtent::minimumValue() const { return d->minimum_; }

// ---------------------------------------------------------------------------

/** \brief Returns the maximum of the vertical extent.
 */
double VerticalExtent::maximumValue() const { return d->maximum_; }

// ---------------------------------------------------------------------------

/** \brief Returns the unit of the vertical extent.
 */
common::UnitOfMeasureNNPtr VerticalExtent::unit() const { return d->unit_; }

// ---------------------------------------------------------------------------

/** \brief Instanciate a VerticalExtent.
 *
 * @param minimumIn minimum.
 * @param maximumIn maximum.
 * @param unitIn unit.
 * @return a new VerticalExtent.
 */
VerticalExtentNNPtr
VerticalExtent::create(double minimumIn, double maximumIn,
                       const common::UnitOfMeasureNNPtr &unitIn) {
    return VerticalExtent::nn_make_shared<VerticalExtent>(minimumIn, maximumIn,
                                                          unitIn);
}

// ---------------------------------------------------------------------------

bool VerticalExtent::isEquivalentTo(const util::BaseObjectNNPtr &other,
                                    util::IComparable::Criterion) const {
    auto otherExtent = util::nn_dynamic_pointer_cast<VerticalExtent>(other);
    if (!otherExtent)
        return false;
    return minimumValue() == otherExtent->minimumValue() &&
           maximumValue() == otherExtent->maximumValue() &&
           unit() == otherExtent->unit();
}

// ---------------------------------------------------------------------------

/** \brief Returns whether this extent contains the other one.
 */
bool VerticalExtent::contains(const VerticalExtentNNPtr &other) const {
    return common::Measure(minimumValue(), *unit()).getSIValue() <=
               common::Measure(other->minimumValue(), *other->unit())
                   .getSIValue() &&
           common::Measure(maximumValue(), *unit()).getSIValue() >=
               common::Measure(other->maximumValue(), *other->unit())
                   .getSIValue();
}

// ---------------------------------------------------------------------------

/** \brief Returns whether this extent intersects the other one.
 */
bool VerticalExtent::intersects(const VerticalExtentNNPtr &other) const {
    return common::Measure(minimumValue(), *unit()).getSIValue() <=
               common::Measure(other->maximumValue(), *other->unit())
                   .getSIValue() &&
           common::Measure(maximumValue(), *unit()).getSIValue() >=
               common::Measure(other->minimumValue(), *other->unit())
                   .getSIValue();
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
struct TemporalExtent::Private {
    std::string start_{};
    std::string stop_{};

    Private(const std::string &start, const std::string &stop)
        : start_(start), stop_(stop) {}
};
//! @endcond

// ---------------------------------------------------------------------------

TemporalExtent::TemporalExtent(const std::string &startIn,
                               const std::string &stopIn)
    : d(internal::make_unique<Private>(startIn, stopIn)) {}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
TemporalExtent::~TemporalExtent() = default;
//! @endcond

// ---------------------------------------------------------------------------

/** \brief Returns the start of the temporal extent.
 */
const std::string &TemporalExtent::start() const { return d->start_; }

// ---------------------------------------------------------------------------

/** \brief Returns the end of the temporal extent.
 */
const std::string &TemporalExtent::stop() const { return d->stop_; }

// ---------------------------------------------------------------------------

/** \brief Instanciate a TemporalExtent.
 *
 * @param start start.
 * @param stop stop.
 * @return a new TemporalExtent.
 */
TemporalExtentNNPtr TemporalExtent::create(const std::string &start,
                                           const std::string &stop) {
    return TemporalExtent::nn_make_shared<TemporalExtent>(start, stop);
}

// ---------------------------------------------------------------------------

bool TemporalExtent::isEquivalentTo(const util::BaseObjectNNPtr &other,
                                    util::IComparable::Criterion) const {
    auto otherExtent = util::nn_dynamic_pointer_cast<TemporalExtent>(other);
    if (!otherExtent)
        return false;
    return start() == otherExtent->start() && stop() == otherExtent->stop();
}

// ---------------------------------------------------------------------------

/** \brief Returns whether this extent contains the other one.
 */
bool TemporalExtent::contains(const TemporalExtentNNPtr &other) const {
    return start() <= other->start() && stop() >= other->stop();
}

// ---------------------------------------------------------------------------

/** \brief Returns whether this extent intersects the other one.
 */
bool TemporalExtent::intersects(const TemporalExtentNNPtr &other) const {
    return start() <= other->stop() && stop() >= other->start();
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
struct Extent::Private {
    optional<std::string> description_{};
    std::vector<GeographicExtentNNPtr> geographicElements_{};
    std::vector<VerticalExtentNNPtr> verticalElements_{};
    std::vector<TemporalExtentNNPtr> temporalElements_{};
};
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
Extent::Extent() : d(internal::make_unique<Private>()) {}

// ---------------------------------------------------------------------------

Extent::Extent(const Extent &other) : d(internal::make_unique<Private>()) {
    *d = *other.d;
}

// ---------------------------------------------------------------------------

Extent::~Extent() = default;
//! @endcond

// ---------------------------------------------------------------------------

/** Return a textual description of the extent.
 *
 * @return the description, or empty.
 */
const optional<std::string> &Extent::description() const {
    return d->description_;
}

// ---------------------------------------------------------------------------

/** Return the geographic element(s) of the extent
 *
 * @return the geographic element(s), or empty.
 */
const std::vector<GeographicExtentNNPtr> &Extent::geographicElements() const {
    return d->geographicElements_;
}

// ---------------------------------------------------------------------------

/** Return the vertical element(s) of the extent
 *
 * @return the vertical element(s), or empty.
 */
const std::vector<VerticalExtentNNPtr> &Extent::verticalElements() const {
    return d->verticalElements_;
}

// ---------------------------------------------------------------------------

/** Return the temporal element(s) of the extent
 *
 * @return the temporal element(s), or empty.
 */
const std::vector<TemporalExtentNNPtr> &Extent::temporalElements() const {
    return d->temporalElements_;
}

// ---------------------------------------------------------------------------

/** \brief Instanciate a Extent.
 *
 * @param descriptionIn Textual description, or empty.
 * @param geographicElementsIn Geographic element(s), or empty.
 * @param verticalElementsIn Vertical element(s), or empty.
 * @param temporalElementsIn Temporal element(s), or empty.
 * @return a new Extent.
 */
ExtentNNPtr
Extent::create(const optional<std::string> &descriptionIn,
               const std::vector<GeographicExtentNNPtr> &geographicElementsIn,
               const std::vector<VerticalExtentNNPtr> &verticalElementsIn,
               const std::vector<TemporalExtentNNPtr> &temporalElementsIn) {
    auto extent = Extent::nn_make_shared<Extent>();
    extent->assignSelf(util::nn_static_pointer_cast<util::BaseObject>(extent));
    extent->d->description_ = descriptionIn;
    extent->d->geographicElements_ = geographicElementsIn;
    extent->d->verticalElements_ = verticalElementsIn;
    extent->d->temporalElements_ = temporalElementsIn;
    return extent;
}

// ---------------------------------------------------------------------------

/** \brief Instanciate a Extent from a bounding box
 *
 * @param west Western-most coordinate of the limit of the dataset extent (in
 * degrees).
 * @param south Southern-most coordinate of the limit of the dataset extent (in
 * degrees).
 * @param east Eastern-most coordinate of the limit of the dataset extent (in
 * degrees).
 * @param north Northern-most coordinate of the limit of the dataset extent (in
 * degrees).
 * @param descriptionIn Textual description, or empty.
 * @return a new Extent.
 */
ExtentNNPtr
Extent::createFromBBOX(double west, double south, double east, double north,
                       const util::optional<std::string> &descriptionIn) {
    return create(
        descriptionIn,
        std::vector<GeographicExtentNNPtr>{
            nn_static_pointer_cast<GeographicExtent>(
                GeographicBoundingBox::create(west, south, east, north))},
        std::vector<VerticalExtentNNPtr>(), std::vector<TemporalExtentNNPtr>());
}

// ---------------------------------------------------------------------------

bool Extent::isEquivalentTo(const util::BaseObjectNNPtr &other,
                            util::IComparable::Criterion criterion) const {
    auto otherExtent = util::nn_dynamic_pointer_cast<Extent>(other);
    bool ret =
        (otherExtent &&
         description().has_value() == otherExtent->description().has_value() &&
         *description() == *otherExtent->description() &&
         geographicElements().size() ==
             otherExtent->geographicElements().size() &&
         verticalElements().size() == otherExtent->verticalElements().size() &&
         temporalElements().size() == otherExtent->temporalElements().size());
    if (ret) {
        for (size_t i = 0; ret && i < geographicElements().size(); ++i) {
            ret = geographicElements()[i]->isEquivalentTo(
                otherExtent->geographicElements()[i], criterion);
        }
        for (size_t i = 0; ret && i < verticalElements().size(); ++i) {
            ret = verticalElements()[i]->isEquivalentTo(
                otherExtent->verticalElements()[i], criterion);
        }
        for (size_t i = 0; ret && i < temporalElements().size(); ++i) {
            ret = temporalElements()[i]->isEquivalentTo(
                otherExtent->temporalElements()[i], criterion);
        }
    }
    return ret;
}

// ---------------------------------------------------------------------------

/** \brief Returns whether this extent contains the other one.
 *
 * Behaviour only well specified if each sub-extent category as at most
 * one element.
 */
bool Extent::contains(const ExtentNNPtr &other) const {
    bool res = true;
    if (geographicElements().size() == 1 &&
        other->geographicElements().size() == 1) {
        res = geographicElements()[0]->contains(other->geographicElements()[0]);
    }
    if (res && verticalElements().size() == 1 &&
        other->verticalElements().size() == 1) {
        res = verticalElements()[0]->contains(other->verticalElements()[0]);
    }
    if (res && temporalElements().size() == 1 &&
        other->temporalElements().size() == 1) {
        res = temporalElements()[0]->contains(other->temporalElements()[0]);
    }
    return res;
}

// ---------------------------------------------------------------------------

/** \brief Returns whether this extent intersects the other one.
 *
 * Behaviour only well specified if each sub-extent category as at most
 * one element.
 */
bool Extent::intersects(const ExtentNNPtr &other) const {
    bool res = true;
    if (geographicElements().size() == 1 &&
        other->geographicElements().size() == 1) {
        res =
            geographicElements()[0]->intersects(other->geographicElements()[0]);
    }
    if (res && verticalElements().size() == 1 &&
        other->verticalElements().size() == 1) {
        res = verticalElements()[0]->intersects(other->verticalElements()[0]);
    }
    if (res && temporalElements().size() == 1 &&
        other->temporalElements().size() == 1) {
        res = temporalElements()[0]->intersects(other->temporalElements()[0]);
    }
    return res;
}

// ---------------------------------------------------------------------------

/** \brief Returns the intersection of this extent with another one.
 *
 * Behaviour only well specified if there is one single GeographicExtent
 * in each object.
 * Returns nullptr otherwise.
 */
ExtentPtr Extent::intersection(const ExtentNNPtr &other) const {
    if (geographicElements().size() == 1 &&
        other->geographicElements().size() == 1) {
        if (contains(other)) {
            return other.as_nullable();
        }
        auto self = NN_CHECK_ASSERT(
            util::nn_dynamic_pointer_cast<Extent>(shared_from_this()));
        if (other->contains(self)) {
            return self.as_nullable();
        }
        auto geogIntersection = geographicElements()[0]->intersection(
            other->geographicElements()[0]);
        if (geogIntersection) {
            return create(util::optional<std::string>(),
                          std::vector<GeographicExtentNNPtr>{
                              NN_CHECK_ASSERT(geogIntersection)},
                          std::vector<VerticalExtentNNPtr>{},
                          std::vector<TemporalExtentNNPtr>{});
        }
    }
    return nullptr;
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
struct Identifier::Private {
    optional<Citation> authority_{};
    std::string code_{};
    optional<std::string> codeSpace_{};
    optional<std::string> version_{};
    optional<std::string> description_{};
    optional<std::string> uri_{};
};
//! @endcond

// ---------------------------------------------------------------------------

Identifier::Identifier(const std::string &codeIn)
    : d(internal::make_unique<Private>()) {
    d->code_ = codeIn;
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
Identifier::Identifier(const Identifier &other)
    : d(internal::make_unique<Private>(*(other.d))) {}

// ---------------------------------------------------------------------------

Identifier::~Identifier() = default;
//! @endcond

// ---------------------------------------------------------------------------

/** \brief Instanciate a Identifier.
 *
 * @param codeIn Alphanumeric value identifying an instance in the codespace
 * @param properties See \ref general_properties.
 * Generally, the Identifier::CODESPACE_KEY should be set.
 * @return a new Identifier.
 */
IdentifierNNPtr Identifier::create(const std::string &codeIn,
                                   const PropertyMap &properties) {
    auto id(Identifier::nn_make_shared<Identifier>(codeIn));
    id->setProperties(properties);
    return id;
}

// ---------------------------------------------------------------------------

Identifier &Identifier::operator=(const Identifier &other) {
    if (this != &other) {
        *d = *other.d;
    }
    return *this;
}

// ---------------------------------------------------------------------------

/** \brief Return a citation for the organization responsible for definition and
 * maintenance of the code.
 *
 * @return the citation for the authority, or empty.
 */
const optional<Citation> &Identifier::authority() const {
    return d->authority_;
}

// ---------------------------------------------------------------------------

/** \brief Return the alphanumeric value identifying an instance in the
 * codespace.
 *
 * e.g. "4326" (for EPSG:4326 WGS 84 GeographicCRS)
 *
 * @return the code.
 */
const std::string &Identifier::code() const { return d->code_; }

// ---------------------------------------------------------------------------

/** \brief Return the organization responsible for definition and maintenance of
 * the code.
 *
 * e.g "EPSG"
 *
 * @return the authority codespace, or empty.
 */
const optional<std::string> &Identifier::codeSpace() const {
    return d->codeSpace_;
}

// ---------------------------------------------------------------------------

/** \brief Return the version identifier for the namespace.
 *
 * When appropriate, the edition is identified by the effective date, coded
 * using ISO 8601 date format.
 *
 * @return the version or empty.
 */
const optional<std::string> &Identifier::version() const { return d->version_; }

// ---------------------------------------------------------------------------

/** \brief Return the natural language description of the meaning of the code
 * value.
 *
 * @return the description or empty.
 */
const optional<std::string> &Identifier::description() const {
    return d->description_;
}

// ---------------------------------------------------------------------------

/** \brief Return the URI of the identifier.
 *
 * @return the URI or empty.
 */
const optional<std::string> &Identifier::uri() const { return d->uri_; }

// ---------------------------------------------------------------------------

void Identifier::setProperties(
    const PropertyMap &properties) // throw(InvalidValueTypeException)
{
    {
        auto oIter = properties.find(AUTHORITY_KEY);
        if (oIter != properties.end()) {
            if (auto genVal =
                    util::nn_dynamic_pointer_cast<BoxedValue>(oIter->second)) {
                if (genVal->type() == BoxedValue::Type::STRING) {
                    d->authority_ = Citation(genVal->stringValue());
                } else {
                    throw InvalidValueTypeException("Invalid value type for " +
                                                    AUTHORITY_KEY);
                }
            } else {
                if (auto citation = util::nn_dynamic_pointer_cast<Citation>(
                        oIter->second)) {
                    d->authority_ = Citation(*citation);
                } else {
                    throw InvalidValueTypeException("Invalid value type for " +
                                                    AUTHORITY_KEY);
                }
            }
        }
    }

    {
        auto oIter = properties.find(CODE_KEY);
        if (oIter != properties.end()) {
            if (auto genVal =
                    util::nn_dynamic_pointer_cast<BoxedValue>(oIter->second)) {
                if (genVal->type() == BoxedValue::Type::INTEGER) {
                    std::ostringstream buffer;
                    buffer << genVal->integerValue();
                    d->code_ = buffer.str();
                } else if (genVal->type() == BoxedValue::Type::STRING) {
                    d->code_ = genVal->stringValue();
                } else {
                    throw InvalidValueTypeException("Invalid value type for " +
                                                    CODE_KEY);
                }
            } else {
                throw InvalidValueTypeException("Invalid value type for " +
                                                CODE_KEY);
            }
        }
    }

    {
        std::string temp;
        if (properties.getStringValue(CODESPACE_KEY, temp)) {
            d->codeSpace_ = temp;
        }
    }

    {
        std::string temp;
        if (properties.getStringValue(VERSION_KEY, temp)) {
            d->version_ = temp;
        }
    }

    {
        std::string temp;
        if (properties.getStringValue(DESCRIPTION_KEY, temp)) {
            d->description_ = temp;
        }
    }

    {
        std::string temp;
        if (properties.getStringValue(URI_KEY, temp)) {
            d->uri_ = temp;
        }
    }
}

// ---------------------------------------------------------------------------

std::string Identifier::exportToWKT(WKTFormatterNNPtr formatter) const {
    const bool isWKT2 = formatter->version() == WKTFormatter::Version::WKT2;
    const std::string &l_code = code();
    if (codeSpace() && !codeSpace()->empty() && !l_code.empty()) {
        if (isWKT2) {
            formatter->startNode(WKTConstants::ID, false);
            formatter->addQuotedString(*(codeSpace()));
            try {
                (void)std::stoi(l_code);
                formatter->add(l_code);
            } catch (const std::exception &) {
                formatter->addQuotedString(l_code);
            }
            if (version().has_value()) {
                auto l_version = *(version());
                try {
                    (void)c_locale_stod(l_version);
                    formatter->add(l_version);
                } catch (const std::exception &) {
                    formatter->addQuotedString(l_version);
                }
            }
            if (authority().has_value() && authority()->title().has_value() &&
                *(authority()->title()) != *(codeSpace())) {
                formatter->startNode(WKTConstants::CITATION, false);
                formatter->addQuotedString(*(authority()->title()));
                formatter->endNode();
            }
            if (uri().has_value()) {
                formatter->startNode(WKTConstants::URI, false);
                formatter->addQuotedString(*(uri()));
                formatter->endNode();
            }
            formatter->endNode();
        } else {
            formatter->startNode(WKTConstants::AUTHORITY, false);
            formatter->addQuotedString(*(codeSpace()));
            formatter->addQuotedString(l_code);
            formatter->endNode();
        }
    }
    return formatter->toString();
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
std::string Identifier::canonicalizeName(const std::string &str) {
    std::string res(tolower(str));
    for (auto c : std::vector<std::string>{" ", "_", "-", "/", "(", ")"}) {
        res = replaceAll(res, c, "");
    }
    return res;
}
//! @endcond

// ---------------------------------------------------------------------------

/** \brief Returns whether two names are considered equivalent.
 *
 * Two names are equivalent by removing any space, underscore, dash, slash,
 * { or } character from them, and comparing ina case insensitive way.
 */
bool Identifier::isEquivalentName(const std::string &a, const std::string &b) {
    return canonicalizeName(a) == canonicalizeName(b);
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
struct PositionalAccuracy::Private {
    std::string value_{};
};
//! @endcond

// ---------------------------------------------------------------------------

PositionalAccuracy::PositionalAccuracy(const std::string &valueIn)
    : d(internal::make_unique<Private>()) {
    d->value_ = valueIn;
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
PositionalAccuracy::~PositionalAccuracy() = default;
//! @endcond

// ---------------------------------------------------------------------------

/** \brief Return the value of the positional accuracy.
 */
const std::string &PositionalAccuracy::value() const { return d->value_; }

// ---------------------------------------------------------------------------

/** \brief Instanciate a PositionalAccuracy.
 *
 * @param valueIn positional accuracy value.
 * @return a new PositionalAccuracy.
 */
PositionalAccuracyNNPtr PositionalAccuracy::create(const std::string &valueIn) {
    return PositionalAccuracy::nn_make_shared<PositionalAccuracy>(valueIn);
}

} // namespace metadata
NS_PROJ_END
