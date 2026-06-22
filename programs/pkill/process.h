#ifndef PKILL_PROCESS_H
#define PKILL_PROCESS_H

#include <psutils/psutils.h>

#include <regex.h>
#include <stdbool.h>

#include "options.h"

struct pkill_process_filter_context {
	const struct pkill_options *opts;
	const regex_t *regex;
	pid_t self;
};

bool pkill_process_filter(const psutils_process_t *process, void *context_ptr);
bool pkill_select_by_age(const struct pkill_options *opts, const psutils_process_t *candidate, const psutils_process_t *selected);

#endif
