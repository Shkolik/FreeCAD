include_directories(
  ${CMAKE_BINARY_DIR}
  ${CMAKE_BINARY_DIR}/src
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
)

include_directories(
  SYSTEM
  ${Boost_INCLUDE_DIRS}
  ${OCC_INCLUDE_DIR}
  ${PYCXX_INCLUDE_DIR}
  ${Python3_INCLUDE_DIRS}
  ${ZLIB_INCLUDE_DIR}
  ${XercesC_INCLUDE_DIRS}
  ${FREETYPE_INCLUDE_DIRS}
)

link_directories(${OCC_LIBRARY_DIR})

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Blending)

generate_from_xml(Blending/BlendPointPy)
generate_from_xml(Blending/BlendCurvePy)

set(Surface_LIBS
  FreeCADApp
  Part
)

# BlendPoint Wrapper
SET(BlendingPy_SRCS
  Blending/BlendPointPy.xml
  Blending/BlendPointPyImp.cpp
  Blending/BlendCurvePy.xml
  Blending/BlendCurvePyImp.cpp
)
SOURCE_GROUP("Blending" FILES ${BlendingPy_SRCS})

SET(Blending_SRCS
  Blending/FeatureBlendCurve.cpp
  Blending/FeatureBlendCurve.h
  Blending/BlendPoint.cpp
  Blending/BlendPoint.h
  Blending/BlendCurve.cpp
  Blending/BlendCurve.h
)

SET(Gordon_SRCS
  Gordon/FeatureGordonSurface.cpp
  Gordon/FeatureGordonSurface.h
)

SET(OCC_Gordon_SRCS
  Gordon/ApproxResult.h
  Gordon/BSplineAlgorithms.cpp
  Gordon/BSplineAlgorithms.h
  Gordon/BSplineApproxInterp.cpp
  Gordon/BSplineApproxInterp.h
  Gordon/CurveNetworkSorter.cpp
  Gordon/CurveNetworkSorter.h
  Gordon/CurvesToSurface.cpp
  Gordon/CurvesToSurface.h
  Gordon/GordonSurfaceBuilder.cpp
  Gordon/GordonSurfaceBuilder.h
  Gordon/InterpolateCurveNetwork.cpp
  Gordon/InterpolateCurveNetwork.h
  Gordon/IntersectBSplines.cpp
  Gordon/IntersectBSplines.h
  Gordon/IntersectionPoint.h
  Gordon/PointsToBSplineInterpolation.cpp
  Gordon/PointsToBSplineInterpolation.h
  Gordon/occ_gordon_internal.h
  Gordon/occ_std_adapters.h
  Gordon/Error.h
  Gordon/Error.cpp
  Gordon/DebugSurface.h
  Gordon/occ_gordon.h
  Gordon/occ_gordon.cpp
)


SET(Surface_SRCS
  ${Blending_SRCS}
  ${BlendingPy_SRCS}
  ${Gordon_SRCS}
  ${OCC_Gordon_SRCS}
  AppSurface.cpp
  PreCompiled.cpp
  PreCompiled.h
  FeatureExtend.cpp
  FeatureExtend.h
  FeatureGeomFillSurface.cpp
  FeatureGeomFillSurface.h
  FeatureFilling.cpp
  FeatureFilling.h
  FeatureSections.cpp
  FeatureSections.h
  FeatureSewing.cpp
  FeatureSewing.h
  FeatureCut.cpp
  FeatureCut.h
)

if(FREECAD_USE_PCH)
    add_definitions(-D_PreComp_)
    GET_MSVC_PRECOMPILED_SOURCE("PreCompiled.cpp" PCH_SRCS ${Surface_SRCS})
    ADD_MSVC_PRECOMPILED_HEADER(Surface PreCompiled.h PreCompiled.cpp PCH_SRCS)
endif(FREECAD_USE_PCH)

link_directories(${OCC_LIBRARY_DIR})
add_library(Surface SHARED ${Surface_SRCS})
target_link_libraries(Surface ${Surface_LIBS})
if (FREECAD_WARN_ERROR)
    target_compile_warn_error(Surface)
endif()

SET_BIN_DIR(Surface Surface /Mod/Surface)
SET_PYTHON_PREFIX_SUFFIX(Surface)

install(TARGETS Surface DESTINATION ${CMAKE_INSTALL_LIBDIR})
