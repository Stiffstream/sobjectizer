/*
 * An example of the use of custom worker thread factories.
 */
#include <so_5/all.hpp>

#include <algorithm>
#include <iostream>
#include <iterator>

using namespace std::chrono_literals;

// Implementation of custom worker thread.
// It can be reused, so the implementation has to be prepared
// for multiple calls to start()/join() methods.
class worker_thread final : public so_5::disp::abstract_work_thread_t
{
	// Possible statuses of the worker thread.
	enum class status_t
	{
		// No actual thread body, has to wait.
		wait_thread_body,
		// Actual thread body received but execution isn't started yet.
		thread_body_received,
		// The execution of thread body has been started but not completed yet.
		thread_body_accepted,
		// Shutdown operation started.
		shutdown_initiated
	};

	// Actual thread body to be executed.
	// It has non-empty value only when the status is thread_body_received.
	body_func_t m_thread_body;

	// The current status.
	status_t m_status{ status_t::wait_thread_body };

	// A lock for protecting the content of the object.
	std::mutex m_lock;

	// A condition variable for waiting changes of m_status.
	std::condition_variable m_status_cv;

	// Actual worker thread.
	std::thread m_thread;

public:
	worker_thread()
	{
		m_thread = std::thread{ [this] { body(); } };
	}

	~worker_thread() noexcept override
	{
		// Actual thread should receive shutdown notification.
		{
			std::lock_guard< std::mutex > lock{ m_lock };
			m_status = status_t::shutdown_initiated;
			m_status_cv.notify_all();
		}

		// Actual thread has to be joined.
		m_thread.join();
	}

	void
	start( body_func_t thread_body ) override
	{
		std::lock_guard< std::mutex > lock{ m_lock };
		if( status_t::wait_thread_body == m_status )
		{
			m_thread_body = std::move(thread_body);
			m_status = status_t::thread_body_received;
			m_status_cv.notify_one();
		}
		else
		{
			throw std::runtime_error{
				"Unable to start execution "
				"of thread_body when worker thread status isn't "
				"wait_thread_body: "
				+ std::to_string( static_cast<int>(m_status) )
			};
		}
	}

	void
	join() override
	{
		std::unique_lock< std::mutex > lock{ m_lock };

		// If thread_body isn't completed yet then we have to wait.
		while( status_t::thread_body_accepted == m_status )
			m_status_cv.wait( lock );
	}

private:
	void body()
	{
		for(;;)
		{
			auto body_to_execute = wait_body_to_execute();
			if( body_to_execute )
			{
				body_to_execute();
			}
			else
				break;
		}
	}

	// Returns empty value if work has to be finished.
	[[nodiscard]]
	body_func_t wait_body_to_execute()
	{
		body_func_t result;

		std::unique_lock< std::mutex > lock{ m_lock };

		// If it's a repeated call then thread_body_accepted status
		// has to be changed to wait_thread_body.
		if( status_t::thread_body_accepted == m_status )
		{
			m_status = status_t::wait_thread_body;
			// Wakeup 'join()' call if it is here.
			m_status_cv.notify_one();
		}

		// Do status checking loop until we receive a new body to execute
		// or shutdown notification.
		for(;;)
		{
			if( status_t::shutdown_initiated == m_status )
				break; // Work has to be finished.
			else if( status_t::thread_body_received == m_status )
			{
				result = std::move(m_thread_body);
				m_status = status_t::thread_body_accepted;
				break;
			}
			else
			{
				m_status = status_t::wait_thread_body;
				m_status_cv.wait( lock );
			}
		}

		return result;
	}
};

// Implementation of custom worker thread factory.
class thread_factory final : public so_5::disp::abstract_work_thread_factory_t
{
	// Alias for unique_ptr to worker_thread.
	using thread_unique_ptr = std::unique_ptr< worker_thread >;

	// Name of the pool.
	const std::string m_name;

	// Holder for free threads.
	std::vector< thread_unique_ptr > m_free_threads;

public:
	thread_factory(
		std::string pool_name,
		std::size_t pool_size )
		:	m_name{ std::move(pool_name) }
	{
		m_free_threads.reserve( pool_size );
		std::generate_n( std::back_inserter(m_free_threads), pool_size,
				[]() { return std::make_unique< worker_thread >(); } );
	}

	[[nodiscard]]
	so_5::disp::abstract_work_thread_t &
	acquire( so_5::environment_t & /*env*/ ) override
	{
		if( m_free_threads.empty() )
			throw std::runtime_error{ m_name + ": no free threads in the pool" };

		thread_unique_ptr thread_holder{ std::move(m_free_threads.back()) };
		m_free_threads.pop_back();

		std::cout << "*** " << m_name << ": thread acquired: "
				<< thread_holder.get() << std::endl;

		return *(thread_holder.release());
	}

	void release( so_5::disp::abstract_work_thread_t & thread ) noexcept override
	{
		// If we receive a reference to some different type then
		// dynamic_cast will throw and that terminates the application.
		auto & worker = dynamic_cast<worker_thread &>(thread);

		std::cout << "*** " << m_name << ": thread released: "
				<< &worker << std::endl;

		// Released thread has to be returned to the pool.
		m_free_threads.push_back( thread_unique_ptr{ &worker } );
	}
};

// Global signal for finishing the work.
struct shutdown final : public so_5::signal_t
{
	[[nodiscard]]
	static so_5::mbox_t mbox( so_5::environment_t & env )
	{
		return env.create_mbox( "shutdown" );
	}
};

// Logging infrastructure.
struct trace_msg
{
	const so_5::agent_t * m_who;
	std::string m_what;
	std::thread::id m_thread_id;
};

