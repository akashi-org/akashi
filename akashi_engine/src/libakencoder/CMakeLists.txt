project (akencoder CXX)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_library(${PROJECT_NAME} ${AKASHI_BUILD_LIB_TYPE}
  ./akencoder.cpp
  ./producer/producer.cpp
  ./consumer/consumer.cpp
)

file(GLOB INTERFACE_HEADERS
  akencoder.h
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
  PRIVATE Threads::Threads
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

