#ifndef PGREP_PROCESS_H
#define PGREP_PROCESS_H

#include <psutils/psutils.h>

#include <regex.h>
#include <stdbool.h>

#include "options.h"

struct pgrep_process_filter_context {
	const struct pgrep_options *opts;
	const regex_t *regex;
	pid_t self;
};

bool pgrep_process_filter(const psutils_process_t *process, void *context_ptr);
bool pgrep_select_by_age(const struct pgrep_options *opts, const psutils_process_t *candidate, const psutils_process_t *selected);

#endif
