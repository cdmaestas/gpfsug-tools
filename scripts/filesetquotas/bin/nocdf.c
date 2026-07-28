#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Thin setuid-style wrapper that execs the gpfsdf reporting script.
 * NOTE: the target path is site-specific; adjust for your deployment. */
int main(int argc, char *argv[]) {
  (void)argc;
  const char *script = "/root/bin/gpfsdf";

  execv(script, argv);

  /* execv only returns on failure - report it instead of falling through
   * main() with an undefined return value. */
  perror(script);
  return EXIT_FAILURE;
}
