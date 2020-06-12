/*
 * A simple test for message limits (dropping the message).
 */

#include <iostream>
#include <map>
#include <exception>
#include <stdexcept>
#include <cstdlib>
#include <thread>
#include <chrono>

#include <so_5/all.hpp>

struct msg_one : public so_5::signal_t {};
struct msg_two : public so_5::signal_t {};

class a_test_t : public so_5::agent_t
{
public :
	a_test_t(
		so_5::environment_t & env )
		:	so_5::agent_t( env
				+ limit_then_transform( 2,
					[this]( const any_unspecified_message & ) {
						return make_transformed< std::string >(
								so_direct_mbox(),
								"Hello, World!" );
					} )
				+ limit_then_drop< msg_two >( 1000 ) )
	{}

	virtual void
	so_evt_start() override
	{
		so_deregister_agent_coop_normally();
	}
};

void
init( so_5::environment_t & env )
{
	env.register_agent_as_coop( env.make_agent< a_test_t >() );
}

int
main()
{
	so_5::launch( &init );

	return 0;
}

