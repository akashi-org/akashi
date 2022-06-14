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

set(AKASHI_BOOST_TAR_PATH "" CACHE STRING "If non-null string provided, use it as the destination url of boost's tarball")

# private options

set(AKASHI_PRIV_CONFIG_TARGET_NAME "akashi_priv_config_target")
set(AKASHI_ENGINE_VERSION_MAJOR "0")
set(AKASHI_ENGINE_VERSION_MINOR "1")

set(AKASHI_PRIV_SHARED_DEPS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/shared_temp/)
set(AKASHI_PRIV_SHARED_DEPS "akashi_shared_deps")

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/PrepareSharedDeps.cmake)

if(CMAKE_BUILD_TYPE STREQUAL "" OR CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "" FORCE)
  message("-- Using Debug mode")
  # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-sanitize-recover=all")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined -rdynamic")
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

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}/include
)

if(AKASHI_BUILD_TESTS)
  enable_testing()
  add_subdirectory("./src/libaktest")
endif()

add_subdirectory("./src/libakplayer")
add_subdirectory("./src/libakcore")

add_subdirectory("./src/libakcodec")
set_target_properties(akprobe PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_subdirectory("./src/libakbuffer")
add_subdirectory("./src/libakaudio")
add_subdirectory("./src/libakgraphics")

add_subdirectory("./src/libakeval")
set_target_properties(akeval PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

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

add_dependencies(akashi_kernel ${AKASHI_PRIV_CONFIG_TARGET_NAME})
add_custom_target(${AKASHI_PRIV_CONFIG_TARGET_NAME} ALL
  VERBATIM
  BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/include/version.h
  COMMAND /bin/bash ${CMAKE_CURRENT_SOURCE_DIR}/akashi_version.sh ${CMAKE_CURRENT_BINARY_DIR}/include ${AKASHI_ENGINE_VERSION_MAJOR} ${AKASHI_ENGINE_VERSION_MINOR}
)

add_subdirectory("./src/akplayer")
set_target_properties(akashi_player PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)
