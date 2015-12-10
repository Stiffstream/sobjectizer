/*
 * A test of binding of agent to active object dispatcher.
 */

#include <iostream>
#include <exception>
#include <stdexcept>
#include <memory>
#include <set>

#include <so_5/all.hpp>

class test_agent_t
	:
		public so_5::agent_t
{
		typedef so_5::agent_t base_type_t;

	public:
		test_agent_t(
			so_5::environment_t & env )
			:
				base_type_t( env )
		{}

		virtual ~test_agent_t()
		{}

		virtual void
		so_evt_start();

		static std::size_t agents_cout()
		{
			return 20;
		}

		static bool
		ok()
		{
			return m_test_ok && agents_cout() == m_threads.size();
		}

	private:
		static std::mutex m_lock;
		static std::set< std::thread::id > m_threads;
		static bool m_test_ok;
};

std::mutex test_agent_t::m_lock;
std::set< std::thread::id > test_agent_t::m_threads;
bool test_agent_t::m_test_ok = true;

void
test_agent_t::so_evt_start()
{
	auto tid = std::this_thread::get_id();

	std::lock_guard< std::mutex > lock( m_lock );

	if( m_threads.end() != m_threads.find( tid ) )
		m_test_ok = false;

	m_threads.insert( tid );
}


class test_agent_finisher_t
	:
		public so_5::agent_t
{
		typedef so_5::agent_t base_type_t;

	public:
		test_agent_finisher_t(
			so_5::environment_t & env )
			:
				base_type_t( env )
		{}
		virtual ~test_agent_finisher_t()
		{}

		virtual void
		so_evt_start()
		{
			std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
			so_environment().stop();
		}
};

void
init( so_5::environment_t & env )
{
	so_5::coop_unique_ptr_t coop =
		env.create_coop( "test_coop" );

	for( std::size_t i = 0; i < test_agent_t::agents_cout(); ++i )
	{
		coop->add_agent(
			new test_agent_t( env ),
			so_5::disp::active_obj::create_disp_binder(
				"active_obj" ) );
	}

	coop->add_agent( new test_agent_finisher_t( env ) );

	env.register_coop( std::move( coop ) );
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
					"active_obj",
					so_5::disp::active_obj::create_disp() );
			} );

		if( !test_agent_t::ok() )
			throw std::runtime_error( "!test_agent_t::ok()" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

