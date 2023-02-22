/*
	SObjectizer 5.
*/

/*!
	\file
	\brief A local mbox definition.
*/

#pragma once

#include <map>
#include <vector>

#include <so_5/types.hpp>
#include <so_5/exception.hpp>

#include <so_5/spinlocks.hpp>

#include <so_5/msg_tracing.hpp>

#include <so_5/mbox.hpp>
#include <so_5/enveloped_msg.hpp>

#include <so_5/impl/local_mbox_basic_subscription_info.hpp>

#include <so_5/impl/message_sink_ptr_compare.hpp>
#include <so_5/impl/msg_tracing_helpers.hpp>

#include <so_5/details/invoke_noexcept_code.hpp>

namespace so_5
{

namespace impl
{

namespace local_mbox_details
{

/*!
 * \since
 * v.5.5.4
 *
 * \brief An information block about one subscriber.
 */
class subscriber_info_t : public subscription_info_with_sink_t
{
public :
	//! Constructor for the case when info object is created only
	//! for searching of existing subscription info.
	subscriber_info_t( message_sink_t & subscriber )
		:	subscription_info_with_sink_t{ subscriber }
	{}

	//! Constructor for the case when subscriber info is being
	//! created during setting a delivery filter.
	subscriber_info_t(
		message_sink_t & subscriber,
		const delivery_filter_t & filter )
		:	subscription_info_with_sink_t{ subscriber, filter }
	{}

	//! Comparison uses only pointer to subscriber.
	/*!
	 * \note Since v.5.5.8 not only agent pointer, but also
	 * the priorities of the agents are used in comparision.
	 */
	bool
	operator<( const subscriber_info_t & o ) const
	{
		return special_message_sink_ptr_compare(
				this->sink_reference(),
				o.sink_reference() );
	}
};

//
// subscriber_adaptive_container_t
//
/*!
 * \since
 * v.5.5.12
 *
 * \brief A special container for holding subscriber_info objects.
 *
 * \note Uses std::vector as a storage for small amount of
 * subscriber_infos and std::map for large amount.
 */
class subscriber_adaptive_container_t
{
	struct subscriber_ptr_compare_type
		{
			[[nodiscard]]
			bool
			operator()(
				message_sink_t * a, message_sink_t * b ) const noexcept
			{
				return special_message_sink_ptr_compare( *a, *b );
			}
		};

	using vector_type = std::vector< subscriber_info_t >;
	using vector_iterator_type = vector_type::iterator;
	using const_vector_iterator_type = vector_type::const_iterator;

	using map_type = std::map<
			message_sink_t *,
			subscriber_info_t,
			subscriber_ptr_compare_type
		>;
	using map_iterator_type = map_type::iterator;
	using const_map_iterator_type = map_type::const_iterator;

	enum class storage_type { vector, map };

	// NOTE! This is just arbitrary values.
	// There were no any benchmarks to prove that those values are useful.
	struct size_limits
	{
		static constexpr const std::size_t switch_to_vector = 16;
		static constexpr const std::size_t switch_to_map = 32;
	};

	//! The current storage type to be used by container.
	storage_type m_storage = { storage_type::vector };

	//! Container for small amount of subscriber_infos.
	vector_type m_vector;
	//! Container for large amount of subscriber_infos.
	map_type m_map;

public :
	//! Iterator type.
	class iterator
	{
		friend class subscriber_adaptive_container_t;

		storage_type m_storage;
		vector_iterator_type m_it_v;
		map_iterator_type m_it_m;

	public :
		iterator( vector_iterator_type it_v )
			:	m_storage{ storage_type::vector }
			,	m_it_v{ std::move(it_v) }
			{}
		iterator( map_iterator_type it_m )
			:	m_storage{ storage_type::map }
			,	m_it_m{ std::move(it_m) }
			{}

		subscriber_info_t &
		operator*()
			{
				if( storage_type::vector == m_storage )
					return *m_it_v;
				else
					return m_it_m->second;
			}

		subscriber_info_t *
		operator->()
			{
				return &**this;
			}

		iterator &
		operator++()
			{
				if( storage_type::vector == m_storage )
					++m_it_v;
				else
					++m_it_m;

				return *this;
			}

