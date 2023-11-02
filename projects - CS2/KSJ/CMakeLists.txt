cmake_minimum_required(VERSION 2.6)
project(EC)
set(OUT EC)

add_executable ( ${OUT}
    main.cpp "../../cs2/shared/cs.cpp" "../../cs2/shared/features.cpp" )

set_target_properties(${OUT} PROPERTIES LINKER_LANGUAGE CXX)


set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall -DNDEBUG -Wno-format-truncation -Wno-strict-aliasing -O3 -s")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-format-truncation -Wno-strict-aliasing")

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
target_link_libraries(${OUT} vm "-Wall -DNDEBUG -pthread -s -O3")
