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

#include <so_5/mchain_select_ifaces.hpp>

#include <so_5/details/at_scope_exit.hpp>
#include <so_5/details/invoke_noexcept_code.hpp>
#include <so_5/details/remaining_time_counter.hpp>

#include <iterator>
#include <array>

namespace so_5 {

namespace mchain_props {

namespace details {

//
// adv_select_data_t
//
//NOTE: there is no any additional data for select() function.
using adv_select_data_t = bulk_processing_basic_data_t;

} /* namespace details */

} /* namespace mchain_props */

//
// mchain_select_result_t
//
/*!
 * \since
 * v.5.7.0
 *
 * \brief A result of select from several mchains.
 */
class mchain_select_result_t
	{
		//! Count of extracted incoming messages.
		std::size_t m_extracted;
		//! Count of handled incoming messages.
		std::size_t m_handled;
		//! Count of messages sent.
		std::size_t m_sent;
		//! Count of closed chains.
		std::size_t m_closed;

	public :
		//! Default constructor.
		mchain_select_result_t() noexcept
			:	m_extracted{ 0u }
			,	m_handled{ 0u }
			,	m_sent{ 0u }
			,	m_closed{ 0u }
			{}

		//! Initializing constructor.
		mchain_select_result_t(
			//! Count of extracted incoming messages.
			std::size_t extracted,
			//! Count of handled incoming messages.
			std::size_t handled,
			//! Count of messages sent.
			std::size_t sent,
			//! Count of closed chains.
			std::size_t closed ) noexcept
			:	m_extracted{ extracted }
			,	m_handled{ handled }
			,	m_sent{ sent }
			,	m_closed{ closed }
			{}

		//! Count of extracted incoming messages.
		[[nodiscard]]
		std::size_t
		extracted() const noexcept { return m_extracted; }

		//! Count of handled incoming messages.
		[[nodiscard]]
		std::size_t
		handled() const noexcept { return m_handled; }

		//! Count of messages sent.
		[[nodiscard]]
		std::size_t
		sent() const noexcept { return m_sent; }

		//! Count of closed chains.
		[[nodiscard]]
		std::size_t
		closed() const noexcept { return m_closed; }

		/*!
		 * \return true if extracted() is not 0.
		 */
		[[nodiscard]]
		bool
		was_extracted() const noexcept { return 0u != m_extracted; }

		/*!
		 * \return true if handled() is not 0.
		 */
		[[nodiscard]]
		bool
		was_handled() const noexcept { return 0u != m_handled; }

		/*!
		 * \return true if sent() is not 0.
		 */
		[[nodiscard]]
		bool
		was_sent() const noexcept { return 0u != m_sent; }

		/*!
		 * \return true if closed() is not 0.
		 */
		[[nodiscard]]
		bool
		was_closed() const noexcept { return 0u != m_closed; }

//FIXME: should this method take closed() into the account?
		/*!
		 * \return true if nothing happened (no extracted messages, no
		 * handled messages, no sent messages).
		 */
		[[nodiscard]]
		bool
		is_nothing_happened() const noexcept
			{
				return !was_extracted() && !was_handled() && !was_sent();
			}
	};

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
template< mchain_props::msg_count_status_t Msg_Count_Status >
class mchain_select_params_t final
	: public mchain_bulk_processing_params_t<
	  		mchain_props::details::adv_select_data_t,
	  		mchain_select_params_t< Msg_Count_Status > >
	{
		using base_type = mchain_bulk_processing_params_t<
				mchain_props::details::adv_select_data_t,
				mchain_select_params_t< Msg_Count_Status > >;

	public :
		//! Make of clone with different Msg_Count_Status or return
		//! a reference to the same object.
		template< mchain_props::msg_count_status_t New_Msg_Count_Status >
		SO_5_NODISCARD
		decltype(auto)
		so5_clone_if_necessary() noexcept
			{
				if constexpr( New_Msg_Count_Status != Msg_Count_Status )
					return mchain_select_params_t< New_Msg_Count_Status >{
							this->so5_data()
						};
				else
					return *this;
			}

		//! The default constructor.
		mchain_select_params_t() = default;

		//! Initializing constructor for the case of cloning.
		mchain_select_params_t(
			typename base_type::data_type data )
			:	base_type{ std::move(data) }
			{}
	};

//
// from_all
//
/*!
 * \brief Helper function for creation of mchain_select_params instance
 * with default values.
 *
 * \attention
 * Since v.5.6.0 at least handle_all(), handle_n() or extract_n() should be
 * called before passing result of from_all() to select() or
 * prepare_select() functions.
 *
 * Usage example:
 * \code
	select( so_5::from_all().handle_n(3).empty_timeout(3s), ... );
 * \endcode
 */
inline mchain_select_params_t< mchain_props::msg_count_status_t::undefined >
from_all()
	{
		return {};
	}

namespace mchain_props {

namespace details {

//
// receive_select_case_t
//
//FIXME: document this!
/*!
 *
 * \since
 * v.5.7.0
 */
class receive_select_case_t : public select_case_t
	{
	public :
		using select_case_t::select_case_t;

		[[nodiscard]]
		handling_result_t
		try_handle( select_notificator_t & notificator ) override
			{
				// Please note that value of m_notificator will be
				// returned to nullptr if a message extracted or
				// channel is closed.
				m_notificator = &notificator;

//FIXME: which value should have m_notificator if extract(demand) throws?
				demand_t demand;
				const auto status = extract( demand );
				// Notificator pointer must retain its value only if
				// there is no messages in mchain.
				// In other cases this pointer must be dropped.
				if( extraction_status_t::no_messages != status )
					m_notificator = nullptr;

				if( extraction_status_t::msg_extracted == status )
					return try_handle_extracted_message( demand );

				return mchain_receive_result_t{ 0u, 0u, status };
			}

