/*
 * A test for checking of running a block of code like non-thread-safe
 * event handler.
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

namespace test {

using custom_demand_handler_t = std::function< void() >;

class custom_demand_scheduler_t
{
public:
	virtual ~custom_demand_scheduler_t() = default;

	virtual void
	schedule( custom_demand_handler_t handler ) = 0;
};

class tricky_disp_t final
	:	public so_5::disp_binder_t
	,	public so_5::event_queue_t
	,	public custom_demand_scheduler_t
{
	struct custom_demand_t
	{
		custom_demand_handler_t m_handler;
	};

	so_5::mchain_t m_demands;

	std::thread m_worker_thread;

public:
	tricky_disp_t(
		so_5::environment_t & env )
		:	m_demands{ so_5::create_mchain(env) }
	{
		m_worker_thread = std::thread{
				[this]() { body(); }
			};
	}

	~tricky_disp_t() noexcept override
	{
		so_5::close_retain_content( so_5::terminate_if_throws, m_demands );
		m_worker_thread.join();
	}

	void
	schedule( custom_demand_handler_t handler ) override
	{
		so_5::send< custom_demand_t >( m_demands, std::move(handler) );
	}

private:
	void
	body()
	{
		const auto this_thread_id = so_5::query_current_thread_id();

		so_5::receive( so_5::from( m_demands ).handle_all(),
			[this_thread_id]( so_5::mutable_mhood_t< so_5::execution_demand_t > d ) {
				d->call_handler( this_thread_id );
			},
			[]( const custom_demand_t & d ) {
				d.m_handler();
			} );
	}

	void
	preallocate_resources( so_5::agent_t & ) override
	{}

	void
	undo_preallocation( so_5::agent_t & ) noexcept override
	{}

	void
	bind( so_5::agent_t & agent ) noexcept override
	{
		agent.so_bind_to_dispatcher( *this );
	}

	void
	unbind( so_5::agent_t & ) noexcept override
	{}

	void
	push( so_5::execution_demand_t demand ) override
	{
		so_5::send< so_5::mutable_msg< so_5::execution_demand_t > >(
				m_demands, std::move(demand) );
	}
};

class a_test_t final : public so_5::agent_t
{
	struct msg_stop final : public so_5::signal_t {};

	custom_demand_scheduler_t & m_scheduler;

public:
	a_test_t( context_t ctx,
		custom_demand_scheduler_t & scheduler )
		:	so_5::agent_t{ std::move(ctx) }
		,	m_scheduler{ scheduler }
	{}

	void
	so_define_agent() override
	{
		so_subscribe_self()
			.event( [this]( mhood_t< msg_stop > ) {
					so_deregister_agent_coop_normally();
				} )
			;
	}

	void
	so_evt_start() override
	{
		m_scheduler.schedule( [this]() {
				so_5::send< msg_stop >( *this );
			} );
	}
};

} /* namespace test */

int
main()
{
	using namespace test;

	try
	{
		run_with_time_limit(
			[]()
			{
				so_5::launch( []( so_5::environment_t & env ) {
						env.introduce_coop( []( so_5::coop_t & coop ) {
								auto tricky_disp = std::make_shared< tricky_disp_t >(
										coop.environment() );

								coop.make_agent_with_binder< a_test_t >(
										tricky_disp,
										*tricky_disp );
							} );
					} );
			},
			5 );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 2;
	}

	return 0;
}

