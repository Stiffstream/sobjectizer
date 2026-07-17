cmake_minimum_required(VERSION 3.14)

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

option(SOBJECTIZER_SANITIZE "Sanitizer to use [default: none]" none)

if((NOT SOBJECTIZER_BUILD_STATIC) AND (NOT SOBJECTIZER_BUILD_SHARED))
	message(FATAL_ERROR "at least one of SOBJECTIZER_BUILD_STATIC/SOBJECTIZER_BUILD_SHARED must be defined")
endif()

if(NOT SOBJECTIZER_BUILD_STATIC)
	if(BUILD_ALL OR BUILD_EXAMPLES)
		message(FATAL_ERROR "BUILD_ALL/BUILD_EXAMPLES can't be used"
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

set(SOBJECTIZER_SUPPORTED_SANITIZERS "none" "thread" "address")
if(NOT (SOBJECTIZER_SANITIZE IN_LIST SOBJECTIZER_SUPPORTED_SANITIZERS))
	message(FATAL_ERROR "Unsupported sanitizer: ${SOBJECTIZER_SANITIZE}")
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(NOT CMAKE_CXX_STANDARD)
	message(STATUS "C++17 will be used by default")
	set(CMAKE_CXX_STANDARD 17)
	set(CMAKE_CXX_STANDARD_REQUIRED ON)
	set(CXX_EXTENSIONS NO)
else()
	message(STATUS "C++ standard is set to C++${CMAKE_CXX_STANDARD}")
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	add_compile_options(
		-Wall
		-Wextra
		-Werror
		-Wpedantic
		-Wno-c++98-compat
		-Wno-c++98-compat-pedantic
		-Wno-padded
		-Wno-missing-noreturn
		-Wno-documentation-unknown-command
		-Wno-documentation-deprecated-sync
		-Wno-documentation
		-Wno-weak-vtables
		-Wno-missing-prototypes
		-Wno-missing-variable-declarations
		-Wno-exit-time-destructors
		-Wno-global-constructors
	)
	if(SOBJECTIZER_SANITIZE STREQUAL "thread")
		add_compile_options(-fsanitize=thread)
		add_link_options(-fsanitize=thread)
	elseif(SOBJECTIZER_SANITIZE STREQUAL "address")
		add_compile_options(-fsanitize=address)
		add_link_options(-fsanitize=address)
	endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	add_compile_options(
		-Wextra
		-Wpedantic
		-Werror
		-Wall
		-Wno-unused-function
	)
	if(SOBJECTIZER_SANITIZE STREQUAL "thread")
		add_compile_options(-fsanitize=thread)
		add_link_options(-fsanitize=thread)
	elseif(SOBJECTIZER_SANITIZE STREQUAL "address")
		add_compile_options(-fsanitize=address)
		add_link_options(-fsanitize=address)
	endif()
else()
	if(SOBJECTIZER_SANITIZE NOT STREQUAL "none")
		message(FATAL_ERROR "Sanitizers supported for Clang and GCC only")
	endif()
endif()

add_subdirectory(so_5)
if(BUILD_ALL OR BUILD_TESTS)
	enable_testing()
	add_subdirectory(test/so_5)
	if(WIN32)
		configure_file(${CMAKE_SOURCE_DIR}/cmake/run_tests.bat ${CMAKE_BINARY_DIR} NEWLINE_STYLE WIN32)
	endif()
endif()
if(BUILD_ALL OR BUILD_EXAMPLES)
	add_subdirectory(sample/so_5)
endif()

