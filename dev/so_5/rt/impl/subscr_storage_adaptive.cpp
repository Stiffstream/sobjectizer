/*
 * SObjectizer-5
 */

/*!
 * \since
 * v.5.5.3
 *
 * \file
 * \brief An adaptive storage for agent's subscriptions information.
 */

#include <so_5/rt/impl/h/subscription_storage_iface.hpp>

#include <algorithm>
#include <vector>

namespace so_5
{

namespace impl
{

/*!
 * \since
 * v.5.5.3
 *
 * \brief An adaptive storage for agent's subscriptions information.
 */
namespace adaptive_subscr_storage
{

/*!
 * \since
 * v.5.5.3
 *
 * \brief An adaptive storage for agent's subscriptions information.
 *
 * Uses two actual storages: one for small amount of subscriptions,
 * another for the big amount.
 *
 * Controls the size of the current storage. If size of the small storage
 * exceeded threshold then switches from small to the big one. If size of the
 * big storage drops below the threshold then switches to the small storage.
 */
class storage_t : public subscription_storage_t
	{
	public :
		storage_t(
			agent_t * owner,
			std::size_t threshold,
			subscription_storage_unique_ptr_t small_storage,
			subscription_storage_unique_ptr_t large_storage );

		virtual void
		create_event_subscription(
			const mbox_t & mbox_ref,
			const std::type_index & type_index,
			const message_limit::control_block_t * limit,
			const state_t & target_state,
			const event_handler_method_t & method,
			thread_safety_t thread_safety ) override;

		virtual void
		drop_subscription(
			const mbox_t & mbox,
			const std::type_index & msg_type,
			const state_t & target_state ) override;

		void
		drop_subscription_for_all_states(
			const mbox_t & mbox,
			const std::type_index & msg_type ) override;

		const event_handler_data_t *
		find_handler(
			mbox_id_t mbox_id,
			const std::type_index & msg_type,
			const state_t & current_state ) const SO_5_NOEXCEPT override;

		void
		debug_dump( std::ostream & to ) const override;

		void
		drop_content() override;

		subscription_storage_common::subscr_info_vector_t
		query_content() const override;

		void
		setup_content(
			subscription_storage_common::subscr_info_vector_t && info ) override;

		std::size_t
		query_subscriptions_count() const override;

	private :
		const std::size_t m_threshold;

		subscription_storage_unique_ptr_t m_small_storage;
		subscription_storage_unique_ptr_t m_large_storage;

		subscription_storage_t * m_current_storage = nullptr;

		void
		try_switch_to_smaller_storage();
	};

storage_t::storage_t(
	agent_t * owner,
	std::size_t threshold,
	subscription_storage_unique_ptr_t small_storage,
	subscription_storage_unique_ptr_t large_storage )
	:	subscription_storage_t( owner )
	,	m_threshold( threshold )
	,	m_small_storage( std::move( small_storage ) )
	,	m_large_storage( std::move( large_storage ) )
	{
		m_current_storage = m_small_storage.get();
	}

void
storage_t::create_event_subscription(
	const mbox_t & mbox,
	const std::type_index & msg_type,
	const message_limit::control_block_t * limit,
	const state_t & target_state,
	const event_handler_method_t & method,
	thread_safety_t thread_safety )
	{
		if( m_current_storage == m_small_storage.get() &&
				m_threshold <= m_current_storage->query_subscriptions_count() )
			{
				// Storage must be switched to the large one.
				// Exceptions are going out.
				// It means that exception during switching
				// to the large storage will prohibit subscription.
				m_large_storage->setup_content(
						m_small_storage->query_content() );

				m_small_storage->drop_content();

				m_current_storage = m_large_storage.get();
			}

		m_current_storage->create_event_subscription(
				mbox,
				msg_type,
				limit,
				target_state,
				method,
				thread_safety );
	}

void
storage_t::drop_subscription(
	const mbox_t & mbox,
	const std::type_index & msg_type,
	const state_t & target_state )
	{
		m_current_storage->drop_subscription( mbox, msg_type, target_state );

		try_switch_to_smaller_storage();
	}

void
storage_t::drop_subscription_for_all_states(
	const mbox_t & mbox,
	const std::type_index & msg_type )
	{
		m_current_storage->drop_subscription_for_all_states( mbox, msg_type );

		try_switch_to_smaller_storage();
	}

const event_handler_data_t *
storage_t::find_handler(
	mbox_id_t mbox_id,
	const std::type_index & msg_type,
	const state_t & current_state ) const SO_5_NOEXCEPT
	{
		return m_current_storage->find_handler(
				mbox_id,
				msg_type,
				current_state );
	}

void
storage_t::debug_dump( std::ostream & to ) const
	{
		m_current_storage->debug_dump( to );
	}

void
storage_t::drop_content()
	{
		m_current_storage->drop_content();
		m_current_storage = m_small_storage.get();
	}

subscription_storage_common::subscr_info_vector_t
storage_t::query_content() const
	{
		return m_current_storage->query_content();
	}

void
storage_t::setup_content(
	subscription_storage_common::subscr_info_vector_t && info )
	{
		auto s = info.size() <= m_threshold ?
				m_small_storage.get() : m_large_storage.get();

		s->setup_content( std::move( info ) );

		m_current_storage = s;
	}

std::size_t
storage_t::query_subscriptions_count() const
	{
		return m_current_storage->query_subscriptions_count();
	}

void
storage_t::try_switch_to_smaller_storage()
	{
		if( m_current_storage == m_large_storage.get() &&
				m_threshold >= m_large_storage->query_subscriptions_count() )
			{
				// All exceptions are ignored because
				// query_content, setup_content and drop_content
				// must provide strong exception guarantee.
				try
					{
						m_small_storage->setup_content(
								m_large_storage->query_content() );

						m_large_storage->drop_content();

						m_current_storage = m_small_storage.get();
					}
				catch( ... )
					{}
			}
	}

} /* namespace adaptive_subscr_storage */

} /* namespace impl */

SO_5_FUNC subscription_storage_factory_t
adaptive_subscription_storage_factory(
	std::size_t threshold )
	{
		return [threshold]( agent_t * owner ) {
			return impl::subscription_storage_unique_ptr_t(
					new impl::adaptive_subscr_storage::storage_t(
							owner,
							threshold,
							vector_based_subscription_storage_factory(
									threshold )( owner ),
							map_based_subscription_storage_factory()( owner ) ) );
		};
	}

SO_5_FUNC subscription_storage_factory_t
adaptive_subscription_storage_factory(
	std::size_t threshold,
	const subscription_storage_factory_t & small_storage_factory,
	const subscription_storage_factory_t & large_storage_factory )
	{
		return [=]( agent_t * owner ) {
			return impl::subscription_storage_unique_ptr_t(
					new impl::adaptive_subscr_storage::storage_t(
							owner,
							threshold,
							small_storage_factory( owner ),
							large_storage_factory( owner ) ) );
		};
	}

SO_5_FUNC subscription_storage_factory_t
default_subscription_storage_factory()
	{
		return adaptive_subscription_storage_factory( 8 );
	}

} /* namespace so_5 */

