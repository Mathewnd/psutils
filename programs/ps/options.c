#include "options.h"

#include <ctype.h>
#include <errno.h>
#include <grp.h>
#include <inttypes.h>
#include <pwd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const char *prog_name;

static int append_array(void **array, size_t count, size_t elem_size) {
	void *new_array = realloc(*array, (count + 1) * elem_size);
	if (new_array == NULL)
		return -1;

	*array = new_array;
	return 0;
}

static int append_format(struct ps_options *opts, const char *format) {
	if (append_array((void **)&opts->formats, opts->formats_count, sizeof(*opts->formats)) < 0)
		return -1;

	opts->formats[opts->formats_count++] = format;
	return 0;
}

static int append_pid(pid_t **values, size_t *count, pid_t value) {
	if (append_array((void **)values, *count, sizeof(**values)) < 0)
		return -1;

	(*values)[(*count)++] = value;
	return 0;
}

static int append_uid(uid_t **values, size_t *count, uid_t value) {
	if (append_array((void **)values, *count, sizeof(**values)) < 0)
		return -1;

	(*values)[(*count)++] = value;
	return 0;
}

static int append_gid(gid_t **values, size_t *count, gid_t value) {
	if (append_array((void **)values, *count, sizeof(**values)) < 0)
		return -1;

	(*values)[(*count)++] = value;
	return 0;
}

static int append_string(char ***values, size_t *count, const char *value) {
	if (append_array((void **)values, *count, sizeof(**values)) < 0)
		return -1;

	char *copy = strdup(value);
	if (copy == NULL)
		return -1;

	(*values)[(*count)++] = copy;
	return 0;
}

typedef int (*list_token_fn)(struct ps_options *opts, const char *token, const char *list_name);

static int parse_list(struct ps_options *opts, const char *value, const char *list_name, list_token_fn token_fn) {
	char *copy = strdup(value);
	if (copy == NULL)
		return -1;

	char *token = copy;
	for (char *p = copy;; ++p) {
		if (*p && *p != ',')
			continue;

		char saved = *p;
		*p = 0;
		if (!*token) {
			fprintf(stderr, "%s: empty entry in %s\n", prog_name, list_name);
			free(copy);
			return -1;
		}

		if (token_fn(opts, token, list_name) < 0) {
			free(copy);
			return -1;
		}

		if (!saved)
			break;

		token = p + 1;
	}

	free(copy);
	return 0;
}

static int parse_id_number(const char *token, uintmax_t *id) {
	char *end;

	if (*token == '-')
		return -1;

	errno = 0;
	uintmax_t value = strtoumax(token, &end, 10);
	if (errno || *end)
		return -1;

	*id = value;
	return 0;
}

static int parse_gid(const char *token, gid_t *gid) {
	uintmax_t value;

	if (parse_id_number(token, &value) == 0) {
		*gid = (gid_t)value;
		if ((uintmax_t)*gid != value)
			return -1;

		return 0;
	}

	if (isdigit((unsigned char)*token) || *token == '+' || *token == '-')
		return -1;

	struct group *group = getgrnam(token);
	if (!group)
		return -1;

	*gid = group->gr_gid;
	return 0;
}

static int parse_group_token(struct ps_options *opts, const char *token, const char *list_name) {
	gid_t gid;

	if (parse_gid(token, &gid) < 0) {
		fprintf(stderr, "%s: invalid group '%s' in %s\n", prog_name, token, list_name);
		return -1;
	}

	if (append_gid(&opts->groups, &opts->groups_count, gid) < 0)
		return -1;

	return 0;
}

static int parse_real_group_token(struct ps_options *opts, const char *token, const char *list_name) {
	gid_t gid;

	if (parse_gid(token, &gid) < 0) {
		fprintf(stderr, "%s: invalid group '%s' in %s\n", prog_name, token, list_name);
		return -1;
	}

	if (append_gid(&opts->real_groups, &opts->real_groups_count, gid) < 0)
		return -1;

	return 0;
}

static int parse_name_token(struct ps_options *opts, const char *token, const char *list_name) {
	(void)list_name;

	if (append_string(&opts->names, &opts->names_count, token) < 0)
		return -1;

	return 0;
}

