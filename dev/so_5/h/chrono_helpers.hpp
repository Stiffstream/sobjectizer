/*
	SObjectizer 5.
*/

/*!
	\file
  \since v.5.3.0
	\brief Utilities for working with std::crhono tools.
*/

#if !defined( SO_5__CHRONO_HELPERS_HPP )
#define SO_5__CHRONO_HELPERS_HPP

#include <chrono>

namespace so_5
{

/*!
 * \since v.5.3.0
 * \brief Helper tools for std::chrono interaction.
 */
namespace chrono_helpers
{
	/*!
	 * \since v.5.3.0
	 * \brief Translation from std::chrono::duration to millisecons.
	 */
	template< class REP, class PERIOD >
	inline unsigned int
	to_ms( const std::chrono::duration< REP, PERIOD > & d )
		{
			return static_cast< unsigned int >(
					std::chrono::duration_cast< std::chrono::milliseconds >(d)
							.count() );
		}
} /* namespace chrono_helpers */

}

#endif

