/*
 * A simple test for simple select on several mchains.
 */

#include <so_5/cpp_coro/mchain_select.hpp>
#include <so_5/cpp_coro/this_thread_scheduler.hpp>
#include <so_5/cpp_coro/task.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

#include <test/3rd_party/utest_helper/helper.hpp>

#include <test/so_5/mchain/mchain_params.hpp>

using namespace std;

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

void
do_test()
{
	struct hello {};

	so_5::wrapped_env_t env;

	const auto service_mbox = env.environment().introduce_coop(
			so_5::disp::one_thread::make_dispatcher(
					env.environment() ).binder(),
			[]( so_5::coop_t & coop )
			{
				auto service = coop.make_agent< a_service_t >();
				return service->so_direct_mbox();
			} );

	value_obtainer_t obtainer{ service_mbox };

	auto params = build_mchain_params();

	for( const auto & p : params )
	{
		cout << "=== " << p.first << " ===" << endl;

		auto ch1 = env.environment().create_mchain( p.second );

		bool hello_received = false;

		so_5::send< hello >( ch1 );

		so_5::cpp_coro::this_thread_scheduler_t scheduler{
				env.environment()
			};
		so_5::cpp_coro::current_scheduler_setter_t current_scheduler{ scheduler };

		auto r = scheduler.sync_wait(
				so_5::cpp_coro::select(
					so_5::from_all().handle_n(1),
					receive_case( ch1,
						[&]( hello ) -> so_5::cpp_coro::task_t<void>
						{
							const auto own_thread_id = std::this_thread::get_id();
							std::osyncstream(std::cout) << "tid:" << own_thread_id
									<< " | start" << std::endl;

							int x = co_await obtainer;
							std::osyncstream(std::cout) << "tid:"
									<< std::this_thread::get_id()
									<< " | x=" << x << std::endl;

							ensure_or_die( own_thread_id == std::this_thread::get_id(),
									"wakeup on the different context" );

							int y = co_await obtainer;
							std::osyncstream(std::cout) << "tid:"
									<< std::this_thread::get_id()
									<< " | y=" << y << std::endl;

							ensure_or_die( own_thread_id == std::this_thread::get_id(),
									"wakeup on the different context" );

							hello_received = true;
						} )
					)
				);

		UT_CHECK_CONDITION( 1 == r.extracted() );
		UT_CHECK_CONDITION( 1 == r.handled() );
		UT_CHECK_CONDITION( hello_received );
	}
}

} /* namespace test */

using namespace test;

int
main()
{
	try
	{
		run_with_time_limit(
			do_test,
			20,
			"test for simple multi chain select" );
	}
	catch( const exception & ex )
	{
		cerr << "Error: " << ex.what() << endl;
		return 1;
	}

	return 0;
}

