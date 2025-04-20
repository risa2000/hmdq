/******************************************************************************
 * HMDQ Tools - tools for VR headsets and other hardware introspection        *
 * https://github.com/risa2000/hmdq                                           *
 *                                                                            *
 * Copyright (c) 2025, Richard Musil. All rights reserved.                    *
 *                                                                            *
 * This source code is licensed under the BSD 3-Clause "New" or "Revised"     *
 * License found in the LICENSE file in the root directory of this project.   *
 * SPDX-License-Identifier: BSD-3-Clause                                      *
 ******************************************************************************/

/* For geometry operations */
#include <geos/geom/Geometry.h>
#include <geos/geom/GeometryFactory.h>
#include <geos/geom/Polygon.h>
#include <geos/geom/Triangle.h>

/* For WKT read/write */
#include <geos/io/WKTReader.h>
#include <geos/io/WKTWriter.h>

#include <catch2/catch_test_macros.hpp>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <iostream>
#include <memory>
#include <string>

//  global setup
//------------------------------------------------------------------------------
/* New factory with default (float) precision model */
geos::geom::GeometryFactory::Ptr factory = geos::geom::GeometryFactory::create();

/*
 * Reader requires a factory to bind the geometry to
 * for shared resources like the PrecisionModel
 */
geos::io::WKTReader reader(*factory);
/* Convert Geometry to WKT */
geos::io::WKTWriter writer;

/* Input WKT strings */
std::string wkt_a("POLYGON ((0 0, 10 0, 10 10, 0 10, 0 0))");
std::string wkt_b("POLYGON ((5 5, 15 5, 15 15, 5 15, 5 5))");

/* Convert WKT to Geometry */
std::unique_ptr<geos::geom::Geometry> geom_a(reader.read(wkt_a));
std::unique_ptr<geos::geom::Geometry> geom_b(reader.read(wkt_b));

//  tests
//------------------------------------------------------------------------------
TEST_CASE("GEOS testing", "[geos]")
{
    SECTION("intersection", "[polygons]")
    {
        /* Calculate intersection */
        std::unique_ptr<geos::geom::Geometry> inter = geom_a->intersection(geom_b.get());

        writer.setTrim(true); /* Only needed before GEOS 3.12 */
        std::string inter_wkt = writer.write(inter.get());

        /* Print out results */
        fmt::print("Geometry A:         {}\n", wkt_a);
        fmt::print("Geometry B:         {}\n", wkt_b);
        fmt::print("Intersection(A, B): {}\n", inter_wkt);
    }

    SECTION("surface", "[polygons]")
    {
        auto surf_a = geom_a->getArea();
        auto surf_b = geom_b->getArea();

        /* Calculate intersection */
        std::unique_ptr<geos::geom::Geometry> inter = geom_a->intersection(geom_b.get());
        auto surf_i = inter->getArea();

        /* Print out results */
        fmt::print("Surface A:      {}\n", surf_a);
        fmt::print("Surface B:      {}\n", surf_b);
        fmt::print("Surface (A, B): {}\n", surf_i);
    }

    SECTION("intersection", "[triangles]")
    {
        using namespace geos::geom;

        auto t1{factory->createPolygon(
            CoordinateSequence({CoordinateXY{0, 0}, CoordinateXY{1, 0},
                                CoordinateXY{0, 1}, CoordinateXY{0, 0}}))};
        auto canvas{factory->createPolygon(CoordinateSequence(
            {CoordinateXY{0, 0}, CoordinateXY{0, 1}, CoordinateXY{1, 1},
             CoordinateXY{1, 0}, CoordinateXY{0, 0}}))};
        auto isect = canvas->intersection(t1.get());
        std::string isect_wkt = writer.write(isect.get());

        /* Print out results */
        fmt::print("Triangle A:         {}\n", writer.write(t1.get()));
        fmt::print("Polygon B:          {}\n", writer.write(canvas.get()));
        fmt::print("Intersection(A, B): {}\n", writer.write(isect.get()));
    }
}
