/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief Various stuff related to multi chain select.
 * \since
 * v.5.5.16
 */

#pragma once

#include <so_5/rt/h/mchain_select_ifaces.hpp>

#include <so_5/details/h/at_scope_exit.hpp>
#include <so_5/details/h/invoke_noexcept_code.hpp>
#include <so_5/details/h/remaining_time_counter.hpp>

#include <iterator>
#include <array>

namespace so_5 {

//
// mchain_select_params_t
//
/*!
 * \brief Parameters for advanced select from multiple mchains.
 *
 * \sa so_5::select().
 *
 * \note Adds nothing to mchain_bulk_processing_params_t.
 *
 * \since
 * v.5.5.16
 */
class mchain_select_params_t final
	: public mchain_bulk_processing_params_t< mchain_select_params_t >
	{};

//
// from_all
//
/*!
 * \brief Helper function for creation of mchain_select_params instance
 * with default values.
 *
 * Usage example:
 * \code
	select( so_5::from_all().handle_n(3).empty_timeout(3s), ... );
 * \endcode
 */
inline mchain_select_params_t
from_all()
	{
		return mchain_select_params_t{};
	}

namespace mchain_props {

namespace details {

//
// actual_select_case_t
//
/*!
 * \brief Actual implementation of one multi chain select case.
 *
 * \since
 * v.5.5.16
 */
template< std::size_t N >
class actual_select_case_t : public select_case_t
	{
		so_5::details::handlers_bunch_t< N > m_handlers;

	public :
		//! Initializing constructor.
		/*!
		 * \tparam Handlers list of message handlers for messages extracted
		 * from the mchain.
		 */
		template< typename... Handlers >
		actual_select_case_t(
			//! Chain to be used for select.
			mchain_t chain,
			//! Message handlers.
			Handlers &&... handlers )
			:	select_case_t( std::move(chain) )
			{
				so_5::details::fill_handlers_bunch(
						m_handlers,
						0,
						std::forward< Handlers >(handlers)... );
			}

	protected :
		virtual mchain_receive_result_t
		try_handle_extracted_message( demand_t & demand ) override
			{
				const bool handled = m_handlers.handle(
						demand.m_msg_type,
						demand.m_message_ref,
						demand.m_demand_type );

				return mchain_receive_result_t{
						1u,
						handled ? 1u : 0u,
						extraction_status_t::msg_extracted };
			}
	};

//
// select_cases_holder_t
//
/*!
 * \brief A holder for serie of select_cases.
 *
 * Provides access to select_cases via iterator and begin() and end() methods.
 *
 * \note This is moveable class, but not copyable.
 *
 * \since
 * v.5.5.16
 */
template< std::size_t Cases_Count >
class select_cases_holder_t
	{
		//! Type of array for holding select_cases.
		using array_type_t = std::array< select_case_unique_ptr_t, Cases_Count >;
		//! Storage for select_cases.
		array_type_t m_cases;

	public :
		select_cases_holder_t( const select_cases_holder_t & ) = delete;
		select_cases_holder_t &
		operator=( const select_cases_holder_t & ) = delete;

		//! Default constructor.
		select_cases_holder_t()
			{}
		//! Move constructor.
		select_cases_holder_t( select_cases_holder_t && o )
			{
				swap( o );
			}

		//! Move operator.
		select_cases_holder_t &
		operator=( select_cases_holder_t && o ) SO_5_NOEXCEPT
			{
				select_cases_holder_t tmp( std::move( o ) );
				swap( tmp );

				return *this;
			}

		//! Swap operation.
		void
		swap( select_cases_holder_t & o ) SO_5_NOEXCEPT
			{
				for( std::size_t i = 0; i != Cases_Count; ++i )
					m_cases[ i ] = std::move(o.m_cases[ i ]);
			}

		//! Helper method for setting up specific select_case.
		/*!
		 * This method will be used during creation of select_cases_holder.
		 */
		void
		set_case( std::size_t index, select_case_unique_ptr_t c )
			{
				m_cases[ index ] = std::move(c);
			}

		//! Get count of select_cases in holder.
		std::size_t
		size() const { return Cases_Count; }

