project (akevent CXX)

add_library(${PROJECT_NAME} ${AKASHI_BUILD_LIB_TYPE}
  ./akevent.cpp
)

target_include_directories(${PROJECT_NAME}
  PRIVATE "../../src"
)

file(GLOB INTERFACE_HEADERS
  akevent.h
)
set_target_properties(${PROJECT_NAME} PROPERTIES 
  PUBLIC_HEADER "${INTERFACE_HEADERS}" # [XXX] we need double quotes here!
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE akcore
  PRIVATE akstate
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_precompile_headers(${PROJECT_NAME} PRIVATE
    ./pch.h
  )
endif()

install(TARGETS ${PROJECT_NAME}
  PUBLIC_HEADER DESTINATION include/lib${PROJECT_NAME}
)
