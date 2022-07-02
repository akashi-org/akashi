project (akgraphics CXX)

find_package(SDL2 REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)
pkg_check_modules(GLM REQUIRED glm)

add_library(${PROJECT_NAME} ${AKASHI_BUILD_LIB_TYPE}
  "./akgraphics.cpp"
  "./osc.cpp"

  "./backend/opengl/context.cpp"
  "./backend/opengl/render_context.cpp"
  "./backend/opengl/fbo.cpp"
  "./backend/opengl/stage.cpp"
  "./backend/opengl/camera.cpp"
  "./backend/opengl/core/shader.cpp"
  "./backend/opengl/core/color.cpp"
  "./backend/opengl/core/error.cpp"
  "./backend/opengl/core/texture.cpp"
  "./backend/opengl/core/loader_gl.cpp"
  "./backend/opengl/core/loader_egl.cpp"
  "./backend/opengl/meshes/mesh.cpp"
  "./backend/opengl/meshes/quad.cpp"
  "./backend/opengl/meshes/rect.cpp"
  "./backend/opengl/meshes/circle.cpp"
  "./backend/opengl/meshes/triangle.cpp"
  "./backend/opengl/meshes/line.cpp"
  "./backend/opengl/resource/image.cpp"
  "./backend/opengl/resource/font.cpp"
  # "./backend/opengl/utility/grid.cpp"
  "./backend/opengl/actors/actor.cpp"
  "./backend/opengl/actors/video_actor.cpp"
  "./backend/opengl/actors/video_actor_texture.cpp"
  "./backend/opengl/actors/image_actor.cpp"
  "./backend/opengl/actors/text_actor.cpp"
  "./backend/opengl/actors/unit_actor.cpp"
  "./backend/opengl/actors/shape_actor.cpp"
  "./backend/opengl/hwaccel/vaapi_encode.cpp"

  "./backend/opengl/osc/osc_root.cpp"
  "./backend/opengl/osc/osc_render_context.cpp"
  "./backend/opengl/osc/osc_camera.cpp"
  "./backend/opengl/osc/widgets/widget.cpp"
  "./backend/opengl/osc/widgets/play_btn.cpp"
  "./backend/opengl/osc/widgets/volume_widget.cpp"
  "./backend/opengl/osc/widgets/timecode.cpp"
  "./backend/opengl/osc/widgets/fseek_btn.cpp"
  "./backend/opengl/osc/widgets/zoom_btn.cpp"
  "./backend/opengl/osc/widgets/seek_widget/seek_widget.cpp"
  "./backend/opengl/osc/widgets/seek_widget/seek_handle.cpp"
  "./backend/opengl/osc/widgets/seek_widget/seek_ruler.cpp"
  "./backend/opengl/osc/widgets/seek_widget/seek_rail.cpp"
  "./backend/opengl/osc/common/text_label.cpp"
  "./backend/opengl/osc/common/volume_gauge.cpp"
  "./backend/opengl/osc/common/bg_rect.cpp"
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
  PRIVATE "GLM_ENABLE_EXPERIMENTAL"
)

target_include_directories(${PROJECT_NAME}
  PRIVATE "../../src"
  PRIVATE ${SDL2_INCLUDE_DIRS}
  PRIVATE ${SDL2_IMAGE_INCLUDE_DIRS}
  PRIVATE ${SDL2_TTF_INCLUDE_DIRS}
  PRIVATE ${GLM_INCLUDE_DIRS}
)

target_link_directories(${PROJECT_NAME}
  PRIVATE ${SDL2_LIBRARY_DIRS}
  PRIVATE ${SDL2_IMAGE_LIBRARY_DIRS}
  PRIVATE ${SDL2_TTF_LIBRARY_DIRS}
  PRIVATE ${GLM_LIBRARY_DIRS}
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE akcore
  PRIVATE akbuffer
  PRIVATE akstate
  PRIVATE akvgfx
  PRIVATE ${SDL2_LIBRARIES}
  PRIVATE ${SDL2_IMAGE_LIBRARIES}
  PRIVATE ${SDL2_TTF_LIBRARIES}
  PRIVATE ${GLM_LIBRARIES}
  PRIVATE EGL
  PRIVATE va
  PUBLIC ${AKASHI_PRIV_SHARED_DEPS}_glad
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_precompile_headers(${PROJECT_NAME} PRIVATE
    ./pch.h
  )
endif()

install(TARGETS ${PROJECT_NAME}
  PUBLIC_HEADER DESTINATION include/lib${PROJECT_NAME}
)
