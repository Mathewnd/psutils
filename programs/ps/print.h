#ifndef PS_PRINT_H
#define PS_PRINT_H

#include <stddef.h>

#include <psutils/psutils.h>

#include "options.h"

int print_process_table(const struct ps_options *opts, const psutils_process_t *table, size_t count);

#endif
