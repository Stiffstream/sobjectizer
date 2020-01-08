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

