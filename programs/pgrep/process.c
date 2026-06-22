#include "process.h"

#include <string.h>
#include <time.h>

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

static int timespec_compare(struct timespec a, struct timespec b) {
	if (a.tv_sec < b.tv_sec)
		return -1;
	if (a.tv_sec > b.tv_sec)
		return 1;
	if (a.tv_nsec < b.tv_nsec)
		return -1;
	if (a.tv_nsec > b.tv_nsec)
		return 1;

	return 0;
}

static const char *match_text(const struct pgrep_options *opts, const psutils_process_t *process) {
	if (!opts->full_match)
		return process->name;

	if (process->command[0])
		return process->command;

	return process->name;
}

static bool attributes_match(const struct pgrep_options *opts, const psutils_process_t *process) {
	if (opts->parents_count && !pid_in_list(process->ppid, opts->parents, opts->parents_count))
		return false;

	if (opts->process_groups_count && !pid_in_list(process->pgid, opts->process_groups, opts->process_groups_count))
		return false;

	if (opts->sessions_count && !pid_in_list(process->sid, opts->sessions, opts->sessions_count))
		return false;

	if (opts->effective_users_count && !uid_in_list(process->euid, opts->effective_users, opts->effective_users_count))
		return false;

	if (opts->real_users_count && !uid_in_list(process->uid, opts->real_users, opts->real_users_count))
		return false;

	if (opts->real_groups_count && !gid_in_list(process->gid, opts->real_groups, opts->real_groups_count))
		return false;

	if (opts->terminals_count && !terminal_in_list(process->tty_name, opts->terminals, opts->terminals_count))
		return false;

	return true;
}

static bool pattern_match(const struct pgrep_options *opts, const regex_t *regex, const psutils_process_t *process) {
	if (opts->pattern == NULL)
		return true;

	const char *text = match_text(opts, process);
	if (opts->exact)
		return strcmp(text, opts->pattern) == 0;

	return regexec(regex, text, 0, NULL, 0) == 0;
}

static bool process_matches(const struct pgrep_options *opts, const regex_t *regex, const psutils_process_t *process) {
	bool matches = attributes_match(opts, process) && pattern_match(opts, regex, process);

	return opts->inverse ? !matches : matches;
}

bool pgrep_process_filter(const psutils_process_t *process, void *context_ptr) {
	const struct pgrep_process_filter_context *context = context_ptr;

	if (process->pid == context->self)
		return false;

	return process_matches(context->opts, context->regex, process);
}

bool pgrep_select_by_age(const struct pgrep_options *opts, const psutils_process_t *candidate, const psutils_process_t *selected) {
	int cmp = timespec_compare(candidate->start_timestamp, selected->start_timestamp);

	if (opts->newest)
		return cmp > 0;

	if (opts->oldest)
		return cmp < 0;

	return false;
}
