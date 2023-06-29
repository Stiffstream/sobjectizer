/*
 * A test for custom_direct_mbox_factory.
 */

#include <iostream>
#include <sstream>

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

#include <test/3rd_party/utest_helper/helper.hpp>

struct msg_check : public so_5::signal_t {};

class protocol_t
	{
		std::mutex m_lock;
		std::string m_protocol;

	public:
		protocol_t() = default;

		protocol_t( const protocol_t & ) = delete;
		protocol_t( protocol_t && ) = delete;

		void
		append( const std::string & what )
			{
				std::lock_guard< std::mutex > lock{ m_lock };
				m_protocol += what;
			}

		[[nodiscard]]
		std::string
		get()
			{
				std::lock_guard< std::mutex > lock{ m_lock };
				return m_protocol;
			}
	};

class test_mbox_t final : public so_5::abstract_message_box_t
	{
		const so_5::mbox_t m_target;
		protocol_t & m_protocol;

	public:
		test_mbox_t(
			so_5::mbox_t target,
			protocol_t & protocol )
			:	m_target{ std::move(target) }
			,	m_protocol{ protocol }
		{}

		so_5::mbox_id_t
		id() const override
			{
				return m_target->id();
			}

		void
		subscribe_event_handler(
			const std::type_index & type_index,
			::so_5::abstract_message_sink_t & subscriber ) override
			{
				m_protocol.append( "subscribe;" );
				m_target->subscribe_event_handler(
						type_index,
						subscriber );
			}

		void
		unsubscribe_event_handler(
			const std::type_index & type_index,
			::so_5::abstract_message_sink_t & subscriber ) noexcept override
			{
				m_target->unsubscribe_event_handler(
						type_index,
						subscriber );
				m_protocol.append( "unsubscribe;" );
			}

		std::string
		query_name() const override
			{
				return m_target->query_name();
			}

		so_5::mbox_type_t
		type() const override
			{
				return m_target->type();
			}

		void
		do_deliver_message(
			so_5::message_delivery_mode_t delivery_mode,
			const std::type_index & msg_type,
			const ::so_5::message_ref_t & message,
			unsigned int redirection_deep ) override
			{
				m_protocol.append( "deliver;" );
				m_target->do_deliver_message(
						delivery_mode,
						msg_type,
						message,
						redirection_deep );
			}

		void
		set_delivery_filter(
			const std::type_index & msg_type,
			const ::so_5::delivery_filter_t & filter,
			::so_5::abstract_message_sink_t & subscriber ) override
			{
				m_protocol.append( "set_delivery_filter;" );
				m_target->set_delivery_filter(
						msg_type,
						filter,
						subscriber );
			}

		void
		drop_delivery_filter(
			const std::type_index & msg_type,
			::so_5::abstract_message_sink_t & subscriber ) noexcept override
			{
				m_target->drop_delivery_filter(
						msg_type,
						subscriber );
				m_protocol.append( "drop_delivery_filter;" );
			}

		so_5::environment_t &
		environment() const noexcept override
			{
				return m_target->environment();
			}
	};

class a_test_t : public so_5::agent_t
	{
		typedef so_5::agent_t base_type_t;

	public :
		a_test_t(
			context_t ctx,
			protocol_t & protocol )
			:	base_type_t( ctx +
					custom_direct_mbox_factory(
						[&protocol](
							so_5::partially_constructed_agent_ptr_t,
							so_5::mbox_t actual_mbox ) -> so_5::mbox_t
						{
							return {
									std::make_unique< test_mbox_t >(
											std::move(actual_mbox), protocol )
								};
						}
					)
				)
			,	m_protocol( protocol )
			{
			}

		void
		so_define_agent() override
			{
				so_subscribe( so_direct_mbox() )
					.event( &a_test_t::evt_check );
			}

		void
		so_evt_start() override
			{
				so_5::send< msg_check >( *this );
			}

		void
		evt_check( mhood_t< msg_check > )
			{
				m_protocol.append( "received;" );

				so_environment().stop();
			}

	private :
		protocol_t & m_protocol;
	};

UT_UNIT_TEST( simple )
{
	protocol_t protocol;

	run_with_time_limit(
		[&protocol]()
		{
			so_5::launch(
				[&protocol]( so_5::environment_t & env )
				{
					env.register_agent_as_coop(
							env.make_agent< a_test_t >( std::ref(protocol) ) );
				} );
		},
		5 );

	UT_CHECK_EQ( protocol.get(), "subscribe;deliver;received;unsubscribe;" );
}

int
main()
	{
		UT_RUN_UNIT_TEST( simple )

		return 0;
	}

