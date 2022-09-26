#ifndef RX_TYPES_H
#define RX_TYPES_H

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define _in_
#define _in_opt_
#define _out_

typedef enum _RX_ACCESS_MASK {
    RX_READ_ACCESS      = 0,
    RX_WRITE_ACCESS     = 1,
    RX_ALL_ACCESS       = 2
} RX_ACCESS_MASK ;

typedef void *rx_handle;
typedef int rx_bool;

typedef __SIZE_TYPE__ size_t;
typedef __ssize_t     ssize_t;

typedef struct {
    char value[120];
} SHORT_STRING ;

typedef struct {
    char value[260];
} LONG_STRING ;

typedef struct {
    int x, y;
} vec2_i ;


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* RX_TYPES_H */

