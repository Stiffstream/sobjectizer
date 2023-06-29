/*
 * A unit-test for testing a possible message loss at the registration
 * of an agent.
 *
 * Special binder is used for one agent. That binder pauses bind() operation
 * (that pause allows other agents from the same coop to start processing
 * their evt_start demands).
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

namespace so5_test
{

struct msg_first final : public so_5::signal_t {};
struct msg_second final : public so_5::signal_t {};
struct msg_third final : public so_5::signal_t {};

class a_first_t final : public so_5::agent_t
{
	bool m_first_received{ false };

	so_5::mbox_t m_second;

public:
	a_first_t(context_t ctx, so_5::priority_t priority )
		: so_5::agent_t{ ctx + priority }
	{}

	void
	set_second_mbox( const so_5::mbox_t & mbox )
	{
		m_second = mbox;
	}

	void
	so_define_agent() override
	{
		std::cout << "a_first_t::so_define_agent {" << std::endl;
		so_subscribe_self()
			.event( &a_first_t::evt_first )
			.event( &a_first_t::evt_third )
			;
		std::cout << "a_first_t::so_define_agent }" << std::endl;
	}

	void
	so_evt_start() override
	{
		std::cout << "a_first_t::so_evt_start {" << std::endl;
		so_5::send< msg_second >( m_second );
		std::cout << "a_first_t::so_evt_start }" << std::endl;
	}

private:
	void
	evt_first( mhood_t<msg_first> )
	{
		m_first_received = true;
		std::cout << "a_first_t::evt_first!" << std::endl;
	}

	void
	evt_third( mhood_t<msg_third> )
	{
		if( m_first_received )
			so_deregister_agent_coop_normally();
		else
			throw std::runtime_error{ "msg_first was missed!" };
	}
};

class a_second_t final : public so_5::agent_t
{
	so_5::mbox_t m_first;

public:
	a_second_t(context_t ctx, so_5::priority_t priority)
		: so_5::agent_t{ ctx + priority }
	{}

	void
	set_first_mbox( const so_5::mbox_t & mbox )
	{
		m_first = mbox;
	}

	void
	so_define_agent() override
	{
		std::cout << "a_second_t::so_define_agent {" << std::endl;
		so_subscribe_self()
			.event( &a_second_t::evt_second )
			;
		std::cout << "a_second_t::so_define_agent }" << std::endl;
	}

	void
	so_evt_start() override
	{
		std::cout << "a_second_t::so_evt_start {" << std::endl;
		so_5::send< msg_first >( m_first );
		std::cout << "a_second_t::so_evt_start }" << std::endl;
	}

private:
	void
	evt_second( mhood_t<msg_second> )
	{
		so_5::send< msg_third >( m_first );
	}
};

class custom_dispatcher_t final
	:	public so_5::event_queue_t
{
	std::thread m_worker_thread;

	so_5::mchain_t m_queue;

public:
	custom_dispatcher_t( so_5::environment_t & env )
		:	m_queue{ so_5::create_mchain( env ) }
	{}
	~custom_dispatcher_t() override
	{
		if( m_worker_thread.joinable() )
		{
			so_5::close_retain_content(
					so_5::terminate_if_throws,
					m_queue );
			m_worker_thread.join();
		}
	}

	void
	start()
	{
		m_worker_thread = std::thread{ [this]() { this->body(); } };
	}

	void
	push( so_5::execution_demand_t demand ) override
	{
		so_5::send< so_5::execution_demand_t >( m_queue, std::move(demand) );
	}

	void
	push_evt_start( so_5::execution_demand_t demand ) override
	{
		this->push( std::move(demand) );
	}

	void
	push_evt_finish( so_5::execution_demand_t demand ) noexcept override
	{
		this->push( std::move(demand) );
	}

private:
	void
	body()
	{
		const auto worker_id = so_5::query_current_thread_id();

		so_5::receive( so_5::from( m_queue ).handle_all(),
				[worker_id]( so_5::execution_demand_t demand ) {
					demand.call_handler( worker_id );
				} );
	}
};

class problematic_dispatcher_binder_t final
	:	public so_5::disp_binder_t
{
	std::unique_ptr< custom_dispatcher_t > m_disp;

public:
	problematic_dispatcher_binder_t(
		std::unique_ptr< custom_dispatcher_t > disp )
		:	m_disp{ std::move(disp) }
	{}

	void
	preallocate_resources(
		so_5::agent_t & /*agent*/ ) override
	{
		// Nothing to do.
	}

	void
	undo_preallocation(
		so_5::agent_t & /*agent*/ ) noexcept override
	{
		// Nothing to do.
	}

	void
	bind(
		so_5::agent_t & agent ) noexcept override
	{
		std::cout << "*** pausing the binding ***" << std::endl;
		std::this_thread::sleep_for( std::chrono::seconds{1} );

		agent.so_bind_to_dispatcher( *m_disp );
	}

	void
	unbind(
		so_5::agent_t & /*agent*/ ) noexcept override
	{
		// Nothing to do.
	}
};

class normal_dispatcher_binder_t final
	:	public so_5::disp_binder_t
{
	std::unique_ptr< custom_dispatcher_t > m_disp;

public:
	normal_dispatcher_binder_t(
		std::unique_ptr< custom_dispatcher_t > disp )
		:	m_disp{ std::move(disp) }
	{}

	void
	preallocate_resources(
		so_5::agent_t & /*agent*/ ) override
	{
		// Nothing to do.
	}

	void
	undo_preallocation(
		so_5::agent_t & /*agent*/ ) noexcept override
	{
		// Nothing to do.
	}

	void
	bind(
		so_5::agent_t & agent ) noexcept override
	{
		std::cout << "--- binding without a pause ---" << std::endl;

		agent.so_bind_to_dispatcher( *m_disp );
	}

	void
	unbind(
		so_5::agent_t & /*agent*/ ) noexcept override
	{
		// Nothing to do.
	}
};
void
run()
{
	so_5::launch( []( so_5::environment_t & env ) {
		env.introduce_coop( []( so_5::coop_t & coop ) {
				// A separate dispatcher for the first agent.
				auto first_disp = std::make_unique< custom_dispatcher_t >(
							coop.environment() );
				// The dispatcher has to be started manually.
				first_disp->start();
				// Problematic binder will be used for the first agent.
				auto first_binder = std::make_shared< problematic_dispatcher_binder_t >(
						std::move(first_disp) );
				auto * first = coop.make_agent_with_binder< a_first_t >(
						first_binder,
						so_5::prio::p0 );

				// A separate dispatcher for the second agent.
				auto second_disp = std::make_unique< custom_dispatcher_t >(
							coop.environment() );
				// The dispatcher has to be started manually.
				second_disp->start();
				// Normal binder will be used for the second agent.
				auto second_binder = std::make_shared< normal_dispatcher_binder_t >(
						std::move(second_disp) );
				auto * second = coop.make_agent_with_binder< a_second_t >(
						second_binder,
						so_5::prio::p1 );

				first->set_second_mbox( second->so_direct_mbox() );
				second->set_first_mbox( first->so_direct_mbox() );
			} );
	} );
}

} /* namespace so5_test */

using namespace so5_test;

int
main()
{
	try
	{
		run_with_time_limit(
			[]{
				run();
			},
			5 );

		return 0;
	}
	catch( const std::exception & x )
	{
		std::cerr << "Exception: " << x.what() << std::endl;
	}

	return 2;
}

