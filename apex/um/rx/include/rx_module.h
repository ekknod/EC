#ifndef RX_MODULE_H
#define RX_MODULE_H

#include "rx_types.h"
#include "rx_handle.h"
 
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

uintptr_t rx_current_module(void);

int       rx_module_count(void);

uintptr_t rx_module_base(
    _in_     uintptr_t         module
    ) ;

uintptr_t rx_module_base_ex(
    _in_     rx_handle         process,
    _in_     uintptr_t         module
    ) ;

const char *rx_module_path(
    _in_     uintptr_t         module
    ) ;

LONG_STRING rx_module_path_ex(
    _in_     rx_handle         process,
    _in_     uintptr_t         module
    ) ;

uintptr_t rx_find_module(
    _in_     const char        *name
    ) ;

uintptr_t rx_find_module_ex(
    _in_     rx_handle         process,
    _in_     const char        *name
    ) ;

uintptr_t rx_find_export(
    _in_     uintptr_t         module,
    _in_     const char        *name
    ) ;

uintptr_t rx_find_export_ex(
    _in_     rx_handle         process,
    _in_     uintptr_t         module,
    _in_     const char        *name
    ) ;


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* RX_MODULE_H */

