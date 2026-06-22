#include <psutils/psutils.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "options.h"
#include "process.h"

int main(int argc, char **argv) {
	struct ps_options opts = {0};
	psutils_process_t *table = NULL;
	size_t count = 0;
	int ret = 0;

	int error = psutils_init();
	if (error) {
		fprintf(stderr, "%s: failed to initialize psutils: %s\n", argv[0], strerror(error));
		return 1;
	}

	if (parse_options(argc, argv, &opts) < 0) {
		ret = 1;
		goto cleanup;
	}

	error = get_process_table(&opts, &table, &count);
	if (error) {
		fprintf(stderr, "%s: failed to get process table: %s\n", argv[0], strerror(error));
		ret = 1;
		goto cleanup;
	}

	cleanup:
	free(table);
	free_options(&opts);
	return ret;
}
