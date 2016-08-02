/*
 * Test for calling so_drop_subscription in service-handler which is
 * a lambda-function.
 */

#include <iostream>

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

class a_service_provider_t : public so_5::agent_t
{
public :
	struct service_mbox : public so_5::message_t
	{
		const so_5::mbox_t m_mbox;
		service_mbox( so_5::mbox_t mbox ) : m_mbox(std::move(mbox)) {}
	};
	struct request : public so_5::signal_t {};
	struct done : public so_5::signal_t {};

	a_service_provider_t( context_t ctx, so_5::mbox_t target )
		: so_5::agent_t( ctx )
		, m_target( std::move(target) )
	{}

	virtual void
	so_evt_start() override
	{
		for( std::size_t i = 0; i != 1000; ++i )
		{
			auto unique_mbox = so_environment().create_mbox();
			std::ostringstream ss;
			ss << "request_from_" << unique_mbox->id() << "_accepted";
			auto reply_string = ss.str();

			so_subscribe( unique_mbox ).event< request >(
				[this, unique_mbox, reply_string]() -> std::string {
					so_drop_subscription< request >( unique_mbox );
					return reply_string;
				} );
			so_5::send< service_mbox >( m_target, unique_mbox );
		}

		so_5::send< done >( m_target );
	}

private :
	const so_5::mbox_t m_target;
};

class a_service_consumer_t : public so_5::agent_t
{
public :
	a_service_consumer_t( context_t ctx ) : so_5::agent_t( ctx ) {}

	virtual void
	so_define_agent() override
	{
		so_subscribe_self()
			.event( &a_service_consumer_t::on_service_mbox )
			.event( &a_service_consumer_t::on_done );
	}

	virtual void
	so_evt_finish() override
	{
		std::cout << "values_received: " << m_values_received << std::endl;
	}

private :
	unsigned int m_values_received = 0;

	void
	on_service_mbox( const a_service_provider_t::service_mbox & msg )
	{
		const auto s =
			so_5::request_value< std::string, a_service_provider_t::request >(
				msg.m_mbox, so_5::infinite_wait );
		if( !s.empty() )
			++m_values_received;
	}

	void
	on_done( mhood_t< a_service_provider_t::done > )
	{
		so_deregister_agent_coop_normally();
	}
};

void
init( so_5::environment_t & env )
{
	env.introduce_coop(
		so_5::disp::active_obj::create_private_disp( env )->binder(),
		[]( so_5::coop_t & coop ) {
			auto consumer = coop.make_agent< a_service_consumer_t >();
			coop.make_agent< a_service_provider_t >( consumer->so_direct_mbox() );
		} );
}

int
main()
{
	try
	{
		run_with_time_limit(
			[]()
			{
				so_5::launch( &init );
			},
			20,
			"so_drop_subscription in lambda service-handler" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

