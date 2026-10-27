/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief Implementation of scheduler interface for an agent.
 *
 * \since v.5.8.7
 */

#include <so_5/impl/this_agent_scheduler.hpp>

#include <so_5/agent.hpp>

#include <so_5/exception.hpp>

#include <so_5/send_functions.hpp>

namespace so_5::impl
{

//
// this_agent_scheduler_t::special_mbox_for_delayed_resumptions_t
//
//FIXME: document this!
class this_agent_scheduler_t::special_mbox_for_delayed_resumptions_t final
	: public abstract_message_box_t
	{
		//! The scheduler for that this special mbox is created.
		this_agent_scheduler_t & m_owner;

	public:
		//! Initializing constructor.
		special_mbox_for_delayed_resumptions_t(
			this_agent_scheduler_t & owner )
			: m_owner{ owner }
			{}

		mbox_id_t
		id() const override
			{
				// This mbox has no ID.
				return null_mbox_id();
			}

		void
		subscribe_event_handler(
			const std::type_index & /*type_index*/,
			abstract_message_sink_t & /*subscriber*/ ) override
			{
				SO_5_THROW_EXCEPTION(
						rc_not_implemented,
						"subscribe_event_handler is not supported for "
						"special_mbox_for_delayed_resumptions_t" );
			}

		void
		unsubscribe_event_handler(
			const std::type_index & /*type_index*/,
			abstract_message_sink_t & /*subscriber*/ ) noexcept override
			{
				// Nothing to do.
			}

		std::string
		query_name() const override
			{
				return "this_agent_scheduler_t::special_mbox_for_"
						"delayed_resumptions_t";
			}

		mbox_type_t
		type() const override
			{
				return mbox_type_t::multi_producer_single_consumer;
			}

		void
		do_deliver_message(
			message_delivery_mode_t /*delivery_mode*/,
			const std::type_index & msg_type,
			const message_ref_t & message,
			unsigned int /*redirection_deep*/ ) override
			{
				std::lock_guard lock{ m_owner.m_lock };

				m_owner.m_special_queue.push_coro_resumption_demand(
						execution_demand_t{
								std::addressof(m_owner.m_receiver),
								message_limit::control_block_t::none(),
								null_mbox_id(),
								msg_type,
								message,
								agent_t::get_demand_handler_for_cpp_coro_resumtion_ptr()
						} );
			}

		void
		set_delivery_filter(
			const std::type_index & /*msg_type*/,
			const delivery_filter_t & /*filter*/,
			abstract_message_sink_t & /*subscriber*/ ) override
			{
				SO_5_THROW_EXCEPTION(
						rc_not_implemented,
						"set_delivery_filter is not supported for "
						"special_mbox_for_delayed_resumptions_t" );
			}

		void
		drop_delivery_filter(
			const std::type_index & /*msg_type*/,
			abstract_message_sink_t & /*subscriber*/ ) noexcept override
			{
				// Nothing to do.
			}

		so_5::environment_t &
		environment() const noexcept override
			{
				return m_owner.m_receiver.so_environment();
			}
	};

//
// this_agent_scheduler_t
//
this_agent_scheduler_t::this_agent_scheduler_t(
	agent_t & receiver,
	std::mutex & lock,
	cpp_coro_case_special_queue_t & special_queue )
	: m_receiver{ receiver }
	, m_lock{ lock }
	, m_special_queue{ special_queue }
	, m_delayed_msg_dest{ new special_mbox_for_delayed_resumptions_t{ *this } }
	{}

so_5::environment_t &
this_agent_scheduler_t::environment() const
	{
		return m_receiver.so_environment();
	}

void
this_agent_scheduler_t::try_schedule(
	so_5::cpp_coro::resumable_item_t & what_to_resume )
	{
		std::lock_guard lock{ m_lock };

		switch( what_to_resume.scheduler_data().m_status )
		{
		case so_5::cpp_coro::resumable_item_t::status_t::neutral:
		case so_5::cpp_coro::resumable_item_t::status_t::notified:
			// There is no need to schedule the coroutine because it's
			// running now.
		break;

		case so_5::cpp_coro::resumable_item_t::status_t::ready:
			// There is no need to schedule the coroutine because it's
			// already scheduled.
		break;

		case so_5::cpp_coro::resumable_item_t::status_t::waiting:
			what_to_resume.scheduler_data().m_status =
					so_5::cpp_coro::resumable_item_t::status_t::ready;

			// Extra data form resumable_item_t is necessary for a message
			// to be sent to the agent.
			auto & extra_data = what_to_resume.scheduler_data()
					.m_extra_data.get_or_create< notification_messages_t >(
							&notification_messages_t::factory );

			// NOTE: it's important that extra_data.cancel_timeout_elapsed
			// is not called here.

			extra_data.m_ordinary_resume =
					std::make_unique< msg_resume_coro >(
							what_to_resume,
							msg_resume_coro::ordinary{} );

			m_special_queue.push_coro_resumption_demand(
					execution_demand_t{
							std::addressof(m_receiver),
							message_limit::control_block_t::none(),
							null_mbox_id(),
							typeid(msg_resume_coro),
							extra_data.m_ordinary_resume,
							agent_t::get_demand_handler_for_cpp_coro_resumtion_ptr()
					} );
		break;
		}
	}

so_5::cpp_coro::try_suspend_result_t
this_agent_scheduler_t::try_suspend(
	so_5::cpp_coro::resumable_item_t & what_to_handle,
	so_5::cpp_coro::monotonic_clock_t::duration sleep_time )
	{
		std::lock_guard lock{ m_lock };

		auto & scheduler_data = what_to_handle.scheduler_data();

		switch( scheduler_data.m_status )
		{
		case so_5::cpp_coro::resumable_item_t::status_t::neutral:
			// This case will be handled outside of the switch.
		break;

		case so_5::cpp_coro::resumable_item_t::status_t::notified:
			// Coroutine is running now. The status has to be changed, but
			// coroutine shouldn't be suspended.
			scheduler_data.m_status =
					so_5::cpp_coro::resumable_item_t::status_t::neutral;

			return so_5::cpp_coro::try_suspend_result_t::should_be_resumed;

		case so_5::cpp_coro::resumable_item_t::status_t::ready:
			// This shouldn't happen.
			SO_5_THROW_EXCEPTION( rc_unexpected_coro_status,
					"coro has 'ready' status in "
					"this_agent_scheduler_t::try_suspend()" );
		break;

		case so_5::cpp_coro::resumable_item_t::status_t::waiting:
			// This shouldn't happen.
			SO_5_THROW_EXCEPTION( rc_unexpected_coro_status,
					"try_suspend is called for already suspended coroutine" );
		break;
		}

		scheduler_data.m_status =
				so_5::cpp_coro::resumable_item_t::status_t::waiting;

		if( so_5::cpp_coro::infinite_speep_time() != sleep_time )
		{
			// Extra data form resumable_item_t is necessary for a message
			// to be sent to the agent.
			auto & notifications = scheduler_data.m_extra_data
					.get_or_create< notification_messages_t >(
							notification_messages_t::factory );

			notifications.m_timeout_elapsed =
					std::make_unique< msg_resume_coro >(
							what_to_handle,
							msg_resume_coro::delayed{} );

			// Use send_periodic because we need the timer_id.
			notifications.m_timer = so_5::send_periodic(
					m_delayed_msg_dest,
					sleep_time,
					std::chrono::milliseconds::zero(),
					so_5::mhood_t< msg_resume_coro >(
							notifications.m_timeout_elapsed ) );
		}

		//FIXME: remove after debugging.
		std::cout << "coro will be suspended "
			<< what_to_handle.scheduler_data().m_to_be_resumed.address()
			<< " (" << std::addressof(what_to_handle) << ")"
			<< std::endl;

		return so_5::cpp_coro::try_suspend_result_t::should_be_suspended;
	}

} /* namespace so_5::impl */

