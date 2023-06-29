/*
 * A test for automatic unsubscription on dereg.
 */

#include <iostream>
#include <sstream>

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

class test_mbox_t : public so_5::abstract_message_box_t
	{
	private :
		const so_5::mbox_t m_actual_mbox;

		unsigned int m_subscriptions = 0;
		unsigned int m_unsubscriptions = 0;

	public :
		test_mbox_t( so_5::environment_t & env )
			:	m_actual_mbox( env.create_mbox() )
			{
			}

		~test_mbox_t() override
			{
				if( m_subscriptions != m_unsubscriptions )
					{
						std::cerr << "subscriptions(" << m_subscriptions
								<< ") != unsubscriptions(" << m_unsubscriptions
								<< "). Test aborted!" << std::endl;

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
			so_5::message_delivery_mode_t delivery_mode,
			const std::type_index & type_index,
			const so_5::message_ref_t & message_ref,
			unsigned int redirection_deep ) override
			{
				m_actual_mbox->do_deliver_message(
						delivery_mode,
						type_index,
						message_ref,
						redirection_deep );
			}

		virtual void
		subscribe_event_handler(
			const std::type_index & type_index,
			so_5::abstract_message_sink_t & subscriber ) override
			{
				++m_subscriptions;
				m_actual_mbox->subscribe_event_handler( type_index, subscriber );
			}

		virtual void
		unsubscribe_event_handler(
			const std::type_index & type_index,
			so_5::abstract_message_sink_t & subscriber ) noexcept override
			{
				++m_unsubscriptions;
				m_actual_mbox->unsubscribe_event_handler( type_index, subscriber );
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
			so_5::abstract_message_sink_t & subscriber ) override
			{
				m_actual_mbox->set_delivery_filter( msg_type, filter, subscriber );
			}

		virtual void
		drop_delivery_filter(
			const std::type_index & msg_type,
			so_5::abstract_message_sink_t & subscriber ) noexcept override
			{
				m_actual_mbox->drop_delivery_filter( msg_type, subscriber );
			}

		so_5::environment_t &
		environment() const noexcept override
			{
				return m_actual_mbox->environment();
			}

		static so_5::mbox_t
		create( so_5::environment_t & env )
			{
				return so_5::mbox_t( new test_mbox_t( env ) );
			}
	};

struct msg_one : public so_5::signal_t {};
struct msg_two : public so_5::signal_t {};
struct msg_three : public so_5::signal_t {};
struct msg_four : public so_5::signal_t {};
struct msg_five : public so_5::signal_t {};

class a_test_t : public so_5::agent_t
	{
		typedef so_5::agent_t base_type_t;

		state_t st_1{ this, "st_1" };
		state_t st_2{ this, "st_2" };
		state_t st_3{ this, "st_3" };
		state_t st_4{ this, "st_4" };
		state_t st_5{ this, "st_5" };

		const so_5::mbox_t m_mbox;

	public :
		a_test_t(
			so_5::environment_t & env,
			so_5::subscription_storage_factory_t factory )
			:	base_type_t( env + factory )
			,	m_mbox( test_mbox_t::create( env ) )
			{
			}

		void
		so_define_agent() override
			{
				const auto subscribe_to = [this]( const state_t & st ) {
					st.event( m_mbox, &a_test_t::evt_one );
					st.event( m_mbox, &a_test_t::evt_two );
					st.event( m_mbox, &a_test_t::evt_three );
					st.event( m_mbox, &a_test_t::evt_four );
					st.event( m_mbox, &a_test_t::evt_five );
				};

				subscribe_to( so_default_state() );
				subscribe_to( st_1 );
				subscribe_to( st_2 );
				subscribe_to( st_3 );
				subscribe_to( st_4 );
				subscribe_to( st_5 );
			}

		void
		so_evt_start() override
			{
				so_deregister_agent_coop_normally();
			}

	private :
		void evt_one( mhood_t<msg_one> ) {}
		void evt_two( mhood_t<msg_two> ) {}
		void evt_three( mhood_t<msg_three> ) {}
		void evt_four( mhood_t<msg_four> ) {}
		void evt_five( mhood_t<msg_five> ) {}
	};

void
do_test()
{
	using factory_info_t =
			std::pair< std::string, so_5::subscription_storage_factory_t >;
	
	factory_info_t factories[] = {
		{ "vector[1]", so_5::vector_based_subscription_storage_factory( 1 ) }
	,	{ "vector[8]", so_5::vector_based_subscription_storage_factory( 8 ) }
	,	{ "vector[16]", so_5::vector_based_subscription_storage_factory( 16 ) }
	,	{ "map", so_5::map_based_subscription_storage_factory() }
	,	{ "hash_table", so_5::hash_table_based_subscription_storage_factory() }
	,	{ "adaptive[1]", so_5::adaptive_subscription_storage_factory( 1 ) }
	,	{ "adaptive[2]", so_5::adaptive_subscription_storage_factory( 2 ) }
	,	{ "adaptive[3]", so_5::adaptive_subscription_storage_factory( 3 ) }
	,	{ "adaptive[8]", so_5::adaptive_subscription_storage_factory( 8 ) }
	,	{ "default", so_5::default_subscription_storage_factory() }
	}; 

	for( auto & f : factories )
	{
		std::cout << "checking factory: " << f.first << " -> " << std::flush;

		run_with_time_limit(
			[f] {
				so_5::launch(
					[f]( so_5::environment_t & env )
					{
						env.register_agent_as_coop(
								env.make_agent< a_test_t >( f.second ) );
					} );
			},
			5,
			"checking factory " + f.first );

		std::cout << "OK" << std::endl;
	}
}

int
main()
{
	try
	{
		do_test();
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}
