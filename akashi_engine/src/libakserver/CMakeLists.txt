project (akserver CXX)

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

FetchContent_Declare(httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib
  GIT_TAG v0.7.9
  GIT_SHALLOW TRUE
  GIT_PROGRESS TRUE
)
FetchContent_GetProperties(httplib)
if(NOT httplib_POPULATED)
  FetchContent_Populate(httplib)
  
  set(HTTPLIB_COMPILE OFF CACHE BOOL "")
  # set(HTTPLIB_COMPILE ON CACHE BOOL "")

  # set(BUILD_SHARED_LIBS ON CACHE BOOL "") 

  file(COPY ${httplib_SOURCE_DIR}/LICENSE DESTINATION ${CMAKE_BINARY_DIR}/../vendor/credits/httplib/)

  # add_subdirectory(${httplib_SOURCE_DIR} ${httplib_BINARY_DIR})
endif()

FetchContent_Declare(nlohmann_json
  # GIT_REPOSITORY https://github.com/nlohmann/json
  GIT_REPOSITORY https://github.com/ArthurSonzogni/nlohmann_json_cmake_fetchcontent
  GIT_TAG v3.9.1
  GIT_SHALLOW TRUE
  GIT_PROGRESS TRUE
)
FetchContent_GetProperties(nlohmann_json)
if(NOT nlohmann_json_POPULATED)
  FetchContent_Populate(nlohmann_json)
  set(JSON_BuildTests OFF CACHE BOOL "")
  set(JSON_Install OFF CACHE BOOL "")
  # set(JSON_ImplicitConversions OFF CACHE BOOL "")
  
  file(COPY ${nlohmann_json_SOURCE_DIR}/LICENSE.MIT DESTINATION ${CMAKE_BINARY_DIR}/../vendor/credits/nlohmann_json/)

  add_subdirectory(${nlohmann_json_SOURCE_DIR} ${nlohmann_json_BINARY_DIR})
endif()


add_library(${PROJECT_NAME} ${AKASHI_BUILD_LIB_TYPE}
  ./server.cpp
  ./json_rpc.cpp
)

file(GLOB INTERFACE_HEADERS
  akserver.h
  api.h
  server.h
)
set_target_properties(${PROJECT_NAME} PROPERTIES 
  PUBLIC_HEADER "${INTERFACE_HEADERS}" # [XXX] we need double quotes here!
)


target_include_directories(${PROJECT_NAME}
  PRIVATE "../../src"
  PRIVATE ${nlohmann_json_SOURCE_DIR}/include
  PRIVATE ${httplib_SOURCE_DIR}
)

# target_link_directories(${PROJECT_NAME}
# )

target_link_libraries(${PROJECT_NAME}
  PRIVATE akcore
  PRIVATE nlohmann_json::nlohmann_json
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_precompile_headers(${PROJECT_NAME} PRIVATE
    ./pch.h
  )
endif()

install(TARGETS ${PROJECT_NAME}
  PUBLIC_HEADER DESTINATION include/lib${PROJECT_NAME}
)