		//! Iterator class for accessing select_cases.
		/*!
		 * Implements ForwardIterator concept.
		 */
		class const_iterator
			: public std::iterator< std::forward_iterator_tag, select_case_t >
			{
				using actual_it_t = typename array_type_t::const_iterator;

				actual_it_t m_it;

			public :
				const_iterator() {}
				const_iterator( actual_it_t it ) : m_it( std::move(it) ) {}

				const_iterator & operator++() { ++m_it; return *this; }
				const_iterator operator++(int) { const_iterator o{ m_it }; ++m_it; return o; }

				bool operator==( const const_iterator & o ) const { return m_it == o.m_it; }
				bool operator!=( const const_iterator & o ) const { return m_it != o.m_it; }

				select_case_t & operator*() const { return **m_it; }
				select_case_t * operator->() const { return m_it->get(); }
			};

		//! Get iterator for the first item in select_cases_holder.
		const_iterator
		begin() const { return const_iterator{ m_cases.begin() }; }

		//! Get iterator for the item just behind the last item in select_cases_holder.
		const_iterator
		end() const { return const_iterator{ m_cases.end() }; }
	};

//
// fill_select_cases_holder
//

template< typename Holder >
void
fill_select_cases_holder(
	Holder & holder,
	std::size_t index,
	select_case_unique_ptr_t c )
	{
		holder.set_case( index, std::move(c) );
	}

template< typename Holder, typename... Cases >
void
fill_select_cases_holder(
	Holder & holder,
	std::size_t index,
	select_case_unique_ptr_t c,
	Cases &&... other_cases )
	{
		fill_select_cases_holder( holder, index, std::move(c));
		fill_select_cases_holder(
				holder, index + 1, std::forward< Cases >(other_cases)... );
	}

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnon-virtual-dtor"
#endif

//
// actual_select_notificator_t
//
/*!
 * \brief Actual implementation of notificator for multi chain select.
 *
 * \since
 * v.5.5.16
 */
class actual_select_notificator_t : public select_notificator_t
	{
	private :
		std::mutex m_lock;
		std::condition_variable m_condition;

		//! Queue of already notified select_cases.
		select_case_t * m_tail = nullptr;

		/*!
		 * \attention This method must be called only on locked object.
		 */
		void
		push_to_notified_chain( select_case_t & what ) SO_5_NOEXCEPT
			{
				what.set_next( m_tail );
				m_tail = &what;
			}

	public :
		/*!
		 * \brief Initializing constructor.
		 *
		 * Intended to be used with select_cases_holder and its iterators.
		 *
		 * Every select_case is automatically added to the list of notified
		 * select_cases.
		 */
		template< typename Fwd_it >
		actual_select_notificator_t( Fwd_it b, Fwd_it e )
			{
				// All select_cases from range [b,e) must be included in
				// ready_cases list.
				while( b != e )
					{
						b->set_next( m_tail );
						m_tail = &(*b);
						++b;
					}
			}

		virtual void
		notify( select_case_t & what ) SO_5_NOEXCEPT override
			{
				select_case_t * old_tail = nullptr;
				{
					std::lock_guard< std::mutex > lock{ m_lock };

					old_tail = m_tail;
					push_to_notified_chain( what );
				}

				if( !old_tail )
					m_condition.notify_one();
			}

		/*!
		 * \brief Return specifed select_case object to the chain of
		 * 'notified select_cases'.
		 *
		 * If a message has been read from a mchain then there could be
		 * other messages in that mchain. Because of that the select_case
		 * for that mchain must be seen as 'notified' -- it should be
		 * processed on next call to wait() method. This method must be
		 * used for immediately return of select_case to the chain of
		 * 'notified select_cases'.
		 */
		void
		return_to_ready_chain( select_case_t & what ) SO_5_NOEXCEPT
			{
				std::lock_guard< std::mutex > lock{ m_lock };
				push_to_notified_chain( what );
			}

