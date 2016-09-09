/*
 * An example for imitation of news board which handles requests from
 * different types of clients: news-writers and news-readers.
 */

#if defined( _MSC_VER )
	#if defined( __clang__ )
		#pragma clang diagnostic ignored "-Wreserved-id-macro"
	#endif
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include <algorithm>
#include <chrono>
#include <ctime>
#include <iostream>
#include <iterator>
#include <list>
#include <random>
#include <tuple>

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
void imitate_hard_work()
	{
		std::this_thread::sleep_for(
				std::chrono::milliseconds{ random_value( 25, 125 ) } );
	}

// Type of clock to work with time values.
using clock_type = std::chrono::system_clock;

// A helper function to calculate difference between to time points.
// The result is converted to string.
std::string ms_from_time( const clock_type::time_point & previous_point )
	{
		using namespace std::chrono;

		const auto t = clock_type::now();
		if( t > previous_point )
			return std::to_string( duration_cast< milliseconds >(
					clock_type::now() - previous_point ).count() ) + "ms";
		else
			return "0ms";
	}

// A message for logging something.
struct msg_log
	{
		std::string m_who;
		std::string m_what;
	};

// A helper for logging simplification.
void log(
	const so_5::mbox_t & logger_mbox,
	std::string who,
	std::string what )
	{
		so_5::send< msg_log >( logger_mbox, std::move(who), std::move(what) );
	}

// Builder of logger agent.
so_5::mbox_t create_logger_coop( so_5::environment_t & env )
	{
		so_5::mbox_t result;

		env.introduce_coop( [&]( so_5::coop_t & coop )
			{
				// Logger agent.
				auto a = coop.define_agent();
				// Reacts to just one message.
				a.event( a, []( const msg_log & evt ) {
					// String representation for date/time.
					char local_time_sz[ 32 ];
					auto t = clock_type::to_time_t( clock_type::now() );
					std::strftime( local_time_sz, sizeof local_time_sz,
							"%Y.%m.%d %H:%M:%S", std::localtime( &t ) );

					// Simplest form of logging.
					std::cout << "[" << local_time_sz << "] {" << evt.m_who
							<< "}: " << evt.m_what << std::endl;
				} );

				// Direct mbox of logger agent will be returned.
				result = a.direct_mbox();
			} );

		return result;
	}

//
// Messages for interaction with news board.
//

// Type of story ID.
using story_id_type = unsigned long;

// Base class for all messages. It stores timestamp.
struct news_board_message_base : public so_5::message_t
	{
		// Time at which an operation was started.
		// This time will be used for calculation of operation duration.
		const clock_type::time_point m_timestamp;

		news_board_message_base( clock_type::time_point timestamp )
			:	m_timestamp( std::move(timestamp) )
			{}
	};

// Base class for all request messages. It stores reply_to value.
struct news_board_request_base : public news_board_message_base
	{
		// Mbox of request initiator.
		// Response will be sent to that mbox.
		const so_5::mbox_t m_reply_to;

		news_board_request_base(
			clock_type::time_point timestamp,
			so_5::mbox_t reply_to )
			:	news_board_message_base( std::move(timestamp) )
			,	m_reply_to( std::move(reply_to) )
			{}
	};

// Request for publishing new story.
struct msg_publish_story_req : public news_board_request_base
	{
		const std::string m_title;
		const std::string m_content;

		msg_publish_story_req(
			clock_type::time_point timestamp,
			so_5::mbox_t reply_to,
			std::string title,
			std::string content )
			:	news_board_request_base(
					std::move(timestamp), std::move(reply_to) )
			,	m_title( std::move(title) )
			,	m_content( std::move(content) )
			{}
	};

// Reply for publishing new story.
struct msg_publish_story_resp : public news_board_message_base
	{
		story_id_type m_id;

		msg_publish_story_resp(
			clock_type::time_point timestamp,
			story_id_type id )
			:	news_board_message_base( std::move(timestamp) )
			,	m_id( id )
			{}
	};

