#include <so_5/all.hpp>

#include <so_5/cpp_coro/mchain_select.hpp>

#include <so_5/mchain_as_direct_mbox.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

using namespace std::chrono_literals;

namespace test
{

[[nodiscard]]
auto
make_mchain_as_mbox( const so_5::agent_context_t & ctx )
{
	return so_5::mchain_as_direct_mbox::use_mchain(
			so_5::create_mchain( ctx.environment() ) );
}

class a_child_t final : public so_5::agent_t
{
	struct msg_dummy final : public so_5::signal_t {};
	struct msg_start final : public so_5::signal_t {};

	const so_5::mbox_t m_parent;

public:
	static bool so_evt_start_completed_normally;

	struct msg_ready final : public so_5::signal_t {};

	a_child_t( context_t ctx, so_5::mbox_t parent )
		: so_5::agent_t{ ctx
				+ so_5::enable_cpp_coro()
				+ make_mchain_as_mbox( ctx ) }
		, m_parent{ std::move(parent) }
	{}

	so_5::cpp_coro::task_t< void >
	so_evt_async_start() override
	{
		so_5::send< msg_start >( *this );

		co_await so_5::cpp_coro::select(
				so_5::from_all().handle_all(),
				so_5::receive_case(
					so_5::mchain_as_direct_mbox::as_mchain( so_direct_mbox() ),
					[this]( so_5::mhood_t< msg_start > )
					{
						so_5::send< msg_ready >( m_parent );
					},
					[this]( so_5::mhood_t< msg_dummy > )
					{
						std::cout << "msg_dummy received" << std::endl;
					} ) );

		std::cout << "async select completed" << std::endl;

		so_evt_start_completed_normally = true;
	}

	void
	so_evt_finish() override
	{
		std::cout << "a_child_t::evt_finish" << std::endl;
	}
};

bool
a_child_t::so_evt_start_completed_normally = false;

class a_parent_t final : public so_5::agent_t
{
	struct msg_complete final : public so_5::signal_t {};

public:
	a_parent_t( context_t ctx )
		: so_5::agent_t{ std::move(ctx) }
	{}

	void
	so_define_agent() override
	{
		so_subscribe_self()
			.event( [this]( mhood_t< a_child_t::msg_ready > ) {
					so_5::send_delayed< msg_complete >( *this, 50ms );
				} )
			.event( [this]( mhood_t< msg_complete > ) {
					so_environment().stop();
				} )
			;
	}

	void
	so_evt_start() override
	{
		so_5::introduce_child_coop( *this,
				so_5::disp::one_thread::make_dispatcher(
						so_environment() ).binder(),
				[this]( so_5::coop_t & coop ) {
					coop.make_agent< a_child_t >( so_direct_mbox() );
				} );
	}
};

void
run_demo()
{
	so_5::launch( []( so_5::environment_t & env ) {
			env.introduce_coop( []( so_5::coop_t & coop ) {
					coop.make_agent< a_parent_t >();
				} );
		} );

	ensure_or_die( a_child_t::so_evt_start_completed_normally,
			"so_evt_async_start has to be completed normally" );
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

