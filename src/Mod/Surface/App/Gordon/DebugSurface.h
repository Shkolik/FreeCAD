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

#ifndef DEBUGSURFACE_H
#define DEBUGSURFACE_H
#include "PreCompiled.h"

#include <string>
#include <Base/Tools.h>
#include <Base/Console.h>

#include <App/Application.h>
#include <App/Document.h>
#include <Gui/Document.h>
#include <Mod/Part/App/PartFeature.h>

FC_LOG_LEVEL_INIT("Surface", true, true)

namespace
{

void warning(const std::string& message)
{
    FC_WARN(message);
}

void error(const std::string& message)
{
    FC_ERR(message);
}

void log(const std::string& message)
{
    FC_LOG(message);
}

std::string getRealVectorStr(const TColStd_Array1OfReal& vec)
{
    std::string str = "[";
    for (size_t i = vec.Lower(); i <= vec.Upper(); ++i) {
        str += fmt::format("'{:.2f}', ", vec.Value(i));
    }
    if (str.size() > 1) {
        str.erase(str.size() - 2);  // Remove the last ", "
    }
    str += "]";
    return str;
}

std::string getIntVectorStr(const TColStd_Array1OfInteger& vec)
{
    std::string str = "[";
    for (size_t i = vec.Lower(); i <= vec.Upper(); ++i) {
        str += "'   " + std::to_string(vec.Value(i)) + "', ";
    }
    if (str.size() > 1) {
        str.erase(str.size() - 2);  // Remove the last ", "
    }
    str += "]";
    return str;
}

std::string getVectorStr(const std::vector<double>& vec)
{
     std::string str = "[";
     for (size_t i = 0; i < vec.size(); ++i) {
         str += std::to_string(vec[i]) + " ";
     }
     str += "]";
     return str;
 }

void printVector(const std::vector<double>& vec, const std::string& prefix)
{
    std::string str = "\n" + getVectorStr(vec) + "\n";
    warning(prefix + str);
}

void print2DArray(const TColgp_Array2OfPnt& matrix, const std::string& prefix)
{
    std::string matrix_str = "\n";
    for (int spline_u_idx = matrix.LowerRow(); spline_u_idx <= matrix.UpperRow(); ++spline_u_idx) {
        std::string row = "";
        // guides
        for (int spline_v_idx = matrix.LowerCol(); spline_v_idx <= matrix.UpperCol();
             ++spline_v_idx) {
            auto point = matrix(spline_u_idx, spline_v_idx);
            row += "(" + std::to_string(point.X()) + ", " + std::to_string(point.Y()) + ", "
                + std::to_string(point.Z()) + ")";
        }
        matrix_str += row + "\n";
    }

    warning("\n" + prefix + matrix_str);
}

void printCurve(const Handle(Geom_BSplineCurve) spline, const int idx, const std::string& prefix)
{
    warning(prefix + std::to_string(idx) + "\n" + "Degree: " + std::to_string(spline->Degree())
            + " / NbPoles: " + std::to_string(spline->NbPoles()) + "\n"
            + "Closed: " + std::to_string(spline->IsClosed())
            + " / Periodic: " + std::to_string(spline->IsPeriodic()) + "\n"
            + "Knots: " + getRealVectorStr(spline->Knots()) + "\n"
            + "Mults: " + getIntVectorStr(spline->Multiplicities()) + "\n");
}

void printSurface(const Handle(Geom_BSplineSurface) surf, const std::string& prefix)
{
    auto uIso = Handle(Geom_BSplineCurve)::DownCast(surf->UIso(surf->UKnot(1)));
    auto vIso = Handle(Geom_BSplineCurve)::DownCast(surf->VIso(surf->VKnot(1)));

    printCurve(uIso, 0, "U-IsoCurve:\n");
    printCurve(vIso, 0, "V-IsoCurve:\n");
}


void printIntersectionMatrix(const math_Matrix& params_u,
                                const math_Matrix& params_v,
                                const int profiles,
                                const int guides,
                                const std::string& prefix)
{
    std::string matrix_u = "\n";
    std::string matrix_v = "\n";
    for (int spline_u_idx = 0; spline_u_idx < profiles; ++spline_u_idx) {
        std::string row_u = "";
        std::string row_v = "";
        // guides
        for (int spline_v_idx = 0; spline_v_idx < guides; ++spline_v_idx) {
            row_u += std::to_string(params_u(spline_u_idx, spline_v_idx)) + " ";
            row_v += std::to_string(params_v(spline_u_idx, spline_v_idx)) + " ";
        }
        matrix_u += row_u + "\n";
        matrix_v += row_v + "\n";
    }

    warning("\n" + prefix + "params_u" + matrix_u);
    warning("\n" + prefix + "params_v" + matrix_v);
}


void showBSplineSurface(Handle(Geom_BSplineSurface) surface, const char* name = "Surface")
{    
    // Create a face from the BSpline surface
    BRepBuilderAPI_MakeFace faceMaker(surface, Precision::Confusion());
    if (!faceMaker.IsDone()) {
        return;
    }

    App::Document* doc = App::GetApplication().getActiveDocument();
    std::string objName = doc->getUniqueObjectName(name);
    Part::Feature* object = static_cast<Part::Feature*>(doc->addObject("Part::Feature", objName.c_str()));
    
    object->Shape.setValue(faceMaker.Face());
}
}  // namespace


#endif // DEBUG_H
