project (aktest CXX)

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)
FetchContent_Declare(catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v2.13.1
  GIT_SHALLOW TRUE
  GIT_PROGRESS TRUE
)
FetchContent_GetProperties(catch2)
if(NOT catch2_POPULATED)
  FetchContent_Populate(catch2)
  set(CATCH_INSTALL_DOCS OFF CACHE BOOL "") # default: ON
  # set(CATCH_INSTALL_EXTRAS OFF CACHE BOOL "") # default: ON
  # set(CATCH_DEVELOPMENT_BUILD OFF CACHE BOOL "") # default: OFF
  add_subdirectory(${catch2_SOURCE_DIR} ${catch2_BINARY_DIR})
  file(COPY ${catch2_SOURCE_DIR}/single_include/ DESTINATION ${CMAKE_SOURCE_DIR}/shared_temp/catch2/include/)
  file(COPY ${catch2_SOURCE_DIR}/contrib DESTINATION ${CMAKE_SOURCE_DIR}/shared_temp/catch2/)
  file(COPY ${catch2_SOURCE_DIR}/LICENSE.txt DESTINATION ${CMAKE_BINARY_DIR}/../vendor/catch2/)
endif()

# For applyling pch to Catch2, we need to do some hacks
# http://mochan.info/c++/2019/11/12/pre-compiled-headers-gcc-clang-cmake.html
# 
# https://github.com/catchorg/Catch2/blob/devel/docs/ci-and-misc.md#precompiled-headers-pchs

add_library(${PROJECT_NAME} STATIC
  main.cpp
)

target_compile_definitions(${PROJECT_NAME} 
  PRIVATE CATCH_CONFIG_ENABLE_BENCHMARKING
  PRIVATE CATCH_CONFIG_NO_POSIX_SIGNALS # https://github.com/catchorg/Catch2/issues/2192
)

target_include_directories(${PROJECT_NAME}
  PUBLIC ${CMAKE_SOURCE_DIR}/shared_temp/catch2/include/catch2
)

target_link_libraries(${PROJECT_NAME}
  PUBLIC Catch2::Catch2
)
