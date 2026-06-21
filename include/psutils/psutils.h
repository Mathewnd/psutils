#ifndef PSUTILS_PSUTILS_H
#define PSUTILS_PSUTILS_H

#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>
#include <time.h>

#define PSUTILS_PROCESS_NAME_SIZE 64
#define PSUTILS_PROCESS_TTY_NAME_SIZE 64
#define PSUTILS_PROCESS_COMMAND_SIZE 256

typedef struct {
	pid_t pid;
	pid_t ppid;
	pid_t pgid;
	pid_t sid;
	uid_t uid;
	uid_t euid;
	uid_t suid;
	gid_t gid;
	gid_t egid;
	gid_t sgid;
	size_t thread_count;
	char name[PSUTILS_PROCESS_NAME_SIZE];
	char command[PSUTILS_PROCESS_COMMAND_SIZE];
	char tty_name[PSUTILS_PROCESS_TTY_NAME_SIZE];
	struct timespec start_timestamp;
	struct timespec total_runtime;
	int state;
	int cpu_time;
	int nice;
	size_t virtual_pages;
	size_t physical_pages;
} psutils_process_t;

typedef bool (*psutils_process_filter_t)(const psutils_process_t *process);

int psutils_init(void);
int psutils_get_processes(psutils_process_t *table, size_t *process_count, psutils_process_filter_t filter);

#endif