		/*!
		 * \brief Wait for any notified select_case.
		 *
		 * Waiting no more than \a wait_time.
		 *
		 * \return nullptr if there is no notified select_cases after
		 * waiting for \a wait_time.
		 */
		select_case_t *
		wait(
			//! Maximum waiting time for notified select_case.
			duration_t wait_time )
			{
				std::unique_lock< std::mutex > lock{ m_lock };
				if( !m_tail )
					m_condition.wait_for(
							lock,
							wait_time,
							[this]{ return m_tail != nullptr; } );

				auto * result = m_tail;
				m_tail = nullptr;

				return result;
			}
	};

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

//
// select_actions_performer_t
//
/*!
 * \brief Helper class for performing select-specific operations.
 *
 * \tparam Holder type of actual select_cases_holder_t.
 *
 * \since
 * v.5.5.16
 */
template< typename Holder >
class select_actions_performer_t
	{
		const mchain_select_params_t & m_params;
		const Holder & m_select_cases;
		actual_select_notificator_t m_notificator;

		std::size_t m_closed_chains = 0;
		std::size_t m_extracted_messages = 0;
		std::size_t m_handled_messages = 0;
		extraction_status_t m_status;
		bool m_can_continue = { true };

	public :
		select_actions_performer_t(
			const mchain_select_params_t & params,
			const Holder & select_cases )
			:	m_params( params )
			,	m_select_cases( select_cases )
			,	m_notificator( select_cases.begin(), select_cases.end() )
			{}
		~select_actions_performer_t()
			{
				for( auto & c : m_select_cases )
					c.on_select_finish();
			}

		void
		handle_next( const duration_t & wait_time )
			{
				select_case_t * ready_chain = m_notificator.wait( wait_time );
				if( !ready_chain )
					{
						m_status = extraction_status_t::no_messages;
						update_can_continue_flag();
					}
				else
					handle_ready_chain( ready_chain );
			}

		extraction_status_t
		last_status() const { return m_status; }

		bool
		can_continue() const { return m_can_continue; }

		mchain_receive_result_t
		make_result() const
			{
				return mchain_receive_result_t{
						m_extracted_messages,
						m_handled_messages,
						m_extracted_messages ? extraction_status_t::msg_extracted :
								( m_closed_chains == m_select_cases.size() ?
								  	extraction_status_t::chain_closed :
									extraction_status_t::no_messages )
					};
			}

	private :
		void
		handle_ready_chain( select_case_t * ready_chain )
			{
				while( ready_chain && m_can_continue )
					{
						auto * current = ready_chain;
						ready_chain = current->giveout_next();

						const auto result = current->try_receive( m_notificator );
						m_status = result.status();

						if( extraction_status_t::msg_extracted == m_status )
							{
								m_extracted_messages += result.extracted();
								m_handled_messages += result.handled();

								// The mchain from 'current' can contain more
								// messages. We should return this case to 'ready_chain'
								// of the notificator.
								m_notificator.return_to_ready_chain( *current );
							}
						else if( extraction_status_t::chain_closed == m_status )
							{
								++m_closed_chains;

								// Since v.5.5.17 chain_closed handler must be
								// used on chain_closed event.
								if( const auto & handler = m_params.closed_handler() )
									so_5::details::invoke_noexcept_code(
										[&handler, current] {
											handler( current->chain() );
										} );
							}

						update_can_continue_flag();
					}
			}

