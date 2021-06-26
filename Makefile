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

#===================================================================
# Paths
#===================================================================

AG_SRC   = $(CURDIR)/ag_src
INCLUDE  = -I $(AG_SRC)
PREFIX  ?= /usr/local
BINDIR   = $(PREFIX)/bin
LIBDIR   = $(PREFIX)/lib
MANPAGES = $(CURDIR)/doc/

# Bindings
SWIG    ?= swig
PYBIND   = $(CURDIR)/bindings/python
JSBIND   = $(CURDIR)/bindings/javascript
PY_INC  ?= $(shell python-config --includes)

#===================================================================
# Flags
#===================================================================

CC        ?= gcc
PY_CFLAGS := $(CFLAGS)
CFLAGS    += -Wall -Wextra -Wformat=2 -Wno-format-nonliteral -Wshadow
CFLAGS    += -Wpointer-arith -Wcast-qual -Wmissing-prototypes -Wno-missing-braces
CFLAGS    += -fPIC -std=c99 -D_GNU_SOURCE -MMD -O3
LDFLAGS    = -shared
LDLIBS     = -lpcre -llzma -lz -pthread

# Bindings
PY_CFLAGS += -fPIC -std=c99 -D_GNU_SOURCE -MMD -O3

#===================================================================
# Rules
#===================================================================

# Conflicts
.PHONY : all clean examples install uninstall libag.pc
.PHONY : bindings python-binding

# Paths
INCDIR  = $(PREFIX)/include
BINDIR  = $(PREFIX)/bin
MANDIR  = $(PREFIX)/man
PKGDIR  = $(LIBDIR)/pkgconfig
PKGFILE = $(DESTDIR)$(PKGDIR)/libag.pc

