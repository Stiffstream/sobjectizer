#include <iostream>

#include <so_5/all.hpp>

namespace example
{

// Signal to be sent to a worker.
struct msg_do_something final : public so_5::signal_t {};

// Signal to be sent to finish the work of the child SOEnv.
struct msg_shutdown_soenv final : public so_5::signal_t {};

// Type of agent that will work in a child SOEnv.
class a_worker_t final : public so_5::agent_t
{
	// Mbox for receiving notifications from.
	const so_5::mbox_t m_source;

public:
	a_worker_t( context_t ctx, so_5::mbox_t source )
		:	so_5::agent_t{ std::move(ctx) }
		,	m_source{ std::move(source) }
	{}

	void so_define_agent() override
	{
		// A subscription to notifications required.
		so_subscribe( m_source ).event( &a_worker_t::evt_do_something );
	}

private:
	void evt_do_something( mhood_t< msg_do_something > )
	{
		std::cout << "*** worker handles msg_do_something in child SOEnv (addr="
				<< std::addressof(so_environment()) << ")"
				<< std::endl;
	}
};

// Type of agent that will call stop() for SOEnv.
class a_soenv_shutdowner_t final : public so_5::agent_t
{
	// Mbox for receiving notifications from.
	const so_5::mbox_t m_source;

public:
	a_soenv_shutdowner_t( context_t ctx, so_5::mbox_t source )
		:	so_5::agent_t{ std::move(ctx) }
		,	m_source{ std::move(source) }
	{}

	void so_define_agent() override
	{
		// A subscription to notifications required.
		so_subscribe( m_source ).event( &a_soenv_shutdowner_t::evt_shutdown );
	}

private:
	void evt_shutdown( mhood_t< msg_shutdown_soenv > )
	{
		std::cout << "*** shutting down SOEnv (addr="
				<< std::addressof(so_environment()) << ")"
				<< std::endl;

		// NOTE: we'll stop own SOEnv (in that the agent works).
		so_environment().stop();
	}
};

// Type of agent that runs a child SOEnv.
// It calls so_5::launch in its so_evt_start (and because so_5::launch is a
// synchronous call, this call will block the agent until the child SOEnv has
// finished).
class a_child_soenv_runner_t final : public so_5::agent_t
{
	// Mbox for delivering notifications to.
	const so_5::mbox_t m_interaction_mbox;

public:
	a_child_soenv_runner_t( context_t ctx, so_5::mbox_t interaction_mbox )
		:	so_5::agent_t{ std::move(ctx) }
		,	m_interaction_mbox{ std::move(interaction_mbox) }
	{}

	void so_evt_start() override
	{
		// The main trick: a child empty coop is necessary with dereg_notificator.
		// When a coop with a_child_soenv_runner_t is being deregistered, its
		// children coops will be deregistered first.
		// It means that a dereg_notificator for this child coop will be called.
		// And we're using dereg_notificator for sending a special command
		// to the child SOEnv.
		// This command is received by the a_soenv_shutdowner_t agent and the
		// child SOEnv is stopped. When the child SOEnv is stopped the
		// so_5::launch() call is returned.
		so_5::introduce_child_coop( *this,
				[this]( so_5::coop_t & coop ) {
					coop.add_dereg_notificator(
							[dest = m_interaction_mbox](
								so_5::environment_t &,
								const so_5::coop_handle_t &,
								const so_5::coop_dereg_reason_t & ) noexcept
							{
								// This message is necessary to shutdown the child SOEnv.
								so_5::send< msg_shutdown_soenv >( dest );
							} );
				} );

		// Run a child SOEnv.
		so_5::launch( [this]( so_5::environment_t & env ) {
					std::cout << "Initialization of child SOEnv (addr="
							<< std::addressof(env) << ")"
							<< std::endl;

					// We need an agent that finishes the child SOEnv.
					env.register_agent_as_coop(
							env.make_agent< a_soenv_shutdowner_t >( m_interaction_mbox ) );

					// We need a worker agent.
					env.register_agent_as_coop(
							env.make_agent< a_worker_t >( m_interaction_mbox ),
							so_5::disp::active_obj::make_dispatcher( env ).binder() );
				} );

		// The child SOEnv is completely finished when we're here.
		std::cout << "Child SOEnv stopped" << std::endl;
	}
};

// Type of agent that coordinates the example's work.
class a_work_coordinator_t final : public so_5::agent_t
{
	// A signal for finishing the example.
	struct msg_finish final : public so_5::signal_t {};

	// A mbox to be used for notifications/commands.
	const so_5::mbox_t m_dest;

	// Timer ID that is necessary for periodic msg_do_something messages.
	so_5::timer_id_t m_work_timer;

public:
	a_work_coordinator_t( context_t ctx )
		:	so_5::agent_t{ std::move(ctx) }
		,	m_dest{ so_environment().create_mbox() }
	{}

	void so_define_agent() override
	{
		so_subscribe_self().event( &a_work_coordinator_t::evt_finish );
	}

	void so_evt_start() override
	{
		std::cout << "Work coordinator started in SOEnv (addr="
				<< std::addressof(so_environment()) << ")"
				<< std::endl;

		// Start a child coop with a_child_soenv_runner_t inside.
		// NOTE: that agent has to be bound to active_obj dispatcher
		// to have own worker thread.
		so_5::introduce_child_coop(
				*this,
				so_5::disp::active_obj::make_dispatcher( so_environment() ).binder(),
				[this]( so_5::coop_t & coop )
				{
					coop.make_agent< a_child_soenv_runner_t >( m_dest );
				} );

		// Initiate periodic notifications that will be handled in the child SOEnv.
		m_work_timer = so_5::send_periodic< msg_do_something >(
				m_dest,
				std::chrono::milliseconds{75},
				std::chrono::milliseconds{75} );

		// Limit the example's work time.
		so_5::send_delayed< msg_finish >( *this,
				std::chrono::milliseconds{500} );
	}

public:
	void evt_finish( mhood_t< msg_finish > )
	{
		std::cout << "Work coordinator is about to finish its work" << std::endl;

		// Register own coop. It should lead to stopping the child SOEnv.
		so_deregister_agent_coop_normally();
	}
};

} /* namespace example */

int main()
{
	using namespace example;

	try
	{
		// Starting SObjectizer.
		so_5::launch(
			// A function for SO Environment initialization.
			[]( so_5::environment_t & env )
			{
				std::cout << "Initializing the parent SOEnv (addr="
						<< std::addressof(env) << ")"
						<< std::endl;

				env.register_agent_as_coop(
						env.make_agent< a_work_coordinator_t >() );
			} );

		std::cout << "The parent SOEnv stopped" << std::endl;
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

