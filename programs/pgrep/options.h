#ifndef PGREP_OPTIONS_H
#define PGREP_OPTIONS_H

#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

struct pgrep_options {
	bool full_match;
	bool list_name;
	bool newest;
	bool oldest;
	bool inverse;
	bool exact;
	const char *delimiter;
	const char *pattern;
	pid_t *parents;
	size_t parents_count;
	pid_t *process_groups;
	size_t process_groups_count;
	pid_t *sessions;
	size_t sessions_count;
	uid_t *effective_users;
	size_t effective_users_count;
	uid_t *real_users;
	size_t real_users_count;
	gid_t *real_groups;
	size_t real_groups_count;
	char **terminals;
	size_t terminals_count;
};

int parse_options(int argc, char **argv, struct pgrep_options *opts);
void free_options(struct pgrep_options *opts);

#endif
