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

cmake_minimum_required(VERSION 3.9)
cmake_policy(SET CMP0042 NEW)
set (CMAKE_C_STANDARD 99)

# Get PkgConfig package
find_package(PkgConfig REQUIRED) 

project(libag_wrapper)

# Node headers
include_directories(${CMAKE_JS_INC})

#  Our binding lib
add_library(${PROJECT_NAME} SHARED libag_node.c ${CMAKE_JS_SRC})
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")

# Check if we can find libag
pkg_check_modules(LIBAG libag)

# If not, try to use local libag
if(NOT EQUAL ${LIBAG_FOUND})
	message(WARNING "libag not found!, trying to use local version")

	get_filename_component(LIBAG_PATH "../../"
		REALPATH BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

	target_include_directories(${PROJECT_NAME} PUBLIC ${LIBAG_PATH})
	target_link_directories(${PROJECT_NAME} PUBLIC ${LIBAG_PATH})
else()
	target_include_directories(${PROJECT_NAME} PUBLIC ${LIBAG_INCLUDE_DIRS})
	target_link_directories(${PROJECT_NAME} PUBLIC ${LIBAG_LIBRARY_DIRS})
endif(NOT EQUAL ${LIBAG_FOUND})

# libraries
target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB} libag.so)

# Define NAPI_VERSION
add_definitions(-DNAPI_VERSION=8)
