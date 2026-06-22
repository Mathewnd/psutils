#include "sysdeps.h"

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <astral/sysctl.h>

int psutils_sysdep_init(void) {
	return 0;
}

static void copy_string(char *dst, size_t dst_size, const char *src, size_t src_size) {
	size_t len = strnlen(src, src_size);

	if (len >= dst_size)
		len = dst_size - 1;

	memcpy(dst, src, len);
	dst[len] = 0;
}

static void copy_process(psutils_process_t *dst, const sysctl_proc_info_t *src) {
	dst->pid = src->pid;
	dst->ppid = src->ppid;
	dst->pgid = src->pgid;
	dst->sid = src->sid;
	dst->uid = src->uid;
	dst->euid = src->euid;
	dst->suid = src->suid;
	dst->gid = src->gid;
	dst->egid = src->egid;
	dst->sgid = src->sgid;
	dst->thread_count = src->thread_count;
	copy_string(dst->name, sizeof(dst->name), src->name, sizeof(src->name));
	copy_string(dst->tty_name, sizeof(dst->tty_name), src->tty_name, sizeof(src->tty_name));
	/* TODO: Populate argv/command once the kernel exposes process arguments. */
	dst->command[0] = 0;
	dst->start_timestamp = src->start_timestamp;
	dst->total_runtime = src->total_runtime;
	dst->state = src->state;
	dst->cpu_time = src->cpu_time;
	dst->nice = src->nice;
	dst->virtual_pages = src->virtual_pages;
	dst->physical_pages = src->physical_pages;
}

int psutils_sysdep_get_processes(psutils_process_t *table, size_t *table_size) {
	int name[6] = {
		SYS_CTL_KERN,
		SYS_CTL_KERN_PROC,
		SYS_CTL_KERN_PROC_ALL,
		0,
		sizeof(sysctl_proc_info_t),
		0
	};
	sysctl_proc_info_t *sysctl_table;
	size_t sysctl_size;
	size_t count;

	if (table == NULL) {
		if (sysctl(name, 6, NULL, &sysctl_size, NULL, 0) < 0)
			return errno;

		*table_size = sysctl_size / sizeof(sysctl_proc_info_t);
		return 0;
	}

	sysctl_size = *table_size * sizeof(sysctl_proc_info_t);
	sysctl_table = malloc(sysctl_size);
	if (sysctl_table == NULL)
		return ENOMEM;

	if (sysctl(name, 6, sysctl_table, &sysctl_size, NULL, 0) < 0) {
		int error = errno;
		free(sysctl_table);
		return error;
	}

	count = sysctl_size / sizeof(*sysctl_table);
	for (size_t i = 0; i < count; ++i)
		copy_process(&table[i], &sysctl_table[i]);

	*table_size = count;
	free(sysctl_table);
	return 0;
}