// Request for updates from news board.
struct msg_updates_req : public news_board_request_base
	{
		// Last known story ID.
		story_id_type m_last_id;

		msg_updates_req(
			clock_type::time_point timestamp,
			so_5::mbox_t reply_to,
			story_id_type last_id )
			:	news_board_request_base(
					std::move(timestamp), std::move(reply_to) )
			,	m_last_id( last_id )
			{}

	};

// Reply for request for updates.
struct msg_updates_resp : public news_board_message_base
	{
		// Type of new stories list.
		using story_list = std::list< std::tuple< story_id_type, std::string > >;

		// List of short info about new storis.
		const story_list m_updates;

		msg_updates_resp(
			clock_type::time_point timestamp,
			story_list updates )
			:	news_board_message_base( std::move(timestamp) )
			,	m_updates( std::move(updates) )
			{}
	};

// Request for content of the story.
struct msg_story_content_req : public news_board_request_base
	{
		// Story ID.
		const story_id_type m_id;

		msg_story_content_req(
			clock_type::time_point timestamp,
			so_5::mbox_t reply_to,
			story_id_type id )
			:	news_board_request_base(
					std::move(timestamp), std::move(reply_to) )
			,	m_id( id )
			{}
	};

// Positive response to a request for story content.
struct msg_story_content_resp_ack : public news_board_message_base
	{
		// Story content.
		const std::string m_content;

		msg_story_content_resp_ack(
			clock_type::time_point timestamp,
			std::string content )
			:	news_board_message_base( std::move(timestamp) )
			,	m_content( std::move(content) )
			{}
	};

// Negative response to a request for story content.
// This message is used when story was removed from the board.
struct msg_story_content_resp_nack : public news_board_message_base
	{
		msg_story_content_resp_nack(
			clock_type::time_point timestamp )
			:	news_board_message_base( std::move(timestamp) )
			{}
	};

//
// News board data.
//

struct news_board_data
	{
		// Information about one story.
		struct story_info
			{
				std::string m_title;
				std::string m_content;
			};

		// Type of map from story ID to story data.
		using story_map = std::map< story_id_type, story_info >;

		// Published stories.
		story_map m_stories;

		// ID counter.
		story_id_type m_last_id = 0;
	};

//
// Agents to work with news board data.
//

// Agent for receiving and storing new stories to news board.
void define_news_receiver_agent(
	so_5::coop_t & coop,
	news_board_data & board_data,
	const so_5::mbox_t & board_mbox,
	const so_5::mbox_t & logger_mbox )
	{
		coop.define_agent(
				// This agent should have lowest priority among
				// board-related agents.
				coop.make_agent_context() + so_5::prio::p1 )
			// It handles just one message.
			.event( board_mbox,
				[&board_data, logger_mbox]( const msg_publish_story_req & evt )
				{
					// Store new story to board.
					auto story_id = ++(board_data.m_last_id);
					board_data.m_stories.emplace( story_id,
							news_board_data::story_info{ evt.m_title, evt.m_content } );

					// Log this fact.
					log( logger_mbox,
							"board.receiver",
							"new story published, id=" + std::to_string( story_id ) +
							", title=" + evt.m_title );

					// Take some time for processing.
					imitate_hard_work();

					// Send reply to story-sender.
					so_5::send< msg_publish_story_resp >(
							evt.m_reply_to,
							evt.m_timestamp,
							story_id );

					// Remove oldest story if there are too much stories.
					if( 40 < board_data.m_stories.size() )
						{
							auto removed_id = board_data.m_stories.begin()->first;
							board_data.m_stories.erase( board_data.m_stories.begin() );
							log( logger_mbox,
									"board.receiver",
									"old story removed, id=" + std::to_string( removed_id ) );
						}
				} );
	}

