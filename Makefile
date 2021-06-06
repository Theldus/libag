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

#===================================================================
# Paths
#===================================================================

AG_SRC   = $(CURDIR)/ag_src
INCLUDE  = -I $(AG_SRC)
PREFIX  ?= /usr/local
BINDIR   = $(PREFIX)/bin
LIBDIR   = $(PREFIX)/lib
MANPAGES = $(CURDIR)/doc/

#===================================================================
# Flags
#===================================================================

CC      ?= gcc
CFLAGS  += -Wall -Wextra -Wformat=2 -Wno-format-nonliteral -Wshadow
CFLAGS  += -Wpointer-arith -Wcast-qual -Wmissing-prototypes -Wno-missing-braces
CFLAGS  += -fPIC -std=c99 -D_GNU_SOURCE -MMD -O3
LDFLAGS  = -shared
LDLIBS   = -lpcre -llzma -lz -pthread

#===================================================================
# Rules
#===================================================================

# Conflicts
.PHONY : all clean examples install uninstall libag.pc

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

# Clean
clean:
	@echo "  CLEAN        "
	$(Q)rm -f $(AG_SRC)/*.o
	$(Q)rm -f $(CURDIR)/*.o
	$(Q)rm -f $(CURDIR)/libag.so
	$(Q)rm -f $(CURDIR)/examples/*.o
	$(Q)rm -f $(CURDIR)/examples/simple
	$(Q)rm -f $(CURDIR)/examples/init_config
	$(Q)rm -f $(DEP)

# Our dependencies =)
-include $(DEP)
