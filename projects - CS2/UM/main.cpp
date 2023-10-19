#include "../../cs2/shared/shared.h"
#include <gl/GL.h>
#include <GLFW/glfw3.h>
#pragma comment(lib, "glfw3.lib")
#pragma comment(lib, "opengl32.lib")

static const GLFWvidmode *mode;
 
namespace gl
{
	void DrawFillRect(float x, float y, float width, float height, int r, int g, int b)
	{
		glBegin(GL_QUADS);
		glColor4f(r / 255.f, g / 255.f, b / 255.f, 1);
		glVertex2f(x - (width / 2), y);
		glVertex2f(x - (width / 2), y - height);
		glVertex2f(x + (width / 2), y - height);
		glVertex2f(x + (width / 2), y);
		glEnd();
	}

	void DrawRect(float x, float y, float width, float height, int r, int g, int b)
	{
		glLineWidth(2);
		glBegin(GL_LINE_LOOP);
		glColor4f(r / 255.f, g / 255.f, b / 255.f, 1);
		glVertex2f(x - (width / 2), y);
		glVertex2f(x - (width / 2), y - height);
		glVertex2f(x + (width / 2), y - height);
		glVertex2f(x + (width / 2), y);
		glEnd();
	}
}

namespace client
{
	void mouse_move(int x, int y)
	{
		mouse_event(MOUSEEVENTF_MOVE, (DWORD)x, (DWORD)y, 0, 0);
	}

	void mouse1_down(void)
	{
		mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
	}

	void mouse1_up(void)
	{
		mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
	}

	void DrawRect(void *hwnd, int x, int y, int w, int h, unsigned char r, unsigned char g, unsigned b)
	{
		UNREFERENCED_PARAMETER(hwnd);	
		float fl_x = ((float)x / (float)mode->width) * 2.0f;
		float fl_y = ((mode->height - (float)y) / (float)mode->height) * 2.0f;

		fl_x -= 1.0f;
		fl_y -= 1.0f;

		float fl_w = ((float)w / mode->width)  * 2.0f;
		float fl_h = ((float)h / mode->height) * 2.0f;

		fl_x += (fl_w/2.0f);

		gl::DrawRect(fl_x, fl_y, fl_w, fl_h, r, g, b);
	}

	void DrawFillRect(void *hwnd, int x, int y, int w, int h, unsigned char r, unsigned char g, unsigned b)
	{
		UNREFERENCED_PARAMETER(hwnd);	
		float fl_x = ((float)x / (float)mode->width) * 2.0f;
		float fl_y = ((mode->height - (float)y) / (float)mode->height) * 2.0f;

		fl_x -= 1.0f;
		fl_y -= 1.0f;

		float fl_w = ((float)w / mode->width)  * 2.0f;
		float fl_h = ((float)h / mode->height) * 2.0f;

		fl_x += (fl_w/2.0f);

		gl::DrawFillRect(fl_x, fl_y, fl_w, fl_h, r, g, b);
	}
}

int main(void)
{
	if (!glfwInit())
	{
		return 0;
	}

	glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
	glfwWindowHint(GLFW_DECORATED, GL_FALSE);

	mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
	GLFWwindow *window = glfwCreateWindow(mode->width, mode->height-1, "EC", NULL, NULL);

	glfwMakeContextCurrent(window);
	

	glfwSetWindowAttrib(window, GLFW_FLOATING, GLFW_TRUE);
	glfwSetWindowAttrib(window, GLFW_MOUSE_PASSTHROUGH, GLFW_TRUE);

	glfwSwapInterval(1);

	while (!glfwWindowShouldClose(window))
	{
		int w, h;
		glfwGetFramebufferSize(window, &w, &h);
		glViewport(0, 0, w, h);

		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		cs2::run();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	glfwDestroyWindow(window);

	return 0;
}

