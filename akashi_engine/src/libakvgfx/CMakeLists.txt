project (akvgfx CXX)

find_package(PkgConfig REQUIRED)
pkg_check_modules(CAIRO REQUIRED cairo)

add_library(${PROJECT_NAME} ${AKASHI_BUILD_LIB_TYPE}
  "./akvgfx.cpp"

  "./backend/cairo/surface.cpp"
  "./backend/cairo/surface_creator.cpp"
  "./backend/cairo/utils.cpp"
)

# file(GLOB INTERFACE_HEADERS
# )
# set_target_properties(${PROJECT_NAME} PROPERTIES 
#   PUBLIC_HEADER "${INTERFACE_HEADERS}" # [XXX] we need double quotes here!
# )

# target_compile_definitions(${PROJECT_NAME}
# )

target_include_directories(${PROJECT_NAME}
  PRIVATE "../../src"
  PRIVATE ${CAIRO_INCLUDE_DIRS}
)

target_link_directories(${PROJECT_NAME}
  PRIVATE ${CAIRO_LIBRARY_DIRS}
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE akcore
  PRIVATE akstate
  PRIVATE ${CAIRO_LIBRARIES}
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_precompile_headers(${PROJECT_NAME} PRIVATE
    ./pch.h
  )
endif()

# install(TARGETS ${PROJECT_NAME}
#   PUBLIC_HEADER DESTINATION include/lib${PROJECT_NAME}
# )
