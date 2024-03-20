cmake_minimum_required(VERSION 2.6)
project(EC)
set(OUT EC)

include_directories("../../library/SDL3/include")
link_directories( ${CMAKE_CURRENT_LIST_DIR} )

add_executable ( ${OUT}
    main.cpp "../../cs2/shared/cs2.cpp" "../../cs2/shared/cs2features.cpp"
    "../../apex/shared/apex.cpp" "../../apex/shared/apexfeatures.cpp"  )

set_target_properties(${OUT} PROPERTIES LINKER_LANGUAGE CXX)


set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall -DNDEBUG -Wno-format-truncation -Wno-strict-aliasing -O3 -s")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-format-truncation -Wno-strict-aliasing")

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
target_link_libraries(${OUT} vm "-Wall -DNDEBUG -pthread -s -lSDL3 -O3")