void create_logger( so_5::environment_t & env )
{
	class logger_actor final : public so_5::agent_t {
	public:
		logger_actor( context_t ctx ) : so_5::agent_t{ std::move(ctx) }
		{
			so_subscribe( so_environment().create_mbox( "log" ) )
				.event( [](mhood_t<trace_msg> cmd ) {
					std::cout << cmd->m_thread_id << ": (" << cmd->m_who
						<< ") " << cmd->m_what << std::endl;
				} );

			so_subscribe( shutdown::mbox( so_environment() ) )
				.event( [this]( mhood_t<shutdown> ) {
					// Delay the actual shutdown.
						so_5::send_delayed< shutdown >( *this, 500ms );
				} );

			so_subscribe_self()
				.event( [this]( mhood_t<shutdown> ) {
					so_deregister_agent_coop_normally();
				} );
		}
	};
	env.introduce_coop( [&]( so_5::coop_t & coop ) {
		coop.make_agent< logger_actor >();
	} );
}

void trace( const so_5::agent_t & agent, std::string what )
{
	so_5::send< trace_msg >( agent.so_environment().create_mbox( "log" ),
		&agent, std::move(what), std::this_thread::get_id() );
}

//
// The main part of the example.
//

// Worker agent to be used on the context of thread_pool dispatcher.
class pool_worker final : public so_5::agent_t
{
	const std::string m_name;

public:
	pool_worker( context_t ctx, std::string name )
		:	so_5::agent_t{ std::move(ctx) }
		,	m_name{ std::move(name) }
	{}

	void so_evt_start() override
	{
		trace( *this, m_name + " started" );

		// Need to lock the current thread for some time.
		std::this_thread::sleep_for( 25ms );
	}

	void so_evt_finish() override
	{
		trace( *this, m_name + " finished" );
	}
};

// Manager agent that periodically creates a bunch of pool_workers and runs
// them on a separate thread_pool dispatcher.
class pool_manager final : public so_5::agent_t
{
	// Signal for creation of a new child coop.
	struct create_child final : public so_5::signal_t {};
	// Message for destruction of the existing child coop.
	struct destroy_child final : public so_5::message_t
	{
		so_5::coop_handle_t m_child;

		destroy_child( so_5::coop_handle_t child )
			:	m_child{ std::move(child) }
		{}
	};

	// Thread factory to be used for dispatcher of new cooperations.
	std::shared_ptr< thread_factory > m_thread_pool;

	// Counter for generations of children coops.
	int m_generation_counter{};

public:
	pool_manager( context_t ctx )
		:	so_5::agent_t{ std::move(ctx) }
		,	m_thread_pool{
				std::make_shared< thread_factory >( "pool_manager's factory", 4u )
			}
	{}

	void so_define_agent() override
	{
		so_subscribe_self()
			.event( &pool_manager::evt_create_child )
			.event( &pool_manager::evt_destroy_child )
			;

		so_subscribe( shutdown::mbox( so_environment() ) )
			.event( &pool_manager::evt_shutdown )
			;
	}

	void so_evt_start() override
	{
		trace( *this, "pool_manager started" );

		// Initiate child creation/destruction loop.
		so_5::send< create_child >( *this );
	}

	void so_evt_finish() override
	{
		trace( *this, "pool_manager finished" );
	}

private:
	void evt_create_child( mhood_t< create_child > )
	{
		// A new dispatcher for new child coop.
		auto disp = so_5::disp::thread_pool::make_dispatcher(
				so_environment(),
				"child_pool",
				so_5::disp::thread_pool::disp_params_t{}
					.thread_count( 4 )
					// Custom worker thread factory for this dispatcher.
					.work_thread_factory( m_thread_pool )
			);

		auto coop_holder = so_environment().make_coop(
				// It will be a child for the coop of pool_manager agent.
				so_coop(),
				// It will use the created thread_pool dispatcher as the
				// default dispatcher for all agents.
				disp.binder(
					so_5::disp::thread_pool::bind_params_t{}
						// Every agent will be a separate entity with
						// own event queue.
						.fifo( so_5::disp::thread_pool::fifo_t::individual ) ) );

		// Fill the coop with agents.
		for( int i = 0; i < 7; ++i )
			coop_holder->make_agent< pool_worker >(
					"pool_worker_" + std::to_string( m_generation_counter ) +
					"_" + std::to_string( i ) );

		// Now new coop can be registered.
		auto new_coop_handle = so_environment().register_coop(
				std::move(coop_holder) );

		// Initiate the destruction of the new coop.
		so_5::send_delayed< destroy_child >( *this, 2s, std::move(new_coop_handle) );

		// New generation will be created on the next iteration.
		++m_generation_counter;
	}

	void evt_destroy_child( mhood_t< destroy_child > cmd )
	{
		// The current child coop has to be deregistered.
		so_environment().deregister_coop(
				cmd->m_child,
				so_5::dereg_reason::normal );

		// Initiate the construction of a new child.
		so_5::send_delayed< create_child >( *this, 1s );
	}

	void evt_shutdown( mhood_t< shutdown > )
	{
		so_deregister_agent_coop_normally();
	}
};

void run_example()
{
	so_5::launch( []( so_5::environment_t & env ) {
			create_logger( env );

			env.register_agent_as_coop(
					env.make_agent< pool_manager >() );

			// Allow to work for some time...
			std::this_thread::sleep_for( 5s /*20s*/ );
			// ...and finish the example.
			so_5::send< shutdown >( shutdown::mbox( env ) );
		} );
}

int main()
{
	try
	{
		run_example();
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
	}

	return 0;
}

