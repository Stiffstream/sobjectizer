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
