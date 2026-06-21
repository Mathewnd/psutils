#ifndef PS_OPTIONS_H
#define PS_OPTIONS_H

#include <stddef.h>
#include <sys/types.h>

struct ps_options {
	int all_processes;
	int terminal_processes;
	int exclude_session_leaders;
	int full_listing;
	int long_listing;
	int has_selection;
	gid_t *groups;
	size_t groups_count;
	gid_t *real_groups;
	size_t real_groups_count;
	char **names;
	size_t names_count;
	pid_t *processes;
	size_t processes_count;
	char **terminals;
	size_t terminals_count;
	uid_t *users;
	size_t users_count;
	uid_t *real_users;
	size_t real_users_count;
	const char **formats;
	size_t formats_count;
};

int parse_options(int argc, char **argv, struct ps_options *opts);
void free_options(struct ps_options *opts);

#endif
