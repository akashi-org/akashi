project (akgraphics CXX)

find_package(SDL2 REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)
pkg_check_modules(GLM REQUIRED glm)

set(AKASHI_PRIV_GLAD_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../shared_temp/glad/)

add_library(${PROJECT_NAME} ${AKASHI_BUILD_LIB_TYPE}
  "./akgraphics.cpp"

  "./backend/opengl/context.cpp"
  "./backend/opengl/framebuffer.cpp"
  "./backend/opengl/render.cpp"
  "./backend/opengl/layer.cpp"
  "./backend/opengl/util.cpp"
  "./backend/opengl/core/loader.cpp"
  "./backend/opengl/core/shader.cpp"
  "./backend/opengl/core/buffer.cpp"
  "./backend/opengl/core/texture.cpp"
  "./backend/opengl/core/mvp.cpp"
  "./backend/opengl/resource/resource.cpp"
  "./backend/opengl/resource/image.cpp"
  "./backend/opengl/resource/font.cpp"
  "./backend/opengl/objects/quad/quad.cpp"
  "./backend/opengl/objects/quad/video_quad.cpp"
  "./backend/opengl/objects/quad/layer_quad.cpp"

  "./backend/ogl/context.cpp"
  "./backend/ogl/glcontext.cpp"
)

file(GLOB INTERFACE_HEADERS
  akgraphics.h
  item.h
)
set_target_properties(${PROJECT_NAME} PROPERTIES 
  PUBLIC_HEADER "${INTERFACE_HEADERS}" # [XXX] we need double quotes here!
)

target_compile_definitions(${PROJECT_NAME}
  PRIVATE ${SDL2_DEFINITIONS}
  PRIVATE ${SDL2_IMAGE_DEFINITIONS}
  PRIVATE ${SDL2_TTF_DEFINITIONS}
  PRIVATE ${GLM_DEFINITIONS}
)

target_include_directories(${PROJECT_NAME}
  PRIVATE "../../src"
  PRIVATE ${SDL2_INCLUDE_DIRS}
  PRIVATE ${SDL2_IMAGE_INCLUDE_DIRS}
  PRIVATE ${SDL2_TTF_INCLUDE_DIRS}
  PRIVATE ${GLM_INCLUDE_DIRS}
  PRIVATE ${AKASHI_PRIV_GLAD_ROOT_DIR}/include
)

target_link_directories(${PROJECT_NAME}
  PRIVATE ${SDL2_LIBRARY_DIRS}
  PRIVATE ${SDL2_IMAGE_LIBRARY_DIRS}
  PRIVATE ${SDL2_TTF_LIBRARY_DIRS}
  PRIVATE ${GLM_LIBRARY_DIRS}
  PUBLIC ${AKASHI_PRIV_GLAD_ROOT_DIR}/lib
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE akcore
  PRIVATE akbuffer
  PRIVATE akstate
  PRIVATE ${SDL2_LIBRARIES}
  PRIVATE ${SDL2_IMAGE_LIBRARIES}
  PRIVATE ${SDL2_TTF_LIBRARIES}
  PRIVATE ${GLM_LIBRARIES}
  PRIVATE EGL
  PRIVATE va
  PRIVATE glad
  INTERFACE dl
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_precompile_headers(${PROJECT_NAME} PRIVATE
    ./pch.h
  )
endif()

install(TARGETS ${PROJECT_NAME}
  PUBLIC_HEADER DESTINATION include/lib${PROJECT_NAME}
)

include(ExternalProject)
ExternalProject_Add(
  shared_glad
  URL "https://github.com/Dav1dde/glad/archive/refs/tags/v0.1.34.tar.gz"
  TIMEOUT 600
  URL_MD5 eea8f198923672b4be7a3c81e22076ac

  SOURCE_DIR ${CMAKE_BINARY_DIR}/.ext/glad/
  INSTALL_DIR ${AKASHI_PRIV_GLAD_ROOT_DIR}

  CONFIGURE_COMMAND cd <SOURCE_DIR> && mkdir -p build && mkdir -p <INSTALL_DIR> && cd build && cmake .. -DBUILD_SHARED_LIBS=OFF -DGLAD_INSTALL=ON -DGLAD_ALL_EXTENSIONS=OFF -DGLAD_REPRODUCIBLE=OFF -DHAS_EGL=ON -DGLAD_PROFILE="core" -DGLAD_API="gl=4.2" -DGLAD_EXTENSIONS="GL_EXT_texture_compression_s3tc" -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -GNinja
  BUILD_COMMAND cd <SOURCE_DIR>/build && ninja -j4
  INSTALL_COMMAND cd <SOURCE_DIR>/build && ninja install
)

add_dependencies(${PROJECT_NAME} shared_glad)
