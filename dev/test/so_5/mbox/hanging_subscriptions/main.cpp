/*
 * A test for checking deletion of subscriptions to mbox in destructor.
 */

#include <iostream>
#include <sstream>

#include <so_5/all.hpp>

class test_mbox_t : public so_5::rt::abstract_message_box_t
	{
	private :
		const so_5::rt::mbox_t m_actual_mbox;

	public :
		static unsigned int subscriptions;
		static unsigned int unsubscriptions;

		test_mbox_t( so_5::rt::environment_t & env )
			:	m_actual_mbox( env.create_local_mbox() )
			{
			}

		virtual ~test_mbox_t()
			{
			}

		virtual so_5::mbox_id_t
		id() const
			{
				return m_actual_mbox->id();
			}

		virtual void
		deliver_message(
			const std::type_index & type_index,
			const so_5::rt::message_ref_t & message_ref ) const
			{
				m_actual_mbox->deliver_message( type_index, message_ref );
			}

		virtual void
		deliver_service_request(
			const std::type_index & type_index,
			const so_5::rt::message_ref_t & svc_request_ref ) const
			{
				m_actual_mbox->deliver_service_request(
						type_index,
						svc_request_ref );
			}

		virtual void
		subscribe_event_handler(
			const std::type_index & type_index,
			so_5::rt::agent_t * subscriber )
			{
				++subscriptions;
				m_actual_mbox->subscribe_event_handler( type_index, subscriber );
			}

		virtual void
		unsubscribe_event_handlers(
			const std::type_index & type_index,
			so_5::rt::agent_t * subscriber )
			{
				++unsubscriptions;
				m_actual_mbox->unsubscribe_event_handlers( type_index, subscriber );
			}

		virtual std::string
		query_name() const { return m_actual_mbox->query_name(); }

		virtual so_5::rt::mbox_type_t
		type() const override
			{
				return m_actual_mbox->type();
			}

		static so_5::rt::mbox_t
		create( so_5::rt::environment_t & env )
			{
				return so_5::rt::mbox_t( new test_mbox_t( env ) );
			}
	};

unsigned int test_mbox_t::subscriptions = 0;
unsigned int test_mbox_t::unsubscriptions = 0;

struct msg_one : public so_5::rt::signal_t {};
struct msg_two : public so_5::rt::signal_t {};

class a_first_t : public so_5::rt::agent_t
{
	public :
		a_first_t(
			so_5::rt::environment_t & env,
			const so_5::rt::mbox_t & mbox )
			:	so_5::rt::agent_t( env )
			,	m_mbox( mbox )
		{
			so_subscribe( m_mbox ).event( &a_first_t::evt_one );
			so_subscribe( m_mbox ).event( &a_first_t::evt_two );
		}

		void
		evt_one( const so_5::rt::event_data_t< msg_one > & )
		{}

		void
		evt_two( const so_5::rt::event_data_t< msg_two > & )
		{}

	private :
		const so_5::rt::mbox_t m_mbox;
};

int
main()
{
	try
	{
		so_5::launch(
			[]( so_5::rt::environment_t & env )
			{
				{
					auto test_mbox = test_mbox_t::create( env );

					auto coop1 = env.create_coop( "test" );

					{
						std::unique_ptr< a_first_t > first(
								new a_first_t( env, test_mbox ) );
					}

					coop1->define_agent()
						.event< msg_one >( test_mbox, [] {} );
					coop1->define_agent()
						.event< msg_two >( test_mbox, [] {} )
						.event< msg_one >( test_mbox, [] {} );
				}

				auto coop = env.create_coop( "test2" );

				coop->define_agent()
					.on_start( [&env]() { env.stop(); } );

				env.register_coop( std::move( coop ) );
			} );

		if( test_mbox_t::subscriptions != test_mbox_t::unsubscriptions )
			{
				std::cerr << "subscriptions(" << test_mbox_t::subscriptions
						<< ") != unsubscriptions(" << test_mbox_t::unsubscriptions
						<< ") !!!" << std::endl;
				return 3;
			}
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

