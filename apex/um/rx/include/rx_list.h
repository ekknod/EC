#ifndef RX_LIST_H
#define RX_LIST_H

#include "rx_types.h"
#include "rx_handle.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus */

typedef enum _RX_SNAP_TYPE {
    RX_SNAP_TYPE_PROCESS = 0,
    RX_SNAP_TYPE_LIBRARY = 1
} RX_SNAP_TYPE ;

typedef struct _RX_PROCESS_ENTRY {
    char       exe[17];
    char       data[260];
    const char *path;
    const char *name;
    int        pid;
} RX_PROCESS_ENTRY , *PRX_PROCESS_ENTRY ;

typedef struct _RX_LIBRARY_ENTRY {
    char       data[260];
    const char *path;
    const char *name;
    uintptr_t  start, end;
    int        pid;
} RX_LIBRARY_ENTRY, *PRX_LIBRARY_ENTRY ;

rx_handle rx_create_snapshot(
    _in_     RX_SNAP_TYPE      type,
    _in_opt_ int               pid
    ) ;

rx_bool rx_next_process(
    _in_     rx_handle         snapshot,
    _out_    PRX_PROCESS_ENTRY entry
    ) ;

rx_bool rx_next_library(
    _in_     rx_handle         snapshot,
    _out_    PRX_LIBRARY_ENTRY entry
    ) ;


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* RX_LIST_H */

