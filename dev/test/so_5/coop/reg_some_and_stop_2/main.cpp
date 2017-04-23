/*
 * Another test of registering and deregistering cooperations.
 *
 * Several cooperations are registered.
 * A message is sent to agents.
 * Part of cooperations are deregistered.
 * Another message is sent to agents.
 */

#include <iostream>
#include <map>
#include <exception>
#include <stdexcept>

#include <so_5/all.hpp>

const char * g_test_mbox_name = "test_mbox";

struct test_message : public so_5::signal_t {};

class test_agent_t
	:
		public so_5::agent_t
{
		typedef so_5::agent_t base_type_t;

	public:

		test_agent_t(
			so_5::environment_t & env,
			const so_5::mbox_t & test_mbox )
			:
				base_type_t( env ),
				m_test_mbox( test_mbox )
		{
			++m_agent_count;
		}

		virtual ~test_agent_t()
		{
			--m_agent_count;
		}

		virtual void
		so_define_agent();

		void
		evt_test(
			const so_5::event_data_t< test_message > & msg );

		// Count of life agents.
		static so_5::atomic_counter_t m_agent_count;

		// Count of evt_test calls.
		static so_5::atomic_counter_t m_message_rec_cnt;

	private:
		so_5::mbox_t m_test_mbox;

};

so_5::atomic_counter_t test_agent_t::m_agent_count;
so_5::atomic_counter_t test_agent_t::m_message_rec_cnt;

void
test_agent_t::so_define_agent()
{
	so_subscribe( m_test_mbox )
		.in( so_default_state() )
			.event( &test_agent_t::evt_test );
}

void
test_agent_t::evt_test(
	const so_5::event_data_t< test_message > & )
{
	++m_message_rec_cnt;
}
void
reg_coop(
	const std::string & coop_name,
	const so_5::mbox_t & test_mbox,
	so_5::environment_t & env )
{
	so_5::coop_unique_ptr_t coop =
		env.create_coop( coop_name );

	coop->add_agent( new test_agent_t( env, test_mbox ) );
	coop->add_agent( new test_agent_t( env, test_mbox ) );

	env.register_coop( std::move( coop ) );
}

void
init( so_5::environment_t & env )
{
	so_5::mbox_t test_mbox =
		env.create_mbox( g_test_mbox_name );

	reg_coop( "test_coop_1", test_mbox, env );
	reg_coop( "test_coop_2", test_mbox, env );
	reg_coop( "test_coop_3", test_mbox, env );
	reg_coop( "test_coop_4", test_mbox, env );
	reg_coop( "test_coop_5", test_mbox, env );
	reg_coop( "test_coop_6", test_mbox, env );

	// Give time to subscription.
	std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );

	// Initiate message.
	env.create_mbox( g_test_mbox_name )->deliver_signal< test_message >();

	// Give time to process message.
	std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );

	auto x = test_agent_t::m_agent_count.load();
	x -= test_agent_t::m_message_rec_cnt;
	if( 0 != x )
		throw std::runtime_error(
			"check 1: test_agent_t::m_agent_count != "
			"test_agent_t::m_message_rec_cnt" );

	// Deregister some cooperations.
	env.deregister_coop( "test_coop_1", so_5::dereg_reason::normal );

	env.deregister_coop( "test_coop_6", so_5::dereg_reason::normal );

	env.deregister_coop( "test_coop_3", so_5::dereg_reason::normal );

	test_agent_t::m_message_rec_cnt = 0;

	// Give time to finish deregistration.
	std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );

	// Send another message.
	env.create_mbox( g_test_mbox_name )->deliver_signal< test_message >();

	// Give time to process message.
	std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );

	x = test_agent_t::m_agent_count;
	x -= test_agent_t::m_message_rec_cnt;
	if( 0 != x )
		throw std::runtime_error(
			"check 2: test_agent_t::m_agent_count != "
			"test_agent_t::m_message_rec_cnt" );

	env.stop();

}

int
main()
{
	try
	{
		so_5::launch( &init );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}
