#include <iostream>
#include <set>
#include <chrono>

#include <cstdio>
#include <cstdlib>

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

class a_ring_member_t : public so_5::agent_t
	{
	public :
		struct msg_start : public so_5::signal_t {};

		struct msg_your_turn
			{
				unsigned long long m_request_number;
			};

		a_ring_member_t( context_t ctx )
			:	so_5::agent_t( ctx )
			{}

		void
		set_next_mbox( const so_5::mbox_t & mbox )
			{
				m_next_mbox = mbox;
			}

		virtual void
		so_define_agent()
			{
				so_default_state()
					.event< msg_start >( &a_ring_member_t::evt_start )
					.event( &a_ring_member_t::evt_your_turn );
			}

		void
		evt_start()
			{
				so_5::send< msg_your_turn >( m_next_mbox, 0ull );
			}

		void
		evt_your_turn( const msg_your_turn & evt )
			{
				++m_rounds_passed;
				if( m_rounds_passed < m_rounds )
					so_5::send< msg_your_turn >( m_next_mbox, evt.m_request_number + 1 );
				else
					{
						so_environment().stop();
					}
			}

	private :
		so_5::mbox_t m_self_mbox;
		so_5::mbox_t m_next_mbox;

		unsigned int m_rounds_passed = 0;
		const unsigned int m_rounds = 20;
	};

using lock_factory_t = so_5::disp::mpsc_queue_traits::lock_factory_t;

class case_setter_t
	{
	public :
		case_setter_t( lock_factory_t lock_factory )
			:	m_lock_factory{ std::move(lock_factory) }
			{}
		virtual ~case_setter_t() {}

		virtual void
		tune_env_params( so_5::environment_params_t & ) = 0;

		virtual so_5::disp_binder_unique_ptr_t
		binder() = 0;

	protected :
		const lock_factory_t &
		lock_factory() const { return m_lock_factory; }

		template< typename P >
		P
		setup_lock_factory( P params ) const
			{
				params.tune_queue_params(
					[&]( so_5::disp::mpsc_queue_traits::queue_params_t & p ) {
						p.lock_factory( m_lock_factory );
					} );

				return params;
			}

	private :
		const lock_factory_t m_lock_factory;
	};

using case_setter_unique_ptr_t = std::unique_ptr< case_setter_t >;

class default_disp_setter_t : public case_setter_t
	{
	public :
		default_disp_setter_t( lock_factory_t lock_factory )
			:	case_setter_t{ std::move(lock_factory) }
			{}

		virtual void
		tune_env_params( so_5::environment_params_t & params ) override
			{
				params.default_disp_params(
					setup_lock_factory( so_5::disp::one_thread::disp_params_t{} ) );
			}

		virtual so_5::disp_binder_unique_ptr_t
		binder() override
			{
				return so_5::create_default_disp_binder();
			}
	};

class one_thread_case_setter_t : public case_setter_t
	{
	public :
		one_thread_case_setter_t( lock_factory_t lock_factory )
			:	case_setter_t{ std::move(lock_factory) }
			{}

		virtual void
		tune_env_params( so_5::environment_params_t & params ) override
			{
				params.add_named_dispatcher(
					"one_thread",
					so_5::disp::one_thread::create_disp(
						setup_lock_factory( so_5::disp::one_thread::disp_params_t{} ) )
				);
			}
		virtual so_5::disp_binder_unique_ptr_t
		binder() override
			{
				return so_5::disp::one_thread::create_disp_binder( "one_thread" );
			}
	};

class active_obj_case_setter_t : public case_setter_t
	{
	public :
		active_obj_case_setter_t( lock_factory_t lock_factory )
			:	case_setter_t{ std::move(lock_factory) }
			{}

		virtual void
		tune_env_params( so_5::environment_params_t & params ) override
			{
				params.add_named_dispatcher(
					"active_obj",
					so_5::disp::active_obj::create_disp(
						setup_lock_factory( so_5::disp::active_obj::disp_params_t{} ) )
				);
			}

		virtual so_5::disp_binder_unique_ptr_t
		binder() override
			{
				return so_5::disp::active_obj::create_disp_binder( "active_obj" );
			}
	};

class active_group_case_setter_t : public case_setter_t
	{
	public :
		active_group_case_setter_t( lock_factory_t lock_factory )
			:	case_setter_t{ std::move(lock_factory) }
			{}

		virtual void
		tune_env_params( so_5::environment_params_t & params ) override
			{
				params.add_named_dispatcher(
					"active_group",
					so_5::disp::active_group::create_disp(
						setup_lock_factory( so_5::disp::active_group::disp_params_t{} ) )
				);
			}

		virtual so_5::disp_binder_unique_ptr_t
		binder() override
			{
				auto id = ++m_id;
				return so_5::disp::active_group::create_disp_binder(
						"active_group", std::to_string(id) );
			}

	private :
		unsigned int m_id = {0};
	};

class prio_strictly_ordered_case_setter_t : public case_setter_t
	{
	public :
		prio_strictly_ordered_case_setter_t( lock_factory_t lock_factory )
			:	case_setter_t{ std::move(lock_factory) }
			{}

		virtual void
		tune_env_params( so_5::environment_params_t & params ) override
			{
				using namespace so_5::disp::prio_one_thread::strictly_ordered;
				params.add_named_dispatcher(
					"prio::strictly_ordered",
					create_disp( setup_lock_factory( disp_params_t{} ) )
				);
			}

		virtual so_5::disp_binder_unique_ptr_t
		binder() override
			{
				using namespace so_5::disp::prio_one_thread::strictly_ordered;
				return create_disp_binder( "prio::strictly_ordered" );
			}
	};

