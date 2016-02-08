cmake_minimum_required(VERSION 2.8)

option(BUILD_ALL      "Enable building examples and tests [default: OFF]" OFF)
option(BUILD_EXAMPLES "Enable building examples [default: OFF]"           OFF)
option(BUILD_TESTS    "Enable building tests    [default: OFF]"           OFF)

if (ANDROID AND NOT CRYSTAX)
    message(FATAL_ERROR "You should use CrystaX-enabled CMake toolchain")
endif()

if (ANDROID)
    if (ANDROID_NDK_ABI_NAME STREQUAL "armeabi")
        message(FATAL_ERROR "'${ANDROID_NDK_ABI_NAME}' target is not supported due to lack of the lock-free atomics")
    endif()
    if (ANDROID_NDK_ABI_NAME STREQUAL "mips")
        message(FATAL_ERROR "'${ANDROID_NDK_ABI_NAME}' target is not supported due to lack of the lock-free 64-bit atomics")
    endif()
    if (NOT ANDROID_NDK_ABI_NAME MATCHES "^(armeabi-v7a|armeabi-v7a-hard|arm64-v8a|x86|x86_64|mips64)$")
        message(FATAL_ERROR "Unsupported ABI: '${ANDROID_NDK_ABI_NAME}'")
    endif()
endif()

# Extract SO_5_VERSION from version.rb
set(SO_5_VERSION "")

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/so_5/version.rb" 
     _SO_5_VERSION_RB_CONTENTS 
     REGEX "^[ \t]*VERSION[ \t]*=[ \t]*\"[0-9]+[.][0-9]+[.][0-9]+[^\"]*\"[ \t]*")

set(_SO_5_VERSION_REGEX "\"([0-9.]+)\"")
if("${_SO_5_VERSION_RB_CONTENTS}" MATCHES "^[ \t]*VERSION[ \t]*=[ \t]*${_SO_5_VERSION_REGEX}[ \t]*")
    set(SO_5_VERSION "${CMAKE_MATCH_1}")
else()
    message("Failed extract version information from ${CMAKE_CURRENT_SOURCE_DIR}/so_5/version.rb")
    message("_SO_5_VERSION_RB_CONTENTS :${_SO_5_VERSION_RB_CONTENTS}:")
    message(FATAL_ERROR "SO_5_VERSION \"${SO_5_VERSION}\"")
endif()

if ("x${CMAKE_CXX_COMPILER_ID}" MATCHES "xGNU" 
     OR "x${CMAKE_CXX_COMPILER_ID}" MATCHES "xClang"
     OR "x${CMAKE_CXX_COMPILER_ID}" MATCHES "xIntel"
)
    add_definitions(-std=c++11 -Wall)
    if (NOT ANDROID)
        add_definitions(-pthread -mtune=native -march=native)
        set(CMAKE_EXE_LINKER_FLAGS "-pthread")
        set(CMAKE_SHARED_LINKER_FLAGS "-pthread")
    endif()
elseif (${MSVC})
    # message("Using MSVC compiler!!!")
else ()
    message(FATAL_ERROR "Your C++ compiler not supported.\nPlease mail me on san@masterspline.net Your compiler ID '${CMAKE_CXX_COMPILER_ID}' and this error message.")
endif ()

include_directories( ${CMAKE_CURRENT_LIST_DIR} )

add_subdirectory(so_5)
if(BUILD_ALL OR BUILD_EXAMPLES)
    add_subdirectory(sample/so_5)
endif()
if(BUILD_ALL OR BUILD_TESTS)
    enable_testing()
    add_subdirectory(test/so_5)
    if(WIN32)
        configure_file(${CMAKE_SOURCE_DIR}/cmake/run_tests.bat ${CMAKE_BINARY_DIR} NEWLINE_STYLE WIN32)
    endif()
endif()
