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

AG_SRC  = $(CURDIR)/ag_src
INCLUDE = -I $(AG_SRC)
PREFIX ?= /usr/local
BINDIR  = $(PREFIX)/bin

#===================================================================
# Flags
#===================================================================

CC      ?= gcc
CFLAGS  += -Wall -Wextra -Wformat=2 -Wno-format-nonliteral -Wshadow
CFLAGS  += -Wpointer-arith -Wcast-qual -Wmissing-prototypes -Wno-missing-braces
CFLAGS  += -std=gnu89 -D_GNU_SOURCE -O2
LDFLAGS  = -lpcre -llzma -lz -pthread

#===================================================================
# Rules
#===================================================================

# Conflicts
.PHONY : all clean

# Sources
C_SRC = $(wildcard $(AG_SRC)/*.c)

# Objects
OBJ = $(C_SRC:.c=.o)

all: ag

# Build objects rule
%.o: %.c
	$(CC) $< $(CFLAGS) -c -o $@

# Ag standalone build
ag: $(OBJ)
	$(CC) $^ $(CFLAGS) $(LDFLAGS) -o $@

clean:
	@rm -f $(AG_SRC)/*.o
	@rm -f $(CURDIR)/ag