static int parse_pid(const char *token, pid_t *pid) {
	char *end;

	errno = 0;
	intmax_t value = strtoimax(token, &end, 10);
	if (errno || *end || value < 0)
		return -1;

	*pid = (pid_t)value;
	if ((intmax_t)*pid != value)
		return -1;

	return 0;
}

static int parse_process_token(struct ps_options *opts, const char *token, const char *list_name) {
	pid_t pid;

	if (parse_pid(token, &pid) < 0) {
		fprintf(stderr, "%s: invalid process id '%s' in %s\n", prog_name, token, list_name);
		return -1;
	}

	if (append_pid(&opts->processes, &opts->processes_count, pid) < 0)
		return -1;

	return 0;
}

static int parse_terminal_token(struct ps_options *opts, const char *token, const char *list_name) {
	(void)list_name;

	if (append_string(&opts->terminals, &opts->terminals_count, token) < 0)
		return -1;

	return 0;
}

static int parse_uid(const char *token, uid_t *uid) {
	uintmax_t value;

	if (parse_id_number(token, &value) == 0) {
		*uid = (uid_t)value;
		if ((uintmax_t)*uid != value)
			return -1;

		return 0;
	}
	if (isdigit((unsigned char)*token) || *token == '+' || *token == '-')
		return -1;

	struct passwd *passwd = getpwnam(token);
	if (!passwd)
		return -1;

	*uid = passwd->pw_uid;
	return 0;
}

static int parse_user_token(struct ps_options *opts, const char *token, const char *list_name) {
	uid_t uid;

	if (parse_uid(token, &uid) < 0) {
		fprintf(stderr, "%s: invalid user '%s' in %s\n", prog_name, token, list_name);
		return -1;
	}

	if (append_uid(&opts->users, &opts->users_count, uid) < 0)
		return -1;

	return 0;
}

static int parse_real_user_token(struct ps_options *opts, const char *token, const char *list_name) {
	uid_t uid;

	if (parse_uid(token, &uid) < 0) {
		fprintf(stderr, "%s: invalid user '%s' in %s\n", prog_name, token, list_name);
		return -1;
	}

	if (append_uid(&opts->real_users, &opts->real_users_count, uid) < 0)
		return -1;

	return 0;
}

static void free_strings(char **strings, size_t count) {
	for (size_t i = 0; i < count; ++i)
		free(strings[i]);

	free(strings);
}

void free_options(struct ps_options *opts) {
	free(opts->groups);
	free(opts->real_groups);
	free_strings(opts->names, opts->names_count);
	free(opts->processes);
	free_strings(opts->terminals, opts->terminals_count);
	free(opts->users);
	free(opts->real_users);
	free(opts->formats);
}

static void usage(FILE *stream) {
	fprintf(stream,
	    "%s: usage: %s [-aA] [-defl] [-g grouplist] [-G grouplist] [-n namelist]\n"
	    "          [-o format]... [-p proclist] [-t termlist] [-u userlist] [-U userlist]\n", prog_name, prog_name);
}

int parse_options(int argc, char **argv, struct ps_options *opts) {
	int ch;

	prog_name = argv[0];
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
			if (parse_list(opts, optarg, "grouplist", parse_group_token) < 0)
				return -1;
			opts->has_selection = 1;
			break;
		case 'G':
			if (parse_list(opts, optarg, "real grouplist", parse_real_group_token) < 0)
				return -1;
			opts->has_selection = 1;
			break;
		case 'n':
			if (parse_list(opts, optarg, "namelist", parse_name_token) < 0)
				return -1;
			break;
		case 'o':
			if (append_format(opts, optarg) < 0) {
				fprintf(stderr, "%s: failed to allocate format list\n", prog_name);
				return -1;
			}
			break;
		case 'p':
			if (parse_list(opts, optarg, "proclist", parse_process_token) < 0)
				return -1;
			opts->has_selection = 1;
			break;
		case 't':
			if (parse_list(opts, optarg, "termlist", parse_terminal_token) < 0)
				return -1;
			opts->has_selection = 1;
			break;
		case 'u':
			if (parse_list(opts, optarg, "userlist", parse_user_token) < 0)
				return -1;
			opts->has_selection = 1;
			break;
		case 'U':
			if (parse_list(opts, optarg, "real userlist", parse_real_user_token) < 0)
				return -1;
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
