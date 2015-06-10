/*
 * A test for so_5::details::invoke_noexcept_code.
 *
 * Test must crash.
 */

#include <stdexcept>

#include <so_5/details/h/invoke_noexcept_code.hpp>

void function_with_throw()
{
	throw std::runtime_error( "unexcpected_error" );
}

int
main()
{
	so_5::details::invoke_noexcept_code( [] { function_with_throw(); } );

	return 0;
}

