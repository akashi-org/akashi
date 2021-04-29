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

target_compile_definitions(${PROJECT_NAME}
  PRIVATE "BOOST_CHRONO_HEADER_ONLY"
)

target_include_directories(${PROJECT_NAME}
  PRIVATE "../../src"
  PRIVATE "../../shared/boost_1_74_0/include"
)

target_link_directories(${PROJECT_NAME}
  PRIVATE "../../shared_temp/boost_filesystem/lib"
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE akcore
  PRIVATE akstate
  PRIVATE akserver # for asp
  PRIVATE Threads::Threads
  PRIVATE atomic
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_precompile_headers(${PROJECT_NAME} PRIVATE
    ./pch.h
  )
endif()

include(ExternalProject)
ExternalProject_Add(
  boost_filesystem
  # URL https://boostorg.jfrog.io/artifactory/main/release/1.74.0/source/boost_1_74_0.tar.gz
  # TIMEOUT 600
  URL ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/boost_1_74_0.tar.gz
  URL_MD5 3c8fb92ce08b9ad5a5f0b35731ac2c8e
  
  SOURCE_DIR ${CMAKE_BINARY_DIR}/.ext/boost/
  INSTALL_DIR ${CMAKE_BINARY_DIR}/../shared_temp/boost_filesystem/

  CONFIGURE_COMMAND ""
  BUILD_COMMAND cd <SOURCE_DIR> && ./bootstrap.sh && ./b2 install -j4 --with-filesystem --prefix=<INSTALL_DIR> link=static threading=multi variant=release
  INSTALL_COMMAND ""
)
add_dependencies(${PROJECT_NAME} boost_filesystem)
