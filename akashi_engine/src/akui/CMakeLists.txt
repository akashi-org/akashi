project (akashi_renderer C CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

find_package(Qt5 COMPONENTS Widgets Gui REQUIRED)
# [TODO] prepare an option
find_package(Qt5 COMPONENTS Test REQUIRED)

qt5_add_resources(RESOURCES "resource.qrc")

add_executable(${PROJECT_NAME}
  renderer.cpp
  app.cpp
  window.cpp
  interface/asp_general.cpp
  interface/asp_media.cpp
  interface/asp_gui.cpp
  utils/widget.cpp
  container/ControlArea.cpp
  container/MonitorArea.cpp
  components/PlayerWidget/PlayerWidget.cpp
  components/SliderSection/SliderSection.cpp
  components/WidgetSection/WidgetSection.cpp
  ${UI_FILES}
  ${RESOURCES}
)

target_compile_definitions(${PROJECT_NAME}
  PRIVATE "QT_NO_KEYWORDS" # for avoiding issues with other libraries like Python.h, Boost
)


target_include_directories(${PROJECT_NAME}
  PRIVATE "../../src"
)

target_link_directories(${PROJECT_NAME}
  PRIVATE "${CMAKE_BINARY_DIR}/../vendor/lib"
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE Threads::Threads
  PRIVATE Qt5::Widgets
  PRIVATE Qt5::Gui
  PRIVATE Qt5::Test # for asp
  PRIVATE akcore
  PRIVATE akplayer
  PRIVATE akstate
  PRIVATE akevent
  PRIVATE akgraphics # [TODO] RenderParams, GetProcAddress
  PRIVATE akserver # for asp
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_precompile_headers(${PROJECT_NAME} PRIVATE
    pch.h
  )
endif()
