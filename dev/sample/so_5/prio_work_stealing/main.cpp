/*
 * An example for demonstration of prio_dedicated_threads::one_per_prio
 * dispatcher.
 *
 * There is a request generator agent. It randomly sends request for
 * creating images. Every request contains ID, dimension of the image
 * to be generated and some metadata. It is supposed that time for
 * image creation depends from image dimension -- big dimension leads
 * to big amount of time for image creation.
 *
 * Requests are processed by several agents.
 *
 * First of all there are two managers. One of them (request_acceptor)
 * receives new requests, calculates priorities for them and stores
 * requests in queues (one queue for each priority).
 *
 * Second manager (request_scheduler) schedules requests from queues to
 * actual processor agents.
 *
 * Request_acceptor and request_scheduler works on the same
 * prio_one_thread::strictly_ordered dispatcher. Request_acceptor agent
 * has lower priority than request_scheduler agent. It means that
 * messages for scheduling requests to processor agents are processed before
 * messages about new requests.
 *
 * There are several processor agents: one for each priority. All processor
 * agents are bound to prio_dedicated_threads::one_per_prio dispatcher.
 * It means that every processor works on separate thread.
 *
 * Request_scheduler implements work stealing mechanism.
 * When a processor with priority P tells that it is free and ready for
 * processing new image then request_scheduler tries to find next request of
 * the same priority P.  If there is no such requests then request_scheduler
 * triest to find a request for lower priority (P-1) and so on.
 */

#include <algorithm>
#include <chrono>
#include <iostream>
#include <iterator>
#include <random>
#include <queue>

#include <so_5/all.hpp>

//
// Auxilary tools.
//

// Helper function to generate a random integer in the specified range.
unsigned int random_value( unsigned int left, unsigned int right )
	{
		std::random_device rd;
		std::mt19937 gen{ rd() };
		return std::uniform_int_distribution< unsigned int >{left, right}(gen);
	}

// Imitation of some hard-working.
// Blocks the current thread for random amount of time.
void imitate_hard_work( unsigned int pause )
	{
		std::this_thread::sleep_for( std::chrono::milliseconds{ pause } );
	}

// Type of clock to work with time values.
using clock_type = std::chrono::steady_clock;

//
// Messages for interaction between requests generator and
// requests scheduler.
//

// Maximum dimension allowed.
const unsigned int max_dimension = 10000;

// Request metadata. Contains information related to request processing.
struct request_metadata
	{
		// When request was generated.
		clock_type::time_point m_generated_at;
		// When request was queued.
		clock_type::time_point m_queued_at;
		// When processing of request was started.
		clock_type::time_point m_processing_started_at;
		// When processing of request was finished.
		clock_type::time_point m_processing_finished_at;

		// Priority initially assigned to request.
		so_5::priority_t m_queue_prio;
		// Priority at which request was processed.
		so_5::priority_t m_processor_prio;
	};

// Alias for shared_ptr to request_metadata.
using request_metadata_shptr = std::shared_ptr< request_metadata >;

// Request for image generation.
struct generation_request : public so_5::message_t
	{
		const unsigned int m_id;
		const unsigned int m_dimension;
		request_metadata_shptr m_metadata;

		generation_request(
			unsigned int id,
			unsigned int dimension,
			request_metadata_shptr metadata )
			:	m_id( id )
			,	m_dimension( dimension )
			,	m_metadata( std::move(metadata) )
			{}
	};

// Alias for smart pointer to generation_request. It will be used
// for storing and resending pending requests.
using generation_request_shptr = so_5::intrusive_ptr_t< generation_request >;

// Positive response for image generation request.
struct generation_result : public so_5::message_t
	{
		const unsigned int m_id;
		request_metadata_shptr m_metadata;

		generation_result(
			unsigned int id,
			request_metadata_shptr metadata )
			:	m_id( id )
			,	m_metadata( metadata )
			{}
	};

// Negative response for image generation request.
struct generation_rejected : public so_5::message_t
	{
		const unsigned int m_id;

		generation_rejected( unsigned int id )
			:	m_id( id )
			{}
	};

