/*
 * A simple test for prio_dedicated_threads::one_per_prio dispatcher.
 * Contexts for all priorities must be different.
 */

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

#include <set>
#include <sstream>

struct msg_context_info : public so_5::message_t
{
	so_5::current_thread_id_t m_thread_id;

	msg_context_info( so_5::current_thread_id_t thread_id )
		:	m_thread_id( thread_id )
	{}
};

class a_supervisor_t : public so_5::agent_t
{
public:
	a_supervisor_t( context_t ctx )
		:	so_5::agent_t( ctx )
	{}

	virtual void
	so_define_agent() override
	{
		so_subscribe_self().event( &a_supervisor_t::evt_context_info );
	}

private :
	std::set< so_5::current_thread_id_t > m_contexts;

	void
	evt_context_info( const msg_context_info & evt )
	{
		auto r = m_contexts.insert( evt.m_thread_id );
		if( !r.second )
		{
			std::ostringstream ss;
			ss << "thread_id is not unique! id=" << evt.m_thread_id
				<< ", already registered ids: ";
			for( const auto & s : m_contexts )
				ss << s << ",";

			throw std::runtime_error( ss.str() );
		}
		else if( so_5::prio::total_priorities_count == m_contexts.size() )
		{
			std::cout << "contexts: ";
			for( const auto & s : m_contexts )
				std::cout << s << " ";
			std::cout << std::endl;

			so_environment().stop();
		}
	}
};

void
init( so_5::environment_t & env )
{
	so_5::mbox_t supervisor_mbox;
	env.introduce_coop( [&]( so_5::coop_t & coop ) {
		auto a = coop.make_agent< a_supervisor_t >();
		supervisor_mbox = a->so_direct_mbox();
	} );

	using namespace so_5::disp::prio_dedicated_threads::one_per_prio;
	env.introduce_coop( create_private_disp( env )->binder(),
		[&]( so_5::coop_t & coop )
		{
			so_5::prio::for_each_priority( [&]( so_5::priority_t p ) {
				coop.define_agent( coop.make_agent_context() + p )
					.on_start( [supervisor_mbox] {
						so_5::send< msg_context_info >( supervisor_mbox,
								so_5::query_current_thread_id() );
						} );
			} );
		} );
}

int
main()
{
	try
	{
		run_with_time_limit(
			[]()
			{
				so_5::launch( init );
			},
			20,
			"prio_dedicated_threads::one_per_prio dispatcher test for "
			"context uniquness" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

