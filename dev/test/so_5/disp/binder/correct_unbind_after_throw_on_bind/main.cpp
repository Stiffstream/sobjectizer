/*
 * A test of handling an exception during binding to dispatcher.
 */

#include <iostream>
#include <map>
#include <exception>
#include <stdexcept>
#include <cstdlib>
#include <thread>
#include <chrono>

#include <so_5/all.hpp>

so_5::atomic_counter_t g_agents_count;

struct some_message : public so_5::signal_t {};

class a_ordinary_t
	:
		public so_5::agent_t
{
		typedef so_5::agent_t base_type_t;

	public:
		a_ordinary_t(
			so_5::environment_t & env )
			:
				base_type_t( env )
		{
			++g_agents_count;
		}

		virtual ~a_ordinary_t()
		{
			--g_agents_count;
		}

		virtual void
		so_define_agent()
		{
			so_subscribe( so_direct_mbox() )
				.in( so_default_state() )
					.event( &a_ordinary_t::some_handler );

			so_direct_mbox()->deliver_signal< some_message >();
		}

		virtual void
		so_evt_start();

		void
		some_handler(
			const so_5::event_data_t< some_message > & );
};

void
a_ordinary_t::so_evt_start()
{
	// This method should not be called.
	std::cerr << "error: a_ordinary_t::so_evt_start called.";
	std::abort();
}

void
a_ordinary_t::some_handler(
	const so_5::event_data_t< some_message > & )
{
	// This method should not be called.
	std::cerr << "error: a_ordinary_t::some_handler called.";
	std::abort();
}

class dispatcher_t
	:	public so_5::dispatcher_t
{
	public :
		dispatcher_t()
			:	m_bind_calls( 0 )
			,	m_unbind_calls( 0 )
		{}
		~dispatcher_t() override
		{
			if( !m_agents.empty() )
			{
				std::cerr << "error: there must not be any agents in "
					"dispatcher_t destructor";
				std::abort();
			}

			if( m_bind_calls != m_unbind_calls )
			{
				std::cerr << "error: bind and unbind calls mismatch, bind: "
						<< m_bind_calls << ", unbind: "
						<< m_unbind_calls << std::endl;
				std::abort();
			}
		}

		virtual void
		start( so_5::environment_t & /*env*/ ) override
		{}

		virtual void
		shutdown() override
		{
			if( !m_agents.empty() )
			{
				std::cerr << "error: there must not be any agents in "
					"dispatcher_t::shutdown";
				std::abort();
			}
		}

		virtual void
		wait() override
		{}

		void
		bind_agent( so_5::agent_ref_t agent )
		{
			m_agents.emplace_back( std::move( agent ) );
			++m_bind_calls;
		}

		void
		unbind_agent( so_5::agent_ref_t agent )
		{
			if( m_agents.empty() )
			{
				std::cerr << "error: m_agents must not be empty in unbind_agent";
				std::abort();
			}

			if( m_agents.back().get() != agent.get() )
			{
				std::cerr << "error: unexpected agent in unbind_agent: "
					<< "actual: " << agent.get() << ", expected: "
					<< m_agents.back().get();
				std::abort();
			}

			m_agents.pop_back();
			m_unbind_calls++;
		}

	private :
		std::vector< so_5::agent_ref_t > m_agents;

		unsigned int m_bind_calls;
		unsigned int m_unbind_calls;
};

class throwing_disp_binder_t
	:
		public so_5::disp_binder_t
{
	public:
		throwing_disp_binder_t() : m_agents_bound( 0 ) {}
		virtual ~throwing_disp_binder_t() {}

		virtual so_5::disp_binding_activator_t
		bind_agent(
			so_5::environment_t & env,
			so_5::agent_ref_t agent_ref )
		{
			auto & disp = dynamic_cast< dispatcher_t & >(
					*env.query_named_dispatcher( "test" ).get() );

			if( m_agents_bound < 3 )
			{
				m_agents_bound++;
				disp.bind_agent( std::move( agent_ref ) );

				return []() {};
			}

			throw std::runtime_error( "test exception from disp_binder" );
		}

		virtual void
		unbind_agent(
			so_5::environment_t & env,
			so_5::agent_ref_t agent_ref )
		{
			auto & disp = dynamic_cast< dispatcher_t & >(
					*env.query_named_dispatcher( "test" ).get() );
			disp.unbind_agent( std::move( agent_ref ) );
		}

	private :
		unsigned int m_agents_bound;
};

void
reg_coop(
	so_5::environment_t & env )
{
	so_5::coop_unique_ptr_t coop = env.create_coop( "test_coop",
			so_5::disp_binder_unique_ptr_t(
					new throwing_disp_binder_t() )  );

	coop->add_agent( new a_ordinary_t( env ) );
	coop->add_agent( new a_ordinary_t( env ) );
	coop->add_agent( new a_ordinary_t( env ) );
	coop->add_agent( new a_ordinary_t( env ) );
	coop->add_agent( new a_ordinary_t( env ) );
	coop->add_agent( new a_ordinary_t( env ) );

	try
	{
		env.register_coop( std::move( coop ) );
	}
	catch( const std::exception & x )
	{
		std::cout << "correct_unbind_after_throw_on_bind, expected exception: "
			<< x.what() << std::endl;
	}
}

void
init( so_5::environment_t & env )
{
	reg_coop( env );

	env.stop();
}

int
main()
{
	try
	{
		so_5::launch(
			&init,
			[]( so_5::environment_params_t & params )
			{
				params.add_named_dispatcher(
					"test",
					so_5::dispatcher_unique_ptr_t( new dispatcher_t() ) );
			} );

		if( 0 != g_agents_count )
		{
			std::cerr << "g_agents_count: " << g_agents_count << "\n";
			throw std::runtime_error( "g_agents_count != 0" );
		}
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

