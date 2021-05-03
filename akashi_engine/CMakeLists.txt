cmake_minimum_required(VERSION 3.16)
project (akashi_engine C CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# public options
option(AKASHI_BUILD_TESTS "" ON)

option(AKASHI_BUILD_SHARED_LIBS "" OFF)
if(AKASHI_BUILD_SHARED_LIBS)
  set(AKASHI_BUILD_LIB_TYPE SHARED)
else()
  set(AKASHI_BUILD_LIB_TYPE STATIC)
endif()

option(AKASHI_BUILD_LFSWATCH "If set, build libfswatch and install it to this project" ON)

set(AKASHI_PRIV_BOOST_TARGET_NAME "akashi_priv_boost_target")
set(AKASHI_PRIV_BOOST_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shared_temp/boost/)
set(AKASHI_PRIV_BOOST_INCLUDE_DIR ${AKASHI_PRIV_BOOST_ROOT_DIR}/include)
set(AKASHI_PRIV_BOOST_LIBRARY_DIR ${AKASHI_PRIV_BOOST_ROOT_DIR}/lib)

if(CMAKE_BUILD_TYPE STREQUAL "" OR CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "" FORCE)
  message("-- Using Debug mode")
  # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-sanitize-recover=all")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
  message("-- Using Release mode")
elseif(CMAKE_BUILD_TYPE STREQUAL "Check")
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "" FORCE)
  message("-- Using Check mode")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-sanitize-recover=all")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined")
  #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread")
  #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
  #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=leak")
else()
  message(FATAL_ERROR "Invalid CMAKE_BUILD_TYPE found! Possible values are:\n'Debug', 'Release', 'Check'")
endif()

string(CONCAT COMPILER_FLAGS
  " -Wall"
  " -Wextra"
  " -std=c++17"
  #  " -fno-rtti"
  " -fdiagnostics-color=always"
  " -fdiagnostics-show-template-tree"
  " -fno-strict-aliasing"
)
add_definitions(${COMPILER_FLAGS})

# for older gcc(<9)
link_libraries(
  stdc++fs
)

if(AKASHI_BUILD_TESTS)
  enable_testing()
  add_subdirectory("./src/libaktest")
endif()

add_subdirectory("./src/libakplayer")
add_subdirectory("./src/libakcore")
add_subdirectory("./src/libakcodec")
add_subdirectory("./src/libakbuffer")
add_subdirectory("./src/libakaudio")
add_subdirectory("./src/libakgraphics")
add_subdirectory("./src/libakeval")
add_subdirectory("./src/libakwatch")
add_subdirectory("./src/libakserver")
add_subdirectory("./src/libakstate")
add_subdirectory("./src/libakevent")

add_subdirectory("./src/akkernel")
set_target_properties(akashi_kernel PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_subdirectory("./src/akui")
set_target_properties(akashi_renderer PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_subdirectory("./src/akencoder")
set_target_properties(akashi_encoder PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_library(${AKASHI_PRIV_BOOST_TARGET_NAME} INTERFACE)
include(ExternalProject)
ExternalProject_Add(
  boost_akashi
  URL https://boostorg.jfrog.io/artifactory/main/release/1.74.0/source/boost_1_74_0.tar.gz
  TIMEOUT 600
  # URL ${CMAKE_CURRENT_SOURCE_DIR}/shared/boost_1_74_0.tar.gz
  URL_MD5 3c8fb92ce08b9ad5a5f0b35731ac2c8e
  
  SOURCE_DIR ${CMAKE_BINARY_DIR}/.ext/boost/
  INSTALL_DIR ${AKASHI_PRIV_BOOST_ROOT_DIR}

  CONFIGURE_COMMAND ""
  BUILD_COMMAND cd <SOURCE_DIR> && ./bootstrap.sh && ./b2 install -j4 --with-filesystem --prefix=<INSTALL_DIR> link=static threading=multi variant=release
  INSTALL_COMMAND COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/vendor/credits/boost/
    COMMAND ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/LICENSE_1_0.txt ${CMAKE_CURRENT_SOURCE_DIR}/vendor/credits/boost/
)
add_dependencies(${AKASHI_PRIV_BOOST_TARGET_NAME} boost_akashi)
target_compile_definitions(${AKASHI_PRIV_BOOST_TARGET_NAME}
  INTERFACE "BOOST_CHRONO_HEADER_ONLY"
)
