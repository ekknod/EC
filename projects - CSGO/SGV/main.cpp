#include "../../csgo/shared/shared.h"
#include <SDL3/SDL.h>
#pragma comment(lib, "SDL3.lib")

static SDL_Surface* sdl_surface;

namespace client
{
	void mouse_move(int x, int y)
	{
		// mouse_event(MOUSEEVENTF_MOVE, (DWORD)x, (DWORD)y, 0, 0);
		cs::input::mouse_move(x, y);
	}

	void mouse1_down(void)
	{
		// mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
	}

	void mouse1_up(void)
	{
		// mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
	}

	void DrawRect(void *hwnd, int x, int y, int w, int h, unsigned char r, unsigned char g, unsigned char b)
	{
		UNREFERENCED_PARAMETER(hwnd);
		SDL_Rect rect{};
		rect.x = (int)x;
		rect.y = (int)y;
		rect.w = (int)w;
		rect.h = (int)h;

		SDL_FillSurfaceRect(sdl_surface, &rect, SDL_MapRGB(sdl_surface->format, r, g, b));
	}

	void DrawFillRect(void *hwnd, int x, int y, int w, int h, unsigned char r, unsigned char g, unsigned char b)
	{
		UNREFERENCED_PARAMETER(hwnd);	
		SDL_Rect rect{};
		rect.x = (int)x;
		rect.y = (int)y;
		rect.w = (int)w;
		rect.h = (int)h;
		SDL_FillSurfaceRect(sdl_surface, &rect, SDL_MapRGB(sdl_surface->format, r, g, b));
	}
}

int main(void)
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		return 0;
	}

	SDL_Window *window = SDL_CreateWindow("EC", 640, 480, SDL_WINDOW_TRANSPARENT | SDL_WINDOW_BORDERLESS);
	if (window == NULL)
	{
		return 0;
	}

	SDL_DisplayID id = SDL_GetDisplayForWindow(window);
	const SDL_DisplayMode *disp = SDL_GetCurrentDisplayMode(id);
	SDL_SetWindowSize(window, disp->w, disp->h);
	SDL_SetWindowPosition(window, 0, 0);

	SDL_SetWindowAlwaysOnTop(window, SDL_TRUE);
	SDL_SetWindowOpacity(window, 0.20f);
	sdl_surface = SDL_GetWindowSurface(window);

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
		csgo::run();
		SDL_UpdateWindowSurface(window);
		SDL_FillSurfaceRect(sdl_surface, NULL, 0x000000);
	}
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}

