#include "print.h"

#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef enum {
	FIELD_FLAGS,
	FIELD_STATE,
	FIELD_UID,
	FIELD_PID,
	FIELD_PPID,
	FIELD_CPU,
	FIELD_PRIORITY,
	FIELD_NICE,
	FIELD_ADDR,
	FIELD_SIZE,
	FIELD_WCHAN,
	FIELD_STIME,
	FIELD_TTY,
	FIELD_TIME,
	FIELD_CMD,
	FIELD_RUSER,
	FIELD_USER,
	FIELD_RGROUP,
	FIELD_GROUP,
	FIELD_PGID,
	FIELD_PCPU,
	FIELD_VSZ,
	FIELD_ETIME,
	FIELD_COMM,
	FIELD_ARGS,
} field_t;

typedef struct {
	field_t field;
	const char *header;
} column_t;

typedef struct {
	const char *name;
	const char *header;
	field_t field;
} field_spec_t;

static const column_t default_columns[] = {
	{FIELD_PID, "PID"},
	{FIELD_TTY, "TTY"},
	{FIELD_TIME, "TIME"},
	{FIELD_CMD, "CMD"},
};

static const column_t full_columns[] = {
	{FIELD_UID, "UID"},
	{FIELD_PID, "PID"},
	{FIELD_PPID, "PPID"},
	{FIELD_CPU, "C"},
	{FIELD_STIME, "STIME"},
	{FIELD_TTY, "TTY"},
	{FIELD_TIME, "TIME"},
	{FIELD_CMD, "CMD"},
};

static const column_t long_columns[] = {
	{FIELD_FLAGS, "F"},
	{FIELD_STATE, "S"},
	{FIELD_UID, "UID"},
	{FIELD_PID, "PID"},
	{FIELD_PPID, "PPID"},
	{FIELD_CPU, "C"},
	{FIELD_PRIORITY, "PRI"},
	{FIELD_NICE, "NI"},
	{FIELD_ADDR, "ADDR"},
	{FIELD_SIZE, "SZ"},
	{FIELD_WCHAN, "WCHAN"},
	{FIELD_TTY, "TTY"},
	{FIELD_TIME, "TIME"},
	{FIELD_CMD, "CMD"},
};

static const column_t full_long_columns[] = {
	{FIELD_FLAGS, "F"},
	{FIELD_STATE, "S"},
	{FIELD_UID, "UID"},
	{FIELD_PID, "PID"},
	{FIELD_PPID, "PPID"},
	{FIELD_CPU, "C"},
	{FIELD_PRIORITY, "PRI"},
	{FIELD_NICE, "NI"},
	{FIELD_ADDR, "ADDR"},
	{FIELD_SIZE, "SZ"},
	{FIELD_WCHAN, "WCHAN"},
	{FIELD_STIME, "STIME"},
	{FIELD_TTY, "TTY"},
	{FIELD_TIME, "TIME"},
	{FIELD_CMD, "CMD"},
};

static const field_spec_t field_specs[] = {
	{"ruser", "RUSER", FIELD_RUSER},
	{"user", "USER", FIELD_USER},
	{"rgroup", "RGROUP", FIELD_RGROUP},
	{"group", "GROUP", FIELD_GROUP},
	{"pid", "PID", FIELD_PID},
	{"ppid", "PPID", FIELD_PPID},
	{"pgid", "PGID", FIELD_PGID},
	{"pcpu", "%CPU", FIELD_PCPU},
	{"vsz", "VSZ", FIELD_VSZ},
	{"nice", "NI", FIELD_NICE},
	{"etime", "ELAPSED", FIELD_ETIME},
	{"time", "TIME", FIELD_TIME},
	{"tty", "TT", FIELD_TTY},
	{"comm", "COMMAND", FIELD_COMM},
	{"args", "COMMAND", FIELD_ARGS},
};

static void free_columns(column_t *columns, size_t column_count) {
	for (size_t i = 0; i < column_count; ++i)
		free((char *)columns[i].header);

	free(columns);
}

static int append_column(column_t **columns, size_t *column_count, field_t field, const char *header) {
	char *header_copy = strdup(header);
	if (header_copy == NULL)
		return ENOMEM;

	column_t *new_columns = realloc(*columns, (*column_count + 1) * sizeof(**columns));
	if (new_columns == NULL) {
		free(header_copy);
		return ENOMEM;
	}

	*columns = new_columns;
	(*columns)[*column_count].field = field;
	(*columns)[*column_count].header = header_copy;
	++*column_count;
	return 0;
}

static int copy_columns(column_t **columns, size_t *column_count, const column_t *src, size_t src_count) {
	for (size_t i = 0; i < src_count; ++i) {
		int error = append_column(columns, column_count, src[i].field, src[i].header);
		if (error)
			return error;
	}

	return 0;
}

static const field_spec_t *find_field_spec(const char *name) {
	for (size_t i = 0; i < sizeof(field_specs) / sizeof(*field_specs); ++i) {
		if (strcmp(field_specs[i].name, name) == 0)
			return &field_specs[i];
	}

	return NULL;
}

