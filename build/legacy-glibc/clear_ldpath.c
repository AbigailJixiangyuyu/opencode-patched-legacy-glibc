#define _GNU_SOURCE
#include <stdlib.h>

__attribute__((constructor))
static void clear_ld_preload(void) {
    unsetenv("LD_PRELOAD");
}