	protected :
		//! Attempt to handle extracted message.
		/*!
		 * This method will be overriden in derived classes.
		 */
		[[nodiscard]]
		virtual mchain_receive_result_t
		try_handle_extracted_message( demand_t & demand ) = 0;
	};

//
// send_select_case_t
//
//FIXME: document this!
/*!
 *
 * \since
 * v.5.7.0
 */
class send_select_case_t : public select_case_t
	{
	private :
		//! Type of message to be sent.
		std::type_index m_msg_type;
		//! Message to be sent.
		message_ref_t m_message;

	public :
		//! Initializing constructor.
		send_select_case_t(
			mchain_t chain,
			std::type_index msg_type,
			message_ref_t message )
			:	select_case_t{ std::move(chain) }
			,	m_msg_type{ std::move(msg_type) }
			,	m_message{ std::move(message) }
			{}

		[[nodiscard]]
		handling_result_t
		try_handle( select_notificator_t & notificator ) override
			{
				// Please note that value of m_notificator will be
				// returned to nullptr if a message stored into the mchain
				// or channel is closed.
				m_notificator = &notificator;

//FIXME: which value should have m_notificator if push() throws?
				const auto status = push( m_msg_type, m_message );
				// Notificator pointer must retain its value only if
				// message is deffered.
				// In other cases this pointer must be dropped.
				if( push_status_t::deffered != status )
					m_notificator = nullptr;

				if( push_status_t::stored == status )
					on_successful_push();

				return mchain_send_result_t{
						push_status_t::stored == status ? 1u : 0u,
						status
					};
			}

	protected :
		//! Hook for handling successful push attempt.
		/*!
		 * This method will be overriden in derived classes.
		 */
		virtual void
		on_successful_push() = 0;
	};

//
// actual_receive_select_case_t
//
/*!
 * \brief Actual implementation of one multi chain select case.
 *
 * \since
 * v.5.5.16
 */
template< std::size_t N >
class actual_receive_select_case_t : public receive_select_case_t
	{
		so_5::details::handlers_bunch_t< N > m_handlers;

	public :
		//! Initializing constructor.
		/*!
		 * \tparam Handlers list of message handlers for messages extracted
		 * from the mchain.
		 */
		template< typename... Handlers >
		actual_receive_select_case_t(
			//! Chain to be used for select.
			mchain_t chain,
			//! Message handlers.
			Handlers &&... handlers )
			:	receive_select_case_t( std::move(chain) )
			{
				so_5::details::fill_handlers_bunch(
						m_handlers,
						0,
						std::forward< Handlers >(handlers)... );
			}

	protected :
		[[nodiscard]]
		mchain_receive_result_t
		try_handle_extracted_message( demand_t & demand ) override
			{
				const bool handled = m_handlers.handle(
						demand.m_msg_type,
						demand.m_message_ref );

				return mchain_receive_result_t{
						1u,
						handled ? 1u : 0u,
						extraction_status_t::msg_extracted };
			}
	};

//
// actual_send_select_case_t
//
//FIXME: document this!
/*!
 *
 * \since
 * v.5.7.0
 */
template< typename On_Success_Handler >
class actual_send_select_case_t : public send_select_case_t
	{
	private :
		//! Actual handler of successful send attempt.
		On_Success_Handler m_success_handler;

	public :
		//! Initializing constructor for the case when success_handler is a const lvalue
		actual_send_select_case_t(
			mchain_t chain,
			std::type_index msg_type,
			message_ref_t message,
			const On_Success_Handler & success_handler )
			:	send_select_case_t{
					std::move(chain), std::move(msg_type), std::move(message) }
			,	m_success_handler{ success_handler }
			{}

		//! Initializing constructor for the case when success_handler is a rvalue.
		actual_send_select_case_t(
			mchain_t chain,
			std::type_index msg_type,
			message_ref_t message,
			On_Success_Handler && success_handler )
			:	send_select_case_t{
					std::move(chain), std::move(msg_type), std::move(message) }
			,	m_success_handler{ std::move(success_handler) }
			{}

