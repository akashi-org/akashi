project (akwatch CXX)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

link_directories(
  ${CMAKE_BINARY_DIR}/../shared_temp/fswatch/lib
)

include_directories(
  "../../src"
  ${CMAKE_BINARY_DIR}/../shared_temp/fswatch/include
)

add_library(${PROJECT_NAME} ${AKASHI_BUILD_LIB_TYPE}
  "./akwatch.cpp"

  "./backend/libfswatch/context.cpp"
  "./backend/libfswatch/util.cpp"
)

file(GLOB INTERFACE_HEADERS
  akwatch.h
  item.h
)
set_target_properties(${PROJECT_NAME} PROPERTIES 
  PUBLIC_HEADER "${INTERFACE_HEADERS}" # [XXX] we need double quotes here!
)

target_include_directories(${PROJECT_NAME}
  PRIVATE "../../src"
  PUBLIC ${CMAKE_BINARY_DIR}/../shared_temp/fswatch/include
)

target_link_directories(${PROJECT_NAME}
  PUBLIC ${CMAKE_BINARY_DIR}/../shared_temp/fswatch/lib
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE akcore
  PRIVATE Threads::Threads
  PRIVATE fswatch
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_precompile_headers(${PROJECT_NAME} PRIVATE
    ./pch.h
  )
endif()

install(TARGETS ${PROJECT_NAME}
  PUBLIC_HEADER DESTINATION include/lib${PROJECT_NAME}
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/../vendor/lib
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/../shared_temp/fswatch/lib/libfswatch.so* ${CMAKE_BINARY_DIR}/../vendor/lib
)

include(ExternalProject)
if(AKASHI_BUILD_LFSWATCH)
  ExternalProject_Add(
    fswatch
    URL https://github.com/emcrisostomo/fswatch/releases/download/1.9.1/fswatch-1.9.1.tar.gz
    URL_MD5 48b8f5d990838ff0c6d27ae8fc97e784
    
    SOURCE_DIR ${CMAKE_BINARY_DIR}/.ext/fswatch
    INSTALL_DIR ${CMAKE_BINARY_DIR}/../shared_temp/fswatch
  
    CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=<INSTALL_DIR>
    BUILD_COMMAND make -j4
    INSTALL_COMMAND make install
      COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/../vendor/credits/fswatch
      COMMAND ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/LICENSE ${CMAKE_BINARY_DIR}/../vendor/credits/fswatch/
      COMMAND ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/COPYING ${CMAKE_BINARY_DIR}/../vendor/credits/fswatch/
  )
  add_dependencies(${PROJECT_NAME} fswatch)
endif()
