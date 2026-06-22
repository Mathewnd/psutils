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

typedef int (*list_token_fn)(struct pgrep_options *opts, const char *token, const char *list_name);

static int append_array(void **array, size_t count, size_t elem_size) {
	void *new_array = realloc(*array, (count + 1) * elem_size);
	if (new_array == NULL)
		return -1;

	*array = new_array;
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

static int parse_list(struct pgrep_options *opts, const char *value, const char *list_name, list_token_fn token_fn) {
	char *copy = strdup(value);
	if (copy == NULL)
		return -1;

	char *token = copy;
	for (char *p = copy;; ++p) {
		if (*p && *p != ',' && !isspace((unsigned char)*p))
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

static int parse_uid(const char *token, uid_t *uid) {
	uintmax_t value;

	if (parse_id_number(token, &value) == 0) {
		*uid = (uid_t)value;
		return (uintmax_t)*uid == value ? 0 : -1;
	}
	if (isdigit((unsigned char)*token) || *token == '+' || *token == '-')
		return -1;

	struct passwd *passwd = getpwnam(token);
	if (!passwd)
		return -1;

	*uid = passwd->pw_uid;
	return 0;
}

static int parse_gid(const char *token, gid_t *gid) {
	uintmax_t value;

	if (parse_id_number(token, &value) == 0) {
		*gid = (gid_t)value;
		return (uintmax_t)*gid == value ? 0 : -1;
	}
	if (isdigit((unsigned char)*token) || *token == '+' || *token == '-')
		return -1;

	struct group *group = getgrnam(token);
	if (!group)
		return -1;

	*gid = group->gr_gid;
	return 0;
}

static int parse_parent_token(struct pgrep_options *opts, const char *token, const char *list_name) {
	pid_t pid;

	if (parse_pid(token, &pid) < 0) {
		fprintf(stderr, "%s: invalid parent process id '%s' in %s\n", prog_name, token, list_name);
		return -1;
	}

	return append_pid(&opts->parents, &opts->parents_count, pid);
}

static int parse_process_group_token(struct pgrep_options *opts, const char *token, const char *list_name) {
	pid_t pid;

	if (parse_pid(token, &pid) < 0) {
		fprintf(stderr, "%s: invalid process group id '%s' in %s\n", prog_name, token, list_name);
		return -1;
	}
	if (pid == 0)
		pid = getpgrp();

	return append_pid(&opts->process_groups, &opts->process_groups_count, pid);
}

static int parse_session_token(struct pgrep_options *opts, const char *token, const char *list_name) {
	pid_t pid;

	if (parse_pid(token, &pid) < 0) {
		fprintf(stderr, "%s: invalid session id '%s' in %s\n", prog_name, token, list_name);
		return -1;
	}
	if (pid == 0)
		pid = getsid(0);

	return append_pid(&opts->sessions, &opts->sessions_count, pid);
}

static int parse_effective_user_token(struct pgrep_options *opts, const char *token, const char *list_name) {
	uid_t uid;

	if (parse_uid(token, &uid) < 0) {
		fprintf(stderr, "%s: invalid user '%s' in %s\n", prog_name, token, list_name);
		return -1;
	}

	return append_uid(&opts->effective_users, &opts->effective_users_count, uid);
}

static int parse_real_user_token(struct pgrep_options *opts, const char *token, const char *list_name) {
	uid_t uid;

	if (parse_uid(token, &uid) < 0) {
		fprintf(stderr, "%s: invalid user '%s' in %s\n", prog_name, token, list_name);
		return -1;
	}

	return append_uid(&opts->real_users, &opts->real_users_count, uid);
}

static int parse_real_group_token(struct pgrep_options *opts, const char *token, const char *list_name) {
	gid_t gid;

	if (parse_gid(token, &gid) < 0) {
		fprintf(stderr, "%s: invalid group '%s' in %s\n", prog_name, token, list_name);
		return -1;
	}

	return append_gid(&opts->real_groups, &opts->real_groups_count, gid);
}

static int parse_terminal_token(struct pgrep_options *opts, const char *token, const char *list_name) {
	(void)list_name;

	return append_string(&opts->terminals, &opts->terminals_count, token);
}

static void free_strings(char **strings, size_t count) {
	for (size_t i = 0; i < count; ++i)
		free(strings[i]);

	free(strings);
}

void free_options(struct pgrep_options *opts) {
	free(opts->parents);
	free(opts->process_groups);
	free(opts->sessions);
	free(opts->effective_users);
	free(opts->real_users);
	free(opts->real_groups);
	free_strings(opts->terminals, opts->terminals_count);
}

static void usage(FILE *stream) {
	fprintf(stream,
	    "%s: usage: %s [-flnovx] [-d delimiter] [-P ppidlist] [-g pgrplist]\n"
	    "             [-s sidlist] [-u euidlist] [-U uidlist] [-G gidlist]\n"
	    "             [-t termlist] [pattern]\n", prog_name, prog_name);
}

int parse_options(int argc, char **argv, struct pgrep_options *opts) {
	int ch;

	prog_name = argv[0];
	opts->delimiter = "\n";

	opterr = 0;
	while ((ch = getopt(argc, argv, ":d:flnoP:g:s:u:U:G:t:vx")) != -1) {
		switch (ch) {
		case 'd':
			opts->delimiter = optarg;
			break;
		case 'f':
			opts->full_match = true;
			break;
		case 'l':
			opts->list_name = true;
			break;
		case 'n':
			opts->newest = true;
			break;
		case 'o':
			opts->oldest = true;
			break;
		case 'P':
			if (parse_list(opts, optarg, "parent process list", parse_parent_token) < 0)
				return -1;
			break;
		case 'g':
			if (parse_list(opts, optarg, "process group list", parse_process_group_token) < 0)
				return -1;
			break;
		case 's':
			if (parse_list(opts, optarg, "session list", parse_session_token) < 0)
				return -1;
			break;
		case 'u':
			if (parse_list(opts, optarg, "effective user list", parse_effective_user_token) < 0)
				return -1;
			break;
		case 'U':
			if (parse_list(opts, optarg, "real user list", parse_real_user_token) < 0)
				return -1;
			break;
		case 'G':
			if (parse_list(opts, optarg, "real group list", parse_real_group_token) < 0)
				return -1;
			break;
		case 't':
			if (parse_list(opts, optarg, "terminal list", parse_terminal_token) < 0)
				return -1;
			break;
		case 'v':
			opts->inverse = true;
			break;
		case 'x':
			opts->exact = true;
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

	if (opts->newest && opts->oldest) {
		fprintf(stderr, "%s: options '-n' and '-o' are mutually exclusive\n", prog_name);
		return -1;
	}
	if ((opts->newest || opts->oldest) && opts->inverse) {
		fprintf(stderr, "%s: option '-v' cannot be combined with '-n' or '-o'\n", prog_name);
		return -1;
	}

	if (optind < argc)
		opts->pattern = argv[optind++];

	if (optind < argc) {
		fprintf(stderr, "%s: unexpected operand '%s'\n", prog_name, argv[optind]);
		usage(stderr);
		return -1;
	}

	return 0;
}
