cmake_minimum_required(VERSION 2.6)
include(FindPkgConfig)

set(extra_cmake_modules ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
#include(${extra_cmake_modules}/flex.cmake)
set(uish_SRCS
  main.c)

option(ENABLE_DEBUG_SYMBOLS
    "Enable debugging symbols"
    ON)

if (ENABLE_DEBUG_SYMBOLS)
    set(CMAKE_BUILD_TYPE DEBUG)
endif(ENABLE_DEBUG_SYMBOLS)

pkg_check_modules(libedit REQUIRED libedit)
include_directories(${libedit_INCLUDE_DIRS})
set(LIBS ${LIBS} ${libedit_LIBRARIES})

add_executable(uish ${uish_SRCS})
target_link_libraries(uish ${LIBS})

