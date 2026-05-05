cmake_minimum_required(VERSION 3.20)
project(AngryPanda LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 CONFIG REQUIRED)

add_executable(AngryPanda
    src/main.cpp
    src/boss.cpp
    src/enemy.cpp
    src/game.cpp
    src/input.cpp
    src/player.cpp
)

file(COPY "${CMAKE_SOURCE_DIR}/assets"
     DESTINATION ${CMAKE_BINARY_DIR})

target_include_directories(AngryPanda PRIVATE src)
target_link_libraries(AngryPanda PRIVATE SDL2::SDL2main SDL2::SDL2)
