#ifndef PS_PROCESS_H
#define PS_PROCESS_H

#include <stddef.h>
#include <psutils/psutils.h>
#include "options.h"

int get_process_table(const struct ps_options *opts, psutils_process_t **table, size_t *count);

#endif