		void
		update_can_continue_flag()
			{
				auto fn = [this] {
					if( m_closed_chains == m_select_cases.size() )
						return false;

					if( m_params.to_handle() &&
							m_handled_messages >= m_params.to_handle() )
						return false;

					if( m_params.to_extract() &&
							m_extracted_messages >= m_params.to_extract() )
						return false;

					if( m_params.stop_on() && m_params.stop_on()() )
						return false;

					return true;
				};

				m_can_continue = fn();
			}
	};

template< typename Holder >
mchain_receive_result_t
do_adv_select_with_total_time(
	const mchain_select_params_t & params,
	const Holder & select_cases )
	{
		using namespace so_5::details;

		select_actions_performer_t< Holder > performer{ params, select_cases };

		remaining_time_counter_t time_counter{ params.total_time() };
		do
			{
				performer.handle_next( time_counter.remaining() );
				time_counter.update();
			}
		while( time_counter && performer.can_continue() );

		return performer.make_result();
	}

template< typename Holder >
mchain_receive_result_t
do_adv_select_without_total_time(
	const mchain_select_params_t & params,
	const Holder & select_cases )
	{
		using namespace so_5::details;

		select_actions_performer_t< Holder > performer{ params, select_cases };

		remaining_time_counter_t wait_time{ params.empty_timeout() };
		do
			{
				performer.handle_next( wait_time.remaining() );
				if( extraction_status_t::msg_extracted == performer.last_status() )
					// Becase some message extracted we must restart wait_time
					// counting.
					wait_time = remaining_time_counter_t{ params.empty_timeout() };
				else
					// There could be one of two situations:
					// 1) several threads do select on the same mchain.
					//    Both threads will be awoken when some message is
					//    pushed into the mchain. But only one thread will get
					//    this message. Second thread will receive no_messages
					//    status. In this case we should wait for the next message,
					//    but wait_time must be decremented.
					// 2) some chain is closed. Wait time should be updated and
					//    next wait attempt must be performed.
					wait_time.update();
			}
		while( wait_time && performer.can_continue() );

		return performer.make_result();
	}

//
// perform_select
//
/*!
 * \brief Helper function with implementation of main select action.
 *
 * \since
 * v.5.5.17
 */
template< typename Cases_Holder >
mchain_receive_result_t
perform_select(
	//! Parameters for advanced select.
	const mchain_select_params_t & params,
	//! Select cases.
	const Cases_Holder & cases_holder )
	{
		if( is_infinite_wait_timevalue( params.total_time() ) )
			return do_adv_select_without_total_time( params, cases_holder );
		else
			return do_adv_select_with_total_time( params, cases_holder );
	}

} /* namespace details */

} /* namespace mchain_props */

//
// case_
//
/*!
 * \brief A helper for creation of select_case object for one multi chain
 * select.
 *
 * \attention It is an error if there are more than one handler for the
 * same message type in \a handlers.
 *
 * \sa so_5::select()
 *
 * \since
 * v.5.5.16
 */
template< typename... Handlers >
mchain_props::select_case_unique_ptr_t
case_(
	//! Message chain to be used in select.
	mchain_t chain,
	//! Message handlers for messages extracted from that chain.
	Handlers &&... handlers )
	{
		using namespace mchain_props;
		using namespace mchain_props::details;

		return select_case_unique_ptr_t{
				new actual_select_case_t< sizeof...(handlers) >{
						std::move(chain),
						std::forward< Handlers >(handlers)... } };
	}

/*!
 * \brief An advanced form of multi chain select.
 *
 * \attention The behaviour is not defined if a mchain is used in different
 * select_cases.
 *
 * \par Usage examples:
	\code
	using namespace so_5;

	mchain_t ch1 = env.create_mchain(...);
	mchain_t ch2 = env.create_mchain(...);

	// Receive and handle 3 messages.
	// It could be 3 messages from ch1. Or 2 messages from ch1 and 1 message
	// from ch2. Or 1 message from ch1 and 2 messages from ch2. And so on...
	//
	// If there is no 3 messages in mchains the select will wait infinitely.
	// A return from select will be after handling of 3 messages or
	// if all mchains are closed explicitely.
	select( from_all().handle_n( 3 ),
		case_( ch1,
				[]( const first_message_type & msg ) { ... },
				[]( const second_message_type & msg ) { ... } ),
		case_( ch2,
				[]( const third_message_type & msg ) { ... },
				handler< some_signal_type >( []{ ... ] ),
				... ) );

	// Receive and handle 3 messages.
	// If there is no 3 messages in chains the select will wait
	// no more that 200ms.
	// A return from select will be after handling of 3 messages or
	// if all mchains are closed explicitely, or if there is no messages
	// for more than 200ms.
	select( from_all().handle_n( 3 ).empty_timeout( milliseconds(200) ),
		case_( ch1,
				[]( const first_message_type & msg ) { ... },
				[]( const second_message_type & msg ) { ... } ),
		case_( ch2,
				[]( const third_message_type & msg ) { ... },
				handler< some_signal_type >( []{ ... ] ),
				... ) );

	// Receive all messages from mchains.
	// If there is no message in any of mchains then wait no more than 500ms.
	// A return from select will be after explicit close of all mchains
	// or if there is no messages for more than 500ms.
	select( from_all().empty_timeout( milliseconds(500) ),
		case_( ch1,
				[]( const first_message_type & msg ) { ... },
				[]( const second_message_type & msg ) { ... } ),
		case_( ch2,
				[]( const third_message_type & msg ) { ... },
				handler< some_signal_type >( []{ ... ] ),
				... ) );

	// Receve any number of messages from mchains but do waiting and
	// handling for no more than 2s.
	select( from_all().total_time( seconds(2) ),
		case_( ch1,
				[]( const first_message_type & msg ) { ... },
				[]( const second_message_type & msg ) { ... } ),
		case_( ch2,
				[]( const third_message_type & msg ) { ... },
				handler< some_signal_type >( []{ ... ] ),
				... ) );

	// Receve 1000 messages from chains but do waiting and
	// handling for no more than 2s.
	select( from_all().extract_n( 1000 ).total_time( seconds(2) ),
		case_( ch1,
				[]( const first_message_type & msg ) { ... },
				[]( const second_message_type & msg ) { ... } ),
		case_( ch2,
				[]( const third_message_type & msg ) { ... },
				handler< some_signal_type >( []{ ... ] ),
				... ) );
	\endcode
 *
 * \since
 * v.5.5.16
 */
template< typename... Cases >
mchain_receive_result_t
select(
	//! Parameters for advanced select.
	const mchain_select_params_t & params,
	//! Select cases.
	Cases &&... cases )
	{
		using namespace mchain_props;
		using namespace mchain_props::details;

		select_cases_holder_t< sizeof...(cases) > cases_holder;
		fill_select_cases_holder(
				cases_holder, 0, std::forward< Cases >(cases)... );

		return perform_select( params, cases_holder );
	}

//
// select
//
/*!
 * \brief A simple form of multi chain select.
 *
 * This is just a shortcat for more advanced version of select(). A call:
 * \code
	using namespace so_5;

	select( std::chrono::seconds(5), case_(...), ... );
 * \endcode
 * Can be rewritten as:
 * \code
	select( from_all().empty_timeout( std::chrono::seconds(5) ).extract_n(1),
		case_(...), ... );
 * \endcode
 *
 * \note The function returns control if:
 * - there is no any message for \a wait_time;
 * - all mchains are closed;
 * - any message has been extracted from any mchain. It is possible that
 *   this message is not handled at all if there is no a handler of it.
 *
 * \since
 * v.5.5.16
 */
template< typename Duration, typename... Cases >
mchain_receive_result_t
select( Duration wait_time, Cases &&... cases )
	{
		return select(
				mchain_select_params_t{}.extract_n( 1 ).empty_timeout( wait_time ),
				std::forward< Cases >(cases)... );
	}

//
// prepared_select_t
//
/*!
 * \brief Special container for holding select parameters and select cases.
 *
 * \note Instances of that type usually used without specifying the actual
 * type:
 * \code
	auto prepared = so_5::prepare_select(
		so_5::from_all().handle_n(10).empty_timeout(10s),
		case_( ch1, some_handlers... ),
		case_( ch2, more_handlers... ), ... );
	...
	auto r = so_5::select( prepared );
 * \endcode
 *
 * \note This is a moveable type, not copyable.
 * 
 * \since
 * v.5.5.17
 */
template< std::size_t Cases_Count >
class prepared_select_t
	{
		//! Parameters for select.
		mchain_select_params_t m_params;

