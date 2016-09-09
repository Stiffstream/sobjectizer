/*
 * Test for so_5::tuple_as_message_t.
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
		
void
create_coop(
	so_5::coop_t & coop )
{
	using namespace so_5;
	using namespace std;

	using hello = tuple_as_message_t< mtag< 0 >, string >;
	using bye = tuple_as_message_t< mtag< 1 >, string, string >;
	using repeat = tuple_as_message_t< mtag< 2 >, int, int >;

	auto & env = coop.environment();
	auto agent = coop.define_agent();
	auto mb = agent.direct_mbox();
	agent.on_start( [mb] {
			send< hello >( mb, "Hello" );
		} )
		.event( agent, [mb]( const hello & evt ) {
			cout << "hello: " << get<0>( evt ) << endl;
			send< repeat >( mb, 0, 3 );
		} )
		.event( agent, [mb]( const repeat & evt ) {
			cout << "repetition: " << get<0>( evt ) << endl;
			auto next = get<0>( evt ) + 1;
			if( next < get<1>( evt ) )
				send< repeat >( mb, next, get<1>( evt ) );
			else
				send< bye >( mb, "Good", "Bye" );
		} )
		.event( agent, [&env]( const bye & evt ) {
			cout << "bye: " << get<0>( evt ) << " " << get<1>( evt ) << endl;
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
				so_5::launch( []( so_5::environment_t & env )
					{
						env.introduce_coop( create_coop );
					} );
			},
			20,
			"introduce_coop test" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

