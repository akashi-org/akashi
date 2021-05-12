project (akashi_kernel CXX)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(${PROJECT_NAME}
  ./main.cpp
  ./kernel.cpp
  ./kernel_event.cpp
  ./kernel_handler.cpp
  ./workers/transport.cpp
  ./workers/process.cpp
)

# target_compile_definitions(${PROJECT_NAME}
# )

target_include_directories(${PROJECT_NAME}
  PRIVATE "../../src"
  PRIVATE ${AKASHI_PRIV_BOOST_INCLUDE_DIR}
)

target_link_directories(${PROJECT_NAME}
  PRIVATE ${AKASHI_PRIV_BOOST_LIBRARY_DIR}
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE akcore
  PRIVATE akstate
  PRIVATE akserver # for asp
  PRIVATE Threads::Threads
  PRIVATE atomic
  PRIVATE ${AKASHI_PRIV_BOOST_TARGET_NAME}
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_precompile_headers(${PROJECT_NAME} PRIVATE
    ./pch.h
  )
endif()
