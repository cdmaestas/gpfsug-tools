# mmmkfile

Create a file of a given size on a GPFS/Spectrum Scale (or any POSIX)
filesystem, in the spirit of the classic `mkfile(8)`. By default the blocks are
actually allocated (the file is **not** sparse) using the platform's native
preallocation call, so the space is reserved efficiently without pushing data
through the write path. Handy for benchmarking, filling a pool, or exercising
fileset quotas.

## Build

Portable build (uses `posix_fallocate()` on Linux, `fcntl(F_PREALLOCATE)` on
macOS/BSD):

```
make
```

Native GPFS build (uses `gpfs_prealloc()`; needs the GPFS devel headers/libs,
by default under `/usr/lpp/mmfs`):

```
make GPFS=1
# or, if your install lives elsewhere:
make GPFS=1 GPFS_INCLUDE=/path/include GPFS_LIB=/path/lib
```

## Usage

```
mmmkfile [-n] [-v] <size> <filename>
```

- `<size>` is a number with an optional base-1024 unit suffix:
  `b` bytes (default), `k` KiB, `m` MiB, `g` GiB, `t` TiB — e.g. `100m`, `2g`.
- `-n` sets the size only, without allocating blocks (a sparse file).
- `-v` prints the file name and size.
- `-h` prints help.

Examples:

```
mmmkfile 10g   /gpfs/scratch/testfile     # preallocate 10 GiB
mmmkfile -n 1t /gpfs/scratch/sparsefile   # 1 TiB sparse (no space used)
```

See the manual page for details: `man -l ../../man/mmmkfile.1`.

## History

This is a clean reimplementation of the original 2012 `mmmkfile.C`, which was
abandoned mid-development (it did not compile and contained a heap-buffer
overflow in an unfinished multithreaded zero-fill path). Preallocation is a
single kernel/GPFS call, so the thread pool was never needed; this version drops
it and fails loudly on any error.