		iterator
		operator++(int)
			{
				if( storage_type::vector == m_storage )
				{
					iterator r{ *this };
					++m_it_v;
					return r;
				}
				else
				{
					iterator r{ *this };
					++m_it_m;
					return r;
				}
			}

		bool
		operator==( const iterator & o ) const
			{
				if( storage_type::vector == m_storage )
					return m_it_v == o.m_it_v;
				else
					return m_it_m == o.m_it_m;
			}

		bool
		operator!=( const iterator & o ) const
			{
				return !( *this == o );
			}
	};

	//! Const iterator type.
	class const_iterator
	{
		storage_type m_storage;
		const_vector_iterator_type m_it_v;
		const_map_iterator_type m_it_m;

	public :
		const_iterator( const_vector_iterator_type it_v )
			:	m_storage{ storage_type::vector }
			,	m_it_v{ std::move(it_v) }
			{}
		const_iterator( const_map_iterator_type it_m )
			:	m_storage{ storage_type::map }
			,	m_it_m{ std::move(it_m) }
			{}

		const subscriber_info_t &
		operator*() const
			{
				if( storage_type::vector == m_storage )
					return *m_it_v;
				else
					return m_it_m->second;
			}

		const subscriber_info_t *
		operator->() const
			{
				return &**this;
			}

		const_iterator &
		operator++()
			{
				if( storage_type::vector == m_storage )
					++m_it_v;
				else
					++m_it_m;

				return *this;
			}

		const_iterator
		operator++(int)
			{
				if( storage_type::vector == m_storage )
				{
					const_iterator r{ *this };
					++m_it_v;
					return r;
				}
				else
				{
					const_iterator r{ *this };
					++m_it_m;
					return r;
				}
			}

		bool
		operator==( const const_iterator & o ) const
			{
				if( storage_type::vector == m_storage )
					return m_it_v == o.m_it_v;
				else
					return m_it_m == o.m_it_m;
			}

		bool
		operator!=( const const_iterator & o ) const
			{
				return !( *this == o );
			}
	};

private :
	//! Is vector used as a storage.
	bool
	is_vector() const { return storage_type::vector == m_storage; }

	//! Insertion of new item to vector.
	void
	insert_to_vector( subscriber_info_t && item )
		{
			m_vector.insert(
					std::lower_bound( m_vector.begin(), m_vector.end(), item ),
					std::move( item ) );
		}

	//! Insertion of new item to map.
	void
	insert_to_map( subscriber_info_t && item )
		{
			auto * subscriber = item.sink_pointer();
			//FIXME: can subscriber ptr be NULL here?
			m_map.emplace( subscriber, std::move( item ) );
		}

	//! Switching storage from vector to map.
	void
	switch_storage_to_map()
		{
			vector_type empty_vector;

			map_type new_storage;
			std::for_each( m_vector.begin(), m_vector.end(),
				[&new_storage]( const subscriber_info_t & info ) {
					new_storage.emplace( info.sink_pointer(), info );
				} );

			m_map.swap( new_storage );
			m_vector.swap( empty_vector );
			m_storage = storage_type::map;
		}

	//! Switching storage from map to vector.
	void
	switch_storage_to_vector()
		{
			// All exceptions will be ignored.
			// It is because an exception can be thrown on staged 1-2-3,
			// but not on stage 4.
			try
				{
					// Stage 1. New containers to swap values with old containers.
					// An exception could be thrown in the constructors.
					map_type empty_map;
					vector_type new_storage;

					// Stage 2. Preallocate necessary space in the new vector.
					// An exception can be thrown here.
					new_storage.reserve( m_map.size() );

					// Stage 3. Copy items from old map to the new vector.
					// We do not expect exceptions here at v.5.5.12, but
					// the situation can change in the future versions.

					// Use the fact that items in map is already ordered.
					std::for_each( m_map.begin(), m_map.end(),
						[&new_storage]( const map_type::value_type & info ) {
							new_storage.push_back( info.second );
						} );

					// Stage 4. Swapping.
					// No exceptions expected here.
					so_5::details::invoke_noexcept_code(
						[&]() {
							m_vector.swap( new_storage );
							m_map.swap( empty_map );
							m_storage = storage_type::vector;
						} );
				}
			catch(...)
				{}
		}