//
// Request generator agent.
//
class request_generator : public so_5::agent_t
	{
		// Signal which agent sends to itself with random delays.
		// A new request will be produced for every occurrence of that signal.
		struct produce_next : public so_5::signal_t {};

	public :
		request_generator(
			context_t ctx,
			so_5::mbox_t interaction_mbox )
			:	so_5::agent_t( ctx )
			,	m_interaction_mbox( std::move( interaction_mbox ) )
			{}

		virtual void so_define_agent() override
			{
				so_subscribe_self()
					.event< produce_next >( &request_generator::evt_produce_next );

				so_subscribe( m_interaction_mbox )
					.event( &request_generator::evt_generation_result )
					.event( &request_generator::evt_generation_rejected );
			}

		virtual void so_evt_start() override
			{
				// Will start requests generation immediately.
				so_5::send< produce_next >( *this );
			}

	private :
		// Mbox for sending new requests and receiving responses.
		const so_5::mbox_t m_interaction_mbox;

		// Counter for request ID generation.
		unsigned int m_last_id = 0;

		// Values for make distribution in the proportion 60%/30%/10%.
		unsigned int m_x = 0;
		unsigned int m_y = 0;
		unsigned int m_z = 0;

		void evt_produce_next()
			{
				auto id = ++m_last_id;
				auto dimension = generate_next_dimension();

				auto metadata = std::make_shared< request_metadata >();
				metadata->m_generated_at = clock_type::now();

				so_5::send< generation_request >( m_interaction_mbox,
						id, dimension, std::move( metadata ) );

				std::cout << "generated {" << id << "}, dimension: "
						<< dimension << std::endl;

				so_5::send_delayed< produce_next >( *this,
						std::chrono::milliseconds( random_value( 0, 100 ) ) );
			}

		void evt_generation_result( const generation_result & evt )
			{
				auto ms =
					[]( const clock_type::time_point a,
						const clock_type::time_point b )
					{
						return std::to_string( std::chrono::duration_cast<
								std::chrono::milliseconds >( b - a ).count() ) + "ms";
					};

				const auto & meta = *evt.m_metadata;

				const auto & d1 = ms( meta.m_generated_at, meta.m_queued_at );
				const auto & d2 = ms( meta.m_queued_at,
						meta.m_processing_started_at );
				const auto & d3 = ms( meta.m_processing_started_at,
						meta.m_processing_finished_at );

				std::cout << "result {" << evt.m_id << "}: "
						<< "in route: " << d1 << ", waiting(p"
						<< so_5::to_size_t( meta.m_queue_prio )
						<< "): " << d2
						<< ", processing(p"
						<< so_5::to_size_t( meta.m_processor_prio )
						<< "): " << d3 
						<< std::endl;
			}

		void evt_generation_rejected( const generation_rejected & evt )
			{
				std::cout << "*** REJECTION: " << evt.m_id << std::endl;
			}

		/*
		 * Generation of new dimension.
		 *
		 * Implements algorithm for the following value distrubution:
		 * 60% of values in range [100,3000)
		 * 30% of values in range [3000, 8000)
		 * 10% of values in range [8000, 10000]
		 */
		unsigned int generate_next_dimension()
			{
				if( 0 == m_x + m_y + m_z )
					{
						// Distribution params must be reinitialized.
						m_x = 60; m_y = 30; m_z = 10;
					}

				auto make_result =
					[]( unsigned int & param, unsigned int l, unsigned int r ) {
							param -= 1;
							return random_value( l, r );
					};

				const auto v = random_value( 0, m_x + m_y + m_z );
				if( v < m_x )
					return make_result( m_x, 100, 2999 );
				else if( v < m_x + m_y )
					return make_result( m_y, 3000, 7999 );
				else
					return make_result( m_z, 8000, 10000 );
			}
	};

