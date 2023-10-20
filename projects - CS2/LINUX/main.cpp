#include "../../cs2/shared/shared.h"

#include <sys/time.h>
#include <linux/types.h>
#include <linux/input-event-codes.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>


#include <GL/gl.h>
#include "../../library/glfw/include/GLFW/glfw3.h"

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
		float fl_x = ((float)x / (float)mode->width) * 2.0f;
		float fl_y = ((mode->height - (float)y) / (float)mode->height) * 2.0f;

		fl_x -= 1.0f;
		fl_y -= 1.0f;

		float fl_w = ((float)w / mode->width)  * 2.0f;
		float fl_h = ((float)h / mode->height) * 2.0f;

		fl_x -= (fl_w/2.0f);

		gl::DrawRect(fl_x, fl_y, fl_w, fl_h, r, g, b);
	}

	void DrawFillRect(void *hwnd, int x, int y, int w, int h, unsigned char r, unsigned char g, unsigned char b)
	{
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

//
// to-do fix: for some reason esp position is little off
//
int main(void)
{
	fd = open_device("event-mouse", 11);
	if (fd == -1)
	{
		LOG("failed to open mouse\n");
		return 0;
	}

	if (!glfwInit())
	{
		return 0;
	}


	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

	glfwDefaultWindowHints();
	glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
	glfwWindowHint(GLFW_DECORATED, GL_FALSE);
	glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
	mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

	GLFWwindow *window = glfwCreateWindow(mode->width, mode->height, "EC", NULL, NULL);

	glfwMakeContextCurrent(window);

	glfwSetWindowAttrib(window, GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
	glfwSetWindowAttrib(window, GLFW_FLOATING, GLFW_TRUE);
	glfwSetWindowAttrib(window, GLFW_MOUSE_PASSTHROUGH, GLFW_TRUE);

	glfwSwapInterval(1);

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		int w, h;
		glfwGetFramebufferSize(window, &w, &h);
		glViewport(0, 0, w, h);

		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		cs2::run();

		glfwSwapBuffers(window);
	}
	glfwDestroyWindow(window);
	return 0;
}