// Agent for handling requests about updates on news board.
void define_news_directory_agent(
	so_5::coop_t & coop,
	news_board_data & board_data,
	const so_5::mbox_t & board_mbox,
	const so_5::mbox_t & logger_mbox )
	{
		coop.define_agent(
				// This agent should have priority higher than news_receiver.
				coop.make_agent_context() + so_5::prio::p2 )
			// It handles just one message.
			.event( board_mbox,
				[&board_data, logger_mbox]( const msg_updates_req & req )
				{
					log( logger_mbox,
							"board.directory",
							"request for updates received, last_id=" +
								std::to_string( req.m_last_id ) );

					// Take some time for processing.
					imitate_hard_work();

					// Searching for new stories for that request
					// and building result list.
					msg_updates_resp::story_list new_stories;
					std::transform(
							board_data.m_stories.upper_bound( req.m_last_id ),
							std::end( board_data.m_stories ),
							std::back_inserter( new_stories ),
							[]( const news_board_data::story_map::value_type & v ) {
								return std::make_tuple( v.first, v.second.m_title );
							} );

					log( logger_mbox,
							"board.directory",
							std::to_string( new_stories.size() ) + " new stories found" );

					// Sending response.
					so_5::send< msg_updates_resp >(
							req.m_reply_to,
							req.m_timestamp,
							std::move(new_stories) );
				} );
	}

// Agent for handling requests for story content.
void define_story_extractor_agent(
	so_5::coop_t & coop,
	news_board_data & board_data,
	const so_5::mbox_t & board_mbox,
	const so_5::mbox_t & logger_mbox )
	{
		coop.define_agent(
				// This agent should have priority higher that news_directory.
				coop.make_agent_context() + so_5::prio::p3 )
			// It handles just one message.
			.event( board_mbox,
				[&board_data, logger_mbox]( const msg_story_content_req & req )
				{
					log( logger_mbox,
							"board.extractor",
							"request for story content received, id=" +
								std::to_string( req.m_id ) );

					// Take some time for processing.
					imitate_hard_work();

					auto it = board_data.m_stories.find( req.m_id );
					if( it != board_data.m_stories.end() )
						{
							log( logger_mbox,
									"board.extractor",
									"story {" + std::to_string( req.m_id ) + "} found" );

							so_5::send< msg_story_content_resp_ack >(
									req.m_reply_to,
									req.m_timestamp,
									it->second.m_content );
						}
					else
						{
							log( logger_mbox,
									"board.extractor",
									"story {" + std::to_string( req.m_id ) + "} NOT found" );

							so_5::send< msg_story_content_resp_nack >(
									req.m_reply_to,
									req.m_timestamp );
						}
				} );
	}


so_5::mbox_t create_board_coop(
	so_5::environment_t & env,
	const so_5::mbox_t & logger_mbox )
	{
		auto board_mbox = env.create_mbox();

		using namespace so_5::disp::prio_one_thread::quoted_round_robin;
		using namespace so_5::prio;

		// Board cooperation will use quoted_round_robin dispatcher
		// with different quotes for agents.
		env.introduce_coop(
			create_private_disp( env,
				quotes_t{ 1 }
					.set( p1, 10 ) // 10 events for news_receiver.
					.set( p2, 20 ) // 20 events for news_directory.
					.set( p3, 30 ) // 30 events for story_extractor.
				)->binder(),
			[&]( so_5::coop_t & coop )
			{
				// Lifetime of news board data will be controlled by cooperation.
				auto board_data = coop.take_under_control( new news_board_data() );

				define_news_receiver_agent(
						coop, *board_data, board_mbox, logger_mbox );
				define_news_directory_agent(
						coop, *board_data, board_mbox, logger_mbox );
				define_story_extractor_agent(
						coop, *board_data, board_mbox, logger_mbox );
			} );

		return board_mbox;
	}

//
// Story publishers.
//

