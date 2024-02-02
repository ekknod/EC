#include "../../csgo/shared/shared.h"

#include <SDL3/SDL.h>
#include <devguid.h>
#include <SetupAPI.h>
#include <format>

#pragma comment(lib, "SDL3.lib")
#pragma comment (lib, "Setupapi.lib")

SDL_Renderer *sdl_renderer;

namespace kmbox
{
	static HANDLE kmbox_handle = 0;

	void mouse_move(int x, int y);
	void mouse_left(int state);

	BOOL open();
	BOOL scan_devices(LPCSTR deviceName, LPSTR lpOut);
}

namespace client
{
	void mouse_move(int x, int y)
	{
		if (kmbox::kmbox_handle)
			kmbox::mouse_move(x, y);
		else
			cs::input::mouse_move(x, y);
	}

	void mouse1_down(void)
	{
		if (kmbox::kmbox_handle)
			kmbox::mouse_left(1);
	}

	void mouse1_up(void)
	{
		if (kmbox::kmbox_handle)
			kmbox::mouse_left(0);
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
	if (kmbox::open())
	{
		LOG("kmbox device found\n");
	}
	else
	{
		LOG("kmbox not found, using normal input\n");
	}

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		return 0;
	}

	SDL_Window *window = SDL_CreateWindow("EC", 640, 480, SDL_WINDOW_BORDERLESS);
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

		csgo::run();

		SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, 255);
		SDL_RenderPresent(sdl_renderer);
	}
	SDL_DestroyWindow(window);
	SDL_Quit();
}

void kmbox::mouse_move(int x, int y)
{
	char buffer[120]{};
	snprintf(buffer, 120, "km.move(%d, %d)\r", x, y);
	WriteFile(kmbox_handle, (void*)buffer, (DWORD)strlen(buffer), 0, NULL);
}

void kmbox::mouse_left(int state)
{
	char buffer[120]{};
	snprintf(buffer, 120, "km.left(%d)\r", state);
	WriteFile(kmbox_handle, (void*)buffer, (DWORD)strlen(buffer), 0, NULL);
}

BOOL kmbox::open()
{
	if (kmbox_handle) { return TRUE; }

	LPCSTR deviceName = "USB-SERIAL CH340";

	char port[120]{};
	strcat_s(port, "\\\\.\\");

	if (!scan_devices(deviceName, port))
	{
		return 0;
	}

	kmbox_handle = CreateFileA(
		port,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if (kmbox_handle == (HANDLE)-1)
	{
		kmbox_handle = 0;
		return 0;
	}

	DCB dcb = { 0 };
	dcb.DCBlength = sizeof(dcb);
	GetCommState(kmbox_handle, &dcb);

	dcb.BaudRate = CBR_115200;
	dcb.ByteSize = 8;
	dcb.StopBits = ONESTOPBIT;
	dcb.Parity = NOPARITY;

	SetCommState(kmbox_handle, &dcb);

	COMMTIMEOUTS cto = { 0 };
	cto.ReadIntervalTimeout = 1;
	cto.ReadTotalTimeoutConstant = 0;
	cto.ReadTotalTimeoutMultiplier = 0;
	cto.WriteTotalTimeoutConstant = 0;
	cto.WriteTotalTimeoutMultiplier = 0;

	SetCommTimeouts(kmbox_handle, &cto);

	return TRUE;
}

// https://github.com/nbqofficial/cpp-arduino
BOOL kmbox::scan_devices(LPCSTR deviceName, LPSTR lpOut)
{
	BOOL status = 0;
	char com[] = "COM";

	HDEVINFO deviceInfo = SetupDiGetClassDevs(&GUID_DEVCLASS_PORTS, NULL, NULL, DIGCF_PRESENT);
	if (deviceInfo == INVALID_HANDLE_VALUE) { return false; }

	SP_DEVINFO_DATA dev_info_data;
	dev_info_data.cbSize = sizeof(dev_info_data);

	DWORD count = 0;

	while (SetupDiEnumDeviceInfo(deviceInfo, count++, &dev_info_data))
	{
		BYTE buffer[256];
		if (SetupDiGetDeviceRegistryProperty(deviceInfo, &dev_info_data, SPDRP_FRIENDLYNAME, NULL, buffer, sizeof(buffer), NULL))
		{
			DWORD i = (DWORD)strlen(lpOut);
			LPCSTR lp_pos = strstr((LPCSTR)buffer, com);
			DWORD len = i + (DWORD)strlen(lp_pos);

			if (strstr((LPCSTR)buffer, deviceName) && lp_pos)
			{
				for (DWORD j = 0; i < len; i++, j++)
				{
					lpOut[i] = lp_pos[j];
				}

				lpOut[i - 1] = '\0';
				status = true;
				break;
			}
		}
	}

	SetupDiDestroyDeviceInfoList(deviceInfo);
	return status;
}

