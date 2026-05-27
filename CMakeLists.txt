cmake_minimum_required(VERSION 3.16)
project(BrushBound)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Binary output ke root project agar bisa akses shaders/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

# ── Sources ────────────────────────────────────────────────────
# glad.c ada di root project
set(SOURCES
    src/main.cpp
    ${CMAKE_SOURCE_DIR}/glad.c
)

add_executable(BrushBound ${SOURCES})

# ── Include directories ────────────────────────────────────────
target_include_directories(BrushBound PRIVATE
    src/
    ${CMAKE_SOURCE_DIR}/include        # glad/, GLFW/, glm/, KHR/
    ${CMAKE_SOURCE_DIR}/include/glad
    ${CMAKE_SOURCE_DIR}/include/GLFW
    ${CMAKE_SOURCE_DIR}/include/glm
)

# ── Find OpenGL ────────────────────────────────────────────────
find_package(OpenGL REQUIRED)

# ── GLFW ──────────────────────────────────────────────────────
# Coba system install dulu, fallback ke include/ lokal
find_package(glfw3 QUIET)
if(glfw3_FOUND)
    message(STATUS "Using system GLFW")
    target_link_libraries(BrushBound PRIVATE glfw)
else()
    message(STATUS "System GLFW not found, using local")
    find_library(GLFW_LIB glfw3 glfw
        HINTS
            ${CMAKE_SOURCE_DIR}/lib
            /usr/lib
            /usr/local/lib
            /usr/lib/x86_64-linux-gnu
    )
    if(GLFW_LIB)
        target_link_libraries(BrushBound PRIVATE ${GLFW_LIB})
    else()
        message(FATAL_ERROR "GLFW not found! Run: sudo apt install libglfw3-dev")
    endif()
endif()

# ── GLM (header-only) ─────────────────────────────────────────
find_package(glm QUIET)
if(glm_FOUND)
    target_link_libraries(BrushBound PRIVATE glm::glm)
    message(STATUS "Using system GLM")
else()
    message(STATUS "Using local GLM from include/glm")
endif()

# ── Link libraries ─────────────────────────────────────────────
target_link_libraries(BrushBound PRIVATE
    OpenGL::GL
    dl
    pthread
)

# ── Compile flags ──────────────────────────────────────────────
target_compile_options(BrushBound PRIVATE -O2 -Wall -Wno-unused-variable)

message(STATUS "")
message(STATUS "=== BrushBound Build Config ===")
message(STATUS "Source dir : ${CMAKE_SOURCE_DIR}")
message(STATUS "Binary dir : ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "glad.c     : ${CMAKE_SOURCE_DIR}/glad.c")
message(STATUS "includes   : ${CMAKE_SOURCE_DIR}/include")
message(STATUS "================================")