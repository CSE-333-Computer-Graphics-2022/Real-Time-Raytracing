cmake_minimum_required(VERSION 3.0.2)

project(RayTracing_OpenGL)

SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules")
set(ASSETS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets")

add_definitions("-DASSETS_DIR=\"${ASSETS_DIR}\"")

set(X11_LIBS "")

find_package(OpenGL REQUIRED)
find_package(GLFW REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(GLEW REQUIRED)


add_subdirectory(external_sources/glad)

include_directories(
    ${OPENGL_INCLUDE_DIR}
    ${GLFW_INCLUDE_DIR}
    "${CMAKE_SOURCE_DIR}/common"
    "${CMAKE_SOURCE_DIR}/external_sources/glm"
    "${CMAKE_SOURCE_DIR}/external_sources/stb_image"
)

file(GLOB src
    common/*.cpp
    src/*.cpp
    src/*.h
    external_sources/stb_image/*.cpp
)

add_executable("rt"
    ${src}
)

target_link_libraries("rt"
    PRIVATE ${OPENGL_LIBRARIES}
    PRIVATE ${GLFW_LIBRARY}
    PRIVATE ${X11_LIBS}
    PRIVATE ${CMAKE_DL_LIBS}
    PRIVATE glad-interface
)

set_target_properties("rt"
    PROPERTIES
    OUTPUT_NAME "rt"
    RUNTIME_OUTPUT_DIRECTORY "rt"
    FOLDER "src")
