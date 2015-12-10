/*
	SObjectizer 5.
*/

#include <so_5/rt/h/nonempty_name.hpp>
#include <so_5/h/exception.hpp>

namespace so_5
{

//
// nonempty_name_t
//

nonempty_name_t::nonempty_name_t(
	const char * name )
	:
		m_nonempty_name( name )
{
	if( m_nonempty_name.empty() )
		throw exception_t(
			"empty name",
			rc_empty_name );
}

nonempty_name_t::nonempty_name_t(
	const std::string & name )
	:
		m_nonempty_name( name )
{
	if( m_nonempty_name.empty() )
		throw exception_t(
			"empty name",
			rc_empty_name );
}

nonempty_name_t::~nonempty_name_t()
{
}

} /* namespace so_5 */
