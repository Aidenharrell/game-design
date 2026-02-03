cmake_minimum_required(VERSION 3.20)
project(AngryPanda LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# SDL2 configuration: expects SDL2 to be installed and discoverable by CMake
# If using vcpkg, set CMAKE_TOOLCHAIN_FILE accordingly when configuring.
find_package(SDL2 CONFIG REQUIRED)

add_executable(AngryPanda
    src/main.cpp
    src/game.cpp
    src/input.cpp
    src/player.cpp
)

target_include_directories(AngryPanda PRIVATE src)
target_link_libraries(AngryPanda PRIVATE SDL2::SDL2 SDL2::SDL2main)
