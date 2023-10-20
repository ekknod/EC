#include "../../cs2/shared/shared.h"

#include <SDL3/SDL.h>
#pragma comment(lib, "SDL3.lib")

SDL_Renderer *sdl_renderer;

namespace client
{
	void mouse_move(int x, int y)
	{
		// kmbox_mouse_move();


		cs::input::move(x, y);
	}

	void mouse1_down(void)
	{
		// kmbox_mouse_down();
	}

	void mouse1_up(void)
	{
		// kmbox_mouse_up();
	}

	void DrawRect(void *hwnd, int x, int y, int w, int h, unsigned char r, unsigned char g, unsigned char b)
	{
		UNREFERENCED_PARAMETER(hwnd);

		
		SDL_FRect rect{};
		rect.x = (float)x;
		rect.y = (float)y;
		rect.w = (float)w;
		rect.h = (float)h;
		SDL_SetRenderDrawColor(sdl_renderer, r, g, b, 255);
		SDL_RenderRect(sdl_renderer, &rect);
		
		/*
		SDL_FRect rect{};
		rect.x = (float)x;
		rect.y = (float)y;
		rect.w = (float)w;
		rect.h = (float)h;
		SDL_SetRenderDrawColor(sdl_renderer, r, g, b, 20);
		SDL_RenderFillRect(sdl_renderer, &rect);
		*/
	}

	void DrawFillRect(void *hwnd, int x, int y, int w, int h, unsigned char r, unsigned char g, unsigned char b)
	{
		UNREFERENCED_PARAMETER(hwnd);
		SDL_FRect rect{};
		rect.x = (float)x;
		rect.y = (float)y;
		rect.w = (float)w;
		rect.h = (float)h;
		SDL_SetRenderDrawColor(sdl_renderer, r, g, b, 20);
		SDL_RenderFillRect(sdl_renderer, &rect);
	}
}

int main(void)
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		return 0;
	}

	int w = 1920, h = 1080;
	for (int i = 0; i < 10; i++)
	{
		const SDL_DisplayMode *disp = SDL_GetCurrentDisplayMode(i);
		if (disp)
		{
			w = disp->w;
			h = disp->h;
			break;
		}
	}

	SDL_Window *window = SDL_CreateWindow("EC", w, h, SDL_WINDOW_FULLSCREEN);
	if (window == NULL)
	{
		return 0;
	}

	SDL_DisplayID id = SDL_GetDisplayForWindow(window);

	const SDL_DisplayMode *disp = SDL_GetCurrentDisplayMode(id);
	SDL_SetWindowSize(window, disp->w, disp->h);
	SDL_SetWindowPosition(window, 0, 0);
	
	sdl_renderer = SDL_CreateRenderer(window, 0, SDL_RENDERER_ACCELERATED);
	SDL_SetRenderDrawBlendMode(sdl_renderer, SDL_BLENDMODE_BLEND);

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

		SDL_RenderClear(sdl_renderer);

		cs2::run();

		SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, 255);
		SDL_RenderPresent(sdl_renderer);
	}
	SDL_DestroyWindow(window);
	SDL_Quit();
}

