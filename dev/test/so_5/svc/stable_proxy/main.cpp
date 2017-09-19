/*
 * A test for the case where reference to mbox is stored inside
 * service_invocation_proxy_t.
 */

#include <iostream>
#include <exception>
#include <sstream>

#include <so_5/all.hpp>

#include "../a_time_sentinel.hpp"

class test_mbox_t : public so_5::abstract_message_box_t
	{
	private :
		const so_5::mbox_t m_actual_mbox;

	public :
		static bool test_passed;

		test_mbox_t( so_5::environment_t & env )
			:	m_actual_mbox( env.create_mbox() )
			{
				std::cout << "test_mbox_t::ctor()" << std::endl;
			}
		virtual ~test_mbox_t() override
			{
				std::cout << "test_mbox_t::dtor()" << std::endl;

				if( !test_passed )
					{
						std::cerr << "test_mbox_t::test_passed is not set!" << std::endl;
						std::abort();
					}
			}

		virtual so_5::mbox_id_t
		id() const override
			{
				return m_actual_mbox->id();
			}

		virtual void
		do_deliver_message(
			const std::type_index &,
			const so_5::message_ref_t &,
			unsigned int ) const override
			{
				// DO NOTHING FOR THAT TEST
			}

		virtual void
		do_deliver_service_request(
			const std::type_index & type_index,
			const so_5::message_ref_t & svc_request_ref,
			unsigned int overlimit_reaction_deep ) const override
			{
				m_actual_mbox->do_deliver_service_request(
						type_index,
						svc_request_ref,
						overlimit_reaction_deep );
			}

		virtual void
		subscribe_event_handler(
			const std::type_index &,
			const so_5::message_limit::control_block_t *,
			so_5::agent_t * ) override
			{
				// DO NOTHING FOR THAT TEST
			}

		virtual void
		unsubscribe_event_handlers(
			const std::type_index &,
			so_5::agent_t * ) override
			{
				// DO NOTHING FOR THAT TEST
			}

		virtual std::string
		query_name() const override { return m_actual_mbox->query_name(); }

		virtual so_5::mbox_type_t
		type() const override
			{
				return m_actual_mbox->type();
			}

		virtual void
		set_delivery_filter(
			const std::type_index & msg_type,
			const so_5::delivery_filter_t & filter,
			so_5::agent_t & subscriber ) override
			{
				m_actual_mbox->set_delivery_filter( msg_type, filter, subscriber );
			}

		virtual void
		drop_delivery_filter(
			const std::type_index & msg_type,
			so_5::agent_t & subscriber ) SO_5_NOEXCEPT override
			{
				m_actual_mbox->drop_delivery_filter( msg_type, subscriber );
			}

		static so_5::mbox_t
		create( so_5::environment_t & env )
			{
				return so_5::mbox_t( new test_mbox_t( env ) );
			}
	};

bool test_mbox_t::test_passed = false;

struct msg_convert : public so_5::message_t
	{
		int m_value;

		msg_convert( int value ) : m_value( value )
			{}
	};

typedef decltype( (static_cast< so_5::abstract_message_box_t * >(nullptr))->get_one< std::string >().wait_forever() ) proxy_t;

class a_client_t
	:	public so_5::agent_t
	{
	public :
		a_client_t(
			so_5::environment_t & env )
			:	so_5::agent_t( env )
			// In ordinal case the mbox will be destroyed immediatelly
			// after a_client_t constructor finished.
			// But because smart reference to mbox is stored inside proxy
			// the mbox will live to the end on test.
			,	m_svc( test_mbox_t::create( env )->
						get_one< std::string >().wait_forever() )
			{}

		virtual void
		so_evt_start()
			{
				std::cout << "a_client_t::so_evt_start() enter" << std::endl;

				try
					{
						m_svc.sync_get( new msg_convert( 3 ) );

						std::cerr << "An exception no_svc_handlers expected"
								<< std::endl;

						std::abort();
					}
				catch( const so_5::exception_t & x )
					{
						if( so_5::rc_no_svc_handlers != x.error_code() )
							{
								std::cerr << "Unexpected error_code: "
										<< x.error_code()
										<< ", expected: "
										<< so_5::rc_no_svc_handlers
										<< std::endl;

								std::abort();
							}
					}

				test_mbox_t::test_passed = true;

				so_environment().stop();

				std::cout << "a_client_t::so_evt_start() exit" << std::endl;
			}

	private :
		proxy_t m_svc;
	};

void
init(
	so_5::environment_t & env )
	{
		test_mbox_t::test_passed = false;

		auto coop = env.create_coop(
				"test_coop",
				so_5::disp::active_obj::create_disp_binder( "active_obj" ) );

		coop->add_agent( new a_client_t( env ) );
		coop->add_agent( new a_time_sentinel_t( env ) );

		env.register_coop( std::move( coop ) );
	}

int
main()
	{
		try
			{
				so_5::launch(
					&init,
					[]( so_5::environment_params_t & p ) {
						p.add_named_dispatcher(
							"active_obj",
							so_5::disp::active_obj::create_disp() );
					} );
			}
		catch( const std::exception & ex )
			{
				std::cerr << "Error: " << ex.what() << std::endl;
				return 1;
			}

		return 0;
	}

