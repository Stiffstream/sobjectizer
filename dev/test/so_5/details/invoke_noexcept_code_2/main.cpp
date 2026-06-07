/*
 * A test for so_5::details::invoke_noexcept_code.
 */

#include <so_5/details/invoke_noexcept_code.hpp>

struct functor_with_tricky_call_operator
{
	void
	operator()() && {}
};


int
main()
{
	so_5::details::invoke_noexcept_code( functor_with_tricky_call_operator{} );

	return 0;
}

