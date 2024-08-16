#include "../../cs2/shared/cs2game.h"
// #include "../../csgo/shared/csgogame.h"
#include "../../apex/shared/apexgame.h"
#include "../../library/SDL3/include/SDL3/SDL.h"

SDL_Renderer *sdl_renderer;

namespace client
{
	void mouse_move(int x, int y)
	{
		if (cs2::game_handle)
			cs2::input::move(x, y);
		// else if (csgo::game_handle)
		//	csgo::input::mouse_move(x, y);
		else
			apex::input::mouse_move(x, y);
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

	SDL_Window *window = SDL_CreateWindow("EC", 640, 480, SDL_WINDOW_FULLSCREEN);
	if (window == NULL)
	{
		return 0;
	}

	SDL_DisplayID id = SDL_GetDisplayForWindow(window);
	const SDL_DisplayMode *disp = SDL_GetCurrentDisplayMode(id);
	SDL_SetWindowSize(window, disp->w, disp->h);
	SDL_SetWindowPosition(window, 0, 0);
	
	sdl_renderer = SDL_CreateRenderer(window, 0, SDL_RENDERER_SOFTWARE);
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

		//
		// crosshair
		//
		client::DrawRect(0, disp->w / 2 - 5, disp->h / 2 - 5, 10, 10, 255, 0, 255);

		SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, 255);
		SDL_RenderPresent(sdl_renderer);
	}
	SDL_DestroyWindow(window);
	SDL_Quit();
}