# Sources
C_SRC = $(wildcard $(AG_SRC)/*.c) \
	libag.c

# Objects
OBJ = $(C_SRC:.c=.o)

# Dependencies
DEP = $(OBJ:.o=.d)

all: libag.so examples

# Pretty print
Q := @
ifeq ($(V), 1)
	Q :=
endif

# Build objects rule
%.o: %.c
	@echo "  CC      $@"
	$(Q)$(CC) $< $(CFLAGS) $(INCLUDE) -c -o $@

# Ag standalone build
libag.so: $(OBJ)
	@echo "  LD      $@"
	$(Q)$(CC) $^ $(CFLAGS) $(LDFLAGS) $(LDLIBS) -o $@

# Install
install: libag.so libag.pc
	@echo "  INSTALL      $@"
	$(Q)install -d $(DESTDIR)$(LIBDIR)
	$(Q)install -m 755 $(CURDIR)/libag.so $(DESTDIR)$(LIBDIR)
	$(Q)install -d $(DESTDIR)$(INCDIR)/
	$(Q)install -m 644 $(CURDIR)/libag.h $(DESTDIR)$(INCDIR)/
	$(Q)install -d $(DESTDIR)$(MANDIR)/man1
	$(Q)install -d $(DESTDIR)$(MANDIR)/man3
	$(Q)install -m 644 $(MANPAGES)/man1/*.1 $(DESTDIR)$(MANDIR)/man1/
	$(Q)install -m 644 $(MANPAGES)/man3/*.3 $(DESTDIR)$(MANDIR)/man3/

# Uninstall
uninstall:
	@echo "  UNINSTALL      $@"
	$(Q)rm -f $(DESTDIR)$(INCDIR)/libag.h
	$(Q)rm -f $(DESTDIR)$(LIBDIR)/libag.so
	$(Q)rm -f $(DESTDIR)$(PKGDIR)/libag.pc
	$(Q)rm -f $(DESTDIR)$(MANDIR)/man1/ag.1
	$(Q)rm -f $(DESTDIR)$(MANDIR)/man3/ag_finish.3
	$(Q)rm -f $(DESTDIR)$(MANDIR)/man3/ag_free_all_results.3
	$(Q)rm -f $(DESTDIR)$(MANDIR)/man3/ag_free_result.3
	$(Q)rm -f $(DESTDIR)$(MANDIR)/man3/ag_get_stats.3
	$(Q)rm -f $(DESTDIR)$(MANDIR)/man3/ag_init.3
	$(Q)rm -f $(DESTDIR)$(MANDIR)/man3/ag_init_config.3
	$(Q)rm -f $(DESTDIR)$(MANDIR)/man3/ag_search.3
	$(Q)rm -f $(DESTDIR)$(MANDIR)/man3/ag_search_ts.3
	$(Q)rm -f $(DESTDIR)$(MANDIR)/man3/ag_set_config.3
	$(Q)rm -f $(DESTDIR)$(MANDIR)/man3/ag_start_workers.3
	$(Q)rm -f $(DESTDIR)$(MANDIR)/man3/ag_stop_workers.3

# Generate libag.pc
libag.pc:
	@install -d $(DESTDIR)$(PKGDIR)
	@echo 'prefix='$(DESTDIR)$(PREFIX)               >  $(PKGFILE)
	@echo 'libdir='$(DESTDIR)$(LIBDIR)               >> $(PKGFILE)
	@echo 'includedir=$${prefix}/include'            >> $(PKGFILE)
	@echo 'Name: libag'                              >> $(PKGFILE)
	@echo 'Description: The Silver Searcher Library' >> $(PKGFILE)
	@echo 'Version: 1.0'                             >> $(PKGFILE)
	@echo 'Libs: -L$${libdir} -lag'                  >> $(PKGFILE)
	@echo 'Libs.private: -lpcre -lzma -lz -pthread'  >> $(PKGFILE)
	@echo 'Cflags: -I$${includedir}/'                >> $(PKGFILE)

# Examples
examples: examples/simple examples/init_config
examples/%.o: examples/%.c
	@echo "  CC      $@"
	$(Q)$(CC) $^ -c -I $(CURDIR) -o $@
examples/simple: examples/simple.o libag.so
	@echo "  LD      $@"
	$(Q)$(CC) $< -o $@ libag.so -Wl,-rpath,$(CURDIR)
examples/init_config: examples/init_config.o libag.so
	@echo "  LD      $@"
	$(Q)$(CC) $< -o $@ libag.so -Wl,-rpath,$(CURDIR)

# Bindings
bindings: python-binding node-binding
python-binding: $(PYBIND)/_libag.so
node-binding: $(JSBIND)/build/Release/libag_wrapper.node

# Python binding
$(PYBIND)/_libag.so: $(PYBIND)/libag_wrap.o $(OBJ)
	@echo "  LD      $@"
	$(Q)$(CC) $^ $(PY_CFLAGS) $(LDFLAGS) $(LDLIBS) -o $@
$(PYBIND)/libag_wrap.o: $(PYBIND)/libag_wrap.c
	@echo "  CC      $@"
	$(Q)$(CC) $^ $(PY_CFLAGS) $(PY_INC) -I $(CURDIR) -c -o $@
$(PYBIND)/libag_wrap.c: $(PYBIND)/libag.i
	@echo "  SWIG      $@"
	$(Q)$(SWIG) -python -o $(PYBIND)/libag_wrap.c $(PYBIND)/libag.i

# Node binding
$(JSBIND)/build/Release/libag_wrapper.node: $(JSBIND)/libag_node.c libag.so
	@echo "  CC      $@"
	$(Q)cd $(JSBIND) && cmake-js --CDCMAKE_PREFIX_PATH=$(PREFIX)

# Clean
clean:
	@echo "  CLEAN        "
	$(Q)rm -f $(AG_SRC)/*.o
	$(Q)rm -f $(CURDIR)/*.o
	$(Q)rm -f $(CURDIR)/libag.so
	$(Q)rm -f $(CURDIR)/examples/*.o
	$(Q)rm -f $(CURDIR)/examples/simple
	$(Q)rm -f $(CURDIR)/examples/init_config
	$(Q)rm -f $(PYBIND)/*.o
	$(Q)rm -f $(PYBIND)/*.py
	$(Q)rm -f $(PYBIND)/*.pyc
	$(Q)rm -f $(PYBIND)/*.so
	$(Q)rm -f $(PYBIND)/*.d
	$(Q)rm -f $(PYBIND)/libag_wrap.c
	$(Q)rm -rf $(PYBIND)/__pycache__
	$(Q)rm -f $(DEP)

# Our dependencies =)
-include $(DEP)
