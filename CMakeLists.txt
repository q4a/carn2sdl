cmake_minimum_required(VERSION 3.7)

project(carn2sdl)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_VERBOSE_MAKEFILE ON)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "" FORCE)
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-D_DEBUG)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ARCH_TYPE x64)
else()
    set(ARCH_TYPE x86)
endif()

if (WIN32)
    set(SDL2_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/SDL2/include)
    set(SDL2TTF_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/SDL2_ttf/include)

    set(SDL2_LIBRARY_DIRS ${PROJECT_SOURCE_DIR}/SDL2/lib/${ARCH_TYPE})
    set(SDL2TFF_LIBRARY_DIRS ${PROJECT_SOURCE_DIR}/SDL2_ttf/lib/${ARCH_TYPE})

    set(SDL2_LIBRARIES SDL2.lib SDL2main.lib)
    set(SDL2TTF_LIBRARIES SDL2_ttf.lib)
else()
    if (ARCH_TYPE STREQUAL "x64")
        add_definitions(-D_W64)
    endif()

    include(FindPkgConfig)
    PKG_SEARCH_MODULE(SDL2 REQUIRED sdl2)
    #PKG_SEARCH_MODULE(SDL2IMAGE REQUIRED SDL2_image>=2.0.0)
    PKG_SEARCH_MODULE(SDL2TTF REQUIRED SDL2_ttf>=2.0.0)
endif()

include_directories(${SDL2_INCLUDE_DIRS} ${SDL2TTF_INCLUDE_DIRS})
link_directories(${SDL2_LIBRARY_DIRS} ${SDL2TFF_LIBRARY_DIRS})

add_executable(carn2sdl Menu2/Hunt2.cpp Menu2/Menu.cpp Menu2/Resources.cpp Menu2/Menu2.rc)
target_link_libraries(carn2sdl ${SDL2_LIBRARIES} ${SDL2TTF_LIBRARIES})
