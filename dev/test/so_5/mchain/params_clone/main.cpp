/*
 * A simple test for mchain.
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

using namespace std;
using namespace std::string_literals;

template< typename A, typename B >
decltype(auto)
ensure_same_pointers( A && a, const B & b, std::string description )
{
	ensure_or_die( &a == &b, description );

	return std::forward<A>(a);
}

template< typename A, typename B >
decltype(auto)
ensure_different_pointers( A && a, const B & b, std::string description )
{
	const auto void_ptr =
			[](const auto & v) { return static_cast<const void *>(&v); };

	ensure_or_die( void_ptr(a) != void_ptr(b), description );

	return std::forward<A>(a);
}

void
check_receive_params( so_5::environment_t & env )
{
	auto ch = so_5::create_mchain( env );

	auto p1 = so_5::from( ch );
	ensure_same_pointers( p1.no_wait_on_empty(), p1,
			"receive: no_wait_on_empty() should return the same object"s );

	auto p2 = ensure_different_pointers( p1.handle_n(1), p1,
			"receive: first call to handle_n() should create a new object"s );

	ensure_same_pointers( p2.extract_n(1), p2,
			"receive: repeated call to extract_n() should return the same object" );
	ensure_same_pointers( p2.handle_all(), p2,
			"receive: repeated call to handle_all() should return the same object" );

	ensure_same_pointers(
			p2.empty_timeout( std::chrono::milliseconds(200) ),
			p2,
			"receive: empty_timeout() should return the same object" );
}

void
check_select_params()
{
	auto p1 = so_5::from_all();
	ensure_same_pointers( p1.no_wait_on_empty(), p1,
			"select: no_wait_on_empty() should return the same object"s );

	auto p2 = ensure_different_pointers( p1.handle_n(1), p1,
			"select: first call to handle_n() should create a new object"s );

	ensure_same_pointers( p2.extract_n(1), p2,
			"select: repeated call to extract_n() should return the same object" );
	ensure_same_pointers( p2.handle_all(), p2,
			"select: repeated call to handle_all() should return the same object" );

	ensure_same_pointers(
			p2.empty_timeout( std::chrono::milliseconds(200) ),
			p2,
			"select: empty_timeout() should return the same object" );
}

int
main()
{
	run_with_time_limit(
		[]()
		{
			so_5::wrapped_env_t env;

			check_receive_params( env.environment() );
			check_select_params();
		},
		20 );

	return 0;
}

