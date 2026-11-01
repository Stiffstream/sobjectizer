#include <so_5/all.hpp>

#include <so_5/cpp_coro/details/current_coro_handle.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

#include <syncstream>

namespace test
{

class value_obtainer_t
{
	int m_value{};

public:
	value_obtainer_t( int value ) : m_value{ value } {}

	value_obtainer_t(const value_obtainer_t &) = delete;

	value_obtainer_t(value_obtainer_t &&) = delete;

	bool
	await_ready() const noexcept { return false; }

	std::coroutine_handle<>
	await_suspend( std::coroutine_handle<> parent )
	{
		return parent;
	}

	int
	await_resume() const noexcept { return m_value; }
};

class a_demo_t final : public so_5::agent_t
{
	value_obtainer_t m_obtainer;

public:
	a_demo_t( context_t ctx )
		: so_5::agent_t{ ctx + so_5::enable_cpp_coro() }
		, m_obtainer{ 42 }
	{}

	so_5::cpp_coro::task_t< void >
	so_evt_async_start() override
	{
		const auto own_thread_id = std::this_thread::get_id();
		std::osyncstream(std::cout) << "tid:" << own_thread_id
				<< " | start" << std::endl;

		int x = co_await m_obtainer;
		std::osyncstream(std::cout) << "tid:" << std::this_thread::get_id()
				<< " | x=" << x << ", this=" << this << std::endl;

		ensure_or_die( own_thread_id == std::this_thread::get_id(),
				"wakeup on the different context" );

		ensure_or_die( 42 == x, "unexpected result" );

		so_deregister_agent_coop_normally();
	}
};

void
run_demo()
{
	so_5::launch( []( so_5::environment_t & env ) {
			env.introduce_coop(
					so_5::disp::active_obj::make_dispatcher( env ).binder(),
					[]( so_5::coop_t & coop ) {
						coop.make_agent< a_demo_t >();
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

