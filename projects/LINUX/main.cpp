#include "../../cs2/shared/cs2game.h"
// #include "../../csgo/shared/csgogame.h"
#include "../../apex/shared/apexgame.h"

#include <sys/time.h>
#include <linux/types.h>
#include <linux/input-event-codes.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>

#include "../../library/SDL3/include/SDL3/SDL.h"

static SDL_Renderer* renderer;


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

	void DrawRect(void *hwnd, int x, int y, int w, int h, unsigned char r, unsigned char g, unsigned char b)
	{
		SDL_FRect rect{};
		rect.x = (float)x;
		rect.y = (float)y;
		rect.w = (float)w;
		rect.h = (float)h;
		SDL_SetRenderDrawColor(renderer, r, g, b, 50);
		SDL_RenderFillRect(renderer, &rect);

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderRect(renderer, &rect);
	}

	void DrawFillRect(void *hwnd, int x, int y, int w, int h, unsigned char r, unsigned char g, unsigned char b)
	{
		SDL_FRect rect{};
		rect.x = (float)x;
		rect.y = (float)y;
		rect.w = (float)w;
		rect.h = (float)h;
		SDL_SetRenderDrawColor(renderer, r, g, b, 255);
		SDL_RenderFillRect(renderer, &rect);
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

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		return 0;
	}
	
	SDL_Window *temp = SDL_CreateWindow("EC", 1, 1, SDL_WINDOW_MINIMIZED | SDL_WINDOW_OPENGL);
	if (temp == NULL)
	{
		return 0;
	}
	SDL_DisplayID id = SDL_GetDisplayForWindow(temp);
	SDL_SetWindowPosition(temp, 0, 0);

	int total_displays = 0;
	SDL_DisplayID* display_ids = SDL_GetDisplays(&total_displays);

	int display_min_x = INT_MAX;
	int display_min_y = INT_MAX;
	int display_max_x = 0;
	int display_max_y = 0;
	for (int i = 0; i < total_displays && display_ids[i] != 0; i++)
	{
		SDL_Rect rect;
		if (SDL_GetDisplayBounds(display_ids[i], &rect) == 0) {
			display_min_x = SDL_min(display_min_x, rect.x);
			display_min_y = SDL_min(display_min_y, rect.y);
			display_max_x = SDL_max(display_max_x, rect.x + rect.w);
			display_max_y = SDL_max(display_max_y, rect.y + rect.h);
		}
	}
	if (display_min_x == INT_MAX) {
		return 0;
	}
	SDL_free(display_ids);

	SDL_Window *window = SDL_CreatePopupWindow(temp, 0, 0, 640, 480,
		SDL_WINDOW_TRANSPARENT | SDL_WINDOW_BORDERLESS | SDL_WINDOW_TOOLTIP);
		
	SDL_SetWindowSize(window, display_max_x - display_min_x, display_max_y - display_min_y);
	SDL_SetWindowPosition(window, display_min_x, display_min_y);

	SDL_SetWindowAlwaysOnTop(window, SDL_TRUE);

	renderer = SDL_CreateRenderer(window, 0, SDL_RENDERER_PRESENTVSYNC);
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

	BOOL quit = 0;
	while (!quit)
	{
		SDL_Event e;
		while ( SDL_PollEvent( &e ) )
		{
			if (e.type == SDL_EVENT_QUIT)
			{
				quit = 1;
				break;
			}
		}

		SDL_RenderClear(renderer);

		if (cs2::game_handle)
		{
			if (cs2::running())
			{
				cs2::features::run();
			}
			else
			{
				cs2::features::reset();
			}
		}
		/*
		else if (csgo::game_handle)
		{
			if (csgo::running())
			{
				csgo::features::run();
			}
			else
			{
				csgo::features::reset();
			}
		}
		*/
		else if (apex::game_handle)
		{
			if (apex::running())
			{
				apex::features::run();
			}
			else
			{
				apex::features::reset();
			}
		}
		else
		{
			BOOL is_running = 0;
			if (!is_running) is_running = cs2::running();
			// if (!is_running) is_running = csgo::running();
			if (!is_running) is_running = apex::running();
		}
		
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
		SDL_RenderPresent(renderer);
	}
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}

