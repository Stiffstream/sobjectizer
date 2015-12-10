/*
 * This file should be #included into test source file.
 */
#include <thread>
#include <mutex>
#include <condition_variable>
#include <cstdlib>

namespace separate_so_thread
{

void
entry_point( so_5::environment_t * env )
{
	try
	{
		env->run();
	}
	catch( const std::exception & ex )
	{
		std::cerr << "error: " << ex.what() << std::endl;
		std::abort();
	}
}

typedef std::map< so_5::environment_t *, std::thread >
	env_thread_map_t;

env_thread_map_t		g_env_thread_map;
std::mutex	g_lock;

void
start( so_5::environment_t & env )
{
	std::lock_guard< std::mutex > lock( g_lock );

	if( g_env_thread_map.end() == g_env_thread_map.find( &env ) )
	{
		std::thread thread( entry_point, &env );

		g_env_thread_map[ &env ] = std::move( thread );
	}
}

void
wait( so_5::environment_t & env )
{
	std::lock_guard< std::mutex > lock( g_lock );

	env_thread_map_t::iterator it = g_env_thread_map.find( &env );
	if( g_env_thread_map.end() != it )
	{
		it->second.join();
		g_env_thread_map.erase( it );
	}
}

class init_finish_signal_mixin_t
{
	public :
		init_finish_signal_mixin_t()
			:	m_init_finish_lock( m_init_finish_mutex )
		{
		}

		void
		wait_for_init_finish()
		{
			m_init_finish_signal.wait( m_init_finish_lock );
		}

	protected :
		void
		init_finished()
		{
			std::lock_guard< std::mutex > lock( m_init_finish_mutex );
			m_init_finish_signal.notify_one();
		}

	private :
		std::mutex m_init_finish_mutex;
		std::unique_lock< std::mutex > m_init_finish_lock;
		std::condition_variable m_init_finish_signal;
};

template< class ENV, class FUNCTOR >
void
run_on( ENV & env, FUNCTOR f )
{
	start( env );
	env.wait_for_init_finish();

	f();

	env.stop();
	wait( env );
}

} /* namespace separate_so_thread */

