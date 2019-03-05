/*
 * A test of simple sequence of messages for
 * prio_one_thread::quoted_round_robin dispatcher.
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

struct msg_hello : public so_5::signal_t {};

void
define_receiver_agent(
	so_5::coop_t & coop,
	so_5::disp::prio_one_thread::quoted_round_robin::dispatcher_handle_t & disp,
	so_5::priority_t priority,
	const so_5::mbox_t & common_mbox,
	std::string & sequence )
	{
		class actor_t final : public so_5::agent_t
			{
				std::string & m_seq;
			public:
				actor_t(
					context_t ctx,
					so_5::priority_t priority,
					const so_5::mbox_t & common_mbox,
					std::string & seq )
					:	so_5::agent_t{ ctx + priority }
					,	m_seq{ seq }
					{
						so_subscribe( common_mbox ).event(
							[this, priority](mhood_t<msg_hello>) {
								m_seq += std::to_string( so_5::to_size_t( priority ) );
							} );
					}
			};

		coop.make_agent_with_binder< actor_t >(
				disp.binder(),
				priority,
				std::cref(common_mbox),
				std::ref(sequence) );
	}

std::string &
define_main_agent(
	so_5::coop_t & coop,
	so_5::disp::prio_one_thread::quoted_round_robin::dispatcher_handle_t & disp,
	const so_5::mbox_t & common_mbox )
	{
		class actor_t final : public so_5::agent_t
			{
				std::string m_seq;
			public:
				actor_t( context_t ctx, const so_5::mbox_t & common_mbox )
					:	so_5::agent_t{ ctx + so_5::prio::p0 }
				{
					so_subscribe( common_mbox ).event(
						[this]( mhood_t<msg_hello> ) {
							m_seq += "0";
							if( "76543210" != m_seq )
								throw std::runtime_error(
										"Unexpected value of sequence: " + m_seq );
							else
								so_environment().stop();
						} );
				}

				std::string & sequence() noexcept { return m_seq; }
			};

		return coop.make_agent_with_binder< actor_t >(
				disp.binder(), std::cref(common_mbox) )->sequence();
	}

void
define_starter_agent(
	so_5::coop_t & coop,
	so_5::disp::prio_one_thread::quoted_round_robin::dispatcher_handle_t disp )
	{
		using disp_t = so_5::disp::prio_one_thread::quoted_round_robin::dispatcher_handle_t;

		class actor_t final : public so_5::agent_t
			{
				disp_t m_disp;
			public :
				actor_t( context_t ctx, disp_t disp )
					:	so_5::agent_t{ ctx + so_5::prio::p0 }
					,	m_disp{ std::move(disp) }
					{}

				void so_evt_start() override
					{
						auto common_mbox = so_environment().create_mbox();

						so_environment().introduce_coop(
							[&]( so_5::coop_t & child )
							{
								using namespace so_5::prio;

								std::string & sequence = define_main_agent(
										child, m_disp, common_mbox );
								define_receiver_agent(
										child, m_disp, p1, common_mbox, sequence );
								define_receiver_agent(
										child, m_disp, p2, common_mbox, sequence );
								define_receiver_agent(
										child, m_disp, p3, common_mbox, sequence );
								define_receiver_agent(
										child, m_disp, p4, common_mbox, sequence );
								define_receiver_agent(
										child, m_disp, p5, common_mbox, sequence );
								define_receiver_agent(
										child, m_disp, p6, common_mbox, sequence );
								define_receiver_agent(
										child, m_disp, p7, common_mbox, sequence );
							} );

						so_5::send< msg_hello >( common_mbox );
					}
			};

		coop.make_agent_with_binder< actor_t >( disp.binder(), disp );
	}

void
fill_coop(
	so_5::coop_t & coop )
	{
		using namespace so_5::disp::prio_one_thread::quoted_round_robin;

		define_starter_agent( coop,
			make_dispatcher( coop.environment(), quotes_t{1} ) );
	}

int
main()
{
	try
	{
		// Do several iterations to increase probability of errors detection.
		std::cout << "runing iterations" << std::flush;
		for( int i = 0; i != 100; ++i )
		{
			run_with_time_limit(
				[]()
				{
					so_5::launch(
						[]( so_5::environment_t & env )
						{
							env.introduce_coop( fill_coop );
						} );
				},
				20 );
			std::cout << "." << std::flush;
		}
		std::cout << "done" << std::endl;
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

