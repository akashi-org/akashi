project (akashi_encoder CXX)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# include(FetchContent)
# set(FETCHCONTENT_QUIET OFF)

add_executable(${PROJECT_NAME}
  ./main.cpp
  ./encode_loop.cpp
  ./window_glfw.cpp
  ./decoder.cpp
  ./audio_renderer.cpp
)

set_target_properties(${PROJECT_NAME} PROPERTIES
  LINK_FLAGS "-Wl,--allow-shlib-undefined" # for libpython.so
)

target_include_directories(${PROJECT_NAME}
  PRIVATE "../../src"
)

# target_link_directories(${PROJECT_NAME}
# )

target_link_libraries(${PROJECT_NAME}
  PRIVATE akcore
  PRIVATE akstate
  PRIVATE akeval
  PRIVATE akcodec
  PRIVATE akbuffer
  PRIVATE akgraphics
  PRIVATE Threads::Threads
  PRIVATE atomic
  PUBLIC ${AKASHI_PRIV_SHARED_DEPS}_glfw
  PRIVATE EGL
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_precompile_headers(${PROJECT_NAME} PRIVATE
    ./pch.h
  )
endif()
