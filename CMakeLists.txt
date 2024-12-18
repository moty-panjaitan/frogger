cmake_minimum_required(VERSION 3.20)

project(frogger VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})

file(GLOB_RECURSE HEADERS "${CMAKE_SOURCE_DIR}/src/*.hpp")
file(GLOB_RECURSE SOURCES "${CMAKE_SOURCE_DIR}/src/*.cc")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(DEBUG)
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_definitions(RELEASE)
endif()

add_compile_options(-Wall -Wextra -Wpedantic -Werror)
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_link_libraries(${PROJECT_NAME} ${CURSES_LIBRARIES})

set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})