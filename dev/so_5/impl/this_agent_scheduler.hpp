/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief Implementation of scheduler interface for an agent.
 *
 * \since v.5.8.7
 */

#if !defined( SO_5_IMPL_THIS_AGENT_SCHEDULER_HPP )
#define SO_5_IMPL_THIS_AGENT_SCHEDULER_HPP

#include <so_5/impl/cpp_coro_case_special_queue.hpp>

#include <so_5/cpp_coro/scheduler.hpp>

#include <so_5/timers.hpp>

namespace so_5::impl
{

//
// msg_resume_coro
//

/*!
 * \brief Message to be used for resumption of a C++20 coroutine.
 *
 * \since v.5.8.7
 */
struct msg_resume_coro final : public so_5::message_t
	{
		//! A helper type used for debugging purposes.
		struct ordinary {};

		//! A helper type used for debugging purposes.
		struct delayed {};

		//! What has to be resumed.
		/*!
		 * \attention
		 * This value may be dropped to nullptr if delivery of this
		 * message is cancelled. It means that this value has to be checked
		 * when the message is delivered to the agent and resumption must
		 * be done only if this pointer is not nullptr.
		 */
		so_5::cpp_coro::resumable_item_t * m_what;

		//! Constructor for case when the coroutine has to be resumed via
		//! try_scheduler method.
		msg_resume_coro(
			so_5::cpp_coro::resumable_item_t & what,
			ordinary )
			: m_what{ std::addressof(what) }
			{
				//FIXME: remove after debugging.
				std::cout << this << " msg_resume_this(ordinary) created, what="
						<< m_what << std::endl;
			}

		//! Constructor for case when the coroutine has to be resumed after
		//! a timeout.
		msg_resume_coro(
			so_5::cpp_coro::resumable_item_t & what,
			delayed )
			: m_what{ std::addressof(what) }
			{
				//FIXME: remove after debugging.
				std::cout << this << " msg_resume_this(delayed) created, what="
						<< m_what << std::endl;
			}

		//FIXME: remove after debugging.
		~msg_resume_coro() override
			{
				std::cout << this << " msg_resume_this deleted, what="
						<< m_what << std::endl;
			}
	};

//
// this_agent_scheduler_t
//
//! Implementation of scheduler interface for a particual agent.
class this_agent_scheduler_t final
	: public so_5::cpp_coro::scheduler_t
	{
		class special_mbox_for_delayed_resumptions_t;
		friend class special_mbox_for_delayed_resumptions_t;

	public:
//FIXME: thread safety for cancellation operations has to be described
//in the comment.
		//! Special type to be used as an extra_data inside
		//! resumable_item_t.
		struct notification_messages_t
			{
				//! Current instance of msg_resume_coro for ordinary
				//! resumption of a coroutine via try_schedule method.
				so_5::message_ref_t m_ordinary_resume;

				//! ID of timer for resumption after a timeout.
				/*!
				 * Maybe empty if timeout is not used or has been cancelled.
				 */
				so_5::timer_id_t m_timer;

				//! Current instance of msg_resume_coro for resumption after
				//! a timeout.
				/*!
				 * Maybe empty if timeout is not used or has been cancelled.
				 */
				so_5::message_ref_t m_timeout_elapsed;

				/*!
				 * \note
				 * Destructor cancels all pending messages.
				 */
				~notification_messages_t()
					{
						cancel_all();

						//FIXME: remove after debugging.
						std::cout << "notification_messages_t destroyed" << std::endl;
					}

				//! Cancellation of all pending messages.
				void
				cancel_all()
					{
						cancel_ordinary_resume();
						cancel_timeout_elapsed();
					}

				//! Cancellation of ordinary resumption only.
				void
				cancel_ordinary_resume()
					{
						if( m_ordinary_resume )
						{
							// NOTE: don't expect exceptions here.
							auto & msg = dynamic_cast<msg_resume_coro &>(
									*m_ordinary_resume);
							msg.m_what = nullptr;

							//FIXME: remove after debugging.
							std::cout << "ordinary msg_resume_this cancelled, msg="
									<< std::addressof(msg) << std::endl;
							
							m_ordinary_resume.reset();
						}

					}

				//! Cancellation of delayed resumption only.
				void
				cancel_timeout_elapsed()
					{
						if( m_timeout_elapsed )
						{
							m_timer.release();

							auto & msg = dynamic_cast<msg_resume_coro &>(
									*m_timeout_elapsed);
							msg.m_what = nullptr;

							//FIXME: remove after debugging.
							std::cout << "delayed msg_resume_this cancelled, msg="
									<< std::addressof(msg) << std::endl;

							m_timeout_elapsed.reset();
						}
					}

				//! Deleter for instances of this type.
				/*!
				 * Pointer to this method will be used as deleter for
				 * resumable_item_t's extra_data deleter.
				 */
				static void
				deleter( void * info ) noexcept
					{
						delete static_cast<notification_messages_t *>(info);
					}

				//! Factory for creation of new instances of this type.
				[[nodiscard]] static
				so_5::cpp_coro::resumable_item_t::scheduler_data_t::extra_data_t
				factory()
					{
						auto info = std::make_unique< notification_messages_t >();
						return { info.release(), &notification_messages_t::deleter };
					}
			};

	private:
		//! The agent for that this scheduler is bound.
		/*!
		 * This reference is necessary for execution_demand_t instances.
		 */
		agent_t & m_receiver;

		//! Lock to be used for thread-safety.
		/*!
		 * \attention
		 * It's assumed that this lock is shared between this scheduler
		 * and the special_queue for the agent.
		 */
		std::mutex & m_lock;

		//! The special queue for this agent.
		cpp_coro_case_special_queue_t & m_special_queue;

		//! Mbox for delivering delayed msg_resume_coro.
		/*!
		 * \note
		 * The msg_resume_coro cannot be delivered the usual way.
		 * Because of that a special mbox is used that calls
		 * push_coro_resumption_demand for the special_queue.
		 */
		so_5::mbox_t m_delayed_msg_dest;

	public:
		//! Initializing constructor.
		this_agent_scheduler_t(
			agent_t & receiver,
			std::mutex & lock,
			cpp_coro_case_special_queue_t & special_queue );

		so_5::environment_t &
		environment() const override;

		void
		try_schedule(
			so_5::cpp_coro::resumable_item_t & what_to_resume ) override;

		so_5::cpp_coro::try_suspend_result_t
		try_suspend(
			so_5::cpp_coro::resumable_item_t & what_to_handle,
			so_5::cpp_coro::monotonic_clock_t::duration sleep_time ) override;
	};

} /* namespace so_5::impl */

#endif

