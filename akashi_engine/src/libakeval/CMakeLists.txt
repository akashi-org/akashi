project (akeval CXX)

find_package(PythonLibs 3.9 EXACT REQUIRED)
message("-- Python Found " ${PYTHONLIBS_VERSION_STRING})

add_library(${PROJECT_NAME} ${AKASHI_BUILD_LIB_TYPE}
  "./akeval.cpp"

  "./backend/python/context.cpp"
  "./backend/python/parser.cpp"
  "./backend/python/utils/print.cpp"
  "./backend/python/core/module.cpp"
  "./backend/python/core/func.cpp"
  "./backend/python/core/string.cpp"
  "./backend/python/core/object.cpp"
  "./backend/python/core/class.cpp"
  "./backend/python/core/value.cpp"
  "./backend/python/core/import.cpp"
  "./backend/python/libraries/akashi_core/init.cpp"
  "./backend/python/libraries/akashi_core/kron.cpp"
  "./backend/python/libraries/akashi_core/kron_utils.cpp"
  "./backend/python/libraries/akashi_core/time.cpp"
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
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_precompile_headers(${PROJECT_NAME} PRIVATE
    ./pch.h
  )
endif()

install(TARGETS ${PROJECT_NAME}
  PUBLIC_HEADER DESTINATION include/lib${PROJECT_NAME}
)
