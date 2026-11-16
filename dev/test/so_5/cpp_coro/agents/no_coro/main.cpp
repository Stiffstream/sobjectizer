#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

namespace test
{

class a_demo_t final : public so_5::agent_t
{
	struct msg_a final : public so_5::signal_t {};
	struct msg_b final : public so_5::signal_t {};
	struct msg_c final : public so_5::signal_t {};

public:
	a_demo_t( context_t ctx )
		: so_5::agent_t{ ctx + so_5::enable_cpp_coro() }
	{}

	void
	so_define_agent() override
	{
		so_subscribe_self()
			.event( &a_demo_t::evt_a )
			.event( &a_demo_t::evt_b )
			.event( &a_demo_t::evt_c )
			;
	}

	void
	so_evt_start() override
	{
		so_5::send_delayed< msg_a >( *this,
				std::chrono::milliseconds{ 25 } );
	}

	void
	so_evt_finish() override
	{
		std::cout << "evt_finish" << std::endl;
	}

private:
	void
	evt_a( mhood_t< msg_a > )
	{
		std::cout << "evt_a" << std::endl;

		so_5::send_delayed< msg_b >( *this,
				std::chrono::milliseconds{ 25 } );
	}

	void
	evt_b( mhood_t< msg_b > )
	{
		std::cout << "evt_b" << std::endl;

		so_5::send_delayed< msg_c >( *this,
				std::chrono::milliseconds{ 25 } );
	}

	void
	evt_c( mhood_t< msg_c > )
	{
		std::cout << "evt_c" << std::endl;

		so_deregister_agent_coop_normally();
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

