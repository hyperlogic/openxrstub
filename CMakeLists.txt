cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
project(openxrstub LANGUAGES CXX)

set(PROJECT_NAME openxrstub)

if(WIN32)
    set(VCPKG_INCLUDE_DIR "$ENV{VCPKG_ROOT}/installed/x64-windows/include")
    set(VCPKG_LIB_DIR "$ENV{VCPKG_ROOT}/installed/x64-windows/lib")
    set(SDL2_LIBRARIES ${VCPKG_LIB_DIR}/SDL2.lib ${VCPKG_LIB_DIR}/manual-link/SDL2main.lib)
    include_directories($VCPKG_INCLUDE_DIR)
    set(OPENXR_LIBRARIES openxr_loader.lib)
else()
    find_package(SDL2 REQUIRED)
    find_package(OpenXR REQUIRED)
endif()

find_package(OpenGL REQUIRED)


add_executable(${PROJECT_NAME} src/main.cpp)

if(WIN32)
    # set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS /SUBSYSTEM:WINDOWS)
endif()

target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES} ${OPENGL_LIBRARIES} ${OPENXR_LIBRARIES})


