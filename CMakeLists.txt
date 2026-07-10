cmake_minimum_required(VERSION 3.28)

project(framer7b-c)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} SRC)

if(DEFINED ENV{ZEPHYR_BASE})
    target_sources(app PRIVATE ${SRC})
    target_include_directories(app PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
else()
    add_library(framer7b-c STATIC)
    target_sources(framer7b-c PRIVATE ${SRC})
    target_include_directories(framer7b-c PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

    target_compile_options(framer7b-c PRIVATE -Werror -Wno-attributes)

    option(BUILD_TESTS "Build tests" OFF)
    if(BUILD_TESTS)
        add_subdirectory(TEST)
    endif()
endif()
