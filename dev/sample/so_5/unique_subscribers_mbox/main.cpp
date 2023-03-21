/*
 * Simple usage of unique_subscribers mbox.
 */

#include <so_5/all.hpp>

#include <iostream>
#include <memory>

// Data type to be passed between agents.
struct data_t
{
	std::string m_stage{ "initial" };
	std::string m_prefix;
	std::string m_payload;
	std::string m_suffix;
};

std::ostream & operator<<( std::ostream & to, const data_t & what )
{
	return (to << "(<" << what.m_stage << ">:[" << what.m_prefix
			<< "]= '" << what.m_payload << "' =[" << what.m_suffix << "])");
}

// Messages to be used for interaction between agents.
// Because all messages are similar we use class templates for them.
struct preprocess_tag {};
struct process_tag {};
struct postprocess_tag {};

template< typename Tag >
struct msg_handle_data final : public so_5::message_t
{
	std::unique_ptr< data_t > m_data;
	const so_5::mbox_t m_reply_to;

	msg_handle_data(
		std::unique_ptr< data_t > data,
		so_5::mbox_t reply_to )
		:	m_data{ std::move(data) }
		,	m_reply_to{ std::move(reply_to) }
	{}
};

template< typename Tag >
struct msg_handling_finished final : public so_5::message_t
{
	std::unique_ptr< data_t > m_data;

	explicit msg_handling_finished(
		std::unique_ptr< data_t > data )
		:	m_data{ std::move(data) }
	{}
};

using msg_preprocess_data = msg_handle_data< preprocess_tag >;
using msg_preprocess_finished = msg_handling_finished< preprocess_tag >;

using msg_process_data = msg_handle_data< process_tag >;
using msg_process_finished = msg_handling_finished< process_tag >;

using msg_postprocess_data = msg_handle_data< postprocess_tag >;
using msg_postprocess_finished = msg_handling_finished< postprocess_tag >;

// Type of agent for coordination of data processing.
class processing_manager_t final : public so_5::agent_t
{
	const so_5::mbox_t m_processing_mbox;

public:
	processing_manager_t( context_t ctx, so_5::mbox_t processing_mbox )
		:	so_5::agent_t{ std::move(ctx) }
		,	m_processing_mbox{ std::move(processing_mbox) }
	{}

	void so_define_agent() override
	{
		so_subscribe_self()
			.event( &processing_manager_t::evt_preprocess_finished )
			.event( &processing_manager_t::evt_process_finished )
			.event( &processing_manager_t::evt_postprocess_finished )
			;
	}

	void so_evt_start() override
	{
		// Data to be processed by several workers.
		auto data = std::make_unique< data_t >();
		data->m_payload = "Hello, World";

		std::cout << "data to be processed: " << *data << std::endl;

		// Initiate processing.
		so_5::send< so_5::mutable_msg<msg_preprocess_data> >(
				m_processing_mbox,
				std::move(data),
				so_direct_mbox() );
	}

private:
	void evt_preprocess_finished(
		mutable_mhood_t< msg_preprocess_finished > cmd )
	{
		std::cout << "preprocessed data: " << *(cmd->m_data) << std::endl;

		// Initiate the next stage.
		so_5::send< so_5::mutable_msg<msg_process_data> >(
				m_processing_mbox,
				std::move(cmd->m_data),
				so_direct_mbox() );
	}

	void evt_process_finished(
		mutable_mhood_t< msg_process_finished > cmd )
	{
		std::cout << "processed data: " << *(cmd->m_data) << std::endl;

		// Initiate the next stage.
		so_5::send< so_5::mutable_msg<msg_postprocess_data> >(
				m_processing_mbox,
				std::move(cmd->m_data),
				so_direct_mbox() );
	}

	void evt_postprocess_finished(
		mutable_mhood_t< msg_postprocess_finished > cmd )
	{
		std::cout << "postprocessed data: " << *(cmd->m_data) << std::endl;

		// Finish the example.
		so_deregister_agent_coop_normally();
	}
};

// Type of agent for preprocessing of the data.
class preprocessor_t final : public so_5::agent_t
{
	const so_5::mbox_t m_processing_mbox;

public:
	preprocessor_t( context_t ctx, so_5::mbox_t processing_mbox )
		:	so_5::agent_t{ std::move(ctx) }
		,	m_processing_mbox{ std::move(processing_mbox) }
	{}

	void so_define_agent() override
	{
		so_subscribe( m_processing_mbox )
			.event( []( mutable_mhood_t<msg_preprocess_data> cmd ) {
					cmd->m_data->m_stage = "preprocessed";
					cmd->m_data->m_prefix = "-=#";
					cmd->m_data->m_suffix = "#=-";

					so_5::send< so_5::mutable_msg<msg_preprocess_finished> >(
							cmd->m_reply_to,
							std::move(cmd->m_data) );
				} );
	}
};

// Type of agent for processing of the data.
class processor_t final : public so_5::agent_t
{
	const so_5::mbox_t m_processing_mbox;

public:
	processor_t( context_t ctx, so_5::mbox_t processing_mbox )
		:	so_5::agent_t{ std::move(ctx) }
		,	m_processing_mbox{ std::move(processing_mbox) }
	{}

	void so_define_agent() override
	{
		so_subscribe( m_processing_mbox )
			.event( []( mutable_mhood_t<msg_process_data> cmd ) {
					cmd->m_data->m_stage = "processed";
					// Reverse the content of the data.
					cmd->m_data->m_payload = std::string{
							cmd->m_data->m_payload.rbegin(),
							cmd->m_data->m_payload.rend()
						};

					so_5::send< so_5::mutable_msg<msg_process_finished> >(
							cmd->m_reply_to,
							std::move(cmd->m_data) );
				} );
	}
};

// Type of agent for processing of the data.
class postprocessor_t final : public so_5::agent_t
{
	const so_5::mbox_t m_processing_mbox;

public:
	postprocessor_t( context_t ctx, so_5::mbox_t processing_mbox )
		:	so_5::agent_t{ std::move(ctx) }
		,	m_processing_mbox{ std::move(processing_mbox) }
	{}

	void so_define_agent() override
	{
		so_subscribe( m_processing_mbox )
			.event( []( mutable_mhood_t<msg_postprocess_data> cmd ) {
					cmd->m_data->m_stage = "postprocessed";
					// Add prefix and suffix to the payload.
					cmd->m_data->m_payload = cmd->m_data->m_prefix + " " +
							cmd->m_data->m_payload + " " + cmd->m_data->m_suffix;

					so_5::send< so_5::mutable_msg<msg_postprocess_finished> >(
							cmd->m_reply_to,
							std::move(cmd->m_data) );
				} );
	}
};

int main()
{
	try
	{
		so_5::launch( []( so_5::environment_t & env ) {
				env.introduce_coop( []( so_5::coop_t & coop ) {
						// Mbox to be used for exchanging the data.
						auto processing_mbox =
								so_5::make_unique_subscribers_mbox( coop.environment() );

						// Fill the coop with manager and workers.
						coop.make_agent< processing_manager_t >( processing_mbox );
						coop.make_agent< preprocessor_t >( processing_mbox );
						coop.make_agent< processor_t >( processing_mbox );
						coop.make_agent< postprocessor_t >( processing_mbox );
					} );
			} );

		return 0;
	}
	catch( const std::exception & x )
	{
		std::cerr << "Exception caught: " << x.what() << std::endl;
	}

	return 2;
}

