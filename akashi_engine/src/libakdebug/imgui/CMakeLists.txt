project (imgui CXX)

add_library(${PROJECT_NAME} SHARED
  ./src/imgui.cpp
  ./src/imgui_draw.cpp
  ./src/imgui_tables.cpp
  ./src/imgui_widgets.cpp
  ./src/imgui_impl_glfw.cpp
  ./src/imgui_impl_opengl3.cpp
  ./src/imgui_stdlib.cpp
  
  # ./src/imgui_demo.cpp
)

target_compile_options(${PROJECT_NAME}
  PRIVATE -O3
)


target_include_directories(${PROJECT_NAME}
  PUBLIC "./include"
)
