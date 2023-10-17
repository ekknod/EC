#include "../../cs2/shared/shared.h"

#include <sys/time.h>
#include <linux/types.h>
#include <linux/input-event-codes.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>

static int fd;

struct input_event
{
	struct timeval time;
	__u16 type, code;
	__s32 value;
};

static ssize_t
send_input(int dev, __u16 type, __u16 code, __s32 value)
{
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
	wfix = write(dev, &start, sizeof(start));
	wfix = write(dev, &end, sizeof(end));
	return wfix;
}

static int open_device(const char *name, size_t length)
{
	int fd = -1;
	DIR *d = opendir("/dev/input/by-id/");
	struct dirent *e;
	char p[512];

	while ((e = readdir(d)))
	{
		if (e->d_type != 10)
			continue;
		if (strcmp(strrchr(e->d_name, '\0') - length, name) == 0)
		{
			snprintf(p, sizeof(p), "/dev/input/by-id/%s", e->d_name);
			fd = open(p, O_RDWR);
			break;
		}
	}
	closedir(d);
	return fd;
}

namespace client
{
	void mouse_move(int x, int y)
	{
		send_input(fd, EV_REL, 0, x);
		send_input(fd, EV_REL, 1, y);
	}

	void mouse1_down(void)
	{
		send_input(fd, EV_KEY, 0x110, 1);
	}

	void mouse1_up(void)
	{
		send_input(fd, EV_KEY, 0x110, 0);
	}

	void DrawRect(void *hwnd, LONG x, LONG y, LONG w, LONG h, unsigned char r, unsigned char g, unsigned b)
	{
	}

	void DrawFillRect(VOID *hwnd, LONG x, LONG y, LONG w, LONG h, unsigned char r, unsigned char g, unsigned b)
	{
	}
}

int main(void)
{
	fd = open_device("event-mouse", 11);
	if (fd == -1)
	{
		LOG("failed to open mouse\n");
		return 0;
	}

	while (1)
	{
		cs2::run();
	}
	return 0;
}

