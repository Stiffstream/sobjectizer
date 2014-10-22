#OPTION(DEBUG_MODE               "Enable debug output [default: OFF]"             OFF)
#OPTION(ENABLE_OPTIMIZATION      "Enable optimization [default: OFF]"             ON)

cmake_minimum_required(VERSION 2.8)

set(SO_5_VERSION 5.5.1)

if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU" 
     OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang"
     OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "Intel"
)
	add_definitions(-std=c++11 -pthread -mtune=native -march=native)
	set(CMAKE_EXE_LINKER_FLAGS "-pthread")
	set(CMAKE_SHARED_LINKER_FLAGS "-pthread")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	# message("Using MSVC compiler")
else ()
    message(FATAL_ERROR "Your C++ compiler does not supported.\nPlease mail me on san@masterspline.net Your compiler ID '${CMAKE_CXX_COMPILER_ID}' and this error message.")
endif ()

include_directories( ${CMAKE_CURRENT_LIST_DIR} )

add_subdirectory(so_5)
add_subdirectory(sample/so_5) # EXCLUDE_FROM_ALL)
#add_subdirectory(test/so_5)   # EXCLUDE_FROM_ALL)
