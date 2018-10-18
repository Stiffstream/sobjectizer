/*
 * SObjectizer-5
 */

/*!
 * \since
 * v.5.5.13
 *
 * \file
 * \brief Implementation details for message chains.
 */

#pragma once

#include <so_5/rt/h/mchain.hpp>
#include <so_5/rt/h/mchain_select_ifaces.hpp>
#include <so_5/rt/h/environment.hpp>

#include <so_5/h/ret_code.hpp>
#include <so_5/h/exception.hpp>
#include <so_5/h/error_logger.hpp>

#include <so_5/details/h/at_scope_exit.hpp>

#include <deque>
#include <vector>
#include <mutex>
#include <condition_variable>

namespace so_5 {

namespace mchain_props {

namespace details {

//
// ensure_queue_not_empty
//
/*!
 * \since
 * v.5.5.13
 *
 * \brief Helper function which throws an exception if queue is empty.
 */
template< typename Q >
void
ensure_queue_not_empty( Q && queue )
	{
		if( queue.is_empty() )
			SO_5_THROW_EXCEPTION(
					rc_msg_chain_is_empty,
					"an attempt to get message from empty demand queue" );
	}

//
// ensure_queue_not_full
//
/*!
 * \since
 * v.5.5.13
 *
 * \brief Helper function which throws an exception if queue is full.
 */
template< typename Q >
void
ensure_queue_not_full( Q && queue )
	{
		if( queue.is_full() )
			SO_5_THROW_EXCEPTION(
					rc_msg_chain_is_full,
					"an attempt to push a message to full demand queue" );
	}

//
// unlimited_demand_queue
//
/*!
 * \since
 * v.5.5.13
 *
 * \brief Implementation of demands queue for size-unlimited message chain.
 */
class unlimited_demand_queue
	{
	public :
		/*!
		 * \note This constructor is necessary just for a convinience.
		 */
		unlimited_demand_queue( const capacity_t & ) {}

		//! Is queue full?
		/*!
		 * \note Unlimited queue can't be null. Because of that this
		 * method always returns \a false.
		 */
		bool
		is_full() const { return false; }

		//! Is queue empty?
		bool
		is_empty() const { return m_queue.empty(); }

		//! Access to front item from queue.
		demand_t &
		front()
			{
				ensure_queue_not_empty( *this );
				return m_queue.front();
			}

		//! Remove the front item from queue.
		void
		pop_front()
			{
				ensure_queue_not_empty( *this );
				m_queue.pop_front();
			}

		//! Add a new item to the end of the queue.
		void
		push_back( demand_t && demand )
			{
				m_queue.push_back( std::move(demand) );
			}

		//! Size of the queue.
		std::size_t
		size() const { return m_queue.size(); }

	private :
		//! Queue's storage.
		std::deque< demand_t > m_queue;
	};

//
// limited_dynamic_demand_queue
//
/*!
 * \since
 * v.5.5.13
 *
 * \brief Implementation of demands queue for size-limited message chain with
 * dynamically allocated storage.
 */
class limited_dynamic_demand_queue
	{
	public :
		//! Initializing constructor.
		limited_dynamic_demand_queue(
			const capacity_t & capacity )
			:	m_max_size{ capacity.max_size() }
			{}

		//! Is queue full?
		bool
		is_full() const { return m_max_size == m_queue.size(); }

		//! Is queue empty?
		bool
		is_empty() const { return m_queue.empty(); }

		//! Access to front item from queue.
		demand_t &
		front()
			{
				ensure_queue_not_empty( *this );
				return m_queue.front();
			}

		//! Remove the front item from queue.
		void
		pop_front()
			{
				ensure_queue_not_empty( *this );
				m_queue.pop_front();
			}

		//! Add a new item to the end of the queue.
		void
		push_back( demand_t && demand )
			{
				ensure_queue_not_full( *this );
				m_queue.push_back( std::move(demand) );
			}

		//! Size of the queue.
		std::size_t
		size() const { return m_queue.size(); }

	private :
		//! Queue's storage.
		std::deque< demand_t > m_queue;
		//! Maximum size of the queue.
		const std::size_t m_max_size;
	};

//
// limited_preallocated_demand_queue
//
/*!
 * \since
 * v.5.5.13
 *
 * \brief Implementation of demands queue for size-limited message chain with
 * preallocated storage.
 */
class limited_preallocated_demand_queue
	{
	public :
		//! Initializing constructor.
		limited_preallocated_demand_queue(
			const capacity_t & capacity )
			:	m_storage( capacity.max_size(), demand_t{} )
			,	m_max_size{ capacity.max_size() }
			,	m_head{ 0 }
			,	m_size{ 0 }
			{}

