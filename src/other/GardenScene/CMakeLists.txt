cmake_minimum_required(VERSION 3.10)
project(GardenScene)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(C:/Users/collet.mutangiri1/AppData/Local/msys64/mingw64/include)
link_directories(C:/Users/collet.mutangiri1/AppData/Local/msys64/mingw64/lib)

add_executable(GardenScene main.cpp)

target_link_libraries(GardenScene
    freeglut
    opengl32
    glu32
)
