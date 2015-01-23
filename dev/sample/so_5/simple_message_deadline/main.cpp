/*
 * Demonstration of very simple implementation of message deadlines
 * by using collector+performer idiom.
 */

#include <iostream>
#include <ctime>
#include <sstream>
#include <queue>

#include <so_5/all.hpp>

// A request to be processed.
struct msg_request : public so_5::rt::message_t
{
	std::string m_id;
	std::time_t m_deadline;
	const so_5::rt::mbox_t m_reply_to;

	msg_request(
		std::string id,
		std::time_t deadline,
		const so_5::rt::mbox_t & reply_to )
		:	m_id( std::move( id ) )
		,	m_deadline( deadline )
		,	m_reply_to( reply_to )
	{}
};

// Just a useful alias.
using msg_request_smart_ptr_t = so_5::intrusive_ptr_t< msg_request >;

// A reply to request.
struct msg_reply : public so_5::rt::message_t
{
	std::string m_id;
	std::string m_result;
	std::time_t m_deadline;
	std::time_t m_started_at;

	msg_reply(
		std::string id,
		std::string result,
		std::time_t deadline,
		std::time_t started_at )
		:	m_id( std::move( id ) )
		,	m_result( std::move( result ) )
		,	m_deadline( deadline )
		,	m_started_at( started_at )
	{}
};

// Simple helper function for time formatting.
std::string
time_to_string( std::time_t t )
{
	char r[ 32 ];
	std::strftime( r, sizeof(r) - 1, "%H:%M:%S", std::localtime(&t) );

	return r;
}

// Agent for generation of serie of requests.
class a_generator_t : public so_5::rt::agent_t
{
public :
	a_generator_t(
		so_5::rt::environment_t & env,
		const so_5::rt::mbox_t & processor_mbox )
		:	so_5::rt::agent_t( env )
		,	m_processor_mbox( processor_mbox )
	{}

	virtual void
	so_define_agent() override
	{
		so_default_state().event( &a_generator_t::evt_reply );
	}

	virtual void
	so_evt_start() override
	{
		unsigned int delays[] = { 1, 4, 5, 3, 9, 15, 12 };

		const std::time_t now = std::time(nullptr);

		int i = 0;
		for( auto d : delays )
		{
			std::ostringstream idstream;
			idstream << "i=" << i << ";d=" << d;

			const std::string id = idstream.str();
			const std::time_t deadline = now + d;

			so_5::send< msg_request >( m_processor_mbox,
					id,
					deadline,
					so_direct_mbox() );
			std::cout << "sent: [" << id << "], deadline: "
					<< time_to_string( deadline ) << std::endl;

			++m_expected_replies;
			++i;
		}
	}

private :
	const so_5::rt::mbox_t m_processor_mbox;

	unsigned int m_expected_replies = 0;

	void
	evt_reply( const msg_reply & evt )
	{
		std::cout
				<< time_to_string( std::time(nullptr) ) << ": "
				<< evt.m_result << ": ["
				<< evt.m_id << "], started: "
				<< time_to_string( evt.m_started_at )
				<< ", deadline: "
				<< time_to_string( evt.m_deadline )
				<< std::endl;

		--m_expected_replies;
		if( !m_expected_replies )
			so_deregister_agent_coop_normally();
	}
};

// Agent-collector for handling message deadlines.
class a_collector_t : public so_5::rt::agent_t
{
public :
	struct msg_select_next_job : public so_5::rt::signal_t {};

	a_collector_t( so_5::rt::environment_t & env )
		:	so_5::rt::agent_t( env )
	{}

	void
	set_performer_mbox( const so_5::rt::mbox_t & mbox )
	{
		m_performer_mbox = mbox;
	}

	virtual void
	so_define_agent() override
	{
		this >>= st_performer_is_free;

		st_performer_is_free
			.event( &a_collector_t::evt_first_request );

		st_performer_is_busy
			.event( &a_collector_t::evt_yet_another_request )
			.event< msg_select_next_job >( &a_collector_t::evt_select_next_job )
			.event< msg_check_deadline >( &a_collector_t::evt_check_deadline );
	}

private :
	struct msg_check_deadline : public so_5::rt::signal_t {};

	// Comparator for priority queue of stored requests.
	struct request_comparator_t
	{
		bool
		operator()(
			const msg_request_smart_ptr_t & a,
			const msg_request_smart_ptr_t & b ) const
		{
			// Request with earlier deadline has greater priority.
			return a->m_deadline > b->m_deadline;
		}
	};

	const so_5::rt::state_t st_performer_is_free = so_make_state();
	const so_5::rt::state_t st_performer_is_busy = so_make_state();

	so_5::rt::mbox_t m_performer_mbox;

	// Queue of pending requests.
	std::priority_queue<
				msg_request_smart_ptr_t,
				std::vector< msg_request_smart_ptr_t >,
				request_comparator_t >
			m_pending_requests;

