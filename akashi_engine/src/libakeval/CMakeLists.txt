project (akeval CXX)

find_package(PythonLibs 3.9 EXACT REQUIRED)
message("-- Python Found " ${PYTHONLIBS_VERSION_STRING})

FetchContent_Declare(pybind11
  # GIT_REPOSITORY https://github.com/nlohmann/json
  GIT_REPOSITORY https://github.com/pybind/pybind11
  GIT_TAG v2.6.2
  GIT_SHALLOW TRUE
  GIT_PROGRESS TRUE
)
FetchContent_GetProperties(pybind11)
if(NOT pybind11_POPULATED)
  FetchContent_Populate(pybind11)
  set(PYBIND11_INSTALL ON CACHE BOOL "")
  set(PYBIND11_TEST OFF CACHE BOOL "")
  # set(PYBIND11_NOPYTHON OFF CACHE BOOL "")
  add_subdirectory(${pybind11_SOURCE_DIR} ${pybind11_BINARY_DIR})
  file(COPY ${pybind11_SOURCE_DIR}/LICENSE DESTINATION ${CMAKE_BINARY_DIR}/../vendor/credits/pybind11/)
endif()


add_library(${PROJECT_NAME} ${AKASHI_BUILD_LIB_TYPE}
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
  PUBLIC ${PYTHON_INCLUDE_DIRS} # [TODO] PUBLIC => PRIVATE ?
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE akcore
  PRIVATE akstate
  PRIVATE ${PYTHON_LIBRARIES}
  PRIVATE pybind11::embed
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_precompile_headers(${PROJECT_NAME} PRIVATE
    ./pch.h
  )
endif()

install(TARGETS ${PROJECT_NAME}
  PUBLIC_HEADER DESTINATION include/lib${PROJECT_NAME}
)
