/*
 * Test for checking redirection deep for bind_transformer.
 */

#include <iostream>
#include <sstream>

#include <so_5/bind_transformer_helpers.hpp>
#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

namespace test {

class test_envelope_t final : public so_5::enveloped_msg::envelope_t
	{
		std::mutex m_lock;

		so_5::outliving_reference_t< std::string > m_receiver;
		const std::string m_id;

		so_5::message_ref_t m_payload;

		void
		append_text( const char * what )
			{
				std::lock_guard< std::mutex > lock{ m_lock };
				m_receiver.get() += m_id;
				m_receiver.get() += ":";
				m_receiver.get() += what;
			}

	public:
		test_envelope_t(
			so_5::outliving_reference_t< std::string > receiver,
			std::string id,
			so_5::message_ref_t payload )
			:	m_receiver{ receiver }
			,	m_id( std::move(id) )
			,	m_payload{ std::move(payload) }
			{}

		void
		access_hook(
			access_context_t context,
			handler_invoker_t & invoker ) noexcept override
			{
				switch( context )
					{
					case access_context_t::handler_found:
						append_text( "pre_invoke;" );
						invoker.invoke( so_5::enveloped_msg::payload_info_t{ m_payload } );
						append_text( "post_invoke;" );
					break;

					case access_context_t::transformation:
						append_text( "transform;" );
						invoker.invoke( payload_info_t{ m_payload } );
					break;

					case access_context_t::inspection:
						append_text( "inspect;" );
						invoker.invoke( payload_info_t{ m_payload } );
					break;
					}
			}
	};

template< typename Msg, typename... Args >
void
post_enveloped(
	std::string & receiver,
	std::string id,
	const so_5::mbox_t & mbox,
	Args && ...args )
	{
		so_5::message_ref_t msg{
				std::make_unique<Msg>( std::forward<Args>(args)... ) };
		so_5::message_ref_t enveloped{
				std::make_unique<test_envelope_t>(
						so_5::outliving_mutable(receiver),
						std::move(id),
						std::move(msg) ) };

		mbox->do_deliver_message(
				so_5::message_delivery_mode_t::ordinary,
				so_5::message_payload_type<Msg>::subscription_type_index(),
				std::move(enveloped),
				1 );
	}

struct msg_source final : public so_5::message_t
	{
		std::string m_v;

		explicit msg_source( std::string v ) : m_v{ std::move(v) }
			{}
	};

struct msg_result final : public so_5::message_t
	{
		std::string m_v;

		explicit msg_result( std::string v ) : m_v{ std::move(v) }
			{}
	};

void
do_test( so_5::environment_t & env )
	{
		auto src = env.create_mbox();
		auto dest = so_5::create_mchain( env );

		so_5::single_sink_binding_t binding;
		so_5::bind_transformer< msg_source >(
				binding,
				src,
				[dest]( const auto & msg ) {
					return so_5::make_transformed< msg_result >(
							dest->as_mbox(),
							"<" + msg.m_v + ">" );
				} );

		std::string log;
		post_enveloped< msg_source >(
				log,
				"1",
				src,
				"a" );

		so_5::receive( so_5::from( dest ).handle_n( 1 ),
				[&log]( const msg_result & msg ) {
					log += msg.m_v;
					log += ";";
				} );

		const std::string expected{ "1:transform;<a>;" };
		ensure_or_die( log == expected,
				"log: '" + log + "', expected: '" + expected + "'" );
	}

} /* namespace test */

int
main()
	{
		run_with_time_limit(
			[]()
			{
				so_5::launch(
					[]( so_5::environment_t & env )
					{
						test::do_test( env );
					} );
			},
			5 );

		return 0;
	}

