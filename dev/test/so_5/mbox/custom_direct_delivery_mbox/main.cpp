/*
 * A test for custom_direct_mbox_factory and agent_t::call_push_demand.
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

namespace test
{

class a_test_t : public so_5::agent_t
	{
		typedef so_5::agent_t base_type_t;

		static void
		direct_delivery_demand_handler(
			so_5::current_thread_id_t /*working_thread_id*/,
			so_5::execution_demand_t & d )
			{
				auto * self = static_cast<a_test_t *>(d.m_receiver);
				self->on_message(
						d.m_msg_type,
						d.m_message_ref );
			}

		class special_direct_mbox_t final
			: public so_5::abstract_message_box_t
			{
				a_test_t * m_target;

				const so_5::mbox_id_t m_id;

			public :
				special_direct_mbox_t(
					a_test_t * target,
					so_5::mbox_id_t id )
					: m_target{ target }
					, m_id{ id }
					{}

				so_5::mbox_id_t
				id() const override { return m_id; }

				void
				subscribe_event_handler(
					const std::type_index & /*type_index*/,
					so_5::abstract_message_sink_t & /*subscriber*/ ) override
					{ /* Nothing to do */ }

				void
				unsubscribe_event_handler(
					const std::type_index & /*type_index*/,
					so_5::abstract_message_sink_t & /*subscriber*/ ) noexcept
					{ /* Nothing to do */ }

				std::string
				query_name() const override { return "<special_direct_mbox>"; }

				so_5::mbox_type_t
				type() const override
					{
						return so_5::mbox_type_t::multi_producer_single_consumer;
					}

				void
				do_deliver_message(
					so_5::message_delivery_mode_t /*delivery_mode*/,
					const std::type_index & msg_type,
					const so_5::message_ref_t & message,
					unsigned int /*redirection_deep*/ ) override
					{
						so_5::agent_t::call_push_demand(
								*m_target,
								nullptr,
								m_id,
								msg_type,
								message,
								&a_test_t::direct_delivery_demand_handler
							);
					}

				void
				set_delivery_filter(
					const std::type_index & /*msg_type*/,
					const so_5::delivery_filter_t & /*filter*/,
					so_5::abstract_message_sink_t & /*subscriber*/ ) override
					{
						throw std::runtime_error{
								"set_delivery_filter is not supported"
							};
					}

				void
				drop_delivery_filter(
					const std::type_index & /*msg_type*/,
					so_5::abstract_message_sink_t & /*subscriber*/ ) noexcept override
					{ /* Nothing to do */ }

				so_5::environment_t &
				environment() const noexcept override
					{
						return m_target->so_environment();
					}
			};
	public :
		struct msg_first final : public so_5::signal_t {};
		struct msg_second final : public so_5::signal_t {};
		struct msg_third final : public so_5::signal_t {};

		a_test_t( context_t ctx )
			:	base_type_t( ctx +
					custom_direct_mbox_factory(
						[](
							so_5::partially_constructed_agent_ptr_t agent_ptr,
							so_5::mbox_t actual_mbox ) -> so_5::mbox_t
						{
							return {
									std::make_unique< special_direct_mbox_t >(
											static_cast<a_test_t *>(agent_ptr.ptr()),
											actual_mbox->id() )
								};
						}
					)
				)
			{
			}

		void
		so_evt_start() override
			{
				so_5::send< msg_first >( *this );
			}

	private :
		void
		on_message(
			const std::type_index & msg_type,
			const so_5::message_ref_t & /*msg*/ )
			{
				if( msg_type == typeid(msg_first) )
					so_5::send< msg_second >( *this );
				else if( msg_type == typeid(msg_second) )
					so_5::send< msg_third >( *this );
				else if( msg_type == typeid(msg_third) )
					so_deregister_agent_coop_normally();
			}
	};

} /* namespace test */

using namespace test;

int
main()
	{
		run_with_time_limit(
			[]()
			{
				so_5::launch(
					[]( so_5::environment_t & env )
					{
						env.register_agent_as_coop(
								env.make_agent< a_test_t >() );
					} );
			},
			5 );

		return 0;
	}

