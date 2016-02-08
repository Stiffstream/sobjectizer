/*
 * A test of simple sequence of messages for prio_one_thread::strictly_ordered
 * dispatcher.  A starting msg_hello is sent from outside of
 * prio_one_thread::strictly_ordered workin thread.
 */

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

struct msg_hello : public so_5::signal_t {};

void
define_receiver_agent(
	so_5::coop_t & coop,
	so_5::priority_t priority,
	const so_5::mbox_t & common_mbox,
	std::string & sequence )
	{
		coop.define_agent( coop.make_agent_context() + priority )
			.event< msg_hello >(
				common_mbox,
				[priority, &sequence] {
					sequence += std::to_string(
						static_cast< std::size_t >( priority ) );
				} );
	}

std::string &
define_main_agent(
	so_5::coop_t & coop,
	const so_5::mbox_t & common_mbox )
	{
		auto sequence = std::make_shared< std::string >();

		coop.define_agent( coop.make_agent_context() + so_5::prio::p0 )
			.event< msg_hello >(
				common_mbox,
				[&coop, sequence] {
					*sequence += "0";
					if( "76543210" != *sequence )
						throw std::runtime_error( "Unexpected value of sequence: " +
								*sequence );
					else
						coop.environment().stop();
				} );

		return *sequence;
	}

void
fill_coop(
	const so_5::mbox_t & common_mbox,
	so_5::coop_t & coop )
	{
		using namespace so_5::prio;

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

							auto common_mbox = env.create_mbox();
							env.introduce_coop(
								create_private_disp( env )->binder(),
								[&common_mbox]( so_5::coop_t & coop ) {
									fill_coop( common_mbox, coop );
								} );

							so_5::send< msg_hello >( common_mbox );
						} );
				},
				20,
				"simple sequence prio_one_thread::strictly_ordered dispatcher test" );
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

