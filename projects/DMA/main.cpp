#include <Winsock2.h>
#include "../../cs2/shared/cs2game.h"
#include "../../csgo/shared/csgogame.h"
#include "../../apex/shared/apexgame.h"

#include <SDL3/SDL.h>
#include <devguid.h>
#include <SetupAPI.h>
#include <format>
#include <time.h>

#pragma warning(disable : 4996)

#pragma comment(lib, "SDL3.lib")
#pragma comment(lib, "Setupapi.lib")
#pragma comment(lib, "ws2_32.lib")

SDL_Renderer *sdl_renderer;

namespace kmbox
{
	namespace net {
		typedef struct
		{
			unsigned int  mac;
			unsigned int  rand;
			unsigned int  indexpts;
			unsigned int  cmd;
		}cmd_head_t;

		typedef struct
		{
			int button;
			int x;
			int y;
			int wheel;
			int point[10];
		}soft_mouse_t;

		typedef struct
		{
			cmd_head_t head;
			union {
				soft_mouse_t    cmd_mouse;
			};
		}client_tx;

		// data can be found on device screen
		static char ip[14] = "192.168.2.xxx"; // must replace with your devices ip.
		static char port[5] = "xxxx"; // must replace with your devices port.
		static char uuid[9] = "xxxxxxxx"; // must replace with your devices uuid.

		static client_tx tx;
		static client_tx rx;
		static soft_mouse_t softmouse;

		static BOOL is_net = 0;
		static SOCKET net_socket = 0;
		static SOCKADDR_IN address_srv = { 0 };

		BOOL open();
		void send(unsigned int cmd);
	}

	static HANDLE kmbox_handle = 0;
	static BOOL is_kmbox = 0;

	void mouse_move(int x, int y);
	void mouse_left(int state);

	BOOL open();
	BOOL scan_devices(LPCSTR deviceName, LPSTR lpOut);
}

namespace client
{
	void mouse_move(int x, int y)
	{
		if (kmbox::is_kmbox)
			kmbox::mouse_move(x, y);
		else
		{
			if (cs2::game_handle)
				cs2::input::move(x, y);
			else if (csgo::game_handle)
				csgo::input::mouse_move(x, y);
			else
				apex::input::mouse_move(x, y);
		}
	}

	void mouse1_down(void)
	{
		if (kmbox::is_kmbox)
			kmbox::mouse_left(1);
	}

	void mouse1_up(void)
	{
		if (kmbox::is_kmbox)
			kmbox::mouse_left(0);
	}

	void DrawRect(void* hwnd, int x, int y, int w, int h, unsigned char r, unsigned char g, unsigned char b)
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

	void DrawFillRect(void* hwnd, int x, int y, int w, int h, unsigned char r, unsigned char g, unsigned char b)
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
	kmbox::is_kmbox = kmbox::open() || kmbox::net::open();
	if (kmbox::is_kmbox)
	{
		LOG("kmbox device found\n");
	}
	else
	{
		LOG("kmbox not found, using normal input\n");
	}

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		return 0;
	}

	SDL_Window* window = SDL_CreateWindow("EC", 640, 480, SDL_WINDOW_BORDERLESS);
	if (window == NULL)
	{
		return 0;
	}

	SDL_DisplayID id = SDL_GetDisplayForWindow(window);
	const SDL_DisplayMode* disp = SDL_GetCurrentDisplayMode(id);
	SDL_SetWindowSize(window, disp->w, disp->h);
	SDL_SetWindowPosition(window, 0, 0);

	sdl_renderer = SDL_CreateRenderer(window, 0, SDL_RENDERER_SOFTWARE);
	SDL_SetRenderDrawBlendMode(sdl_renderer, SDL_BLENDMODE_BLEND);

	BOOL quit = 0;
	while (!quit)
	{
		SDL_Event e;
		while (SDL_PollEvent(&e))
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
			if (!is_running) is_running = csgo::running();
			if (!is_running) is_running = apex::running();
		}

		SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, 255);
		SDL_RenderPresent(sdl_renderer);
	}
	SDL_DestroyWindow(window);
	SDL_Quit();
}

