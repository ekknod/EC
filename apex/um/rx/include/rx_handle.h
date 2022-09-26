#ifndef RX_HANDLE_H
#define RX_HANDLE_H

#include "rx_types.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

rx_handle
rx_initialize_object(
    _in_     int     (*on_start)(rx_handle, void*),
    _in_     void    (*on_close)(rx_handle),
    _in_opt_ void    *start_parameters,
    _in_     size_t  size
    ) ;

void
rx_close_handle(
    _in_  rx_handle  object
    ) ;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* RX_HANDLE_H */

