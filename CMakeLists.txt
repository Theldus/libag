# MIT License
#
# Copyright (c) 2021 Davidson Francis <davidsondfgl@gmail.com>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

cmake_minimum_required(VERSION 3.5.2)
set(CMAKE_C_STANDARD 99)

project(ag C)
include(GNUInstallDirs)

# General
add_compile_options(-Wall -Wextra)
include_directories(${CMAKE_SOURCE_DIR})
add_definitions(-D_GNU_SOURCE)

# Files
file(GLOB AG_SRC ag_src/*.c)
file(GLOB LIBAG_DOC doc/man3/*.3)

# libag
add_library(ag SHARED ${AG_SRC} libag.c)
set_target_properties(ag PROPERTIES
	PUBLIC_HEADER libag.h)

target_include_directories(ag PRIVATE ag_src/)
target_link_libraries(ag pcre lzma z pthread)

# pkg-config
configure_file(doc/libag.pc.in libag.pc @ONLY)

# Install libag
install(TARGETS ag
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
# Install manpages
install(FILES doc/man1/ag.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
install(FILES ${LIBAG_DOC} DESTINATION ${CMAKE_INSTALL_MANDIR}/man3)
# Install pkgconfig
install(FILES ${CMAKE_BINARY_DIR}/libag.pc
	DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)

# Examples
add_executable(simple
	examples/simple.c)
target_link_libraries(simple ag)

add_executable(init_config
	examples/init_config.c)
target_link_libraries(init_config ag)
