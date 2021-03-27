project (akcodec-test CXX)

add_executable(${PROJECT_NAME}
  "./backend/ffmpeg/test_transcode.cpp"
)
target_include_directories(${PROJECT_NAME}
  PUBLIC ${CMAKE_SOURCE_DIR}/shared_temp/catch2/include/catch2/
  PUBLIC "../../../src"
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_precompile_headers(${PROJECT_NAME} PRIVATE
    pch.h
  )
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE
  CATCH_CONFIG_FAST_COMPILE
  CATCH_CONFIG_DISABLE_MATCHERS
  CATCH_CONFIG_ENABLE_BENCHMARKING
)
target_link_libraries(${PROJECT_NAME}
  PUBLIC Catch2::Catch2
  PUBLIC aktest
  PUBLIC akcodec
  PUBLIC akcore
  PUBLIC akbuffer
)

include(CTest)
# include(Catch)
include(${CMAKE_SOURCE_DIR}/shared_temp/catch2/contrib/Catch.cmake)
catch_discover_tests(${PROJECT_NAME} WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
