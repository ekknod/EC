/*
 * =====================================================================================
 *
 *       Filename:  rx_handle.c
 *
 *    Description:  cleanup
 *
 *        Version:  1.0
 *        Created:  27.09.2018 ??:??:??
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  github.com/ekknod 
 *   Organization:  
 *
 * =====================================================================================
 */

#include "../include/rx_handle.h"
#include <malloc.h>

struct rx_cleanup
{
	void (*close)(rx_handle);
};

rx_handle
rx_initialize_object(
    _in_ int (*on_start)(rx_handle, void *),
    _in_ void (*on_close)(rx_handle),
    _in_opt_ void *start_parameters,
    _in_ size_t size)
{
	struct rx_cleanup *c = malloc(sizeof(struct rx_cleanup) + size);
	c++->close = on_close;
	if (on_start(c, start_parameters) < 0)
	{
		free(--c);
		return 0;
	}
	return c;
}

void rx_close_handle(
    _in_ rx_handle object)
{
	struct rx_cleanup *c = object;
	if (c-- != 0)
	{
		c->close(object);
		free(c);
	}
}
