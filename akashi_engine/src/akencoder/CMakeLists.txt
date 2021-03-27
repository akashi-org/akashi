project (akashi_encoder CXX)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

FetchContent_Declare(glfw
  GIT_REPOSITORY https://github.com/glfw/glfw
  GIT_TAG 3.3.2
  GIT_SHALLOW TRUE
  GIT_PROGRESS TRUE
)
FetchContent_GetProperties(glfw)
if(NOT glfw_POPULATED)
  FetchContent_Populate(glfw)
  set(GLFW_STANDALONE OFF CACHE BOOL "")
  set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "")
  set(GLFW_BUILD_TESTS OFF CACHE BOOL "")
  set(GLFW_BUILD_DOCS OFF CACHE BOOL "")
  set(GLFW_INSTALL OFF CACHE BOOL "")

  set(GLFW_USE_WAYLAND OFF CACHE BOOL "") # use X11 by default
  
  add_subdirectory(${glfw_SOURCE_DIR} ${glfw_BINARY_DIR})
  file(COPY ${glfw_SOURCE_DIR}/LICENSE.md DESTINATION ${CMAKE_BINARY_DIR}/../vendor/credits/glfw/)
endif()


add_executable(${PROJECT_NAME}
  ./main.cpp
  ./akencoder.cpp
  ./encode_queue.cpp
  ./producer/producer.cpp
  ./producer/window_glfw.cpp
  ./producer/decoder.cpp
  ./producer/audio_renderer.cpp
  ./consumer/consumer.cpp
)

target_include_directories(${PROJECT_NAME}
  PRIVATE ${glfw_SOURCE_DIR}/include/
  PRIVATE "../../src"
)

target_link_directories(${PROJECT_NAME}
  PUBLIC ${glfw_BINARY_DIR}
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE akcore
  PRIVATE akstate
  PRIVATE akeval
  PRIVATE akcodec
  PRIVATE akbuffer
  PRIVATE akgraphics
  PRIVATE Threads::Threads
  PRIVATE atomic
  PRIVATE glfw
  PRIVATE EGL
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_precompile_headers(${PROJECT_NAME} PRIVATE
    ./pch.h
  )
endif()