		//! Is queue full?
		bool
		is_full() const { return m_max_size == m_size; }

		//! Is queue empty?
		bool
		is_empty() const { return 0 == m_size; }

		//! Access to front item from queue.
		demand_t &
		front()
			{
				ensure_queue_not_empty( *this );
				return m_storage[ m_head ];
			}

		//! Remove the front item from queue.
		void
		pop_front()
			{
				ensure_queue_not_empty( *this );
				m_storage[ m_head ] = demand_t{};
				m_head = (m_head + 1) % m_max_size;
				--m_size;
			}

		//! Add a new item to the end of the queue.
		void
		push_back( demand_t && demand )
			{
				ensure_queue_not_full( *this );
				auto index = (m_head + m_size) % m_max_size;
				m_storage[ index ] = std::move(demand);
				++m_size;
			}

		//! Size of the queue.
		std::size_t
		size() const { return m_size; }

	private :
		//! Queue's storage.
		std::vector< demand_t > m_storage;
		//! Maximum size of the queue.
		const std::size_t m_max_size;

		//! Index of the queue head.
		std::size_t m_head;
		//! The current size of the queue.
		std::size_t m_size;
	};

//
// status
//
/*!
 * \since
 * v.5.5.13
 *
 * \brief Status of the message chain.
 */
enum class status
	{
		//! Bag is open and can be used for message sending.
		open,
		//! Bag is closed. New messages cannot be sent to it.
		closed
	};

} /* namespace details */

//
// mchain_template
//
/*!
 * \since
 * v.5.5.13
 *
 * \brief Template-based implementation of message chain.
 *
 * \tparam Queue type of demand queue for message chain.
 * \tparam Tracing_Base type with message tracing implementation details.
 */
template< typename Queue, typename Tracing_Base >
class mchain_template
	:	public abstract_message_chain_t
	,	private Tracing_Base
	{
	public :
		//! Initializing constructor.
		template< typename... Tracing_Args >
		mchain_template(
			//! SObjectizer Environment for which message chain is created.
			so_5::environment_t & env,
			//! Mbox ID for this chain.
			mbox_id_t id,
			//! Chain parameters.
			const mchain_params_t & params,
			//! Arguments for Tracing_Base's constructor.
			Tracing_Args &&... tracing_args )
			:	Tracing_Base( std::forward<Tracing_Args>(tracing_args)... )
			,	m_env( env )
			,	m_id( id )
			,	m_capacity( params.capacity() )
			,	m_not_empty_notificator( params.not_empty_notificator() )
			,	m_queue( params.capacity() )
			{}

		virtual mbox_id_t
		id() const override
			{
				return m_id;
			}

		virtual void
		subscribe_event_handler(
			const std::type_index & /*msg_type*/,
			const so_5::message_limit::control_block_t * /*limit*/,
			agent_t * /*subscriber*/ ) override
			{
				SO_5_THROW_EXCEPTION(
						rc_msg_chain_doesnt_support_subscriptions,
						"mchain doesn't suppor subscription" );
			}

		virtual void
		unsubscribe_event_handlers(
			const std::type_index & /*msg_type*/,
			agent_t * /*subscriber*/ ) override
			{}

		virtual std::string
		query_name() const override
			{
				std::ostringstream s;
				s << "<mchain:id=" << m_id << ">";

				return s.str();
			}

		virtual mbox_type_t
		type() const override
			{
				return mbox_type_t::multi_producer_single_consumer;
			}

		virtual void
		do_deliver_message(
			const std::type_index & msg_type,
			const message_ref_t & message,
			unsigned int /*overlimit_reaction_deep*/ ) const override
			{
				// Constness must be removed explicitly.
				// Until do_deliver_message() lost const in v.5.6.0.
				const_cast< mchain_template * >(this)->
					try_to_store_message_to_queue(
							msg_type,
							message,
							invocation_type_t::event );
			}

		virtual void
		do_deliver_service_request(
			const std::type_index & msg_type,
			const message_ref_t & message,
			unsigned int /*overlimit_reaction_deep*/ ) const override
			{
				// Constness must be removed explicitly.
				// Until do_deliver_service_request() lost const in v.5.6.0.
				const_cast< mchain_template * >(this)->
					try_to_store_message_to_queue(
							msg_type,
							message,
							invocation_type_t::service_request );
			}

		void
		do_deliver_enveloped_msg(
			const std::type_index & msg_type,
			const message_ref_t & message,
			unsigned int /*overlimit_reaction_deep*/ ) override
			{
				try_to_store_message_to_queue(
						msg_type,
						message,
						invocation_type_t::enveloped_msg );
			}

		/*!
		 * \attention Will throw an exception because delivery
		 * filter is not applicable to MPSC-mboxes.
		 */
		virtual void
		set_delivery_filter(
			const std::type_index & /*msg_type*/,
			const delivery_filter_t & /*filter*/,
			agent_t & /*subscriber*/ ) override
			{
				SO_5_THROW_EXCEPTION(
						rc_msg_chain_doesnt_support_delivery_filters,
						"set_delivery_filter is called for mchain" );
			}

		virtual void
		drop_delivery_filter(
			const std::type_index & /*msg_type*/,
			agent_t & /*subscriber*/ ) SO_5_NOEXCEPT override
			{}

		virtual extraction_status_t
		extract(
			demand_t & dest,
			duration_t empty_queue_timeout ) override
			{
				std::unique_lock< std::mutex > lock{ m_lock };

				// If queue is empty we must wait for some time.
				bool queue_empty = m_queue.is_empty();
				if( queue_empty )
					{
						if( details::status::closed == m_status )
							// Waiting for new messages has no sence because
							// chain is closed.
							return extraction_status_t::chain_closed;

						auto predicate = [this, &queue_empty]() -> bool {
								queue_empty = m_queue.is_empty();
								return !queue_empty ||
										details::status::closed == m_status;
							};

						// Count of sleeping thread must be incremented before
						// going to sleep and decremented right after.
						++m_threads_to_wakeup;
						auto decrement_threads = so_5::details::at_scope_exit(
								[this] { --m_threads_to_wakeup; } );

						if( !details::is_infinite_wait_timevalue( empty_queue_timeout ) )
							// A wait with finite timeout must be performed.
							m_underflow_cond.wait_for(
									lock, empty_queue_timeout, predicate );
						else
							// Wait until arrival of any message or closing of chain.
							m_underflow_cond.wait( lock, predicate );
					}

				// If queue is still empty nothing can be extracted and
				// we must stop operation.
				if( queue_empty )
					return details::status::open == m_status ?
							// The chain is still open so there must be this result
							extraction_status_t::no_messages :
							// The chain is closed and there must be different result
							extraction_status_t::chain_closed;

				return extract_demand_from_not_empty_queue( dest );
			}

		virtual bool
		empty() const override
			{
				return m_queue.is_empty();
			}

		virtual std::size_t
		size() const override
			{
				return m_queue.size();
			}

		virtual void
		close( close_mode_t mode ) override
			{
				std::lock_guard< std::mutex > lock{ m_lock };

				if( details::status::closed == m_status )
					return;

				m_status = details::status::closed;

				const bool was_full = m_queue.is_full();

				if( close_mode_t::drop_content == mode )
					{
						while( !m_queue.is_empty() )
							{
								this->trace_demand_drop_on_close(
										*this, m_queue.front() );
								m_queue.pop_front();
							}
					}

				// If queue is empty now and there is any multi chain select
				// than select_tail must be handled.
				if( m_queue.is_empty() )
					notify_multi_chain_select_ops();

				if( m_threads_to_wakeup )
					// Someone is waiting on empty chain for new messages.
					// It must be informed that no new messages will be here.
					m_underflow_cond.notify_all();

				if( was_full )
					// Someone can wait on full chain for free place for new message.
					// It must be informed that the chain is closed.
					m_overflow_cond.notify_all();
			}

		virtual environment_t &
		environment() const override
			{
				return m_env;
			}

	protected :
		virtual extraction_status_t
		extract(
			demand_t & dest,
			select_case_t & select_case ) override
			{
				std::unique_lock< std::mutex > lock{ m_lock };

				const bool queue_empty = m_queue.is_empty();
				if( queue_empty )
					{
						if( details::status::closed == m_status )
							// There is no need to wait for something.
							return extraction_status_t::chain_closed;

						// In other cases select_tail must be modified.
						select_case.set_next( m_select_tail );
						m_select_tail = &select_case;

						return extraction_status_t::no_messages;
					}
				else
					return extract_demand_from_not_empty_queue( dest );
			}

		virtual void
		remove_from_select(
			select_case_t & select_case ) override
			{
				std::lock_guard< std::mutex > lock{ m_lock };

				select_case_t * c = m_select_tail;
				select_case_t * prev = nullptr;
				while( c )
					{
						select_case_t * const next = c->query_next();
						if( c == &select_case )
							{
								if( prev )
									prev->set_next( next );
								else
									m_select_tail = next;

								return;
							}

						prev = c;
						c = next;
					}
			}

		virtual void
		do_deliver_message_from_timer(
			const std::type_index & msg_type,
			const message_ref_t & message ) override
			{
				// Since v.5.5.23 we should detect actual invocation type
				// for a timer event.
				const auto invocation_type =
						message_t::kind_t::enveloped_msg == message_kind( message ) ?
						invocation_type_t::enveloped_msg : invocation_type_t::event;

				try_to_store_message_from_timer_to_queue(
						msg_type,
						message,
						invocation_type );
			}

	private :
		//! SObjectizer Environment for which message chain is created.
		environment_t & m_env;

		//! Status of the chain.
		details::status m_status = { details::status::open };

		//! Mbox ID for chain.
		const mbox_id_t m_id;

		//! Chain capacity.
		const capacity_t m_capacity;

		//! Optional notificator for 'not_empty' condition.
		const not_empty_notification_func_t m_not_empty_notificator;

		//! Chain's demands queue.
		mutable Queue m_queue;

		//! Chain's lock.
		mutable std::mutex m_lock;

		//! Condition variable for waiting on empty queue.
		mutable std::condition_variable m_underflow_cond;
		//! Condition variable for waiting on full queue.
		mutable std::condition_variable m_overflow_cond;

		/*!
		 * \brief Count of threads sleeping on empty mchain.
		 *
		 * This value is incremented before sleeping on m_underflow_cond and
		 * decremented just after a return from this sleep.
		 *
		 * \since
		 * v.5.5.16
		 */
		std::size_t m_threads_to_wakeup = { 0 };

		/*!
		 * \brief A queue of multi-chain selects in which this chain is used.
		 *
		 * \since
		 * v.5.5.16
		 */
		mutable select_case_t * m_select_tail = nullptr;

		//! Actual implementation of pushing message to the queue.
		/*!
		 * \note
		 * This implementation must be used for ordinary delivery operations.
		 * For delivery operations from timer thread another method must be
		 * called (see try_to_store_message_from_timer_to_queue()).
		 */
		void
		try_to_store_message_to_queue(
			const std::type_index & msg_type,
			const message_ref_t & message,
			invocation_type_t demand_type )
			{
				typename Tracing_Base::deliver_op_tracer tracer{
						*this, // as tracing base.
						*this, // as chain.
						msg_type,
						message,
						demand_type };

				std::unique_lock< std::mutex > lock{ m_lock };

				// Message cannot be stored to closed chain.
				if( details::status::closed == m_status )
					return;

				// If queue full and waiting on full queue is enabled we
				// must wait for some time until there will be some space in
				// the queue.
				bool queue_full = m_queue.is_full();
				if( queue_full && m_capacity.is_overflow_timeout_defined() )
					{
						m_overflow_cond.wait_for(
								lock,
								m_capacity.overflow_timeout(),
								[this, &queue_full] {
									queue_full = m_queue.is_full();
									return !queue_full ||
											details::status::closed == m_status;
								} );
					}

				// If queue still full we must perform some reaction.
				if( queue_full )
					{
						const auto reaction = m_capacity.overflow_reaction();
						if( overflow_reaction_t::drop_newest == reaction )
							{
								// New message must be simply ignored.
								tracer.overflow_drop_newest();
								return;
							}
						else if( overflow_reaction_t::remove_oldest == reaction )
							{
								// The oldest message must be simply removed.
								tracer.overflow_remove_oldest( m_queue.front() );
								m_queue.pop_front();
							}
						else if( overflow_reaction_t::throw_exception == reaction )
							{
								tracer.overflow_throw_exception();
								SO_5_THROW_EXCEPTION(
										rc_msg_chain_overflow,
										"an attempt to push message to full mchain "
										"with overflow_reaction_t::throw_exception policy" );
							}
						else
							{
								so_5::details::abort_on_fatal_error( [&] {
										tracer.overflow_throw_exception();
										SO_5_LOG_ERROR( m_env, log_stream ) {
											log_stream << "overflow_reaction_t::abort_app "
													"will be performed for mchain (id="
													<< m_id << "), msg_type: "
													<< msg_type.name()
													<< ". Application will be aborted"
													<< std::endl;
										}
									} );
							}
					}

				complete_store_message_to_queue(
						tracer,
						msg_type,
						message,
						demand_type );
			}

		/*!
		 * \brief An implementation of storing another message to
		 * chain for the case of delated/periodic messages.
		 *
		 * This implementation handles overloaded chains differently:
		 * - there is no waiting on overloaded chain (even if such waiting
		 *   is specified in mchain params);
		 * - overflow_reaction_t::throw_exception is replaced by
		 *   overflow_reaction_t::drop_newest.
		 *
		 * These defferences are necessary because the context of timer
		 * thread is very special: there can't be any long-time operation
		 * (like waiting for free space on overloaded chain) and there can't
		 * be an exception about mchain's overflow.
		 *
		 * \since
		 * v.5.5.18
		 */
		void
		try_to_store_message_from_timer_to_queue(
			const std::type_index & msg_type,
			const message_ref_t & message,
			invocation_type_t demand_type )
			{
				typename Tracing_Base::deliver_op_tracer tracer{
						*this, // as tracing base.
						*this, // as chain.
						msg_type,
						message,
						demand_type };

				std::unique_lock< std::mutex > lock{ m_lock };

				// Message cannot be stored to closed chain.
				if( details::status::closed == m_status )
					return;

				bool queue_full = m_queue.is_full();
				// NOTE: there is no awaiting on full mchain.
				// If queue full we must perform some reaction.
				if( queue_full )
					{
						const auto reaction = m_capacity.overflow_reaction();
						if( overflow_reaction_t::drop_newest == reaction ||
								overflow_reaction_t::throw_exception == reaction )
							{
								// New message must be simply ignored.
								tracer.overflow_drop_newest();
								return;
							}
						else if( overflow_reaction_t::remove_oldest == reaction )
							{
								// The oldest message must be simply removed.
								tracer.overflow_remove_oldest( m_queue.front() );
								m_queue.pop_front();
							}
						else
							{
								so_5::details::abort_on_fatal_error( [&] {
										tracer.overflow_throw_exception();
										SO_5_LOG_ERROR( m_env, log_stream ) {
											log_stream << "overflow_reaction_t::abort_app "
													"will be performed for mchain (id="
													<< m_id << "), msg_type: "
													<< msg_type.name()
													<< ". Application will be aborted"
													<< std::endl;
										}
									} );
							}
					}

				complete_store_message_to_queue(
						tracer,
						msg_type,
						message,
						demand_type );
			}

		/*!
		 * \brief Implementation of extract operation for the case when
		 * message queue is not empty.
		 *
		 * \attention This helper method must be called when chain object
		 * is locked in some hi-level method.
		 *
		 * \since
		 * v.5.5.16
		 */
		extraction_status_t
		extract_demand_from_not_empty_queue(
			demand_t & dest )
			{
				// If queue was full then someone can wait on it.
				const bool queue_was_full = m_queue.is_full();
				dest = std::move( m_queue.front() );
				m_queue.pop_front();

				this->trace_extracted_demand( *this, dest );

				if( queue_was_full )
					m_overflow_cond.notify_all();

				return extraction_status_t::msg_extracted;
			}

		/*!
		 * \note This method declared as const by the same reason
		 * as try_to_store_message_to_queue() method.
		 *
		 * \since
		 * v.5.5.16
		 */
		void
		notify_multi_chain_select_ops() const SO_5_NOEXCEPT
			{
				if( m_select_tail )
					{
						auto old = m_select_tail;
						m_select_tail = nullptr;
						old->notify();
					}
			}

		/*!
		 * \brief A reusable method with implementation of
		 * last part of storing a message into chain.
		 *
		 * \note
		 * Intented to be called from try_to_store_message_to_queue()
		 * and try_to_store_message_from_timer_to_queue().
		 *
		 * \since
		 * v.5.5.18
		 */
		void
		complete_store_message_to_queue(
			typename Tracing_Base::deliver_op_tracer & tracer,
			const std::type_index & msg_type,
			const message_ref_t & message,
			invocation_type_t demand_type )
			{
				const bool was_empty = m_queue.is_empty();
				
				m_queue.push_back(
						demand_t{ msg_type, message, demand_type } );

				tracer.stored( m_queue );

				// If chain was empty then multi-chain cases must be notified.
				// And if not_empty_notificator is defined then it must be used too.
				if( was_empty )
					{
						if( m_not_empty_notificator )
							so_5::details::invoke_noexcept_code(
								[this] { m_not_empty_notificator(); } );

						notify_multi_chain_select_ops();
					}

				// Should be wake up some sleeping thread?
				if( m_threads_to_wakeup && m_threads_to_wakeup >= m_queue.size() )
					// Someone is waiting on empty queue.
					m_underflow_cond.notify_one();
			}
	};

} /* namespace mchain_props */

} /* namespace so_5 */

