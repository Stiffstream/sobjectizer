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

			so_5::send< some_message >( *this );
		}

		virtual void
		so_evt_start();

		void
		some_handler(
			mhood_t< some_message > );
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
	mhood_t< some_message > )
{
	// This method should not be called.
	std::cerr << "error: a_ordinary_t::some_handler called.";
	std::abort();
}

class dispatcher_t
{
	public :
		dispatcher_t() = default;
		~dispatcher_t()
		{
			if( !m_agents.empty() )
			{
				std::cerr << "error: there must not be any agents in "
					"dispatcher_t destructor; agents.size()="
					<< m_agents.size();
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

		void
		bind_agent( so_5::agent_t & agent )
		{
			m_agents.insert( &agent );
			++m_bind_calls;
		}

		void
		unbind_agent( so_5::agent_t & agent )
		{
			if( m_agents.empty() )
			{
				std::cerr << "error: m_agents must not be empty in unbind_agent";
				std::abort();
			}

			auto it = m_agents.find( &agent );
			if( it == m_agents.end() )
			{
				std::cerr << "error: unknown agent in unbind_agent: "
					<< "agent: " << &agent;
				std::abort();
			}

			m_agents.erase( it );
			m_unbind_calls++;
		}

	private :
		std::set< so_5::agent_t * > m_agents;

		unsigned int m_bind_calls{};
		unsigned int m_unbind_calls{};
};

class throwing_disp_binder_t
	:
		public so_5::disp_binder_t
{
	public:
		throwing_disp_binder_t( dispatcher_t & disp )
			:	m_disp{ disp }
			{}

		void
		preallocate_resources(
			so_5::agent_t & agent ) override
		{
			if( m_agents_bound < 3 )
			{
				m_agents_bound++;
				m_disp.bind_agent( agent );

				return;
			}

			throw std::runtime_error( "test exception from disp_binder" );
		}

		void
		undo_preallocation(
			so_5::agent_t & agent ) noexcept override
		{
			m_disp.unbind_agent( agent );
		}

		void
		bind(
			so_5::agent_t & /*agent*/ ) noexcept override {}

		void
		unbind(
			so_5::agent_t & agent ) noexcept override
		{
			m_disp.unbind_agent( agent );
		}

	private :
		dispatcher_t & m_disp;
		unsigned int m_agents_bound{};
};

void
reg_coop(
	so_5::environment_t & env,
	dispatcher_t & disp )
{
	auto coop = env.make_coop(
			std::make_shared< throwing_disp_binder_t >( std::ref(disp) ) );

	coop->make_agent< a_ordinary_t >();
	coop->make_agent< a_ordinary_t >();
	coop->make_agent< a_ordinary_t >();
	coop->make_agent< a_ordinary_t >();
	coop->make_agent< a_ordinary_t >();
	coop->make_agent< a_ordinary_t >();

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
	dispatcher_t disp;

	reg_coop( env, disp );

	env.stop();
}

int
main()
{
	try
	{
		so_5::launch( &init );

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