	protected :
		void
		on_successful_push() override
			{
				m_success_handler();
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
		select_cases_holder_t( select_cases_holder_t && o ) noexcept
			{
				swap( o );
			}

		//! Move operator.
		select_cases_holder_t &
		operator=( select_cases_holder_t && o ) noexcept
			{
				select_cases_holder_t tmp( std::move( o ) );
				swap( tmp );

				return *this;
			}

		//! Swap operation.
		void
		swap( select_cases_holder_t & o ) noexcept
			{
				for( std::size_t i = 0; i != Cases_Count; ++i )
					m_cases[ i ] = std::move(o.m_cases[ i ]);
			}

		//! Helper method for setting up specific select_case.
		/*!
		 * This method will be used during creation of select_cases_holder.
		 */
		void
		set_case( std::size_t index, select_case_unique_ptr_t c ) noexcept
			{
				m_cases[ index ] = std::move(c);
			}

		//! Get count of select_cases in holder.
		[[nodiscard]]
		std::size_t
		size() const noexcept { return Cases_Count; }

		//! Iterator class for accessing select_cases.
		/*!
		 * Implements ForwardIterator concept.
		 */
		class const_iterator
			{
				using actual_it_t = typename array_type_t::const_iterator;

				actual_it_t m_it;

			public :
				using difference_type = std::ptrdiff_t;
				using value_type = select_case_t;
				using pointer = const value_type*;
				using reference = const value_type&;
				using iterator_category = std::forward_iterator_tag;

				const_iterator() = default;
				const_iterator( actual_it_t it ) noexcept : m_it( std::move(it) ) {}

				const_iterator & operator++() noexcept
					{ ++m_it; return *this; }
				const_iterator operator++(int) noexcept
					{ const_iterator o{ m_it }; ++m_it; return o; }

				bool operator==( const const_iterator & o ) const noexcept
					{ return m_it == o.m_it; }
				bool operator!=( const const_iterator & o ) const noexcept
					{ return m_it != o.m_it; }

				select_case_t & operator*() const noexcept
					{ return **m_it; }
				select_case_t * operator->() const noexcept
					{ return m_it->get(); }
			};

		//! Get iterator for the first item in select_cases_holder.
		[[nodiscard]]
		const_iterator
		begin() const noexcept { return const_iterator{ m_cases.begin() }; }

		//! Get iterator for the item just behind the last item in select_cases_holder.
		[[nodiscard]]
		const_iterator
		end() const noexcept { return const_iterator{ m_cases.end() }; }
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

/*!
 * \brief The current status of prepared-select instance.
 *
 * If prepared-select instance is activated (is used in select() call)
 * then this instance can't be activated yet more time.
 *
 * \since
 * v.5.6.1
 */
enum class prepared_select_status_t
	{
		//! Prepared-select instance is not used in select() call.
		passive,
		//! Prepared-select instance is used in select() call now.
		active
	};

/*!
 * \brief A data for prepared-select instance.
 *
 * \note
 * This data is protected by mutex. To get access to data it is
 * necessary to use instances activation_locker_t class.
 *
 * \attention
 * This class is not Moveable nor Copyable.
 *
 * \since
 * v.5.6.1
 */
template< std::size_t Cases_Count >
class prepared_select_data_t
	{
		//! The object's lock.
		std::mutex m_lock;

		//! The current status of extensible-select object.
		prepared_select_status_t m_status{
				prepared_select_status_t::passive };

		//! Parameters for select.
		const mchain_select_params_t<
				mchain_props::msg_count_status_t::defined > m_params;

		//! A list of cases for extensible-select operation.
		select_cases_holder_t< Cases_Count > m_cases;

	public :
		prepared_select_data_t(
			const prepared_select_data_t & ) = delete;
		prepared_select_data_t(
			prepared_select_data_t && ) = delete;

		//! Initializing constructor.
		template< typename... Cases >
		prepared_select_data_t(
			mchain_select_params_t<
					msg_count_status_t::defined > && params,
			Cases && ...cases ) noexcept
			:	m_params{ std::move(params) }
			{
				static_assert( sizeof...(Cases) == Cases_Count,
						"Cases_Count and sizeof...(Cases) mismatch" );

				fill_select_cases_holder(
						m_cases, 0u, std::forward<Cases>(cases)... );
			}

		friend class activation_locker_t;

		/*!
		 * \brief Special class for locking prepared-select instance
		 * for activation inside select() call.
		 *
		 * This class acquires prepared-select instance's mutex for a
		 * short time twice:
		 * 
		 * - the first time in the constructor to check the status of
		 *   prepared-select instance and switch status to `active`;
		 * - the second time in the destructor to return status to
		 *   `passive`.
		 *
		 * This logic allow an instance of activation_locker_t live for
		 * long time but not to block other instances of
		 * activation_locker_t.
		 *
		 * \attention
		 * The constructor of activation_locker_t throws if prepared-select
		 * instance is used in select() call.
		 *
		 * \note
		 * This class is not Moveable nor Copyable.
		 *
		 * \since
		 * v.5.6.1
		 */
		class activation_locker_t
			{
				outliving_reference_t< prepared_select_data_t > m_data;

			public :
				activation_locker_t(
					const activation_locker_t & ) = delete;
				activation_locker_t(
					activation_locker_t && ) = delete;

				activation_locker_t(
					outliving_reference_t< prepared_select_data_t > data )
					:	m_data{ data }
					{
						// Lock the data object only for changing the status.
						std::lock_guard< std::mutex > lock{ m_data.get().m_lock };

						if( prepared_select_status_t::active ==
								m_data.get().m_status )
							SO_5_THROW_EXCEPTION( rc_prepared_select_is_active_now,
									"an activate prepared-select "
									"that is already active" );

						m_data.get().m_status = prepared_select_status_t::active;
					}

				~activation_locker_t() noexcept
					{
						// Lock the data object only for changing the status.
						std::lock_guard< std::mutex > lock{ m_data.get().m_lock };

						m_data.get().m_status = prepared_select_status_t::passive;
					}

				const auto &
				params() const noexcept
					{
						return m_data.get().m_params;
					}

				const auto &
				cases() const noexcept
					{
						return m_data.get().m_cases;
					}
			};
	};

//
// extensible_select_cases_holder_t
//
/*!
 * \brief A holder for serie of select_cases for the case of extensible select.
 *
 * Provides access to select_cases via iterator and begin() and end() methods.
 *
 * \note This is moveable class, but not copyable.
 *
 * \since
 * v.5.6.1
 */
class extensible_select_cases_holder_t
	{
		//! Type of array for holding select_cases.
		using array_type_t = std::vector< select_case_unique_ptr_t >;
		//! Storage for select_cases.
		array_type_t m_cases;

	public :
		extensible_select_cases_holder_t(
			const extensible_select_cases_holder_t & ) = delete;
		extensible_select_cases_holder_t &
		operator=(
			const extensible_select_cases_holder_t & ) = delete;

		//! Swap operation.
		friend void
		swap(
			extensible_select_cases_holder_t & a,
			extensible_select_cases_holder_t & b ) noexcept
			{
				using std::swap;
				swap( a.m_cases, b.m_cases );
			}

		//! Default constructor.
		extensible_select_cases_holder_t()
			{}
		//! Constructor with initial capacity.
		extensible_select_cases_holder_t(
			std::size_t initial_capacity )
			{
				if( 0u != initial_capacity )
					m_cases.reserve( initial_capacity );
			}
		//! Move constructor.
		extensible_select_cases_holder_t(
			extensible_select_cases_holder_t && o ) noexcept
			:	m_cases{ std::move(o.m_cases) }
			{}

		//! Move operator.
		extensible_select_cases_holder_t &
		operator=( extensible_select_cases_holder_t && o ) noexcept
			{
				extensible_select_cases_holder_t tmp( std::move( o ) );
				swap( *this, tmp );

				return *this;
			}

		//! Helper method for setting up specific select_case.
		/*!
		 * This method will be used during creation of select_cases_holder.
		 */
		void
		add_case( select_case_unique_ptr_t c )
			{
				m_cases.push_back( std::move(c) );
			}

		//! Get count of select_cases in holder.
		std::size_t
		size() const noexcept { return m_cases.size(); }

		//! Iterator class for accessing select_cases.
		/*!
		 * Implements ForwardIterator concept.
		 */
		class const_iterator
			{
				using actual_it_t = typename array_type_t::const_iterator;

				actual_it_t m_it;

			public :
				using difference_type = std::ptrdiff_t;
				using value_type = select_case_t;
				using pointer = const value_type*;
				using reference = const value_type&;
				using iterator_category = std::forward_iterator_tag;

				const_iterator() = default;
				const_iterator( actual_it_t it ) noexcept : m_it( std::move(it) ) {}

				const_iterator & operator++() noexcept
					{ ++m_it; return *this; }
				const_iterator operator++(int) noexcept
					{ const_iterator o{ m_it }; ++m_it; return o; }

				bool operator==( const const_iterator & o ) const noexcept
					{ return m_it == o.m_it; }
				bool operator!=( const const_iterator & o ) const noexcept
					{ return m_it != o.m_it; }

				select_case_t & operator*() const noexcept
					{ return **m_it; }
				select_case_t * operator->() const noexcept
					{ return m_it->get(); }
			};

		//! Get iterator for the first item in select_cases_holder.
		const_iterator
		begin() const noexcept { return const_iterator{ m_cases.begin() }; }

		//! Get iterator for the item just behind the last item in select_cases_holder.
		const_iterator
		end() const noexcept { return const_iterator{ m_cases.end() }; }
	};

inline void
fill_select_cases_holder(
	extensible_select_cases_holder_t & /*holder*/ )
	{}

template< typename... Cases >
void
fill_select_cases_holder(
	extensible_select_cases_holder_t & holder,
	select_case_unique_ptr_t c,
	Cases &&... other_cases )
	{
		holder.add_case( std::move(c) );
		if constexpr( 0u != sizeof...(other_cases) )
			fill_select_cases_holder(
					holder,
					std::forward<Cases>(other_cases)... );
	}

/*!
 * \brief The current status of extensible-select instance.
 *
 * If extensible-select instance is activated (is used in select() call)
 * then this instance can't be modified or activated yet more time.
 *
 * \since
 * v.5.6.1
 */
enum class extensible_select_status_t
	{
		//! Extensible-select instance is not used in select() call.
		passive,
		//! Extensible-select instance is used in select() call now.
		active
	};

/*!
 * \brief A data for extensible-select instance.
 *
 * \note
 * This data is protected by mutex. To get access to data it is
 * necessary to use instances of modification_locker_t and
 * activation_locker_t classes.
 *
 * \attention
 * This class is not Moveable nor Copyable.
 *
 * \since
 * v.5.6.1
 */
class extensible_select_data_t
	{
		//! The object's lock.
		std::mutex m_lock;

		//! The current status of extensible-select object.
		extensible_select_status_t m_status{
				extensible_select_status_t::passive };

		//! Parameters for select.
		const mchain_select_params_t<
				mchain_props::msg_count_status_t::defined > m_params;

		//! A list of cases for extensible-select operation.
		extensible_select_cases_holder_t m_cases;

	public :
		extensible_select_data_t(
			const extensible_select_data_t & ) = delete;
		extensible_select_data_t(
			extensible_select_data_t && ) = delete;

		//! Initializing constructor.
		extensible_select_data_t(
			mchain_select_params_t<
					msg_count_status_t::defined > && params,
			extensible_select_cases_holder_t && cases ) noexcept
			:	m_params{ std::move(params) }
			,	m_cases{ std::move(cases) }
			{}

		friend class modification_locker_t;

		/*!
		 * \brief Special class for locking extensible-select instance
		 * for modification.
		 *
		 * Acquires extensible-select instance's in the constructor and
		 * releases in the destructor.
		 *
		 * It is possible to have several modification_locker_t instances
		 * for one extensible-select instance in different threads at
		 * the same time. All of them except one will be blocked on
		 * extensible-select instance's mutex.
		 *
		 * \attention
		 * The constructor of modification_locker_t throws if extensible-select
		 * instance is used in select() call.
		 *
		 * \note
		 * This class is not Moveable nor Copyable.
		 *
		 * \since
		 * v.5.6.1
		 */
		class modification_locker_t
			{
				outliving_reference_t< extensible_select_data_t > m_data;
				std::lock_guard< std::mutex > m_lock;

			public :
				modification_locker_t(
					const modification_locker_t & ) = delete;
				modification_locker_t(
					modification_locker_t && ) = delete;

				modification_locker_t(
					outliving_reference_t< extensible_select_data_t > data )
					:	m_data{ data }
					,	m_lock{ m_data.get().m_lock }
					{
						if( extensible_select_status_t::active ==
								m_data.get().m_status )
							SO_5_THROW_EXCEPTION( rc_extensible_select_is_active_now,
									"an attempt to modify extensible-select "
									"that is already active" );
					}

				//! Get access to cases-holder for modification.
				auto &
				cases() const noexcept
					{
						return m_data.get().m_cases;
					}
			};

		friend class activation_locker_t;

		/*!
		 * \brief Special class for locking extensible-select instance
		 * for activation inside select() call.
		 *
		 * This class acquires extensible-select instance's mutex for a
		 * short time twice:
		 * 
		 * - the first time in the constructor to check the status of
		 *   extensible-select instance and switch status to `active`;
		 * - the second time in the destructor to return status to
		 *   `passive`.
		 *
		 * This logic allow an instance of activation_locker_t live for
		 * long time but not to block other instances of
		 * modification_locker_t or activation_locker_t.
		 *
		 * \attention
		 * The constructor of activation_locker_t throws if extensible-select
		 * instance is used in select() call.
		 *
		 * \note
		 * This class is not Moveable nor Copyable.
		 *
		 * \since
		 * v.5.6.1
		 */
		class activation_locker_t
			{
				outliving_reference_t< extensible_select_data_t > m_data;

			public :
				activation_locker_t(
					const activation_locker_t & ) = delete;
				activation_locker_t(
					activation_locker_t && ) = delete;

				activation_locker_t(
					outliving_reference_t< extensible_select_data_t > data )
					:	m_data{ data }
					{
						// Lock the data object only for changing the status.
						std::lock_guard< std::mutex > lock{ m_data.get().m_lock };

						if( extensible_select_status_t::active ==
								m_data.get().m_status )
							SO_5_THROW_EXCEPTION( rc_extensible_select_is_active_now,
									"an activate extensible-select "
									"that is already active" );

						m_data.get().m_status = extensible_select_status_t::active;
					}

				~activation_locker_t() noexcept
					{
						// Lock the data object only for changing the status.
						std::lock_guard< std::mutex > lock{ m_data.get().m_lock };

						m_data.get().m_status = extensible_select_status_t::passive;
					}

				const auto &
				params() const noexcept
					{
						return m_data.get().m_params;
					}

				const auto &
				cases() const noexcept
					{
						return m_data.get().m_cases;
					}
			};
	};

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
		push_to_notified_chain( select_case_t & what ) noexcept
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

		void
		notify( select_case_t & what ) noexcept override
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
		return_to_ready_chain( select_case_t & what ) noexcept
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
		[[nodiscard]]
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

//
// successful_send_attempt_info_t
//
//FIXME: document this!
struct successful_send_attempt_info_t
	{
		std::size_t m_sent_messages;
	};

//
// failed_send_attempt_info_t
//
//FIXME: document this!
struct failed_send_attempt_info_t
	{
		push_status_t m_status;
	};

//
// unknown_send_attempt_info_t
//
//FIXME: document this!
struct unknown_send_attempt_info_t {};

//
// send_attempt_result_t
//
//FIXME: document this!
using send_attempt_result_t = std::variant<
		unknown_send_attempt_info_t,
		successful_send_attempt_info_t,
		failed_send_attempt_info_t >;

//
// can_select_be_continued
//
//FIXME: document this!
[[nodiscard]]
inline bool
can_select_be_continued( const send_attempt_result_t & result ) noexcept
	{
		struct visitor_t
			{
				[[nodiscard]] bool
				operator()( const unknown_send_attempt_info_t & ) const noexcept
					{
						return true;
					}

				[[nodiscard]] bool
				operator()( const successful_send_attempt_info_t & ) const noexcept
					{
						return false;
					}

				[[nodiscard]] bool
				operator()( const failed_send_attempt_info_t & ) const noexcept
					{
						return false;
					}
			};

		return std::visit( visitor_t{}, result );
	}

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
		const mchain_select_params_t< msg_count_status_t::defined > & m_params;

		const Holder & m_select_cases;
		actual_select_notificator_t m_notificator;

		std::size_t m_closed_chains = 0;
		std::size_t m_extracted_messages = 0;
		std::size_t m_handled_messages = 0;

		send_attempt_result_t m_send_result{ unknown_send_attempt_info_t{} };

		extraction_status_t m_last_extraction_status =
				extraction_status_t::no_messages;

		bool m_can_continue = true;

	public :
		select_actions_performer_t(
			const mchain_select_params_t< msg_count_status_t::defined > & params,
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
						m_last_extraction_status = extraction_status_t::no_messages;
						update_can_continue_flag();
					}
				else
					handle_ready_chain( ready_chain );
			}

