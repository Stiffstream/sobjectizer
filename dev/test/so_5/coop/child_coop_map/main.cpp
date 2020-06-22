/*
 * A unit-test for testing deregistration of empty coop.
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

#include <set>

class a_child_t final : public so_5::agent_t
{
	struct die_t final : public so_5::signal_t {};

	const std::chrono::milliseconds m_work_time;

public:
	a_child_t( context_t ctx, std::chrono::milliseconds work_time )
		:	so_5::agent_t{ std::move(ctx) }
		,	m_work_time{ work_time }
	{}

	void so_define_agent() override
	{
		so_subscribe_self().event(
			[this](mhood_t<die_t>) {
				so_deregister_agent_coop_normally();
			} );
	}

	void so_evt_start() override
	{
		so_5::send_delayed< die_t >( *this, m_work_time );
	}
};

class a_supervisor_t final : public so_5::agent_t
{
	std::set< so_5::coop_handle_t > m_children;

public :
	using so_5::agent_t::agent_t;

	void so_define_agent() override
	{
		so_subscribe_self()
			.event( [this](mhood_t<so_5::msg_coop_deregistered> cmd) {
					m_children.erase( cmd->m_coop );
					if( m_children.empty() )
					{
						so_deregister_agent_coop_normally();
					}
				} );
	}

	void so_evt_start() override
	{
		for( int i = 0; i != 10; ++i )
		{
			so_5::introduce_child_coop( *this,
					[this, i]( so_5::coop_t & coop ) {
						m_children.insert( coop.handle() );

						coop.make_agent< a_child_t >(
								std::chrono::milliseconds{ i+1 } * 5 );

						coop.add_dereg_notificator(
								so_5::make_coop_dereg_notificator( so_direct_mbox() ) );
					} );
		}
	}
};

void
init( so_5::environment_t & env )
{
	// Supervison coop.
	env.introduce_coop( []( so_5::coop_t & parent ) {
			parent.make_agent< a_supervisor_t >();
		} );
}

int
main()
{
	try
	{
		run_with_time_limit( []{ so_5::launch( init ); }, 20 );

		return 0;
	}
	catch( const std::exception & x )
	{
		std::cerr << "Exception: " << x.what() << std::endl;
	}

	return 2;
}