	iterator
	find_in_vector( message_sink_t & subscriber )
		{
			subscriber_info_t info{ subscriber };
			auto pos = std::lower_bound( m_vector.begin(), m_vector.end(), info );
			if( pos != m_vector.end()
					&& pos->sink_pointer() == std::addressof(subscriber) )
				return iterator{ pos };
			else
				return iterator{ m_vector.end() };
		}

	iterator
	find_in_map( message_sink_t & subscriber )
		{
			return iterator{ m_map.find( std::addressof(subscriber) ) };
		}

public :
	//! Default constructor.
	subscriber_adaptive_container_t()
		{}
	//! Copy constructor.
	subscriber_adaptive_container_t(
		const subscriber_adaptive_container_t & o )
		:	m_storage{ o.m_storage }
		,	m_vector( o.m_vector )
		,	m_map( o.m_map )
		{}
	//! Move constructor.
	subscriber_adaptive_container_t(
		subscriber_adaptive_container_t && o )
		:	m_storage{ o.m_storage }
		,	m_vector( std::move( o.m_vector ) )
		,	m_map( std::move( o.m_map ) )
		{
			// Other object is now empty.
			// It must use vector as a storage.
			o.m_storage = storage_type::vector;
		}

	void
	swap( subscriber_adaptive_container_t & o )
		{
			std::swap( m_storage, o.m_storage );
			m_vector.swap( o.m_vector );
			m_map.swap( o.m_map );
		}

	//! Copy operator.
	subscriber_adaptive_container_t &
	operator=( const subscriber_adaptive_container_t & o )
		{
			subscriber_adaptive_container_t tmp{ o };
			this->swap( tmp );
			return *this;
		}

	//! Move operator.
	subscriber_adaptive_container_t &
	operator=( const subscriber_adaptive_container_t && o )
		{
			subscriber_adaptive_container_t tmp{ std::move(o) };
			this->swap( tmp );
			return *this;
		}

	void
	insert( subscriber_info_t info )
		{
			if( is_vector() )
				{
					if( m_vector.size() == size_limits::switch_to_map )
						switch_storage_to_map();
				}

			if( is_vector() )
				insert_to_vector( std::move( info ) );
			else
				insert_to_map( std::move( info ) );
		}

	template< typename... Args >
	void
	emplace( Args &&... args )
		{
			insert( subscriber_info_t{ std::forward<Args>( args )... } );
		}

	void
	erase( const iterator & it )
		{
			if( is_vector() )
				m_vector.erase( it.m_it_v );
			else
				{
					m_map.erase( it.m_it_m );

					// May be it is a time for switching to smaller storage?
					if( m_map.size() < size_limits::switch_to_vector )
						switch_storage_to_vector();
				}
		}

	iterator
	find( message_sink_t & subscriber )
		{
			if( is_vector() )
				return find_in_vector( subscriber );
			else
				return find_in_map( subscriber );
		}

	iterator
	begin()
		{
			if( is_vector() )
				return iterator{ m_vector.begin() };
			else
				return iterator{ m_map.begin() };
		}

	iterator
	end()
		{
			if( is_vector() )
				return iterator{ m_vector.end() };
			else
				return iterator{ m_map.end() };
		}

	const_iterator
	begin() const
		{
			if( is_vector() )
				return const_iterator{ m_vector.begin() };
			else
				return const_iterator{ m_map.begin() };
		}

	const_iterator
	end() const
		{
			if( is_vector() )
				return const_iterator{ m_vector.end() };
			else
				return const_iterator{ m_map.end() };
		}

	bool
	empty() const
		{
			if( is_vector() )
				return m_vector.empty();
			else
				return m_map.empty();
		}

	std::size_t
	size() const
		{
			if( is_vector() )
				return m_vector.size();
			else
				return m_map.size();
		}
};

//
// data_t
//

/*!
 * \since
 * v.5.5.9
 *
 * \brief A coolection of data required for local mbox implementation.
 */
struct data_t
	{
		data_t( mbox_id_t id, environment_t & env )
			:	m_id{ id }
			,	m_env{ env }
			{}

		//! ID of this mbox.
		const mbox_id_t m_id;

