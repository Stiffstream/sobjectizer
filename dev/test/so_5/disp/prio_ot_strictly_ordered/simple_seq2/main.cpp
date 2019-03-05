/*
 * A test of simple sequence of messages for prio_one_thread::strictly_ordered
 * dispatcher.  A starting msg_hello is sent from on_start() action from the
 * same coop.
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

struct msg_hello : public so_5::signal_t {};

void
define_receiver_agent(
	so_5::coop_t & coop,
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

		coop.make_agent< actor_t >(
				priority,
				std::cref(common_mbox),
				std::ref(sequence) );
	}

std::string &
define_main_agent(
	so_5::coop_t & coop,
	const so_5::mbox_t & common_mbox )
	{
		class actor_t final : public so_5::agent_t
			{
				std::string m_seq;
				const so_5::mbox_t m_common_mbox;

			public:
				actor_t( context_t ctx, so_5::mbox_t common_mbox )
					:	so_5::agent_t{ ctx + so_5::prio::p0 }
					,	m_common_mbox{ std::move(common_mbox) }
					{
						so_subscribe( m_common_mbox ).event(
							[this]( mhood_t<msg_hello> ) {
								m_seq += "0";
								if( "76543210" != m_seq )
									throw std::runtime_error(
											"Unexpected value of sequence: " + m_seq );
								else
									so_environment().stop();
							} );
					}

				void so_evt_start() override
					{
						so_5::send< msg_hello >( m_common_mbox );
					}

				std::string & sequence() noexcept { return m_seq; }
			};

		return coop.make_agent< actor_t >( common_mbox )->sequence();
	}

void
fill_coop(
	so_5::coop_t & coop )
	{
		using namespace so_5::prio;

		auto common_mbox = coop.environment().create_mbox();
		std::string & sequence = define_main_agent( coop, common_mbox );
		define_receiver_agent( coop, p1, common_mbox, sequence );
		define_receiver_agent( coop, p2, common_mbox, sequence );
		define_receiver_agent( coop, p3, common_mbox, sequence );
		define_receiver_agent( coop, p4, common_mbox, sequence );
		define_receiver_agent( coop, p5, common_mbox, sequence );
		define_receiver_agent( coop, p6, common_mbox, sequence );
		define_receiver_agent( coop, p7, common_mbox, sequence );
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
							using namespace so_5::disp::prio_one_thread::strictly_ordered;
							env.introduce_coop(
								make_dispatcher( env ).binder(),
								fill_coop );
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

