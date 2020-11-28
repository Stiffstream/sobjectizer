/*
 * A test of binding agents to active group dispatchers.
 */

#include <iostream>
#include <exception>
#include <stdexcept>
#include <memory>
#include <map>
#include <thread>
#include <mutex>

#include <so_5/all.hpp>

typedef std::map< std::thread::id, unsigned int >
	threads_count_map_t;

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

		~test_agent_t() override
		{}

		void
		so_evt_start() override;

		static std::size_t
		agents_cout()
		{
			return 10;
		}

		static bool
		ok()
		{
			threads_count_map_t::const_iterator it = m_threads.begin();
			for(; it != m_threads.end(); ++it )
				if( agents_cout() != it->second )
				{
					std::cerr << it->first << " => " << it->second
						<< std::endl;
					return false;
				}

			return true;
		}

	private:
		static std::mutex m_lock;
		static threads_count_map_t m_threads;
		static bool m_test_ok;
};

std::mutex test_agent_t::m_lock;
std::map< std::thread::id, unsigned int > test_agent_t::m_threads;

void
test_agent_t::so_evt_start()
{
	auto tid = std::this_thread::get_id();

	std::lock_guard< std::mutex > lock( m_lock );

	if( m_threads.end() == m_threads.find( tid ) )
		m_threads[ tid ] = 1;
	else
		++m_threads[ tid ];
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
		~test_agent_finisher_t() override
		{}

		void
		so_evt_start() override
		{
			std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
			so_environment().stop();
		}
};


void
push_group(
	so_5::coop_t & coop,
	const std::string & group_name,
	so_5::environment_t & env )
{
	auto disp = so_5::disp::active_group::make_dispatcher( env );

	for( std::size_t i = 0; i < test_agent_t::agents_cout(); ++i )
	{
		coop.make_agent_with_binder< test_agent_t >( disp.binder( group_name ) );
	}
}
void
init( so_5::environment_t & env )
{
	auto coop = env.make_coop();

	push_group( *coop, "grp_1", env );
	push_group( *coop, "grp_2", env );
	push_group( *coop, "grp_3", env );
	push_group( *coop, "grp_4", env );

	coop->make_agent< test_agent_finisher_t >();

	env.register_coop( std::move( coop ) );
}

int
main()
{
	try
	{
		so_5::launch( &init );

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
