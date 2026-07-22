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

#if !defined( SO_5_COMPILER_FEATURES_HPP )
#define SO_5_COMPILER_FEATURES_HPP

#if defined( _MSC_VER )
	#define SO_5_MSVC
#endif

#if defined( __clang__ )
	#define SO_5_CLANG
#endif

#endif

