#include <so_5/all.hpp>

#include <so_5/cpp_coro/details/current_coro_handle.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

#include <syncstream>

namespace test
{

class value_obtainer_t
{
	const so_5::mbox_t m_service;
	int m_value{};

public:
	value_obtainer_t( so_5::mbox_t service );

	value_obtainer_t(const value_obtainer_t &) = delete;

	value_obtainer_t(value_obtainer_t &&) = delete;

	void
	set_value( int value );

	bool
	await_ready() const noexcept;

	void
	await_suspend( std::coroutine_handle<> parent );

	int
	await_resume() const noexcept;
};

struct msg_obtain_value final : public so_5::message_t
{
	value_obtainer_t & m_target;
	std::coroutine_handle<> m_to_wakeup;

	msg_obtain_value(
		value_obtainer_t & target,
		std::coroutine_handle<> to_wakeup )
		: m_target{ target }
		, m_to_wakeup{ to_wakeup }
	{}
};

value_obtainer_t::value_obtainer_t( so_5::mbox_t service )
	: m_service{ std::move(service) }
{}

void
value_obtainer_t::set_value( int value )
{
	std::osyncstream(std::cout) << "tid:" << std::this_thread::get_id()
		<< " | set_value(" << value << ")" << std::endl;
	m_value = value;
}

bool
value_obtainer_t::await_ready() const noexcept
{
	return false;
}

void
value_obtainer_t::await_suspend( std::coroutine_handle<> parent )
{
	std::osyncstream(std::cout) << "tid:" << std::this_thread::get_id()
		<< " | suspension of " << parent.address() << std::endl;

	so_5::send< so_5::mutable_msg<msg_obtain_value> >( m_service, *this, parent );
}

int
value_obtainer_t::await_resume() const noexcept
{
	std::osyncstream(std::cout) << "tid:" << std::this_thread::get_id()
		<< " | resumption with value " << m_value << std::endl;

	return m_value;
}

class a_service_t final : public so_5::agent_t
{
	int m_current_value{ 1 };

public:
	a_service_t( context_t ctx )
		: so_5::agent_t{ std::move(ctx) }
	{}

	void
	so_define_agent() override
	{
		so_subscribe_self().event( &a_service_t::evt_obtain_value );
	}

private:
	void
	evt_obtain_value( mutable_mhood_t< msg_obtain_value > cmd )
	{
		cmd->m_target.set_value( ++m_current_value );
		cmd->m_to_wakeup.resume();
	}
};

class a_demo_t final : public so_5::agent_t
{
	value_obtainer_t m_obtainer;

public:
	a_demo_t( context_t ctx, so_5::mbox_t service )
		: so_5::agent_t{ ctx + so_5::enable_cpp_coro() }
		, m_obtainer{ std::move(service) }
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

		int y = co_await m_obtainer;
		std::osyncstream(std::cout) << "tid:" << std::this_thread::get_id()
				<< " | y=" << y << ", this=" << this << std::endl;

		ensure_or_die( own_thread_id == std::this_thread::get_id(),
				"wakeup on the different context" );

		so_deregister_agent_coop_normally();
	}

	void
	so_evt_finish() override
	{
		std::cout << "evt_finish" << std::endl;
	}
};

void
run_demo()
{
	so_5::launch( []( so_5::environment_t & env ) {
			env.introduce_coop(
					so_5::disp::active_obj::make_dispatcher( env ).binder(),
					[]( so_5::coop_t & coop ) {
						auto service = coop.make_agent< a_service_t >()->so_direct_mbox();
						coop.make_agent< a_demo_t >( service );
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

