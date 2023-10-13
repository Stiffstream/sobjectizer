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

namespace test
{

class a_test_t : public so_5::agent_t
{
public :
	a_test_t(
		so_5::environment_t & env )
		:	so_5::agent_t( env
				+ limit_then_transform< any_unspecified_message >( 2,
					[this]( const auto & ) {
						return make_transformed< std::string >(
								so_direct_mbox(),
								"Hello, World!" );
					} )
				)
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

} /* namespace test */

int
main()
{
	so_5::launch( &test::init );

	return 0;
}

