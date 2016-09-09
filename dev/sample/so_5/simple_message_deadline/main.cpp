/*
 * Demonstration of very simple implementation of message deadlines
 * by using collector+performer idiom.
 */

#if defined( _MSC_VER )
	#if defined( __clang__ )
		#pragma clang diagnostic ignored "-Wreserved-id-macro"
	#endif
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include <iostream>
#include <ctime>
#include <sstream>
#include <queue>

#include <so_5/all.hpp>

// A request to be processed.
struct msg_request : public so_5::message_t
{
	std::string m_id;
	std::time_t m_deadline;
	const so_5::mbox_t m_reply_to;

	msg_request(
		std::string id,
		std::time_t deadline,
		const so_5::mbox_t & reply_to )
		:	m_id( std::move( id ) )
		,	m_deadline( deadline )
		,	m_reply_to( reply_to )
	{}
};

// Just a useful alias.
using msg_request_smart_ptr_t = so_5::intrusive_ptr_t< msg_request >;

// A successful reply to request.
struct msg_positive_reply : public so_5::message_t
{
	std::string m_id;
	std::string m_result;
	std::time_t m_started_at;

	msg_positive_reply(
		std::string id,
		std::string result,
		std::time_t started_at )
		:	m_id( std::move( id ) )
		,	m_result( std::move( result ) )
		,	m_started_at( started_at )
	{}
};

// A negative reply to request.
struct msg_negative_reply : public so_5::message_t
{
	std::string m_id;
	std::time_t m_deadline;

	msg_negative_reply(
		std::string id,
		std::time_t deadline )
		:	m_id( std::move( id ) )
		,	m_deadline( deadline )
	{}
};

// Simple helper function for time formatting.
std::string time_to_string( std::time_t t )
{
	char r[ 32 ];
	std::strftime( r, sizeof(r) - 1, "%H:%M:%S", std::localtime(&t) );

	return r;
}

// Agent for generation of serie of requests.
class a_generator_t : public so_5::agent_t
{
public :
	a_generator_t( context_t ctx, so_5::mbox_t processor_mbox )
		:	so_5::agent_t( ctx )
		,	m_processor_mbox( std::move(processor_mbox) )
	{}

	virtual void so_define_agent() override
	{
		so_default_state()
				.event( &a_generator_t::evt_positive_reply )
				.event( &a_generator_t::evt_negative_reply );
	}

	virtual void so_evt_start() override
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
	const so_5::mbox_t m_processor_mbox;

	unsigned int m_expected_replies = 0;

	void evt_positive_reply( const msg_positive_reply & evt )
	{
		std::cout
				<< time_to_string( std::time(nullptr) ) << " - OK: ["
				<< evt.m_id << "], started_at: "
				<< time_to_string( evt.m_started_at )
				<< ", result: " << evt.m_result
				<< std::endl;

		count_reply();
	}

	void evt_negative_reply( const msg_negative_reply & evt )
	{
		std::cout
				<< time_to_string( std::time(nullptr) ) << " - FAIL: ["
				<< evt.m_id << "], deadline: "
				<< time_to_string( evt.m_deadline )
				<< std::endl;

		count_reply();
	}

	void count_reply()
	{
		--m_expected_replies;
		if( !m_expected_replies )
			so_deregister_agent_coop_normally();
	}
};

// Agent-collector for handling message deadlines.
class a_collector_t : public so_5::agent_t
{
public :
	struct msg_select_next_job : public so_5::signal_t {};

	a_collector_t( context_t ctx ) : so_5::agent_t( ctx )
	{}

	void set_performer_mbox( const so_5::mbox_t & mbox )
	{
		m_performer_mbox = mbox;
	}

	virtual void so_define_agent() override
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
	struct msg_check_deadline : public so_5::signal_t {};

