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

# Get python include path: -I/something/ -I/something/
if("$ENV{PY_INC}" STREQUAL "")
	execute_process(
		COMMAND python-config --includes
		OUTPUT_VARIABLE PY_INC
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
else()
	set(PY_INC "$ENV{PY_INC}")
endif("$ENV{PY_INC}" STREQUAL "")

# If not, try to use Python 3
if("${PY_INC}" STREQUAL "")
	message(WARNING "Python 2 not found, trying to use Python 3!")
	execute_process(
		COMMAND python3-config --includes
		OUTPUT_VARIABLE PY_INC
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	if("${PY_INC}" STREQUAL "")
		message(WARNING
			"Python 3 not found!, python bindings cannot be built without "
			"python-config/python3-config please check your python installation "
			"and/or manually define the variable PY_INC!")
	endif("${PY_INC}" STREQUAL "")
endif("${PY_INC}" STREQUAL "")

# Copy examples files
file(COPY ${CMAKE_SOURCE_DIR}/bindings/python/examples
	DESTINATION ${CMAKE_BINARY_DIR}/bindings/python/
)

#
# Get the first path component of PY_INC
# Yes, this is ugly, but 'python-config' tries to imitate
# pkg-config and appends '-I' to the path component....
#
set(PY_LIST_PATH ${PY_INC})
separate_arguments(PY_LIST_PATH)
list(GET PY_LIST_PATH 0 PY_INC_PATH)
string(REPLACE "-I" "" PY_INC_PATH ${PY_INC_PATH})

# Invoke swig to generate libag_wrap.c
add_custom_command(
	OUTPUT ${CMAKE_BINARY_DIR}/bindings/python/libag_wrap.c
	COMMAND swig -python -o
		${CMAKE_BINARY_DIR}/bindings/python/libag_wrap.c
		-I${CMAKE_SOURCE_DIR}
		${PY_INC}
		${CMAKE_SOURCE_DIR}/bindings/python/libag.i
)

# libag wrapper
add_library(python-binding SHARED EXCLUDE_FROM_ALL
	${CMAKE_BINARY_DIR}/bindings/python/libag_wrap.c
	$<TARGET_OBJECTS:libag_objects>
)

set_target_properties(python-binding PROPERTIES
	PREFIX "_"
	OUTPUT_NAME "libag"
)
target_include_directories(python-binding PRIVATE ${PY_INC_PATH})
target_link_libraries(python-binding pcre lzma z pthread)
