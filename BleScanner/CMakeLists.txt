cmake_minimum_required(VERSION 3.14)
project(BleScanner VERSION 1.0 LANGUAGES CXX)

#----------------------------------------
# 1) C++ Settings
#----------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#----------------------------------------
# 2) Root of external libraries (override via CMake command-line)
#----------------------------------------
#     e.g.: cmake -S . -B build -DLIB_ROOT=/path/to/Libs
set(LIB_ROOT "${CMAKE_CURRENT_LIST_DIR}/Libs"
        CACHE PATH "Root directory of third-party libraries")

#----------------------------------------
# 3) mbedTLS
#----------------------------------------
add_subdirectory(${LIB_ROOT}/mbedtls-3.6.0)

#----------------------------------------
# 4) GLFW (disable examples, tests, and documentation)
#----------------------------------------
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
add_subdirectory(${LIB_ROOT}/glfw)

#----------------------------------------
# 5) ImGui as a standalone static library
#----------------------------------------
set(IMGUI_DIR "${LIB_ROOT}/imgui")
file(GLOB IMGUI_SOURCES
        ${IMGUI_DIR}/*.cpp
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
)
target_link_libraries(imgui PUBLIC glfw)

#----------------------------------------
# 6) Application
#----------------------------------------
# We don't load build outputs, but rather the source files from /src
file(GLOB APP_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp
)

add_executable(${PROJECT_NAME}
        ${APP_SOURCES}
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
)

# Definitions and libraries
target_compile_definitions(${PROJECT_NAME} PRIVATE
        _WIN32_WINNT=0x0A00
)
target_link_libraries(${PROJECT_NAME} PRIVATE
        imgui
        glfw
        opengl32               # Windows OpenGL
        MbedTLS::mbedtls       # from add_subdirectory(mbedtls-3.6.0)
        MbedTLS::mbedcrypto
        MbedTLS::mbedx509
        windowsapp             # if you need WinRT, etc.
)
