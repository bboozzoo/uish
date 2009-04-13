cmake_minimum_required(VERSION 2.6)
set(extra_cmake_modules ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(${extra_cmake_modules}/flex.cmake)
include(FindPkgConfig)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

#include(${extra_cmake_modules}/flex.cmake)
set(uish_SRCS
  main.c
  uish.c)
add_flex_files(uish_SRCS scan.l)

option(ENABLE_DEBUG_SYMBOLS
    "Enable debugging symbols"
    ON)

option(ENABLE_DEBUG
    "Enable debugging"
    ON)

if (ENABLE_DEBUG_SYMBOLS)
    set(CMAKE_BUILD_TYPE DEBUG)
endif(ENABLE_DEBUG_SYMBOLS)

if (ENABLE_DEBUG)
    set(uish_SRCS ${uish_SRCS} debug.c)
endif(ENABLE_DEBUG)

pkg_check_modules(libedit REQUIRED libedit)
include_directories(${libedit_INCLUDE_DIRS})
set(LIBS ${LIBS} ${libedit_LIBRARIES})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(uish ${uish_SRCS})
target_link_libraries(uish ${LIBS})

