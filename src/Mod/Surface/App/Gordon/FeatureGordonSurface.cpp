// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 Andrew Shkolik <shkolik@gmail.com>                  *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
#include <BRepAdaptor_Curve.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <Standard_Version.hxx>
#include <TopoDS.hxx>
#include <gp_Pnt.hxx>
#endif
#include "FeatureGordonSurface.h"
#include "occ_gordon.h"

using namespace Surface;

PROPERTY_SOURCE(Surface::GordonSurface, Part::Spline)

GordonSurface::GordonSurface()
{
    ADD_PROPERTY_TYPE(ProfileEdges, (nullptr, ""), "GordonSurface", App::Prop_None, "Profiles edges.");
    ADD_PROPERTY_TYPE(GuideEdges, (nullptr, ""), "GordonSurface", App::Prop_None, "Guide edges.");
    ADD_PROPERTY_TYPE(ProfileDirections, (false), "GordonSurface", App::Prop_None, "Profile directions.");
    ADD_PROPERTY_TYPE(GuideDirections, (false), "GordonSurface", App::Prop_None, "Guide directions.");
    ADD_PROPERTY_TYPE(Tolerance, (1.e-3), "GordonSurface", App::Prop_None, "Tolerance");
    
    ProfileEdges.setScope(App::LinkScope::Global);
    GuideEdges.setScope(App::LinkScope::Global);

    ProfileEdges.setSize(0);
    GuideEdges.setSize(0);
    ProfileDirections.setSize(0);
    GuideDirections.setSize(0);
    Tolerance.setValue(1.e-3);
}

short GordonSurface::mustExecute() const
{
    if (ProfileEdges.isTouched() || GuideEdges.isTouched() || ProfileDirections.isTouched()
        || GuideDirections.isTouched() || Tolerance.isTouched()) {
        return 1;
    }
    return 0;
}


std::vector<Handle(Geom_BSplineCurve)> getCurves(const App::PropertyLinkSubList& edges, const App::PropertyBoolList& directions)
{
    std::vector<Handle(Geom_BSplineCurve)> curves;

    auto objects = edges.getValues();
    auto subNames = edges.getSubValues();
    auto dirs = directions.getValues();

    if (objects.size() != dirs.size() || objects.size() != subNames.size() || dirs.size() != subNames.size()) {
        Standard_Failure::Raise(std::format("Inconsistent number of edges ({}), sub-shapes ({}), and directions ({}).",objects.size(),dirs.size(),subNames.size()).c_str());
    }

    for (std::size_t i = 0; i < objects.size(); i++) {
        App::DocumentObject* obj = objects[i];
        std::string sub = subNames[i];
        if (obj && obj->isDerivedFrom<Part::Feature>()) {
            // get the sub-edge of the part's shape and copy it to nat make changes to original geometry
            const Part::TopoShape& shape = static_cast<Part::Feature*>(obj)->Shape.getShape().makeElementCopy();
            TopoDS_Shape edgeShape = shape.getSubShape(sub.c_str());
            if (!edgeShape.IsNull() && edgeShape.ShapeType() == TopAbs_EDGE) {   
                Standard_Real u1, u2;
                const TopoDS_Edge& edge = TopoDS::Edge(edgeShape);
                TopLoc_Location heloc;  // this will be output curve location
                Handle(Geom_Curve) c_geom = BRep_Tool::Curve(edge, heloc, u1, u2);  // The geometric curve
                     
                ShapeConstruct_Curve scc;
                Handle(Geom_BSplineCurve)  bspline = scc.ConvertToBSpline(c_geom, u1, u2, Precision::Confusion());
                if (bspline.IsNull()) {
                    Standard_Failure::Raise(
                        "A curve was not a B-spline and could not be converted into one.");
                }                    
                if (dirs[i]) {
                    bspline->Reverse();
                }
                
                bspline->Transform(heloc.Transformation());  // apply original transformation to control points

                /*showBSpline(bspline, "BSpline");*/

                curves.emplace_back(bspline);
            }
            else {
                Standard_Failure::Raise("Sub-shape is not an edge");
            }
        }
    }

    return curves;
}
    
App::DocumentObjectExecReturn* GordonSurface::execute()
{
    try {
        if ((ProfileEdges.getSize()) < 2) {
            return new App::DocumentObjectExecReturn("Provide at least 2 profiles.");
        }
        if ((GuideEdges.getSize()) < 2) {
            return new App::DocumentObjectExecReturn("Provide at least 2 guides.");
        }

        std::vector<Handle(Geom_BSplineCurve)> vcurves, ucurves;
        
        // Create a Gordon surface
        ucurves = getCurves(ProfileEdges, ProfileDirections);
        vcurves = getCurves(GuideEdges, GuideDirections);

        // there is no reason to go under 1e-7 precision
        double tol = Tolerance.getValue() < Precision::Confusion() ? Precision::Confusion() : Tolerance.getValue();
        auto surface = occ_gordon::interpolate_curve_network(ucurves, vcurves, tol);

        if (surface.IsNull()) {
            return new App::DocumentObjectExecReturn("Failed to create a Gordon surface.");
        }

        // Create a face from the BSpline surface
        BRepBuilderAPI_MakeFace faceMaker(surface, Precision::Confusion());
        if (!faceMaker.IsDone()) {
            return new App::DocumentObjectExecReturn(
                "Failed to create a face from the BSpline surface.");
        }

        this->Shape.setValue(faceMaker.Face());
        return App::DocumentObject::StdReturn;
    }
    catch (const Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
}

void GordonSurface::onDocumentRestored()
{
    // init ProfileDirections and GuideDirections if not exists
    if (ProfileDirections.getSize() != ProfileEdges.getSize()) {
        ProfileDirections.setSize(ProfileEdges.getSize());
        for (std::size_t i = 0; i < ProfileDirections.getSize(); ++i) {
            ProfileDirections.set1Value(i, false);
        }
    }
    if (GuideDirections.getSize() != GuideEdges.getSize()) {
        GuideDirections.setSize(GuideEdges.getSize());
        for (std::size_t i = 0; i < GuideDirections.getSize(); ++i) {
            GuideDirections.set1Value(i, false);
        }
    }

    Part::Spline::onDocumentRestored();
}