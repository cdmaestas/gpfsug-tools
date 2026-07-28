/*
 * mmmkfile - create a file of a given size on a GPFS/Spectrum Scale (or any
 *            POSIX) filesystem, in the spirit of the classic mkfile(8).
 *
 * By default the file's blocks are actually allocated (the file is NOT sparse),
 * using the platform's native preallocation primitive:
 *
 *   - GPFS gpfs_prealloc()     when built with -DHAVE_GPFS (make GPFS=1)
 *   - fcntl(F_PREALLOCATE)     on macOS / BSD
 *   - posix_fallocate()        everywhere else (Linux, ...)
 *
 * With -n the size is set but no blocks are allocated (a sparse file).
 *
 * This is a clean reimplementation of the abandoned 2012 mmmkfile.C: it drops
 * the unfinished multithreaded zero-fill (preallocation is a single kernel/GPFS
 * call, so the pthread pool was never needed) and fails loudly on any error.
 */

#if !defined(__APPLE__)
#define _XOPEN_SOURCE 700 /* expose posix_fallocate, ftruncate, getopt */
#endif

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef HAVE_GPFS
#include <gpfs.h>
#endif

static const char *progname = "mmmkfile";

static void usage(FILE *out) {
  fprintf(out,
          "Usage: %s [-n] [-v] <size> <filename>\n"
          "\n"
          "  Create <filename> and give it <size> bytes of allocated space.\n"
          "\n"
          "  <size> is a number with an optional unit suffix (base 1024):\n"
          "      b  bytes (default)   k  KiB   m  MiB   g  GiB   t  TiB\n"
          "  e.g. 4096, 512k, 100m, 2g, 1t\n"
          "\n"
          "  -n  do not allocate blocks; set the size only (sparse file)\n"
          "  -v  verbose: report the file name and size\n"
          "  -h  show this help\n",
          progname);
}

/*
 * Parse a size string like "100m" into a byte count. Returns 0 on success and
 * -1 on any malformed input or overflow.
 */
static int parse_size(const char *s, unsigned long long *out) {
  char *end = NULL;
  unsigned long long val;
  unsigned long long mult = 1ULL;

  if (s == NULL || *s == '\0') {
    return -1;
  }

  errno = 0;
  val = strtoull(s, &end, 10);
  if (errno != 0 || end == s) {
    return -1; /* not a number, or out of range */
  }

  if (*end != '\0') {
    switch (*end) {
      case 'b':
      case 'B':
        mult = 1ULL;
        break;
      case 'k':
      case 'K':
        mult = 1024ULL;
        break;
      case 'm':
      case 'M':
        mult = 1024ULL * 1024;
        break;
      case 'g':
      case 'G':
        mult = 1024ULL * 1024 * 1024;
        break;
      case 't':
      case 'T':
        mult = 1024ULL * 1024 * 1024 * 1024;
        break;
      default:
        return -1;
    }
    if (*(end + 1) != '\0') {
      return -1; /* trailing junk after the unit */
    }
  }

  if (val != 0 && mult > ULLONG_MAX / val) {
    return -1; /* multiplication would overflow */
  }
  *out = val * mult;
  return 0;
}

/*
 * Allocate `length` bytes of real disk space for the open file `fd`, starting
 * at offset 0. Returns 0 on success, -1 on error (errno set).
 */
static int preallocate(int fd, off_t length) {
#if defined(HAVE_GPFS)
  /* gpfs_prealloc returns 0 on success, -1 on error with errno set. */
  return gpfs_prealloc(fd, (gpfs_off64_t)0, (gpfs_off64_t)length);
#elif defined(__APPLE__)
  fstore_t fst;
  fst.fst_flags = F_ALLOCATECONTIG;
  fst.fst_posmode = F_PEOFPOSMODE;
  fst.fst_offset = 0;
  fst.fst_length = length;
  fst.fst_bytesalloc = 0;
  if (fcntl(fd, F_PREALLOCATE, &fst) == -1) {
    /* Retry allowing non-contiguous allocation. */
    fst.fst_flags = F_ALLOCATEALL;
    if (fcntl(fd, F_PREALLOCATE, &fst) == -1) {
      return -1;
    }
  }
  /* F_PREALLOCATE reserves space past EOF but does not move EOF. */
  return ftruncate(fd, length);
#else
  /* posix_fallocate returns an errno value directly and does not set errno. */
  int rc = posix_fallocate(fd, 0, length);
  if (rc != 0) {
    errno = rc;
    return -1;
  }
  return 0;
#endif
}

int main(int argc, char *argv[]) {
  int opt;
  int sparse = 0;
  int verbose = 0;
  unsigned long long size = 0;
  const char *filename;
  int fd;

  if (argc > 0 && argv[0] != NULL && argv[0][0] != '\0') {
    progname = argv[0];
  }

  while ((opt = getopt(argc, argv, "nvh")) != -1) {
    switch (opt) {
      case 'n':
        sparse = 1;
        break;
      case 'v':
        verbose = 1;
        break;
      case 'h':
        usage(stdout);
        return EXIT_SUCCESS;
      default:
        usage(stderr);
        return EXIT_FAILURE;
    }
  }

  if (argc - optind != 2) {
    usage(stderr);
    return EXIT_FAILURE;
  }

  if (parse_size(argv[optind], &size) != 0) {
    fprintf(stderr, "%s: invalid size '%s'\n", progname, argv[optind]);
    return EXIT_FAILURE;
  }
  filename = argv[optind + 1];

  /* Guard against a size that will not fit in the signed off_t used below.
   * Compute off_t's max portably (all value bits set) rather than relying on
   * OFF_MAX, which is not defined on all platforms (e.g. glibc/Linux). */
  const unsigned long long off_t_max =
      ((unsigned long long)1 << (sizeof(off_t) * CHAR_BIT - 1)) - 1ULL;
  if (size > off_t_max) {
    fprintf(stderr, "%s: size %llu bytes is too large for this platform\n",
            progname, size);
    return EXIT_FAILURE;
  }

  fd = open(filename, O_RDWR | O_CREAT, 0644);
  if (fd < 0) {
    fprintf(stderr, "%s: cannot open %s: %s\n", progname, filename,
            strerror(errno));
    return EXIT_FAILURE;
  }

  if (sparse) {
    if (ftruncate(fd, (off_t)size) != 0) {
      fprintf(stderr, "%s: cannot set size of %s: %s\n", progname, filename,
              strerror(errno));
      close(fd);
      return EXIT_FAILURE;
    }
  } else if (preallocate(fd, (off_t)size) != 0) {
    fprintf(stderr, "%s: cannot allocate %llu bytes for %s: %s\n", progname,
            size, filename, strerror(errno));
    close(fd);
    return EXIT_FAILURE;
  }

  if (close(fd) != 0) {
    fprintf(stderr, "%s: error closing %s: %s\n", progname, filename,
            strerror(errno));
    return EXIT_FAILURE;
  }

  if (verbose) {
    printf("%s: %llu bytes%s\n", filename, size, sparse ? " (sparse)" : "");
  }
  return EXIT_SUCCESS;
}
