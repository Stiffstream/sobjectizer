/*
 * Test for so_5::tuple_as_message_t and so_5::typed_mtag.
 */

#include <iostream>
#include <map>
#include <exception>

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

// There is a strange error under clang-3.9.0 and MSVC++14.0 (update3).
#if defined( _MSC_VER ) || defined( __clang__ )
namespace std
{

template< std::size_t I, typename TAG, typename... TYPES >
typename tuple_element<I, tuple<TYPES...> >::type const &
get( const so_5::tuple_as_message_t< TAG, TYPES... > & v )
{
	const tuple< TYPES... > & t = v;
	return get<I>(t);
}

}
#endif

using namespace so_5;
using namespace std;

namespace mod1 {

struct tag {};

using first = tuple_as_message_t< typed_mtag< tag, 0 >, int >;
using second = tuple_as_message_t< typed_mtag< tag, 1 >, int >;
using third = tuple_as_message_t< typed_mtag< tag, 2 >, int >;

} /* namespace mod1 */

namespace mod2 {

struct tag {};

using first = tuple_as_message_t< typed_mtag< tag, 0 >, int >;
using second = tuple_as_message_t< typed_mtag< tag, 1 >, int >;
using third = tuple_as_message_t< typed_mtag< tag, 2 >, int >;

} /* namespace mod2 */

void
create_coop(
	coop_t & coop )
{
	auto & env = coop.environment();
	auto agent = coop.define_agent();
	auto mb = agent.direct_mbox();
	agent.on_start( [mb] {
			send< mod1::first >( mb, 0 );
		} )
		.event( mb, [mb]( const mod1::first & evt ) {
			auto v = get<0>( evt );
			cout << "mod1::first: " << v << endl;
			send< mod2::first >( mb, v + 1 );
		} )
		.event( mb, [mb]( const mod1::second & evt ) {
			auto v = get<0>( evt );
			cout << "mod1::second: " << v << endl;
			send< mod2::second >( mb, v + 1 );
		} )
		.event( mb, [mb]( const mod1::third & evt ) {
			auto v = get<0>( evt );
			cout << "mod1::third: " << v << endl;
			send< mod2::third >( mb, v + 1 );
		} )
		.event( mb, [mb]( const mod2::first & evt ) {
			auto v = get<0>( evt );
			cout << "mod2::first: " << v << endl;
			send< mod1::second >( mb, v + 1 );
		} )
		.event( mb, [mb]( const mod2::second & evt ) {
			auto v = get<0>( evt );
			cout << "mod2::second: " << v << endl;
			send< mod1::third >( mb, v + 1 );
		} )
		.event( mb, [mb, &env]( const mod2::third & evt ) {
			auto v = get<0>( evt );
			cout << "mod2::third: " << v << endl;
			env.stop();
		} );
}

int
main()
{
	try
	{
		run_with_time_limit(
			[]()
			{
				so_5::launch( []( environment_t & env )
					{
						env.introduce_coop( create_coop );
					} );
			},
			20,
			"introduce_coop test" );
	}
	catch( const exception & ex )
	{
		cerr << "error: " << ex.what() << endl;
		return 1;
	}

	return 0;
}

