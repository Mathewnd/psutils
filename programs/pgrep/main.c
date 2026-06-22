#include <psutils/psutils.h>

#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "options.h"
#include "process.h"

int main(int argc, char **argv) {
	struct pgrep_options opts = {0};
	psutils_process_t *table = NULL;
	size_t count = 0;
	regex_t regex;
	bool regex_compiled = false;
	bool matched = false;
	struct pgrep_process_filter_context filter_context = {
		.opts = &opts,
		.self = getpid(),
	};
	int ret = 1;

	int error = psutils_init();
	if (error) {
		fprintf(stderr, "%s: failed to initialize psutils: %s\n", argv[0], strerror(error));
		return 1;
	}

	if (parse_options(argc, argv, &opts) < 0)
		goto cleanup;

	if (opts.pattern && !opts.exact) {
		error = regcomp(&regex, opts.pattern, REG_EXTENDED | REG_NOSUB);
		if (error) {
			char buffer[128];
			regerror(error, &regex, buffer, sizeof(buffer));
			fprintf(stderr, "%s: invalid pattern '%s': %s\n", argv[0], opts.pattern, buffer);
			goto cleanup;
		}

		regex_compiled = true;
		filter_context.regex = &regex;
	}

	error = psutils_get_processes(NULL, &count, pgrep_process_filter, &filter_context);
	if (error) {
		fprintf(stderr, "%s: failed to get process table: %s\n", argv[0], strerror(error));
		goto cleanup;
	}
	if (count == 0)
		goto cleanup;

	table = malloc(count * sizeof(*table));
	if (table == NULL) {
		fprintf(stderr, "%s: failed to allocate process table\n", argv[0]);
		goto cleanup;
	}

	error = psutils_get_processes(table, &count, pgrep_process_filter, &filter_context);
	if (error) {
		fprintf(stderr, "%s: failed to get process table: %s\n", argv[0], strerror(error));
		goto cleanup;
	}

	if (opts.newest || opts.oldest) {
		psutils_process_t *selected = NULL;

		for (size_t i = 0; i < count; ++i) {
			matched = true;
			if (selected == NULL || pgrep_select_by_age(&opts, &table[i], selected))
				selected = &table[i];
		}
	} else {
		matched = count > 0;
	}

	ret = matched ? 0 : 1;

cleanup:
	if (regex_compiled)
		regfree(&regex);
	free(table);
	free_options(&opts);
	return ret;
}
