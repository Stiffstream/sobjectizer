/*
	SObjectizer 5.
*/

#if !defined( _SO_5__DECLSPEC_HPP_ )
#define _SO_5__DECLSPEC_HPP_

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

#if defined( SO_5_PRJ )
	#define SO_5_TYPE SO_5_EXPORT

	#define SO_5_FUNC SO_5_EXPORT

#else
	#define SO_5_TYPE SO_5_IMPORT

	#define SO_5_FUNC SO_5_IMPORT

#endif

#endif
