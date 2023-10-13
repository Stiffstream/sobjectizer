/*
 * A unit-test for agent_t::so_this_agent_disp_binder().
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/ensure.hpp>
#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

namespace test
{

struct msg_done final : public so_5::signal_t {};

struct msg_take_tid final : public so_5::message_t
	{
		so_5::current_thread_id_t m_tid;

		explicit msg_take_tid( so_5::current_thread_id_t tid )
			:	m_tid{ tid }
			{}
	};

class a_collector_t final : public so_5::agent_t
	{
		std::set< so_5::current_thread_id_t > m_tids;

	public:
		using so_5::agent_t::agent_t;

		void
		so_define_agent() override
			{
				so_subscribe_self()
					.event( &a_collector_t::evt_done )
					.event( &a_collector_t::evt_take_tid )
					;
			}

		void
		so_evt_start() override
			{
				so_5::send< msg_take_tid >( *this, so_5::query_current_thread_id() );
			}

		void
		so_evt_finish() override
			{
				ensure_or_die( 3u == m_tids.size(),
						"unexpected number of collected TIDs: " +
						std::to_string( m_tids.size() ) );
			}

	private:
		void
		evt_done( mhood_t< msg_done > )
			{
				so_deregister_agent_coop_normally();
			}

		void
		evt_take_tid( mhood_t< msg_take_tid > cmd )
			{
				m_tids.insert( cmd->m_tid );
			}
	};

class a_child_t final : public so_5::agent_t
	{
		const so_5::mbox_t m_collector_mbox;

	public:
		a_child_t( context_t ctx, so_5::mbox_t collector_mbox )
			:	so_5::agent_t{ std::move(ctx) }
			,	m_collector_mbox{ std::move(collector_mbox) }
			{}

		void
		so_evt_start() override
			{
				so_5::send< msg_take_tid >( m_collector_mbox,
						so_5::query_current_thread_id() );
				so_5::send< msg_done >( m_collector_mbox );
			}
	};

class a_parent_t final : public so_5::agent_t
	{
		const so_5::mbox_t m_collector_mbox;

	public:
		a_parent_t( context_t ctx, so_5::mbox_t collector_mbox )
			:	so_5::agent_t{ std::move(ctx) }
			,	m_collector_mbox{ std::move(collector_mbox) }
			{}

		void
		so_evt_start() override
			{
				so_5::send< msg_take_tid >( m_collector_mbox,
						so_5::query_current_thread_id() );

				so_5::introduce_child_coop(
						*this,
						so_this_agent_disp_binder(),
						[this]( so_5::coop_t & coop ) {
							coop.make_agent< a_child_t >( m_collector_mbox );
						} );
			}
	};

void
init( so_5::environment_t & env )
{
	env.introduce_coop( [&]( so_5::coop_t & coop ) {
			// The collector will use the default dispatcher
			// (because it's automatically selected for the coop).
			auto mbox = coop.make_agent< a_collector_t >()->so_direct_mbox();

			// The parent agent will use active_obj dispatcher.
			coop.make_agent_with_binder< a_parent_t >(
					so_5::disp::active_obj::make_dispatcher( env ).binder(),
					mbox );
		} );
}

} /* namespace test */

int
main()
{
	try
	{
		run_with_time_limit( []{ so_5::launch( test::init ); }, 5 );

		return 0;
	}
	catch( const std::exception & x )
	{
		std::cerr << "Exception: " << x.what() << std::endl;
	}

	return 2;
}

