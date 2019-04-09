/*
	SObjectizer 5.
*/

/*!
 * \since
 * v.5.3.0
 *
 * \file 
 * \brief Detection of compiler version and absence of various features.
 */

#pragma once

#if defined( _MSC_VER )
	#define SO_5_MSVC
#endif

#if defined( __clang__ )
	#define SO_5_CLANG
#endif

//FIXME: is it really needed now?

// Try to use __has_cpp_attribute if it is supported.
#if defined(__has_cpp_attribute)
	// clang-4 and clang-5 produce warnings when [[nodiscard]]
	// is used with -std=c++11 and -std=c++14.
	#if __has_cpp_attribute(nodiscard) && \
			!(defined(SO_5_CLANG) && __cplusplus < 201703L)
		#define SO_5_NODISCARD [[nodiscard]]
	#endif
#endif

// Handle the result of __has_cpp_attribute.
#if !defined( SO_5_NODISCARD )
	#define SO_5_NODISCARD
#endif

