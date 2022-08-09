include(ExternalProject)

# boost
add_library(${AKASHI_PRIV_SHARED_DEPS}_boost INTERFACE)

if(AKASHI_BOOST_TAR_PATH STREQUAL "")
  set(AKASHI_BOOST_REAL_TAR_PATH "https://boostorg.jfrog.io/artifactory/main/release/1.74.0/source/boost_1_74_0.tar.gz")
else()
  set(AKASHI_BOOST_REAL_TAR_PATH ${AKASHI_BOOST_TAR_PATH})
  message("-- Using Custom Boost File Path: " ${AKASHI_BOOST_REAL_TAR_PATH})
endif()

ExternalProject_Add(
  shared_boost
  URL ${AKASHI_BOOST_REAL_TAR_PATH}
  TIMEOUT 600
  URL_MD5 3c8fb92ce08b9ad5a5f0b35731ac2c8e

  SOURCE_DIR ${CMAKE_BINARY_DIR}/.ext/boost/
  INSTALL_DIR ${AKASHI_PRIV_SHARED_DEPS_ROOT}/boost

  CONFIGURE_COMMAND ""
  BUILD_COMMAND cd <SOURCE_DIR> && ./bootstrap.sh && ./b2 install -j4 --with-filesystem --prefix=<INSTALL_DIR> link=static threading=multi variant=release
  INSTALL_COMMAND COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/vendor/credits/boost/
    COMMAND ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/LICENSE_1_0.txt ${CMAKE_SOURCE_DIR}/vendor/credits/boost/
)
add_dependencies(${AKASHI_PRIV_SHARED_DEPS}_boost shared_boost)
target_compile_definitions(${AKASHI_PRIV_SHARED_DEPS}_boost
  INTERFACE "BOOST_CHRONO_HEADER_ONLY"
)
target_include_directories(${AKASHI_PRIV_SHARED_DEPS}_boost
  INTERFACE ${AKASHI_PRIV_SHARED_DEPS_ROOT}/boost/include
)
target_link_directories(${AKASHI_PRIV_SHARED_DEPS}_boost
  INTERFACE ${AKASHI_PRIV_SHARED_DEPS_ROOT}/boost/lib
)


# glfw 
add_library(${AKASHI_PRIV_SHARED_DEPS}_glfw INTERFACE)

ExternalProject_Add(
  shared_glfw
  URL "https://github.com/glfw/glfw/archive/refs/tags/3.3.2.tar.gz"
  TIMEOUT 600
  URL_MD5 865e54ff0a100e9041a40429db98be0b

  SOURCE_DIR ${CMAKE_BINARY_DIR}/.ext/glfw/
  INSTALL_DIR ${AKASHI_PRIV_SHARED_DEPS_ROOT}/glfw

  CONFIGURE_COMMAND cd <SOURCE_DIR> && mkdir -p build && mkdir -p <INSTALL_DIR> && cd build && cmake .. -DBUILD_SHARED_LIBS=OFF -DGLFW_BUILD_EXAMPLES=OFF -DGLFW_BUILD_TESTS=OFF -DGLFW_BUILD_DOCS=OFF -DGLFW_INSTALL=ON -DGLFW_USE_WAYLAND=OFF -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -GNinja
  BUILD_COMMAND cd <SOURCE_DIR>/build && ninja -j4
  INSTALL_COMMAND COMMAND cd <SOURCE_DIR>/build && ninja install
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/vendor/credits/glfw/
    COMMAND ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/LICENSE.md ${CMAKE_SOURCE_DIR}/vendor/credits/glfw/
)
add_dependencies(${AKASHI_PRIV_SHARED_DEPS}_glfw shared_glfw)
target_include_directories(${AKASHI_PRIV_SHARED_DEPS}_glfw
  INTERFACE ${AKASHI_PRIV_SHARED_DEPS_ROOT}/glfw/include
)
target_link_directories(${AKASHI_PRIV_SHARED_DEPS}_glfw
  INTERFACE ${AKASHI_PRIV_SHARED_DEPS_ROOT}/glfw/lib
)
target_link_libraries(${AKASHI_PRIV_SHARED_DEPS}_glfw
  INTERFACE X11
  INTERFACE glfw3
)

# glad
add_library(${AKASHI_PRIV_SHARED_DEPS}_glad INTERFACE)

ExternalProject_Add(
  shared_glad
  URL "https://github.com/Dav1dde/glad/archive/refs/tags/v0.1.34.tar.gz"
  TIMEOUT 600
  URL_MD5 eea8f198923672b4be7a3c81e22076ac

  SOURCE_DIR ${CMAKE_BINARY_DIR}/.ext/glad/
  INSTALL_DIR ${AKASHI_PRIV_SHARED_DEPS_ROOT}/glad

  CONFIGURE_COMMAND cd <SOURCE_DIR> && mkdir -p build && mkdir -p <INSTALL_DIR> && cd build && cmake .. -DBUILD_SHARED_LIBS=OFF -DGLAD_INSTALL=ON -DGLAD_ALL_EXTENSIONS=OFF -DGLAD_REPRODUCIBLE=OFF -DHAS_EGL=ON -DGLAD_PROFILE="core" -DGLAD_API="gl=4.2" -DGLAD_EXTENSIONS="GL_EXT_texture_compression_s3tc,GL_ARB_debug_output" -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -GNinja
  BUILD_COMMAND cd <SOURCE_DIR>/build && ninja -j4
  INSTALL_COMMAND COMMAND cd <SOURCE_DIR>/build && ninja install
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/vendor/credits/glad/
    COMMAND ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/LICENSE ${CMAKE_SOURCE_DIR}/vendor/credits/glad/
)
add_dependencies(${AKASHI_PRIV_SHARED_DEPS}_glad shared_glad)
target_include_directories(${AKASHI_PRIV_SHARED_DEPS}_glad
  INTERFACE ${AKASHI_PRIV_SHARED_DEPS_ROOT}/glad/include
)
target_link_directories(${AKASHI_PRIV_SHARED_DEPS}_glad
  INTERFACE ${AKASHI_PRIV_SHARED_DEPS_ROOT}/glad/lib
)
target_link_libraries(${AKASHI_PRIV_SHARED_DEPS}_glad
  INTERFACE dl
  INTERFACE glad
)
