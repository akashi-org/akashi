project (akencoder CXX)

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


add_library(${PROJECT_NAME} ${AKASHI_BUILD_LIB_TYPE}
  ./akencoder.cpp
  ./encode_queue.cpp
  ./producer/producer.cpp
  ./consumer/consumer.cpp
)

file(GLOB INTERFACE_HEADERS
  akencoder.h
)
set_target_properties(${PROJECT_NAME} PROPERTIES 
  PUBLIC_HEADER "${INTERFACE_HEADERS}" # [XXX] we need double quotes here!
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
  PRIVATE Threads::Threads
  PRIVATE atomic
  PRIVATE glfw
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_precompile_headers(${PROJECT_NAME} PRIVATE
    ./pch.h
  )
endif()

install(TARGETS ${PROJECT_NAME}
  PUBLIC_HEADER DESTINATION include/lib${PROJECT_NAME}
)

