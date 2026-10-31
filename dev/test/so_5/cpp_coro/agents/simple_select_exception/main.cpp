#include <so_5/all.hpp>

#include <so_5/cpp_coro/mchain_select.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

namespace test
{

class a_child_t final : public so_5::agent_t
{
	struct msg_a final : public so_5::signal_t {};
	struct msg_b final : public so_5::signal_t {};

	so_5::mchain_t m_target;

public:
	a_child_t( context_t ctx )
		: so_5::agent_t{ ctx + so_5::enable_cpp_coro() }
		, m_target{ so_5::create_mchain( so_environment() ) }
	{}

	void
	so_define_agent() override
	{
		so_subscribe_self().event( &a_child_t::evt_a );
	}

	void
	so_evt_start() override
	{
		so_5::send< msg_a >( *this );
		so_5::send_delayed< msg_b >( m_target, std::chrono::milliseconds{ 25 } );
	}

	so_5::exception_reaction_t
	so_exception_reaction() const noexcept override
	{
		return so_5::deregister_coop_on_exception;
	}

private:
	so_5::cpp_coro::task_t< void >
	evt_a( mhood_t< msg_a > )
	{
		co_await so_5::cpp_coro::select(
				so_5::from_all().handle_n(1),
				so_5::receive_case( m_target, [this]( so_5::mhood_t< msg_b > ) {
						throw std::runtime_error{ "Boom!" };
					} ) );
	}
};

class a_parent_t final : public so_5::agent_t
{
public:
	using so_5::agent_t::agent_t;

	void
	so_define_agent() override
	{
		so_subscribe_self()
			.event( &a_parent_t::evt_coop_deregistered )
			;
	}

	void
	so_evt_start() override
	{
		so_5::introduce_child_coop( *this,
			[this]( so_5::coop_t & coop )
			{
				coop.add_dereg_notificator(
						so_5::make_coop_dereg_notificator( so_direct_mbox() ) );

				coop.make_agent< a_child_t >();
			} );
	}

private:
	void
	evt_coop_deregistered(
		const so_5::msg_coop_deregistered & cmd )
	{
		ensure_or_die(
				so_5::dereg_reason::unhandled_exception == cmd.m_reason.reason(),
				"unhandled_exception is expected as dereg reason" );

		so_deregister_agent_coop_normally();
	}
};

void
run_demo()
{
	so_5::launch( []( so_5::environment_t & env ) {
			env.register_agent_as_coop(
					env.make_agent< a_parent_t >() );
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