class prio_quoted_round_robin_case_setter_t : public case_setter_t
	{
	public :
		prio_quoted_round_robin_case_setter_t( lock_factory_t lock_factory )
			:	case_setter_t{ std::move(lock_factory) }
			{}

		virtual void
		tune_env_params( so_5::environment_params_t & params ) override
			{
				using namespace so_5::disp::prio_one_thread::quoted_round_robin;
				params.add_named_dispatcher(
					"prio::quoted_round_robin",
					create_disp( quotes_t{ 10 }, setup_lock_factory( disp_params_t{} ) )
				);
			}

		virtual so_5::disp_binder_unique_ptr_t
		binder() override
			{
				using namespace so_5::disp::prio_one_thread::quoted_round_robin;
				return create_disp_binder( "prio::quoted_round_robin" );
			}
	};

class one_per_prio_case_setter_t : public case_setter_t
	{
	public :
		one_per_prio_case_setter_t( lock_factory_t lock_factory )
			:	case_setter_t{ std::move(lock_factory) }
			{}

		virtual void
		tune_env_params( so_5::environment_params_t & params ) override
			{
				using namespace so_5::disp::prio_dedicated_threads::one_per_prio;
				params.add_named_dispatcher(
					"prio::one_per_prio",
					create_disp( setup_lock_factory( disp_params_t{} ) ) );
			}

		virtual so_5::disp_binder_unique_ptr_t
		binder() override
			{
				using namespace so_5::disp::prio_dedicated_threads::one_per_prio;
				return create_disp_binder( "prio::one_per_prio" );
			}
	};

void
create_coop(
	so_5::environment_t & env,
	case_setter_t & setter )
	{
		so_5::mbox_t first_agent_mbox;

		env.introduce_coop(
			[&]( so_5::coop_t & coop )
			{
				const std::size_t ring_size = 16;

				std::vector< a_ring_member_t * > agents;
				agents.reserve( ring_size );

				std::vector< so_5::mbox_t > mboxes;
				mboxes.reserve( ring_size );

				for( unsigned int i = 0; i != ring_size; ++i )
					{
						auto member = coop.make_agent_with_binder< a_ring_member_t >(
								setter.binder() );
						agents.push_back( member );
						mboxes.push_back( member->so_direct_mbox() );
					}

				for( unsigned int i = 0; i != ring_size; ++i )
					{
						agents[ i ]->set_next_mbox(
								mboxes[ (i + 1) % ring_size ] );
					}

				first_agent_mbox = mboxes[ 0 ]; 
			} );

		so_5::send< a_ring_member_t::msg_start >( first_agent_mbox );
	}

using case_maker_t = std::function<
	case_setter_unique_ptr_t(lock_factory_t) >;

template< typename SETTER >
case_maker_t maker()
	{
		return []( lock_factory_t lock_factory ) {
			case_setter_unique_ptr_t setter{ new SETTER{ std::move(lock_factory) } };
			return setter;
		};
	}

void
do_test()
	{
		struct case_info_t
			{
				std::string m_disp_name;
				case_maker_t m_maker;
			};
		std::vector< case_info_t > cases;
		cases.push_back( case_info_t{
				"default_disp", maker< default_disp_setter_t >() } );
		cases.push_back( case_info_t{
				"one_thread", maker< one_thread_case_setter_t >() } );
		cases.push_back( case_info_t{
				"active_obj", maker< active_obj_case_setter_t >() } );
		cases.push_back( case_info_t{
				"active_group", maker< active_group_case_setter_t >() } );
		cases.push_back( case_info_t{
				"prio::strictly_ordered",
				maker< prio_strictly_ordered_case_setter_t >() } );
		cases.push_back( case_info_t{
				"prio::quoted_round_robin",
				maker< prio_quoted_round_robin_case_setter_t >() } );
		cases.push_back( case_info_t{
				"prio::one_per_prio",
				maker< one_per_prio_case_setter_t >() } );

		struct lock_factory_info_t
			{
				std::string m_name;
				lock_factory_t m_factory;
			};
		std::vector< lock_factory_info_t > factories;
		factories.push_back( lock_factory_info_t{
				"combined_lock", so_5::disp::mpsc_queue_traits::combined_lock_factory() } );
		factories.push_back( lock_factory_info_t{
				"combined_lock(1s)",
				so_5::disp::mpsc_queue_traits::combined_lock_factory(
						std::chrono::seconds(1) ) } );
		factories.push_back( lock_factory_info_t{
				"simple_lock",
				so_5::disp::mpsc_queue_traits::simple_lock_factory() } );

		for( const auto & c : cases )
			for( const auto & f : factories )
				{
					std::cout << "--- " << c.m_disp_name << "+" << f.m_name << "---"
							<< std::endl;

					run_with_time_limit( [&] {
								auto setter = c.m_maker( f.m_factory );

								so_5::launch(
									[&]( so_5::environment_t & env ) {
										create_coop( env, *setter );
									},
									[&]( so_5::environment_params_t & params ) {
										setter->tune_env_params( params );
									} );
							},
							100,
							"dispatcher: " + c.m_disp_name + ", lock: " + f.m_name );

					std::cout << "--- DONE ---" << std::endl;
				}
	}

int
main()
{
	try
	{
		do_test();

		return 0;
	}
	catch( const std::exception & x )
	{
		std::cerr << "*** Exception caught: " << x.what() << std::endl;
	}

	return 2;
}

