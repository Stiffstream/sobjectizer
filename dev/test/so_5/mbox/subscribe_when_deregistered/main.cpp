/*
 * A test for subscription inside so_evt_finish() where they
 * should have no effects.
 */

#include <iostream>
#include <exception>
#include <cstdlib>
#include <mutex>
#include <condition_variable>

#include <so_5/all.hpp>

struct msg1 : public so_5::message_t {};
struct msg2 : public so_5::message_t {};
struct msg3 : public so_5::message_t {};
struct msg4 : public so_5::message_t {};
struct msg5 : public so_5::message_t {};

class test_agent_t
	:
		public so_5::agent_t
{
		typedef so_5::agent_t base_type_t;

	public:
		test_agent_t(
			so_5::environment_t & env )
			:
				base_type_t( env ),
				m_mbox( so_environment().create_mbox() )
		{}

		virtual ~test_agent_t()
		{}

		virtual void
		so_define_agent();

		virtual void
		so_evt_finish();

#define ABORT_HANDLER( handler, msg ) \
	void\
	handler ( const so_5::event_data_t< msg > & ) \
	{\
		std::cerr << "Error: " #msg " handler called..." << std::endl; \
		std::abort(); \
	}
		ABORT_HANDLER( handler1, msg1 )
		ABORT_HANDLER( handler2, msg2 )
		ABORT_HANDLER( handler3, msg3 )
		ABORT_HANDLER( handler4, msg4 )
		ABORT_HANDLER( handler5, msg5 )

	private:
		// Mbox for subscription.
		so_5::mbox_t m_mbox;
};


void
test_agent_t::so_define_agent()
{
}

void
test_agent_t::so_evt_finish()
{
	so_subscribe( m_mbox )
		.event( &test_agent_t::handler1 );
	so_subscribe( m_mbox )
		.event( &test_agent_t::handler2 );
	so_subscribe( m_mbox )
		.event( &test_agent_t::handler3 );
	so_subscribe( m_mbox )
		.event( &test_agent_t::handler4 );
	so_subscribe( m_mbox )
		.event( &test_agent_t::handler5 );
}

class stage_monitors_t
{
	private :
		std::mutex m_reg_mutex;
		std::condition_variable m_reg_signal;

		std::mutex m_dereg_mutex;
		std::condition_variable m_dereg_signal;

		enum stage_t {
			NOT_STARTED,
			COOP_REGISTERED,
			COOP_DEREGISTERED
		};

		stage_t m_stage;

	public :
		stage_monitors_t()
			:	m_stage( NOT_STARTED )
		{}

		void
		wait_for_registration()
		{
			std::unique_lock< std::mutex > lock( m_reg_mutex );
			if( COOP_REGISTERED != m_stage )
				m_reg_signal.wait( lock );
		}

		void
		notify_about_registration()
		{
			std::unique_lock< std::mutex > lock( m_reg_mutex );
			m_stage = COOP_REGISTERED;
			m_reg_signal.notify_one();
		}

		void
		wait_for_deregistration()
		{
			std::unique_lock< std::mutex > lock( m_dereg_mutex );
			if( COOP_DEREGISTERED != m_stage )
				m_dereg_signal.wait( lock );
		}

		void
		notify_about_deregistration()
		{
			std::unique_lock< std::mutex > lock( m_dereg_mutex );
			m_stage = COOP_DEREGISTERED;
			m_dereg_signal.notify_one();
		}
};

stage_monitors_t g_stage_monitors;

void
init(
	so_5::environment_t & env )
{
	for( int i = 0; i < 8; ++i )
	{
		so_5::coop_unique_ptr_t coop = env.create_coop(
			"test_coop",
			so_5::disp::active_obj::create_disp_binder( "active_obj" ) );

		coop->add_agent( new test_agent_t( env ) );
		coop->add_agent( new test_agent_t( env ) );
		coop->add_agent( new test_agent_t( env ) );
		coop->add_agent( new test_agent_t( env ) );
		coop->add_agent( new test_agent_t( env ) );
		coop->add_agent( new test_agent_t( env ) );
		coop->add_agent( new test_agent_t( env ) );
		coop->add_agent( new test_agent_t( env ) );

		env.register_coop( std::move( coop ) );
		g_stage_monitors.wait_for_registration();

		env.deregister_coop(
				"test_coop",
				so_5::dereg_reason::normal );
		g_stage_monitors.wait_for_deregistration();
	}
	env.stop();
}

class listener_t : public so_5::coop_listener_t
{
	public :
		virtual void
		on_registered(
			so_5::environment_t &,
			const std::string & )
		{
			g_stage_monitors.notify_about_registration();
		}

		virtual void
		on_deregistered(
			so_5::environment_t &,
			const std::string &,
			const so_5::coop_dereg_reason_t &)
		{
			g_stage_monitors.notify_about_deregistration();
		}
};

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
				params.coop_listener(
						so_5::coop_listener_unique_ptr_t( new listener_t() ) );
			} );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