class story_publisher : public so_5::agent_t
	{
		struct msg_time_for_new_story : public so_5::signal_t {};

	public :
		story_publisher(
			context_t ctx,
			std::string publisher_name,
			so_5::mbox_t board_mbox,
			so_5::mbox_t logger_mbox )
			:	so_5::agent_t( ctx )
			,	m_name( std::move(publisher_name) )
			,	m_board_mbox( std::move(board_mbox) )
			,	m_logger_mbox( std::move(logger_mbox) )
			{}

		virtual void so_define_agent() override
			{
				this >>= st_await_new_story;

				st_await_new_story.event< msg_time_for_new_story >(
						&story_publisher::evt_time_for_new_story );

				st_await_publish_response.event(
						&story_publisher::evt_publish_response );
			}

		virtual void so_evt_start() override
			{
				initiate_time_for_new_story_signal();
			}

	private :
		// The agent will wait 'msg_time_for_new_story' signal in this state.
		const state_t st_await_new_story{ this };
		// The agent will wait a response to publising request in this state.
		const state_t st_await_publish_response{ this };

		const std::string m_name;

		const so_5::mbox_t m_board_mbox;
		const so_5::mbox_t m_logger_mbox;

		// This counter will be used in store generation procedure.
		unsigned int m_stories_counter = 0;

		void initiate_time_for_new_story_signal()
			{
				so_5::send_delayed< msg_time_for_new_story >(
						*this,
						std::chrono::milliseconds{ random_value( 100, 1500 ) } );
			}

		void evt_time_for_new_story()
			{
				// Create new story.
				auto story_number = ++m_stories_counter;
				std::string title = "A story from " + m_name + " #" +
						std::to_string( story_number );
				std::string content = "This is a content from a story '" +
						title + "' provided by " + m_name;

				log( m_logger_mbox, m_name, "Publish new story: " + title );

				// Publishing the story.
				so_5::send< msg_publish_story_req >( m_board_mbox,
						clock_type::now(),
						so_direct_mbox(),
						std::move(title),
						std::move(content) );

				// Waiting a response.
				this >>= st_await_publish_response;
			}

		void evt_publish_response( const msg_publish_story_resp & resp )
			{
				log( m_logger_mbox, m_name, "Publish finished, id=" +
						std::to_string( resp.m_id ) + ", publish took " +
						ms_from_time( resp.m_timestamp ) );

				// Waiting for a time for next story.
				this >>= st_await_new_story;
				initiate_time_for_new_story_signal();
			}
	};

void create_publisher_coop(
	so_5::environment_t & env,
	const so_5::mbox_t & board_mbox,
	const so_5::mbox_t & logger_mbox )
	{
		// All publishers will work on the same working thread.
		env.introduce_coop(
			so_5::disp::one_thread::create_private_disp( env )->binder(),
			[&]( so_5::coop_t & coop )
			{
				for( int i = 0; i != 5; ++i )
					coop.make_agent< story_publisher >(
							"publisher" + std::to_string(i+1),
							board_mbox,
							logger_mbox );
			} );
	}

//
// News readers.
//

