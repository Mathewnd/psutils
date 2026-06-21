#ifndef PSUTILS_SYSDEPS_H
#define PSUTILS_SYSDEPS_H

#include <stddef.h>

#include <psutils/psutils.h>

int psutils_sysdep_init(void);
int psutils_sysdep_get_processes(psutils_process_t *table, size_t *table_size);

#endif
