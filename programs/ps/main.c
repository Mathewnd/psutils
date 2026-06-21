#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <psutils/psutils.h>

static char *prog_name;

struct ps_options {
	int all_processes;
	int terminal_processes;
	int exclude_session_leaders;
	int full_listing;
	int long_listing;
	int has_selection;
	const char *grouplist;
	const char *real_grouplist;
	const char *namelist;
	const char *proclist;
	const char *termlist;
	const char *userlist;
	const char *real_userlist;
	const char **formats;
	size_t formats_count;
	size_t formats_capacity;
};

static int append_format(struct ps_options *opts, const char *format) {
	if (opts->formats_count == opts->formats_capacity) {
		size_t new_capacity = opts->formats_capacity ? opts->formats_capacity * 2 : 4;
		const char **new_formats = realloc(opts->formats, new_capacity * sizeof(*opts->formats));
		if (!new_formats) {
			return -1;
		}

		opts->formats = new_formats;
		opts->formats_capacity = new_capacity;
	}

	opts->formats[opts->formats_count++] = format;
	return 0;
}

static void usage(FILE *stream) {
	fprintf(stream,
	    "%s: usage: %s [-aA] [-defl] [-g grouplist] [-G grouplist] [-n namelist]\n"
	    "          [-o format]... [-p proclist] [-t termlist] [-u userlist] [-U userlist]\n", prog_name, prog_name);
}

static int parse_options(int argc, char **argv, struct ps_options *opts) {
	int ch;

	opterr = 0;
	while ((ch = getopt(argc, argv, ":aAdeflg:G:n:o:p:t:u:U:")) != -1) {
		switch (ch) {
		case 'a':
			opts->terminal_processes = 1;
			opts->has_selection = 1;
			break;
		case 'A':
		case 'e':
			opts->all_processes = 1;
			opts->has_selection = 1;
			break;
		case 'd':
			opts->exclude_session_leaders = 1;
			opts->has_selection = 1;
			break;
		case 'f':
			opts->full_listing = 1;
			break;
		case 'l':
			opts->long_listing = 1;
			break;
		case 'g':
			opts->grouplist = optarg;
			opts->has_selection = 1;
			break;
		case 'G':
			opts->real_grouplist = optarg;
			opts->has_selection = 1;
			break;
		case 'n':
			opts->namelist = optarg;
			break;
		case 'o':
			if (append_format(opts, optarg) < 0) {
				fprintf(stderr, "%s: failed to allocate format list\n", prog_name);
				return -1;
			}
			break;
		case 'p':
			opts->proclist = optarg;
			opts->has_selection = 1;
			break;
		case 't':
			opts->termlist = optarg;
			opts->has_selection = 1;
			break;
		case 'u':
			opts->userlist = optarg;
			opts->has_selection = 1;
			break;
		case 'U':
			opts->real_userlist = optarg;
			opts->has_selection = 1;
			break;
		case ':':
			fprintf(stderr, "%s: option '%c' requires an argument\n", prog_name, optopt);
			usage(stderr);
			return -1;
		case '?':
		default:
			fprintf(stderr, "%s: illegal option '%c'\n", prog_name, optopt);
			usage(stderr);
			return -1;
		}
	}

	if (optind < argc) {
		fprintf(stderr, "%s: unexpected operand '%s'\n", prog_name, argv[optind]);
		usage(stderr);
		return -1;
	}

	return 0;
}

int main(int argc, char **argv) {
	struct ps_options opts = {0};
	int ret = 0;

	prog_name = argv[0];

	psutils_init();
	if (parse_options(argc, argv, &opts) < 0) {
		ret = 1;
	}

	free(opts.formats);
	return ret;
}