class news_reader : public so_5::agent_t
	{
		struct msg_time_for_updates : public so_5::signal_t {};

	public :
		news_reader(
			context_t ctx,
			std::string reader_name,
			so_5::mbox_t board_mbox,
			so_5::mbox_t logger_mbox )
			:	so_5::agent_t( ctx )
			,	m_name( std::move(reader_name) )
			,	m_board_mbox( std::move(board_mbox) )
			,	m_logger_mbox( std::move(logger_mbox) )
			{}

		virtual void so_define_agent() override
			{
				this >>= st_sleeping;

				st_sleeping.event< msg_time_for_updates >(
						&news_reader::evt_time_for_updates );

				st_await_updates.event(
						&news_reader::evt_updates_received );

				st_await_story_content.event(
						&news_reader::evt_story_content );
				st_await_story_content.event(
						&news_reader::evt_story_not_found );
			}

		virtual void so_evt_start() override
			{
				initiate_time_for_updates_signal();
			}

	private :
		// The agent will wait 'msg_time_for_updates' signal in this state.
		const state_t st_sleeping{ this };
		// The agent will wait updates from news board in this state. 
		const state_t st_await_updates{ this };
		// The agent will wait story content in this state.
		const state_t st_await_story_content{ this };

		const std::string m_name;

		const so_5::mbox_t m_board_mbox;
		const so_5::mbox_t m_logger_mbox;

		// ID of last received story from news board.
		story_id_type m_last_id = 0;

		// List a stories to be requested from news board.
		msg_updates_resp::story_list m_stories_to_read;

		void initiate_time_for_updates_signal()
			{
				so_5::send_delayed< msg_time_for_updates >(
						*this,
						std::chrono::milliseconds{ random_value( 500, 2500 ) } );
			}

		void evt_time_for_updates()
			{
				request_updates();
			}

		void evt_updates_received( const msg_updates_resp & resp )
			{
				log( m_logger_mbox,
						m_name,
						std::to_string( resp.m_updates.size() ) +
							" updates received, took " +
							ms_from_time( resp.m_timestamp ) );

				if( resp.m_updates.empty() )
					{
						// Nothing new. We should sleep.
						this >>= st_sleeping;
						initiate_time_for_updates_signal();
					}
				else
					{
						this >>= st_await_story_content;
						// Read no more than 3 latest stories.
						unsigned int c = 0;
						for( auto it = resp.m_updates.rbegin();
								it != resp.m_updates.rend() && c != 3; ++it, ++c )
							m_stories_to_read.push_front( *it );

						request_story_content();
					}
			}

		void evt_story_content( const msg_story_content_resp_ack & resp )
			{
				const auto & id = std::get<0>( *std::begin(m_stories_to_read) );
				const auto & title = std::get<1>( *std::begin(m_stories_to_read) );

				log( m_logger_mbox,
						m_name,
						"read story {" + std::to_string( id ) + "} '" +
							title + "': \"" + resp.m_content + "\", took " +
							ms_from_time( resp.m_timestamp ) );

				remove_current_story_and_read_next();
			}

		void evt_story_not_found( const msg_story_content_resp_nack & resp )
			{
				const auto & id = std::get<0>( *std::begin(m_stories_to_read) );
				const auto & title = std::get<1>( *std::begin(m_stories_to_read) );

				log( m_logger_mbox,
						m_name,
						"unable to read story {" + std::to_string( id ) + "} '" +
							title + "', took " + ms_from_time( resp.m_timestamp ) );

				remove_current_story_and_read_next();
			}

		void request_updates()
			{
				log( m_logger_mbox,
						m_name,
						"requesting updates, last_id=" + std::to_string( m_last_id ) );
				so_5::send< msg_updates_req >( m_board_mbox,
						clock_type::now(),
						so_direct_mbox(),
						m_last_id );

				this >>= st_await_updates;
			}

		void request_story_content()
			{
				auto id = std::get<0>( *std::begin(m_stories_to_read) );

				log( m_logger_mbox,
						m_name,
						"requesting story {" + std::to_string( id ) + "}" );

				so_5::send< msg_story_content_req >(
						m_board_mbox,
						clock_type::now(),
						so_direct_mbox(),
						id );
			}

		void remove_current_story_and_read_next()
			{
				m_last_id = std::get<0>( *std::begin(m_stories_to_read) );
				m_stories_to_read.pop_front();

				if( !m_stories_to_read.empty() )
					request_story_content();
				else
					request_updates();
			}
	};

void create_reader_coop(
	so_5::environment_t & env,
	const so_5::mbox_t & board_mbox,
	const so_5::mbox_t & logger_mbox )
	{
		// All readers will work on the same working thread.
		env.introduce_coop(
			so_5::disp::one_thread::create_private_disp( env )->binder(),
			[&]( so_5::coop_t & coop )
			{
				for( int i = 0; i != 50; ++i )
					coop.make_agent< news_reader >(
							"reader" + std::to_string(i+1),
							board_mbox,
							logger_mbox );
			} );
	}


void init( so_5::environment_t & env )
	{
		auto logger_mbox = create_logger_coop( env );
		auto board_mbox = create_board_coop( env, logger_mbox );

		create_publisher_coop( env, board_mbox, logger_mbox );
		create_reader_coop( env, board_mbox, logger_mbox );
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