	void
	evt_first_request( const so_5::rt::event_data_t< msg_request > & evt )
	{
		// Performer is waiting for a request.
		// So the request can be sent for processing right now.
		this >>= st_performer_is_busy;

		m_performer_mbox->deliver_message( evt.make_reference() );
	}

	void
	evt_yet_another_request(
		const so_5::rt::event_data_t< msg_request > & evt )
	{
		// Performer is busy. So the request must be stored in the queue.
		// And deadline for it must be controlled.

		const std::time_t now = std::time(nullptr);
		if( now < evt->m_deadline )
		{
			m_pending_requests.push( evt.make_reference() );

			// Just use delayed signal for every pending request.
			so_5::send_delayed_to_agent< msg_check_deadline >(
					*this,
					std::chrono::seconds( evt->m_deadline - now ) );
		}
		else
		{
			// Deadline is already passed.
			// Negative reply must be sent right now.
			send_negative_reply( *evt );
		}
	}

	void
	evt_select_next_job()
	{
		if( m_pending_requests.empty() )
			this >>= st_performer_is_free;
		else
		{
			auto & request = m_pending_requests.top();
			m_performer_mbox->deliver_message( request );
			m_pending_requests.pop();
		}
	}

	void
	evt_check_deadline()
	{
		const std::time_t now = std::time(nullptr);
		while( !m_pending_requests.empty() )
		{
			auto & request = m_pending_requests.top();
			if( now >= request->m_deadline )
			{
				send_negative_reply( *request );
				m_pending_requests.pop();
			}
			else
				break;
		}
	}

	void
	send_negative_reply( const msg_request & request )
	{
		so_5::send< msg_reply >(
				request.m_reply_to,
				request.m_id,
				"failed(deadline)",
				request.m_deadline,
				std::time(nullptr) );
	}
};

// Agent for handling requests.
class a_performer_t : public so_5::rt::agent_t
{
public :
	a_performer_t(
		so_5::rt::environment_t & env,
		const so_5::rt::mbox_t & collector_mbox )
		:	so_5::rt::agent_t( env )
		,	m_collector_mbox( collector_mbox )
	{}

	virtual void
	so_define_agent() override
	{
		this >>= st_free;

		st_free.event( &a_performer_t::evt_request );

		st_busy.event< msg_processing_done >(
				&a_performer_t::evt_processing_done );
	}

private :
	struct msg_processing_done : public so_5::rt::signal_t {};

	const so_5::rt::state_t st_free = so_make_state();
	const so_5::rt::state_t st_busy = so_make_state();

	const so_5::rt::mbox_t m_collector_mbox;

	// Request which is currently processed.
	msg_request_smart_ptr_t m_request;

	// When the processing started.
	std::time_t m_processing_started_at;

	void
	evt_request( const so_5::rt::event_data_t< msg_request > & evt )
	{
		this >>= st_busy;
		m_request = evt.make_reference();
		m_processing_started_at = std::time(nullptr);

		so_5::send_delayed_to_agent< msg_processing_done >(
				*this,
				std::chrono::seconds( 4 ) );
	}

	void
	evt_processing_done()
	{
		// Reply must be sent to request generator.
		so_5::send< msg_reply >(
				m_request->m_reply_to,
				m_request->m_id,
				"successful",
				m_request->m_deadline,
				m_processing_started_at );

		// Switching to free state and cleaning up resources.
		this >>= st_free;
		m_request = msg_request_smart_ptr_t();

		// New job must be requested.
		so_5::send< a_collector_t::msg_select_next_job >( m_collector_mbox );
	}
};

void
create_coop( so_5::rt::environment_t & env )
{
	using namespace so_5::disp::thread_pool;

	auto c = env.create_coop( so_5::autoname,
			create_disp_binder( "thread_pool",
				[]( params_t & p ) { p.fifo( fifo_t::individual ); } ) );

	std::unique_ptr< a_collector_t > collector{ new a_collector_t{ env } };
	std::unique_ptr< a_performer_t > performer{
			new a_performer_t{ env, collector->so_direct_mbox() } };
	collector->set_performer_mbox( performer->so_direct_mbox() );

	std::unique_ptr< a_generator_t > generator{
			new a_generator_t{ env, collector->so_direct_mbox() } };

	c->add_agent( std::move( collector ) );
	c->add_agent( std::move( performer ) );
	c->add_agent( std::move( generator ) );

	env.register_coop( std::move( c ) );
}

int
main( int argc, char ** argv )
{
	try
	{
		so_5::launch(
			create_coop,
			[]( so_5::rt::environment_params_t & params )
			{
				params.add_named_dispatcher( "thread_pool",
						so_5::disp::thread_pool::create_disp( 3 ) );
			} );

		return 0;
	}
	catch( const std::exception & x )
	{
		std::cerr << "*** Exception caught: " << x.what() << std::endl;
	}

	return 2;
}

