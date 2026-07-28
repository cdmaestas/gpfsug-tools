# Makefile for mmmkfile.
#
# Default build uses the portable preallocation path:
#   - posix_fallocate() on Linux and other POSIX systems
#   - fcntl(F_PREALLOCATE) on macOS / BSD
#
#     make
#
# To build against the native GPFS API (gpfs_prealloc), on a node with the
# GPFS "gpfs.base" / devel files installed:
#
#     make GPFS=1
#
# Override GPFS_INCLUDE / GPFS_LIB if your install is not under /usr/lpp/mmfs.

CC ?= cc
CFLAGS ?= -O2 -Wall -Wextra -D_FILE_OFFSET_BITS=64
LDFLAGS ?=
LDLIBS ?=

GPFS_INCLUDE ?= /usr/lpp/mmfs/include
GPFS_LIB ?= /usr/lpp/mmfs/lib

ifdef GPFS
CFLAGS += -DHAVE_GPFS -I$(GPFS_INCLUDE)
LDFLAGS += -L$(GPFS_LIB)
LDLIBS += -lgpfs
endif

PROG = mmmkfile

$(PROG): mmmkfile.c
	$(CC) $(CFLAGS) -o $@ mmmkfile.c $(LDFLAGS) $(LDLIBS)

clean:
	rm -f $(PROG)

.PHONY: clean