static int append_format_column(column_t **columns, size_t *column_count, char *token) {
	char *header = strchr(token, '=');
	if (header) {
		*header = 0;
		++header;
	}

	const field_spec_t *spec = find_field_spec(token);
	if (spec == NULL) {
		fprintf(stderr, "ps: unknown format specifier '%s'\n", token);
		return EINVAL;
	}

	const char *header_value = header ? header : spec->header;
	return append_column(columns, column_count, spec->field, header_value);
}

static int parse_format_argument(column_t **columns, size_t *column_count, const char *format) {
	char *copy = strdup(format);
	if (copy == NULL)
		return ENOMEM;

	if (!*copy) {
		fprintf(stderr, "ps: empty format specifier\n");
		free(copy);
		return EINVAL;
	}

	char *token = copy;
	while (*token) {
		char *p = token;
		while (*p && *p != ',' && *p != ' ' && *p != '\t' && *p != '=')
			++p;

		if (*p == '=') {
			int error = append_format_column(columns, column_count, token);
			free(copy);
			return error;
		}

		char saved = *p;
		*p = 0;
		if (!*token) {
			fprintf(stderr, "ps: empty format specifier\n");
			free(copy);
			return EINVAL;
		}

		int error = append_format_column(columns, column_count, token);
		if (error) {
			free(copy);
			return error;
		}

		if (!saved)
			break;

		token = p + 1;
		if (!*token) {
			fprintf(stderr, "ps: empty format specifier\n");
			free(copy);
			return EINVAL;
		}
	}

	free(copy);
	return 0;
}

static int build_columns(const struct ps_options *opts, column_t **columns, size_t *column_count) {
	if (opts->formats_count) {
		for (size_t i = 0; i < opts->formats_count; ++i) {
			int error = parse_format_argument(columns, column_count, opts->formats[i]);
			if (error) {
				free_columns(*columns, *column_count);
				return error;
			}
		}

		return 0;
	}

	int error;
	if (opts->full_listing && opts->long_listing)
		error = copy_columns(columns, column_count, full_long_columns, sizeof(full_long_columns) / sizeof(*full_long_columns));
	else if (opts->full_listing)
		error = copy_columns(columns, column_count, full_columns, sizeof(full_columns) / sizeof(*full_columns));
	else if (opts->long_listing)
		error = copy_columns(columns, column_count, long_columns, sizeof(long_columns) / sizeof(*long_columns));
	else
		error = copy_columns(columns, column_count, default_columns, sizeof(default_columns) / sizeof(*default_columns));

	if (error)
		free_columns(*columns, *column_count);

	return error;
}

static const char *user_name(uid_t uid, char *buffer, size_t buffer_size) {
	struct passwd *passwd = getpwuid(uid);
	if (passwd && passwd->pw_name)
		return passwd->pw_name;

	snprintf(buffer, buffer_size, "%ju", (uintmax_t)uid);
	return buffer;
}

static const char *group_name(gid_t gid, char *buffer, size_t buffer_size) {
	struct group *group = getgrgid(gid);
	if (group && group->gr_name)
		return group->gr_name;

	snprintf(buffer, buffer_size, "%ju", (uintmax_t)gid);
	return buffer;
}

static void format_duration(long seconds, char *buffer, size_t buffer_size) {
	long days = seconds / 86400;
	seconds %= 86400;
	long hours = seconds / 3600;
	seconds %= 3600;
	long minutes = seconds / 60;
	seconds %= 60;

	if (days)
		snprintf(buffer, buffer_size, "%ld-%02ld:%02ld:%02ld", days, hours, minutes, seconds);
	else
		snprintf(buffer, buffer_size, "%02ld:%02ld:%02ld", hours, minutes, seconds);
}

static int get_uptime(long *uptime) {
	struct timespec now;

	*uptime = 0;
	if (clock_gettime(CLOCK_BOOTTIME, &now) < 0)
		return errno;

	*uptime = now.tv_sec;
	return 0;
}

static void format_elapsed(const psutils_process_t *process, char *buffer, size_t buffer_size) {
	long uptime;
	if (get_uptime(&uptime)) {
		snprintf(buffer, buffer_size, "-");
		return;
	}

	long elapsed = uptime > process->start_timestamp.tv_sec ? uptime - process->start_timestamp.tv_sec : 0;
	long days = elapsed / 86400;
	elapsed %= 86400;
	long hours = elapsed / 3600;
	elapsed %= 3600;
	long minutes = elapsed / 60;
	elapsed %= 60;

	if (days)
		snprintf(buffer, buffer_size, "%ld-%02ld:%02ld:%02ld", days, hours, minutes, elapsed);
	else if (hours)
		snprintf(buffer, buffer_size, "%02ld:%02ld:%02ld", hours, minutes, elapsed);
	else
		snprintf(buffer, buffer_size, "%02ld:%02ld", minutes, elapsed);
}