	// Comparator for priority queue of stored requests.
	struct request_comparator_t
	{
		bool operator()(
			const msg_request_smart_ptr_t & a,
			const msg_request_smart_ptr_t & b ) const
		{
			// Request with earlier deadline has greater priority.
			return a->m_deadline > b->m_deadline;
		}
	};

	const state_t st_performer_is_free{ this };
	const state_t st_performer_is_busy{ this };

	so_5::mbox_t m_performer_mbox;

	// Queue of pending requests.
	std::priority_queue<
				msg_request_smart_ptr_t,
				std::vector< msg_request_smart_ptr_t >,
				request_comparator_t >
			m_pending_requests;

	void evt_first_request( mhood_t< msg_request > evt )
	{
		// Performer is waiting for a request.
		// So the request can be sent for processing right now.
		this >>= st_performer_is_busy;

		m_performer_mbox->deliver_message( evt.make_reference() );
	}

	void evt_yet_another_request( mhood_t< msg_request > evt )
	{
		// Performer is busy. So the request must be stored in the queue.
		// And deadline for it must be controlled.

		const std::time_t now = std::time(nullptr);
		if( now < evt->m_deadline )
		{
			m_pending_requests.push( evt.make_reference() );

			// Just use delayed signal for every pending request.
			so_5::send_delayed< msg_check_deadline >(
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

	// Reaction for request for next job from performer.
	void evt_select_next_job()
	{
		if( m_pending_requests.empty() )
			// Because there are no more pending jobs
			// we should wait them in performer_is_free state.
			this >>= st_performer_is_free;
		else
		{
			// We ara still in performer_is_busy state and
			// next job must be sent to the performer.

			auto & request = m_pending_requests.top();
			m_performer_mbox->deliver_message( request );
			m_pending_requests.pop();
		}
	}

	// Reaction for check deadline timer signal.
	void evt_check_deadline()
	{
		const std::time_t now = std::time(nullptr);

		// Just remove all jobs for which deadline is already passed.
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

	void send_negative_reply( const msg_request & request )
	{
		so_5::send< msg_negative_reply >(
				request.m_reply_to,
				request.m_id,
				request.m_deadline );
	}
};

// Agent for handling requests.
class a_performer_t : public so_5::agent_t
{
public :
	a_performer_t( context_t ctx, so_5::mbox_t collector_mbox )
		:	so_5::agent_t( ctx )
		,	m_collector_mbox( std::move(collector_mbox) )
	{}

	virtual void so_define_agent() override
	{
		so_default_state().event( &a_performer_t::evt_request );
	}

private :
	const so_5::mbox_t m_collector_mbox;

	void evt_request( const msg_request & evt )
	{
		const std::time_t started_at = std::time(nullptr);

		// Imitation of some work.
		std::this_thread::sleep_for( std::chrono::seconds(4) );

		// Reply must be sent to request generator.
		so_5::send< msg_positive_reply >(
				evt.m_reply_to,
				evt.m_id,
				"-=<" + evt.m_id + ">=-",
				started_at );

		// New job must be requested.
		so_5::send< a_collector_t::msg_select_next_job >( m_collector_mbox );
	}
};

void init( so_5::environment_t & env )
{
	using namespace so_5::disp::thread_pool;

	env.introduce_coop(
		create_private_disp( env, 3 )->binder(
				bind_params_t{}.fifo( fifo_t::individual ) ),
		[]( so_5::coop_t & c ) {
			auto collector = c.make_agent< a_collector_t >();
			auto performer = c.make_agent< a_performer_t >(
					collector->so_direct_mbox() );
			collector->set_performer_mbox( performer->so_direct_mbox() );

			c.make_agent< a_generator_t >( collector->so_direct_mbox() );
		});
}

int main()
{
	try
	{
		so_5::launch( &init );

		return 0;
	}
	catch( const std::exception & x )
	{
		std::cerr << "*** Exception caught: " << x.what() << std::endl;
	}

	return 2;
}

