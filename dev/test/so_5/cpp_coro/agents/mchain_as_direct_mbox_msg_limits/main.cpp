#include <so_5/all.hpp>

#include <so_5/cpp_coro/mchain_select.hpp>

#include <so_5/mchain_as_direct_mbox.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

namespace test
{

struct msg_work final : public so_5::signal_t {};
struct msg_done final : public so_5::signal_t {};

[[nodiscard]]
auto
make_mchain_as_mbox( const so_5::agent_context_t & ctx )
{
	return so_5::mchain_as_direct_mbox::use_chain(
			so_5::create_mchain( ctx.environment() ) );
}

class a_test_t final : public so_5::agent_t
{
public:
	a_test_t( context_t ctx )
		: so_5::agent_t{ ctx
				+ so_5::enable_cpp_coro()
				+ make_mchain_as_mbox( ctx )
				+ limit_then_abort<msg_work>(1)
				+ limit_then_abort<msg_done>(1) }
	{}

	void
	so_evt_start() override
	{
		so_5::send< msg_work >( *this );
		so_5::send< msg_work >( *this );
		so_5::send< msg_work >( *this );
		so_5::send< msg_work >( *this );
		so_5::send< msg_work >( *this );
		so_5::send< msg_work >( *this );

		so_5::send< msg_done >( *this );
		so_5::send< msg_done >( *this );
		so_5::send< msg_done >( *this );
		so_5::send< msg_done >( *this );
	}

	so_5::cpp_coro::task_t< void >
	so_evt_async_start() override
	{
		bool done_received = false;
		int work_received = 0;

		co_await so_5::cpp_coro::select(
				so_5::from_all()
					.handle_all()
					.stop_on( [&done_received]() { return done_received; } ),
				so_5::receive_case(
					so_5::mchain_as_direct_mbox::as_mchain( so_direct_mbox() ),
					[&work_received]( so_5::mhood_t< msg_work > )
					{
						++work_received;
					},
					[this, &done_received]( so_5::mhood_t< msg_done > )
					{
						done_received = true;

						so_deregister_agent_coop_normally();
					} ) );

		std::cout << "work_received=" << work_received << std::endl;
	}
};

void
run_demo()
{
	so_5::launch( []( so_5::environment_t & env ) {
			env.introduce_coop( []( so_5::coop_t & coop ) {
					coop.make_agent< a_test_t >();
				} );
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

