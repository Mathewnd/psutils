#include "options.h"

int main(int argc, char **argv) {
	struct pgrep_options opts = {0};
	int ret = 0;

	if (parse_options(argc, argv, &opts) < 0)
		ret = 1;

	free_options(&opts);
	return ret;
}
