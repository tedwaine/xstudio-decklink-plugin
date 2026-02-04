cmake_minimum_required(VERSION 3.28 FATAL_ERROR)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/modules")
include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/macros.cmake")

project(xstudio_blackmagic_decklink VERSION 0.1 LANGUAGES CXX)

add_subdirectory(src)