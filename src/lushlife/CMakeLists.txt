#CMake file for infamous
cmake_minimum_required(VERSION 2.8)

set(PLUGIN "lushlife")

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/")

include(CheckCXXCompilerFlag)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g -Og")
check_cxx_compiler_flag("-msse2 -mfpmath=sse" SUPPORTS_SSE )
check_cxx_compiler_flag("-ffast-math" SUPPORTS_FAST_MATH )
if(SUPPORTS_SSE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse2 -mfpmath=sse")
endif(SUPPORTS_SSE)
if(SUPPORTS_FAST_MATH)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffast-math")
endif(SUPPORTS_FAST_MATH) 

if(NOT LIBDIR)
  set(LIBDIR "lib")
endif(NOT LIBDIR)

# check for our various libraries
find_package(PkgConfig)
pkg_check_modules(CAIRO cairo)
pkg_check_modules(NTK ntk)
pkg_check_modules(FFTW fftw3f)
pkg_check_modules(LV2 REQUIRED lv2>=.0.1.0)

include_directories (${LV2_INCLUDE_DIRS} ${FFTW_INCLUDE_DIRS} ${NTK_INCLUDE_DIRS} ${CAIRO_INCLUDE_DIRS} ../ffffltk ../draw . )
link_directories (${LV2_LIBRARY_DIRS} ${FFTW_LIBRARY_DIRS} ${NTK_LIBRARY_DIRS} ${CAIRO_LIBRARY_DIRS})

# config libraries

add_library(${PLUGIN} SHARED
  lushlife.cc
  lushlife.h
  retuner.cc
  retuner.h
  lfo.cc
  lfo.h
)

set_target_properties(${PLUGIN} PROPERTIES PREFIX "")
set_target_properties(${PLUGIN} PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE)
target_link_libraries(${PLUGIN} ${LV2_LIBRARIES} ${FFTW_LIBRARIES} m zita-resampler)

# config install
install(TARGETS ${PLUGIN}
  LIBRARY
  DESTINATION ${CMAKE_INSTALL_PREFIX}/${LIBDIR}/lv2/${PLUGIN}.lv2
)
install (FILES manifest.ttl ${PLUGIN}.ttl ${PLUGIN}_presets.ttl
  DESTINATION ${CMAKE_INSTALL_PREFIX}/${LIBDIR}/lv2/${PLUGIN}.lv2
)

if(CAIRO_FOUND AND NTK_FOUND)
    find_program(FLTK_FLUID_EXECUTABLE ntk-fluid)
    fltk_wrap_ui(${PLUGIN}_ui ${PLUGIN}_ui.fl)

    add_library(${PLUGIN}_ui SHARED
      lushlife.h
      lushlife_ui_main.cxx
      ${${PLUGIN}_ui_FLTK_UI_SRCS}
    )
    set_target_properties(${PLUGIN}_ui PROPERTIES PREFIX "")
    set_target_properties(${PLUGIN}_ui PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE) 
    target_link_libraries(${PLUGIN}_ui ${LV2_LIBRARIES} ${NTK_LIBRARIES} ${CAIRO_LIBRARIES} m)

    install(TARGETS ${PLUGIN}_ui
      LIBRARY
      DESTINATION ${CMAKE_INSTALL_PREFIX}/${LIBDIR}/lv2/${PLUGIN}.lv2
    )

else()
    message(WARNING "UI Libraries Missing, no GUI will be installed")
endif() 
