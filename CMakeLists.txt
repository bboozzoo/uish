cmake_minimum_required(VERSION 2.6)
set(extra_cmake_modules ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(${extra_cmake_modules}/flex.cmake)
include(FindPkgConfig)

#include(${extra_cmake_modules}/flex.cmake)
set(uish_SRCS
  main.c
  debug.c
  uish.c)
add_flex_files(uish_SRCS scan.l)

option(ENABLE_DEBUG_SYMBOLS
    "Enable debugging symbols"
    ON)

if (ENABLE_DEBUG_SYMBOLS)
    set(CMAKE_BUILD_TYPE DEBUG)
endif(ENABLE_DEBUG_SYMBOLS)

pkg_check_modules(libedit REQUIRED libedit)
include_directories(${libedit_INCLUDE_DIRS})
set(LIBS ${LIBS} ${libedit_LIBRARIES})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(uish ${uish_SRCS})
target_link_libraries(uish ${LIBS})