		extraction_status_t
		last_extraction_status() const noexcept { return m_last_extraction_status; }

		bool
		can_continue() const noexcept { return m_can_continue; }

		mchain_select_result_t
		make_result() const noexcept
			{
				return {
						m_extracted_messages,
						m_handled_messages,
						detect_sent_messages_count(),
						m_closed_chains,
					};
			}

	private :
		friend struct select_result_handler_t;
		struct select_result_handler_t final
			{
				select_actions_performer_t * m_performer;
				select_case_t * m_current;

				void operator()( const mchain_receive_result_t & result ) const {
					m_performer->on_receive_result( m_current, result );
				}

				void operator()( const mchain_send_result_t & result ) const {
					m_performer->on_send_result( m_current, result );
				}
			};

		void
		handle_ready_chain( select_case_t * ready_chain )
			{
				while( ready_chain && m_can_continue )
					{
						auto * current = ready_chain;
						ready_chain = current->giveout_next();

						std::visit(
								select_result_handler_t{ this, current },
								current->try_handle( m_notificator ) );

						update_can_continue_flag();
					}
			}

		void
		on_receive_result(
			select_case_t * current,
			const mchain_receive_result_t & result )
			{
				m_last_extraction_status = result.status();

				if( extraction_status_t::msg_extracted == result.status() )
					{
						m_extracted_messages += result.extracted();
						m_handled_messages += result.handled();

						// The mchain from 'current' can contain more
						// messages. We should return this case to 'ready_chain'
						// of the notificator.
						m_notificator.return_to_ready_chain( *current );
					}
				else if( extraction_status_t::chain_closed == result.status() )
					{
						react_on_closed_chain( current );
					}
			}

