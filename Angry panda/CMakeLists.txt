cmake_minimum_required(VERSION 3.20)
project(AngryPanda LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -------------------------------
# Platform-specific SDL2 setup
# -------------------------------
if(WIN32)
    # On Windows, assume vcpkg is used
    if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
        message(FATAL_ERROR "Please configure CMAKE_TOOLCHAIN_FILE to point to vcpkg.cmake")
    endif()
    find_package(SDL2 CONFIG REQUIRED)
else()
    # On Linux / WSL, use system-installed SDL2
    find_package(SDL2 REQUIRED)
endif()

# -------------------------------
# Executable
# -------------------------------
add_executable(AngryPanda
    src/main.cpp
    src/game.cpp
    src/input.cpp
    src/player.cpp
)

target_include_directories(AngryPanda PRIVATE src)
target_link_libraries(AngryPanda PRIVATE SDL2::SDL2 SDL2::SDL2main)