		//! Cases for select.
		mchain_props::details::select_cases_holder_t< Cases_Count > m_cases_holder;

	public :
		prepared_select_t( const prepared_select_t & ) = delete;
		prepared_select_t &
		operator=( const prepared_select_t & ) = delete;

		//! Initializing constructor.
		template< typename... Cases >
		prepared_select_t(
			mchain_select_params_t params,
			Cases &&... cases )
			:	m_params( std::move(params) )
			{
				static_assert( sizeof...(Cases) == Cases_Count,
						"Cases_Count and sizeof...(Cases) mismatch" );

				mchain_props::details::fill_select_cases_holder(
						m_cases_holder, 0u, std::forward<Cases>(cases)... );
			}

		//! Move constructor.
		prepared_select_t(
			prepared_select_t && other )
			:	m_params( std::move(other.m_params) )
			,	m_cases_holder( std::move(other.m_cases_holder) )
			{}

		//! Move operator.
		prepared_select_t &
		operator=( prepared_select_t && other ) SO_5_NOEXCEPT
			{
				prepared_select_t tmp( std::move(other) );
				this->swap(tmp);
				return *this;
			}

		//! Swap operation.
		void
		swap( prepared_select_t & o ) SO_5_NOEXCEPT
			{
				std::swap( o.m_params, o.m_params );
				m_cases_holder.swap( o.m_cases_holder );
			}

