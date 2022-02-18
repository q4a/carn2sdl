cmake_minimum_required(VERSION 3.7)

include (FindPkgConfig)

project(Menu2SDL)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_VERBOSE_MAKEFILE ON)

PKG_SEARCH_MODULE(SDL2 REQUIRED sdl2)
#PKG_SEARCH_MODULE(SDL2IMAGE REQUIRED SDL2_image>=2.0.0)
PKG_SEARCH_MODULE(SDL2TTF REQUIRED SDL2_ttf>=2.0.0)

include_directories(${SDL2_INCLUDE_DIRS} ${SDL2TTF_INCLUDE_DIRS})
link_directories(${SDL2_LIBRARY_DIRS} ${SDL2TFF_LIBRARY_DIRS})

add_executable(Menu2SDL Menu2/Hunt2.cpp Menu2/Menu.cpp Menu2/Resources.cpp)
target_link_libraries(Menu2SDL ${SDL2_LIBRARIES} ${SDL2TTF_LIBRARIES}) 
