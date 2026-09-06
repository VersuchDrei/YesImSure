cmake_minimum_required(VERSION 3.21)
message("Using toolchain file ${CMAKE_TOOLCHAIN_FILE}.")

########################################################################################################################
## Define project
########################################################################################################################
project(
        YesImSure
        VERSION 1.8.0
        DESCRIPTION "Skips 'are you sure?' confirmation prompts in Skyrim's crafting menus."
        LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

include(GNUInstallDirs)

set(PROJECT_FRIENDLY_NAME "Yes I'm Sure")

configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.rc.in
        ${CMAKE_CURRENT_BINARY_DIR}/version.rc
        @ONLY)

set(headers
        include/AutoTOML.hpp)

set(sources
        src/Hooks.cpp
        src/main.cpp

        ${CMAKE_CURRENT_BINARY_DIR}/version.rc)

source_group(
        TREE ${CMAKE_CURRENT_SOURCE_DIR}
        FILES
        ${headers}
        ${sources})

########################################################################################################################
## CommonLibSSE-NG (git submodule: extern/CommonLibSSE, branch ng)
########################################################################################################################
## The dead colorglass vcpkg registry used previously could not ship a CommonLibSSE-NG new
## enough for Skyrim 1.7.99 / 1.7.104 (Aug 2026). CommonLibSSE-NG >= 7.0.0 is now consumed
## directly as a submodule, as recommended upstream.
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/extern/CommonLibSSE/CMakeLists.txt")
        message(FATAL_ERROR
                "CommonLibSSE-NG submodule is missing. Run:\n"
                "  git submodule update --init --recursive")
endif()

set(ENABLE_SKYRIM_SE ON CACHE BOOL "" FORCE)
set(ENABLE_SKYRIM_AE ON CACHE BOOL "" FORCE)
set(ENABLE_SKYRIM_VR OFF CACHE BOOL "" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SKSE_SUPPORT_XBYAK ON CACHE BOOL "" FORCE)

add_subdirectory(extern/CommonLibSSE CommonLibSSE EXCLUDE_FROM_ALL)
get_target_property(COMMONLIB_SRC_DIR CommonLibSSE SOURCE_DIR)
include(${COMMONLIB_SRC_DIR}/cmake/CommonLibSSE.cmake)

########################################################################################################################
## Configure target DLL
########################################################################################################################
find_package(tomlplusplus CONFIG REQUIRED)
find_package(xbyak CONFIG REQUIRED)

add_commonlibsse_plugin(${PROJECT_NAME}
        AUTHOR "VersuchDrei"
        USE_ADDRESS_LIBRARY
        SOURCES ${headers} ${sources})
add_library("${PROJECT_NAME}::${PROJECT_NAME}" ALIAS "${PROJECT_NAME}")

target_include_directories(${PROJECT_NAME}
        PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/src>
        $<INSTALL_INTERFACE:src>)

target_include_directories(${PROJECT_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

target_link_libraries(${PROJECT_NAME}
        PRIVATE
        tomlplusplus::tomlplusplus
        xbyak::xbyak)

target_precompile_headers(${PROJECT_NAME}
        PRIVATE
        src/PCH.h)

install(TARGETS ${PROJECT_NAME}
        DESTINATION "${CMAKE_INSTALL_LIBDIR}")
