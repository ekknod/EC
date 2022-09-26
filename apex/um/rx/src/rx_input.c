/*
 * =====================================================================================
 *
 *       Filename:  rx_input.c
 *
 *    Description:  handles keyboard/mouse
 *
 *        Version:  1.0
 *        Created:  27.09.2018 17:00:47
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  github.com/ekknod 
 *   Organization:  
 *
 * =====================================================================================
 */

#include "../include/rx_input.h"
#include <sys/time.h>
#include <linux/types.h>
#include <linux/input-event-codes.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>

extern int snprintf(char *s, size_t n, const char *format, ...);

struct input_event
{
	struct timeval time;
	__u16 type, code;
	__s32 value;
};

struct input_parameters
{
	RX_INPUT_TYPE type;
	RX_INPUT_MODE mode;
};

struct rx_input
{
	int fd;
	pthread_t thread;
	rx_bool keys[RX_KEYCODE_LAST];
	vec2_i axis;
};

static ssize_t
send_input(rx_handle input, __u16 type, __u16 code, __s32 value);

static int
open_input(rx_handle, void *);

static void
    close_input(rx_handle);

rx_handle
rx_open_input(
    _in_ RX_INPUT_TYPE type,
    _in_ RX_INPUT_MODE mode)
{
	struct input_parameters parameters;

	parameters.type = type;
	parameters.mode = mode;
	return rx_initialize_object(open_input, close_input, &parameters, sizeof(struct rx_input));
}

rx_bool
rx_key_down(
    _in_ rx_handle input,
    _in_ RX_KEYCODE key)
{
	struct rx_input *self = input;
	return self->keys[key];
}

vec2_i
rx_input_axis(
    _in_ rx_handle mouse_input)
{
	struct rx_input *self = mouse_input;
	return self->axis;
}

void rx_send_input_axis(
    _in_ rx_handle mouse_input,
    _in_ RX_MOUSE_AXIS axis,
    _in_ int px)
{
	send_input(mouse_input, EV_REL, axis, px);
}

void rx_send_input_key(
    _in_ rx_handle input,
    _in_ RX_KEYCODE key,
    _in_ rx_bool down)
{
	send_input(input, EV_KEY, key, (__s32)down);
}

static ssize_t
send_input(rx_handle input, __u16 type, __u16 code, __s32 value)
{
	struct rx_input *self = input;
	struct input_event start, end;
	ssize_t wfix;

	gettimeofday(&start.time, 0);
	start.type = type;
	start.code = code;
	start.value = value;

	gettimeofday(&end.time, 0);
	end.type = EV_SYN;
	end.code = SYN_REPORT;
	end.value = 0;
	wfix = write(self->fd, &start, sizeof(start));
	wfix = write(self->fd, &end, sizeof(end));
	return wfix;
}

static int
open_device(const char *name, size_t length, int access_mask)
{
	int fd = -1;
	DIR *d = opendir("/dev/input/by-id/");
	struct dirent *e;
	char p[260];

	while ((e = readdir(d)))
	{
		if (e->d_type != 10)
			continue;
		if (strcmp(strrchr(e->d_name, '\0') - length, name) == 0)
		{
			snprintf(p, sizeof(p), "/dev/input/by-id/%s", e->d_name);
			fd = open(p, access_mask);
			break;
		}
	}
	closedir(d);
	return fd;
}

static void *
mouse_thread(void *input)
{
	struct rx_input *self = input;
	struct input_event event;

	while (1)
	{
		if (read(self->fd, &event, sizeof(event)) == -1)
			continue;
		switch (event.type)
		{
		case EV_REL:
			((int *)(&self->axis))[event.code] = event.value;
			break;
		case EV_KEY:
			self->keys[event.code] = (rx_bool)event.value;
			break;
		default:
			break;
		}
	}
	return 0;
}

static void *
keyboard_thread(void *input)
{
	struct rx_input *self = input;
	struct input_event event;

	while (1)
	{
		if (read(self->fd, &event, sizeof(event)) == -1)
			continue;
		if (event.type == EV_KEY)
		{
			self->keys[event.code] = (rx_bool)event.value;
		}
	}
	return 0;
}

static int
open_input(rx_handle input, void *parameters)
{
	int status = -1;
	struct rx_input *self = input;
	struct input_parameters *params = parameters;
	static void *(*thread)(void *);

	switch (params->type)
	{
	case RX_INPUT_TYPE_MOUSE:
		self->fd = open_device("event-mouse", 11, params->mode);
		thread = mouse_thread;
		break;
	case RX_INPUT_TYPE_KEYBOARD:
		self->fd = open_device("event-kbd", 9, params->mode);
		thread = keyboard_thread;
		break;
	default:
		goto end;
	}

	status = self->fd;
	if (status == -1)
		goto end;

	if (params->mode == RX_INPUT_MODE_SEND)
		self->thread = 0;
	else
		pthread_create(&self->thread, 0, thread, input);

	memset(self->keys, 0, sizeof(self->keys));
	memset(&self->axis, 0, sizeof(self->axis));
end:
	return status;
}

static void
close_input(rx_handle input)
{
	struct rx_input *self = input;
	if (self->thread)
	{
		pthread_cancel(self->thread);
	}
	close(self->fd);
}
