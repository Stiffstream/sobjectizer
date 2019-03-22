/*
 * A test for coop reg/dereg notifications.
 */

#include <iostream>
#include <sstream>

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

struct msg_child_deregistered : public so_5::signal_t {};

class a_child_t : public so_5::agent_t
{
		typedef so_5::agent_t base_type_t;

	public :
		a_child_t(
			so_5::environment_t & env )
			:	base_type_t( env )
		{
		}

		void
		so_evt_start()
		{
			so_deregister_agent_coop_normally();
		}
};

class a_test_t : public so_5::agent_t
{
	typedef so_5::agent_t base_type_t;

	public :
		a_test_t(
			so_5::environment_t & env,
			const so_5::coop_reg_notificator_t & reg_notificator,
			const so_5::coop_dereg_notificator_t & dereg_notificator )
			:	base_type_t( env )
			,	m_reg_notificator( reg_notificator )
			,	m_dereg_notificator( dereg_notificator )
			,	m_mbox( env.create_mbox() )
		{}

		void
		so_define_agent()
		{
			so_subscribe( m_mbox ).event( &a_test_t::evt_child_deregistered );
		}

		void
		so_evt_start()
		{
			auto child_coop = so_environment().make_coop( so_coop() );
			child_coop->add_reg_notificator(
					[this](
						auto & env,
						const auto & handle ) noexcept {
						m_reg_notificator( env, handle );
					} );
			child_coop->add_dereg_notificator(
					[this](
						auto & env,
						const auto & handle,
						const auto & reason ) noexcept {
						m_dereg_notificator( env, handle, reason );
					} );
			child_coop->add_dereg_notificator(
					[this]( so_5::environment_t &,
						const so_5::coop_handle_t &,
						const so_5::coop_dereg_reason_t &) noexcept
					{
						so_5::send< msg_child_deregistered >( m_mbox );
					} );

			child_coop->make_agent< a_child_t >();

			so_environment().register_coop( std::move( child_coop ) );
		}

		void
		evt_child_deregistered( mhood_t< msg_child_deregistered > )
		{
			so_environment().stop();
		}

	private :
		const so_5::coop_reg_notificator_t m_reg_notificator;
		const so_5::coop_dereg_notificator_t m_dereg_notificator;

		const so_5::mbox_t m_mbox;
};

class test_env_t
{
	public :
		test_env_t()
			:	m_reg_notify_received( false )
			,	m_dereg_notify_received( false )
		{}

		void
		init( so_5::environment_t & env )
		{
			auto on_reg =
					[this]( so_5::environment_t &,
							const so_5::coop_handle_t & )
					{
						m_reg_notify_received = true;
					};
			auto on_dereg =
					[this]( so_5::environment_t &,
							const so_5::coop_handle_t &,
							const so_5::coop_dereg_reason_t &)
					{
						m_dereg_notify_received = true;
					};

			env.register_agent_as_coop(
					env.make_agent< a_test_t >( on_reg, on_dereg ) );
		}

		void
		check_result() const
		{
			if( !m_reg_notify_received )
				throw std::runtime_error( "registration notify not received" );
			if( !m_dereg_notify_received )
				throw std::runtime_error( "deregistration notify not received" );
		}

	private :
		bool m_reg_notify_received;
		bool m_dereg_notify_received;
};

int
main()
{
	run_with_time_limit( [] {
			test_env_t test_env;
			so_5::launch(
				[&test_env]( so_5::environment_t & env )
				{
					test_env.init( env );
				} );

			test_env.check_result();
		},
		10 );

	return 0;
}

