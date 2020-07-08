cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
project(openxrstub LANGUAGES CXX)

set(PROJECT_NAME openxrstub)

find_package(OpenGL REQUIRED)
include_directories(${GL_INCLUDE_DIRS})

find_package(GLEW REQUIRED)

find_package(SDL2 REQUIRED)
get_target_property(SDL2_INCLUDE_DIRS SDL2::SDL2 INTERFACE_INCLUDE_DIRECTORIES)
include_directories(${SDL2_INCLUDE_DIRS})

# find_package(openxr_loader REQUIRED)

set(OPENXR_LIBRARIES ${_VCPKG_INSTALLED_DIR}/${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}-${_VCPKG_TARGET_TRIPLET_PLAT}/lib/openxr_loader.lib)

add_executable(${PROJECT_NAME} src/main.cpp)

if(WIN32)
    # set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS /SUBSYSTEM:WINDOWS)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE ${OPENGL_LIBRARIES} ${OPENXR_LIBRARIES} SDL2::SDL2 SDL2::SDL2main GLEW::GLEW)