static void format_stime(const psutils_process_t *process, char *buffer, size_t buffer_size) {
	long uptime;
	if (get_uptime(&uptime)) {
		snprintf(buffer, buffer_size, "-");
		return;
	}

	time_t start = time(NULL) - uptime + process->start_timestamp.tv_sec;
	struct tm tm;

	if (localtime_r(&start, &tm) == NULL) {
		snprintf(buffer, buffer_size, "-");
		return;
	}

	strftime(buffer, buffer_size, "%H:%M", &tm);
}

static char state_char(int state) {
	(void)state;
	return '-';
}

static void field_value(const psutils_process_t *process, field_t field, char *buffer, size_t buffer_size) {
	char tmp[64];

	switch (field) {
	case FIELD_FLAGS:
	case FIELD_PRIORITY:
	case FIELD_ADDR:
	case FIELD_WCHAN:
	case FIELD_PCPU:
		snprintf(buffer, buffer_size, "-");
		break;
	case FIELD_STATE:
		snprintf(buffer, buffer_size, "%c", state_char(process->state));
		break;
	case FIELD_UID:
	case FIELD_USER:
		snprintf(buffer, buffer_size, "%s", user_name(process->euid, tmp, sizeof(tmp)));
		break;
	case FIELD_RUSER:
		snprintf(buffer, buffer_size, "%s", user_name(process->uid, tmp, sizeof(tmp)));
		break;
	case FIELD_GROUP:
		snprintf(buffer, buffer_size, "%s", group_name(process->egid, tmp, sizeof(tmp)));
		break;
	case FIELD_RGROUP:
		snprintf(buffer, buffer_size, "%s", group_name(process->gid, tmp, sizeof(tmp)));
		break;
	case FIELD_PID:
		snprintf(buffer, buffer_size, "%d", process->pid);
		break;
	case FIELD_PPID:
		snprintf(buffer, buffer_size, "%d", process->ppid);
		break;
	case FIELD_PGID:
		snprintf(buffer, buffer_size, "%d", process->pgid);
		break;
	case FIELD_CPU:
		snprintf(buffer, buffer_size, "%d", process->cpu_time);
		break;
	case FIELD_NICE:
		snprintf(buffer, buffer_size, "%d", process->nice);
		break;
	case FIELD_SIZE:
		snprintf(buffer, buffer_size, "%zu", process->virtual_pages);
		break;
	case FIELD_VSZ:
		snprintf(buffer, buffer_size, "%zu", process->virtual_pages * 4);
		break;
	case FIELD_STIME:
		format_stime(process, buffer, buffer_size);
		break;
	case FIELD_TTY:
		snprintf(buffer, buffer_size, "%s", process->tty_name[0] ? process->tty_name : "?");
		break;
	case FIELD_TIME:
		format_duration(process->total_runtime.tv_sec, buffer, buffer_size);
		break;
	case FIELD_ETIME:
		format_elapsed(process, buffer, buffer_size);
		break;
	case FIELD_CMD:
	case FIELD_COMM:
		snprintf(buffer, buffer_size, "%s", process->name);
		break;
	case FIELD_ARGS:
		if (process->command[0])
			snprintf(buffer, buffer_size, "%s", process->command);
		else
			snprintf(buffer, buffer_size, "[%s]", process->name);
		break;
	}
}

static bool all_headers_empty(const column_t *columns, size_t column_count) {
	for (size_t i = 0; i < column_count; ++i) {
		if (columns[i].header[0])
			return false;
	}

	return true;
}

static void print_row_separator(size_t column, size_t column_count) {
	if (column + 1 < column_count)
		putchar(' ');
}

int print_process_table(const struct ps_options *opts, const psutils_process_t *table, size_t count) {
	column_t *columns = NULL;
	size_t column_count = 0;
	int error = build_columns(opts, &columns, &column_count);
	if (error)
		return error;

	size_t *widths = calloc(column_count, sizeof(*widths));
	if (widths == NULL) {
		free_columns(columns, column_count);
		return ENOMEM;
	}

	for (size_t i = 0; i < column_count; ++i)
		widths[i] = strlen(columns[i].header);

	char value[256];
	for (size_t i = 0; i < count; ++i) {
		for (size_t j = 0; j < column_count; ++j) {
			field_value(&table[i], columns[j].field, value, sizeof(value));
			size_t len = strlen(value);
			if (len > widths[j])
				widths[j] = len;
		}
	}

	if (!all_headers_empty(columns, column_count)) {
		for (size_t i = 0; i < column_count; ++i) {
			printf("%-*s", (int)widths[i], columns[i].header);
			print_row_separator(i, column_count);
		}
		putchar('\n');
	}

	for (size_t i = 0; i < count; ++i) {
		for (size_t j = 0; j < column_count; ++j) {
			field_value(&table[i], columns[j].field, value, sizeof(value));
			printf("%-*s", (int)widths[j], value);
			print_row_separator(j, column_count);
		}
		putchar('\n');
	}

	free(widths);
	free_columns(columns, column_count);
	return 0;
}