		void
		on_send_result(
			select_case_t * current,
			const mchain_send_result_t & result )
			{
				// No extracted messages for that case.
				m_last_extraction_status = extraction_status_t::no_messages;

				switch( result.status() )
					{
					case push_status_t::stored :
						m_send_result = successful_send_attempt_info_t{ result.sent() };
					break;

					case push_status_t::deffered :
						// Nothing to do. Another attempt could be performed later.
					break;

					case push_status_t::not_stored :
						m_send_result = failed_send_attempt_info_t{ result.status() };
					break;

					case push_status_t::chain_closed :
						m_send_result = failed_send_attempt_info_t{ result.status() };
						react_on_closed_chain( current );
					break;
					}
			}

		void
		react_on_closed_chain( select_case_t * current )
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

					return can_select_be_continued( m_send_result );
				};

				m_can_continue = fn();
			}

		[[nodiscard]]
		std::size_t
		detect_sent_messages_count() const noexcept
			{
				if( const auto * p = std::get_if< successful_send_attempt_info_t >(
						&m_send_result ) )
					return p->m_sent_messages;
				else
					return 0u;
			}
	};

template< typename Holder >
mchain_select_result_t
do_adv_select_with_total_time(
	const mchain_select_params_t< msg_count_status_t::defined > & params,
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
mchain_select_result_t
do_adv_select_without_total_time(
	const mchain_select_params_t< msg_count_status_t::defined > & params,
	const Holder & select_cases )
	{
		using namespace so_5::details;

		select_actions_performer_t< Holder > performer{ params, select_cases };

		remaining_time_counter_t wait_time{ params.empty_timeout() };
		do
			{
				performer.handle_next( wait_time.remaining() );
				if( extraction_status_t::msg_extracted ==
						performer.last_extraction_status() )
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
mchain_select_result_t
perform_select(
	//! Parameters for advanced select.
	const mchain_select_params_t< msg_count_status_t::defined > & params,
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
// receive_case
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
[[nodiscard]]
mchain_props::select_case_unique_ptr_t
receive_case(
	//! Message chain to be used in select.
	mchain_t chain,
	//! Message handlers for messages extracted from that chain.
	Handlers &&... handlers )
	{
		using namespace mchain_props;
		using namespace mchain_props::details;

		return select_case_unique_ptr_t{
				new actual_receive_select_case_t< sizeof...(handlers) >{
						std::move(chain),
						std::forward< Handlers >(handlers)... } };
	}

//
// send_case
//
/*!
 * \brief A helper for creation of select_case object for one send-case
 * of a multi chain select.
 *
 * \sa so_5::select()
 *
 * \since
 * v.5.7.0
 */
template<
	typename Msg,
	message_ownership_t Ownership,
	typename On_Success_Handler >
[[nodiscard]]
mchain_props::select_case_unique_ptr_t
send_case(
	//! Message chain to be used in select.
	mchain_t chain,
	//! Message instance to be sent.
	message_holder_t< Msg, Ownership > msg,
	On_Success_Handler && handler )
	{
		using namespace mchain_props;
		using namespace mchain_props::details;

		using actual_handler_type = std::decay_t<On_Success_Handler>;
		using select_case_type = actual_send_select_case_t<actual_handler_type>;

		return select_case_unique_ptr_t{
				new select_case_type{
						std::move(chain),
						message_payload_type<Msg>::subscription_type_index(),
						msg.make_reference(),
						std::forward< On_Success_Handler >(handler)
				}
			};
	}

/*!
 * \brief An advanced form of multi chain select.
 *
 * \attention The behaviour is not defined if a mchain is used in different
 * select_cases.
 *
 * \attention
 * Since v.5.6.0 at least handle_all(), handle_n() or extract_n() should be
 * called before passing result of from_all() to select() function.
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
		receive_case( ch1,
				[]( const first_message_type & msg ) { ... },
				[]( const second_message_type & msg ) { ... } ),
		receive_case( ch2,
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
		receive_case( ch1,
				[]( const first_message_type & msg ) { ... },
				[]( const second_message_type & msg ) { ... } ),
		receive_case( ch2,
				[]( const third_message_type & msg ) { ... },
				handler< some_signal_type >( []{ ... ] ),
				... ) );

	// Receive all messages from mchains.
	// If there is no message in any of mchains then wait no more than 500ms.
	// A return from select will be after explicit close of all mchains
	// or if there is no messages for more than 500ms.
	select( from_all().handle_all().empty_timeout( milliseconds(500) ),
		receive_case( ch1,
				[]( const first_message_type & msg ) { ... },
				[]( const second_message_type & msg ) { ... } ),
		receive_case( ch2,
				[]( const third_message_type & msg ) { ... },
				handler< some_signal_type >( []{ ... ] ),
				... ) );

	// Receve any number of messages from mchains but do waiting and
	// handling for no more than 2s.
	select( from_all().handle_all().total_time( seconds(2) ),
		receive_case( ch1,
				[]( const first_message_type & msg ) { ... },
				[]( const second_message_type & msg ) { ... } ),
		receive_case( ch2,
				[]( const third_message_type & msg ) { ... },
				handler< some_signal_type >( []{ ... ] ),
				... ) );

	// Receve 1000 messages from chains but do waiting and
	// handling for no more than 2s.
	select( from_all().extract_n( 1000 ).total_time( seconds(2) ),
		receive_case( ch1,
				[]( const first_message_type & msg ) { ... },
				[]( const second_message_type & msg ) { ... } ),
		receive_case( ch2,
				[]( const third_message_type & msg ) { ... },
				handler< some_signal_type >( []{ ... ] ),
				... ) );
	\endcode
 *
 * \since
 * v.5.5.16
 */
template<
	mchain_props::msg_count_status_t Msg_Count_Status,
	typename... Cases >
mchain_select_result_t
select(
	//! Parameters for advanced select.
	const mchain_select_params_t< Msg_Count_Status > & params,
	//! Select cases.
	Cases &&... cases )
	{
		static_assert(
				Msg_Count_Status == mchain_props::msg_count_status_t::defined,
				"message count to be processed/extracted should be defined "
				"by using handle_all()/handle_n()/extract_n() methods" );

		using namespace mchain_props;
		using namespace mchain_props::details;

		select_cases_holder_t< sizeof...(cases) > cases_holder;
		fill_select_cases_holder(
				cases_holder, 0, std::forward< Cases >(cases)... );

		return perform_select( params, cases_holder );
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
		receive_case( ch1, some_handlers... ),
		receive_case( ch2, more_handlers... ), ... );
	...
	auto r = so_5::select( prepared );
 * \endcode
 *
 * \note
 * This is a moveable type, not copyable. It is very like to unique_ptr.
 * Because of that an instance of prepared_select_t can empty. It means
 * that the actual content (e.g. prepared-select object) was moved to
 * another prepared_select_t instance. Usage of empty prepared_select_t
 * is an error and can lead to null-pointer dereference. SObjectizer doesn't
 * check the emptiness of prepared_select_t object.
 * 
 * \since
 * v.5.5.17
 */
template< std::size_t Cases_Count >
class prepared_select_t
	{
		template<
			mchain_props::msg_count_status_t Msg_Count_Status,
			typename... Cases >
		friend prepared_select_t< sizeof...(Cases) >
		prepare_select(
			mchain_select_params_t< Msg_Count_Status > params,
			Cases &&... cases );

		//! The actual prepared-select object.
		/*!
		 * \note
		 * Can be null if the actual content was moved to another
		 * prepared_select_t instance.
		 *
		 * \since
		 * v.5.6.1
		 */
		std::unique_ptr<
						mchain_props::details::prepared_select_data_t<Cases_Count> >
				m_data;

		//! Initializing constructor.
		/*!
		 * \note
		 * This constructor is private since v.5.6.1
		 */
		template< typename... Cases >
		prepared_select_t(
			mchain_select_params_t<
					mchain_props::msg_count_status_t::defined > params,
			Cases &&... cases )
			:	m_data{
					std::make_unique<
							mchain_props::details::prepared_select_data_t<Cases_Count> >(
						std::move(params),
						std::forward<Cases>(cases)... ) }
			{}

	public :
		prepared_select_t( const prepared_select_t & ) = delete;
		prepared_select_t &
		operator=( const prepared_select_t & ) = delete;

		//! Move constructor.
		prepared_select_t(
			prepared_select_t && other ) noexcept
			:	m_data( std::move(other.m_data) )
			{}

		//! Move operator.
		prepared_select_t &
		operator=( prepared_select_t && other ) noexcept
			{
				prepared_select_t tmp( std::move(other) );
				swap( *this, tmp);
				return *this;
			}

		//! Swap operation.
		friend void
		swap( prepared_select_t & a, prepared_select_t & b ) noexcept
			{
				using std::swap;
				swap( a.m_data, b.m_data );
			}

		//! Is this handle empty?
		/*!
		 * \since
		 * v.5.6.1
		 */
		bool
		empty() const noexcept { return !m_data; }

		/*!
		 * \name Getters
		 * \{ 
		 */
		auto &
		data() const noexcept { return *m_data; }
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
 * \attention
 * Since v.5.6.0 at least handle_all(), handle_n() or extract_n() should be
 * called before passing result of from_all() to prepare_select() function.
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
		receive_case( ch1,
				[]( const first_message_type & msg ) { ... },
				[]( const second_message_type & msg ) { ... } ),
		receive_case( ch2,
				[]( const third_message_type & msg ) { ... },
				handler< some_signal_type >( []{ ... ] ),
				... ) );

	// Receive all messages from mchains.
	// If there is no message in any of mchains then wait no more than 500ms.
	// A return from select will be after explicit close of all mchains
	// or if there is no messages for more than 500ms.
	auto prepared2 = prepare_select(
		so_5::from_all().handle_all().empty_timeout( milliseconds(500) ),
		receive_case( ch1,
				[]( const first_message_type & msg ) { ... },
				[]( const second_message_type & msg ) { ... } ),
		receive_case( ch2,
				[]( const third_message_type & msg ) { ... },
				handler< some_signal_type >( []{ ... ] ),
				... ) );
 * \endcode
 *
 * \since
 * v.5.5.17
 */
template<
	mchain_props::msg_count_status_t Msg_Count_Status,
	typename... Cases >
prepared_select_t< sizeof...(Cases) >
prepare_select(
	//! Parameters for advanced select.
	mchain_select_params_t< Msg_Count_Status > params,
	//! Select cases.
	Cases &&... cases )
	{
		static_assert(
				Msg_Count_Status == mchain_props::msg_count_status_t::defined,
				"message count to be processed/extracted should be defined "
				"by using handle_all()/handle_n()/extract_n() methods" );

		return prepared_select_t< sizeof...(Cases) >(
				std::move(params),
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
		receive_case( ch1, some_handlers... ),
		receive_case( ch2, more_handlers... ),
		receive_case( ch3, yet_more_handlers... ) );
	...
	while( !some_condition )
	{
		auto r = so_5::select( prepared );
		...
	}
 * \endcode
 *
 * \attention
 * Since v.5.6.1 there is a check for usage of prepared-select object
 * in parallel/nested calls to select(). If such call detected then
 * an exception is thrown.
 *
 * \since
 * v.5.5.17
 */
template< std::size_t Cases_Count >
mchain_select_result_t
select(
	const prepared_select_t< Cases_Count > & prepared )
	{
		using namespace mchain_props::details;

		typename prepared_select_data_t<Cases_Count>::activation_locker_t locker{
				outliving_mutable(prepared.data())
		};

		return mchain_props::details::perform_select(
				locker.params(),
				locker.cases() );
	}

//
// extensible_select_t
//
/*!
 * \brief Special container for holding select parameters and select cases.
 *
 * This type is a *handle* for extensible-select instance. It's like
 * unique_ptr. Just one instance of extensible_select_t owns the instance
 * of extensible-select.
 *
 * \attention
 * Because extensible_select_t is like to unique_ptr it can be in an empty
 * state (it means that there is no actual extensible-select instance behind
 * the handle). Usage of empty extensible_select_t object in call to
 * select() is an error. SObjectizer doesn't check the emptiness of
 * extensible_select_t object. An attempt to pass an empty extensible_select_t
 * to select() will lead to null-pointer dereference.
 *
 * \note
 * This is Moveable type but not Copyable.
 *
 * \since
 * v.5.6.1
 */
class extensible_select_t
	{
		template<
			mchain_props::msg_count_status_t Msg_Count_Status,
			typename... Cases >
		friend extensible_select_t
		make_extensible_select(
			mchain_select_params_t< Msg_Count_Status > params,
			Cases &&... cases );

		//! Actual data for that extensible-select.
		std::unique_ptr< mchain_props::details::extensible_select_data_t > m_data;

		//! Actual initializing constructor.
		extensible_select_t(
			std::unique_ptr< mchain_props::details::extensible_select_data_t > data )
			:	m_data{ std::move(data) }
			{}

	public :
		extensible_select_t( const extensible_select_t & ) = delete;
		extensible_select_t &
		operator=( const extensible_select_t & ) = delete;

		//! Default constructor.
		/*!
		 * \attention
		 * This constructor is intended for the cases like that:
		 * \code
		 * class some_my_data {
		 * 	so_5::extensible_select_t m_select_handle;
		 * 	...
		 * 	void on_some_stage() {
		 * 		m_select_handle = so_5::make_extensible_select(...);
		 * 		...
		 * 	}
		 * 	void on_another_stage() {
		 * 		so_5::add_select_cases(m_select_handle, ...);
		 * 		...
		 * 	}
		 * 	void on_yet_another_stage() {
		 * 		auto r = so_5::select(m_select_handle);
		 * 		...
		 * 	}
		 * 	...
		 * };
		 * \endcode
		 */
		extensible_select_t() = default;

		//! Move constructor.
		extensible_select_t(
			extensible_select_t && other ) noexcept
			:	m_data{ std::move(other.m_data) }
			{}

		//! Move operator.
		extensible_select_t &
		operator=( extensible_select_t && other ) noexcept
			{
				extensible_select_t tmp( std::move(other) );
				swap( *this, tmp);
				return *this;
			}

		//! Swap operation.
		friend void
		swap( extensible_select_t & a, extensible_select_t & b ) noexcept
			{
				using std::swap;
				swap( a.m_data, b.m_data );
			}

		//! Is this handle empty?
		bool
		empty() const noexcept { return !m_data; }

		/*!
		 * \name Getters
		 * \{ 
		 */
		auto &
		data() const noexcept { return *m_data; }
		/*!
		 * \}
		 */
	};

/*!
 * \brief Creation of extensible-select instance.
 *
 * This function creates an instance of extensible-select object that
 * can be used for subsequent calls to add_select_cases() and
 * select().
 *
 * Usage examples:
 * \code
 * // Creation of extensible-select instance with initial set of cases.
 * auto sel = so_5::make_extensible_select(
 * 	so_5::from_all().handle_n(10),
 * 	receive_case(ch1, ...),
 * 	receive_case(ch2, ...));
 *
 * // Creation of extensible-select instance without initial set of cases.
 * auto sel2 = so_5::make_extensible_select(
 * 	so_5::from_all().handle_n(20));
 * // Cases should be added later.
 * so_5::add_select_cases(sel2, receive_case(ch1, ...));
 * so_5::add_select_cases(sel2,
 * 	receive_case(ch2, ...),
 * 	receive_case(ch3, ...));
 * \endcode
 *
 * \since
 * v.5.6.1
 */
template<
	mchain_props::msg_count_status_t Msg_Count_Status,
	typename... Cases >
[[nodiscard]]
extensible_select_t
make_extensible_select(
	//! Parameters for advanced select.
	mchain_select_params_t< Msg_Count_Status > params,
	//! Select cases.
	Cases &&... cases )
	{
		static_assert(
				Msg_Count_Status == mchain_props::msg_count_status_t::defined,
				"message count to be processed/extracted should be defined "
				"by using handle_all()/handle_n()/extract_n() methods" );

		using namespace mchain_props::details;

		extensible_select_cases_holder_t cases_holder{ sizeof...(cases) };
		fill_select_cases_holder(
				cases_holder,
				std::forward<Cases>(cases)... );

		auto data = std::make_unique< extensible_select_data_t >(
				std::move(params),
				std::move(cases_holder) );

		return { std::move(data) };
	}

/*!
 * \brief Add a portion of cases to extensible-select instance.
 *
 * Usage examples:
 * \code
 * // Creation of extensible-select instance without initial set of cases.
 * auto sel2 = so_5::make_extensible_select(
 * 	so_5::from_all().handle_n(20));
 * // Cases should be added later.
 * so_5::add_select_cases(sel2, receive_case(ch1, ...));
 * so_5::add_select_cases(sel2,
 * 	receive_case(ch2, ...),
 * 	receive_case(ch3, ...));
 * \endcode
 *
 * \note
 * An attempt to call this function for extensible-select object that is
 * used in some select() call will lead to an exception.
 *
 * \attention
 * The \a extensible_select object must not be empty!
 *
 * \since
 * v.5.6.1
 */
template< typename... Cases >
void
add_select_cases(
	//! An instance of extensible-select to be extended.
	extensible_select_t & extensible_select,
	//! Select cases.
	Cases &&... cases )
	{
		using namespace mchain_props::details;

		extensible_select_data_t::modification_locker_t locker{
				outliving_mutable(extensible_select.data())
		};

		fill_select_cases_holder(
				locker.cases(),
				std::forward<Cases>(cases)... );
	}

/*!
 * \brief A select operation to be done on previously prepared
 * extensible-select object.
 *
 * Usage example:
 * \code
 * void handle_messages_from(const std::vector<so_5::mchain_t> & chains) {
 * 	auto sel = so_5::make_extensible_select(so_5::from_all().handle_all());
 *
 * 	for(auto & ch : chains)
 * 		so_5::add_select_cases(receive_case(ch, ...));
 *
 * 	auto r = so_5::select(sel);
 * 	... // Handing of the select() result.
 * }
 * \endcode
 *
 * \note
 * An attempt to call this function for extensible-select object that is
 * used in some select() call will lead to an exception.
 *
 * \attention
 * The \a extensible_select object must not be empty!
 *
 * \since
 * v.5.6.1
 */
inline mchain_select_result_t
select(
	const extensible_select_t & extensible_select )
	{
		using namespace mchain_props::details;

		extensible_select_data_t::activation_locker_t locker{
				outliving_mutable(extensible_select.data())
		};

		return mchain_props::details::perform_select(
				locker.params(),
				locker.cases() );
	}

} /* namespace so_5 */

