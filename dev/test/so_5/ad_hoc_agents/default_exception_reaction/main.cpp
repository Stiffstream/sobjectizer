/*
 * A test for default exception reaction for ad-hoc agents.
 */

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

struct msg_throw_exception : public so_5::signal_t {};

class a_test_t : public so_5::agent_t
	{
	public :
		a_test_t( context_t ctx )
			:	so_5::agent_t( ctx )
			,	m_child_mbox( so_environment().create_mbox() )
			{}

		virtual void
		so_define_agent() override
			{
				so_subscribe_self()
					.event( &a_test_t::evt_coop_started )
					.event( &a_test_t::evt_coop_destroyed );
			}

		virtual void
		so_evt_start() override
			{
				auto coop = so_5::create_child_coop( *this, so_5::autoname );

				coop->add_reg_notificator(
						so_5::make_coop_reg_notificator(
								so_direct_mbox() ) );
				coop->add_dereg_notificator(
						so_5::make_coop_dereg_notificator(
								so_direct_mbox() ) );

				coop->set_exception_reaction(
						so_5::exception_reaction_t::deregister_coop_on_exception );

				coop->define_agent().event< msg_throw_exception >(
					m_child_mbox,
					[] {
						throw std::runtime_error( "Test exception" );
					} );

				so_environment().register_coop( std::move( coop ) );
			}

	private :
		const so_5::mbox_t m_child_mbox;

		void
		evt_coop_started(
			const so_5::msg_coop_registered & )
			{
				so_5::send< msg_throw_exception >( m_child_mbox );
			}

		void
		evt_coop_destroyed(
			const so_5::msg_coop_deregistered & )
			{
				so_deregister_agent_coop_normally();
			}
	};

void
init( so_5::environment_t & env )
	{
		env.register_agent_as_coop( so_5::autoname,
				env.make_agent< a_test_t >() );
	}

int
main()
{
	try
	{
		run_with_time_limit(
			[]()
			{
				so_5::launch( &init );
			},
			20,
			"default ad-hoc agent exception reaction test" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

