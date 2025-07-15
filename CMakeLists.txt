cmake_minimum_required(VERSION 3.25)
project(CA)

set(CMAKE_CXX_STANDARD 11)

add_executable(CA main.cpp cmake-build-debug/render.cpp)

set(SFML_STATIC_LIBRARIES TRUE)
set(SFML_DIR C:/SFML/lib/cmake/SFML)
find_package(SFML COMPONENTS system window graphics audio REQUIRED)

include_directories(c:/SFML/include)
target_link_libraries(CA sfml-system sfml-window sfml-graphics sfml-audio)