/*
 * A sample for make_agent_ref function. 
 */

#include <iostream>

// Main SObjectizer header files.
#include <so_5/all.hpp>

using namespace std::chrono_literals;

enum class status_t { inprogress, completed, aborted };

// An agent that will play role of the performer of long-lasting
// asynchronous operation.
template< typename Completion_Handler >
class operation_performer_t final : public so_5::agent_t
{
	public:
		operation_performer_t(
			context_t ctx,
			std::chrono::milliseconds duration,
			Completion_Handler && completion_handler )
			:	so_5::agent_t{ std::move(ctx) }
			,	m_duration{ duration }
			,	m_completion_handler{ std::move(completion_handler) }
		{}

		void so_define_agent() override
		{
			// Subscription to a signal that indicates the completion
			// of async operation.
			so_subscribe_self().event( [this]( mhood_t<completed_t> ) {
					m_status = status_t::completed;
					m_completion_handler( m_status );
				} );
		}

		void so_evt_start() override
		{
			// Delay completion signal to the specified amount of time.
			so_5::send_delayed< completed_t >( *this, m_duration );
		}

		void so_evt_finish() override
		{
			// At the end of work we should abort the operation
			// if it isn't completed yet.
			if( status_t::inprogress == m_status )
			{
				// Suspend the current thread for some time to
				// allow the deregistration of cooperation with the issuer.
				std::this_thread::sleep_for( 15ms );

				// Now we can call completion handler.
				m_completion_handler( status_t::aborted );
			}
		}

	private:
		struct completed_t final : public so_5::signal_t {};

		status_t m_status{ status_t::inprogress };

		const std::chrono::milliseconds m_duration;

		Completion_Handler m_completion_handler;
};

// A class that plays the role of "async operation handle" and allows
// to cancel the operation if it isn't completed.
class async_operation_t final
{
	public:
		// This class is not Copyable, nor Moveable.
		async_operation_t( const async_operation_t & ) = delete;
		async_operation_t( async_operation_t && ) = delete;

		async_operation_t( so_5::environment_t & env )
			:	m_env{ env }
		{}

		~async_operation_t()
		{
			cancel();
		}

		template< typename Completion_Handler >
		void
		start(
			std::chrono::milliseconds duration,
			Completion_Handler && completion_handler )
		{
			// Don't care about previous calls of start() for simplicity.
			m_performer_coop = m_env.register_agent_as_coop(
					m_env.make_agent< operation_performer_t< Completion_Handler > >(
							duration,
							std::move(completion_handler) ) );
		}

		void
		cancel()
		{
			if( m_performer_coop )
				m_env.deregister_coop(
						std::move(m_performer_coop),
						so_5::dereg_reason::normal );
		}

	private:
		so_5::environment_t & m_env;

		so_5::coop_handle_t m_performer_coop;
};

// An agent that issues an async operation and waits its completion.
class async_operation_issuer_t final : public so_5::agent_t
{
	public:
		async_operation_issuer_t(
			context_t ctx,
			std::string name,
			std::chrono::milliseconds operation_duration,
			std::chrono::milliseconds lifetime )
			:	agent_t{ std::move(ctx) }
			,	m_name{ std::move(name) }
			,	m_operation_duration{ operation_duration }
			,	m_lifetime{ lifetime }
		{}

		~async_operation_issuer_t() override
		{
			// Debug print to see when issuer will be destroyed.
			std::cout << m_name << " destroyed" << std::endl;
		}

		// Definition of the agent for SObjectizer.
		void so_define_agent() override
		{
			// Work should be finished on no_more_time arrival.
			so_subscribe_self().event( [this]( mhood_t< no_more_time > ) {
					so_deregister_agent_coop_normally();
				} );
		}

		// A reaction to start of work in SObjectizer.
		void so_evt_start() override
		{
			// Limit our lifetime.
			so_5::send_delayed< no_more_time >( *this, m_lifetime );

			// Initiate an asynchronous operation.
			m_async_op.start( m_operation_duration,
				// Don't capture 'this' because 'this' can be invalid
				// at the moment of callback invocation.
				// Use make_agent_ref() instead.
				[self = so_5::make_agent_ref(this)]( status_t status ) {
					std::cout << self->m_name << " -> "
							<< (status_t::aborted == status ? "aborted" : "completed")
							<< std::endl;
				} );
		}

		// A reaction to finish of work in SObjectizer.
		void so_evt_finish() override
		{
			// Async operation should be aborted if it isn't completed yet.
			m_async_op.cancel();
		}

	private:
		struct no_more_time final : public so_5::signal_t {};

		const std::string m_name;

		const std::chrono::milliseconds m_operation_duration;
		const std::chrono::milliseconds m_lifetime;

		async_operation_t m_async_op{ so_environment() };
};

int main()
{
	try
	{
		// Starting SObjectizer.
		so_5::launch(
			// A function for SO Environment initialization.
			[]( so_5::environment_t & env )
			{
				// Creating and registering several agents with different
				// lifetimes. Every agent will be a separate cooperation.
				env.register_agent_as_coop(
						env.make_agent< async_operation_issuer_t >(
							"The First Issuer (with rather long name)", 125ms, 50ms ) );
				env.register_agent_as_coop(
						env.make_agent< async_operation_issuer_t >(
							"The Second Issuer (with yet more long name)", 125ms, 100ms ) );
				env.register_agent_as_coop(
						env.make_agent< async_operation_issuer_t >(
							"The Third Issuer", 125ms, 150ms ) );
			} );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}
