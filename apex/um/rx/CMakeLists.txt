cmake_minimum_required(VERSION 2.6)
project(rx)

set(OUT rx)

set(LINKTYPE STATIC)

add_library ( ${OUT} ${LINKTYPE}
    src/rx_handle.c src/rx_input.c src/rx_list.c src/rx_module.c src/rx_process.c )

set_target_properties(${OUT} PROPERTIES LINKER_LANGUAGE C)
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Wall -DNDEBUG -Wno-format-truncation -O2 -s -lpthread")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wno-format-truncation")

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

