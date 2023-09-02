project (akplayer CXX)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_library(${PROJECT_NAME} ${AKASHI_BUILD_LIB_TYPE}
  ./akplayer.cpp
  ./event.cpp
  ./eval_buffer.cpp
  ./loop/main_loop.cpp
  ./loop/decode_loop.cpp
  ./loop/event_loop.cpp
  ./loop/watch_loop.cpp
  ./reload/seek_manager.cpp
  ./reload/hr_manager.cpp
  ./reload/utils.cpp
)

file(GLOB INTERFACE_HEADERS
  akplayer.h
)
set_target_properties(${PROJECT_NAME} PROPERTIES 
  PUBLIC_HEADER "${INTERFACE_HEADERS}" # [XXX] we need double quotes here!
)

target_include_directories(${PROJECT_NAME}
  PRIVATE "../../src"
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE akcore
  PRIVATE akbuffer
  PRIVATE akcodec
  PRIVATE akaudio
  PRIVATE akgraphics
  PRIVATE akeval
  PRIVATE akwatch
  PRIVATE akstate
  PRIVATE akevent
  PRIVATE Threads::Threads
  PRIVATE atomic
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_precompile_headers(${PROJECT_NAME} PRIVATE
    ./pch.h
  )
endif()

install(TARGETS ${PROJECT_NAME}
  PUBLIC_HEADER DESTINATION include/lib${PROJECT_NAME}
)