		/*!
		 * \name Getters
		 * \{ 
		 */
		const mchain_select_params_t &
		params() const { return m_params; }

		const mchain_props::details::select_cases_holder_t< Cases_Count > &
		cases() const { return m_cases_holder; }
		/*!
		 * \}
		 */
	};

//
// prepare_select
//
/*!
 * \brief Create prepared select statement to be used later.
 *
 * Accepts all parameters as advanced select() version. For example:
 * \code
	// Receive and handle 3 messages.
	// If there is no 3 messages in chains the select will wait
	// no more that 200ms.
	// A return from select will be after handling of 3 messages or
	// if all mchains are closed explicitely, or if there is no messages
	// for more than 200ms.
	auto prepared1 = prepare_select(
		so_5::from_all().handle_n( 3 ).empty_timeout( milliseconds(200) ),
		case_( ch1,
				[]( const first_message_type & msg ) { ... },
				[]( const second_message_type & msg ) { ... } ),
		case_( ch2,
				[]( const third_message_type & msg ) { ... },
				handler< some_signal_type >( []{ ... ] ),
				... ) );

	// Receive all messages from mchains.
	// If there is no message in any of mchains then wait no more than 500ms.
	// A return from select will be after explicit close of all mchains
	// or if there is no messages for more than 500ms.
	auto prepared2 = prepare_select(
		so_5::from_all().empty_timeout( milliseconds(500) ),
		case_( ch1,
				[]( const first_message_type & msg ) { ... },
				[]( const second_message_type & msg ) { ... } ),
		case_( ch2,
				[]( const third_message_type & msg ) { ... },
				handler< some_signal_type >( []{ ... ] ),
				... ) );
 * \endcode
 *
 * \since
 * v.5.5.17
 */
template< typename... Cases >
prepared_select_t< sizeof...(Cases) >
prepare_select(
	//! Parameters for advanced select.
	const mchain_select_params_t & params,
	//! Select cases.
	Cases &&... cases )
	{
		return prepared_select_t< sizeof...(Cases) >(
				params,
				std::forward<Cases>(cases)... );
	}

/*!
 * \brief A select operation to be done on previously prepared select params.
 *
 * Usage of ordinary forms of select() functions inside loops could be
 * inefficient because of wasting resources on constructions of internal
 * objects with descriptions of select cases on each select() call.  More
 * efficient way is preparation of all select params and reusing them later. A
 * combination of so_5::prepare_select() and so_5::select(prepared_select_t)
 * allows to do that.
 *
 * Usage example:
 * \code
	auto prepared = so_5::prepare_select(
		so_5::from_all().extract_n(10).empty_timeout(200ms),
		case_( ch1, some_handlers... ),
		case_( ch2, more_handlers... ),
		case_( ch3, yet_more_handlers... ) );
	...
	while( !some_condition )
	{
		auto r = so_5::select( prepared );
		...
	}
 * \endcode
 *
 * \since
 * v.5.5.17
 */
template< std::size_t Cases_Count >
mchain_receive_result_t
select(
	const prepared_select_t< Cases_Count > & prepared )
	{
		return mchain_props::details::perform_select(
				prepared.params(),
				prepared.cases() );
	}

} /* namespace so_5 */

