project (akdebug CXX)

add_subdirectory("./imgui")

add_library(${PROJECT_NAME} ${AKASHI_BUILD_LIB_TYPE}
  "./akdebug.cpp"
)

# target_compile_definitions(${PROJECT_NAME}
# )

target_include_directories(${PROJECT_NAME}
  PRIVATE "../../src"
)

# target_link_directories(${PROJECT_NAME}
# )

target_link_libraries(${PROJECT_NAME}
  PRIVATE akcore
  PRIVATE akstate
  PRIVATE imgui
  PUBLIC ${AKASHI_PRIV_SHARED_DEPS}_glad
  PUBLIC ${AKASHI_PRIV_SHARED_DEPS}_glfw
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_precompile_headers(${PROJECT_NAME} PRIVATE
    ./pch.h
  )
endif()

install(TARGETS ${PROJECT_NAME}
  PUBLIC_HEADER DESTINATION include/lib${PROJECT_NAME}
)