		//! Environment for which the mbox is created.
		environment_t & m_env;

		//! Object lock.
		default_rw_spinlock_t m_lock;

		/*!
		 * \since
		 * v.5.4.0
		 *
		 * \brief Map from message type to subscribers.
		 */
		using messages_table_t = std::map<
				std::type_index,
				subscriber_adaptive_container_t >;

		//! Map of subscribers to messages.
		messages_table_t m_subscribers;
	};

} /* namespace local_mbox_details */

//
// local_mbox_template
//

//! A template with implementation of local mbox.
/*!
 * \since
 * v.5.5.9
 *
 * \tparam Tracing_Base base class with implementation of message
 * delivery tracing methods.
 */
template< typename Tracing_Base >
class local_mbox_template
	:	public abstract_message_box_t
	,	private local_mbox_details::data_t
	,	private Tracing_Base
	{
	public:
		template< typename... Tracing_Args >
		local_mbox_template(
			//! ID of this mbox.
			mbox_id_t id,
			//! Environment for which the mbox is created.
			environment_t & env,
			//! Optional parameters for Tracing_Base's constructor.
			Tracing_Args &&... args )
			:	local_mbox_details::data_t{ id, env }
			,	Tracing_Base{ std::forward< Tracing_Args >(args)... }
			{}

		mbox_id_t
		id() const override
			{
				return m_id;
			}

		void
		subscribe_event_handler(
			const std::type_index & type_wrapper,
			message_sink_t & subscriber ) override
			{
				insert_or_modify_subscriber(
						type_wrapper,
						subscriber,
						[&] {
							return local_mbox_details::subscriber_info_t{ subscriber };
						},
						[&]( local_mbox_details::subscriber_info_t & info ) {
							info.set_sink( subscriber );
						} );
			}

		void
		unsubscribe_event_handlers(
			const std::type_index & type_wrapper,
			message_sink_t & subscriber ) override
			{
				modify_and_remove_subscriber_if_needed(
						type_wrapper,
						subscriber,
						[]( local_mbox_details::subscriber_info_t & info ) {
							info.drop_sink();
						} );
			}

		std::string
		query_name() const override
			{
				std::ostringstream s;
				s << "<mbox:type=MPMC:id=" << m_id << ">";

				return s.str();
			}

		mbox_type_t
		type() const override
			{
				return mbox_type_t::multi_producer_multi_consumer;
			}

		void
		do_deliver_message(
			delivery_mode_t delivery_mode,
			const std::type_index & msg_type,
			const message_ref_t & message,
			unsigned int overlimit_reaction_deep ) override
			{
				typename Tracing_Base::deliver_op_tracer tracer{
						*this, // as Tracing_base
						*this, // as abstract_message_box_t
						"deliver_message",
						delivery_mode,
						msg_type,
						message,
						overlimit_reaction_deep };

				ensure_immutable_message( msg_type, message );

				do_deliver_message_impl(
						tracer,
						msg_type,
						message,
						overlimit_reaction_deep );
			}

		void
		set_delivery_filter(
			const std::type_index & msg_type,
			const delivery_filter_t & filter,
			message_sink_t & subscriber ) override
			{
				insert_or_modify_subscriber(
						msg_type,
						subscriber,
						[&] {
							return local_mbox_details::subscriber_info_t{
									subscriber, filter };
						},
						[&]( local_mbox_details::subscriber_info_t & info ) {
							info.set_filter( filter );
						} );
			}

		void
		drop_delivery_filter(
			const std::type_index & msg_type,
			message_sink_t & subscriber ) noexcept override
			{
				modify_and_remove_subscriber_if_needed(
						msg_type,
						subscriber,
						[]( local_mbox_details::subscriber_info_t & info ) {
							info.drop_filter();
						} );
			}

		environment_t &
		environment() const noexcept override
			{
				return m_env;
			}

	private :
		template< typename Info_Maker, typename Info_Changer >
		void
		insert_or_modify_subscriber(
			const std::type_index & type_wrapper,
			message_sink_t & subscriber,
			Info_Maker maker,
			Info_Changer changer )
			{
				std::unique_lock< default_rw_spinlock_t > lock( m_lock );

				auto it = m_subscribers.find( type_wrapper );
				if( it == m_subscribers.end() )
				{
					// There isn't such message type yet.
					local_mbox_details::subscriber_adaptive_container_t container;
					container.insert( maker() );

					m_subscribers.emplace( type_wrapper, std::move( container ) );
				}
				else
				{
					auto & sinks = it->second;

					auto pos = sinks.find( subscriber );
					if( pos != sinks.end() )
					{
						// Agent is already in subscribers list.
						// But its state must be updated.
						changer( *pos );
					}
					else
						// There is no subscriber in the container.
						// It must be added.
						sinks.insert( maker() );
				}
			}

		template< typename Info_Changer >
		void
		modify_and_remove_subscriber_if_needed(
			const std::type_index & type_wrapper,
			message_sink_t & subscriber,
			Info_Changer changer )
			{
				std::unique_lock< default_rw_spinlock_t > lock( m_lock );

				auto it = m_subscribers.find( type_wrapper );
				if( it != m_subscribers.end() )
				{
					auto & sinks = it->second;

					auto pos = sinks.find( subscriber );
					if( pos != sinks.end() )
					{
						// Subscriber is found and must be modified.
						changer( *pos );

						// If info about subscriber becomes empty after modification
						// then subscriber info must be removed.
						if( pos->empty() )
							sinks.erase( pos );
					}

					if( sinks.empty() )
						m_subscribers.erase( it );
				}
			}

		void
		do_deliver_message_impl(
			typename Tracing_Base::deliver_op_tracer const & tracer,
			const std::type_index & msg_type,
			const message_ref_t & message,
			unsigned int overlimit_reaction_deep )
			{
				read_lock_guard_t< default_rw_spinlock_t > lock( m_lock );

				auto it = m_subscribers.find( msg_type );
				if( it != m_subscribers.end() )
					{
						for( const auto & a : it->second )
							do_deliver_message_to_subscriber(
									a,
									tracer,
									msg_type,
									message,
									overlimit_reaction_deep );
					}
				else
					tracer.no_subscribers();
			}

		void
		do_deliver_message_to_subscriber(
			const local_mbox_details::subscriber_info_t & subscriber_info,
			typename Tracing_Base::deliver_op_tracer const & tracer,
			const std::type_index & msg_type,
			const message_ref_t & message,
			unsigned int overlimit_reaction_deep ) const
			{
				const auto delivery_status =
						subscriber_info.must_be_delivered(
								message,
								[]( const message_ref_t & m ) -> message_t & {
									return *m;
								} );

				if( delivery_possibility_t::must_be_delivered == delivery_status )
					{
						//FIXME: this call to `push_to_queue` has to be moved
						//inside a message_sink.
						tracer.push_to_queue( subscriber_info.sink_pointer() );

						subscriber_info.sink_reference().push_event(
								this->m_id,
								msg_type,
								message,
								overlimit_reaction_deep,
								tracer.overlimit_tracer() );
					}
				else
					tracer.message_rejected(
							subscriber_info.sink_pointer(), delivery_status );
			}

		/*!
		 * \brief Ensures that message is an immutable message.
		 *
		 * Checks mutability flag and throws an exception if message is
		 * a mutable one.
		 *
		 * \since
		 * v.5.5.19
		 */
		void
		ensure_immutable_message(
			const std::type_index & msg_type,
			const message_ref_t & what ) const
			{
				if( message_mutability_t::immutable_message !=
						message_mutability( what ) )
					SO_5_THROW_EXCEPTION(
							so_5::rc_mutable_msg_cannot_be_delivered_via_mpmc_mbox,
							"an attempt to deliver mutable message via MPMC mbox"
							", msg_type=" + std::string(msg_type.name()) );
			}
	};

/*!
 * \since
 * v.5.5.9
 *
 * \brief Alias for local mbox without message delivery tracing.
 */
using local_mbox_without_tracing =
	local_mbox_template< msg_tracing_helpers::tracing_disabled_base >;

/*!
 * \since
 * v.5.5.9
 *
 * \brief Alias for local mbox with message delivery tracing.
 */
using local_mbox_with_tracing =
	local_mbox_template< msg_tracing_helpers::tracing_enabled_base >;

} /* namespace impl */

} /* namespace so_5 */

