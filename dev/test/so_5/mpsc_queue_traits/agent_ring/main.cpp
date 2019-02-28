#include <iostream>
#include <set>
#include <chrono>

#include <cstdio>
#include <cstdlib>

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

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
					.event( &a_ring_member_t::evt_start )
					.event( &a_ring_member_t::evt_your_turn );
			}

		void
		evt_start(mhood_t< msg_start >)
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

class case_setter_t;

class case_setter_cleaner_t
	{
		case_setter_t * m_setter;

		void clean() noexcept;

	public :
		case_setter_cleaner_t( case_setter_t & setter ) noexcept : m_setter{&setter} {}
		case_setter_cleaner_t( case_setter_cleaner_t && other ) noexcept
			:	m_setter{ other.m_setter }
			{
				other.m_setter = nullptr;
			}
		~case_setter_cleaner_t() noexcept
			{
				clean();
			}

		case_setter_cleaner_t &
		operator=( case_setter_cleaner_t && other ) noexcept
			{
				clean();
				m_setter = other.m_setter;
				other.m_setter = nullptr;

				return *this;
			}
	};

class case_setter_t
	{
	public :
		case_setter_t( lock_factory_t lock_factory )
			:	m_lock_factory{ std::move(lock_factory) }
			{}
		virtual ~case_setter_t() {}

		virtual void
		tune_env_params( so_5::environment_params_t & )
			{
				// Nothing to do by default.
			}

		virtual case_setter_cleaner_t
		make_dispatcher( so_5::environment_t & env ) = 0;

		virtual so_5::disp_binder_shptr_t
		make_binder() = 0;

		virtual void
		cleanup() noexcept = 0;

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

void
case_setter_cleaner_t::clean() noexcept
	{
		if( m_setter ) m_setter->cleanup();
	}

class default_disp_setter_t final : public case_setter_t
	{
		so_5::disp_binder_shptr_t m_binder;

	public :
		using case_setter_t::case_setter_t;

		void
		tune_env_params( so_5::environment_params_t & params ) override
			{
				params.default_disp_params(
					setup_lock_factory( so_5::disp::one_thread::disp_params_t{} ) );
			}

		case_setter_cleaner_t
		make_dispatcher( so_5::environment_t & env ) override
			{
				m_binder = so_5::make_default_disp_binder( env );
				return { *this };
			}

		so_5::disp_binder_shptr_t
		make_binder() override
			{
				return m_binder;
			}

		void
		cleanup() noexcept override
			{
				m_binder.reset();
			}
	};

class one_thread_case_setter_t final : public case_setter_t
	{
		so_5::disp::one_thread::dispatcher_handle_t m_disp;

	public :
		using case_setter_t::case_setter_t;

		case_setter_cleaner_t
		make_dispatcher( so_5::environment_t & env ) override
			{
				m_disp = so_5::disp::one_thread::make_dispatcher(
						env,
						"one_thread",
						setup_lock_factory( so_5::disp::one_thread::disp_params_t{} ) );
				return { *this };
			}

		so_5::disp_binder_shptr_t
		make_binder() override
			{
				return m_disp.binder();
			}

		void
		cleanup() noexcept override
			{
				m_disp.reset();
			}
	};

class active_obj_case_setter_t : public case_setter_t
	{
		so_5::disp::active_obj::dispatcher_handle_t m_disp;

	public :
		using case_setter_t::case_setter_t;

		case_setter_cleaner_t
		make_dispatcher( so_5::environment_t & env ) override
			{
				m_disp = so_5::disp::active_obj::make_dispatcher(
						env,
						"active_obj",
						setup_lock_factory(
								so_5::disp::active_obj::disp_params_t{} ) );

				return { *this };
			}

		so_5::disp_binder_shptr_t
		make_binder() override
			{
				return m_disp.binder();
			}

		void
		cleanup() noexcept override
			{
				m_disp.reset();
			}
	};

class active_group_case_setter_t : public case_setter_t
	{
		so_5::disp::active_group::dispatcher_handle_t m_disp;

	public :
		using case_setter_t::case_setter_t;

		case_setter_cleaner_t
		make_dispatcher( so_5::environment_t & env ) override
			{
				m_disp = so_5::disp::active_group::make_dispatcher(
						env,
						"active_group",
						setup_lock_factory(
								so_5::disp::active_group::disp_params_t{} ) );

				return { *this };
			}

		so_5::disp_binder_shptr_t
		make_binder() override
			{
				auto id = ++m_id;
				return m_disp.binder( std::to_string(id) );
			}

		void
		cleanup() noexcept override
			{
				m_disp.reset();
			}

	private :
		unsigned int m_id = {0};
	};

class prio_strictly_ordered_case_setter_t : public case_setter_t
	{
		so_5::disp::prio_one_thread::strictly_ordered::dispatcher_handle_t m_disp;

	public :
		using case_setter_t::case_setter_t;

		case_setter_cleaner_t
		make_dispatcher( so_5::environment_t & env ) override
			{
				namespace disp_ns = so_5::disp::prio_one_thread::strictly_ordered;
				m_disp = disp_ns::make_dispatcher(
						env,
						"prio::strictly_ordered",
						setup_lock_factory( disp_ns::disp_params_t{} ) );

				return { *this };
			}

		so_5::disp_binder_shptr_t
		make_binder() override
			{
				return m_disp.binder();
			}

		void
		cleanup() noexcept override
			{
				m_disp.reset();
			}
	};

class prio_quoted_round_robin_case_setter_t : public case_setter_t
	{
		so_5::disp::prio_one_thread::quoted_round_robin::dispatcher_handle_t m_disp;

	public :
		using case_setter_t::case_setter_t;

		case_setter_cleaner_t
		make_dispatcher( so_5::environment_t & env ) override
			{
				namespace disp_ns = so_5::disp::prio_one_thread::quoted_round_robin;
				m_disp = disp_ns::make_dispatcher(
						env,
						"prio::quoted_round_robin",
						disp_ns::quotes_t{ 10 },
						setup_lock_factory( disp_ns::disp_params_t{} ) );
				
				return { *this };
			}

		so_5::disp_binder_shptr_t
		make_binder() override
			{
				return m_disp.binder();
			}

		void
		cleanup() noexcept override
			{
				m_disp.reset();
			}
	};

class one_per_prio_case_setter_t : public case_setter_t
	{
		so_5::disp::prio_dedicated_threads::one_per_prio::dispatcher_handle_t m_disp;

	public :
		using case_setter_t::case_setter_t;

		case_setter_cleaner_t
		make_dispatcher( so_5::environment_t & env ) override
			{
				namespace disp_ns = so_5::disp::prio_dedicated_threads::one_per_prio;
				m_disp = disp_ns::make_dispatcher(
						env,
						"prio::one_per_prio",
						setup_lock_factory( disp_ns::disp_params_t{} ) );

				return { *this };
			}

		so_5::disp_binder_shptr_t
		make_binder() override
			{
				return m_disp.binder();
			}

		void
		cleanup() noexcept override
			{
				m_disp.reset();
			}
	};

void
create_coop(
	so_5::environment_t & env,
	case_setter_t & setter )
	{
		auto setter_cleaner = setter.make_dispatcher( env );

		so_5::mbox_t first_agent_mbox = env.introduce_coop(
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
								setter.make_binder() );
						agents.push_back( member );
						mboxes.push_back( member->so_direct_mbox() );
					}

				for( unsigned int i = 0; i != ring_size; ++i )
					{
						agents[ i ]->set_next_mbox(
								mboxes[ (i + 1) % ring_size ] );
					}

				return mboxes[ 0 ]; 
			} );

		so_5::send< a_ring_member_t::msg_start >( first_agent_mbox );
	}

using case_maker_t = std::function<
	case_setter_unique_ptr_t(lock_factory_t) >;

template< typename Setter >
case_maker_t maker()
	{
		return []( lock_factory_t lock_factory ) {
			return std::make_unique< Setter >( std::move(lock_factory) );
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

