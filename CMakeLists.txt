cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "iOS" OR IOS)
    set(CMAKE_OSX_ARCHITECTURES "arm64")
else()
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
endif()

set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(LuaTrigger VERSION 1.0.0)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)

add_library(${PROJECT_NAME} SHARED ${SOURCES})

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

setup_geode_mod(${PROJECT_NAME})

CPMAddPackage("gh:lua/lua#v5.4.8")

file(GLOB LUA_SOURCES CONFIGURE_DEPENDS
    ${lua_SOURCE_DIR}/*.c
)

add_library(lua STATIC ${LUA_SOURCES})

target_include_directories(lua PUBLIC
    ${lua_SOURCE_DIR}
)

target_compile_definitions(lua PRIVATE
    "l_system(cmd)=(0)"
)

target_link_libraries(${PROJECT_NAME}
    lua
)

target_include_directories(${PROJECT_NAME} PUBLIC
    include
)