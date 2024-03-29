cmake_minimum_required(VERSION 3.8)

project(SObjectizer LANGUAGES CXX)

set(SOBJECTIZER_MASTER_PROJECT OFF)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
	set(SOBJECTIZER_MASTER_PROJECT ON)
endif()

option(BUILD_ALL      "Enable building examples and tests [default: OFF]" OFF)
option(BUILD_EXAMPLES "Enable building examples [default: OFF]"           OFF)
option(BUILD_TESTS    "Enable building tests    [default: OFF]"           OFF)

option(SOBJECTIZER_BUILD_STATIC "Build static SObjectizer library [default: ON]" ON)
option(SOBJECTIZER_BUILD_SHARED "Build shared SObjectizer library [default: ON]" ON)
option(SOBJECTIZER_INSTALL "Generate install target for SObjectizer"
	${SOBJECTIZER_MASTER_PROJECT})

if((NOT SOBJECTIZER_BUILD_STATIC) AND (NOT SOBJECTIZER_BUILD_SHARED))
	message(FATAL_ERROR "at least one of SOBJECTIZER_BUILD_STATIC/SOBJECTIZER_BUILD_SHARED must be defined")
endif()

if(NOT SOBJECTIZER_BUILD_STATIC)
	if(BUILD_ALL OR BUILD_EXAMPLES OR BUILD_TESTS)
		message(FATAL_ERROR "BUILD_ALL/BUILD_EXAMPLES/BUILD_TESTS can't be used"
			" if SOBJECTIZER_BUILD_STATIC is OFF")
	endif()
endif()

if(NOT SOBJECTIZER_BUILD_SHARED)
	if(BUILD_ALL OR BUILD_EXAMPLES OR BUILD_TESTS)
		message(FATAL_ERROR "BUILD_ALL/BUILD_EXAMPLES/BUILD_TESTS can't be used"
			" if SOBJECTIZER_BUILD_SHARED is OFF")
	endif()
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

if(${CMAKE_VERSION} VERSION_LESS "3.8.0")
	set(CMAKE_CXX_STANDARD 17)
	set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

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
