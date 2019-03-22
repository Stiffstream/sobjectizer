/*
 * A test for inability to deregister parent cooperation during
 * child cooperation registration.
 */

#include <iostream>
#include <sstream>

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/ensure.hpp>
#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

struct msg_parent_started final : public so_5::signal_t {};

struct msg_initiate_dereg final : public so_5::message_t
	{
		std::promise<void> m_completion;

		msg_initiate_dereg() = default;
	};

class a_child_t : public so_5::agent_t
{
	public :
		a_child_t(
			so_5::environment_t & env,
			const so_5::mbox_t & parent_mbox )
			:	so_5::agent_t( env )
			,	m_parent_mbox( parent_mbox )
		{}

		void
		so_define_agent() override
		{
			auto original_msg_uptr = std::make_unique< msg_initiate_dereg >();
			auto completion = original_msg_uptr->m_completion.get_future();

			so_5::message_ref_t original_msg{ std::move(original_msg_uptr) };
			so_5::send(
					m_parent_mbox,
					mutable_mhood_t< msg_initiate_dereg >{ original_msg } );

			completion.wait();
		}

		void
		so_evt_finish() override
		{
			std::cout << "a_child_t::so_evt_finish is called!" << std::endl;
			std::abort();
		}

	private :
		const so_5::mbox_t m_parent_mbox;
};

class a_parent_t : public so_5::agent_t
{
	public :
		a_parent_t(
			so_5::environment_t & env,
			const so_5::mbox_t & mbox )
			:	so_5::agent_t( env )
			,	m_mbox( mbox )
		{}

		void
		so_define_agent() override
		{
			so_subscribe_self().event(
					&a_parent_t::evt_initiate_dereg );
		}

		void
		so_evt_start() override
		{
			so_5::send< msg_parent_started >( m_mbox );
		}

		void
		evt_initiate_dereg( mutable_mhood_t< msg_initiate_dereg > cmd )
		{
			so_deregister_agent_coop_normally();
			cmd->m_completion.set_value();
		}

	private :
		const so_5::mbox_t m_mbox;
};

class a_driver_t : public so_5::agent_t
{
	public :
		a_driver_t(
			so_5::environment_t & env )
			:	so_5::agent_t( env )
		{}

		void
		so_define_agent() override
		{
			so_subscribe_self().event(
					&a_driver_t::evt_parent_started );
		}

		void
		so_evt_start() override
		{
			auto coop = so_environment().make_coop(
					so_5::disp::active_obj::make_dispatcher( 
							so_environment() ).binder() );

			m_parent_mbox = coop->make_agent< a_parent_t >( so_direct_mbox() )->
					so_direct_mbox();

			m_parent = so_environment().register_coop( std::move(coop) );
		}

		void
		evt_parent_started(mhood_t< msg_parent_started >)
		{
			auto coop = so_environment().make_coop(
					m_parent,
					so_5::disp::active_obj::make_dispatcher(
							so_environment() ).binder() );

			coop->make_agent< a_child_t >( m_parent_mbox );

			try
			{
				so_environment().register_coop( std::move( coop ) );

				std::cout << "An exception is expected in register_coop!"
						<< std::endl;
				std::abort();
			}
			catch( const so_5::exception_t & x )
			{
				ensure_or_die(
						x.error_code() == so_5::rc_coop_is_not_in_registered_state,
						"rc_coop_is_not_in_registered_state is expected, got: " +
						std::to_string(x.error_code()) );
			}

			so_environment().stop();
		}

	private :
		so_5::coop_handle_t m_parent;
		so_5::mbox_t m_parent_mbox;
};

void
init( so_5::environment_t & env )
{
	auto coop = env.make_coop(
			so_5::disp::active_obj::make_dispatcher( env ).binder() );

	coop->make_agent< a_driver_t >();

	env.register_coop( std::move( coop ) );
}

int
main()
{
	run_with_time_limit( [] {
			so_5::launch( &init );
		},
		10 );

	return 0;
}

