#include "../../cs2/shared/shared.h"

#include <SDL3/SDL.h>
#include <devguid.h>
#include <SetupAPI.h>
#include <format>

#pragma comment(lib, "SDL3.lib")
#pragma comment (lib, "Setupapi.lib")

SDL_Renderer *sdl_renderer;
HANDLE g_serial_handle = 0;

namespace kmbox
{
	void mouse_move(int x, int y);

	BOOL open();
	BOOL scan_devices(LPCSTR deviceName, LPSTR lpOut);
}

namespace client
{
	void mouse_move(int x, int y)
	{
		kmbox::mouse_move(x, y);
		//cs::input::move(x, y);
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

		cs2::run();

		SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, 255);
		SDL_RenderPresent(sdl_renderer);
	}
	SDL_DestroyWindow(window);
	SDL_Quit();
}

void kmbox::mouse_move(int x, int y)
{
	if (!kmbox::open()) { return; }

	auto buffer = std::format("km.move({},{})\r", x, y).c_str();

	DWORD bytes_written;
	WriteFile(g_serial_handle, (void*)buffer, sizeof buffer, &bytes_written, NULL);
}

BOOL kmbox::open()
{
	if (g_serial_handle) { return TRUE; }

	LPCSTR deviceName = "USB-SERIAL CH340";
	char port[] = "\\.\\";

	while (!scan_devices(deviceName, port))
	{
		Sleep(1000);
	}

	g_serial_handle = CreateFileA(
		port,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if (g_serial_handle)
	{
		DCB dcb = { 0 };
		dcb.DCBlength = sizeof(dcb);

		if (!GetCommState(g_serial_handle, &dcb))
		{
			CloseHandle(g_serial_handle);
			return FALSE;
		}

		dcb.BaudRate = CBR_115200;
		dcb.ByteSize = 8;
		dcb.StopBits = ONESTOPBIT;
		dcb.Parity = NOPARITY;

		if (!SetCommState(g_serial_handle, &dcb))
		{
			CloseHandle(g_serial_handle);
			return FALSE;
		}

		COMMTIMEOUTS cto = { 0 };
		cto.ReadIntervalTimeout = 50;
		cto.ReadTotalTimeoutConstant = 50;
		cto.ReadTotalTimeoutMultiplier = 10;
		cto.WriteTotalTimeoutConstant = 50;
		cto.WriteTotalTimeoutMultiplier = 10;

		if (!SetCommTimeouts(g_serial_handle, &cto))
		{
			CloseHandle(g_serial_handle);
			return FALSE;
		}

		return TRUE;
	}

	return FALSE;
}

// https://github.com/nbqofficial/cpp-arduino

BOOL kmbox::scan_devices(LPCSTR deviceName, LPSTR lpOut)
{
	bool status = false;
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
			DWORD i = strlen(lpOut);
			LPCSTR lp_pos = strstr((LPCSTR)buffer, com);
			DWORD len = i + strlen(lp_pos);

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
