/*
	SObjectizer 5.
*/

#include <so_5/h/exception.hpp>

#include <sstream>

namespace so_5
{

//
// exception_t
//

exception_t::exception_t(
	const std::string & error_descr,
	int error_code )
	:
		base_type_t( error_descr ),
		m_error_code( error_code )
{
}

exception_t::exception_t(
	const exception_t & x )
	:
		base_type_t( x ),
		m_error_code( x.m_error_code )
{
}

exception_t::~exception_t()
{
}

int
exception_t::error_code() const
{
	return m_error_code;
}

void 
exception_t::raise(
	const char * file_name,
	unsigned int line_number,
	const std::string & error_descr,
	int error_code )
{
	std::ostringstream s;

	s << "(" << file_name << ":" << line_number << "): error("
		<< error_code << ") " << error_descr;

	throw exception_t( s.str(), error_code );
}

} /* namespace so_5 */
