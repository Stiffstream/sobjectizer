/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief Definition of SO_5_VERSION macro
 *
 * \since v.5.5.19.3
 */

#pragma once

// The current version is 5.8.4
//
/*!
 * A number next to generation number in SObjectizer's version.
 *
 * If SObjectizer's version is 5.6.20.8 then SO_5_VERSION_MAJOR==6.
 *
 * When major version is changed some incompatibilities are introduced
 * in SObjectizer API or working principles.
 */
#define SO_5_VERSION_MAJOR 8ull

/*!
 * A number next to major version in SObjectizer's version.
 *
 * If SObjectizer's version is 5.6.20.8 then SO_5_VERSION_MINOR==20.
 *
 * When minor version is changed some new features are added to SObjectizer
 * but SObjectizer API is compatible on source-code level.
 */
#define SO_5_VERSION_MINOR 4ull

/*!
 * A number next to minor version in SObjectizer's version.
 *
 * If SObjectizer's version is 5.6.20.8 then SO_5_VERSION_PATCH==8.
 *
 * When patch number is changed some bug-fixes are made or some minor features
 * or API fixes are added to SObjectizer but SObjectizer API is compatible on
 * source-code level.
 */
#define SO_5_VERSION_PATCH 0ull

/*!
 * Helper macro for make single number representation of SObjectizer's version.
 *
 * It can be used that way:
 * \code
 * // Some feature is available only from 5.5.20.0.
 * #if SO_5_VERSION >= SO_5_VERSION_MAKE(5, 20, 0)
 * 	... // Some 5.5.20.0 (or above) specific code.
 * #endif
 * \endcode
 */
#define SO_5_VERSION_MAKE(major, minor, patch) \
	(((major) * 1000000ull) + \
	((minor) * 1000ull) + \
	(patch))

/*!
 * A single number representation of SObjectizer-5 version.
 *
 * For example it can be 5019003ull for SO-5.5.19.3.
 * Or 6001023ull for SO-5.6.1.23.
 */
#define SO_5_VERSION SO_5_VERSION_MAKE( \
	SO_5_VERSION_MAJOR,\
	SO_5_VERSION_MINOR,\
	SO_5_VERSION_PATCH)