//
// Data for requests scheduling.
//
struct request_scheduling_data
	{
		// Type of queue for storing pending requests.
		using request_queue = std::queue< generation_request_shptr >;

		struct priority_data
			{
				// Mbox of request processor for that priority.
				so_5::mbox_t m_processor;

				// List of pending requests for that priority.
				request_queue m_requests;

				// Is processor free or busy.
				bool m_processor_is_free = true;
			};

		// Processors and queues for them.
		priority_data m_processors[ so_5::prio::total_priorities_count ];

		// Helper for access information for the specified priority.
		priority_data &
		info_at( so_5::priority_t p )
			{
				return m_processors[ so_5::to_size_t(p) ];
			}
	};

//
// Messages and signals for interaction between request acceptor and processor.
//

// An information about a possibility to schedule request to
// a free processor.
struct processor_can_be_loaded : public so_5::message_t
	{
		// Priority of the processor.
		so_5::priority_t m_priority;

		processor_can_be_loaded( so_5::priority_t priority )
			:	m_priority( priority )
			{}
	};

// Ask for next request to be processed.
struct ask_for_work : public so_5::message_t
	{
		// Priority of the processor.
		so_5::priority_t m_priority;

		ask_for_work( so_5::priority_t priority )
			:	m_priority( priority )
			{}
	};

// Request acceptor.
// Accepts and stores requests to queue of appropriate priority.
class request_acceptor : public so_5::agent_t
	{
	public :
		request_acceptor(
			context_t ctx,
			so_5::mbox_t interaction_mbox,
			request_scheduling_data & data )
			:	so_5::agent_t( ctx
					// This agent has minimal priority.
					+ so_5::prio::p0
					// If there are to many pending requests then
					// new requests must be rejected.
					+ limit_then_transform( 10,
						[this]( const generation_request & req ) {
							return make_transformed< generation_rejected >(
									m_interaction_mbox,
									req.m_id );
							} ) )
			,	m_interaction_mbox( std::move( interaction_mbox ) )
			,	m_data( data )
			{}

		virtual void so_define_agent() override
			{
				so_subscribe( m_interaction_mbox )
					.event( &request_acceptor::evt_request );
			}

	private :
		const so_5::mbox_t m_interaction_mbox;

		request_scheduling_data & m_data;

		// The event handler has that prototype for ability to
		// store the original message object in request queue.
		void evt_request( mhood_t< generation_request > evt )
			{
				using namespace so_5::prio;

				// Detecting priority for that request.
				auto step = double{ max_dimension + 1 } / total_priorities_count;
				// Requests with lowest dimensions must have highest priority.
				auto pos = so_5::to_size_t( so_5::priority_t::p_max ) -
						static_cast< std::size_t >( evt->m_dimension / step );

				// Store request to the queue.
				auto & info = m_data.m_processors[ pos ];

				// Defense for overloading.
				if( info.m_requests.size() < 100 )
					{
						if( info.m_requests.empty() && info.m_processor_is_free )
							// A work for that processor can be scheduled.
							so_5::send< processor_can_be_loaded >(
									m_interaction_mbox,
									so_5::to_priority_t( pos ) );

						info.m_requests.push( evt.make_reference() );

						// Update request information.
						evt->m_metadata->m_queued_at = clock_type::now();
						evt->m_metadata->m_queue_prio = so_5::to_priority_t( pos );
					}
				else
					// Request cannot be processed.
					so_5::send< generation_rejected >( m_interaction_mbox,
							evt->m_id );
			}
	};

