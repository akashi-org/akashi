project (akaudio CXX)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBPULSE REQUIRED libpulse)

add_library(${PROJECT_NAME} ${AKASHI_BUILD_LIB_TYPE}
  "./akaudio.cpp"

  "./backend/pulseaudio/context.cpp"
  "./backend/pulseaudio/callback.cpp"
  "./backend/pulseaudio/callback_context.cpp"
  "./backend/pulseaudio/stream.cpp"
  "./backend/pulseaudio/mixer.cpp"
  "./backend/pulseaudio/util.cpp"
)

file(GLOB INTERFACE_HEADERS
  akaudio.h
)
set_target_properties(${PROJECT_NAME} PROPERTIES 
  PUBLIC_HEADER "${INTERFACE_HEADERS}" # [XXX] we need double quotes here!
)

target_compile_definitions(${PROJECT_NAME} 
  PRIVATE ${LIBPULSE_DEFINITIONS}
)

target_include_directories(${PROJECT_NAME}
  PRIVATE "../../src"
)

target_link_directories(${PROJECT_NAME}
  PRIVATE ${LIBPULSE_LIBRARY_DIRS}
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE atomic
  PRIVATE akcore
  PRIVATE akbuffer
  PRIVATE akevent
  PRIVATE akstate
  PRIVATE ${LIBPULSE_LIBRARIES}
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_precompile_headers(${PROJECT_NAME} PRIVATE
    ./pch.h
  )
endif()

install(TARGETS ${PROJECT_NAME}
  PUBLIC_HEADER DESTINATION include/lib${PROJECT_NAME}
)

if(AKASHI_BUILD_TESTS)
  add_subdirectory("./test")
endif()
