#ifndef RX_PROCESS_H
#define RX_PROCESS_H

#include "rx_types.h"
#include "rx_handle.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

rx_handle rx_open_process(
    _in_     int               pid,
    _in_     RX_ACCESS_MASK    access_mask
    ) ;

rx_bool rx_process_exists(
    _in_      rx_handle        process
    ) ;

rx_bool rx_wow64_process(
    _in_      rx_handle        process
    ) ;

int rx_process_id(
    _in_      rx_handle        process
    ) ;

ssize_t rx_read_process(
    _in_     rx_handle         process,
    _in_     uintptr_t         address,
    _out_    void              *buffer,
    _in_     size_t            length
    ) ;

ssize_t rx_write_process(
    _in_     rx_handle         process,
    _in_     uintptr_t         address,
    _out_    void              *buffer,
    _in_     size_t            length
    ) ;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* RX_PROCESS_H */

