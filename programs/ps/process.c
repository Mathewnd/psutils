#include "process.h"

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
	const struct ps_options *opts;
	uid_t euid;
	char tty_name[PSUTILS_PROCESS_TTY_NAME_SIZE];
} process_filter_context_t;

static bool pid_in_list(pid_t pid, const pid_t *list, size_t count) {
	for (size_t i = 0; i < count; ++i) {
		if (list[i] == pid)
			return true;
	}

	return false;
}

static bool uid_in_list(uid_t uid, const uid_t *list, size_t count) {
	for (size_t i = 0; i < count; ++i) {
		if (list[i] == uid)
			return true;
	}

	return false;
}

static bool gid_in_list(gid_t gid, const gid_t *list, size_t count) {
	for (size_t i = 0; i < count; ++i) {
		if (list[i] == gid)
			return true;
	}

	return false;
}

static bool terminal_matches(const char *process_tty, const char *terminal) {
	if (strcmp(process_tty, terminal) == 0)
		return true;

	if (strncmp(process_tty, "tty", 3) == 0 && strcmp(process_tty + 3, terminal) == 0)
		return true;

	return false;
}

static bool terminal_in_list(const char *process_tty, char *const *terminals, size_t count) {
	for (size_t i = 0; i < count; ++i) {
		if (terminal_matches(process_tty, terminals[i]))
			return true;
	}

	return false;
}

static bool default_match(const psutils_process_t *process, void *context_ptr) {
	const process_filter_context_t *context = context_ptr;

	return process->euid == context->euid && strcmp(process->tty_name, context->tty_name) == 0;
}

static bool selection_match(const psutils_process_t *process, void *context_ptr) {
	const process_filter_context_t *context = context_ptr;
	const struct ps_options *opts = context->opts;

	if (opts->all_processes)
		return true;

	if (opts->terminal_processes && process->tty_name[0])
		return true;

	if (opts->exclude_session_leaders && process->pid != process->sid)
		return true;

	if (pid_in_list(process->sid, opts->session_leaders, opts->session_leaders_count))
		return true;

	if (gid_in_list(process->gid, opts->real_groups, opts->real_groups_count))
		return true;

	if (pid_in_list(process->pid, opts->processes, opts->processes_count))
		return true;

	if (terminal_in_list(process->tty_name, opts->terminals, opts->terminals_count))
		return true;

	if (uid_in_list(process->euid, opts->users, opts->users_count))
		return true;

	if (uid_in_list(process->uid, opts->real_users, opts->real_users_count))
		return true;

	return false;
}

int get_process_table(const struct ps_options *opts, psutils_process_t **table, size_t *count) {
	process_filter_context_t context = {
		.opts = opts,
		.euid = geteuid(),
	};
	psutils_process_filter_t filter = opts->has_selection ? selection_match : default_match;
	int error;

	if (!opts->has_selection) {
		error = psutils_get_current_tty_name(context.tty_name, sizeof(context.tty_name));
		if (error)
			return error;
	}

	size_t matched_count = 0;
	error = psutils_get_processes(NULL, &matched_count, filter, &context);
	if (error)
		return error;

	if (matched_count == 0)
		return 0;

	*table = malloc(matched_count * sizeof(**table));
	if (*table == NULL)
		return ENOMEM;

	*count = matched_count;
	error = psutils_get_processes(*table, count, filter, &context);
	if (error) {
		free(*table);
		return error;
	}

	return 0;
}