// Request scheduler.
// Creates child coop with processors at the start.
// Processes ask_for_work requests from processors.
class request_scheduler : public so_5::agent_t
	{
	public :
		request_scheduler(
			context_t ctx,
			so_5::mbox_t interaction_mbox,
			request_scheduling_data & data )
			:	so_5::agent_t( ctx
					// This agent must have more high priority than acceptor.
					+ so_5::prio::p1 )
			,	m_interaction_mbox( std::move( interaction_mbox ) )
			,	m_data( data )
			{}

		virtual void so_define_agent() override
			{
				so_subscribe( m_interaction_mbox )
					.event( &request_scheduler::evt_processor_can_be_loaded )
					.event( &request_scheduler::evt_ask_for_work );
			}

		virtual void so_evt_start() override
			{
				// Child cooperation with actual processors must be created.
				// It will use prio_dedicated_threads::one_per_prio dispacther.
				so_5::introduce_child_coop(
					*this,
					so_5::disp::prio_dedicated_threads::one_per_prio::create_private_disp(
							so_environment() )->binder(),
					[this]( so_5::coop_t & coop )
					{
						so_5::prio::for_each_priority( [&]( so_5::priority_t p ) {
								create_processor_agent( coop, p );
							} );
					} );
			}

	private :
		const so_5::mbox_t m_interaction_mbox;

		request_scheduling_data & m_data;

		void evt_processor_can_be_loaded( const processor_can_be_loaded & evt )
			{
				auto & info = m_data.info_at( evt.m_priority );

				// Processor was free when message was sent.
				// But its state could have been changed since then.
				// So we need to check it and do scheduling only if
				// processor is still free.
				if( info.m_processor_is_free )
					try_schedule_work_to( evt.m_priority );
			}

		void evt_ask_for_work( const ask_for_work & evt )
			{
				// Processor must be marked as free.
				m_data.info_at( evt.m_priority ).m_processor_is_free = true;

				try_schedule_work_to( evt.m_priority );
			}

		void create_processor_agent(
			so_5::coop_t & coop,
			so_5::priority_t priority )
			{
				auto a = coop.define_agent( coop.make_agent_context() + priority
						+ so_5::agent_t::limit_then_abort< generation_request >( 1 ) );

				// Mbox of processor must be stored to be used later.
				m_data.info_at( priority ).m_processor = a.direct_mbox();

				// Subscriptions for message.
				a.event( a, [this, priority]( const generation_request & evt ) {
						evt.m_metadata->m_processing_started_at = clock_type::now();
						evt.m_metadata->m_processor_prio = priority;

						// Some processing.
						// Time of processing is proportional to dimension of
						// the image to be generated.
						imitate_hard_work( evt.m_dimension / 10 );

						evt.m_metadata->m_processing_finished_at = clock_type::now();

						so_5::send< generation_result >( m_interaction_mbox,
								evt.m_id,
								evt.m_metadata );

						// Processor is free to get next request for processing.
						so_5::send< ask_for_work >( m_interaction_mbox, priority );
					} );
			}

		void try_schedule_work_to( so_5::priority_t priority )
			{
				auto & free_processor_info = m_data.info_at( priority );

				// Should dive no more than several levels deep;
				const auto max_deep = 5;
				auto deep = 0;
				do
				{
					auto & info = m_data.info_at( priority );
					if( !info.m_requests.empty() )
						{
							// There is a work for processor.
							auto req = info.m_requests.front();
							info.m_requests.pop();

							// Message must be delivered to processor who asks
							// for new work.
							free_processor_info.m_processor->deliver_message( req );
							free_processor_info.m_processor_is_free = false;
							break;
						}
					else
						{
							// There is no more work. Try to stole it from
							// lower priority.
							if( so_5::prio::has_prev( priority ) )
								{
									priority = so_5::prio::prev( priority );
									++deep;
								}
							else
								// There is no more priorities to look.
								break;
						}
				}
				while( deep < max_deep );
			}
	};

void init( so_5::environment_t & env )
	{
		// All top-level agents belong to the same coop,
		// but work on different dispacthers.
		env.introduce_coop( []( so_5::coop_t & coop ) {
				auto mbox = coop.environment().create_mbox();

				// Request scheduler and accepter stuff.

				// A special dispatcher.
				namespace disp_ns = so_5::disp::prio_one_thread::strictly_ordered;
				auto prio_disp = disp_ns::create_private_disp( coop.environment() );

				// Common data for both agents. Will be controlled by the coop.
				auto data = coop.take_under_control(
						new request_scheduling_data{} );

				coop.make_agent_with_binder< request_scheduler >(
						prio_disp->binder(), mbox, *data );
				coop.make_agent_with_binder< request_acceptor >(
						prio_disp->binder(), mbox, *data );

				// Requests generator.
				coop.make_agent< request_generator >( mbox );
			} );
	}

int main()
	{
		try
			{
				so_5::launch( init );

				return 0;
			}
		catch( const std::exception & x )
			{
				std::cerr << "Exception: " << x.what() << std::endl;
			}

		return 2;
	}

