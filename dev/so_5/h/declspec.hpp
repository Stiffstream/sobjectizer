/*
	SObjectizer 5.
*/

#pragma once

#if defined( _MSC_VER )
/*
** Visual C++
*/
#define SO_5_EXPORT __declspec(dllexport)
#define SO_5_IMPORT __declspec(dllimport)

#endif /* _MSC_VER */

#if defined( __MINGW32__ ) || defined( __WINGW64__ )
/*
** MinGW C++
*/
#define SO_5_EXPORT __declspec(dllexport)
#define SO_5_IMPORT __declspec(dllimport)

#endif /* __MINGW32__ */

// For the cases when export/import not needed.
#if !defined( SO_5_EXPORT )
	#define SO_5_EXPORT
#endif
#if !defined( SO_5_IMPORT )
	#define SO_5_IMPORT
#endif

#if defined( SO_5_PRJ )
	#define SO_5_TYPE SO_5_EXPORT

	#define SO_5_FUNC SO_5_EXPORT

#else
	#define SO_5_TYPE SO_5_IMPORT

	#define SO_5_FUNC SO_5_IMPORT

#endif

