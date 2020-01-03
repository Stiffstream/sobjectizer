/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief Various stuff related to multi chain select.
 *
 * \note This file contains only publicly visible interfaces.
 * \since
 * v.5.5.16
 */

#pragma once

#include <so_5/mchain.hpp>

#include <memory>
#include <variant>

namespace so_5 {

namespace mchain_props {

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnon-virtual-dtor"
#endif

//
// select_notificator_t
//
/*!
 * \brief An interface of select_case notificator.
 *
 * \note This class has no virtual destructor becase there is no
 * intention to create instances of select_notificators dynamically.
 *
 * \since
 * v.5.5.16
 */
class select_notificator_t
{
public :
	virtual void
	notify( select_case_t & what ) noexcept = 0;
};

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

//
// select_case_t
//
/*!
 * \brief Base class for representation of one case in multi chain select.
 *
 * \attention Objects of this class are not copyable nor moveable.
 *
 * \since
 * v.5.5.16
 */
class select_case_t
	{
	protected :
		//! Message chain to receive message from.
		mchain_t m_chain;

		//FIXME: fix the content of this comment.
		//! Notificator to be used for notify sleeping thread.
		/*!
		 * Can be null. It means that select_case is not used in
		 * select queue for the mchain at that moment.
		 *
		 * There are just two methods where m_notificator changes its value:
		 *
		 * - try_receive() where m_notificator receives an actual pointer
		 * (m_notificator can become nullptr again in try_receive() if
		 * mchain has messages or was closed);
		 * - on_select_finish() where m_notificator receives nullptr value if
		 * it wasn't null yet.
		 *
		 * In the previous versions of SObjectizer m_notificator received
		 * nullptr value during notification of new messages arrival or
		 * closing of mchain. But this lead to data races and the behaviour
		 * was changed. Now m_notificator can hold an actual pointer even
		 * after notification was initiated.
		 */
		select_notificator_t * m_notificator = nullptr;

		//! Next select_case in queue.
		/*!
		 * A select_case object can be included in one of two different queues:
		 * - in select queue inside mchain (in this case m_notificator is not
		 *   nullptr). Next item in queue belongs to different select(). This
		 *   item must be notified in notify() method.
		 * - in ready to use select_case queue. The select_case is added to that
		 *   queue when select_case in notified by mchain. The next item in
		 *   queue belongs to the same select().
		 */
		select_case_t * m_next = nullptr;

		//FIXME: document this!
		[[nodiscard]]
		auto
		extract( demand_t & demand )
			{
				return m_chain->extract( demand, *this );
			}

		//FIXME: document this!
		[[nodiscard]]
		auto
		push(
			//! Type of message/signal to be pushed.
			const std::type_index & msg_type,
			//! Message/signal to be pushed.
			const message_ref_t & message )
			{
				return m_chain->push( msg_type, message, *this );
			}

	public :
		//! The result of attempt of handling this case.
		using handling_result_t = std::variant<
				so_5::mchain_receive_result_t,
				so_5::mchain_send_result_t >;

		//! Initialized constructor.
		select_case_t(
			//! Message chain for that this select_case is created.
			mchain_t chain )
			:	m_chain{ std::move(chain) }
			{}

		select_case_t( const select_case_t & ) = delete;
		select_case_t( select_case_t && ) = delete;

		virtual ~select_case_t() {}

		//! Simple access to next item in the current queue to which
		//! select_case object belongs at this moment.
		/*!
		 * \sa select_case_t::m_next
		 */
		[[nodiscard]]
		select_case_t *
		query_next() const noexcept
			{
				return m_next;
			}

		//! Get the next item in the current queue to which select_case belongs
		//! at this moment and drop this pointer to nullptr value.
		/*!
		 * This method must be used if select_case object must be extracted from
		 * the current queue.
		 *
		 * \sa select_case_t::m_next.
		 */
		[[nodiscard]]
		select_case_t *
		giveout_next() noexcept
			{
				auto n = m_next;
				m_next = nullptr;
				return n;
			}

		//! Set the next item in the current queue to which select_case belongs.
		/*!
		 * \sa select_case_t::m_next.
		 */
		void
		set_next( select_case_t * next ) noexcept
			{
				m_next = next;
			}

		//! Notification for all waiting select_cases.
		/*!
		 * This method is called by mchain if empty mchain becomes non-empty
		 * or if it is closed.
		 *
		 * This method does notification for all members of select_case queue.
		 * It means that mchain calls notify() for the head of the queue and
		 * that head does notification for all other queue's members.
		 */
		void
		notify() noexcept
			{
				auto c = this;
				while( c )
					{
						auto next = c->giveout_next();

						c->m_notificator->notify( *c );

						c = next;
					}
			}

		//! Reaction to the end of select work.
		/*!
		 * \attention This method must be called before return from select()
		 * function to ensure that mchain do not hold a pointer to non-existent
		 * select_case object.
		 *
		 * \attention This method must not throw because it will be called from
		 * destructor of RAII wrappers.
		 */
		void
		on_select_finish() noexcept
			{
				if( m_notificator )
					{
						m_chain->remove_from_select( *this );
						m_notificator = nullptr;
					}
			}

		//! An attempt to handle this case.
		[[nodiscard]]
		virtual handling_result_t
		try_handle( select_notificator_t & notificator ) = 0;

		//! Get the underlying mchain.
		/*!
		 * \since
		 * v.5.5.17
		 */
		[[nodiscard]]
		const mchain_t &
		chain() const noexcept
			{
				return m_chain;
			}
	};

//
// select_case_unique_ptr_t
//
/*!
 * \brief An alias of unique_ptr for select_case.
 * \since
 * v5.5.16
 */
using select_case_unique_ptr_t = std::unique_ptr< select_case_t >;

} /* namespace mchain_props */

} /* namespace so_5 */

