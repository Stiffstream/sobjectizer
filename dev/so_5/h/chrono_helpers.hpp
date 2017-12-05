/*
	SObjectizer 5.
*/

/*!
	\file
	\since
	v.5.3.0

	\brief Utilities for working with std::crhono tools.
	\deprecated Obsolete in v.5.5.0.
*/

#pragma once

#include <chrono>

#include <so_5/h/compiler_features.hpp>

namespace so_5
{

/*!
 * \since
 * v.5.3.0
 *
 * \brief Helper tools for std::chrono interaction.
 */
namespace chrono_helpers
{
	/*!
	 * \since
	 * v.5.3.0
	 *
	 * \brief Translation from std::chrono::duration to millisecons.
	 * \deprecated Obsolete in v.5.5.0
	 */
	template< class Rep, class Period >
	inline unsigned int
	to_ms( const std::chrono::duration< Rep, Period > & d )
		{
			return static_cast< unsigned int >(
					std::chrono::duration_cast< std::chrono::milliseconds >(d)
							.count() );
		}
} /* namespace chrono_helpers */

}

