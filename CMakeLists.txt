# Copyright 2021 Davidson Francis <davidsondfgl@gmail.com>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

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
