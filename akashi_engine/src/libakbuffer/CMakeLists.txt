project (akbuffer CXX)

add_library(${PROJECT_NAME} ${AKASHI_BUILD_LIB_TYPE}
  "./avbuffer.cpp"
  "./video_queue.cpp"
  "./audio_queue.cpp"
)

file(GLOB INTERFACE_HEADERS
  buffer.h
  avbuffer.h
  video_queue.h
  audio_queue.h
)
set_target_properties(${PROJECT_NAME} PROPERTIES 
  PUBLIC_HEADER "${INTERFACE_HEADERS}" # [XXX] we need double quotes here!
)

target_include_directories(${PROJECT_NAME}
  PRIVATE "../../src"
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE akcore
  PRIVATE akstate
  PRIVATE atomic
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
