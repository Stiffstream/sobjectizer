#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

namespace test
{

class a_demo_t final : public so_5::agent_t
{
	struct msg_a final : public so_5::signal_t {};

public:
	a_demo_t( context_t ctx )
		: so_5::agent_t{ ctx
			+ so_5::enable_cpp_coro().max_local_queue_size( 5 ) }
	{}

	void
	so_define_agent() override
	{
		so_subscribe_self()
			.event( &a_demo_t::evt_a )
			;
	}

	void
	so_evt_start() override
	{
		for(int i = 0; i < 6; ++i)
			so_5::send< msg_a >( *this );
	}

private:
	void
	evt_a( mhood_t< msg_a > )
	{
		std::cout << "evt_a" << std::endl;
	}
};

void
run_demo()
{
	so_5::launch( []( so_5::environment_t & env ) {
			env.register_agent_as_coop(
					env.make_agent< a_demo_t >() );
		} );
}

} /* namespace test */

using namespace test;

int
main()
{
	try
	{
		run_with_time_limit(
			[]() {
				run_demo();
			},
			5 );
	}
	catch(const std::exception & ex)
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

