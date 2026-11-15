#include <so_5/all.hpp>

#include <so_5/cpp_coro/mchain_select.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

namespace test
{

class a_demo_t final : public so_5::agent_t
{
	struct msg_a final : public so_5::signal_t {};
	struct msg_b final : public so_5::signal_t {};

	std::string & m_log;

	so_5::mchain_t m_target;

public:
	a_demo_t( context_t ctx, std::string & log )
		: so_5::agent_t{ ctx + so_5::enable_cpp_coro() }
		, m_log{ log }
		, m_target{ so_5::create_mchain( so_environment() ) }
	{}

	void
	so_define_agent() override
	{
		so_subscribe_self().event( &a_demo_t::evt_b );
	}

	void
	so_evt_start() override
	{
		m_log += "start;";

		so_5::send_delayed< msg_a >( m_target, std::chrono::milliseconds{ 25 } );
		so_5::send< msg_b >( *this );
	}

	so_5::cpp_coro::task_t< void >
	so_evt_async_start() override
	{
		m_log += "async_start;";

		co_await so_5::cpp_coro::select(
				so_5::from_all().handle_n(1),
				so_5::receive_case( m_target, [this]( so_5::mhood_t< msg_a > ) {
						m_log += "a;";
					} ) );

		so_deregister_agent_coop_normally();
	}

	void
	so_evt_finish() override
	{
		m_log += "finish;";
	}

private:
	void
	evt_b( mhood_t< msg_b > )
	{
		m_log += "b;";
	}
};

void
run_demo()
{
	std::string log;
	so_5::launch( [&log]( so_5::environment_t & env ) {
			env.register_agent_as_coop(
					env.make_agent< a_demo_t >( std::ref(log) ) );
		} );

	ensure_or_die( log == "start;async_start;a;b;finish;",
			"unexpected result value: " + log );
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

