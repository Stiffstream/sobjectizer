/*
 * A test of simple quoted processing of sequence of messages for
 * prio_one_thread::quoted_round_robin dispatcher.
 */

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

struct msg_receiver_started : public so_5::signal_t {};
struct msg_send_messages : public so_5::signal_t {};

struct msg_request : public so_5::signal_t {};
struct msg_reply : public so_5::message_t
	{
		so_5::priority_t m_priority;

		msg_reply( so_5::priority_t priority ) : m_priority( priority )
			{}
	};


void
define_receiver_agent(
	so_5::coop_t & coop,
	so_5::disp::prio_one_thread::quoted_round_robin::private_dispatcher_t & disp,
	so_5::priority_t priority,
	const so_5::mbox_t & common_mbox )
	{
		coop.define_agent( coop.make_agent_context() + priority, disp.binder() )
			.on_start( [common_mbox] {
					so_5::send< msg_receiver_started >( common_mbox );
				} )
			.event< msg_request >(
				common_mbox,
				[priority, common_mbox] {
					so_5::send< msg_reply >( common_mbox, priority );
				} );
	}

void
define_message_sender(
	so_5::coop_t & coop,
	so_5::disp::prio_one_thread::quoted_round_robin::private_dispatcher_t & disp,
	const so_5::mbox_t & common_mbox )
	{
		coop.define_agent( coop.make_agent_context() + so_5::prio::p0, disp.binder() )
			.event< msg_send_messages >( common_mbox, [common_mbox] {
					for( int i = 0; i != 20; ++i )
						so_5::send< msg_request >( common_mbox );
				} );
	}

void
define_supervison_agent(
	so_5::coop_t & coop,
	const so_5::mbox_t & common_mbox )
	{
		struct supervisor_data
			{
				const std::string m_expected_value;
				std::string m_accumulator;
				const std::size_t m_expected_receivers;
				std::size_t m_started_receivers = 0;
				const std::size_t m_expected_replies;
				std::size_t m_replies = 0;

				supervisor_data(
					std::string expected_value,
					std::size_t expected_receivers,
					std::size_t expected_replies )
					:	m_expected_value( std::move( expected_value ) )
					,	m_expected_receivers( expected_receivers )
					,	m_expected_replies( expected_replies )
					{}
			};

		auto data = std::make_shared< supervisor_data >(
				"777775555333"
				"777775555333"
				"777775555333"
				"777775555333"
				"5555333"
				"333"
				"33",
				3,
				3 * 20u );

		coop.define_agent()
			.event< msg_receiver_started >( common_mbox, [common_mbox, data] {
					data->m_started_receivers += 1;
					if( data->m_started_receivers == data->m_expected_receivers )
						so_5::send< msg_send_messages >( common_mbox );
				} )
			.event( common_mbox,
				[&coop, data]( const msg_reply & reply ) {
					data->m_replies += 1;
					data->m_accumulator += std::to_string(
							so_5::to_size_t( reply.m_priority ) );

					if( data->m_replies >= data->m_expected_replies )
					{
						if( data->m_expected_value != data->m_accumulator )
							throw std::runtime_error( "values mismatch: "
									"expected: " + data->m_expected_value +
									", actual: " + data->m_accumulator );
						else
							coop.environment().stop();
					}
				} );
	}

void
fill_coop(
	so_5::coop_t & coop )
	{
		using namespace so_5::disp::prio_one_thread::quoted_round_robin;
		using namespace so_5::prio;

		auto common_mbox = coop.environment().create_mbox();
		auto rr_disp = create_private_disp( coop.environment(),
				quotes_t{2}
					.set( p7, 5 )
					.set( p5, 4 )
					.set( p3, 3 ) );

		define_supervison_agent( coop, common_mbox );
		define_message_sender( coop, *rr_disp, common_mbox );
		define_receiver_agent( coop, *rr_disp, p7, common_mbox );
		define_receiver_agent( coop, *rr_disp, p5, common_mbox );
		define_receiver_agent( coop, *rr_disp, p3, common_mbox );
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
				20,
				"simple sequence prio_one_thread::quoted_round_robin dispatcher test" );
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

