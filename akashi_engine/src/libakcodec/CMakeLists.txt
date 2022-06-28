project (akcodec CXX)

find_package(PkgConfig REQUIRED)

pkg_check_modules(AVDEVICE REQUIRED libavdevice)
pkg_check_modules(AVFORMAT REQUIRED libavformat)
pkg_check_modules(AVCODEC REQUIRED libavcodec)
pkg_check_modules(SWRESAMPLE REQUIRED libswresample)
pkg_check_modules(SWSCALE REQUIRED libswscale)
pkg_check_modules(AVUTIL REQUIRED libavutil)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_library(${PROJECT_NAME} ${AKASHI_BUILD_LIB_TYPE}
  "./decoder.cpp"
  "./encoder.cpp"
  "./source.cpp"
  "./backend/ffmpeg/source.cpp"
  "./backend/ffmpeg/sink.cpp"
  "./backend/ffmpeg/hwdec.cpp"
  "./backend/ffmpeg/buffer.cpp"
  "./backend/ffmpeg/utils.cpp"
  "./backend/ffmpeg/pts.cpp"
)

file(GLOB INTERFACE_HEADERS
  akcodec.h
  decoder.h
  decode_item.h
)
set_target_properties(${PROJECT_NAME} PROPERTIES 
  PUBLIC_HEADER "${INTERFACE_HEADERS}" # [XXX] we need double quotes here!
)

target_compile_definitions(${PROJECT_NAME}
  PRIVATE "__STDC_CONSTANT_MACROS" # ffmpeg needs this
)

target_include_directories(${PROJECT_NAME}
  PRIVATE "../../src"
)

target_link_directories(${PROJECT_NAME}
  PRIVATE ${AVDEVICE_LIBRARY_DIRS}
  PRIVATE ${AVFORMAT_LIBRARY_DIRS}
  PRIVATE ${AVCODEC_LIBRARY_DIRS}
  PRIVATE ${SWRESAMPLE_LIBRARY_DIRS}
  PRIVATE ${SWSCALE_LIBRARY_DIRS}
  PRIVATE ${AVUTIL_LIBRARY_DIRS}
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE akcore
  PRIVATE akbuffer
  PRIVATE akstate # for akencoder
  PRIVATE Threads::Threads
  PRIVATE ${AVDEVICE_LIBRARIES}
  PRIVATE ${AVFORMAT_LIBRARIES}
  PRIVATE ${AVCODEC_LIBRARIES}
  PRIVATE ${SWRESAMPLE_LIBRARIES}
  PRIVATE ${SWSCALE_LIBRARIES}
  PRIVATE ${AVUTIL_LIBRARIES}
  PRIVATE va
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_precompile_headers(${PROJECT_NAME} PRIVATE
    ./pch.h
  )
endif()

install(TARGETS ${PROJECT_NAME}
  PUBLIC_HEADER DESTINATION include/lib${PROJECT_NAME}
)

set(AKPROBE_TARGET "akprobe")

add_library(${AKPROBE_TARGET} SHARED
  "./backend/ffmpeg/probe.cpp"
)

target_compile_definitions(${AKPROBE_TARGET}
  PRIVATE "__STDC_CONSTANT_MACROS" # ffmpeg needs this
)

target_include_directories(${AKPROBE_TARGET}
  PRIVATE "../../src"
)

target_link_directories(${AKPROBE_TARGET}
  PRIVATE ${AVFORMAT_LIBRARY_DIRS}
  PRIVATE ${AVUTIL_LIBRARY_DIRS}
)

target_link_libraries(${AKPROBE_TARGET}
  PRIVATE ${AVFORMAT_LIBRARIES}
  PRIVATE ${AVUTIL_LIBRARIES}
)


if(AKASHI_BUILD_TESTS)
  add_subdirectory("./test")
endif()
