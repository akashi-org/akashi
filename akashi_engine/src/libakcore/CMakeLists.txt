project (akcore CXX)

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

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
  add_subdirectory(${nlohmann_json_SOURCE_DIR} ${nlohmann_json_BINARY_DIR})
  include_directories(
    ${nlohmann_json_SOURCE_DIR}/include
  )
  file(COPY ${nlohmann_json_SOURCE_DIR}/LICENSE.MIT DESTINATION ${CMAKE_BINARY_DIR}/../vendor/credits/nlohmann_json/)
endif()

FetchContent_Declare(spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.8.0
  GIT_SHALLOW TRUE
  GIT_PROGRESS TRUE
)
FetchContent_GetProperties(spdlog)
if(NOT spdlog_POPULATED)
  FetchContent_Populate(spdlog)

  if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(SPDLOG_ENABLE_PCH ON CACHE BOOL "")
  else ()
    set(SPDLOG_ENABLE_PCH OFF CACHE BOOL "")
  endif ()

  if(AKASHI_BUILD_SHARED_LIBS)
    set(SPDLOG_BUILD_SHARED ON CACHE BOOL "")
  else()
    set(SPDLOG_BUILD_SHARED OFF CACHE BOOL "")
  endif()

  add_definitions(
    "-DSPDLOG_COMPILED_LIB"
  )

  add_subdirectory(${spdlog_SOURCE_DIR} ${spdlog_BINARY_DIR})

  set_target_properties(spdlog PROPERTIES
    POSITION_INDEPENDENT_CODE ON # force adding -fPIC
  )

  link_directories(
    ${spdlog_BINARY_DIR}
  )
  include_directories(
    ${spdlog_SOURCE_DIR}/include/
  )

  file(COPY ${spdlog_SOURCE_DIR}/LICENSE DESTINATION ${CMAKE_BINARY_DIR}/../vendor/credits/spdlog/)
endif()


add_library(${PROJECT_NAME} ${AKASHI_BUILD_LIB_TYPE}
  "./rational.cpp"
  "./logger.cpp"
  "./path.cpp"
  "./config.cpp"
  "./string.cpp"
  "./uuid.cpp"
)

file(GLOB INTERFACE_HEADERS
  akcore.h
  audio.h
  class.h
  config.h
  element.h
  error.h
  logger.h
  memory.h
  path.h
  rational.h
  string.h
  hw_accel.h
  uuid.h
)
set_target_properties(${PROJECT_NAME} PROPERTIES 
  PUBLIC_HEADER "${INTERFACE_HEADERS}" # [XXX] we need double quotes here!
  POSITION_INDEPENDENT_CODE ON # force adding -fPIC
)

target_compile_definitions(${PROJECT_NAME}
  PUBLIC "SPDLOG_COMPILED_LIB"
)

target_include_directories(${PROJECT_NAME}
  PUBLIC ${spdlog_SOURCE_DIR}/include/
  PUBLIC ${nlohmann_json_SOURCE_DIR}/include
  PUBLIC ${AKASHI_PRIV_BOOST_INCLUDE_DIR}
)

target_link_directories(${PROJECT_NAME}
  PUBLIC ${spdlog_BINARY_DIR}
  PUBLIC ${AKASHI_PRIV_BOOST_LIBRARY_DIR} 
)

target_link_libraries(${PROJECT_NAME}
  PUBLIC spdlog::spdlog
  PRIVATE nlohmann_json::nlohmann_json
  PRIVATE ${AKASHI_PRIV_BOOST_TARGET_NAME} 
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_precompile_headers(${PROJECT_NAME} PRIVATE
    ./pch.h
  )
endif()

install(TARGETS ${PROJECT_NAME}
  PUBLIC_HEADER DESTINATION include/lib${PROJECT_NAME}
)

if (CMAKE_BUILD_TYPE STREQUAL "Release" AND AKASHI_BUILD_SHARED_LIBS)
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/../vendor/lib
    COMMAND ${CMAKE_COMMAND} -E copy ${spdlog_BINARY_DIR}/libspdlog* ${CMAKE_BINARY_DIR}/../vendor/lib
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${spdlog_SOURCE_DIR}/include ${CMAKE_BINARY_DIR}/../vendor/include/
  )
endif()

if(AKASHI_BUILD_TESTS)
  add_subdirectory("./test")
endif()