void kmbox::mouse_move(int x, int y)
{
	if (net::is_net) 
	{
		net::softmouse.x = x;
		net::softmouse.y = y;
		net::send(0xaede7345);
	}
	else
	{
		char buffer[120]{};
		snprintf(buffer, 120, "km.move(%d, %d)\r", x, y);
		WriteFile(kmbox_handle, (void*)buffer, (DWORD)strlen(buffer), 0, NULL);
	}
}

void kmbox::mouse_left(int state)
{
	if (net::is_net)
	{
		net::softmouse.button = (state ? (net::softmouse.button | 0x01) : (net::softmouse.button & (~0x01)));
		net::send(0x9823ae8d);
	}
	else
	{
		char buffer[120]{};
		snprintf(buffer, 120, "km.left(%d)\r", state);
		WriteFile(kmbox_handle, (void*)buffer, (DWORD)strlen(buffer), 0, NULL);
	}
}

void kmbox::net::send(unsigned int cmd)
{
	tx.head.indexpts++;
	tx.head.cmd = cmd;
	tx.head.rand = rand();
	memcpy(&tx.cmd_mouse, &softmouse, sizeof(soft_mouse_t));
	sendto(net_socket, (const char*)&tx, sizeof(cmd_head_t) + sizeof(soft_mouse_t), 0, (struct sockaddr*)&address_srv, sizeof(address_srv));
	softmouse.x = 0;
	softmouse.y = 0;
	SOCKADDR_IN sclient{};
	int clen = sizeof(sclient);
	recvfrom(net_socket, (char*)&rx, 1024, 0, (struct sockaddr*)&sclient, &clen);
}

BOOL kmbox::net::open()
{
	if (kmbox_handle) { return FALSE; }

	WSADATA wsa_data;
	if (WSAStartup(MAKEWORD(1, 1), &wsa_data) != 0) { return FALSE; }

	if (LOBYTE(wsa_data.wVersion) != 1 || HIBYTE(wsa_data.wVersion) != 1)
	{
		WSACleanup();
		net_socket = 0;
		return FALSE;
	}

	auto to_hex = [](char* src, int len)
	{
		unsigned int dest[16] = { 0 };
		for (int i = 0; i < len; i++)
		{
			char h1 = src[2 * i];
			char h2 = src[2 * i + 1];
			unsigned char s1 = (unsigned char)( toupper(h1) - 0x30 );
			if (s1 > 9)
				s1 -= 7;
			unsigned char s2 = (unsigned char)( toupper(h2) - 0x30 );
			if (s2 > 9)
				s2 -= 7;
			dest[i] = s1 * 16 + s2;
		}
		return dest[0] << 24 | dest[1] << 16 | dest[2] << 8 | dest[3];
	};

	GetPrivateProfileStringA("kmboxnet", "ip", "", ip, 14, "./kmboxnet.txt");
	GetPrivateProfileStringA("kmboxnet", "port", "", port, 5, "./kmboxnet.txt");
	GetPrivateProfileStringA("kmboxnet", "uuid", "", uuid, 9, "./kmboxnet.txt");

	srand((unsigned)time(NULL));
	net_socket = socket(AF_INET, SOCK_DGRAM, 0);
	address_srv.sin_addr.S_un.S_addr = inet_addr(ip);
	address_srv.sin_family = AF_INET;
	address_srv.sin_port = htons((u_short)atoi(port));
	tx.head.mac = to_hex(uuid, 4);
	tx.head.rand = rand();
	tx.head.indexpts = 0;
	tx.head.cmd = 0xaf3c2828;
	memset(&softmouse, 0, sizeof(softmouse));
	sendto(net_socket, (const char*)&tx, sizeof(cmd_head_t), 0, (struct sockaddr*)&address_srv, sizeof(address_srv));
	Sleep(20);
	int clen = sizeof(address_srv);
	if (recvfrom(net_socket, (char*)&rx, 1024, 0, (struct sockaddr*)&address_srv, &clen) < 0)
	{
		closesocket(net_socket);
		WSACleanup();
		return FALSE;
	}

	if (rx.head.cmd != tx.head.cmd || rx.head.indexpts != tx.head.indexpts)
	{
		return FALSE;
	}

	is_net = TRUE;

	return TRUE;
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
		BYTE buffer[256]{};
		if (SetupDiGetDeviceRegistryPropertyA(deviceInfo, &dev_info_data, SPDRP_FRIENDLYNAME, NULL, buffer, sizeof(buffer), NULL))
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

