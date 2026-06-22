#include <psutils/psutils.h>

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

#include "sysdeps/sysdeps.h"

int psutils_init(void) {
	return psutils_sysdep_init();
}

int psutils_get_processes(psutils_process_t *table, size_t *process_count, psutils_process_filter_t filter, void *filter_context) {
	psutils_process_t *raw_table = NULL;
	size_t raw_count = 0;
	size_t matched_count = 0;
	int error;

	error = psutils_sysdep_get_processes(NULL, &raw_count);
	if (error)
		return error;

	raw_table = malloc(raw_count * sizeof(psutils_process_t));
	if (raw_table == NULL)
		return ENOMEM;

	error = psutils_sysdep_get_processes(raw_table, &raw_count);
	if (error) {
		free(raw_table);
		return error;
	}

	if (filter == NULL) {
		matched_count = table ? min(raw_count, *process_count) : raw_count;

		if (table)
			memcpy(table, raw_table, matched_count * sizeof(psutils_process_t));
	} else {
		size_t max_count = table ? *process_count : SIZE_MAX;

		for (size_t i = 0; i < raw_count && matched_count < max_count; ++i) {
			if (!filter(&raw_table[i], filter_context))
				continue;

			if (table)
				table[matched_count] = raw_table[i];

			++matched_count;
		}
	}

	*process_count = matched_count;
	free(raw_table);
	return 0;
}
