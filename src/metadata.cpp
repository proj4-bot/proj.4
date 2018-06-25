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
#include "proj/internal.hpp"
#include "proj/io.hpp"
#include "proj/io_internal.hpp"
#include "proj/util.hpp"

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

Citation::Citation() : d(internal::make_unique<Private>()) {}

// ---------------------------------------------------------------------------

Citation::Citation(const std::string &titleIn)
    : d(internal::make_unique<Private>()) {
    d->title = titleIn;
}

// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------

const optional<std::string> &Citation::title() const { return d->title; }

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
struct GeographicExtent::Private {};
//! @endcond

// ---------------------------------------------------------------------------

GeographicExtent::GeographicExtent() : d(internal::make_unique<Private>()) {}

// ---------------------------------------------------------------------------

GeographicExtent::~GeographicExtent() = default;

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

GeographicBoundingBox::~GeographicBoundingBox() = default;

// ---------------------------------------------------------------------------

double GeographicBoundingBox::westBoundLongitude() const { return d->west_; }

// ---------------------------------------------------------------------------

double GeographicBoundingBox::southBoundLongitude() const { return d->south_; }

// ---------------------------------------------------------------------------

double GeographicBoundingBox::eastBoundLongitude() const { return d->east_; }

// ---------------------------------------------------------------------------

double GeographicBoundingBox::northBoundLongitude() const { return d->north_; }

// ---------------------------------------------------------------------------

GeographicBoundingBoxNNPtr GeographicBoundingBox::create(double west,
                                                         double south,
                                                         double east,
                                                         double north) {
    return GeographicBoundingBox::nn_make_shared<GeographicBoundingBox>(
        west, south, east, north);
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
struct Extent::Private {
    optional<std::string> description_{};
    std::vector<GeographicExtentNNPtr> geographicElements_{};
};
//! @endcond

// ---------------------------------------------------------------------------

Extent::Extent() : d(internal::make_unique<Private>()) {}

// ---------------------------------------------------------------------------

Extent::Extent(const Extent &other) : d(internal::make_unique<Private>()) {
    *d = *other.d;
}

// ---------------------------------------------------------------------------

Extent::~Extent() = default;

// ---------------------------------------------------------------------------

const optional<std::string> &Extent::description() const {
    return d->description_;
}

// ---------------------------------------------------------------------------

const std::vector<GeographicExtentNNPtr> &Extent::geographicElements() const {
    return d->geographicElements_;
}

// ---------------------------------------------------------------------------

ExtentNNPtr
Extent::create(const optional<std::string> &descriptionIn,
               const std::vector<GeographicExtentNNPtr> &geographicElementsIn) {
    auto extent = Extent::nn_make_shared<Extent>();
    extent->d->description_ = descriptionIn;
    extent->d->geographicElements_ = geographicElementsIn;
    return extent;
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

Identifier::Identifier(const Identifier &other)
    : d(internal::make_unique<Private>(*(other.d))) {}

// ---------------------------------------------------------------------------

Identifier::~Identifier() = default;

// ---------------------------------------------------------------------------

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

const optional<Citation> &Identifier::authority() const {
    return d->authority_;
}

// ---------------------------------------------------------------------------

const std::string &Identifier::code() const { return d->code_; }

// ---------------------------------------------------------------------------

const optional<std::string> &Identifier::codeSpace() const {
    return d->codeSpace_;
}

// ---------------------------------------------------------------------------

const optional<std::string> &Identifier::version() const { return d->version_; }

// ---------------------------------------------------------------------------

const optional<std::string> &Identifier::description() const {
    return d->description_;
}

// ---------------------------------------------------------------------------

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

static std::string canonicalizeName(const std::string &str) {
    std::string res(tolower(str));
    for (auto c : std::vector<std::string>{" ", "_", "-", "/", "(", ")"}) {
        res = replaceAll(res, c, "");
    }
    return res;
}

// ---------------------------------------------------------------------------

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

PositionalAccuracy::~PositionalAccuracy() = default;

// ---------------------------------------------------------------------------

const std::string &PositionalAccuracy::value() const { return d->value_; }

// ---------------------------------------------------------------------------

PositionalAccuracyNNPtr PositionalAccuracy::create(const std::string &valueIn) {
    return PositionalAccuracy::nn_make_shared<PositionalAccuracy>(valueIn);
}

} // namespace metadata
NS_PROJ_END
