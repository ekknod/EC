/*
 * =====================================================================================
 *
 *       Filename:  rx_list.c
 *
 *    Description:  process / process library listing
 *
 *        Version:  1.0
 *        Created:  27.09.2018 16:44:16
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  github.com/ekknod 
 *   Organization:  
 *
 * =====================================================================================
 */

#include "../include/rx_list.h"
#include <dirent.h>
#include <string.h>
#include <unistd.h>

struct list_parameters
{
	RX_SNAP_TYPE type;
	int pid;
};

struct rx_list
{
	DIR *directory;
	int file;
	int pid;
	int type;
};

#define PID_MAX 2147483647

static int
snap_create(rx_handle snapshot, void *parameters);

static void
    snap_destroy(rx_handle);

rx_handle
rx_create_snapshot(
    _in_ RX_SNAP_TYPE type,
    _in_opt_ int pid)
{
	struct list_parameters parameters;

	parameters.type = type;
	parameters.pid = pid;
	return rx_initialize_object(snap_create, snap_destroy, &parameters, sizeof(struct rx_list));
}

extern int snprintf(char *s, size_t n, const char *format, ...);
extern int atoi(const char *nptr);
extern ssize_t readlink(const char *path, char *buf, size_t bufsiz);

#include <stdio.h>
rx_bool
rx_next_process(
    _in_ rx_handle snapshot,
    _out_ PRX_PROCESS_ENTRY entry)
{
	struct rx_list *self = snapshot;
	struct dirent *a0;
	ssize_t a1;

	while ((a0 = readdir(self->directory)))
	{
		if (a0->d_type != 4)
			continue;

		snprintf(entry->exe, sizeof(entry->exe), "/proc/%s/exe", a0->d_name);

		memset(entry->data, 0, sizeof(entry->data));
		if ((a1 = readlink(entry->exe, entry->data, sizeof(entry->data))) == -1)
			continue;

		entry->path = entry->data;
		entry->name = strrchr(entry->path, '/');
		if (entry->name++ == 0)
			continue;

		entry->pid = atoi(a0->d_name);

		if (entry->pid < 1 || entry->pid > PID_MAX)
			continue;

		return 1;
	}
	return 0;
}

static size_t
read_line(struct rx_list *snapshot, char *buffer, size_t length)
{
	size_t pos = 0;

	while (--length > 0 && read(snapshot->file, &buffer[pos], 1))
	{
		if (buffer[pos] == '\n')
		{
			buffer[pos] = '\0';
			return pos;
		}
		pos++;
	}
	return 0;
}

extern unsigned long int strtoul(const char *str, char **endptr, int base);

rx_bool
rx_next_library(
    _in_ rx_handle snapshot,
    _out_ PRX_LIBRARY_ENTRY entry)
{
	struct rx_list *self = snapshot;
	char *a0;

	while (read_line(self, entry->data, sizeof(entry->data)))
	{
		entry->path = strchr(entry->data, '/');
		entry->name = strrchr(entry->data, '/');
		a0 = entry->data;
		entry->start = strtoul(entry->data, (char **)&a0, 16);
		entry->end = strtoul(a0 + 1, (char **)&a0, 16);
		entry->pid = self->pid;
		if (entry->path == 0 || entry->name++ == 0 || entry->end < entry->start)
			continue;
		return 1;
	}
	return 0;
}

extern int open(const char *path, int oflag, ...);

static int
snap_create(rx_handle snapshot, void *parameters)
{
	struct rx_list *self = snapshot;
	struct list_parameters *params = parameters;
	char a0[32];

	self->type = params->type;
	switch (self->type)
	{
	case RX_SNAP_TYPE_PROCESS:
		if ((self->directory = opendir("/proc/")) == 0)
			return -1;
		break;
	case RX_SNAP_TYPE_LIBRARY:

		snprintf(a0, sizeof(a0), "/proc/%d/maps", params->pid);
		self->file = open(a0, RX_READ_ACCESS);
		self->pid = params->pid;
		if (self->file == -1)
			return -1;
		break;
	default:
		return -1;
	}
	return 0;
}

static void
snap_destroy(rx_handle snapshot)
{
	struct rx_list *self = snapshot;
	switch (self->type)
	{
	case RX_SNAP_TYPE_PROCESS:
		closedir(self->directory);
		break;
	case RX_SNAP_TYPE_LIBRARY:
		close(self->file);
		break;
	default:
		break;
	}
}
