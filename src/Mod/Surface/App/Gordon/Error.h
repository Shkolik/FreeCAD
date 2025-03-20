/*
* SPDX-License-Identifier: Apache-2.0
* SPDX-FileCopyrightText: 2018 German Aerospace Center (DLR)
*
* Created: 2018-08-06 Martin Siggel <Martin.Siggel@dlr.de>
*/

#ifndef ERROR_H
#define ERROR_H

#include <exception>
#include <string>

namespace occ_gordon_internal
{

//std::string getRealVectorStr(const TColStd_Array1OfReal& vec);
//std::string getIntVectorStr(const TColStd_Array1OfInteger& vec);
//std::string getVectorStr(const std::vector<double>& vec);
//void printCurve(const Handle(Geom_BSplineCurve) spline, const int idx, const std::string& prefix);
//void printSurface(const Handle(Geom_BSplineSurface) surf, const std::string& prefix);
//void printVector(const std::vector<double>& vec, const std::string& prefix);

enum error_code
{
    GENERIC_ERROR,
    MATH_ERROR,
    NULL_POINTER,
    INDEX_ERROR
};

class error : public std::exception
{
public:
    error(std::string error = "", error_code errorCode = GENERIC_ERROR) noexcept;
    ~error() noexcept override;

    const char* what() const noexcept override;

    // Returns the error code
    virtual error_code get_code() const noexcept;

private:
    std::string err;
    error_code code;
};

} // namespace occ_gordon_internal


#endif // ERROR_H
