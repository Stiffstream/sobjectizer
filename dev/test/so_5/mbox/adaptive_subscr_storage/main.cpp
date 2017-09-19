/*
 * A test for adaptive subscription storage.
 */

#include <iostream>
#include <sstream>

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

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

		virtual ~test_mbox_t() override
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
			const std::type_index & type_index,
			const so_5::message_ref_t & message_ref,
			unsigned int overlimit_reaction_deep ) const override
			{
				m_actual_mbox->do_deliver_message(
						type_index, message_ref, overlimit_reaction_deep );
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
			const std::type_index & type_index,
			const so_5::message_limit::control_block_t * limit,
			so_5::agent_t * subscriber ) override
			{
				++m_subscriptions;
				m_actual_mbox->subscribe_event_handler( type_index, limit, subscriber );
			}

		virtual void
		unsubscribe_event_handlers(
			const std::type_index & type_index,
			so_5::agent_t * subscriber ) override
			{
				++m_unsubscriptions;
				m_actual_mbox->unsubscribe_event_handlers( type_index, subscriber );
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

class a_test_t : public so_5::agent_t
{
		typedef so_5::agent_t base_type_t;

	public :
		a_test_t(
			so_5::environment_t & env,
			so_5::subscription_storage_factory_t factory )
			:	base_type_t( env + factory )
			,	m_mbox( test_mbox_t::create( env ) )
		{
		}

		void
		so_define_agent()
		{
			this >>= st_1_1;
			st_1_1.event< next >( m_mbox, &a_test_t::evt_st_1_1 );
		}

		void
		so_evt_start()
		{
			so_5::send< next >( m_mbox );
		}

	private :
		struct next : public so_5::signal_t {};

		so_5::mbox_t m_mbox;

		so_5::state_t st_1_1{ this, "st_1_1" };
		so_5::state_t st_1_2{ this, "st_1_2" };
		so_5::state_t st_1_3{ this, "st_1_3" };
		so_5::state_t st_1_4{ this, "st_1_4" };
		so_5::state_t st_1_5{ this, "st_1_5" };
		so_5::state_t st_1_6{ this, "st_1_6" };
		so_5::state_t st_1_7{ this, "st_1_7" };
		so_5::state_t st_1_8{ this, "st_1_8" };

		so_5::state_t st_2_1{ this, "st_2_1" };
		so_5::state_t st_2_2{ this, "st_2_2" };
		so_5::state_t st_2_3{ this, "st_2_3" };
		so_5::state_t st_2_4{ this, "st_2_4" };
		so_5::state_t st_2_5{ this, "st_2_5" };
		so_5::state_t st_2_6{ this, "st_2_6" };
		so_5::state_t st_2_7{ this, "st_2_7" };
		so_5::state_t st_2_8{ this, "st_2_8" };

		so_5::state_t st_finish{ this, "st_finish" };

		void
		perform_action(
			const so_5::state_t & next_state,
			void (a_test_t::*event_handler)() )
		{
			this >>= next_state;
			next_state.event< next >( m_mbox, event_handler );
			so_5::send< next >( m_mbox );
		}

		void
		evt_st_1_1()
		{
			perform_action( st_1_2, &a_test_t::evt_st_1_2 );
		}

		void
		evt_st_1_2()
		{
			perform_action( st_1_3, &a_test_t::evt_st_1_3 );
		}

		void
		evt_st_1_3()
		{
			perform_action( st_1_4, &a_test_t::evt_st_1_4 );
		}

		void
		evt_st_1_4()
		{
			perform_action( st_1_5, &a_test_t::evt_st_1_5 );
		}

		void
		evt_st_1_5()
		{
			perform_action( st_1_6, &a_test_t::evt_st_1_6 );
		}

		void
		evt_st_1_6()
		{
			perform_action( st_1_7, &a_test_t::evt_st_1_7 );
		}

		void
		evt_st_1_7()
		{
			perform_action( st_1_8, &a_test_t::evt_st_1_8 );
		}

		void
		evt_st_1_8()
		{
			// Subscription storage must switch back from large to small.
			so_drop_subscription< next >( m_mbox, st_1_1 );
			so_drop_subscription< next >( m_mbox, st_1_2 );
			so_drop_subscription< next >( m_mbox, st_1_3 );
			so_drop_subscription< next >( m_mbox, st_1_4 );
			so_drop_subscription< next >( m_mbox, st_1_5 );
			so_drop_subscription< next >( m_mbox, st_1_6 );
			so_drop_subscription< next >( m_mbox, st_1_7 );
			so_drop_subscription< next >( m_mbox, st_1_8 );

			this >>= st_2_1;
			st_2_1.event< next >( m_mbox, &a_test_t::evt_st_2_1 );
			so_5::send< next >( m_mbox );
		}

		void
		evt_st_2_1()
		{
			perform_action( st_2_2, &a_test_t::evt_st_2_2 );
		}

		void
		evt_st_2_2()
		{
			perform_action( st_2_3, &a_test_t::evt_st_2_3 );
		}

		void
		evt_st_2_3()
		{
			perform_action( st_2_4, &a_test_t::evt_st_2_4 );
		}

		void
		evt_st_2_4()
		{
			perform_action( st_2_5, &a_test_t::evt_st_2_5 );
		}

		void
		evt_st_2_5()
		{
			perform_action( st_2_6, &a_test_t::evt_st_2_6 );
		}

		void
		evt_st_2_6()
		{
			perform_action( st_2_7, &a_test_t::evt_st_2_7 );
		}

		void
		evt_st_2_7()
		{
			perform_action( st_2_8, &a_test_t::evt_st_2_8 );
		}

		void
		evt_st_2_8()
		{
			// Subscription storage must switch back from large to small.
			so_drop_subscription_for_all_states< next >( m_mbox );

			this >>= st_finish;
			st_finish.event< next >(
					m_mbox,
					[this]{
						so_deregister_agent_coop_normally(); } );
			so_5::send< next >( m_mbox );
		}
};

void
do_test()
{
	using namespace std;
	using namespace so_5;

	const std::size_t threshold = 4;

	using factory_info_t =
			pair< std::string, subscription_storage_factory_t >;
	
	factory_info_t factories[] = {
		{ "default", adaptive_subscription_storage_factory( threshold ) }
	,	{ "vector+hash_table",
			adaptive_subscription_storage_factory(
					threshold,
					vector_based_subscription_storage_factory( threshold ),
					hash_table_based_subscription_storage_factory() ) }
	,	{ "hash_table+vector",
			adaptive_subscription_storage_factory(
					threshold,
					hash_table_based_subscription_storage_factory(),
					vector_based_subscription_storage_factory( threshold ) ) }
	,	{ "map+hash_table",
			adaptive_subscription_storage_factory(
					threshold,
					map_based_subscription_storage_factory(),
					hash_table_based_subscription_storage_factory() ) }
	,	{ "hash_table+map",
			adaptive_subscription_storage_factory(
					threshold,
					hash_table_based_subscription_storage_factory(),
					map_based_subscription_storage_factory() ) }
	,	{ "vector+map",
			adaptive_subscription_storage_factory(
					threshold,
					vector_based_subscription_storage_factory( threshold ),
					map_based_subscription_storage_factory() ) }
	,	{ "map+vector",
			adaptive_subscription_storage_factory(
					threshold,
					map_based_subscription_storage_factory(),
					vector_based_subscription_storage_factory( threshold ) ) }
	}; 

	for( auto & f : factories )
	{
		std::cout << "checking factory: " << f.first << " -> " << std::flush;

		run_with_time_limit(
			[&f] {
				for( int i = 0; i != 10; ++i )
					so_5::launch(
						[&]( so_5::environment_t & env )
						{
							env.register_agent_as_coop( "test",
								new a_test_t( env, f.second ) );
						} );
			}, 
			20,
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
