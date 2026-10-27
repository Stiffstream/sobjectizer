#include <so_5/all.hpp>

#include <so_5/cpp_coro/mchain_select.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

namespace test
{

class a_demo_t final : public so_5::agent_t
{
	struct msg_a final : public so_5::signal_t {};
	struct msg_b final : public so_5::signal_t {};

	so_5::mchain_t m_target;

public:
	a_demo_t( context_t ctx )
		: so_5::agent_t{ ctx + so_5::enable_cpp_coro() }
		, m_target{ so_5::create_mchain( so_environment() ) }
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
		so_5::send< msg_a >( *this );
		so_5::send_delayed< msg_b >( m_target, std::chrono::milliseconds{ 25 } );
	}

	void
	so_evt_finish() override
	{
		std::cout << "evt_finish" << std::endl;
	}

private:
	so_5::cpp_coro::task_t< void >
	evt_a( mhood_t< msg_a > )
	{
		std::cout << "evt_a" << std::endl;

		co_await so_5::cpp_coro::select(
				so_this_agent_scheduler(),
				so_5::from_all().handle_n(1),
				so_5::receive_case( m_target,
					[]( so_5::mhood_t< msg_b > ) {
						std::cout << "msg_b received" << std::endl;
					} ) );

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

