project (akeval CXX)

find_package(Python 3.11 COMPONENTS Interpreter Development REQUIRED)
message("-- Python Found " ${Python_VERSION})
message("-- Python INCLUDE_DIRS " ${Python_INCLUDE_DIRS})

FetchContent_Declare(pybind11
  GIT_REPOSITORY https://github.com/pybind/pybind11
  GIT_TAG v2.10.1
  GIT_SHALLOW TRUE
  GIT_PROGRESS TRUE
)
FetchContent_GetProperties(pybind11)
if(NOT pybind11_POPULATED)
  FetchContent_Populate(pybind11)
  set(PYBIND11_INSTALL ON CACHE BOOL "")
  set(PYBIND11_TEST OFF CACHE BOOL "")
  set(PYBIND11_NOPYTHON ON CACHE BOOL "")
  add_subdirectory(${pybind11_SOURCE_DIR} ${pybind11_BINARY_DIR})
  file(COPY ${pybind11_SOURCE_DIR}/LICENSE DESTINATION ${CMAKE_BINARY_DIR}/../vendor/credits/pybind11/)
endif()


add_library(${PROJECT_NAME} SHARED
  "./akeval.cpp"

  "./backend/python/context.cpp"
  "./backend/python/elem/elem.cpp"
  "./backend/python/elem/elem_eval.cpp"
  "./backend/python/elem/elem_tracer.cpp"
  "./backend/python/elem/elem_proxy.cpp"
  "./backend/python/elem/elem_parser.cpp"
)

file(GLOB INTERFACE_HEADERS
  akeval.h
)
set_target_properties(${PROJECT_NAME} PROPERTIES 
  PUBLIC_HEADER "${INTERFACE_HEADERS}" # [XXX] we need double quotes here!
)

target_include_directories(${PROJECT_NAME}
  PRIVATE "../../src"
  PRIVATE ${Python_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE akcore
  PRIVATE akstate
  PRIVATE pybind11::headers
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_precompile_headers(${PROJECT_NAME} PRIVATE
    ./pch.h
  )
endif()

install(TARGETS ${PROJECT_NAME}
  PUBLIC_HEADER DESTINATION include/lib${PROJECT_NAME}
)
