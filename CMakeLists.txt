# CMakeLists
cmake_minimum_required(VERSION 3.10)
project(OpenGLTest)

set(CMAKE_CXX_STANDARD 11)

include_directories(C:/Users/collet.mutangiri1/AppData/Local/msys64/mingw64/include)
link_directories(C:/Users/collet.mutangiri1/AppData/Local/msys64/mingw64/lib)

add_executable(app ./src/main.cpp)

target_link_libraries(app
    freeglut
    opengl32
    glu32
)