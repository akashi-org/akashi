project (akashi_player CXX)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(${PROJECT_NAME}
  ./main.cpp
)

target_include_directories(${PROJECT_NAME}
  PRIVATE "../../src"
)

# target_link_directories(${PROJECT_NAME}
# )

target_link_libraries(${PROJECT_NAME}
  PRIVATE akcore
  PRIVATE Threads::Threads
  PRIVATE atomic
  PRIVATE ${AKASHI_PRIV_SHARED_DEPS}_glfw
  PRIVATE EGL
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_precompile_headers(${PROJECT_NAME} PRIVATE
    ./pch.h
  )
endif()
