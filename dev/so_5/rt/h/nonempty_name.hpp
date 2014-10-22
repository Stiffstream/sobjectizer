/*
	SObjectizer 5.
*/

/*!
	\file
	\brief A nonempty_name class definition.
*/

#if !defined( _SO_5__RT__NONEMPTY_NAME_HPP_ )
#define _SO_5__RT__NONEMPTY_NAME_HPP_

#include <string>

#include <so_5/h/compiler_features.hpp>
#include <so_5/h/declspec.hpp>

namespace so_5
{

namespace rt
{

#if defined( SO_5_MSVC )
	#pragma warning(push)
	#pragma warning(disable: 4251)
#endif

//
// nonempty_name_t
//

//! A class for the name which cannot be empty.
/*!
 * Checks value in the constructor. Throws an exception if the name is empty.
*/
class SO_5_TYPE nonempty_name_t
{
	public:
		//! Constructor with check for the empty value.
		nonempty_name_t(
			const char * name );

		//! Constructor with check for the empty value.
		nonempty_name_t(
			const std::string & name );

		~nonempty_name_t();

		//! Get the value.
		inline const std::string &
		query_name() const
		{
			return m_nonempty_name;
		}

	private:
		//! Value.
		std::string m_nonempty_name;
};

#if defined( SO_5_MSVC )
	#pragma warning(pop)
#endif

} /* namespace rt */

} /* namespace so_5 */

#endif
